/*
 * Filheader.h
 *
 *  Created on: Aug 22, 2016
 *      Author: vivek
 */

#ifndef FILUTIL_H_
#define FILUTIL_H_
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <sys/types.h>
#include <sys/stat.h>
#include "utilities.hpp"
#include "HeaderParam.hpp"
#include "filterbank_def.hpp"
namespace vivek {
class Filterbank;
}
class vivek::Filterbank{
private:

public:
	FILE* file;
	std::vector<HeaderParamBase*> header_params;
	std::string file_name;
	std::string mode;
	unsigned long header_bytes;
	unsigned long data_bytes;
	struct stat filestat;
	Filterbank(std::string file_name, std::string mode,bool verbose);
	unsigned char* data;
	unsigned long nsamps;
	unsigned int nchans;
	unsigned char nbits;
	float fch1;
	float foff;
	float tsamp;
	bool verbose;

	int read_header_keys();
	int read_header();
	void print_header();
	void pretty_print_header();

	int read_num_samples(int ngulp, unsigned char* data);
	int read_num_time(double duration, unsigned char* data);
	int read_nbytes(long nbytes, unsigned char*data);

	int skip_samples(long nsamples);
	int skip_bytes(long bytes);
	int skip_time(double time_sec);

	int goto_sample(int sample_number);
	int goto_timestamp(double timestamp_sec);
	int goto_byte(long byte);

	int write_header();
	int write_data();
	int write_data(unsigned char* data, long length);
	int store_data(unsigned char* data, unsigned long start_sample, unsigned long num_samples);

	int alloc_data();
	int rewind_to_data_start();
	int rewind_to_file_start();
	int unload_filterbank();


	int load_all_data();
	double get_mean();
	double get_rms();

	void plot_data(long start_sample, long num_samples, bool fscrunch);


	HeaderParamBase* get_header_param(std::string key);
	HeaderParamBase* get_header_param(const char* key);
	double get_double_value_for_key(std::string key);
	void remove_header_param(const char* key);

	template <typename T>  inline T get_value_for_key(const char* key){
		HeaderParamBase* base = get_header_param(key);
		return static_cast<HeaderParam<T>*>(base)->value;
	}

	template <typename T> inline void set_value_for_key(const char* key, T value){
		HeaderParamBase* base = get_header_param(key);
		if(base!=NULL){
			static_cast<HeaderParam< T> * > (base)->value = value;
		}
	}


	static int get_zero_dm_time_series(Filterbank* f, unsigned char* timeseries){

		for(int i=0; i< f->nsamps; i++){
			int add;
			for(int j=0; j< f->nchans; j++ ){
				add += f->data[i* f->nchans  +  j];
			}
			timeseries[i] = (unsigned char) ( add ) ;
		}

		return EXIT_SUCCESS;

	}



	static int copy_header(Filterbank* from, Filterbank* to, bool verbose){

		if(to->mode == std::string((char*)FILREAD) ){
			std::cerr << "Unable to write to Filterbank"<< to->file_name << "Reason: Filterbank read only.";
			return EXIT_FAILURE;
		}

		if(verbose) std::cerr<< "Attempting to copy header from:" << from->file_name << " to: " << to->file_name <<std::endl;

		to->header_params.clear();
		to->data_bytes = to->header_bytes = 0;

		for(std::vector<HeaderParamBase*>::iterator it = from->header_params.begin(); it != from->header_params.end(); ++it) {
			HeaderParamBase* from_base = *it;
			std::string dtype = from_base->dtype;
			if(dtype== std::string(INT)) {
				to->header_params.push_back(new HeaderParam<int>(from_base));

			}
			else if(dtype== std::string(DOUBLE)){
				to->header_params.push_back(new HeaderParam<double> (from_base));

			}
			else if(dtype== std::string(LONG)){
				to->header_params.push_back(new HeaderParam<long> (from_base));

			}
			else if(dtype== std::string(STRING)){
				to->header_params.push_back(new HeaderParam<char*> (from_base));
			}
			else if(dtype== std::string(NULL_STR)){
				to->header_params.push_back(new HeaderParam<char*> (from_base));

			}

		}
		int nchans = to->get_value_for_key<int>(NCHANS);
		int nbits = to->get_value_for_key<int>(NBITS);
		int nifs = to->get_value_for_key<int>(NIFS);
		double tsamp = to->get_value_for_key<double>(TSAMP);
		int nbytes = nbits/8;
		long nsamples =from->data_bytes/(nchans*nbytes*nifs);
		to->nchans = nchans;
		to->nbits = nbits;
		to->nsamps = nsamples;
		to->tsamp = tsamp;
		to->fch1 = to->get_value_for_key<double>(FCH1);
		to->foff = to->get_value_for_key<double>(FOFF);
		to->header_bytes = from->header_bytes;
		return EXIT_SUCCESS;

	}

	template <typename T>  inline int add_to_header(std::string key,std::string dtype, T value){
		header_params.push_back(new HeaderParam<T>(key,dtype,value));
		return EXIT_SUCCESS;
	}

	~Filterbank();
	float get_tsamp(void){return tsamp;}
	void set_tsamp(float tsamp){this->tsamp = tsamp;}
	float get_foff(void){return (float)foff;}
	void set_foff(float foff){this->foff = foff;}
	float get_fch1(void){return (float)fch1;}
	void set_fch1(float fch1){this->fch1 = fch1;}
	float get_nchans(void){return (float)nchans;}
	void set_nchans(unsigned int nchans){this->nchans = nchans;}
	unsigned int get_nsamps(void){return (unsigned int)nsamps;}
	void set_nsamps(unsigned int nsamps){
		this->nsamps = nsamps;
		set_value_for_key(NSAMPLES,nsamps);
		if(this->mode == FILREAD) {
		this->data_bytes = nsamps * this->nchans * this->get_value_for_key<int>(NIFS) * this->nbits / 8.0;
		}
		fprintf(stderr, " setting nsamples = %ld and data_bytes = %ld", this->nsamps, this->data_bytes );
	}
	int get_nbits(void){return (int)nbits;}
	void set_nbits(unsigned char nbits){this->nbits = nbits;}
	unsigned char* get_data(void){return this->data;}
	void set_data(unsigned char *data){this->data = data;}
	float get_cfreq(void)
	{
		if (foff < 0)
			return fch1+foff*nchans/2;
		else
			return fch1-foff*nchans/2;
	}


	const std::vector<HeaderParamBase*>& getHeaderParams() const {
		return header_params;
	}

	void setHeaderParams(const std::vector<HeaderParamBase*>& headerParams) {
		header_params = headerParams;
	}

protected:
	int read_num_bytes_to_read();
	int read_num_bytes(int num_bytes, char* bytes);
	int read_int();
	double read_double();

};



#endif /* FILUTIL_H_ */
