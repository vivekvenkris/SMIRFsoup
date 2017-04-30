
#include "ConfigManager.hpp"
#include "SMIRFdef.hpp"
#include "ConfigManager.hpp"


using namespace std;

vector<string> ConfigManager::other_active_nodes_;
map<string,int> ConfigManager::coincidencer_ports_;

map< string, map< int, pair< int, int > > > ConfigManager::node_bp_fb_map_;



int ConfigManager::total_num_fanbeams_ = 0;
int ConfigManager::num_fanbeams_this_server_ = 0;
int ConfigManager::coincidencer_server_base_port_ = 0;

string ConfigManager::this_host_ = "";

string ConfigManager::edge_node_ = "";

/**
 *
 */

int ConfigManager::load_configs(std::string host){

	this_host_ = host;

	if( read_smirf_config() == EXIT_FAILURE) return EXIT_FAILURE;

	if( read_mopsr_config() == EXIT_FAILURE) return EXIT_FAILURE;

	if( populate_coincidencer_ports() == EXIT_FAILURE ) return EXIT_FAILURE;


	return EXIT_SUCCESS;

}


int ConfigManager::populate_coincidencer_ports(){

	std::for_each(other_active_nodes_.begin(), other_active_nodes_.end(), [] (std::string a) {

		int server;
		if ( get_number_from_server_name(a, &server) == EXIT_FAILURE) return EXIT_FAILURE;

		coincidencer_ports_.insert(std::map<std::string,int>::value_type(a, coincidencer_server_base_port_ + server));

	});


	int this_server;
	if ( get_number_from_server_name(this_host_, &this_server) == EXIT_FAILURE) return EXIT_FAILURE;

	coincidencer_ports_.insert(std::map<std::string,int>::value_type(this_host_ , coincidencer_server_base_port_ + this_server));

	cerr << "Coincidencer ports: " <<  endl;
	std::for_each(coincidencer_ports_.begin(), coincidencer_ports_.end(), [] (const std::pair< string, int >& pair) { cerr << "\t" << pair.first << " " << pair.second << std::endl; } );

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

int ConfigManager::read_mopsr_config(){

	char bp_config[65536];
	char bp_cornerturn_config[65536];
	int  num_beam_processors;

	cerr << "Reading MOPSR_BP_CFG from " << MOPSR_BP_CFG << endl;


	if (fileread (MOPSR_BP_CFG, bp_config, 65536) < 0){
		cerr<< "Cannot read file: " << MOPSR_BP_CFG << endl;
		return EXIT_FAILURE;
	}

	cerr << "Reading MOPSR_BP_CORNERTURN_CFG from " << MOPSR_BP_CORNERTURN_CFG << endl;


	if (fileread (MOPSR_BP_CORNERTURN_CFG, bp_cornerturn_config, 65536) < 0){
		cerr<< "Cannot read file: " << MOPSR_BP_CORNERTURN_CFG << endl;
		return EXIT_FAILURE;
	}


	if (ascii_header_get (bp_config , "NUM_BP", "%d", &num_beam_processors) != 1) {
		cerr<< "Cannot read NUM_BP from file: " << MOPSR_BP_CFG << endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_get (bp_cornerturn_config , "NBEAM", "%d", &ConfigManager::total_num_fanbeams_) != 1) {
		cerr<< "Cannot read NBEAM from file: " << MOPSR_BP_CORNERTURN_CFG << endl;
		return EXIT_FAILURE;
	}

	vector<string> all_nodes;
	all_nodes.insert(std::end(all_nodes),std::begin(other_active_nodes_), std::end(other_active_nodes_));
	all_nodes.push_back(this_host_);

	for(string inode: all_nodes){

		vector<int> beam_processors;

		for( int i = 0; i < num_beam_processors; i++){

			char node[1024];

			stringstream bp;
			bp << "BP_" << i;

			if (ascii_header_get (bp_config , bp.str().c_str(), "%s", &node[0]) != 1) {
				cerr<< "Cannot read "<< bp.str().c_str() << " from file: " << MOPSR_BP_CFG << endl;
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
				cerr<< "Cannot read " << ss.str().c_str() << " from file: " << MOPSR_BP_CORNERTURN_CFG << endl;
				return EXIT_FAILURE;
			}

			ss.str(" ");
			ss.clear();

			ss << "BEAM_LAST_RECV_" << bp;
			if (ascii_header_get (bp_cornerturn_config , ss.str().c_str(), "%d", &last) != 1) {
				cerr<< "Cannot read " << ss.str().c_str() << " from file: " << MOPSR_BP_CORNERTURN_CFG << endl;
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



	cerr << "Reading MOPSR_CFG from " << MOPSR_CFG << endl;

	char config[65536];

	if (fileread (MOPSR_CFG, config, 65536) < 0){
		cerr<< "Cannot read file: " << MOPSR_CFG << endl;
		return EXIT_FAILURE;
	}



	if (ascii_header_get (config , "SMIRF_COINCIDENCER_SERVER", "%d", &ConfigManager::coincidencer_server_base_port_) != 1) {
		cerr<< "Cannot read SMIRF_COINCIDENCER_SERVER from file: " << MOPSR_CFG << endl;
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

	char config[65536];

	if (fileread (SMIRF_CFG, config, 65536) < 0){
		cerr<< "Cannot read file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}

	int num_nodes;

	if (ascii_header_get (config , "NUM_NODES", "%d", &num_nodes) != 1) {
		cerr<< "Cannot read NUM_BP from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;
	}


	for( int i=0; i < num_nodes; i++ ) {

		stringstream node_status_stream;
		node_status_stream << "NODE_STATE_" << setfill('0') << setw(2) << i;

		char value[1024];

		if (ascii_header_get (config , node_status_stream.str().c_str(), "%s", &value) != 1) {

			cerr<< "Cannot read " << node_status_stream.str() << " from file: " << SMIRF_CFG << endl;
			return EXIT_FAILURE;

		}


		if( string(value) == "active" ) {

			value[0] = '\0';

			stringstream node_name_stream;
			node_name_stream << "NODE_" << setfill('0') << setw(2) << i;


			if (ascii_header_get (config , node_name_stream.str().c_str(), "%s", &value) != 1) {

				cerr<< "Cannot read " << node_name_stream.str() << " from file: " << SMIRF_CFG << endl;
				return EXIT_FAILURE;

			}
			other_active_nodes_.push_back(value);

		}

	}

	vector<string>::iterator it = std::find(other_active_nodes_.begin(), other_active_nodes_.end(), this_host_ );

	if( it == other_active_nodes_.end()) {

		cerr<< " Current node " << this_host_ << " is not set active on the file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	} else{
		other_active_nodes_.erase(it);
	}

	cerr << "The other active nodes are: " << endl;

	for( int i=0; i< other_active_nodes_.size(); i++ ) 	 cerr << "\t " << other_active_nodes_[i] << endl;

	char edge_node[1024];


	if (ascii_header_get (config , "EDGE_NODE", "%s", &edge_node[0]) != 1) {

		cerr<< "Cannot read " << "EDGE_NODE" << " from file: " << SMIRF_CFG << endl;
		return EXIT_FAILURE;

	}

	ConfigManager::edge_node_ = edge_node;
	cerr << "Edge node is: " << ConfigManager::edge_node_ << endl;

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
					<< BEAM_PROCESSOR_PREFIX
					<< std::setfill('0')
					<< std::setw(2)
					<< bp
					<< PATH_SEPERATOR
					<< utc
					<< PATH_SEPERATOR
					<< FB_DIR
					<< PATH_SEPERATOR
					<< BEAM_DIR_PREFIX
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


