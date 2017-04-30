/*
 * Coincidencer.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: vivek
 */

#include "Coincidencer.hpp"

using namespace std;

int load_stream_from_candidate( ostringstream& oss, Candidate* c){

	c->ra = 1;
	c->dec = 2;
	c->dm = 3;
	c->dm_idx = 4;
	c->acc = 5;
	c->nh = 6;
	c->snr = 7;
	c->freq = 8;
	c->folded_snr = 9;
	c->opt_period = 10;
	c->is_adjacent = true;
	c->is_physical = true;
	c->ddm_count_ratio = 11;
	c->ddm_snr_ratio = 12;
	c->nbins = 13;
	c->nints = 14;
	c->nassoc = c->assoc.size();

	oss	<< c->ra << " "
			<< c->dec << " "
			<< c->dm << " "
			<< c->dm_idx << " "
			<< c->acc << " "
			<< c->nh << " "
			<< c->snr << " "
			<< c->freq << " "
			<< c->folded_snr << " "
			<< c->opt_period << " "
			<< c->is_adjacent << " "
			<< c->is_physical << " "
			<< c->ddm_count_ratio << " "
			<< c->ddm_snr_ratio << " "
			<< c->nbins << " "
			<< c->nints << " "
			<< c->nassoc;

	for( int i=0; i < c->nassoc; i++ ) {

		Candidate ass = c->assoc[i];
		load_stream_from_candidate(oss,&ass);
	}

	return EXIT_SUCCESS;
}


int load_candidate_from_stream(std::istringstream& iss, Candidate* c){

	iss >> c->ra
	>> c->dec
	>> c->dm
	>> c->dm_idx
	>> c->acc
	>> c->nh
	>> c->snr
	>> c->freq
	>> c->folded_snr
	>> c->opt_period
	>> c->is_adjacent
	>> c->is_physical
	>> c->ddm_count_ratio
	>> c->ddm_snr_ratio
	>> c->nbins
	>> c->nints
	>> c->nassoc;





	//	for( int i=0; i < c->nassoc; i++ ) {
	//		Candidate* ass = new Candidate();
	//		load_candidate_from_stream(iss,c);
	//		c->assoc.push_back(*ass);
	//	}

	return EXIT_SUCCESS;

}


void* Coincidencer::candidates_client(void* ptr){

	CandidateCollection* this_candidates = reinterpret_cast<CandidateCollection* >(ptr);

	cerr << "From client thread. " << this_candidates->cands.size() << endl;


	for( string node: ConfigManager::other_active_nodes() ){

		cerr << " client trying to communicate to " << node << endl;

		ClientSocket client_socket ( "127.0.0.1", ConfigManager::coincidencer_ports().at(node) );

		string reply;

		ostringstream oss;

		oss << this_candidates->cands.size() << " ";

		for(Candidate c: this_candidates->cands) load_stream_from_candidate(oss, &c);

		cerr << "sending  " << oss.str().size()  << " characters" << endl;

		client_socket << oss.str();



	}

	return NULL;
}



/**
 *
 * A server is always ON and other nodes start sending its candidates to this node as soon as they are done.
 *
 */
void* Coincidencer::candidates_server(void* ptr){

	map<string, CandidateCollection>*  candidates_map = reinterpret_cast< map<string, CandidateCollection>* >(ptr);

	std::cerr << "From server thread. " << candidates_map->size() << endl;

	ServerSocket* socket = new ServerSocket( "127.0.0.1" , ConfigManager::coincidencer_ports().at(ConfigManager::this_host()));

	while(true){

		int waiting_connections = socket->select(1.0);

		if(waiting_connections > 0 ){

			ServerSocket conversational_socket;

			socket->accept(conversational_socket);

			try{

				string socket_data;
				ostringstream oss;

				conversational_socket >> socket_data;
				oss << socket_data;

				cerr << "Got " << socket_data.size() << " characters" <<  endl;

				std::istringstream iss(oss.str());

				string num_cands_str;
				iss >> num_cands_str;
				int num_cands = ::atoi(num_cands_str.c_str());

				for( int i=0;i < num_cands; i++) load_candidate_from_stream(iss,new Candidate());

			} catch (SocketException& e) {

			}

		}

	}


	return NULL;
}

int Coincidencer::gather_all_candidates(){

	int start_client = pthread_create(&client, NULL, Coincidencer::candidates_client, (void*) this_candidates);

	pthread_join(server,NULL);
	pthread_join(client,NULL);

	cerr << "Exiting coincidencer. Everything is awesome! " << endl;

	return EXIT_SUCCESS;

}



int Coincidencer::coincidence(){
	return EXIT_SUCCESS;
}



Coincidencer::~Coincidencer() {

	delete shortlisted_candidates;
	delete candidate_collection_map;

}

