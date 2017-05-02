/*
 * Coincidencer.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: vivek
 */

#include "Coincidencer.hpp"

using namespace std;




void* Coincidencer::candidates_client(void* ptr){

	Coincidencer*  coincidencer = reinterpret_cast< Coincidencer* >(ptr);

	cerr << "Coincidencer Client:: Sending " << coincidencer->this_candidates.cands.size() << "candidates to all other nodes." << endl;


	for( string node: ConfigManager::other_active_nodes() ){

		cerr << " client trying to communicate to " << node << endl;

		ostringstream oss;

		ClientSocket client_socket ( node , ConfigManager::coincidencer_ports().at(node) );
		//ClientSocket client_socket ( "127.0.0.1", ConfigManager::coincidencer_ports().at(ConfigManager::this_host()) );

		oss << ConfigManager::this_host() << " ";
		//oss << node << " ";

		oss << coincidencer->this_candidates << " ";

		//cerr << "sending  " << oss.str().size()  << " characters" << endl;

		client_socket << oss.str();

	}

	cerr << "Client sent data to everyone. " <<  endl;

	return NULL;
}



/**
 *
 * A server is always ON and other nodes start sending its candidates to this node as soon as they are done.
 *
 */
void* Coincidencer::candidates_server(void* ptr){

	cerr << "Inside server thread ... " << endl;

	Coincidencer*  coincidencer = reinterpret_cast< Coincidencer* >(ptr);

	cerr << "Starting server on localhost:" << ConfigManager::coincidencer_ports().at(ConfigManager::this_host()) << endl;

	ServerSocket* socket = new ServerSocket( "any" , ConfigManager::coincidencer_ports().at(ConfigManager::this_host()));

	while(true){

		int waiting_connections = socket->select(5.0);

		if(waiting_connections > 0 ){

			ServerSocket conversational_socket;

			socket->accept(conversational_socket);

			try{

				ostringstream oss;

				string socket_data;
				conversational_socket >> socket_data;
				oss << socket_data;

				//cerr << "Got " << socket_data.size() << " characters" <<  endl;

				std::istringstream iss(oss.str());

				//cerr << "iss:" << iss.str() << endl;

				string node;
				iss >> node;

				CandidateCollection cc;
				iss >> cc;



				coincidencer->other_candidates.cands.insert(coincidencer->other_candidates.cands.end(), cc.cands.begin(), cc.cands.end());

				coincidencer->candidate_collection_map.insert(map<string,CandidateCollection>::value_type(node,cc));

				if(coincidencer->candidate_collection_map.size() == ConfigManager::other_active_nodes().size()){
					cerr << "Server got data from all nodes. Quitting server" <<  endl;
					break;
				}

			} catch (SocketException& e) {

			}

		}

	}


	return NULL;
}

int Coincidencer::send_candidates_to_all_nodes(){
	int start_client = pthread_create(&client, NULL, Coincidencer::candidates_client, (void*) this);
	return ErrorChecker::check_pthread_create_error(start_client, "Coincidencer send_candidates_to_all_nodes -- starting client");

}

int Coincidencer::gather_all_candidates(){

	cerr << "Waiting for the server to get all candidates." << endl;
	pthread_join(server,NULL);

	cerr << "Waiting for client to send all candidates." << endl;
	pthread_join(client,NULL);


	return EXIT_SUCCESS;

}

void Coincidencer::init_this_candidates(CandidateCollection c){
	this->this_candidates = c;
}



int Coincidencer::coincidence(){

	double freq_bin_width = 1;

	for(Candidate mine: this_candidates.cands){

		int rfi_score = 0;
		int num_similar_candidates = 0;

		for(Candidate other: other_candidates.cands){

			if( abs(mine.freq - other.freq) < bin_width) { // same frequency

				num_similar_candidates++;

				if( abs(mine.start_fanbeam - other.start_fanbeam) > max_fanbeam_traversal) rfi_score++;
				if( abs( mine.start_ns - other.start_ns ) > 0.5 ) rfi_score++;
				if(mine.dm == 0) rfi_score++;

			}

		}

		if(rfi_score <= num_similar_candidates ){

			shortlisted_candidates.append(mine);
		}


	}

	return EXIT_SUCCESS;
}

void Coincidencer::print_shortlisted_candidates(FILE* fo){

	shortlisted_candidates.print_cand_file(fo,0);

}




Coincidencer::~Coincidencer() {


}

