/*
 * Stitcher.h
 *
 *  Created on: Apr 21, 2017
 *      Author: vivek
 */
#pragma once
#ifndef STITCHER_H_
#define STITCHER_H_
#include <stdio.h>
#include <stdlib.h>
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
#include<set>
#include<cmath>
#include <vector>
#include <sstream>
#include <iostream>
#include <fstream>
#include "vivek/filterbank_def.hpp"
#include "vivek/filutil.hpp"
#include "vivek/utilities.hpp"
#include "vivek/Archiver.h"
#include "SMIRFdef.hpp"
#include "ConfigManager.hpp"
#include <utils/cmdline.hpp>
#include <csignal>


class Stitcher{
public:
	CmdLineOptions& args;
	Stitcher(CmdLineOptions& args):args(args){}

	virtual ~Stitcher();

	vivek::Filterbank* stitch(UniquePoint* point, std::map<int,vivek::Filterbank*>* fanbeams);
	vivek::Filterbank* stitch(UniquePoint* point);

	int stitch_and_dump(std::vector<UniquePoint*>* points, std::vector<int>* uniqFBs);
	int stitch_and_transfer(std::vector<UniquePoint*>* points, key_t out_key, std::string candidates_file, std::string dspsr_out_dir);

private:
	int parse_candidates_file(std::string candidates_file);


};


#endif /* STITCHER_H_ */
