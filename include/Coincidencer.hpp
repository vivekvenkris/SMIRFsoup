/*
 * Coincidencer.h
 *
 *  Created on: Apr 25, 2017
 *      Author: vivek
 */

#ifndef COINCIDENCER_H_
#define COINCIDENCER_H_

#include <iostream>
#include <sstream>
#include <fstream>
#include <string>
#include <iomanip>
#include <vector>
#include <map>
#include <cstdlib>

#include <data_types/candidates.hpp>

#include <utils/exceptions.hpp>

#include <network/ServerSocket.hpp>
#include <network/ClientSocket.hpp>
#include <network/SocketException.hpp>

#include "ConfigManager.hpp"
#include "utils/cmdline.hpp"
#include "ShutdownManager.hpp"



class Coincidencer {

private:

	std::map<int, CandidateCollection> candidate_collection_map;

	CandidateCollection other_candidates;

	CandidateCollection this_candidates;

	CandidateCollection shortlisted_candidates;


	pthread_t server;
	pthread_t client;

	CmdLineOptions& args;

	double bin_width;

	int max_fanbeam_traversal;

	static void* candidates_server(void* candidates_map);

	static void* candidates_client (void* candidates);



public:

	Coincidencer(CmdLineOptions& args, double bin_width, int max_fanbeam_traversal):args(args),bin_width(bin_width),max_fanbeam_traversal(max_fanbeam_traversal) {

		if(! args.no_global_coincidence) {
			int start_server = pthread_create(&server, NULL, Coincidencer::candidates_server, (void*) this);
			ErrorChecker::check_pthread_create_error(start_server, "Coincidencer constructor -- starting server");
		}

	}

	void init_this_candidates(CandidateCollection c);

	int send_candidates_to_all_nodes();

	int gather_all_candidates();

	void print_shortlisted_candidates(FILE* fo=stderr);
	void print_shortlisted_candidates_more(FILE* fo=stderr);

	int coincidence();

	virtual ~Coincidencer();
};

int load_candidate_from_stream(std::istringstream& iss, Candidate* c);

#endif /* COINCIDENCER_H_ */
