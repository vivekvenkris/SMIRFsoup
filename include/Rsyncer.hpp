/*
 * Rsyncer.h
 *
 *  Created on: Apr 30, 2017
 *      Author: vivek
 */

#ifndef RSYNCER_H_
#define RSYNCER_H_

#include <string>
#include <vector>
#include <iostream>


class Rsyncer {
private:
	std::string node;
	std::vector<std::string> files;
	pthread_t rsync_thread;

	static void* rsync_files(void* ptr);

public:
	Rsyncer(std::string node):node(node){ }
	void append(std::string file);
	int rsync();
	std::string get_rsync_string();
	virtual ~Rsyncer();

	const std::vector<std::string>& getFiles() const {
		return files;
	}

	const std::string& getNode() const {
		return node;
	}

	pthread_t getRsyncThread() const {
		return rsync_thread;
	}
};

#endif /* RSYNCER_H_ */
