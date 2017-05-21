/*
 * filreader.cpp
 *
 *  Created on: Aug 22, 2016
 *      Author: vivek
 */
#include <vector>
#include <fstream>
#include <sstream>
#include <string>
#include <string.h>
#include "../include/vivek/filutil.hpp"
#include <iostream>
#include <cstring>



vivek::Filterbank::Filterbank(std::string file_name, std::string mode, bool verbose)
 	 	 	 	 	 	 	 	 	 :file_name(file_name),verbose(verbose),mode(mode){

	read_header_keys();

	if(mode == VIRTUALFIL) {
		std::cerr<< "Created virtual filterbank. " << std::endl;
		return;
	}

	if (file_open(&file, file_name.c_str(),mode.c_str()) != EXIT_SUCCESS) exit(EXIT_FAILURE);

	if(this->verbose) std::cerr<< "Opening file:" << file_name <<  " in " << mode <<" mode."<< std::endl;

	if(mode ==FILREAD){
		int header_size =  read_header();
		if(!(header_size)){
			std::cerr<< "ERROR:  could not read header from  "<<  file_name << std::endl;
		}
	}
	else if(mode == FILWRITE){

	}


}

vivek::Filterbank::~Filterbank(){

	for(std::vector<vivek::HeaderParamBase*>::iterator it = header_params.begin(); it != header_params.end(); ++it) delete *it;

	delete[] data;
	if(mode !=VIRTUALFIL) fclose(file);

}


int vivek::Filterbank::read_header_keys(){

	std::ifstream header_keys_file(HEADER_KEYS_FILE);
	std::string line;

	while(std::getline(header_keys_file,line)){
		std::istringstream line_Stream(line);
		std::string key;
		std::string dtype;
		if(!(line_Stream >> key >> dtype)){
			fprintf(stderr,"Error parsing header_keys file at this line: %s \n",line.c_str());
			break;
		}
		if(!dtype.compare(std::string(INT))){
			header_params.push_back( new HeaderParam<int>(key,dtype));
		}
		else if(!dtype.compare(std::string(DOUBLE))){
			header_params.push_back( new HeaderParam<double>(key,dtype));
		}
		else if(!dtype.compare(std::string(STRING))){
			header_params.push_back( new HeaderParam<char*>(key,dtype));
		}
		else if(!dtype.compare(std::string(NULL_STR))){
			header_params.push_back( new HeaderParam<char*>(key,dtype));
		}

	}


	return EXIT_SUCCESS;

}

vivek::HeaderParamBase* vivek::Filterbank::get_header_param(std::string key){
	for(std::vector<vivek::HeaderParamBase*>::iterator it = header_params.begin(); it != header_params.end(); ++it) {
		if((*it)->key == key) return *it;
	}
	return NULL;

}

vivek::HeaderParamBase* vivek::Filterbank::get_header_param(const char* key){
	for(std::vector<vivek::HeaderParamBase*>::iterator it = header_params.begin(); it != header_params.end(); ++it) {
		if(!strcmp((*it)->key.c_str(), key)){
			return *it;
		}
	}
	return NULL;
}

void vivek::Filterbank::remove_header_param(const char* key){
	for(std::vector<vivek::HeaderParamBase*>::iterator it = header_params.begin(); it != header_params.end(); ++it) {
		if(!strcmp((*it)->key.c_str(), key)){
			header_params.erase(it);
			return;
		}
	}
}
int vivek::Filterbank::load_all_data(){

	if(mode !=  std::string((char*)FILREAD)){
		std::cerr<<" fil not opened in read mode aborting now. " << std::endl;
		return EXIT_FAILURE;
	}

	unsigned long nbytes  = this->nsamps * this->nchans * this->nbits/8.0;

	this->data = new_and_check<unsigned char>(nbytes," Loading all data");

	if(this->verbose) fprintf(stderr," reading %lu bytes \n",nbytes);

	return read_nbytes(nbytes,data);
}

int vivek::Filterbank::read_num_samples(int ngulp, unsigned char* data){
	int nchans = get_value_for_key<int>(NCHANS);
	int nifs = get_value_for_key<int>(NIFS);
	size_t count = fread(data,sizeof(unsigned char),nchans*nifs*ngulp,file);

	if(this->verbose) std::cerr <<"bytes read:" <<count << " req:" << nchans<<"*"<<nifs<<"*"<< ngulp << std::endl;

	return check_size(nchans*nifs*ngulp,count);
}

int vivek::Filterbank::read_nbytes(long nbytes, unsigned char* data){
	size_t count = fread(data,sizeof(unsigned char),nbytes,file);
	return check_size( nbytes,count );
}

int vivek::Filterbank::read_num_time(double ntime, unsigned char*data){
	int ngulp = ntime /  get_value_for_key<double>(TSAMP);
	return read_num_samples(ngulp,data);

}

int vivek::Filterbank::skip_samples(long num_samples){
	int nchans = get_value_for_key<int>(NCHANS);
	int nifs = get_value_for_key<int>(NIFS);
	long bytes_to_skip = num_samples*nchans*nifs;
	return !fseek(file,bytes_to_skip,SEEK_CUR);
}

int vivek::Filterbank::skip_bytes(long num_bytes){

	return !fseek(file,num_bytes,SEEK_CUR);
}

int vivek::Filterbank::skip_time(double time_sec){
	int num_samples = time_sec /  get_value_for_key<double>(TSAMP);
	return skip_samples(num_samples);
}

int vivek::Filterbank::goto_sample(int sample_number){
	long bytes_now = ftell(file);
	int nchans = get_value_for_key<int>(NCHANS);
	int nifs = get_value_for_key<int>(NIFS);
	int nbits = get_value_for_key<int>(NBITS);
	int nbytes = nbits/8;
	long bytes_to_go = sample_number*nchans*nifs*nbytes + this->header_bytes;
	int diff = bytes_now - bytes_to_go;
	return !fseek(file,diff,SEEK_CUR);
}
int vivek::Filterbank::goto_timestamp(double timestamp_sec){
	int sample_number = timestamp_sec /  get_value_for_key<double>(TSAMP);
	return goto_sample(sample_number);
}

/* most functions assume 8 bit filterbanks. Could not care less for a general solution*/
int vivek::Filterbank::store_data(unsigned char* data, unsigned long start_sample, unsigned long num_samples){

	int nchans = get_value_for_key<int>(NCHANS);
	int nifs = get_value_for_key<int>(NIFS);

	unsigned long bytes_to_copy = num_samples* nchans* nifs;
	unsigned long byte_to_start = start_sample*nchans*nifs;

	if(this->verbose) fprintf(stderr, "have = %ld copying = %ld  from byte = %ld \n", this->data_bytes , bytes_to_copy, byte_to_start);

	std::memcpy(&this->data[data_bytes],&data[byte_to_start],sizeof(unsigned char)*bytes_to_copy);
	data_bytes +=bytes_to_copy;

	if(this->verbose) fprintf(stderr, "new data_bytes: %ld \n", data_bytes);
	return EXIT_SUCCESS;
}

int vivek::Filterbank::alloc_data(){
	unsigned long size = this->nsamps * this->nchans *this->nbits/8.0;
	this->data = new_and_check<unsigned char>(size," Loading all data");
	return EXIT_SUCCESS;
}

int vivek::Filterbank::unload_filterbank(){

	if(this->verbose) std::cerr<< "writing header.." <<std::endl;
	this->write_header();

	if(this->verbose)std::cerr<< "writing data.." <<std::endl;
	this->write_data();

	if(this->verbose) std::cerr<< "success!" <<std::endl;
	return EXIT_SUCCESS;

}

int vivek::Filterbank::read_header(){

	rewind(file);

	int iter=0;
	bool header_started = false;
	while(1){
		iter++;
		int num_bytes = read_int();
		char header_key_bytes[num_bytes+1];
		if(read_num_bytes(num_bytes,&header_key_bytes[0])){

			if(!strcmp(header_key_bytes,HEADER_START)) {
				vivek::HeaderParamBase* header_param = get_header_param(header_key_bytes);
				char* dummy = "NULL";
				static_cast<HeaderParam< char*> * > (header_param)->value = dummy;
				header_started = true;
				continue;
			}
			if(header_started) {
				if(!strcmp(header_key_bytes,HEADER_END)) {
					vivek::HeaderParamBase* header_param = get_header_param(header_key_bytes);
					char* dummy = "NULL";
					static_cast<HeaderParam< char*> * > (header_param)->value = dummy;
					header_started = false;
					header_bytes = ftell(file);
					int inp_num;
					inp_num = fileno(file);
					if((fstat(inp_num,&filestat)) < 0) { std::cerr<< "ERROR:  could not fstat file: " <<  file_name <<std::endl; break; }
					data_bytes = filestat.st_size - header_bytes;
					int nchans = get_value_for_key<int>(NCHANS);
					int nbits = get_value_for_key<int>(NBITS);
					int nifs = get_value_for_key<int>(NIFS);
					double tsamp = get_value_for_key<double>(TSAMP);
					int nbytes = nbits/8;
					long nsamples =data_bytes/(nchans*nbytes*nifs);
					double tobs = nsamples*tsamp;
					add_to_header<long>(NSAMPLES,LONG,nsamples);
					add_to_header<double>(TOBS,DOUBLE,tobs);

					this->nchans = nchans;
					this->nbits = nbits;
					this->nsamps = nsamples;
					this->tsamp = tsamp;
					this->fch1 = get_value_for_key<double>(FCH1);
					this->foff = get_value_for_key<double>(FOFF);
					break;
				}

				else{
					std::string header_key(&header_key_bytes[0]);
					vivek::HeaderParamBase* header_param = get_header_param(header_key_bytes);
					std::string dtype = header_param->dtype;
					header_param->inheader = true;
					if(dtype== std::string(INT)) {
						static_cast<HeaderParam<int> * > (header_param)->value = read_int();

					}
					else if(dtype== std::string(DOUBLE)){
						static_cast<HeaderParam<double> * > (header_param)->value = read_double();

					}
					else if(dtype== std::string(STRING)){
						num_bytes = read_int();
						if(num_bytes == 0) {
							char* dummy = "NULL";
							static_cast<HeaderParam< char*> * > (header_param)->value = dummy;
							continue;
						}
						char* header_value_bytes = new char[num_bytes+1];
						if(read_num_bytes(num_bytes,header_value_bytes)){
							static_cast<HeaderParam< char*> * > (header_param)->value = header_value_bytes;
						}
					}
					else if(dtype== std::string(NULL_STR)){
						num_bytes = read_int();
						if(num_bytes == 0) {
							char* dummy = "NULL";
							static_cast<HeaderParam< char*> * > (header_param)->value = dummy;
							continue;
						}
					}
				}
			}
		}



	}

	return header_bytes;
}

int vivek::Filterbank::write_header(){
	if(mode !=  std::string((char*)FILWRITE) && mode !=  std::string((char*)VIRTUALFIL)){
		std::cerr<<" File not opened in write mode aborting now. " << std::endl;
		return EXIT_FAILURE;
	}

	int header_start_length = strlen(HEADER_START);
	fwrite(&header_start_length,sizeof(int),1,file);
	fwrite(HEADER_START,sizeof(char),header_start_length,file);
	for(std::vector<vivek::HeaderParamBase*>::iterator it = header_params.begin(); it != header_params.end(); ++it) {
		vivek::HeaderParamBase* base = *it;

		const char* key = (*it)->key.c_str();
		if(!strcmp(key,HEADER_START) || !strcmp(key,HEADER_END) || !strcmp(key,NSAMPLES) || !strcmp(key,TOBS) || !base->inheader ) continue;
		int key_length = strlen(key);
		fwrite(&key_length,sizeof(int),1,file);
		fwrite(key,sizeof(char),key_length,file);

		if(base->dtype == std::string(INT)) {
			int value  = (static_cast<HeaderParam< int> * > (base))->value;
			fwrite(&value,sizeof(value),1,file);

		}
		else if(base->dtype == std::string(DOUBLE)) {
			double value  = (static_cast<HeaderParam< double> * > (base))->value;
			fwrite(&value,sizeof(value),1,file);
		}
		else if(base->dtype == std::string(STRING)) {
			char* value  = (static_cast<HeaderParam< char*> * > (base))->value;
			if(value!=NULL){
				int value_length = strlen(value);
				fwrite(&value_length,sizeof(int),1,file);
				fwrite(value,sizeof(*value),value_length,file);
			}
			else {
				int value_length = 0;
				fwrite(&value_length,sizeof(value_length),1,file);
			}

		}
		else if(base->dtype == std::string(NULL_STR)) {
			int null=0;
			fwrite(&null,sizeof(null),1,file);
		}


	}

	int header_end_length = strlen(HEADER_END);
	fwrite(&header_end_length,sizeof(int),1,file);
	fwrite(HEADER_END,sizeof(char),header_end_length,file);

}

int vivek::Filterbank::write_data(unsigned char* data, long length){
	fwrite(data,sizeof(char),length,file);
	return EXIT_SUCCESS;
}

int vivek::Filterbank::write_data(){
	std::cerr << "Trying to write: " << data_bytes << "bytes to " << file << std::endl;

	//for(unsigned long i=0;i<data_bytes; i++) std::cerr << i << " "<< data[i] << std::endl;

	fwrite(data,sizeof(unsigned char),data_bytes,file);
	return EXIT_SUCCESS;
}



int vivek::Filterbank::rewind_to_data_start(){
	fseek(this->file, this->header_bytes, SEEK_SET);
	std::cerr<< "ftell:" << ftell(this->file)<<std::endl;
	return EXIT_SUCCESS;
}
int vivek::Filterbank::rewind_to_file_start(){
	rewind(this->file);
	return EXIT_SUCCESS;
}

void vivek::Filterbank::print_header(){
	for(std::vector<vivek::HeaderParamBase*>::iterator it = header_params.begin(); it != header_params.end(); ++it) {
		vivek::HeaderParamBase* base = *it;
		if(!base->inheader) continue;
		std::cerr << base->key << "(" << base->dtype << "): " ;
		if(base->dtype == std::string(INT)) {
			std::cerr <<  (static_cast<HeaderParam<int> * > (base))->value   << std::endl;
		}
		else if(base->dtype == std::string(LONG)) {
			std::cerr <<  (static_cast<HeaderParam<long> * > (base))->value   << std::endl;
		}
		else if(base->dtype == std::string(DOUBLE)) {
			double value =  (static_cast<HeaderParam<double> * > (base))->value;
			if(base->key == std::string(SRC_RAJ)){
				char* str_value = new_and_check<char>(100,"print_header");
				sigproc_to_hhmmss(value, str_value);
				std::cerr<< str_value <<std::endl;
			}
			else if(base->key == std::string(SRC_DEJ)){
				char* str_value = new_and_check<char>(100,"print_header");
				sigproc_to_ddmmss(value, str_value);
				std::cerr<< str_value <<std::endl;
			}
			else{
				std::cerr<< value <<std::endl;
			}

		}
		else if(base->dtype == std::string(STRING)) {
			if((static_cast<HeaderParam< char*> * > (base))->value !=NULL)
				std::cerr <<  (static_cast<HeaderParam< char*> * > (base))->value   << std::endl;
		}
		else if(base->dtype == std::string(NULL_STR)) {
			std::cerr << "NULL"  << std::endl;
		}


	}
}


void vivek::Filterbank::pretty_print_header(){

}

int vivek::Filterbank::read_num_bytes(int nbytes, char* bytes){
	size_t count = fread(bytes,nbytes,1,file);
	bytes[nbytes] = '\0';
	return count;

}

int vivek::Filterbank::read_int(){
	int result;
	size_t count = fread(&result,sizeof(result),1,file);
	if(count)
		return result;
	else
		return EXIT_FAILURE;
}

double vivek::Filterbank::read_double(){
	double result;
	size_t count = fread(&result,sizeof(result),1,file);
	if(count)
		return result;
	else
		return EXIT_FAILURE;
}



int vivek::Filterbank::read_num_bytes_to_read(){
	int nbytes = 4;
	unsigned char bytes[nbytes];
	size_t count = fread(&bytes,nbytes,1,file);
	if(count)
		return (bytes[0]+bytes[1]*256 + bytes[2]*65536 + bytes[3]*16777216);
	else
		return EXIT_FAILURE;
}


double vivek::Filterbank::get_rms(){
	double sumsq = 0.0;
	double sum =0.0;
	double mean,meansq;
	for(int loop1=0;loop1<data_bytes;loop1++)
	{
		sumsq+=pow(this->data[loop1],2);
		sum+=this->data[loop1];
	}
	mean  = sum/(double)data_bytes;
	meansq = sumsq/(double)data_bytes;
	float rms = sqrt(meansq-mean*mean);
	return rms;
}


double vivek::Filterbank::get_mean(){
	double sum=0.0,mean=0;
	for(int loop1=0;loop1<data_bytes;loop1++){
		sum+=this->data[loop1];
	}
	mean = sum/data_bytes;
	return mean;
}

