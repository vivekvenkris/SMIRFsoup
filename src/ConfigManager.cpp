
#include "ConfigManager.hpp"
#include "SMIRFdef.hpp"
#include "ConfigManager.hpp"


using namespace std;


char ConfigManager::bp_config[MAX_CONFIG_LEN_BYTES];
char ConfigManager::bp_cornerturn_config[MAX_CONFIG_LEN_BYTES];
char ConfigManager::bs_config[MAX_CONFIG_LEN_BYTES];
char ConfigManager::smirf_config[MAX_CONFIG_LEN_BYTES];
char ConfigManager::mopsr_config[MAX_CONFIG_LEN_BYTES];

std::string ConfigManager::config_root = "";

std::map< std::string, std::map< int, std::pair< int, int > > > ConfigManager::node_bp_fb_map_;

std::map< std::string, std::map< int, int> > ConfigManager::coincidencer_ports_;


std::map< std::string, std::vector< int > > ConfigManager::other_active_node_bs_map_;

std::vector< int > ConfigManager::other_active_bs_;

int ConfigManager::total_num_fanbeams_ = -1;
int ConfigManager::num_fanbeams_this_server_ = -1;
int ConfigManager::coincidencer_server_base_port_ = -1;

std::string ConfigManager::this_host_ = "";
int ConfigManager::this_bs_ = -1;
int ConfigManager::this_gpu_device_ = -1;
int ConfigManager::this_coincidencer_port_ = -1;

/* Node that processes all points that pass from one node to another. */
std::string ConfigManager::edge_node_ = "";
int ConfigManager::edge_bs_ = -1;


std::string ConfigManager::smirf_base_ = "";
std::string ConfigManager::archives_base_= "";
std::string ConfigManager::fb_dir_= "";
std::string ConfigManager::bs_dir_prefix_= "";
std::string ConfigManager::beam_processor_prefix_= "";
std::string ConfigManager::beam_dir_prefix_="";


unsigned int ConfigManager::shared_mem_nbuffers_ = 0;;
unsigned long ConfigManager::shared_mem_buffer_size_ = 0;


/**
 *
 */

int ConfigManager::load_configs(std::string host, int this_bs){

	this_host_ = host;
	this_bs_ = this_bs;

	if( read_smirf_config() == EXIT_FAILURE) return EXIT_FAILURE;

	if( read_mopsr_bs_config() == EXIT_FAILURE) return EXIT_FAILURE;

	if( read_mopsr_bp_config() == EXIT_FAILURE) return EXIT_FAILURE;

	if( populate_coincidencer_ports() == EXIT_FAILURE ) return EXIT_FAILURE;


	return EXIT_SUCCESS;

}


int ConfigManager::populate_coincidencer_ports(){

	cerr << "Coincidencer ports: " <<  endl;

	for(const auto &kv: other_active_node_bs_map_){

		string host = kv.first;

		cerr << " \t host: " <<  host << endl;

		for( const auto &val: kv.second) {

			cerr << "\t \t BS_"<< val << ": " << (coincidencer_server_base_port_ + val) << endl;

			map<int, int> bs_port_map = get_or_default< string, map<int, int> >( coincidencer_ports_, host, map<int,int>() );
			bs_port_map.insert(map<int,int>::value_type(val,coincidencer_server_base_port_ + val));

			coincidencer_ports_.insert(map< string, map<int, int> >::value_type (host, bs_port_map));

		}

	}

	map<int, int> bs_port_map = get_or_default< string, map<int, int> >( coincidencer_ports_, this_host_, map<int,int>() );
	bs_port_map.insert(map<int,int>::value_type(this_bs_,coincidencer_server_base_port_ + this_bs_));

	coincidencer_ports_.insert(map< string, map<int, int> >::value_type (this_host_, bs_port_map));

	this_coincidencer_port_ = coincidencer_server_base_port_ + this_bs_;

	return EXIT_SUCCESS;
}


/**
 * loads the beam_map which then knows which directories to use to find a filterbank.
 *
 * 1. Read NUM_BP from MOPSR_BP_CFG
 *
 * 2. Get all BP?? for this host
 *
 * 3. Get start and end beam numbers for all BP??
 *
 */

int ConfigManager::read_mopsr_bp_config(){


	int  num_beam_processors;

	char config_name[1024];

	if (ascii_header_get (smirf_config , "MOPSR_BP_CONFIG", "%s", &config_name) != 1) {
		cerr<< "Cannot read MOPSR_BS_CONFIG from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}

	ostringstream config_file_name_stream;
	config_file_name_stream << config_root << PATH_SEPERATOR << config_name;

	string bp_config_file = config_file_name_stream.str();

	cerr << "Reading MOPSR_BP_CONFIG from " << bp_config_file << endl;


	if (fileread (bp_config_file.c_str() , bp_config, 65536) < 0){
		cerr<< "Cannot read file: " << bp_config_file << endl;
		return EXIT_FAILURE;
	}

	//************************************************************************

	config_name[0] = '\0';

	if (ascii_header_get (smirf_config , "MOPSR_BP_CORNERTURN_CONFIG", "%s", &config_name) != 1) {
		cerr<< "Cannot read MOPSR_BS_CONFIG from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}

	config_file_name_stream.clear();
	config_file_name_stream.str("");

	config_file_name_stream << config_root << PATH_SEPERATOR << config_name;

	string bp_cornerturn_config_file = config_file_name_stream.str();

	cerr << "Reading MOPSR_BP_CORNERTURN_CONFIG from " << bp_cornerturn_config_file << endl;


	if (fileread (bp_cornerturn_config_file.c_str(), bp_cornerturn_config, 65536) < 0){
		cerr<< "Cannot read file: " << bp_cornerturn_config_file<< endl;
		return EXIT_FAILURE;
	}


	if (ascii_header_get (bp_config , "NUM_BP", "%d", &num_beam_processors) != 1) {
		cerr<< "Cannot read NUM_BP from file: " << bp_config_file << endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_get (bp_cornerturn_config , "NBEAM", "%d", &ConfigManager::total_num_fanbeams_) != 1) {
		cerr<< "Cannot read NBEAM from file: " << bp_cornerturn_config_file << endl;
		return EXIT_FAILURE;
	}


	vector<string> active_nodes_from_map(other_active_node_bs_map_.size());
	get_key_vector_from_map<string, vector<int> >(&other_active_node_bs_map_, &active_nodes_from_map);


	vector<string> all_nodes;
	all_nodes.insert(std::end(all_nodes),std::begin(active_nodes_from_map), std::end(active_nodes_from_map));
	all_nodes.push_back(this_host_);

	for(string inode: all_nodes){

		vector<int> beam_processors;

		for( int i = 0; i < num_beam_processors; i++){

			char node[1024];

			stringstream bp;
			bp << "BP_" << i;

			if (ascii_header_get (bp_config , bp.str().c_str(), "%s", &node[0]) != 1) {
				cerr<< "Cannot read "<< bp.str().c_str() << " from file: " << bp_config_file << endl;
				return EXIT_FAILURE;
			}

			if( inode.find(node) !=  inode.npos  || (inode == edge_node_)) beam_processors.push_back(i);

		}

		if(beam_processors.empty() && inode != ConfigManager::edge_node_){
			cerr << "No beams in this host: " << inode << endl;
			return EXIT_FAILURE;
		}

		map<int, pair<int, int> > bp_map;

		for( int bp: beam_processors){
			int first, last;

			ostringstream ss;

			ss << "BEAM_FIRST_RECV_" << bp;
			if (ascii_header_get (bp_cornerturn_config , ss.str().c_str(), "%d", &first) != 1) {
				cerr<< "Cannot read " << ss.str().c_str() << " from file: " << bp_cornerturn_config_file << endl;
				return EXIT_FAILURE;
			}

			ss.str(" ");
			ss.clear();

			ss << "BEAM_LAST_RECV_" << bp;
			if (ascii_header_get (bp_cornerturn_config , ss.str().c_str(), "%d", &last) != 1) {
				cerr<< "Cannot read " << ss.str().c_str() << " from file: " << bp_cornerturn_config_file << endl;
				return EXIT_FAILURE;
			}

			first++;
			last++;

			std::pair<int,int> values;
			values.first = first;
			values.second = last;

			bp_map.insert(map<int, pair<int, int> >::value_type(bp,values));

			if(inode == this_host_) ConfigManager::num_fanbeams_this_server_ += (last - first + 1);


		}
		ConfigManager::node_bp_fb_map_.insert(map<string, map < int, pair< int, int> > >::value_type(inode,bp_map) );

	}

	for( pair< string, map < int, pair< int, int> > > bp_map : node_bp_fb_map_){
		string current  = (bp_map.first == this_host_)? " <current node> ": " ";
		cerr << "Node:" <<  bp_map.first << current << endl;
		for( pair< int, pair < int, int> > fb_map: bp_map.second ){
			cerr << "\t" << "BEAM_" << setfill('0') << setw(3) << fb_map.second.first << " to BEAM_" << setfill('0') << setw(3) << fb_map.second.second
					<< " on BP" << setfill('0') << setw(2)  << fb_map.first  << endl;
		}

	}



	config_name[0] = '\0';

	if (ascii_header_get (smirf_config , "MOPSR_CONFIG", "%s", &config_name) != 1) {
		cerr<< "Cannot read MOPSR_CONFIG from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}

	config_file_name_stream.clear();
	config_file_name_stream.str("");

	config_file_name_stream << config_root << PATH_SEPERATOR << config_name;

	string mopsr_config_file = config_file_name_stream.str();

	cerr << "Reading MOPSR_CONFIG from " << mopsr_config_file << endl;


	if (fileread (mopsr_config_file.c_str(), mopsr_config, 65536) < 0){
		cerr<< "Cannot read file: " << mopsr_config_file.c_str() << endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_get (mopsr_config , "SMIRF_COINCIDENCER_SERVER", "%d", &ConfigManager::coincidencer_server_base_port_) != 1) {
		cerr<< "Cannot read SMIRF_COINCIDENCER_SERVER from file: " << mopsr_config_file << endl;
		return EXIT_FAILURE;
	}


	return EXIT_SUCCESS;

}


int ConfigManager::read_mopsr_bs_config(){

	char config_name[1024];

	if (ascii_header_get (smirf_config , "MOPSR_BS_CONFIG", "%s", &config_name) != 1) {
		cerr<< "Cannot read MOPSR_BS_CONFIG from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}

	ostringstream config_file_name_stream;
	config_file_name_stream << config_root << PATH_SEPERATOR << config_name;

	string config_file = config_file_name_stream.str();

	cerr << "Reading MOPSR_BS_CONFIG from " << config_file << endl;


	if (fileread (config_file.c_str() , bs_config, 65536) < 0){
		cerr<< "Cannot read file: " << config_file << endl;
		return EXIT_FAILURE;
	}


	int num_bs;

	if (ascii_header_get (bs_config , "NUM_BS", "%d", &num_bs) != 1) {
		cerr<< "Cannot read NUM_BS from file: " << config_file << endl;
		return EXIT_FAILURE;
	}

	for( int bs=0; bs <  num_bs; bs++ ){

		stringstream bs_status_stream;
		bs_status_stream << "BS_STATE_" << bs;

		char value[1024];

		if (ascii_header_get (bs_config , bs_status_stream.str().c_str(), "%s", &value) != 1) {

			cerr<< "Cannot read " << bs_status_stream.str() << " from file: " << config_file << endl;
			return EXIT_FAILURE;

		}

		if( string(value) == "active" ) {

			value[0] = '\0';

			stringstream bs_name_stream;
			bs_name_stream << "BS_" << bs;

			if (ascii_header_get (bs_config , bs_name_stream.str().c_str(), "%s", &value) != 1) {

				cerr<< "Cannot read " << bs_name_stream.str() << " from file: " << config_file << endl;
				return EXIT_FAILURE;

			}
			vector<int> bses = get_or_default< string, vector<int> > (other_active_node_bs_map_,string(value), vector<int>());
			bses.push_back(bs);
			other_active_node_bs_map_.insert( map < string, vector<int > >::value_type(value,bses));

			other_active_bs_.push_back(bs);

		}



	}

	bool this_bs_found = false;

	for( auto &kv : other_active_node_bs_map_) {

		string host = kv.first;
		vector<int> bses = kv.second;

		if( std::find(bses.begin(), bses.end(),this_bs_) != bses.end() ){

			if(host != this_host_ ){

				cerr << " BS_" << this_bs_ << " found  in host: " << host << " while expecting at this host: " << this_host_ <<". Aborting now."<< endl;
				return EXIT_FAILURE;

			} else {

				this_bs_found = true;
			}

		}

	}

	if( !this_bs_found ){

		cerr<< " Current BS: BS_" << this_bs_ << "is not active on host: " << this_host_ << " in the config file: " << config_file << endl;
		return EXIT_FAILURE;

	} else{

		vector<int> bses = other_active_node_bs_map_.at(this_host_);
		vector<int>::iterator position = std::find(bses.begin(), bses.end(), this_bs_);
		bses.erase(position);

		position = std::find(other_active_bs_.begin(), other_active_bs_.end(), this_bs_);
		other_active_bs_.erase(position);

	}


	cerr << "Other active beam searchers are: " << endl;


	for( const auto &kv : other_active_node_bs_map_) {

		ostringstream print_stream;
		print_stream << "\t";

		for(const auto &val : kv.second) print_stream <<" BS_"<< val << " ";

		print_stream << " on " << kv.first ;

		cerr << print_stream.str() << endl;

	}

	ostringstream gpu_device_stream;
	gpu_device_stream << "BS_GPU_ID_" << this_bs_;

	if (ascii_header_get (bs_config , gpu_device_stream.str().c_str(), "%d", &this_gpu_device_) != 1) {

		cerr<< "Cannot read " << gpu_device_stream.str() << " from file: " << config_file << endl;
		return EXIT_FAILURE;

	}


	return EXIT_SUCCESS;
}

/**
 * Read SMIRF CFG file which contains which nodes are active. Coincidencer will wait only for these nodes.
 *
 * 1. Read NUM_NODES.
 *
 * 2. For each node, read NODE_STATUS_??
 *
 * 3. if "active" , read node name from key = NODE_?? and add to node map
 *
 */

int ConfigManager::read_smirf_config(){

	cerr << "Reading SMIRF_CFG from " << SMIRF_CFG << endl;


	if (fileread (SMIRF_CFG, smirf_config, 65536) < 0){
		cerr<< "Cannot read file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}

	char edge_node[1024];


	if (ascii_header_get (smirf_config , "EDGE_NODE", "%s", &edge_node[0]) != 1) {

		cerr<< "Cannot read " << "EDGE_NODE" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	char edge_bs[1024];

	if (ascii_header_get (smirf_config , "EDGE_BS", "%s", &edge_bs[0]) != 1) {

		cerr<< "Cannot read " << "EDGE_BS" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::edge_node_ = edge_node;
	ConfigManager::edge_bs_ = ::atoi(edge_bs);

	cerr << "Edge bs is BS_" << ConfigManager::edge_bs_ << " on node: " << ConfigManager::edge_node_ << endl;


	char config_root[1024];

	if (ascii_header_get (smirf_config , "CONFIG_ROOT", "%s", &config_root[0]) != 1) {

		cerr<< "Cannot read " << "CONFIG_ROOT" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::config_root = string(config_root);


	char smirf_base[1024];

	if (ascii_header_get (smirf_config , "SMIRF_BASE", "%s", &smirf_base[0]) != 1) {

		cerr<< "Cannot read " << "SMIRF_BASE" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::smirf_base_ = string(smirf_base);

	char archives_base[1024];

	if (ascii_header_get (smirf_config , "ARCHIVES_BASE", "%s", &archives_base[0]) != 1) {

		cerr<< "Cannot read " << "ARCHIVES_BASE" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::archives_base_ = string(archives_base);

	char fb_dir[1024];

	if (ascii_header_get (smirf_config , "FB_DIR", "%s", &fb_dir[0]) != 1) {

		cerr<< "Cannot read " << "FB_DIR" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::fb_dir_ = string(fb_dir);


	char bs_dir_prefix[1024];

	if (ascii_header_get (smirf_config , "BS_DIR_PREFIX", "%s", &bs_dir_prefix[0]) != 1) {

		cerr<< "Cannot read " << "BS_DIR_PREFIX" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::bs_dir_prefix_ = string(bs_dir_prefix);


	char beam_processor_prefix[1024];

	if (ascii_header_get (smirf_config , "BEAM_PROCESSOR_PREFIX", "%s", &beam_processor_prefix[0]) != 1) {

		cerr<< "Cannot read " << "BEAM_PROCESSOR_PREFIX" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::beam_processor_prefix_ = string(beam_processor_prefix);

	char beam_dir_prefix[1024];

	if (ascii_header_get (smirf_config , "BEAM_DIR_PREFIX", "%s", &beam_dir_prefix[0]) != 1) {

		cerr<< "Cannot read " << "BEAM_DIR_PREFIX" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::beam_dir_prefix_ = string(beam_dir_prefix);


	return EXIT_SUCCESS;

}

/**
 *  Format:  BASE_ARCHIVES_DIR/BP??/<UTC>/FB/BEAM_???/<UTC>.fil
 */
string ConfigManager::get_fil_file_path(string base,string utc, int fanbeam){
	return get_fil_file_path(base,utc,fanbeam,ConfigManager::this_host_);
}



string ConfigManager::get_fil_file_path(string base,string utc, int fanbeam, string host){

	map<int, pair<int,int> > beam_map = node_bp_fb_map_.at(host);

	map<int, vector<int> >::iterator map_it;
	for(pair<int, pair< int, int> > bp_kv: beam_map){

		int bp = bp_kv.first;
		pair<int,int> values =bp_kv.second;

		if(fanbeam >= values.first && fanbeam <= values.second){

			stringstream root;
			root 	<< base
					<< PATH_SEPERATOR
					<< beam_processor_prefix_
					<< std::setfill('0')
			<< std::setw(2)
			<< bp
			<< PATH_SEPERATOR
			<< utc
			<< PATH_SEPERATOR
			<< fb_dir_
			<< PATH_SEPERATOR
			<< beam_dir_prefix_
			<< std::setfill('0')
			<< std::setw(3)
			<< fanbeam
			<< PATH_SEPERATOR
			<< utc
			<< ".fil";

			return root.str();

		}

	}
	return "";

}


