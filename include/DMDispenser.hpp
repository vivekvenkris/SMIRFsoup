/*
 * DMDispenser.hpp
 *
 *  Created on: Oct 18, 2016
 *      Author: vivek
 */

#ifndef DMDISPENSER_HPP_
#define DMDISPENSER_HPP_
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
#include <transforms/dedisperser.hpp>
#include <data_types/timeseries.hpp>
#include <utils/progress_bar.hpp>
class DMDispenser {
private:
	DispersionTrials<unsigned char>& trials;
	pthread_mutex_t mutex;
	int dm_idx;
	int count;
	ProgressBar* progress;
	bool use_progress_bar;

public:
	DMDispenser(DispersionTrials<unsigned char>& trials);
	void enable_progress_bar();
	int get_dm_trial_idx(void);
	~DMDispenser();
};



#endif /* DMDISPENSER_HPP_ */
