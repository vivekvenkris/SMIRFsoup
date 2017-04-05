
/**
 * Copyright 1993-2012 NVIDIA Corporation.  All rights reserved.
 *
 * Please refer to the NVIDIA end user license agreement (EULA) associated
 * with this source code for terms and conditions that govern your use of
 * this software. Any use, reproduction, disclosure, or distribution of
 * this software and related documentation outside the terms of the EULA
 * is strictly prohibited.
 */
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

#include "vivek/filterbank_def.hpp"
#include "vivek/filutil.hpp"
#include "vivek/utilities.hpp"
#include "vivek/Archiver.h"
#include "SMIRFsoup.hpp"

int stitch_and_dump(std::string utc_dir, std::string fil_name, vivek::Filterbank* cfb, std::vector<UniquePoint*>* uniqPoints , std::vector<int>* uniqFBs, std::string out_dir, bool verbose){

	std::map<int,vivek::Filterbank*> fanbeams;
	std::cerr<< "in dump mode" <<std::endl;

	for( std::vector<int>::iterator fbIterator = uniqFBs->begin(); fbIterator != uniqFBs->end(); fbIterator++){
		int fb = (int)*(fbIterator);

		std::stringstream fb_abs_path;
		fb_abs_path << utc_dir << "/FB/" << BEAM_DIR_PREFIX <<std::setfill('0') << std::setw(3) <<fb <<"/"<<fil_name;

		vivek::Filterbank* f = new vivek::Filterbank(fb_abs_path.str(),FILREAD,verbose);
		f->load_all_data();

		std::cerr<< "mean: " << f->get_mean() << "  rms:" << f->get_rms() <<std::endl;

		fanbeams[fb] = f;

	}
	std::cerr<< "loaded all FB" << fanbeams.size()<<std::endl;


	for(std::vector<UniquePoint*>::iterator upIterator = uniqPoints->begin(); upIterator!=uniqPoints->end();++upIterator){

		if(verbose) std::cerr<< "next point " <<std::endl;


		UniquePoint* point = *upIterator;

		std::stringstream name_stream;
		name_stream << out_dir<< PATH_SEPERATOR << point->ra << point->dec << ".fil";
		std::string out = name_stream.str();

		if(verbose) std::cerr<< "output file name:" << out <<std::endl;


		vivek::Filterbank* stitched_filterbank = new vivek::Filterbank(out, FILWRITE, verbose);
		vivek::Filterbank::copy_header(cfb,stitched_filterbank,verbose);

		double sigproc_dej, sigproc_raj;
		hhmmss_to_sigproc(point->ra.c_str(),&sigproc_raj);
		ddmmss_to_sigproc(point->dec.c_str(),&sigproc_dej);

		stitched_filterbank->set_value_for_key<double>(SRC_DEJ,sigproc_dej);
		stitched_filterbank->set_value_for_key<double>(SRC_RAJ,sigproc_raj);

		stitched_filterbank->alloc_data();

		if(verbose) std::cerr<< "successfully copied header" <<std::endl;


		for(std::vector<Traversal*>::iterator it2 = point->traversals->begin(); it2!=point->traversals->end(); it2++){

			Traversal* traversal = *it2;
			int fb = traversal->fanbeam;
			std::cerr << fb << std::endl;
			vivek::Filterbank* f  = fanbeams.at(fb);
			std::cerr << f->file_name << std::endl;
			stitched_filterbank->store_data(f->data, traversal->startSample, traversal->numSamples);

		}

		std::cerr << "unloading output file" << std::endl;
		stitched_filterbank->unload_filterbank();

		std::cerr<< "mean: " << stitched_filterbank->get_mean() << "  rms:" << stitched_filterbank->get_rms() <<std::endl;

		vivek::Archiver* a = new vivek::Archiver();
		a->transfer_fil_to_DADA_buffer(stitched_filterbank);


		delete stitched_filterbank;

	}
	for (std::map<int,vivek::Filterbank*>::iterator it=fanbeams.begin(); it!=fanbeams.end(); ++it) delete it->second;

	return EXIT_SUCCESS;

}
