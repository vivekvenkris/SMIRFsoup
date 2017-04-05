//#include <data_types/timeseries.hpp>
//#include <data_types/fourierseries.hpp>
//#include <data_types/candidates.hpp>
//#include <data_types/filterbank.hpp>
//#include <transforms/dedisperser.hpp>
//#include <transforms/resampler.hpp>
//#include <transforms/folder.hpp>
//#include <transforms/ffter.hpp>
//#include <transforms/dereddener.hpp>
//#include <transforms/spectrumformer.hpp>
//#include <transforms/birdiezapper.hpp>
//#include <transforms/peakfinder.hpp>
//#include <transforms/distiller.hpp>
//#include <transforms/harmonicfolder.hpp>
//#include <transforms/scorer.hpp>
//#include <utils/exceptions.hpp>
//#include <utils/utils.hpp>
//#include <utils/stats.hpp>
//#include <utils/stopwatch.hpp>
//#include <utils/progress_bar.hpp>
//#include <utils/cmdline.hpp>
//#include <utils/output_stats.hpp>
//#include <string>
//#include <iostream>
//#include <stdio.h>
//#include <unistd.h>
//#include "cuda.h"
//#include "cufft.h"
//#include "pthread.h"
//#include <cmath>
//#include <map>
//
//
//int main4(int argc, char **argv)
//{
//  std::map<std::string,Stopwatch> timers;
//  timers["reading"]      = Stopwatch();
//  timers["dedispersion"] = Stopwatch();
//  timers["searching"]    = Stopwatch();
//  timers["folding"]      = Stopwatch();
//  timers["total"]        = Stopwatch();
//  timers["total"].start();
//
//  CmdLineOptions args;
//  if (!read_cmdline_options(args,argc,argv))
//    ErrorChecker::throw_error("Failed to parse command line arguments.");
//
//  int nthreads = std::min(Utils::gpu_count(),args.max_num_threads);
//  nthreads = std::max(1,nthreads);
//
//  if (args.verbose)
//    std::cout << "Using file: " << args.infilename << std::endl;
//  std::string filename(args.infilename);
//
//  //Stopwatch timer;
//  if (args.progress_bar)
//    printf("Reading data from %s\n",args.infilename.c_str());
//
//  timers["reading"].start();
//  SigprocFilterbank filobj(filename);
//  timers["reading"].stop();
//
//  if (args.progress_bar){
//    printf("Complete (execution time %.2f s)\n",timers["reading"].getTime());
//  }
//
//  Dedisperser dedisperser(filobj,nthreads);
//  if (args.killfilename!=""){
//    if (args.verbose)
//      std::cout << "Using killfile: " << args.killfilename << std::endl;
//    dedisperser.set_killmask(args.killfilename);
//  }
//
//  if (args.verbose)
//    std::cout << "Generating DM list" << std::endl;
//  dedisperser.generate_dm_list(args.dm_start,args.dm_end,args.dm_pulse_width,args.dm_tol);
//  std::vector<float> dm_list = dedisperser.get_dm_list();
//
//  if (args.verbose){
//    std::cout << dm_list.size() << " DM trials" << std::endl;
//    for (int ii=0;ii<dm_list.size();ii++)
//      std::cout << dm_list[ii] << std::endl;
//    std::cout << "Executing dedispersion" << std::endl;
//  }
//
//  if (args.progress_bar)
//    printf("Starting dedispersion...\n");
//
//  timers["dedispersion"].start();
//  PUSH_NVTX_RANGE("Dedisperse",3)
//  DispersionTrials<unsigned char> trials = dedisperser.dedisperse();
//  POP_NVTX_RANGE
//  timers["dedispersion"].stop();
//
//  if (args.progress_bar)
//    printf("Complete (execution time %.2f s)\n",timers["dedispersion"].getTime());
//
//  unsigned int size;
//  if (args.size==0)
//    size = Utils::prev_power_of_two(filobj.get_nsamps());
//  else
//    //size = std::min(args.size,filobj.get_nsamps());
//    size = args.size;
//  if (args.verbose)
//    std::cout << "Setting transform length to " << size << " points" << std::endl;
//
//  AccelerationPlan acc_plan(args.acc_start, args.acc_end, args.acc_tol,
//			    args.acc_pulse_width, size, filobj.get_tsamp(),
//			    filobj.get_cfreq(), filobj.get_foff());
//
//
//  //Multithreading commands
//  timers["searching"].start();
//  std::vector<Worker*> workers(nthreads);
//  std::vector<pthread_t> threads(nthreads);
//  DMDispenser dispenser(trials);
//  if (args.progress_bar)
//    dispenser.enable_progress_bar();
//
//  for (int ii=0;ii<nthreads;ii++){
//    workers[ii] = (new Worker(trials,dispenser,acc_plan,args,size,ii));
//    pthread_create(&threads[ii], NULL, launch_worker_thread, (void*) workers[ii]);
//  }
//
//  DMDistiller dm_still(args.freq_tol,true);
//  HarmonicDistiller harm_still(args.freq_tol,args.max_harm,true,false);
//  CandidateCollection dm_cands;
//  for (int ii=0; ii<nthreads; ii++){
//    pthread_join(threads[ii],NULL);
//    dm_cands.append(workers[ii]->dm_trial_cands.cands);
//  }
//  timers["searching"].stop();
//
//  if (args.verbose)
//    std::cout << "Distilling DMs" << std::endl;
//  dm_cands.cands = dm_still.distill(dm_cands.cands);
//  dm_cands.cands = harm_still.distill(dm_cands.cands);
//
//  CandidateScorer cand_scorer(filobj.get_tsamp(),filobj.get_cfreq(), filobj.get_foff(),
//			      fabs(filobj.get_foff())*filobj.get_nchans());
//  cand_scorer.score_all(dm_cands.cands);
//
//  if (args.verbose)
//    std::cout << "Setting up time series folder" << std::endl;
//
//  MultiFolder folder(dm_cands.cands,trials);
//  timers["folding"].start();
//  if (args.progress_bar)
//    folder.enable_progress_bar();
//
//  if (args.npdmp > 0){
//    if (args.verbose)
//      std::cout << "Folding top "<< args.npdmp <<" cands" << std::endl;
//    folder.fold_n(args.npdmp);
//  }
//  timers["folding"].stop();
//
//  if (args.verbose)
//    std::cout << "Writing output files" << std::endl;
//  //dm_cands.write_candidate_file("./old_cands.txt");
//
//  int new_size = std::min(args.limit,(int) dm_cands.cands.size());
//  dm_cands.cands.resize(new_size);
//
//  CandidateFileWriter cand_files(args.outdir);
//  cand_files.write_binary(dm_cands.cands,"candidates.peasoup");
//
//  OutputFileWriter stats;
//  stats.add_misc_info();
//  stats.add_header(filename);
//  stats.add_search_parameters(args);
//  stats.add_dm_list(dm_list);
//
//  std::vector<float> acc_list;
//  acc_plan.generate_accel_list(0.0,acc_list);
//  stats.add_acc_list(acc_list);
//
//  std::vector<int> device_idxs;
//  for (int device_idx=0;device_idx<nthreads;device_idx++)
//    device_idxs.push_back(device_idx);
//  stats.add_gpu_info(device_idxs);
//  stats.add_candidates(dm_cands.cands,cand_files.byte_mapping);
//  timers["total"].stop();
//  stats.add_timing_info(timers);
//
//  std::stringstream xml_filepath;
//  xml_filepath << args.outdir << "/" << "overview.xml";
//  stats.to_file(xml_filepath.str());
//
//  return 0;
//}
