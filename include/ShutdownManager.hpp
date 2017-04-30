/*
 * ShutdownManager.h
 *
 *  Created on: Apr 22, 2017
 *      Author: vivek
 */

#ifndef SHUTDOWNMANAGER_H_
#define SHUTDOWNMANAGER_H_

class ShutdownManager {
public:

	ShutdownManager();
	virtual ~ShutdownManager();

	static volatile bool shutdown;
	static volatile bool working;
	static void manage_shutdown(int signal);
};

#endif /* SHUTDOWNMANAGER_H_ */
