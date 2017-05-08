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

#define IDLE 0
#define RUNNING 1
#define FINISHED 2


class Rsyncer {
private:
	std::string node;
	std::vector<std::string> files;
	pthread_t rsync_thread;
	int status;

	static void* rsync_files(void* ptr);

public:
	Rsyncer(std::string node):node(node),status(IDLE){  }
	void append(std::string file);
	int rsync();
	std::string get_rsync_string();
	virtual ~Rsyncer();

	int get_status() const { return status; }

	void set_status(int status )  { this->status = status; }

	bool is_running() const { return !(status == RUNNING); }

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
