#ifndef SMIRFSOUP_H
#define SMIRFSOUP_H

#define CENTRAL_BEAM_NUM 177
#define BEAM_DIR_PREFIX "BEAM_"
#define ARCHIVES_DIR "/data/mopsr/survey/archives"
#define FB_DIR "FB"
#define PATH_SEPERATOR "/"


#define POINT_RA 0
#define POINT_DEC 1
#define POINT_START_FANBEAM 2
#define POINT_END_FANBEAM 3
#define POINT_START_NS 4
#define POINT_END_NS 5



#define TRAVERSAL_START_INDEX 6
#define TRAVERSAL_SIZE 5
#define TRAVERSAL_FANBEAM 0
#define TRAVERSAL_NS 1
#define TRAVERSAL_START_SAMPLE 2
#define TRAVERSAL_NUM_SAMPLES 3
#define TRAVERSAL_PERCENT 4

class Traversal{
public:
	double fanbeam;
	double ns;
	long startSample;
	long numSamples;
	int percent;

	Traversal(std::string* traversal){
		fanbeam = atof(traversal[TRAVERSAL_FANBEAM].c_str());
		ns = atof(traversal[TRAVERSAL_NS].c_str());
		startSample = atol(traversal[TRAVERSAL_START_SAMPLE].c_str());
		numSamples = atol(traversal[TRAVERSAL_NUM_SAMPLES].c_str());
		percent = atoi(traversal[TRAVERSAL_PERCENT].c_str());
	}


};

class UniquePoint{
public:
	std::string ra;
	std::string dec;
	double startFanbeam;
	double endFanbeam;
	double startNS;
	double endNS;
	std::vector<Traversal*>* traversals;

	UniquePoint(){
		traversals = new std::vector<Traversal*>();
	}

};


int stitch_and_dump(std::string utc_dir,  std::string fil_name, vivek::Filterbank* cfb, std::vector<UniquePoint*>* uniqPoints, std::vector<int>* uniqFBs,std::string out_dir, bool verbose);


#define numFBPerServer 44

#endif
