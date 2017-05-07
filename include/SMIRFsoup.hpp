#ifndef SMIRFSOUP_H
#define SMIRFSOUP_H

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
#include <csignal>

#include <data_types/timeseries.hpp>
#include <data_types/fourierseries.hpp>
#include <data_types/candidates.hpp>
#include <data_types/filterbank.hpp>
#include <data_types/candidates.hpp>

#include <utils/exceptions.hpp>
#include <utils/utils.hpp>
#include <utils/stats.hpp>
#include <utils/stopwatch.hpp>
#include <utils/cmdline.hpp>
#include <utils/output_stats.hpp>

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


#include <vivek/filterbank_def.hpp>
#include <vivek/filutil.hpp>
#include <vivek/utilities.hpp>
#include <vivek/Archiver.h>

#include "DMDispenser.hpp"
#include "Coincidencer.hpp"
#include "ascii_header.h"
#include "dada_def.h"
#include "futils.h"
#include "strutil.h"

volatile int max_fanbeam_traversal = 0;

volatile int freq_bin_width = 3; //(in 1/ (size* tsamp) units)

class Peasoup{
private:
	vivek::Filterbank& sample_fil;
	CmdLineOptions& args;
	//DispersionTrials<unsigned char>& trials;
	AccelerationPlan& acc_plan;
	Zapper* bzap;
	UniquePoint* point;
	CandidateCollection& all_cands;
	unsigned char* data;
	std::vector<float>& dm_list;
	unsigned int reduced_nsamples;
public:
//	Peasoup(vivek::Filterbank& sample_fil, CmdLineOptions& args, DispersionTrials<unsigned char>& trials,
//	AccelerationPlan& acc_plan, Zapper* bzap, UniquePoint* point, CandidateCollection& all_cands): sample_fil(sample_fil),args(args),trials(trials),acc_plan(acc_plan) , bzap(bzap), point(point),all_cands(all_cands){}

	Peasoup(vivek::Filterbank& sample_fil, CmdLineOptions& args, std::vector<float> dm_list,	unsigned int reduced_nsamples,
		AccelerationPlan& acc_plan, Zapper* bzap, UniquePoint* point, CandidateCollection& all_cands, size_t max_delay): sample_fil(sample_fil),args(args),dm_list(dm_list),
				reduced_nsamples(reduced_nsamples),acc_plan(acc_plan) , bzap(bzap), point(point),all_cands(all_cands),
				data (new_and_check<unsigned char>(dm_list.size()*reduced_nsamples,"tracked data.")) {


	}

	static void* peasoup_thread(void* ptr);

	void do_peasoup();

	virtual ~Peasoup(){
		delete[] data;
	}

	unsigned char* getData() const {
		return data;
	}

	unsigned char* get_data() {
		return data;
	}


};

class Worker {
private:
	DispersionTrials<unsigned char>& trials;
	DMDispenser& manager;
	CmdLineOptions& args;
	AccelerationPlan& acc_plan;
	Zapper* bzap;
	UniquePoint* point;
	unsigned int size;
	std::map<std::string,Stopwatch> timers;

public:
	CandidateCollection dm_trial_cands;

	Worker(DispersionTrials<unsigned char>& trials, DMDispenser& manager,
			AccelerationPlan& acc_plan, CmdLineOptions& args, unsigned int size, Zapper* bzap, UniquePoint* point)
	:trials(trials),manager(manager),acc_plan(acc_plan),args(args),size(size),bzap(bzap),point(point){
	}

	void start(void);
};


void stitch_1D(std::map<int, DispersionTrials<unsigned char> >& dedispersed_series_map, UniquePoint* point,
		unsigned int reduced_nsamples, std::vector<float>& dm_list, unsigned char* data );

int populate_unique_points(std::string abs_file_name, std::vector<UniquePoint*>* unique_points,std::vector<std::string>* str_points,
		std::vector<int>* unique_fbs, int point_index );



int peasoup_multi(vivek::Filterbank* filobj, CmdLineOptions& args, DispersionTrials<unsigned char>& trials, OutputFileWriter& stats,
					AccelerationPlan& acc_plan,Zapper* bzap, int pt_num,UniquePoint* point,
					int candidate_id,  CandidateCollection* all_cands);



CandidateCollection peasoup(vivek::Filterbank* fil,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, AccelerationPlan& acc_plan,
							Zapper* bzap, UniquePoint* point);

CandidateCollection get_zero_dm_candidates(std::map<int,vivek::Filterbank*>* fanbeams, CmdLineOptions& args);


std::string get_candidate_file_name( std::string dir, int point_idx, std::string host);

std::string get_fil_file_path( std::string base, std::string utc, int fanbeam);




#endif




//int stitch_and_dump(std::string utc_dir,  std::string fil_name, std::vector<UniquePoint*>* uniqPoints, std::vector<int>* uniqFBs,std::string out_dir, bool verbose);

