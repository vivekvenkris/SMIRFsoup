#pragma once
#include <iostream>
#include <vector>
#include <sstream>
#include "stdio.h"
#include "ConfigManager.hpp"
#include <iomanip>

//Lightweight plain old data struct
//used to combine multiple writes into
//one big write
struct CandidatePOD {
	float dm;
	int dm_idx;
	float acc;
	int nh;
	float snr;
	float freq;
};

struct Candidate {
public:
	std::string host;
	std::string ra_str;
	std::string dec_str;
	float start_fanbeam;
	float start_ns;
	float dm;
	int dm_idx;
	float acc;
	int nh;
	float snr;
	float freq;
	float folded_snr;
	double opt_period;
	bool is_adjacent;
	bool is_physical;
	float ddm_count_ratio;
	float ddm_snr_ratio;
	std::vector<Candidate> assoc;
	std::vector<float> fold;
	int nbins;
	int nints;

	void append(Candidate& other){
		assoc.push_back(other);
	}

	int count_assoc(void){
		int count = 0;
		for (int ii=0;ii<assoc.size();ii++){
			count ++;
			count += assoc[ii].count_assoc();
		}
		return count;
	}

	Candidate(float dm, int dm_idx, float acc, int nh, float snr, float freq)
	:dm(dm),dm_idx(dm_idx),acc(acc),nh(nh),
	 snr(snr),folded_snr(0.0),freq(freq),
	 opt_period(0.0),is_adjacent(false),is_physical(false),
	 ddm_count_ratio(0.0),ddm_snr_ratio(0.0),nints(0),nbins(0),host(ConfigManager::this_host()),start_fanbeam(0), start_ns(0){}

	Candidate(float dm, int dm_idx, float acc, int nh, float snr, float folded_snr, float freq)
	:dm(dm),dm_idx(dm_idx),acc(acc),nh(nh),snr(snr),
	 folded_snr(folded_snr),freq(freq),opt_period(0.0),
	 is_adjacent(false),is_physical(false),
	 ddm_count_ratio(0.0),ddm_snr_ratio(0.0),nints(0),nbins(0),host(ConfigManager::this_host()),start_fanbeam(0), start_ns(0){}

	Candidate()
	:dm(0.0),dm_idx(0.0),acc(0.0),nh(0.0),snr(0.0),
	 folded_snr(0.0),freq(0.0),opt_period(0.0),
	 is_adjacent(false),is_physical(false),
	 ddm_count_ratio(0.0),ddm_snr_ratio(0.0),nints(0),nbins(0),host(ConfigManager::this_host()),start_fanbeam(0), start_ns(0){}

	void set_fold(float* ar, int nbins, int nints){
		int size = nbins*nints;
		this->nints = nints;
		this->nbins = nbins;
		fold.resize(size);
		for (int ii=0;ii<size;ii++)
			fold[ii] = ar[ii];
	}

	void collect_candidates(std::vector<CandidatePOD>& cands_lite){
		CandidatePOD cand_stats = {dm,dm_idx,acc,nh,snr,freq};
		cands_lite.push_back(cand_stats);
		for (int ii=0;ii<assoc.size();ii++){
			assoc[ii].collect_candidates(cands_lite);
		}
	}

	void print(FILE* fo=stdout){
		fprintf(fo,"%.15f\t%.15f\t%.15f\t%.2f\t%.2f\t%d\t%.1f\t%.1f\t%d\t%d\t%.4f\t%.4f\t%d\n",
				1.0/freq,opt_period,freq,dm,acc,
				nh,snr,folded_snr,is_adjacent,
				is_physical,ddm_count_ratio,
				ddm_snr_ratio,assoc.size());
		for (int ii=0;ii<assoc.size();ii++){
			assoc[ii].print(fo);
		}
	}



	void print_cand_file(FILE* fo, int index){
		std::stringstream source_name;
		source_name << "SMIRF" << "_" <<"BS" << std::setfill('0') << std::setw(2) << ConfigManager::this_bs()
		<< "_" << std::setfill('0') << std::setw(3) << index;
		fprintf(fo,"%s %s %s %.15f %.2f %.2f %.2f\n",
				source_name.str().c_str(),ra_str.c_str(),dec_str.c_str(),opt_period,dm,acc,snr);

	}

	void print_cand_file_more(FILE* fo, int index){
			std::stringstream source_name;
			source_name << "SMIRF" << "_" <<"BS" << std::setfill('0') << std::setw(2) << ConfigManager::this_bs()
			<< "_" << std::setfill('0') << std::setw(3) << index;
			fprintf(fo,"%s %s %s %.15f %.2f %.2f %.2f %.2f %.1f \n",
					source_name.str().c_str(),ra_str.c_str(),dec_str.c_str(),opt_period,dm,acc,snr,folded_snr,nh*1.0);

		}


	friend std::istringstream& operator>> (std::istringstream &in, Candidate& a);
	friend std::ostringstream& operator<< (std::ostringstream &out, const  Candidate& a);
	friend std::ostream& operator<< (std::ostream &out, const  Candidate &a);

};

inline std::istringstream& operator>> (std::istringstream &iss, Candidate& c){

	int assoc_size;

	iss >> c.host
		>> c.ra_str
		>> c.dec_str
		>> c.start_fanbeam
		>> c.start_ns
		>> c.dm
		>> c.dm_idx
		>> c.acc
		>> c.nh
		>> c.snr
		>> c.freq
		>> c.folded_snr
		>> c.opt_period
		>> c.is_adjacent
		>> c.is_physical
		>> c.ddm_count_ratio
		>> c.ddm_snr_ratio
		>> c.nbins
		>> c.nints
		>> assoc_size;

//	std::cerr << c << std::endl;
//	for(int i=0; i< assoc_size; i++ ){
//		Candidate cand;
//		iss >> cand;
//		c.append(cand);
//	}

	return iss;
}

inline std::ostringstream& operator<< (std::ostringstream &oss, const  Candidate& c){

		oss	<< c.host << " "
			<< c.ra_str << " "
			<< c.dec_str << " "
			<< c.start_fanbeam << " "
			<< c.start_ns << " "
			<< c.dm << " "
			<< c.dm_idx << " "
			<< c.acc << " "
			<< c.nh << " "
			<< c.snr << " "
			<< c.freq << " "
			<< c.folded_snr << " "
			<< c.opt_period << " "
			<< c.is_adjacent << " "
			<< c.is_physical << " "
			<< c.ddm_count_ratio << " "
			<< c.ddm_snr_ratio << " "
			<< c.nbins << " "
			<< c.nints << " "
			<< c.assoc.size() << " ";

		//for( Candidate assoc_cand : c.assoc ) oss <<  assoc_cand;

	return oss;
}

inline std::ostream& operator<< (std::ostream &out, const  Candidate &c){

	std::ostringstream oss;
	oss << c.host << " "
				<< c.ra_str << " "
				<< c.dec_str << " "
				<< c.start_fanbeam << " "
				<< c.start_ns << " "
				<< c.dm << " "
				<< c.dm_idx << " "
				<< c.acc << " "
				<< c.nh << " "
				<< c.snr << " "
				<< c.freq << " "
				<< c.folded_snr << " "
				<< c.opt_period << " "
				<< c.is_adjacent << " "
				<< c.is_physical << " "
				<< c.ddm_count_ratio << " "
				<< c.ddm_snr_ratio << " "
				<< c.nbins << " "
				<< c.nints << " "
				<< c.assoc.size() << " ";
	out << oss.str();

	return out;
}



class CandidateCollection {
public:
	std::vector<Candidate> cands;

	CandidateCollection(){}

	void append(CandidateCollection& other){
		cands.insert(cands.end(),other.cands.begin(),other.cands.end());
	}

	void append(std::vector<Candidate> other){
		cands.insert(cands.end(),other.begin(),other.end());
	}

	void append(Candidate c){
		cands.push_back(c);
	}


	void reset(void){
		cands.clear();
	}

	void print(FILE* fo=stdout){
		for (int ii=0;ii<cands.size();ii++)
			cands[ii].print(fo);
	}

	void print_cand_file(FILE* fo, int candidate_id){

		if(candidate_id ==1) fprintf(fo,"SOURCE RA DEC PERIOD DM ACC SNR\n");

		for (int ii=0;ii<cands.size();ii++)
			cands[ii].print_cand_file(fo,candidate_id+ii);

	}

	void print_cand_file_more(FILE* fo,int candidate_id){
		for (int ii=0;ii<cands.size();ii++)
				cands[ii].print_cand_file_more(fo,candidate_id+ii);
	}

	void generate_candidate_binaries(std::string output_directory="./") {
		char filename[80];
		std::stringstream filepath;
		for (int ii=0;ii<cands.size();ii++){
			filepath.str("");
			sprintf(filename,"cand_%04d_%.5f_%.1f_%.1f.peasoup",
					ii,1.0/cands[ii].freq,cands[ii].dm,cands[ii].acc);
			filepath << output_directory << "/" << filename;
			FILE* fo = fopen(filepath.str().c_str(),"w");




			cands[ii].print(fo);
			fclose(fo);
		}
	}

	void write_candidate_file(std::string filepath="./candidates.txt") {
		FILE* fo = fopen(filepath.c_str(),"w");
		fprintf(fo,"#Period...Optimal period...Frequency...DM...Acceleration...Harmonic number...S/N...Folded S/N\n");
		for (int ii=0;ii<cands.size();ii++){
			fprintf(fo,"#Candidate %d\n",ii);
			cands[ii].print(fo);
		}
		fclose(fo);
	}

	friend std::istringstream& operator>> (std::istringstream &in, CandidateCollection& a);
	friend std::ostringstream& operator<< (std::ostringstream &out, const  CandidateCollection& a);
	friend std::ostream& operator<< (std::ostream &out, const  CandidateCollection &a);
};

inline std::istringstream& operator>> (std::istringstream &in, CandidateCollection& cc){

	std::string num_cands_str;
	in >> num_cands_str;
	int num_cands = std::atoi(num_cands_str.c_str());

	std::cerr << "Receiving " <<  num_cands  << "Candidates" << std::endl;

	for( int i=0;i < num_cands; i++) {

		Candidate c;
		in >> c;
		cc.append(c);

	}

	return in;
}

inline  std::ostringstream& operator<< (std::ostringstream &out, const  CandidateCollection& cc){

	std::cerr << "Sending " <<  cc.cands.size()  << "Candidates" << std::endl;

	out << cc.cands.size() << " ";

	for(Candidate c: cc.cands) out << c;

	return out;
}

inline std::ostream& operator<< (std::ostream &out, const  CandidateCollection &a){
	std::ostringstream oss;
	oss <<  a;
	out << oss.str();
	return out;
}

class SpectrumCandidates: public CandidateCollection {
public:
	float dm;
	int dm_idx;
	float acc;

	SpectrumCandidates(float dm, int dm_idx, float acc)
	:dm(dm),dm_idx(dm_idx),acc(acc){}

	void append(float* snrs, float* freqs, int nh, int size){
		cands.reserve(size+cands.size());
		for (int ii=0;ii<size;ii++)
			cands.push_back(Candidate(dm,dm_idx,acc,nh,snrs[ii],freqs[ii]));
	}
};

