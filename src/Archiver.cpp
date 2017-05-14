/*
 * Archiver.cpp
 *
 *  Created on: Mar 15, 2017
 *      Author: vivek
 */

#include "vivek/Archiver.h"
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "utc.h"
#include "MJD.h"



using namespace std;

key_t vivek::Archiver::out_key = -1;
dada_hdu_t* vivek::Archiver::out_hdu = nullptr;
unsigned int vivek::Archiver::nbuffers = -1;
unsigned long vivek::Archiver::buffer_size = 0;

vivek::Archiver::Archiver() {
}

void vivek::Archiver::handle_archiver_segfault(int signal){

	cerr << "Handling seg fault " << endl;

	if(out_hdu != nullptr) {
		cerr << "unlocking out_hdu " << endl;

		if (dada_hdu_unlock_write (out_hdu) < 0) {
			std::cerr << "close: cannot unlock output HDU" << std::endl;
			std::exit(EXIT_FAILURE);
		}
	}
}

int vivek::Archiver::transfer_fil_to_DADA_buffer(string utc, vivek::Filterbank* f, bool final){

	out_hdu = dada_hdu_create (0);

	dada_hdu_set_key(out_hdu, out_key);

	if (dada_hdu_connect (out_hdu) < 0) {
		cerr<<  "ERROR: could not connect to output HDU" << endl;
		return EXIT_FAILURE;
	}
	std::cerr << "connected to output HDU" <<std::endl;
	if (dada_hdu_lock_write (out_hdu) < 0){
		cerr << "open: could not lock write on output HDU\n" << endl;
		return EXIT_FAILURE;
	}
	std::cerr << "locked for write" <<std::endl;

	char * header = ipcbuf_get_next_write (out_hdu->header_block);
	if (!header)
	{
		cerr << "open: could not get next header block " <<  endl;
		return -1;
	}

	//	for(std::vector<vivek::HeaderParamBase*>::iterator it = f->header_params.begin(); it != f->header_params.end(); ++it) {
	//		vivek::HeaderParamBase* base = *it;
	//		const char* key = base->key.c_str();
	//
	//		if(!strcmp(key,HEADER_START) || !strcmp(key,HEADER_END) ) continue;
	//
	//		if(base->dtype == std::string(INT)) {
	//			int value = ( dynamic_cast<vivek::HeaderParam<int> * > (base))->value;
	//			if(ascii_header_set(header, key, "%d", value) <0) {
	//				std::stringstream message;
	//				message << "could not set " << key <<"=" << value << " in outgoing header \n";
	//				multilog (log, LOG_ERR,message.str().c_str());
	//				return EXIT_FAILURE;
	//			}
	//		}
	//		else if(base->dtype == std::string(DOUBLE)) {
	//			double value  = ( dynamic_cast<vivek::HeaderParam<double> * > (base))->value;
	//			if(ascii_header_set(header, key, "%lf", value) <0) {
	//				std::stringstream message;
	//				message << "could not set " << key <<"=" << value << " in outgoing header \n";
	//				multilog (log, LOG_ERR,message.str().c_str());
	//				return EXIT_FAILURE;
	//			}
	//		}
	//		else if(base->dtype == std::string(STRING)) {
	//			char* value  = ( dynamic_cast<vivek::HeaderParam<char*> * > (base))->value;
	//			if(ascii_header_set(header, key, "%s", value) <0) {
	//				std::stringstream message;
	//				message << "could not set " << key <<"=" << value << " in outgoing header \n";
	//				multilog (log, LOG_ERR,message.str().c_str());
	//				return EXIT_FAILURE;
	//			}
	//		}
	//
	//
	//	}
	//	std::cerr << "wrote fil headers" <<std::endl;

	if (ascii_header_set (header, "ORDER", "%s", "TF") < 0){
		std::cerr<< "could not set ORDER=TF in outgoing header" <<std::endl;
		cerr << "could not set ORDER=TF in outgoing header" << endl;
		return EXIT_FAILURE;
	}

	char ra[100];
	sigproc_to_hhmmss(f->get_value_for_key<double>(SRC_RAJ),&ra[0]);
	if (ascii_header_set (header, "RA", "%s", ra) < 0){
		std::cerr<< "could not set RA in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}
	char dec[100];
	sigproc_to_ddmmss(f->get_value_for_key<double>(SRC_DEJ),&dec[0]);
	if (ascii_header_set (header, "DEC", "%s", dec) < 0){
		std::cerr<< "could not set DEC in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "RESOLUTION", "%d", f->get_value_for_key<int>(NBITS) ) < 0){
		std::cerr<<"could not set RESOLUTION in outgoing header\n"<<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "FILE_SIZE", "%i", (16384 + f->data_bytes) )< 0) {
		std::cerr<< "could not set FILE_SIZE=%i in outgoing header\n" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "NANT", "%d", 1) < 0){
		std::cerr<< "could not set NANT=1 in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "HDR_VERSION", "%lf", 1.0) < 0){
		std::cerr<< "could not set HDR_VERSION=1.0 in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "HDR_SIZE", "%d", 16384) < 0){
		std::cerr<< "could not set HDR_SIZE=16384 in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "TELESCOPE", "%s", "mo") < 0){
		std::cerr<< "could not set TELESCOPE=mol in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "INSTRUMENT", "%s", "SigProc") < 0){
		std::cerr<< "could not set INSTRUMENT=SigProc in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "FORMAT", "%s", "SigProc") < 0){
		std::cerr<< "could not set FORMAT=SigProc in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "SOURCE", "%s", f->get_value_for_key<char*>(SOURCE_NAME) ) < 0){
		std::cerr<< "could not set SOURCE=%s in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "CALFREQ", "%f", f->get_value_for_key<double>(FCH1) + f->get_value_for_key<double>(FOFF)/2.0) < 0){
		std::cerr<< "could not set FREQ=%f in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "FREQ", "%f", f->get_value_for_key<double>(FCH1) + f->get_value_for_key<double>(FOFF)/2.0) < 0){
		std::cerr<< "could not set FREQ=%f in outgoing header" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "BW", "%f", f->get_value_for_key<int>(NCHANS)* f->get_value_for_key<double>(CHANNELBW)) < 0){
		std::cerr<< "could not set BW=%f in outgoing header\n" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "TSAMP", "%f",f->get_value_for_key<double>(TSAMP)/1.0e-6) < 0){
		std::cerr<< "could not set TSAMP=%f in outgoing header\n" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "NPOL", "%d",  f->get_value_for_key<int>(NIFS)) < 0){
		std::cerr<< "could not set NPOL=%d in outgoing header\n" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "NBIT", "%d", f->get_value_for_key<int>(NBITS)) < 0){
		std::cerr<< "NBIT =" << f->get_value_for_key<int>(NBITS) <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "NCHAN", "%d",f->get_value_for_key<int>(NCHANS)) < 0){
		std::cerr<< "could not set NCHAN="<< f->get_value_for_key<int>(NCHANS) << " in outgoing header\n" <<std::endl;
		return EXIT_FAILURE;
	}

//	MJD mjd = MJD(f->get_value_for_key<double>(TSTART));
//	struct tm date;
//	double fracsec;
//
//	mjd.gregorian (&date, &fracsec);
//	std::string utc_string =  mjd.datestr("%Y-%m-%d-%H:%M:%S");

	if (ascii_header_set (header, "UTC_START", "%s", utc.c_str() ) < 0){
		std::cerr<< "Could not set UTC_START ="<< utc.c_str() <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "OBS_OFFSET", "%d",0) < 0){
		std::cerr<< "Could not set OBS_OFFSET = 0" <<std::endl;
		return EXIT_FAILURE;
	}
	if (ascii_header_set (header, "NDIM", "%d",1) < 0){
		std::cerr<< "Could not set NDIM = 1" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "NSAMP", "%ld", f->get_value_for_key<long>(NSAMPLES) ) < 0){
		std::cerr<< "Could not set NSAMP ="<<f->get_value_for_key<long>(NSAMPLES)  <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, "STATE", "%s", "Intensity" ) < 0){
		std::cerr<< "Could not set STATE = Intensity" <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, CANDIDATE_FILENAME_KEY, "%s", f->get_value_for_key<char*>(CANDIDATE_FILENAME_KEY) ) < 0){
		std::cerr<< "Could not set"<< CANDIDATE_FILENAME_KEY <<std::endl;
		return EXIT_FAILURE;
	}

	if (ascii_header_set (header, OUT_DIR_KEY, "%s", f->get_value_for_key<char*>(OUT_DIR_KEY)  ) < 0){
		std::cerr<< "Could not set" << OUT_DIR_KEY << " =" << f->get_value_for_key<char*>(OUT_DIR_KEY)  <<std::endl;
		return EXIT_FAILURE;
	}

	if(final) {

		cerr << " Adding " << FINAL_STITCH << " true" << endl;

		if (ascii_header_set (header, FINAL_STITCH, "%s", "true"  ) < 0){
			std::cerr<< "Could not set"<< FINAL_STITCH << " =true" <<std::endl;
			return EXIT_FAILURE;
		}
	}

	if (ipcbuf_mark_filled (out_hdu->header_block, 16384) < 0)
	{
		std::cerr<< "Could not set IPCBUF MARK FILLED"  <<std::endl;
		return -1;
	}

	fprintf(stderr, "wrote headers. Now writing data: %ld \n", f->data_bytes);
	uint64_t out_block_id;
	long bytes_in = 0;
	long bytes_out = 0;
	long ptr = 0;

	while(ptr < f->data_bytes){

		long difference = f->data_bytes - ptr;
		bytes_in =  (difference > buffer_size )? buffer_size : difference;

		char* block = ipcio_open_block_write (out_hdu->data_block, &out_block_id);

		std::memcpy(block,&f->data[ptr],bytes_in);

		if(ipcio_close_block_write (out_hdu->data_block, bytes_in) <0 ) {
			cerr <<"close: ipcio_close_block_write failed" << endl;
		}

		fprintf(stderr, "writing data %ld from %ld \n", bytes_in, ptr);
		ptr += bytes_in;
	}
	std::cerr << "wrote data " <<std::endl;

	if (dada_hdu_unlock_write (out_hdu) < 0) {
		std::cerr << "close: cannot unlock output HDU" << std::endl;
		return EXIT_FAILURE;
	}

	std::cerr << "unlocked write" <<std::endl;

	return EXIT_SUCCESS;


}


vivek::Archiver::~Archiver() {

}

