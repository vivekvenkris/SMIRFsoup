#include "../include/DMDispenser.hpp"


DMDispenser::DMDispenser(DispersionTrials<unsigned char>& trials)
:trials(trials),dm_idx(0),use_progress_bar(false){
	count = trials.get_count();
	pthread_mutex_init(&mutex, NULL);
}

void DMDispenser::enable_progress_bar(){
	progress = new ProgressBar();
	use_progress_bar = true;
}

int DMDispenser::get_dm_trial_idx(void){
	pthread_mutex_lock(&mutex);
	int retval;
	if (dm_idx==0)
		if (use_progress_bar){
			printf("Releasing DMs to workers...\n");
			progress->start();
		}
	if (dm_idx >= trials.get_count()){
		retval =  -1;
		if (use_progress_bar)
			progress->stop();
	} else {
		if (use_progress_bar)
			progress->set_progress((float)dm_idx/count);
		retval = dm_idx;
		dm_idx++;
	}
	pthread_mutex_unlock(&mutex);
	return retval;
}

DMDispenser::~DMDispenser(){
	if (use_progress_bar)
		delete progress;
	pthread_mutex_destroy(&mutex);
}

