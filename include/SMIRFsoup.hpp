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


#include <vivek/filterbank_def.hpp>
#include <vivek/filutil.hpp>
#include <vivek/utilities.hpp>
#include <vivek/Archiver.h>

#include "DMDispenser.hpp"
#include "ascii_header.h"
#include "dada_def.h"
#include "futils.h"
#include "strutil.h"


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

	void start(void);
};


void populate_unique_points(std::string abs_file_name, std::vector<UniquePoint*>* unique_points,std::vector<std::string>* str_points,
							   std::vector<int>* unique_fbs, int point_index );



int peasoup_multi(vivek::Filterbank* filobj,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, OutputFileWriter& stats,
					std::string xml_filename, AccelerationPlan& acc_plan, int pt_num,std::string pt_ra, std::string pt_dec,
					int candidate_id, std::string out_dir);



CandidateCollection peasoup(vivek::Filterbank* fil,CmdLineOptions& args, DispersionTrials<unsigned char>& trials, AccelerationPlan& acc_plan);

CandidateCollection get_zero_dm_candidates(std::map<int,vivek::Filterbank*>* fanbeams, CmdLineOptions& args);


std::string get_candidate_file_name( std::string dir, int point_idx);

std::string get_fil_file_path( std::string base, std::string utc, int fanbeam);



#endif




//int stitch_and_dump(std::string utc_dir,  std::string fil_name, std::vector<UniquePoint*>* uniqPoints, std::vector<int>* uniqFBs,std::string out_dir, bool verbose);

