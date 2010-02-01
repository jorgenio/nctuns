/*
 * Copyright (c) from 2000 to 2009
 * 
 * Network and System Laboratory 
 * Department of Computer Science 
 * College of Computer Science
 * National Chiao Tung University, Taiwan
 * All Rights Reserved.
 * 
 * This source code file is part of the NCTUns 6.0 network simulator.
 *
 * Permission to use, copy, modify, and distribute this software and
 * its documentation is hereby granted (excluding for commercial or
 * for-profit use), provided that both the copyright notice and this
 * permission notice appear in all copies of the software, derivative
 * works, or modified versions, and any portions thereof, and that
 * both notices appear in supporting documentation, and that credit
 * is given to National Chiao Tung University, Taiwan in all publications 
 * reporting on direct or indirect use of this code or its derivatives.
 *
 * National Chiao Tung University, Taiwan makes no representations 
 * about the suitability of this software for any purpose. It is provided 
 * "AS IS" without express or implied warranty.
 *
 * A Web site containing the latest NCTUns 6.0 network simulator software 
 * and its documentations is set up at http://NSL.csie.nctu.edu.tw/nctuns.html.
 *
 * Project Chief-Technology-Officer
 * 
 * Prof. Shie-Yuan Wang <shieyuan@csie.nctu.edu.tw>
 * National Chiao Tung University, Taiwan
 *
 * 09/01/2009
 */

#ifndef __NCTUNS_OFDMA_CHANNEL_MODEL_MR_H__
#define __NCTUNS_OFDMA_CHANNEL_MODEL_MR_H__

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <nctuns_api.h>
#include "ofdma_80216j.h"
#include "ofdma_channel_model_MR.h"

class OFDMA_ChannelModel_MR {
	private:
		bool isBypassing;    // Only makeBitError() is bypassed when set.
		double _bandwidth;   // in MHz
		double _dataRate[7]; // in Mbit/s

		double pathLoss(double);
		double BPSK(double);
		double QPSK(double);
		double QAM(int, double);
		double SNRToENR(double, double);

	public:
		OFDMA_ChannelModel_MR(double, double, bool);
		~OFDMA_ChannelModel_MR();

		double powerToSNR(double);
		double computeSNR(double, double);
		double computeBER(int, double);
		double computeBERbyENR(int, double);
		void makeBitError(char *, int, double);
};

#endif                /* __NCTUNS_OFDMA_CHANNEL_MODEL_MR_H__ */
