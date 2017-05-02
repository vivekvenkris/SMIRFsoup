#include <utils/cmdline.hpp>
#include <string>
#include <sstream>
#include <iostream>
#include <unistd.h>
#include <cstdlib>
std::string get_utc_str()
{
  char buf[128];
  std::time_t t = std::time(NULL);
  std::strftime(buf, 128, "./%Y-%m-%d-%H:%M_peasoup/", std::gmtime(&t));
  return std::string(buf);
}



int read_cmdline_options(CmdLineOptions& args, int argc, char **argv)
{
  try
    {
      TCLAP::CmdLine cmd("SMIRFSoup - The Mother of all pulsar searches", ' ', "1.0");

      TCLAP::ValueArg<std::string> arg_killfilename("K", "killfile",
						    "Channel mask file",
						    false, "", "string",cmd);

      TCLAP::ValueArg<std::string> arg_zapfilename("z", "zapfile",
                                                   "Birdie list file",
                                                   false, "", "string",cmd);

      TCLAP::ValueArg<int> arg_max_num_threads("t", "num_threads",
                                               "The number of GPUs to use (default = 1)",
                                               false, 1, "int", cmd);

      TCLAP::ValueArg<int> arg_limit("", "limit",
				     "upper limit on number of candidates to write out",
				     false, 1000, "int", cmd);

      TCLAP::ValueArg<size_t> arg_size("", "fft_size",
                                       "Transform size to use (defaults to lower power of two)",
                                       false, 0, "size_t", cmd);

      TCLAP::ValueArg<float> arg_dm_start("", "dm_start",
                                          "First DM to dedisperse to. (default =0)",
                                          false, 0.0, "float", cmd);

      TCLAP::ValueArg<float> arg_dm_end("", "dm_end",
                                        "Last DM to dedisperse to. (default = 2000)",
                                        false, 2000.0, "float", cmd);

      TCLAP::ValueArg<float> arg_dm_tol("", "dm_tol",
                                        "DM smearing tolerance (default=1.05)",
                                        false, 1.05, "float",cmd);

      TCLAP::ValueArg<float> arg_dm_pulse_width("", "dm_pulse_width",
                                                "Minimum pulse width for which dm_tol is valid. (default= 327.68 us)",
                                                false, 327.68, "float (us)",cmd);

      TCLAP::ValueArg<float> arg_acc_start("", "acc_start",
					   "First acceleration to resample to. (default=0)",
					   false, 0.0, "float", cmd);

      TCLAP::ValueArg<float> arg_acc_end("", "acc_end",
					 "Last acceleration to resample to. (default= 0)",
					 false, 0.0, "float", cmd);

      TCLAP::ValueArg<float> arg_acc_tol("", "acc_tol",
					 "Acceleration smearing tolerance (1.11=10%)",
					 false, 1.10, "float",cmd);

      TCLAP::ValueArg<float> arg_acc_pulse_width("", "acc_pulse_width",
                                                 "Minimum pulse width for which acc_tol is valid. (default= 327.68 us) ",
						 false, 327.68, "float (us)",cmd);

      TCLAP::ValueArg<float> arg_boundary_5_freq("", "boundary_5_freq",
                                                 "Frequency at which to switch from median5 to median25. (default= 0.05)",
                                                 false, 0.05, "float", cmd);

      TCLAP::ValueArg<float> arg_boundary_25_freq("", "boundary_25_freq",
						  "Frequency at which to switch from median25 to median125. (default= 0.005)",
						  false, 0.5, "float", cmd);

      TCLAP::ValueArg<int> arg_nharmonics("n", "nharmonics",
                                          "Number of harmonic sums to perform.(default= 4)",
                                          false, 4, "int", cmd);

      TCLAP::ValueArg<int> arg_npdmp("", "npdmp",
                                     "Number of candidates to fold and pdmp. (default= 100)",
                                     false, 100, "int", cmd);

      TCLAP::ValueArg<float> arg_min_snr("m", "min_snr",
                                         "The minimum S/N for a candidate (default= 9)",
                                         false, 9.0, "float",cmd);

      TCLAP::ValueArg<float> arg_min_freq("", "min_freq",
                                          "Lowest Fourier freqency to consider. (default= 0.1)",
                                          false, 0.1, "float",cmd);

      TCLAP::ValueArg<float> arg_max_freq("", "max_freq",
                                          "Highest Fourier freqency to consider. (default= 1100)",
                                          false, 1100.0, "float",cmd);

      TCLAP::ValueArg<int> arg_max_harm("", "max_harm_match",
                                        "Maximum harmonic for related candidates. (default= 4)",
                                        false, 4, "float",cmd);

      TCLAP::ValueArg<float> arg_freq_tol("", "freq_tol",
                                          "Tolerance for distilling frequencies default is (0.0001 = 0.01%)",
                                          false, 0.0001, "float",cmd);

      TCLAP::SwitchArg arg_verbose("v", "verbose", "verbose mode", cmd);

      TCLAP::SwitchArg arg_progress_bar("p", "progress_bar", "Enable progress bar for DM search", cmd);


      /* following are added by Vivek */

      TCLAP::ValueArg<std::string> arg_out_dir("o", "out_dir",
     					      "The output directory. (default= SMIRFBASE/utc)",
     					      false, "", "string",cmd);

      TCLAP::SwitchArg arg_dump_mode("D", "dump", "dump mode", cmd);

      TCLAP::SwitchArg arg_transfer_mode("T", "transfer", "Transfer to shared memory pointed by -k ", cmd);

      TCLAP::SwitchArg arg_dynamic_birdies("Z", "dynamic_birdies", " generate dynamic birdies list with zero dm coincidencing ", cmd);


      TCLAP::ValueArg<std::string> arg_archives_base("A", "archives_base","The base directory to find archives", false, ARCHIVES_BASE, "string",cmd);

      TCLAP::ValueArg<std::string> arg_smirf_base("S", "smirf_base", "The base directory to smirf related files", false, SMIRF_BASE, "string",cmd);

      TCLAP::ValueArg<std::string> arg_utc("i", "input_utc","The input utc to process", false, "", "string",cmd);

      TCLAP::ValueArg<int> arg_point_num("l", "line_number", " Line number to process on the unique points file.", false, -1 , "float",cmd);


      TCLAP::ValueArg<std::string> arg_out_suffix("s", "out_suffix"," Add output suffix", false, "", "string",cmd);

      TCLAP::ValueArg<std::string> arg_uniq_pts_file("u", "uniq_pts_file", " Unique points file name default: <utc>.<server>.uniq ", false, "", "string",cmd);

      TCLAP::ValueArg<std::string> arg_uniq_pts_dir("U", "uniq_pts_dir"," Directory to find unique points file", false, SMIRF_BASE, "string",cmd);

      TCLAP::ValueArg<std::string> arg_candidates_file("c", "candidates_file", " Coincidenced candidate file name to use with -T option",
    		  	  	  	  	  	  	  	  	  	  	  false, "candidates.shortlisted", "string",cmd);

      TCLAP::ValueArg<std::string> arg_candidates_dir("C", "candidates_dir"," Directory to find candidates file",false, "", "string",cmd);

      TCLAP::ValueArg<std::string> arg_dada_key("k", "dada_key", " shared memory key for the -T option", false, OUT_KEY, "string",cmd);

      TCLAP::ValueArg<std::string> arg_host_name("H", "host_name", " give node name to pretend running on that machine eg: mpsr-bf00", false, "", "string",cmd);




      cmd.parse(argc, argv);

      args.killfilename      = arg_killfilename.getValue();
      args.zapfilename       = arg_zapfilename.getValue();
      args.max_num_threads   = arg_max_num_threads.getValue();
      args.limit             = arg_limit.getValue();
      args.size              = arg_size.getValue();
      args.dm_start          = arg_dm_start.getValue();
      args.dm_end            = arg_dm_end.getValue();
      args.dm_tol            = arg_dm_tol.getValue();
      args.dm_pulse_width    = arg_dm_pulse_width.getValue();
      args.acc_start         = arg_acc_start.getValue();
      args.acc_end           = arg_acc_end.getValue();
      args.acc_tol           = arg_acc_tol.getValue();
      args.acc_pulse_width   = arg_acc_pulse_width.getValue();
      args.boundary_5_freq   = arg_boundary_5_freq.getValue();
      args.boundary_25_freq  = arg_boundary_25_freq.getValue();
      args.nharmonics        = arg_nharmonics.getValue();
      args.npdmp             = arg_npdmp.getValue();
      args.min_snr           = arg_min_snr.getValue();
      args.min_freq          = arg_min_freq.getValue();
      args.max_freq          = arg_max_freq.getValue();
      args.max_harm          = arg_max_harm.getValue();
      args.freq_tol          = arg_freq_tol.getValue();
      args.verbose           = arg_verbose.getValue();
      args.progress_bar      = arg_progress_bar.getValue();

      /* Added by Vivek */
      args.utc				 = arg_utc.getValue();
      args.out_dir           = arg_out_dir.getValue();

      args.dump_mode		 = arg_dump_mode.getValue();
      args.transfer_mode	 = arg_transfer_mode.getValue();

      args.dynamic_birdies 	 = arg_dynamic_birdies.getValue();

      args.smirf_base		 = arg_smirf_base.getValue();
      args.archives_base	 = arg_archives_base.getValue();

      args.uniq_points_dir 	 = arg_uniq_pts_dir.getValue();
      args.uniq_points_file	 = arg_uniq_pts_file.getValue();

      args.out_suffix		 = arg_out_suffix.getValue();
      args.point_num		 = arg_point_num.getValue();

      args.candidates_dir 	 = arg_candidates_dir.getValue();
      args.candidates_file 	 = arg_candidates_file.getValue();

      args.host				 = arg_host_name.getValue();

      if(args.host.empty()){
    	  char host_name[1024];
    	  gethostname(&host_name[0],1024);
    	  args.host = host_name;
      } else{
    	  std::cerr<< "Pretending to be " << args.host <<std::endl;
      }
	  args.host = args.host.substr(0,args.host.find("."));

      args.gpu_device = 0;

	  if(  args.host.find("bf08") !=  args.host.npos ) args.gpu_device = 0;


      if(args.utc.empty()){
    	  std::cerr << " Please provide a UTC with the -i option. Aborting now." <<std::endl;
    	  return EXIT_FAILURE;
      }


      std::stringstream smirf_utc_dir_stream;
      smirf_utc_dir_stream << args.smirf_base << PATH_SEPERATOR << args.utc;
      smirf_utc_dir_stream >> args.smirf_utc_dir;

      if(args.candidates_dir.empty()) args.candidates_dir = args.smirf_utc_dir;

      if(args.uniq_points_dir.empty()) args.uniq_points_dir = args.smirf_utc_dir;

      if(args.out_dir.empty() ) args.out_dir = args.smirf_utc_dir;

      std::stringstream out_key_stream;
      out_key_stream << std::hex << arg_dada_key.getValue();
      out_key_stream >> args.out_key;

      if(args.verbose){
    	  std::cerr << "Arguments to be used:"
  			  	<< arg_killfilename.getName() << "\t '" << arg_killfilename.getValue() << "' \n "
			  	<< arg_zapfilename.getName() << "\t '"  << arg_zapfilename.getValue() << "' \n "
			  	<< arg_max_num_threads.getName() << "\t '" << arg_max_num_threads.getValue() << "' \n "
			  	<< arg_limit.getName() << "\t '" << arg_limit.getValue() << "' \n "
			  	<< arg_size.getName() << "\t '" << arg_size.getValue() << "' \n "
			  	<< arg_dm_start.getName() << "\t '" << arg_dm_start.getValue() << "' \n "
			  	<< arg_dm_end.getName() << "\t '" << arg_dm_end.getValue() << "' \n "
			  	<< arg_dm_tol.getName() << "\t '" << arg_dm_tol.getValue() << "' \n "
			  	<< arg_dm_pulse_width.getName() << "\t '" << arg_dm_pulse_width.getValue() << "' \n "
			  	<< arg_acc_start.getName() << "\t '" << arg_acc_start.getValue() << "' \n "
			  	<< arg_acc_end.getName() << "\t '" << arg_acc_end.getValue() << "' \n "
			  	<< arg_acc_tol.getName() << "\t '" << arg_acc_tol.getValue() << "' \n "
			  	<< arg_acc_pulse_width.getName() << "\t '" << arg_acc_pulse_width.getValue() << "' \n "
			  	<< arg_boundary_5_freq.getName() << "\t '" << arg_boundary_5_freq.getValue() << "' \n "
			  	<< arg_boundary_25_freq.getName() << "\t '" << arg_boundary_25_freq.getValue() << "' \n "
			  	<< arg_nharmonics.getName() << "\t '" << arg_nharmonics.getValue() << "' \n "
			  	<< arg_npdmp.getName() << "\t '" << arg_npdmp.getValue() << "' \n "
			  	<< arg_min_snr.getName() << "\t '" << arg_min_snr.getValue() << "' \n "
			  	<< arg_min_freq.getName() << "\t '" << arg_min_freq.getValue() << "' \n "
			  	<< arg_max_freq.getName() << "\t '" << arg_max_freq.getValue() << "' \n "
			  	<< arg_max_harm.getName() << "\t '" << arg_max_harm.getValue() << "' \n "
				<< arg_freq_tol.getName() << "\t '" << arg_freq_tol.getValue() << "' \n "
				<< arg_progress_bar.getName() << "\t '" << arg_progress_bar.getValue() << "' \n "
				<< arg_out_dir.getName() << "\t '" << arg_out_dir.getValue() << "' \n "
				<< arg_dump_mode.getName() << "\t '" << arg_dump_mode.getValue() << "' \n "
				<< arg_transfer_mode.getName() << "\t '" << arg_transfer_mode.getValue() << "' \n "
				<< arg_smirf_base.getName() << "\t '" << arg_smirf_base.getValue() << "' \n "
				<< arg_archives_base.getName() << "\t '" << arg_archives_base.getValue() << "' \n "
				<< arg_uniq_pts_dir.getName() << "\t '" << arg_uniq_pts_dir.getValue() << "' \n "
				<< arg_uniq_pts_file.getName() << "\t '" << arg_uniq_pts_file.getValue() << "' \n "
				<< arg_out_suffix.getName() << "\t '" << arg_out_suffix.getValue() << "' \n "
				<< arg_point_num.getName() << "\t '" << arg_point_num.getValue() << "' \n "
				<< arg_candidates_dir.getName() << "\t '" << arg_candidates_dir.getValue() << "' \n "
				<< arg_candidates_file.getName() << "\t '" << arg_candidates_file.getValue() << "' \n ";
      }

    }catch (TCLAP::ArgException &e) {
    std::cerr << "Error: " << e.error() << " for arg " << e.argId()
              << std::endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}
