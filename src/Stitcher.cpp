/*
 * Stitcher.cpp
 *
 *  Created on: Apr 21, 2017
 *      Author: vivek
 */

#include "Stitcher.hpp"
#include <cstdlib>

using namespace std;



Stitcher::~Stitcher() {

}

string get_candidate_file_name( string dir, int point_idx){

	stringstream stream;
	stream << dir << PATH_SEPERATOR << CANDIDATE_FILENAME_PREFIX;

	if(point_idx != -1)
		stream << "_" <<setfill('0') << setw(4) << point_idx;

	stream 	<< "."
			<< "BS"
			<< std::setfill('0')
	<< std::setw(2)
	<< ConfigManager::this_bs();

	return stream.str();
}


string get_candidate_file_dir( string root_dir, string ra, string dec){

	stringstream stream;
	stream << root_dir << PATH_SEPERATOR << get_long_jname_from_pos(ra,dec);

	stream 	<< "."
			<< "BS"
			<< std::setfill('0')
	<< std::setw(2)
	<< ConfigManager::this_bs();

	return stream.str();
}




vivek::Filterbank* Stitcher::stitch(UniquePoint* point, map<int,vivek::Filterbank*>* fanbeams, bool virtual_fil){


	stringstream name_stream;
	name_stream << args.out_dir<< PATH_SEPERATOR << point->ra << point->dec << ".fil";
	string out = name_stream.str();

	if(args.verbose) cerr<< "output file name:" << out <<endl;

	const char* mode = virtual_fil? VIRTUALFIL : FILWRITE;
	vivek::Filterbank* stitched_filterbank = new vivek::Filterbank(out, mode, args.verbose);
	vivek::Filterbank::copy_header(fanbeams->begin()->second,stitched_filterbank,args.verbose);

	if(virtual_fil) {
		cerr << "Setting nsamp to FFT size. " << endl;
		stitched_filterbank->set_nsamps(ConfigManager::fft_size());
	}


	double sigproc_dej, sigproc_raj;
	hhmmss_to_sigproc(point->ra.c_str(),&sigproc_raj);
	ddmmss_to_sigproc(point->dec.c_str(),&sigproc_dej);

	stitched_filterbank->set_value_for_key<double>(SRC_DEJ,sigproc_dej);
	stitched_filterbank->set_value_for_key<double>(SRC_RAJ,sigproc_raj);

	stitched_filterbank->alloc_data();

	if(args.verbose) cerr<< "successfully copied header" <<endl;

	int last_traversal = point->traversals->size() - 1;

	int traversal_num = 0;

	for(vector<Traversal*>::iterator it2 = point->traversals->begin(); it2!=point->traversals->end(); it2++, traversal_num++ ){

		Traversal* traversal = *it2;
		int fb = traversal->fanbeam;
		cerr << fb << endl;
		vivek::Filterbank* f  = fanbeams->at(fb);
		cerr << f->file_name << endl;
		stitched_filterbank->store_data(f->data, traversal->startSample, traversal->numSamples);

		//		if(traversal_num  == last_traversal ) {
		//			cerr << "last extra bits: from "<< (traversal->startSample + traversal->numSamples) << " " << f->nsamps - (traversal->startSample + traversal->numSamples);
		//			stitched_filterbank->store_data(f->data, traversal->startSample + traversal->numSamples, f->nsamps - (traversal->startSample + traversal->numSamples));
		//		}

	}

	if(virtual_fil) {
		cerr << "Setting nsamp to FFT size. " << endl;
		stitched_filterbank->set_nsamps(stitched_filterbank->data_bytes * 1.0/stitched_filterbank->nchans);
	}


	cerr<< "mean: " << stitched_filterbank->get_mean() << "  rms:" << stitched_filterbank->get_rms() <<endl;

	return stitched_filterbank;

}

vivek::Filterbank* Stitcher::stitch(UniquePoint* point, bool virtual_fil){

	vector<int> uniqFBs;
	for(vector<Traversal*>::iterator it = point->traversals->begin(); it!=point->traversals->end(); it++){

		Traversal* t = *it;
		if(find(uniqFBs.begin(), uniqFBs.end(),(int)t->fanbeam)== uniqFBs.end()) uniqFBs.push_back((int)t->fanbeam);

	}

	cerr << " uniqFBs->size(): " << uniqFBs.size() << endl;

	map<int,vivek::Filterbank*> fanbeams;

	for( vector<int>::iterator fbIterator = uniqFBs.begin(); fbIterator != uniqFBs.end(); fbIterator++){
		int fb = (int)*(fbIterator);

		string fb_abs_path = ConfigManager::get_fil_file_path(args.archives_base,args.utc,fb,args.no_bp_structure );

		if(fb_abs_path.empty()) {
			cerr<< "Problem loading fb: " <<  fb << "fil file not found. Aborting now.";
			return nullptr;
		}

		vivek::Filterbank* f = new vivek::Filterbank(fb_abs_path,FILREAD,args.verbose);
		f->load_all_data();

		cerr<< "mean: " << f->get_mean() << "  rms:" << f->get_rms() <<endl;

		fanbeams.insert(map<int,vivek::Filterbank*>::value_type(fb,f));

	}

	vivek::Filterbank* stitched_filterbank = stitch(point,&fanbeams, virtual_fil);

	for (map<int,vivek::Filterbank*>::iterator it=fanbeams.begin(); it!=fanbeams.end(); ++it) delete it->second;

	return stitched_filterbank;

}



int Stitcher::stitch_and_dump(vector<UniquePoint*>* uniqPoints, vector<int>* uniqFBs){

	map<int,vivek::Filterbank*> fanbeams;

	for( vector<int>::iterator fbIterator = uniqFBs->begin(); fbIterator != uniqFBs->end(); fbIterator++){
		int fb = (int)*(fbIterator);

		string fb_abs_path = ConfigManager::get_fil_file_path(args.archives_base,args.utc,fb, args.no_bp_structure);

		if(fb_abs_path.empty()) {
			cerr<< "Problem loading fb: " <<  fb << "fil file not found. Aborting now.";
			return EXIT_FAILURE;
		}

		vivek::Filterbank* f = new vivek::Filterbank(fb_abs_path,FILREAD,args.verbose);
		f->load_all_data();

		if(args.verbose) cerr<< "mean: " << f->get_mean() << "  rms:" << f->get_rms() <<endl;

		fanbeams.insert(map<int,vivek::Filterbank*>::value_type(fb,f));

	}
	cerr<< "loaded all FB" << fanbeams.size()<<endl;

	for(vector<UniquePoint*>::iterator upIterator = uniqPoints->begin(); upIterator!=uniqPoints->end();++upIterator){
		if(args.verbose) cerr<< "next point " <<endl;

		UniquePoint* point = *upIterator;

		vivek::Filterbank* stitched_filterbank = stitch(point,&fanbeams,false);

		if(stitched_filterbank == nullptr) return EXIT_FAILURE;

		cerr << "Unloading filterbank.." << endl;

		stitched_filterbank->unload_filterbank();

		delete stitched_filterbank;
	}


	for (map<int,vivek::Filterbank*>::iterator it=fanbeams.begin(); it!=fanbeams.end(); ++it) delete it->second;

	return EXIT_SUCCESS;

}

/**
 * Implement this the inefficient way to save memory over processing time.
 */
int Stitcher::stitch_and_transfer(vector<UniquePoint*>* uniqPoints, key_t out_key,
		string candidates_file, string dspsr_out_dir){

	// std::signal(SIGSEGV, vivek::Archiver::handle_archiver_segfault);
	vivek::Archiver::out_key =  out_key;
	vivek::Archiver::nbuffers = ConfigManager::shared_mem_nbuffers();
	vivek::Archiver::buffer_size = ConfigManager::shared_mem_buffer_size();

	vector<string> lines;

	FILE* fp;

	file_open(&fp,candidates_file.c_str(),"r");

	loadlines(candidates_file.c_str(),lines);

	string header = "";

	for(string line : lines) if(line.find("SOURCE") != line.npos)  header = line;

//	int line_number = 1;

	map<UniquePoint*, vector<string> > points_with_candidates;

	for(vector<UniquePoint*>::iterator points_it = uniqPoints->begin(); points_it!=uniqPoints->end();++points_it ){
		UniquePoint* point = *points_it;

		string ra = point->ra;
		string dec = point->dec;

		vector<string> candidates;

		for( vector<string>::iterator lines_it = lines.begin(); lines_it != lines.end(); ++lines_it ) {

			string line = *lines_it;
			if( line.find(ra)!= line.npos && line.find(dec)!= line.npos ){

				string source, ra, dec;
				float period, dm, acceleration, snr,folded_snr,nh;

				stringstream ss;
				ss << line;
				ss >> source >> ra >> dec  >> period >>  dm >> acceleration >> snr >> folded_snr >> nh;

				if(	period > 0 && snr >= 9.5 && folded_snr > 0)
					candidates.push_back(line);


			}
		}

		cerr<< "Point " << get_long_jname_from_pos(point->ra, point->dec) << " has " << candidates.size() << " >10 sigma candidates" <<  endl;

		if(!candidates.empty()){

			points_with_candidates.insert(map<UniquePoint*, vector<string> >::value_type(point,candidates));

		}

	}

	cerr << points_with_candidates.size() << " points have candidates" << endl;

	int point_number = 1;

	for(auto &kv: points_with_candidates){

		UniquePoint* point = kv.first;
		vector<string> candidates = kv.second;

		cerr << "Candidates for point: (" << point_number <<") " << get_long_jname_from_pos(point->ra, point->dec) << ":" << endl;
		for( string s: candidates) cerr << "\t" << s << endl;


		string file_name = get_candidate_file_name(args.out_dir, point_number).c_str();
		cerr << "Writing candidate file: " << file_name << " for dspsr." << endl;

		ofstream candidates_file_stream(file_name.c_str() , ofstream::out);

		if (!candidates_file_stream.good()){

			cerr<< "could not open '" << file_name << "' in mode '"
					<< "w " << "' Errorno: " << strerror(errno) << endl;
			return EXIT_FAILURE;

		}

		candidates_file_stream << header << endl;

		std::ostream_iterator<std::string> output_iterator(candidates_file_stream, "\n");
		std::copy(candidates.begin(), candidates.end(), output_iterator);

		candidates_file_stream.close();

		if(!file_exists(file_name)){
			cerr << " File: " << file_name << " Not written properly." <<endl;
			return EXIT_FAILURE;
		}

		vivek::Filterbank* stitched_filterbank = stitch(point, true);

		if(stitched_filterbank == nullptr) {
			cerr << "Problem creating stitched filterbank." << endl;
			return EXIT_FAILURE;
		}

		stitched_filterbank->add_to_header(CANDIDATE_FILENAME_KEY,STRING,file_name);
		stitched_filterbank->add_to_header(OUT_DIR_KEY,STRING,get_candidate_file_dir(dspsr_out_dir,point->ra, point->dec));

		bool final = false;
		if( point_number == points_with_candidates.size() ) {

			cerr << "Processing final point" << endl;
			final = true;

		}

		int result = vivek::Archiver::transfer_fil_to_DADA_buffer(args.utc,stitched_filterbank, final);

		if(result != EXIT_SUCCESS ) return EXIT_FAILURE;


		delete stitched_filterbank;
		point_number++;

	}

	return EXIT_SUCCESS;

}

