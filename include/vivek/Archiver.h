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

#define SHARED_BUFFER_SIZE 8 * 1024 * 1024

const char* const CANDIDATE_FILENAME_KEY = "CAND_FILEs";
const char* const OUT_DIR_KEY = "FOLD_OUT";

namespace vivek {
class Archiver;
}


class vivek::Archiver {
public:
    dada_hdu_t* out_hdu;
    key_t out_key;
    multilog_t* log;

	Archiver();
	Archiver( key_t out_key): out_key(out_key){ out_hdu = 0; log = 0;}

	int transfer_fil_to_DADA_buffer(vivek::Filterbank* f);
	virtual ~Archiver();
};

#endif /* ARCHIVER_H_ */
