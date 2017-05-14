/*
 * Archiver.h
 *
 *  Created on: Mar 15, 2017
 *      Author: vivek
 */

#ifndef ARCHIVER_H_
#define ARCHIVER_H_

#include "dada_client.h"
#include "dada_hdu.h"
#include "dada_def.h"
#include "ascii_header.h"
#include "filutil.hpp"
#include "filterbank_def.hpp"
#include "HeaderParam.hpp"

#include <vector>
#include <string.h>
#include <sstream>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <assert.h>
#include <math.h>
#include <complex.h>
#include <float.h>
#include <iostream>
#include <cstring>

const char* const CANDIDATE_FILENAME_KEY = "CAND_FILE";
const char* const OUT_DIR_KEY = "FOLD_OUT";
const char* const FINAL_STITCH = "FINAL_STITCH";

namespace vivek {
class Archiver;
}


class vivek::Archiver {
public:
    static dada_hdu_t* out_hdu;
    static key_t out_key;
    static unsigned long  buffer_size;
    static unsigned int  nbuffers;

	Archiver();

	static int transfer_fil_to_DADA_buffer(std::string utc, vivek::Filterbank* f, bool final);
	static void handle_archiver_segfault(int signal);

	virtual ~Archiver();
};

#endif /* ARCHIVER_H_ */
