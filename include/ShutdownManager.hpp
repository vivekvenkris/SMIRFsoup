/*
 * ShutdownManager.h
 *
 *  Created on: Apr 22, 2017
 *      Author: vivek
 */

#ifndef SHUTDOWNMANAGER_H_
#define SHUTDOWNMANAGER_H_

#include <csignal>
#include <unistd.h>
#include <cstdlib>
#include <string>
#include <iostream>
class ShutdownManager {
public:

	ShutdownManager() {};
	virtual ~ShutdownManager() {};

	static volatile bool exit_request;
	static void manage_shutdown(int signal);
	static bool shutdown_called();
	static void shutdown(std::string position);
};

#endif /* SHUTDOWNMANAGER_H_ */
