/*
 * Rsyncer.cpp
 *
 *  Created on: Apr 30, 2017
 *      Author: vivek
 */

#include "Rsyncer.hpp"
#include "utils/exceptions.hpp"
#include <cstdlib>

using namespace std;

Rsyncer::~Rsyncer() { }

void Rsyncer::append(string file) {
	this->files.push_back(file);
}

int Rsyncer::rsync(){

	string command = get_rsync_string();

	if(command.empty()) {
		cerr << endl << "No files for node: " << node << endl << endl;
		status = FINISHED;
		return EXIT_SUCCESS;
	}

	cerr << endl << "Running the following command for  " << node << ":"<<  command << endl;

	int return_value =  pthread_create(&rsync_thread, NULL, Rsyncer::rsync_files, (void*) this);

	ostringstream oss;
	oss <<  "rsync thread for node: " << this->node;

	if( ErrorChecker::check_pthread_create_error(return_value,oss.str()) == EXIT_FAILURE) return EXIT_FAILURE;

	return EXIT_SUCCESS;
}

void* Rsyncer::rsync_files(void* ptr){

	Rsyncer* rsyncer = reinterpret_cast<Rsyncer*>(ptr);


	rsyncer->status = RUNNING;

	if(system(rsyncer->get_rsync_string().c_str()) == EXIT_FAILURE) {
		cerr << "Problem with RSYNC system call for node: "<< rsyncer->node << " Aborting." << endl;
		return NULL;
	}

	rsyncer->status = FINISHED;

	return NULL;
}

std::string Rsyncer::get_rsync_string(){

	if(this->files.empty()) return "";

	ostringstream oss;
	oss << "rsync -aRPz --omit-dir-times ";

	for(string file: this->files) oss << this->node << ":" << file << " ";

	oss << "/";

	return oss.str();


}
