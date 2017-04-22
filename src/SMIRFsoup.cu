/*
 ============================================================================
 Name        : SMIRFsoup.cu
 Author      : vkrishnan
 Version     :
 Copyright   : Your copyright notice
 Description : Compute sum of reciprocals using STL on CPU and Thrust on GPU
 ============================================================================
 */

#include <vector>
#include "cuda.h"


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

#include "Stitcher.hpp"
#include "SMIRFsoup.hpp"
#include "SMIRFdef.hpp"
#include "ConfigManager.hpp"



using namespace std;

volatile bool shutdown = false;
volatile bool working  = false;

void shutdown_manager(int signal){

	if( signal ==SIGINT ){

		shutdown = true;
		while(working) usleep(100 * 1000 );

	}

	else if( signal == SIGTERM ) {

		/* introduce some grace here */

		exit(0);

	}

}

void usage ()
{
	fprintf(stdout, "SMIRFsoup [options] UTC NFB UNIQ.PTS\n"
			" Given a directory DIR which is a fan beam observation, it de disperses, stitches and peasoups it.\n"
			" -h              print this help text\n"
			" -v              verbose output\n"
			" -D              dump stitched FB and return\n"
			" -T              transfer stitched filterbank to Shared memory and return"
			" -k              shared memory key for -T option"
			" -O <dir>        output directory \n"
			" -s <suffix>     output suffix ( for <utc>.xml ) \n "
			" -i <UTC>   	  input UTC \n"
			" -A <dir>        directory of Archives. \n"
			" -S <dir>        directory of Smirf. \n"
			" -l <line> 	  line number in uniq.pts file to process \n"
			" -c <file>       shortlisted candidate file to be used with the -T option."
			" -C <dir>        directory of the shortlisted candidate file."
			" -r <file>       RF birdies file \n"
			" -u <file>       uniq.pts file \n"
			" -U <dir>        uniq.pts file parent directory \n"
	);
}






int main(int argc, char **argv) {


	string smirf_base = SMIRF_BASE;
	string archives_base = ARCHIVES_BASE;

	string candidates_dir = "";
	string candidates_file = "";

	string uniq_points_dir = "";
	string uniq_points_file = "uniq.points";

	string out_dir = ".";
	string out_suffix = "";

	key_t  out_key = -1;

	string utc = "";
	string rf_birdies = "";

	vector<UniquePoint*>* unique_points = new vector<UniquePoint*>();
	vector<int>* unique_fbs = new vector<int>();
	vector<string>* string_points = new vector<string>();

	float dm_start = 0;
	float dm_end = 2000;
	float dm_tol = 1.05;
	float dm_pulse_width=40; //ms

	float acc_start         = 0;
	float acc_end           = 0;
	float acc_tol           = 1.25;
	float acc_pulse_width   = 64;

	CmdLineOptions args;
	if (!read_cmdline_options(args,argc,argv))
	    ErrorChecker::throw_error("Failed to parse command line arguments.");

	int arg = 0;

	char verbose = 1;

	bool dump_mode = false;

	bool transfer_mode = false;


	int point_num = -1;


	while ((arg = getopt(argc, argv, "A:Dhi:l:O:r:S:Tu:U:v")) != -1)
	{
		switch (arg)
		{
		case 'A':
			archives_base =  string(optarg);
			break;

		case 'D':
			cerr<< "setting to dump mode." <<endl;
			dump_mode = true;
			break;

		case 'T':
			cerr<< "setting to transfer mode." <<endl;
			transfer_mode = true;
			break;

		case 'h':
			usage ();
			return 0;

		case 'i':
			utc = string(optarg);
			break;

		case 'l':
			point_num = atoi(optarg);
			break;

		case 'O':
			out_dir= string(optarg);
			break;

		case 'r':
			rf_birdies = string(optarg);
			break;

		case 'S':
			smirf_base = string(optarg);
			break;
		case 's':
			out_suffix= string(optarg);
			break;

		case 'u':
			uniq_points_file =  string(optarg);
			break;

		case 'U':
			uniq_points_dir =  string(optarg);
			break;

		case 'k': {
			std::stringstream temp;
			temp << std::hex << string(optarg);
			temp >> out_key;
			cerr << "using key: " << temp.str() << " (" << out_key << ") " << endl;
			break;
		}

		case 'v':
			verbose++;
			break;


		default:
			usage ();
			return 0;
		}
	}

	/**
	 *  Try to load config files to know where filterbanks are. Abort if not found.
	 */

	if(ConfigManager::load_configs() == EXIT_FAILURE) {

		cerr << "Problem loading configs. Aborting." << endl;
		return EXIT_FAILURE;

	}

	/**
	 *  define shutdown hooks
	 */

	std::signal(SIGINT, shutdown_manager);


	/**
	 * Get and populate uniq points file.
	 */
	stringstream smirf_utc_dir_stream;
	smirf_utc_dir_stream << smirf_base << PATH_SEPERATOR <<utc;

	if(uniq_points_dir == "") uniq_points_dir = smirf_utc_dir_stream.str();


	stringstream abs_uniq_points_file_name;
	abs_uniq_points_file_name << uniq_points_dir<< PATH_SEPERATOR <<uniq_points_file;

	populate_unique_points(abs_uniq_points_file_name.str(),unique_points, string_points, unique_fbs,point_num);


	/**
	 * If dump or transfer mode, do and return. Do not peasoup.
	 */
	if(dump_mode || transfer_mode){

		Stitcher stitcher(utc,out_dir,verbose);

		if(dump_mode) {

			stitcher.stitch_and_dump(unique_points, unique_fbs);

		}
		if(transfer_mode) {

			if(out_key < 0 ) {
				cerr <<  " Need a valid shared memory key specify using the -k Aborting." << endl;
			}
			if(point_num >=0) {
				UniquePoint* point = unique_points->at(point_num);
				// to complete.

			}
			UniquePoint* point = unique_points->at(point_num);
			vivek::Filterbank* stitched_filterbank = stitcher.stitch(point);

			vivek::Archiver a(out_key);
			a.transfer_fil_to_DADA_buffer(stitched_filterbank);

		}
		return EXIT_SUCCESS;
	}



	/**
	 * Arguments for peasoup optimized for SMIRF.
	 */

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
	args.nharmonics        = 4;
	args.min_snr           = 9;
	args.min_freq          = 0.1 ;
	args.max_freq          = 1100;
	args.freq_tol          = 0.1;
	args.verbose           = verbose;
	args.progress_bar      = true;
	args.npdmp             = 500;
	args.limit             = 500;
	args.max_harm 		   = 4;


	/**
	 * Use the first filter bank to extract header information and create DM and Acceleration trial list that can
	 * be reused for all stitches.
	 */

	string first_fb_path = ConfigManager::get_fil_file_path(archives_base,utc,unique_fbs->at(0));

	vivek::Filterbank* ffb = new vivek::Filterbank(first_fb_path,FILREAD,verbose);

	long data_bytes = ffb->data_bytes;
	int nsamples = ffb->get_nsamps();
	double tsamp = ffb->get_tsamp();
	double cfreq = ffb->get_cfreq();
	double foff =  ffb->get_foff();
	unsigned int size = Utils::prev_power_of_two(ffb->get_nsamps());


	vector<float> acc_list;
	AccelerationPlan acc_plan(acc_start, acc_end, acc_tol, acc_pulse_width, size, tsamp, cfreq, foff);
	acc_plan.generate_accel_list(0.0,acc_list);

	vector<float> dm_list;
	Dedisperser ffb_dedisperser(*ffb,1);
	ffb_dedisperser.generate_dm_list(dm_start,dm_end,dm_pulse_width,dm_tol);
	dm_list = ffb_dedisperser.get_dm_list();

	/**
	 * Load all fanbeams to RAM.
	 */

	std::map<int,vivek::Filterbank*> fanbeams;

	for( vector<int>::iterator fb_iterator = unique_fbs->begin(); fb_iterator != unique_fbs->end(); fb_iterator++){
		int fb = (int)*(fb_iterator);

		string fb_abs_path = ConfigManager::get_fil_file_path(archives_base,utc,fb);

		vivek::Filterbank* f = new vivek::Filterbank(fb_abs_path, FILREAD, verbose);
		f->load_all_data();

		if(verbose) cerr<< "mean: " << f->get_mean() << "  rms:" << f->get_rms() <<endl;

		fanbeams[fb] = f;

	}

	std::cerr<< "loaded all FB" << fanbeams.size()<<std::endl;

	/**
	 *  Get zero DM candidates that happen on all beams and use this as a birdies list.
	 */

	CandidateCollection zero_dm_candidates = get_zero_dm_candidates(&fanbeams,args);


	/**
	 *
	 * parameters for the xml output file.
	 *
	 */

	OutputFileWriter stats;
	stats.add_misc_info();
	stats.add_search_parameters(args);
	stats.add_dm_list(dm_list);
	stats.add_acc_list(acc_list);

	stringstream xml_filename;
	xml_filename <<  out_dir << PATH_SEPERATOR <<  utc << ".xml";

	if(out_suffix !="") xml_filename<<"."<<out_suffix;

	vector<int> device_idxs;
	for (int device_idx=0;device_idx<1;device_idx++) device_idxs.push_back(device_idx);

	stats.add_gpu_info(device_idxs);
	stats.to_file(xml_filename.str());




	map<int, DispersionTrials<unsigned char> > dedispersed_series_map;

	for(vector<int>::iterator fb_iterator = unique_fbs->begin(); fb_iterator != unique_fbs->end(); fb_iterator++){
		int fb = (int)*(fb_iterator);

		vivek::Filterbank* f = fanbeams.at(fb);

		Dedisperser dedisperser(*f,1);
		dedisperser.set_dm_list(dm_list);

		PUSH_NVTX_RANGE("Dedisperse",3)
		DispersionTrials<unsigned char> trials = dedisperser.dedisperse();
		POP_NVTX_RANGE

		dedispersed_series_map.insert(map<int, DispersionTrials<unsigned char> >::value_type(fb,trials));

		delete f;


	}

	size_t max_delay = dedispersed_series_map.begin()->second.get_max_delay();
	int reduced_nsamples = nsamples - max_delay;


	int point_index=1;
	int candidate_id = 1;

	for(vector<UniquePoint*>::iterator it = unique_points->begin(); it!=unique_points->end();++it){
		UniquePoint* point = *it;

		unsigned char* data = new_and_check<unsigned char>(dm_list.size()*reduced_nsamples,"tracked data.");

		int ptr = 0;

		for(vector<Traversal*>::iterator it2 = point->traversals->begin(); it2!=point->traversals->end(); it2++){
			Traversal* traversal = *it2;

			int startSample = traversal->startSample;

			size_t num = (startSample+traversal->numSamples > (reduced_nsamples)) ? (reduced_nsamples - startSample) : traversal->numSamples;

			DispersionTrials<unsigned char> dedispTimeseries4FB = dedispersed_series_map.find(traversal->fanbeam)->second;

			int trialIndex = 0;

			for( int trial = 0; trial < dm_list.size(); trial++){

				DedispersedTimeSeries<unsigned char> trialTimeSeries = dedispTimeseries4FB[trial];

				unsigned char* trial_data = trialTimeSeries.get_data();

				memcpy(&data[trialIndex + ptr],&trial_data[startSample],sizeof(unsigned char)*num);

				trialIndex+= reduced_nsamples;

			}

			ptr+=num;
			if(ptr >= reduced_nsamples) break;
		}

		DispersionTrials<unsigned char> trials = DispersionTrials<unsigned char>(data,nsamples,tsamp, dm_list,max_delay);

		candidate_id += peasoup_multi(ffb,args,trials,stats,xml_filename.str(),acc_plan,point_index, point->ra, point->dec, candidate_id, out_dir );


		delete[] data;
		point_index++;


	}
}




int peasoup_multi(vivek::Filterbank* fil,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, OutputFileWriter& stats,
		string xml_filename, AccelerationPlan& acc_plan, int pt_num, string pt_ra, string pt_dec, int candidate_id, string out_dir){

	CandidateCollection dm_cands  = peasoup(fil,args,trials,acc_plan);

	string name = get_candidate_file_name(out_dir,pt_num);

	stats.add_candidates(dm_cands.cands,pt_num,pt_ra,pt_dec);

	FILE* fp = fopen(name.c_str(),"w");



	dm_cands.print_cand_file(fp,pt_ra.c_str(),pt_dec.c_str(), candidate_id);

	fclose(fp);

	stats.to_file(xml_filename);

	return dm_cands.cands.size();


}

CandidateCollection peasoup(vivek::Filterbank* fil,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, AccelerationPlan& acc_plan) {

	CandidateCollection dm_cands;

	int nthreads = 1;

	unsigned int size = ( args.size==0 )? Utils::prev_power_of_two(fil->get_nsamps()): args.size;

	DMDispenser dispenser(trials);

	Worker* worker = new Worker(trials,dispenser,acc_plan,args,size,0);
	worker->start();
	dm_cands.append(worker->dm_trial_cands.cands);


	DMDistiller dm_still(args.freq_tol,true);
	dm_cands.cands = dm_still.distill(dm_cands.cands);

	HarmonicDistiller harm_still(args.freq_tol,args.max_harm,true,false);
	dm_cands.cands = harm_still.distill(dm_cands.cands);

	CandidateScorer cand_scorer(fil->get_tsamp(),fil->get_cfreq(), fil->get_foff(), fabs(fil->get_foff())*fil->get_nchans());
	cand_scorer.score_all(dm_cands.cands);

	MultiFolder folder(dm_cands.cands,trials);

	if(args.npdmp > 0 ) {
		folder.fold_n(args.npdmp);
	}

	int new_size = min(args.limit,(int) dm_cands.cands.size());
	dm_cands.cands.resize(new_size);

	delete worker;

	return dm_cands;



}

CandidateCollection get_zero_dm_candidates(map<int,vivek::Filterbank*>* fanbeams, CmdLineOptions& args){

	vivek::Filterbank* ffb = fanbeams->begin()->second;

	long data_bytes = ffb->data_bytes;
	int nsamples = ffb->get_nsamps();
	double tsamp = ffb->get_tsamp();
	double cfreq = ffb->get_cfreq();
	double foff =  ffb->get_foff();
	unsigned int size = Utils::prev_power_of_two(ffb->get_nsamps());

	vector<float> zero_dm_list;
	zero_dm_list.push_back(0.0);

	vector<float> zero_acc_list;
	AccelerationPlan zero_dm_acc_plan(0, 0, 0, 0, size, tsamp, cfreq, foff);
	zero_dm_acc_plan.generate_accel_list(0.0,zero_acc_list);

	CandidateCollection all_cands;

	for (std::map<int,vivek::Filterbank*>::iterator it=fanbeams->begin(); it!=fanbeams->end(); ++it){

		vivek::Filterbank* f  = it->second;

		Dedisperser zero_dm_dedisperser(*f,1);
		zero_dm_dedisperser.set_dm_list(zero_dm_list);

		PUSH_NVTX_RANGE("Dedisperse",3)

		DispersionTrials<unsigned char> trials = zero_dm_dedisperser.dedisperse();

		POP_NVTX_RANGE

		CandidateCollection dm_cands  = peasoup(f,args,trials,zero_dm_acc_plan);
		all_cands.append(dm_cands.cands);

	}
	return all_cands;

}

void populate_unique_points(std::string abs_file_name, std::vector<UniquePoint*>* unique_points,std::vector<std::string>* str_points,  std::vector<int>* unique_fbs, int point_index ){

	std::string line;
	std::ifstream unique_points_file_stream(abs_file_name.c_str());
	int line_number = 0;
	if(unique_points_file_stream.is_open()){
		while(getline(unique_points_file_stream, line)){
			line_number++;
			if(point_index != -1  && point_index != line_number) continue;

			str_points->push_back(line);
			UniquePoint* point = new UniquePoint();
			std::vector<std::string> vstrings = split(line,' ');

			point->ra = vstrings.at(POINT_RA);
			point->num = line_number -1;
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



int transfer_to_shared_memory(void* ptr){
	vivek::Filterbank* stitched_filterbank = reinterpret_cast<vivek::Filterbank*>(ptr);
	vivek::Archiver* a = new vivek::Archiver();
	a->transfer_fil_to_DADA_buffer(stitched_filterbank);
	return EXIT_SUCCESS;

}

void* launch_worker_thread(void* ptr){
	reinterpret_cast<Worker*>(ptr)->start();
	return NULL;
}


void Worker::start(void)
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
		ii = manager.get_dm_trial_idx();

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

























//int peasoup_multi2(vivek::Filterbank* filobj,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, OutputFileWriter& stats,
//		string xml_filename, AccelerationPlan& acc_plan, int pt_num, string pt_ra, string pt_dec){
//	map<string,Stopwatch> timers;
//
//
//	string birdiefile = "";
//
//	int nthreads = 1;
//
//	unsigned int size;
//	if (args.size==0) size = Utils::prev_power_of_two(filobj->get_nsamps());
//	else size = args.size;
//	if (args.verbose)
//		cout << "Setting transform length to " << size << " points" << endl;
//
//
//	//Multithreading commands
//	vector<Worker*> workers(nthreads);
//	vector<pthread_t> threads(nthreads);
//	cerr<< "dispensing trials"<<endl;
//	DMDispenser dispenser(trials);
//	if (args.progress_bar)
//		dispenser.enable_progress_bar();
//	cerr<< "starting  workers"<<endl;
//	for (int ii=0;ii<nthreads;ii++){
//		workers[ii] = (new Worker(trials,dispenser,acc_plan,args,size,ii));
//		pthread_create(&threads[ii], NULL, launch_worker_thread, (void*) workers[ii]);
//	}
//	//	Worker* worker = new Worker(trials,dispenser,acc_plan,args,size,0);
//	//	worker->start();
//
//	DMDistiller dm_still(args.freq_tol,true);
//	HarmonicDistiller harm_still(args.freq_tol,args.max_harm,true,false);
//	CandidateCollection dm_cands;
//	for (int ii=0; ii<nthreads; ii++){
//		pthread_join(threads[ii],NULL);
//		dm_cands.append(workers[ii]->dm_trial_cands.cands);
//	}
//	//dm_cands.append(worker->dm_trial_cands.cands);
//
//	if (args.verbose)
//		cout << "Distilling DMs" << endl;
//
//
//	dm_cands.cands = dm_still.distill(dm_cands.cands);
//	dm_cands.cands = harm_still.distill(dm_cands.cands);
//
//	CandidateScorer cand_scorer(filobj->get_tsamp(),filobj->get_cfreq(), filobj->get_foff(),
//			fabs(filobj->get_foff())*filobj->get_nchans());
//	cand_scorer.score_all(dm_cands.cands);
//
//	if (args.verbose)
//		cout << "Setting up time series folder" << endl;
//
//	MultiFolder folder(dm_cands.cands,trials);
//	if (args.progress_bar)
//		folder.enable_progress_bar();
//
//	if (args.npdmp > 0){
//		if (args.verbose)
//			cout << "Folding top "<< args.npdmp <<" cands" << endl;
//		folder.fold_n(args.npdmp);
//	}
//
//	if (args.verbose)
//		cout << "Writing output files" << endl;
//	//dm_cands.write_candidate_file("./old_cands.txt");
//
//	cerr << "num candidates:" << dm_cands.cands.size() << endl;
//
//	int new_size = min(args.limit,(int) dm_cands.cands.size());
//	dm_cands.cands.resize(new_size);
//
//	stringstream name_stream;
//	name_stream <<pt_ra << pt_dec << ".cand";
//	string out = name_stream.str();
//
//	stats.add_candidates(dm_cands.cands,pt_num,pt_ra,pt_dec);
//
//	FILE* fp = fopen(out.c_str(),"w");
//
//	fprintf(fp,"# RA: %s DEC: %s \n",pt_ra.c_str(),pt_dec.c_str());
//
//
//	dm_cands.print_cand_file(fp,pt_ra.c_str(),pt_dec.c_str(), 0);
//	fclose(fp);
//
//	//stats.add_timing_info(timers);
//
//	stats.to_file(xml_filename);
//	for (vector< Worker* >::iterator it = workers.begin() ; it != workers.end(); ++it) delete (*it);
//	workers.clear();
//	//delete worker;
//	return 0;
//}
//
