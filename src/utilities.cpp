/*
 * utilityfns.C
 *
 *  Created on: 8 Jul 2014
 *      Author: vivek
 */
#include <stdio.h>
#include <string.h>
#include <string>
#include <stdlib.h>
#include <errno.h>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>
#include <regex>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <unistd.h>
#include <utime.h>

#include <iostream>
#include <string>

#include <cstdlib>
#include "../include/vivek/utilities.hpp"

using namespace std;

void log_message(string message, int level){
	fprintf(stderr,message.c_str());
}


/**
 * Get server number from server name
 * assumes server of the form <string><int> <string> eg: mopsr-bf00 or mopsr-bf01.obs.molonglo.local
 */
int get_number_from_server_name(string server_name, int* server){

	regex pattern("\\d+");
	cmatch match;

	regex_search(server_name.c_str(), match, pattern);

	if(match.size() !=1){
		cerr << "Cannot extract server number from server name: " << server_name << endl;
		return EXIT_FAILURE;
	}

	string matched = match[0];
	*server = ::atoi(matched.c_str());

	return EXIT_SUCCESS;
}

int  file_open(FILE** file, const char* absolutename, const char* mode) {

	*file = fopen(absolutename, mode);

	if (!*file) {

		cerr<< "could not open '" << absolutename << "' in mode '"
				<< mode << "' Error: " << strerror(errno) << endl;

		return EXIT_FAILURE;
	}
	return EXIT_SUCCESS;
}



bool file_exists (const string& name) {

	struct stat buffer;
	return (stat (name.c_str(), &buffer) == 0);
}


int check_size(unsigned long req, unsigned long got){
	if(req==got) return EXIT_SUCCESS;
	else {
		cerr << " Could not read " << req << " number of bytes. got only " << got << "bytes." << endl;
		return EXIT_FAILURE;
	}
}


void split(const string &s, char delim, vector<string> &elems) {
	stringstream ss;
	ss.str(s);
	string item;
	while (getline(ss, item, delim)) {
		elems.push_back(item);
	}
}


vector<string> split(const string &s, char delim) {
	vector<string> elems;
	split(s, delim, elems);
	return elems;
}

void sigproc_to_hhmmss(double sigproc,char* hhmmss){

	char sigproc_str[15];
	sprintf(sigproc_str,"%09.2lf",sigproc);
	char hh[3],mm[3],ss[3],ff[strlen(sigproc_str)-6];
	memcpy(hh,&sigproc_str[0],2);
	memcpy(mm,&sigproc_str[2],2);
	memcpy(ss,&sigproc_str[4],2);
	memcpy(ff,&sigproc_str[6],strlen(sigproc_str)-6);
	hh[2] = mm[2] = ss[2] = ff[strlen(sigproc_str)-6-1] = '\0';


	sprintf(hhmmss,"%s:%s:%s%s",hh,mm,ss,ff);

	cerr << "sigproc: "<< sigproc << "hhmmss:" << hhmmss <<endl;


}

void sigproc_to_ddmmss(double sigproc,char* ddmmss){

	char sigproc_str[13];

	if(sigproc < 0 ) sprintf(sigproc_str,"%10.2lf",sigproc);
	else sprintf(sigproc_str,"%09.2lf",sigproc);

	char dd[4],mm[3],ss[3],ff[strlen(sigproc_str)-7];
	memcpy(dd,&sigproc_str[0],3);
	memcpy(mm,&sigproc_str[3],2);
	memcpy(ss,&sigproc_str[5],2);
	memcpy(ff,&sigproc_str[7],strlen(sigproc_str)-7);
	dd[3] = mm[2] = ss[2] = ff[strlen(sigproc_str)-7-1] = '\0';

	sprintf(ddmmss,"%s:%s:%s%s",dd,mm,ss,ff);

	cerr << "sigproc: "<< sigproc << "hhmmss:" << ddmmss <<endl;



}

//! credit AJ
int ddmmss_to_sigproc (const char * ddmmss, double * sigproc)
{
	int ideg = 0;
	int iamin = 0;
	double asec = 0;
	const char *sep = ":";
	char * saveptr;

	char * copy =  new_and_check<char> (strlen(ddmmss) + 1,"ddmmss_to_sigproc");
	strcpy (copy, ddmmss);

	char * str = strtok_r(copy, sep, &saveptr);
	if (str != NULL)
	{
		if (sscanf(str, "%d", &ideg) != 1)
			return -1;

		str = strtok_r(NULL, sep, &saveptr);
		if (str != NULL)
		{
			if (sscanf(str, "%d", &iamin) != 1)
				return -1;

			str = strtok_r(NULL, sep, &saveptr);
			if (str != NULL)
			{
				if (sscanf(str, "%lf", &asec) != 1)
					return -1;
			}
		}
	}

	free (copy);

	if (ideg < 0)
		*sigproc = ((double) ideg*1e4 - (double) iamin*1e2) - asec;
	else
		*sigproc = ((double) ideg*1e4  + (double) iamin*1e2) + asec;

	return 0;
}

//! credit AJ
int hhmmss_to_sigproc (const char * hhmmss, double * sigproc)
{
	int ihour = 0;
	int imin = 0;
	double sec = 0;
	const char *sep = ":";
	char * saveptr;

	char * copy = (char *) new_and_check<char> (strlen(hhmmss) + 1,"hhmmss_to_sigproc");
	strcpy (copy, hhmmss);

	char * str = strtok_r(copy, sep, &saveptr);
	if (str != NULL)
	{
		if (sscanf(str, "%d", &ihour) != 1)
			return -1;

		str = strtok_r(NULL, sep, &saveptr);
		if (str != NULL)
		{
			if (sscanf(str, "%d", &imin) != 1)
				return -1;

			str = strtok_r(NULL, sep, &saveptr);
			if (str != NULL)
			{
				if (sscanf(str, "%lf", &sec) != 1)
					return -1;
			}
		}
	}
	free (copy);

	char s = '\0';
	if (ihour < 0)
	{
		ihour *= -1;
		s = '-';
	}

	*sigproc = ((double)ihour*1e4  + (double)imin*1e2 + sec);
	return 0;
}



int hhmmss_to_double (const char * hhmmss, double * degree_value)
{
	int ihour = 0;
	int imin = 0;
	double sec = 0;
	const char *sep = ":";
	char * saveptr;

	char * copy = (char *) new_and_check<char> (strlen(hhmmss) + 1,"hhmmss_to_sigproc");
	strcpy (copy, hhmmss);

	char * str = strtok_r(copy, sep, &saveptr);
	if (str != NULL)
	{
		if (sscanf(str, "%d", &ihour) != 1)
			return -1;

		str = strtok_r(NULL, sep, &saveptr);
		if (str != NULL)
		{
			if (sscanf(str, "%d", &imin) != 1)
				return -1;

			str = strtok_r(NULL, sep, &saveptr);
			if (str != NULL)
			{
				if (sscanf(str, "%lf", &sec) != 1)
					return -1;
			}
		}
	}
	free (copy);

	char s = '\0';
	if (ihour < 0)
	{
		ihour *= -1;
		s = '-';
	}

	*degree_value = ((double)ihour*15.0  + (double)imin*15.0/60.0 + sec * 15.0/3600.0);
	return 0;
}



int ddmmss_to_double (const char * ddmmss, double * degree_value)
{
	int ideg = 0;
	int iamin = 0;
	double asec = 0;
	const char *sep = ":";
	char * saveptr;

	char * copy =  new_and_check<char> (strlen(ddmmss) + 1,"ddmmss_to_sigproc");
	strcpy (copy, ddmmss);

	char * str = strtok_r(copy, sep, &saveptr);
	if (str != NULL)
	{
		if (sscanf(str, "%d", &ideg) != 1)
			return -1;

		str = strtok_r(NULL, sep, &saveptr);
		if (str != NULL)
		{
			if (sscanf(str, "%d", &iamin) != 1)
				return -1;

			str = strtok_r(NULL, sep, &saveptr);
			if (str != NULL)
			{
				if (sscanf(str, "%lf", &asec) != 1)
					return -1;
			}
		}
	}

	free (copy);

	if (ideg < 0)
		*degree_value = ((double) ideg - (double) iamin/60.0) - asec/3600.0;
	else
		*degree_value = ((double) ideg  + (double) iamin/60.0) + asec/3600.0;

	return 0;
}

string get_jname_from_pos(string ra, string dec){

	ostringstream result;
	if(ra.find(":") != ra.npos){
		ra.erase( ra.begin() +  ra.find(":")); // replace all 'x' to 'y'
	}
	ra = ra.substr(0, ra.find(":"));

	if(dec.find(":") != dec.npos){
		dec.erase( dec.begin() +  dec.find(":")); // replace all 'x' to 'y'
	}
	dec = dec.substr(0, dec.find(":"));

	result << ra << dec;

	return result.str();
}



string get_long_jname_from_pos(string ra, string dec){

	ostringstream result;

	ra.erase(std::remove(ra.begin(), ra.end(), ':'), ra.end());
	ra = ra.substr(0, ra.find("."));

	dec.erase(std::remove(dec.begin(), dec.end(), ':'), dec.end());
	dec = dec.substr(0, dec.find("."));

	result << ra << dec;

	return result.str();
}


int touch(const std::string& pathname)

{
    int fd = open(pathname.c_str(),
                  O_WRONLY|O_CREAT|O_NOCTTY|O_NONBLOCK,
                  0666);
    if (fd<0) // Couldn't open that path.
    {
        std::cerr << ": Couldn't open() path \""
            << pathname
            << "\"\n";
        return EXIT_FAILURE;
    }
    int rc = utimensat(AT_FDCWD,
                       pathname.c_str(),
                       nullptr,
                       0);
    if (rc)
    {
        std::cerr
            << __PRETTY_FUNCTION__
            << ": Couldn't utimensat() path \""
            << pathname
            << "\"\n";
        return EXIT_FAILURE;
    }
    std::clog
        << __PRETTY_FUNCTION__
        << ": Completed touch() on path \""
        << pathname
        << "\"\n";
    return EXIT_SUCCESS;
}



