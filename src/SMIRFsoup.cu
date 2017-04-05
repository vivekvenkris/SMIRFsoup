/*
 ============================================================================
 Name        : SMIRFsoup.cu
 Author      : vkrishnan
 Version     :
 Copyright   : Your copyright notice
 Description : Compute sum of reciprocals using STL on CPU and Thrust on GPU
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <sstream>
#include <cstddef>
#include <iomanip>
#include <map>
#include <set>
#include <cmath>
#include <transforms/dedisperser.hpp>
#include <data_types/timeseries.hpp>
#include <data_types/fourierseries.hpp>
#include <data_types/candidates.hpp>
#include <data_types/filterbank.hpp>
#include <transforms/dedisperser.hpp>
#include <transforms/resampler.hpp>
#include <transforms/folder.hpp>
#include <transforms/ffter.hpp>
#include <transforms/dereddener.hpp>
#include <transforms/spectrumformer.hpp>
#include <transforms/birdiezapper.hpp>
#include <transforms/peakfinder.hpp>
#include <transforms/distiller.hpp>
#include <transforms/harmonicfolder.hpp>
#include <transforms/scorer.hpp>
#include <utils/exceptions.hpp>
#include <utils/utils.hpp>
#include <utils/stats.hpp>
#include <utils/stopwatch.hpp>
#include <utils/cmdline.hpp>
#include <utils/output_stats.hpp>
#include "SMIRFsoup.hpp"
#include "vivek/filterbank_def.hpp"
#include "vivek/filutil.hpp"
#include "vivek/utilities.hpp"
#include "DMDispenser.hpp"
#include "cuda.h"
#include "vivek/Archiver.h"

using namespace std;

int peasoup_multi(vivek::Filterbank* filobj,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, CandidateFileWriter& cand_files,OutputFileWriter& stats,
		std::string cand_filename, std::string xml_filename, AccelerationPlan& acc_plan, int pt_num,std::string pt_ra, std::string pt_dec);

void prepare_peasoup_out_files( CandidateFileWriter& cand_files,OutputFileWriter& stats,CmdLineOptions args, std::vector<float> dm_list,
		std::vector<float> acc_list);

class Worker {
private:
	DispersionTrials<unsigned char>& trials;
	DMDispenser& manager;
	CmdLineOptions& args;
	AccelerationPlan& acc_plan;
	unsigned int size;
	int device;
	std::map<std::string,Stopwatch> timers;

public:
	CandidateCollection dm_trial_cands;

	Worker(DispersionTrials<unsigned char>& trials, DMDispenser& manager,
			AccelerationPlan& acc_plan, CmdLineOptions& args, unsigned int size, int device)
	:trials(trials),manager(manager),acc_plan(acc_plan),args(args),size(size),device(device){}

	void start(void)
	{

		cudaSetDevice(device);
		Stopwatch pass_timer;
		pass_timer.start();

		bool padding = false;
		if (size > trials.get_nsamps())
			padding = true;

		CuFFTerR2C r2cfft(size);
		CuFFTerC2R c2rfft(size);
		float tobs = size*trials.get_tsamp();
		float bin_width = 1.0/tobs;
		DeviceFourierSeries<cufftComplex> d_fseries(size/2+1,bin_width);
		DedispersedTimeSeries<unsigned char> tim;
		ReusableDeviceTimeSeries<float,unsigned char> d_tim(size);
		DeviceTimeSeries<float> d_tim_r(size);
		TimeDomainResampler resampler;
		DevicePowerSpectrum<float> pspec(d_fseries);
		Zapper* bzap;
		if (args.zapfilename!=""){
			if (args.verbose)
				std::cout << "Using zapfile: " << args.zapfilename << std::endl;
			bzap = new Zapper(args.zapfilename);
		}
		Dereddener rednoise(size/2+1);
		SpectrumFormer former;
		PeakFinder cand_finder(args.min_snr,args.min_freq,args.max_freq,size);
		HarmonicSums<float> sums(pspec,args.nharmonics);
		HarmonicFolder harm_folder(sums);
		std::vector<float> acc_list;
		HarmonicDistiller harm_finder(args.freq_tol,args.max_harm,false);
		AccelerationDistiller acc_still(tobs,args.freq_tol,true);
		float mean,std,rms;
		float padding_mean;
		int ii;

		PUSH_NVTX_RANGE("DM-Loop",0)
		while (true){
			//timers["get_trial_dm"].start();
			ii = manager.get_dm_trial_idx();
			//timers["get_trial_dm"].stop();

			if (ii==-1)
				break;
			trials.get_idx(ii,tim);

			if (args.verbose)
				std::cout << "Copying DM trial to device (DM: " << tim.get_dm() << ")"<< std::endl;

			d_tim.copy_from_host(tim);

			//timers["rednoise"].start()
			if (padding){
				padding_mean = stats::mean<float>(d_tim.get_data(),trials.get_nsamps());
				d_tim.fill(trials.get_nsamps(),d_tim.get_nsamps(),padding_mean);
			}

			if (args.verbose)
				std::cout << "Generating accelration list" << std::endl;
			acc_plan.generate_accel_list(tim.get_dm(),acc_list);

			if (args.verbose)
				std::cout << "Searching "<< acc_list.size()<< " acceleration trials for DM "<< tim.get_dm() << std::endl;

			if (args.verbose)
				std::cout << "Executing forward FFT" << std::endl;
			r2cfft.execute(d_tim.get_data(),d_fseries.get_data());

			if (args.verbose)
				std::cout << "Forming power spectrum" << std::endl;
			former.form(d_fseries,pspec);

			if (args.verbose)
				std::cout << "Finding running median" << std::endl;
			rednoise.calculate_median(pspec);

			if (args.verbose)
				std::cout << "Dereddening Fourier series" << std::endl;
			rednoise.deredden(d_fseries);

			if (args.zapfilename!=""){
				if (args.verbose)
					std::cout << "Zapping birdies" << std::endl;
				bzap->zap(d_fseries);
			}

			if (args.verbose)
				std::cout << "Forming interpolated power spectrum" << std::endl;
			former.form_interpolated(d_fseries,pspec);

			if (args.verbose)
				std::cout << "Finding statistics" << std::endl;
			stats::stats<float>(pspec.get_data(),size/2+1,&mean,&rms,&std);

			if (args.verbose)
				std::cout << "Executing inverse FFT" << std::endl;
			c2rfft.execute(d_fseries.get_data(),d_tim.get_data());

			CandidateCollection accel_trial_cands;
			PUSH_NVTX_RANGE("Acceleration-Loop",1)

			for (int jj=0;jj<acc_list.size();jj++){
				if (args.verbose)
					std::cout << "Resampling to "<< acc_list[jj] << " m/s/s" << std::endl;
				resampler.resampleII(d_tim,d_tim_r,size,acc_list[jj]);

				if (args.verbose)
					std::cout << "Execute forward FFT" << std::endl;
				r2cfft.execute(d_tim_r.get_data(),d_fseries.get_data());

				if (args.verbose)
					std::cout << "Form interpolated power spectrum" << std::endl;
				former.form_interpolated(d_fseries,pspec);

				if (args.verbose)
					std::cout << "Normalise power spectrum" << std::endl;
				stats::normalise(pspec.get_data(),mean*size,std*size,size/2+1);

				if (args.verbose)
					std::cout << "Harmonic summing" << std::endl;
				harm_folder.fold(pspec);

				if (args.verbose)
					std::cout << "Finding peaks" << std::endl;
				SpectrumCandidates trial_cands(tim.get_dm(),ii,acc_list[jj]);
				if (args.verbose)
						std::cerr << "SpectrumCandidates" << std::endl;
				cand_finder.find_candidates(pspec,trial_cands);
				if (args.verbose)
					std::cerr << "after pspec" << sums.size() << std::endl;
				cand_finder.find_candidates(sums,trial_cands);

				if (args.verbose)
					std::cout << "Distilling harmonics" << std::endl;
				accel_trial_cands.append(harm_finder.distill(trial_cands.cands));
			}
			POP_NVTX_RANGE
			if (args.verbose)
				std::cout << "Distilling accelerations" << std::endl;
			dm_trial_cands.append(acc_still.distill(accel_trial_cands.cands));
		}
		POP_NVTX_RANGE

		if (args.zapfilename!="")
			delete bzap;

		if (args.verbose)
			std::cout << "DM processing took " << pass_timer.getTime() << " seconds"<< std::endl;
	}

};

void* launch_worker_thread(void* ptr){
	reinterpret_cast<Worker*>(ptr)->start();
	return NULL;
}

void populate_unique_points(std::string abs_file_name, std::vector<UniquePoint*>* unique_points,std::vector<std::string>* strPoints,  std::vector<int>* unique_fbs, int point_index ){

	std::string line;
	std::ifstream unique_points_file_stream(abs_file_name.c_str());
	int line_number = 0;
	if(unique_points_file_stream.is_open()){
		while(getline(unique_points_file_stream, line)){
			line_number++;
			if(point_index != -1  && point_index != line_number) continue;

			strPoints->push_back(line);
			UniquePoint* point = new UniquePoint();
			std::vector<std::string> vstrings = split(line,' ');

			point->ra = vstrings.at(POINT_RA);
			point->dec = vstrings.at(POINT_DEC);

			point->startFanbeam =atof(vstrings.at(POINT_START_FANBEAM).c_str());
			point->endFanbeam = atof(vstrings.at(POINT_END_FANBEAM).c_str());

			point->startNS =atof(vstrings.at(POINT_START_NS).c_str());
			point->endNS = atof(vstrings.at(POINT_END_NS).c_str());


			for(std::vector<std::string>::size_type i = TRAVERSAL_START_INDEX ; i != vstrings.size(); i = i + TRAVERSAL_SIZE) {

				std::string value = vstrings[i];
				Traversal* t = new Traversal(&vstrings[i]);
				point->traversals->push_back(t);
				if(std::find(unique_fbs->begin(), unique_fbs->end(),(int)t->fanbeam)== unique_fbs->end()) unique_fbs->push_back((int)t->fanbeam);
			}
			unique_points->push_back(point);
		}
		unique_points_file_stream.close();
	}

}


void usage ()
{
	fprintf(stdout, "SMIRFsoup [options] UTC NFB UNIQ.PTS\n"
			" Given a directory DIR which is a fan beam observation, it de disperses, stitches and peasoups it.\n"
			" dir             directory with the sturucture UTC/FB/BEAM_???/UTC.fil \n"
			" NFB			  number of fanbeams \n"
			" FILE            uniq.points file name"
			" -h              print this help text\n"
			" -v              verbose output\n"
			" -D              dump stitched FB and return\n"
			" -l point_num	  dump only this point\n"
			" -O              output directory \n"
			" -S              output suffix ( for <utc>.xml and <utc>.peasoup \n "
			" -i 			  input UTC"
	);
}






int main(int argc, char **argv) {

	std::string archives_dir = ARCHIVES_DIR;
	std::string uniq_points_dir = "";
	std::string out_dir = ".";
	std::string out_suffix = "";

	std::string utc = "";
	std::string uniq_points_file_name = "uniq.points";
	std::string rf_birdies = "";

	std::vector<UniquePoint*>* unique_points = new std::vector<UniquePoint*>();
	std::vector<int>* unique_fbs = new std::vector<int>();
	std::vector<std::string>* string_points = new std::vector<std::string>();

	float dm_start = 0;
	float dm_end = 2000;
	float dm_tol = 1.05;
	float dm_pulse_width=40; //ms

	float acc_start         = 0;
	float acc_end           = 0;
	float acc_tol           = 1.25;
	float acc_pulse_width   = 64;



	int arg = 0;

	char verbose = 1;

	char* device = "/xs";

	bool dump_mode = false;


	int point_num = -1;


	while ((arg = getopt(argc, argv, "A:Dhi:l:O:r:S:u:U:v")) != -1)
	{
		switch (arg)
		{
		case 'A':
			archives_dir =  std::string(optarg);
			break;

		case 'D':
			std::cerr<< "setting to dump mode." <<std::endl;
			dump_mode = true;
			break;

		case 'h':
			usage ();
			return 0;

		case 'i':
			utc = std::string(optarg);
			break;

		case 'l':
			point_num = std::atoi(optarg);
			break;

		case 'O':
			out_dir= std::string(optarg);
			break;

		case 'r':
			rf_birdies = std::string(optarg);
			break;

		case 'S':
			out_suffix= std::string(optarg);
			break;

		case 'u':
			uniq_points_file_name =  std::string(optarg);
			break;

		case 'U':
			uniq_points_dir =  std::string(optarg);
			break;

		case 'v':
			verbose++;
			break;


		default:
			usage ();
			return 0;
		}
	}

	CmdLineOptions args;

	args.killfilename      = "";
	args.zapfilename = "";
	args.max_num_threads   = 1;
	args.size              =  0;
	args.acc_start = acc_start;
	args.acc_end = acc_end;
	args.acc_tol = acc_tol;
	args.acc_pulse_width = acc_pulse_width;
	args.dm_start = dm_start;
	args.dm_end = dm_end;
	args.dm_tol = dm_tol;
	args.dm_pulse_width = dm_pulse_width;
	args.boundary_5_freq   = 0.05;
	args.boundary_25_freq  = 0.5;
	args.nharmonics        = 8;
	args.min_snr           = 6;
	args.min_freq          = 0.1 ;
	args.max_freq          = 1100;
	args.freq_tol          = 0.1;
	args.verbose           = false;
	args.progress_bar      = true;
	args.npdmp             = 500;
	args.limit             = 500;
	args.max_harm 		   = 8;


	std::stringstream utc_dir;
	utc_dir << archives_dir << PATH_SEPERATOR <<utc;

	if(uniq_points_dir == "") uniq_points_dir = utc_dir.str();

	std::stringstream abs_uniq_points_file_name;
	abs_uniq_points_file_name << uniq_points_dir<<"/"<<uniq_points_file_name;

	std::stringstream fil_name;
	fil_name<<utc<<".fil";

	std::stringstream cfb_abs_path;
	cfb_abs_path << utc_dir.str() << PATH_SEPERATOR << FB_DIR << PATH_SEPERATOR<<
			BEAM_DIR_PREFIX <<std::setfill('0') << std::setw(3) <<CENTRAL_BEAM_NUM <<PATH_SEPERATOR<<fil_name.str();
	vivek::Filterbank* cfb = new vivek::Filterbank(cfb_abs_path.str(),FILREAD,verbose);

	populate_unique_points(abs_uniq_points_file_name.str(),unique_points, string_points, unique_fbs,point_num);


	if(dump_mode){
		stitch_and_dump(utc_dir.str(),fil_name.str(),cfb,unique_points, unique_fbs,out_dir,verbose);
		return 0;
	}

	long data_bytes = cfb->data_bytes;
	int nsamples = cfb->get_nsamps();
	double tsamp = cfb->get_tsamp();
	double cfreq = cfb->get_cfreq();
	double foff =  cfb->get_foff();
	unsigned int size = Utils::prev_power_of_two(cfb->get_nsamps());

	std::vector<float> acc_list;
	AccelerationPlan acc_plan(acc_start, acc_end, acc_tol, acc_pulse_width, size, tsamp, cfreq, foff);
	acc_plan.generate_accel_list(0.0,acc_list);

	std::vector<float> dm_list;
	Dedisperser cfb_dedisperser(*cfb,1);
	cfb_dedisperser.generate_dm_list(dm_start,dm_end,dm_pulse_width,dm_tol);
	dm_list = cfb_dedisperser.get_dm_list();


	CandidateFileWriter cand_files(".");
	OutputFileWriter stats;
	stats.add_misc_info();
	stats.add_search_parameters(args);
	stats.add_dm_list(dm_list);
	stats.add_acc_list(acc_list);


	std::stringstream cand_filename;
	cand_filename <<  out_dir << PATH_SEPERATOR <<  utc << ".peasoup";
	if(out_suffix !="") cand_filename<<"."<<out_suffix;

	std::stringstream xml_filename;
	xml_filename <<  out_dir << PATH_SEPERATOR <<  utc << ".xml";
	if(out_suffix !="") xml_filename<<"."<<out_suffix;

	std::vector<int> device_idxs;
	for (int device_idx=0;device_idx<1;device_idx++) device_idxs.push_back(device_idx);
	stats.add_gpu_info(device_idxs);
	stats.to_file(xml_filename.str());

	std::map<int, DispersionTrials<unsigned char> > dedispersed_series_map;
	std::vector<int>::iterator fb_iterator;
	for(fb_iterator = unique_fbs->begin(); fb_iterator != unique_fbs->end(); fb_iterator++){
		int fb = (int)*(fb_iterator);

		std::stringstream fb_abs_path;
		fb_abs_path << utc_dir.str() << PATH_SEPERATOR<< FB_DIR<< PATH_SEPERATOR <<
				BEAM_DIR_PREFIX <<std::setfill('0') << std::setw(3) <<fb <<PATH_SEPERATOR<<fil_name.str();

		vivek::Filterbank* f = new vivek::Filterbank(fb_abs_path.str(),FILREAD,verbose);
		f->load_all_data();
		std::cerr<< "mean: " << f->get_mean() << "  rms:" << f->get_rms() <<std::endl;
		Dedisperser dedisperser(*f,1);
		//dedisperser.set_killmask(rf_birdies);
		dedisperser.set_dm_list(dm_list);
		PUSH_NVTX_RANGE("Dedisperse",3)
		DispersionTrials<unsigned char> trials = dedisperser.dedisperse();

		dedispersed_series_map.insert(std::map<int, DispersionTrials<unsigned char> >::value_type(fb,trials));

		POP_NVTX_RANGE

		delete f;


	}
	int numDMTrials = dm_list.size();
	size_t max_delay = dedispersed_series_map.begin()->second.get_max_delay();

	int reduced_nsamples = nsamples - max_delay;
	int point_index=1;

	for(std::vector<UniquePoint*>::iterator it = unique_points->begin(); it!=unique_points->end();++it){
		UniquePoint* point = *it;
		unsigned char* data = new_and_check<unsigned char>(dm_list.size()*reduced_nsamples,"tracked data.");
		int ptr = 0;
		for(std::vector<Traversal*>::iterator it2 = point->traversals->begin(); it2!=point->traversals->end(); it2++){
			Traversal* traversal = *it2;

			int startSample = traversal->startSample;// * reduced_nsamples / (nsamples+0.0));
			size_t num = (startSample+traversal->numSamples > (reduced_nsamples)) ? (reduced_nsamples - startSample) : traversal->numSamples;

			DispersionTrials<unsigned char> dedispTimeseries4FB = dedispersed_series_map.find(traversal->fanbeam)->second;
			int trialIndex = 0;
			for( int trial = 0; trial < numDMTrials; trial++){
				DedispersedTimeSeries<unsigned char> trialTimeSeries = dedispTimeseries4FB[trial];
				unsigned char* trial_data = trialTimeSeries.get_data();
				std::memcpy(&data[trialIndex + ptr],&trial_data[startSample],sizeof(unsigned char)*num);
				trialIndex+= reduced_nsamples;
			}

			ptr+=num;
			if(ptr >= reduced_nsamples) break;
		}

		DispersionTrials<unsigned char> trials = DispersionTrials<unsigned char>(data,nsamples,tsamp, dm_list,max_delay);

		peasoup_multi(cfb,args,trials,cand_files,stats,cand_filename.str(),xml_filename.str(),acc_plan,point_index, point->ra, point->dec );


		delete[] data;
		point_index++;


	}
}


int peasoup_multi(vivek::Filterbank* filobj,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, CandidateFileWriter& cand_files,OutputFileWriter& stats,
		std::string cand_filename, std::string xml_filename, AccelerationPlan& acc_plan, int pt_num, std::string pt_ra, std::string pt_dec){
	std::map<std::string,Stopwatch> timers;
	timers["reading"]      = Stopwatch();
	timers["dedispersion"] = Stopwatch();
	timers["searching"]    = Stopwatch();
	timers["folding"]      = Stopwatch();
	timers["total"]        = Stopwatch();
	timers["total"].start();

	std::string birdiefile = "";

	int nthreads = 1;

	unsigned int size;
	if (args.size==0) size = Utils::prev_power_of_two(filobj->get_nsamps());
	else size = args.size;
	if (args.verbose)
		std::cout << "Setting transform length to " << size << " points" << std::endl;


	//Multithreading commands
	timers["searching"].start();
	std::vector<Worker*> workers(nthreads);
	std::vector<pthread_t> threads(nthreads);
	std::cerr<< "dispensing trials"<<std::endl;
	DMDispenser dispenser(trials);
	if (args.progress_bar)
		dispenser.enable_progress_bar();
	std::cerr<< "starting  workers"<<std::endl;
	for (int ii=0;ii<nthreads;ii++){
		workers[ii] = (new Worker(trials,dispenser,acc_plan,args,size,ii));
		pthread_create(&threads[ii], NULL, launch_worker_thread, (void*) workers[ii]);
	}
//	Worker* worker = new Worker(trials,dispenser,acc_plan,args,size,0);
//	worker->start();

	DMDistiller dm_still(args.freq_tol,true);
	HarmonicDistiller harm_still(args.freq_tol,args.max_harm,true,false);
	CandidateCollection dm_cands;
	for (int ii=0; ii<nthreads; ii++){
		pthread_join(threads[ii],NULL);
		dm_cands.append(workers[ii]->dm_trial_cands.cands);
	}
	//dm_cands.append(worker->dm_trial_cands.cands);
	timers["searching"].stop();

	if (args.verbose)
		std::cout << "Distilling DMs" << std::endl;
	dm_cands.cands = dm_still.distill(dm_cands.cands);
	dm_cands.cands = harm_still.distill(dm_cands.cands);

	CandidateScorer cand_scorer(filobj->get_tsamp(),filobj->get_cfreq(), filobj->get_foff(),
			fabs(filobj->get_foff())*filobj->get_nchans());
	cand_scorer.score_all(dm_cands.cands);

	if (args.verbose)
		std::cout << "Setting up time series folder" << std::endl;

	MultiFolder folder(dm_cands.cands,trials);
	timers["folding"].start();
	if (args.progress_bar)
		folder.enable_progress_bar();

	if (args.npdmp > 0){
		if (args.verbose)
			std::cout << "Folding top "<< args.npdmp <<" cands" << std::endl;
		folder.fold_n(args.npdmp);
	}
	timers["folding"].stop();

	if (args.verbose)
		std::cout << "Writing output files" << std::endl;
	//dm_cands.write_candidate_file("./old_cands.txt");

	std::cerr << "num candidates:" << dm_cands.cands.size() << std::endl;

	int new_size = std::min(args.limit,(int) dm_cands.cands.size());
	dm_cands.cands.resize(new_size);

	//cand_files.write_binary(dm_cands.cands,cand_filename);
	stats.add_candidates(dm_cands.cands,cand_files.byte_mapping,pt_num,pt_ra,pt_dec);
	FILE* fp = fopen(cand_filename.c_str(),"w+");
	dm_cands.print(fp);

	timers["total"].stop();
	//stats.add_timing_info(timers);

	stats.to_file(xml_filename);
	for (std::vector< Worker* >::iterator it = workers.begin() ; it != workers.end(); ++it) delete (*it);
	workers.clear();
	//delete worker;
	return 0;
}

void prepare_peasoup_out_files( CandidateFileWriter& cand_files,OutputFileWriter& stats,CmdLineOptions args, std::vector<float> dm_list, std::vector<float> acc_list ){


	stats.add_misc_info();
	stats.add_search_parameters(args);
	stats.add_dm_list(dm_list);
	stats.add_acc_list(acc_list);


	std::vector<int> device_idxs;
	for (int device_idx=0;device_idx<1;device_idx++) device_idxs.push_back(device_idx);
	stats.add_gpu_info(device_idxs);
}
