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

#ifndef __NCTUNS_80216E_OFDMA_80216E_H__
#define __NCTUNS_80216E_OFDMA_80216E_H__

#include <stdio.h>
#include <math.h>
#include <object.h>
#include <nctuns_api.h>
#include <gbind.h>
#include <event.h>
#include <timer.h>
#include <assert.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>
#include <module/phy/cm.h>
#include <wphyInfo.h>
#include <regcom.h>
#include "ofdma_channel_coding.h"
#include "ofdma_channel_model.h"
#include "../mac_16e/management_message.h"

#define LOG_DEBUG 0

class OFDMA_ChannelCoding;
class OFDMA_ChannelModel;

extern RegTable         RegTable_;
extern const double     frameDuration[];
extern const int        repeat_times[];

using namespace std;
using namespace mobileManageMsg;

/* Number of symbols per slot at different permutation zone */
enum {
	PREAMBLE    = 1, // unit in symbol
	DL_FUSC     = 1, // unit in symbol
	DL_OFUSC    = 1, // unit in symbol
	DL_PUSC     = 2, // unit in symbol
	UL_PUSC     = 3, // unit in symbol
	DL_TUSC1    = 3, // unit in symbol
	DL_TUSC2    = 3  // unit in symbol
};

/* Coding Indication */
enum coding_indication {
	CC,
	BTC,
	CTC,
	ZT_CC,
	CC_OPT,
	LDPC
};

/* "FEC Code type" in our implementation, see Table 363, 357 (16e) */
enum fec_code_type {
	QPSK_1_2,
	QPSK_3_4,
	QAM16_1_2,
	QAM16_3_4,
	QAM64_1_2,
	QAM64_2_3,
	QAM64_3_4,
	BPSK_
};

/* Repetition Coding */
enum repe_code_type {
	REPETATION_CODEWORD_1 = 0,
	REPETATION_CODEWORD_2 = 1,
	REPETATION_CODEWORD_4 = 2,
	REPETATION_CODEWORD_6 = 3
};

/* Spec 16e. Table 276. OFDMA DIUC values */
enum DIUC_VALUES {
	Gap_PAPR_reduction  = 13,
	Extended_DIUC_2     = 14,
	Extended_DIUC       = 15
};

/* Spec 16e. Table 288. OFDMA UIUC values */
enum UIUC_VALUES {
	FAST_FEEDBACK           = 0,
	Extended_UIUC_2         = 11,
	CDMA_BWreq_Ranging      = 12,
	PAPR_Safety_zone        = 13,
	CDMA_Alloc_IE           = 14,
	Extended_UIUC           = 15
};

/* The robust diuc and uiuc value */
enum {
	RobustDIUC = 0,  // 0 ~ 12
	RobustUIUC = 1   // 1 ~ 10
};

/* Spec 16e. Table 268. DLFP structure for all FFT except FFT-128 */
struct DLFP_except_128 {
	uint8_t ch_bitmap:6;
	uint8_t repecode:2;
	uint8_t len:8;
	uint8_t coding:3;
	uint8_t rsv1:1;
	uint8_t rsv2:4;
};

/* Spec 16e. Table 268b. DLFP structure for FFT-128 */
struct DLFP_for_128 {
	uint8_t ch:1;
	uint8_t rsv:1;
	uint8_t repecode:2;
	uint8_t coding:3;
	uint8_t len:5;
};

class OFDMA_80216e:public NslObject {
	protected:
		int     frameDuCode;
		double  BW;
		double  n;
		int     Nfft;
		int     Nused;
		double  CPratio;
		double  Fs;
		double  df;
		double  Tb;
		double  Tg;
		double  Ts;
		double  PS;
		double  PSratio;
		int     _symbolsPerFrame;
		int     _DLsubchannels;
		int     _ULsubchannels;
		int     _DLsymbols;
		int     _ULsymbols;
		int     _TTG;
		int     _RTG;
		int     _RecvSensitivity;
		int     _ChannelID;
		double  _transPower;
		int     _dlmapLen;
		char    *_dlmapBuf;
		//double  _Gain;

		int     beamwidth;          // degrees
		double  freq_;              // MHz
		double  pointingDirection;  // degrees
		double  angularSpeed;       // degrees per second

		OFDMA_ChannelCoding   *_channelCoding;
		OFDMA_ChannelModel    *_channelModel;

		struct DCDBurstProfile  *_DCDProfile;
		struct UCDBurstProfile  *_UCDProfile;

		void    scheduleTimer   (timerObj *, double);
		void    dumpByteString  (char *, char *, int);

	public:
		OFDMA_80216e(uint32_t, uint32_t, struct plist *, const char *);
		~OFDMA_80216e();

		void    setChannelID    (uint8_t);
		uint8_t getChannelID    ();
		void    setDLsymbols    (int);
		void    setULsymbols    (int);
};

#endif                /* __NCTUNS_OFDMA_80216E_H__ */
