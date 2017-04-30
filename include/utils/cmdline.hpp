#pragma once

#ifndef CMDLINE_H_
#define CMDLINE_H_

#include <tclap/CmdLine.h>
#include <string>
#include <sstream>
#include <iostream>
#include "SMIRFdef.hpp"

struct CmdLineOptions {

  std::string killfilename;
  std::string zapfilename;

  int max_num_threads;
  unsigned int size;

  float dm_start;
  float dm_end;
  float dm_tol;
  float dm_pulse_width;
  float acc_start;
  float acc_end;
  float acc_tol;
  float acc_pulse_width;
  float boundary_5_freq;
  float boundary_25_freq;

  int nharmonics;
  int npdmp;
  int limit;

  float min_snr;
  float min_freq;
  float max_freq;
  int max_harm;
  float freq_tol;

  bool verbose;
  bool progress_bar;


  /* Vivek's options */

  bool dump_mode;
  bool transfer_mode;

  std::string smirf_base;
  std::string smirf_utc_dir;

  std::string archives_base;

  std::string candidates_dir;
  std::string candidates_file;

  std::string uniq_points_dir;
  std::string uniq_points_file;

  std::string out_dir;
  std::string out_suffix;

  int point_num;

  key_t  out_key;

  std::string utc;

  std::string host;

  int gpu_device;

  bool dynamic_birdies;


};

std::string get_utc_str();
int read_cmdline_options(CmdLineOptions& args, int argc, char **argv);





#endif /* CMDLINE_H_ */
