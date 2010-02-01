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

#ifndef __NCTUNS_OFDM_80216_H__
#define __NCTUNS_OFDM_80216_H__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include "channel_coding.h"
#include "channel_model.h"
#include "../mac/common.h"

#include <module/phy/cm.h>
#include <wphyInfo.h>

#include <regcom.h>
extern RegTable				RegTable_;

class downBurst;

using namespace std;

#define SPEED_OF_LIGHT		300000000.0

enum {
	DLFP_LEN = 1,
	SHORT_PREAMBLE = 1,
	LONG_PREAMBLE = 2
};

// 8.3.3.4.3 Rate ID encodings
// (Also 'FEC Code type' in our implementation, see Table 360)
enum rate_id_encodings {
	BPSK_1_2,		// 0
	QPSK_1_2,
	QPSK_3_4,
	QAM16_1_2,
	QAM16_3_4,
	QAM64_2_3,
	QAM64_3_4
};

/*  Merge special OFDM DIUC(Table 235) and UIUC values(Table 244) */
enum USAGECODE {
	STC = 0,
	Initial_Ranging = 1,
	REQ_Full = 2,
	REQ_Focus = 3,
	Focused_Contention = 4,
	Gap = 13,
	Subchannel_Network_Entry = 13,
	EndOfMap,
	Extended
};

enum {
	RobustDIUC = 1,
	RobustUIUC = 5
};

extern const double frameDurationCodes[];

// Table 223 - OFDM downlink frame prefix format (1 OFDM symbol, 11 bytes)

#define DLFP_IE_GET_LEN(prefix, num) \
	(prefix)->IE[num].len_msb*256 + (prefix)->IE[num].len_lsb

struct DLFP {
	u_char bs_id:4;
	u_char frame_num:4;
	u_char config_chg_cnt:4;
	u_char rsv:4;

	struct {
		u_char diuc:4;	// FIXME: rate_id or DIUC
		u_char has_preamble:1;
		u_char len_msb:3;	// # of OFDM symbols
		u_char len_lsb:8;
	} IE[4];

	u_char hcs:8;
};

class OFDM_80216:public NslObject {
      private:
	int *tunfd_;

      protected:
	double frameDuration;
	double CPratio;
	double Tb;
	double Tg;
	double Ts;
	double PSratio;
	double PS;
	//double Gain;
	int Nused;
	int symbolsPerFrame;
	double transPower;
	double _RecvSensitivity;
	int _ChannelID;

	int             beamwidth;
	double		freq_; // MHz
	double          pointingDirection;
	double          angularSpeed;   // degrees per second


	ChannelCoding *channelCoding;
	ChannelModel *channelModel;
	struct DCDBurstProfile *DCDProfile;
	struct UCDBurstProfile *UCDProfile;

	void scheduleTimer(timerObj * timer, double microSecond);
	void dumpByteString(char *prompt, char *byteString, int len);	// debug

      public:
	 OFDM_80216(u_int32_t type, u_int32_t id, struct plist *pl,
		    const char *name);
	~OFDM_80216();
};

#endif				/* __NCTUNS_OFDM_80216_H__ */
