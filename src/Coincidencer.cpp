/*
 * Coincidencer.cpp
 *
 *  Created on: Apr 25, 2017
 *      Author: vivek
 */

#include "Coincidencer.hpp"

using namespace std;




void* Coincidencer::candidates_client(void* ptr){

	cerr << "Inside client thread..." << endl;

	Coincidencer*  coincidencer = reinterpret_cast< Coincidencer* >(ptr);

	cerr << "From client thread. " << coincidencer->this_candidates.cands.size() << endl;


	for( string node: ConfigManager::other_active_nodes() ){

		cerr << " client trying to communicate to " << node << endl;

		ostringstream oss;

		//ClientSocket client_socket ( node, ConfigManager::coincidencer_ports().at(node) );
		ClientSocket client_socket ( "127.0.0.1", ConfigManager::coincidencer_ports().at(ConfigManager::this_host()) );

		//oss << ConfigManager::this_host() << " ";
		oss << node << " ";

		oss << coincidencer->this_candidates << " ";

		cerr << "sending  " << oss.str().size()  << " characters" << endl;

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

	std::cerr << "Inside server thread ... " << endl;

	Coincidencer*  coincidencer = reinterpret_cast< Coincidencer* >(ptr);

	std::cerr << "From server thread. " << coincidencer->candidate_collection_map->size() << endl;

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

				string node;
				iss >> node;

				CandidateCollection cc;
				iss >> cc;

				coincidencer->candidate_collection_map->insert(map<string,CandidateCollection>::value_type(node,cc));

				if(coincidencer->candidate_collection_map->size() == ConfigManager::other_active_nodes().size()){
					cerr << "Server got data from all nodes. Quitting server" <<  endl;
					break;
				}

			} catch (SocketException& e) {

			}

		}

	}


	return NULL;
}

int Coincidencer::gather_all_candidates(){

	int start_client = pthread_create(&client, NULL, Coincidencer::candidates_client, (void*) this);

	pthread_join(server,NULL);
	pthread_join(client,NULL);

	cerr << "Exiting coincidencer. Everything is awesome! " << endl;

	return EXIT_SUCCESS;

}

void Coincidencer::init_this_candidates(CandidateCollection c){
	this->this_candidates = c;
}



int Coincidencer::coincidence(){
	return EXIT_SUCCESS;
}



Coincidencer::~Coincidencer() {

	delete shortlisted_candidates;
	delete candidate_collection_map;

}

