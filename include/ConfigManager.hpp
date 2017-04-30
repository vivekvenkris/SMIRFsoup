/*
 * SMIRFutils.hpp
 *
 *  Created on: Apr 21, 2017
 *      Author: Vivek Venkatraman Krishnan
 */
#pragma once
#ifndef CONFIG_MANAGER_HPP_
#define CONFIG_MANAGER_HPP_

#include "SMIRFdef.hpp"
#include <string>
#include <vector>
#include <map>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <iostream>
#include <unistd.h>
#include <algorithm>
#include "ascii_header.h"
#include "dada_def.h"
#include "futils.h"
#include "strutil.h"
#include "vivek/utilities.hpp"

class ConfigManager {

private:
	static int read_smirf_config();
	static int read_mopsr_config();
	static int populate_coincidencer_ports();

	static std::map< std::string, std::map< int, std::pair< int, int > > > node_bp_fb_map_;

	static std::vector<std::string> other_active_nodes_;
	static std::map<std::string, int> coincidencer_ports_;

	static int total_num_fanbeams_;
	static int num_fanbeams_this_server_;
	static int coincidencer_server_base_port_;
	static std::string this_host_;

	/* Node that processes all points that pass from one node to another. */
	static std::string edge_node_;


public:

	static const std::vector<std::string>& other_active_nodes()  { return other_active_nodes_; }
	static std::map< std::string, std::map< int, std::pair< int, int > > > node_bp_fb_map() {return node_bp_fb_map_;};
	static int total_num_fanbeams() { return total_num_fanbeams_; }
	static int num_fanbeams_this_server() { return num_fanbeams_this_server_; }
	static int coincidencer_server_base_port() { return coincidencer_server_base_port_; }
	static const std::map<std::string, int> coincidencer_ports() {return coincidencer_ports_; }
	static const std::string this_host() { return this_host_;}
	static const std::string edge_node() { return edge_node_;}

	static int load_configs(std::string host_name);
	static std::string get_fil_file_path(std::string base, std::string utc, int fanbeam);
	static std::string get_fil_file_path(std::string base, std::string utc, int fanbeam, std::string host);






};


#endif /* CONFIG_MANAGER_HPP_ */
