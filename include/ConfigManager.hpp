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

	static char bp_config[MAX_CONFIG_LEN_BYTES];
	static char bp_cornerturn_config[MAX_CONFIG_LEN_BYTES];
	static char bs_config[MAX_CONFIG_LEN_BYTES];
	static char smirf_config[MAX_CONFIG_LEN_BYTES];
	static char mopsr_config[MAX_CONFIG_LEN_BYTES];

	static std::string config_root;

	static std::map< std::string, std::map< int, std::pair< int, int > > > node_bp_bs_fb_map_;

	static std::map< std::string, std::map< int, int> > coincidencer_ports_;

	static std::map< std::string, std::vector< int > > other_active_node_bs_map_;

	static std::vector< int > other_active_bs_;

	static int total_num_fanbeams_;
	static int num_fanbeams_this_server_;
	static int coincidencer_server_base_port_;

	static std::string this_host_;
	static int this_bs_;
	static int this_gpu_device_;
	static int this_coincidencer_port_;

	static std::string edge_node_;
	static int edge_bs_;

	static std::string smirf_base_;
	static std::string archives_base_;
	static std::string fb_dir_;
	static std::string bs_dir_prefix_;
	static std::string beam_processor_prefix_;
	static std::string beam_dir_prefix_;

	static unsigned int shared_mem_nbuffers_;
	static unsigned long shared_mem_buffer_size_;
	static unsigned long fft_size_;

	static int read_smirf_config();
	static int read_mopsr_bp_config();
	static int read_mopsr_bs_config();
	static int populate_coincidencer_ports();


public:

	static const std::map< std::string, std::vector< int > >& other_active_node_bs_map() { return other_active_node_bs_map_; }
	static const std::map< std::string, std::map< int, std::pair< int, int > > >& node_bp_bs_fb_map() {return node_bp_bs_fb_map_;}
	static const std::vector< int >& other_active_bs() {return other_active_bs_; }
	static const int this_gpu_device() {return this_gpu_device_; }
	static int total_num_fanbeams() { return total_num_fanbeams_; }
	static int num_fanbeams_this_server() { return num_fanbeams_this_server_; }
	static int coincidencer_server_base_port() { return coincidencer_server_base_port_; }
	static const std::map< std::string, std::map< int, int> > coincidencer_ports() {return coincidencer_ports_; }
	static const std::string this_host() { return this_host_;}
	static const int this_bs() { return this_bs_;}
	static const std::string edge_node() { return edge_node_;}
	static const int edge_bs() { return edge_bs_;}
	static const int this_coincidencer_port() { return this_coincidencer_port_;}
	static const std::string smirf_base() { return smirf_base_;}
	static const std::string archives_base() { return archives_base_;}
	static const std::string fb_dir() { return fb_dir_;}
	static const std::string bs_dir_prefix() { return bs_dir_prefix_;}
	static const std::string beam_processor_prefix() { return beam_processor_prefix_;}
	static const std::string beam_dir_prefix() { return beam_dir_prefix_;}

	static const unsigned int  shared_mem_nbuffers() { return shared_mem_nbuffers_;}
	static const unsigned long shared_mem_buffer_size() { return shared_mem_buffer_size_; }
	static const unsigned long fft_size() { return fft_size_;}

	static int load_configs(std::string host_name, int this_bs);
	static std::string get_fil_file_path(std::string base, std::string utc, int fanbeam,bool is_no_bp_structure);
	static std::string get_fil_file_path(std::string base, std::string utc, int fanbeam, std::string host, bool is_no_bp_structure);






};


#endif /* CONFIG_MANAGER_HPP_ */
