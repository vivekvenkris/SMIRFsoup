/*
 * utilityfns.h
 *
 *  Created on: 8 Jul 2014
 *      Author: vivek
 */

#ifndef UTILITYFNS_H_
#define UTILITYFNS_H_

#ifndef ERROR_STR
#define ERROR_STR "Error in function:" __FUNCTION__
#endif

#include <math.h>
#include <map>
#include <vector>
#include <algorithm>
#include <iterator>
#include <stdio.h>
#include<string>
#include<sstream>
#include <vector>

#define POWER 0
#define REALPART  1
#define IMAGPART  2
#define PHASE 3
#define PHASEDEG 4
#define PHASERAD 5
#define PASS 0
#define FAIL -1
typedef float _Complex cmplx;





void log_message(std::string message, int level);
static void chk(int value){
	if(value==EXIT_FAILURE) exit(-1);
}

inline int pow2_round (int x)
{
    if (x < 0)
        return 0;
    --x;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return x+1;
}

class PulseCompare
{
public:
	PulseCompare(int minwidth_):minwidth(minwidth_){};
	bool operator()(const int x, const int y) const {
		return minwidth < (y - x);
	}
private:
	int minwidth;
};

template<typename T> inline T sum_data(T* data,int  n){
	T sum = 0.0;
	for(int i=0;i<n;i++)sum+=data[i];
	return sum;
}

template<typename T> inline T* malloc_and_check(unsigned long N, std::string error_position)
{
	T* data = (T*)malloc(N*sizeof(T));
	if(data==NULL)
	{
		fprintf(stderr,"Error mallocing %d*%ld bytes at: %s ",N,sizeof(T),error_position.c_str());
	}
	return data;
}

template<typename T> inline T* new_and_check(unsigned long N, std::string error_position)
{
	try {
	T* data = new T[N];
	return data;
	}
	catch (std::bad_alloc& ba) {
			fprintf(stderr,"bad_alloc exception on allocating %d*%ld memory at this place: %s  ",N,sizeof(T),error_position.c_str());
			exit(EXIT_FAILURE);
		}
}


template<typename T> inline T* new_and_check_2D(int dim1,int dim2, std::string error_position)
{
	try {
	T** data = new T*[dim1];
	for(int x=0;x<dim1;x++)
		data[x] = new T[dim2];
	return data;
	}
	catch (std::bad_alloc& ba) {
			fprintf(stderr,"bad_alloc exception at allocating memory at this place: %s ",error_position.c_str());
			exit(EXIT_FAILURE);
		}
}

template<typename T> inline T** new_and_check_1Dof2D(int dim1,std::string errorposition)
{
	try {
	T** data = new T*[dim1];
	return data;
	}
	catch (std::bad_alloc& ba) {
			fprintf(stderr, "bad_alloc exception at allocating memory at this place: %s ",errorposition.c_str());
			exit(EXIT_FAILURE);
		}
}
template<typename K,typename V> const K& key(const std::pair<K, V>& keyValue)
{
    return keyValue.first;
}
template<typename K,typename V> const V& value(const std::pair<K, V>& keyValue)
{
    return keyValue.second;
}

template<typename K, typename V,typename C> inline int split_map_into_vectors(std::map<K,V,C>* map,std::vector<K>* keys,std::vector<V>* values)
{
std::transform(map->begin(), map->end(), keys->begin(), key<K,V>);
std::transform(map->begin(), map->end(), values->begin(), value<K,V>);
return EXIT_SUCCESS;
}
template<typename K, typename V> inline int split_map_into_vectors(std::map<K,V>* map,std::vector<K>* keys,std::vector<V>* values)
{
std::transform(map->begin(), map->end(), keys->begin(), key<K,V>);
std::transform(map->begin(), map->end(), values->begin(), value<K,V>);
return EXIT_SUCCESS;
}

template<typename K, typename V> inline int get_key_vector_from_map( std::map<K,V>* map,std::vector<K>* keys ) {

	std::transform(map->begin(), map->end(), keys->begin(), key<K,V>);
	return EXIT_SUCCESS;
}

template<typename K, typename V> inline void clone_and_cast(K* src, V* dest, int n){
	for(int i=0;i<n;i++) dest[i] =(V)src[i];
}


template<typename T> inline T get_scrunched_value(T* data,int bscrunch) {
	float out=0.0;
	for(int bs=0;bs<bscrunch;bs++)
		out+= data[bs];
	return out/bscrunch;
}

template<typename T> inline void scrunch(T* data,T* out, int n, int b){
	int j=0;
	for(int i=0;i<n;i+=b) {
		out[j++]= get_scrunched_value<T>(&data[i],b);
	}
}

template <typename K, typename V> V get_or_default (const  std::map <K,V> & m, const K & key, const V & defval ) {
   typename std::map<K,V>::const_iterator it = m.find( key );
   if ( it == m.end() ) {
      return defval;
   }
   else {
      return it->second;
   }
}

int  file_open(FILE** file, const char* absolutename, const char* mode);
bool file_exists (const std::string& name);
int check_size(unsigned long req, unsigned long got);

void split(const std::string &s, char delim, std::vector<std::string> &elems);
std::vector<std::string> split(const std::string &s, char delim);

void sigproc_to_hhmmss(double sigproc,char* hhmmss);
void sigproc_to_ddmmss(double sigproc,char* ddmmss);
int ddmmss_to_sigproc (const char * ddmmss, double * sigproc);
int hhmmss_to_sigproc (const char * hhmmss, double * sigproc);
int hhmmss_to_double (const char * hhmmss, double * degree_value);
int ddmmss_to_double (const char * ddmmss, double * degree_value);

int get_number_from_server_name(std::string server_name, int* server);

#endif /* UTILITYFNS_H_ */
