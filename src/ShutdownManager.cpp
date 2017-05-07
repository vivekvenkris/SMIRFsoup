/*
 * ShutdownManager.cpp
 *
 *  Created on: Apr 22, 2017
 *      Author: vivek
 */

#include "ShutdownManager.hpp"


using namespace std;

volatile bool ShutdownManager::exit_request = false;

void ShutdownManager::manage_shutdown(int signal){	if( signal == SIGTERM ) exit_request = true; }

bool ShutdownManager::shutdown_called(){ return exit_request == true; }

void ShutdownManager::shutdown(string position){
	cerr << "Shut down requested at position:  " << position << endl;
	std::exit(EXIT_FAILURE);
}
