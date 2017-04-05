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
#include "../include/vivek/utilities.hpp"


void log_message(std::string message, int level){
	fprintf(stderr,message.c_str());
}


FILE* file_open(const char* absolutename, const char* mode) {
	FILE* file = fopen(absolutename, mode);
	if (!file) {
		fprintf(stderr, "could not open '%s': %s\n", absolutename,
				strerror(errno));
		exit(1);
	}
	return file;
}



int check_size(unsigned long req, unsigned long got){
	if(req==got) return EXIT_SUCCESS;
	else {
		std::cerr << " Could not read " << req << " number of bytes. got only " << got << "bytes." << std::endl;
		return EXIT_FAILURE;
	}
}


void split(const std::string &s, char delim, std::vector<std::string> &elems) {
    std::stringstream ss;
    ss.str(s);
    std::string item;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
}


std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> elems;
    split(s, delim, elems);
    return elems;
}

void sigproc_to_hhmmss(double sigproc,char* hhmmss){

  char sigproc_str[15];
  sprintf(sigproc_str,"%08.2lf",sigproc);
  char hh[3],mm[3],ss[3],ff[strlen(sigproc_str)-6];
  memcpy(hh,&sigproc_str[0],2);
  memcpy(mm,&sigproc_str[2],2);
  memcpy(ss,&sigproc_str[4],2);
  memcpy(ff,&sigproc_str[6],strlen(sigproc_str)-6);
  hh[2] = mm[2] = ss[2] = ff[strlen(sigproc_str)-6-1] = '\0';


  sprintf(hhmmss,"%s:%s:%s%s",hh,mm,ss,ff);


}

void sigproc_to_ddmmss(double sigproc,char* ddmmss){

  char sigproc_str[13];
  sprintf(sigproc_str,"%7.6lf",sigproc);

  char dd[4],mm[3],ss[3],ff[strlen(sigproc_str)-7];
  memcpy(dd,&sigproc_str[0],3);
  memcpy(mm,&sigproc_str[3],2);
  memcpy(ss,&sigproc_str[5],2);
  memcpy(ff,&sigproc_str[7],strlen(sigproc_str)-7);
  dd[3] = mm[2] = ss[2] = ff[strlen(sigproc_str)-7-1] = '\0';

  sprintf(ddmmss,"%s:%s:%s%s",dd,mm,ss,ff);


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





