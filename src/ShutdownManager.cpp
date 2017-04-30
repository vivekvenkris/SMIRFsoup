/*
 * ShutdownManager.cpp
 *
 *  Created on: Apr 22, 2017
 *      Author: vivek
 */

#include "ShutdownManager.hpp"
#include <csignal>
#include <unistd.h>
#include <cstdlib>

ShutdownManager::ShutdownManager() {
	// TODO Auto-generated constructor stub

}

ShutdownManager::~ShutdownManager() {
	// TODO Auto-generated destructor stub
}



volatile bool ShutdownManager::shutdown = false;
volatile bool ShutdownManager::working  = false;

void ShutdownManager::manage_shutdown(int signal){

	if( signal ==SIGINT ){

		ShutdownManager::shutdown = true;
		while(ShutdownManager::working) usleep(100 * 1000 );

	}

	else if( signal == SIGTERM ) {

		/* introduce some grace here */

		exit(0);

	}

}
