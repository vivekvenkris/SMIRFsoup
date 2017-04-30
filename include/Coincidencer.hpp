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



class Coincidencer {

private:

	std::map<std::string, CandidateCollection>* candidate_collection_map;

	CandidateCollection other_candidates;

	CandidateCollection* this_candidates;

	CandidateCollection* shortlisted_candidates;

	std::string this_host;

	pthread_t server;
	pthread_t client;

	static void* candidates_server(void* candidates_map);

	static void* candidates_client (void* candidates);



public:
	Coincidencer(CandidateCollection* this_candidates, std::string host): this_candidates(this_candidates), this_host(host) {

		candidate_collection_map = new std::map<std::string, CandidateCollection>();

		shortlisted_candidates = new CandidateCollection();

		int start_server = pthread_create(&server, NULL, Coincidencer::candidates_server, (void*) candidate_collection_map);
		ErrorChecker::check_pthread_create_error(start_server, "Coincidencer constructor -- starting server");

	}

	int gather_all_candidates();

	int coincidence();

	virtual ~Coincidencer();
};

int load_candidate_from_stream(std::istringstream& iss, Candidate* c);

#endif /* COINCIDENCER_H_ */
