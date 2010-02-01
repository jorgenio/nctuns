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

#include <stdio.h>
#include <stdlib.h>
#include <nctuns_api.h>
#include <gbind.h>
#include "ofdm_80216.h"
#include "channel_coding.h"
#include "channel_model.h"

MODULE_GENERATOR(OFDM_80216);

extern char *WiMAXChannelCoding;

const double frameDurationCodes[] =
    { 2.5, 4.0, 5.0, 8.0, 10.0, 12.5, 20.0 };

OFDM_80216::OFDM_80216(u_int32_t type, u_int32_t id, struct plist *pl,
		       const char *name)
:NslObject(type, id, pl, name)
{
//	printf("%d OFDM_80216::OFDM_80216()\n", get_nid());

	/* Bind variables */
	vBind("ChannelID", &_ChannelID);
	//vBind("RecvSensitivity", &_RecvSensitivity);
	vBind("CSThresh", &_RecvSensitivity);
	vBind("freq", &freq_);
	vBind("TransPower", &transPower);
	//vBind("Gain", &Gain);

	/* Register variables */
	REG_VAR("frameDuration", &frameDuration);
	REG_VAR("CPratio", &CPratio);
	REG_VAR("Ts", &Ts);
	REG_VAR("PSratio", &PSratio);
	REG_VAR("PS", &PS);
	REG_VAR("symbolsPerFrame", &symbolsPerFrame);
	REG_VAR("transPower", &transPower);

	REG_VAR("FREQ", &freq_);
	REG_VAR("beamwidth", &beamwidth);
	REG_VAR("pointingDirection", &pointingDirection);
	REG_VAR("angularSpeed", &angularSpeed);
	REG_VAR("CSThresh", &_RecvSensitivity);


	freq_ = 5470; //Mhz
	beamwidth = 360;
	pointingDirection = 0;
	angularSpeed = 0;
	//Gain = 1.0;

	frameDuration = 10.0;	// 10ms
	CPratio = 1.0 / 4.0;	// Tg/Tb
	Tb = 11.11;		// us
	Tg = Tb * CPratio;	// us
	Ts = Tg + Tb;		// us
	PSratio = 1.0 / 64.0;	// PS/Tb
	PS = Tb * PSratio;	// us
	Nused = 192;
	symbolsPerFrame = (int) (frameDuration * 1000 / Ts);

#if 0
	printf
	    ("frameDuration=%lf, CPratio=%lf, Tb=%lf, Tg=%lf, Ts=%lf, PSratio=%lf, PS=%lf, symbolsPerFrame=%d\n",
	     frameDuration, CPratio, Tb, Tg, Ts, PSratio, PS,
	     symbolsPerFrame);
#endif

	if (WiMAXChannelCoding && !strcasecmp(WiMAXChannelCoding, "on")) {
		channelCoding = new ChannelCoding(true);
		channelModel =
		    new ChannelModel(freq_*1e-3, 20.0, 10.0, 20.0, Ts, true);
	} else {
		channelCoding = new ChannelCoding(false);
		channelModel =
		    new ChannelModel(freq_*1e-3, 20.0, 10.0, 20.0, Ts, false);
		//channelModel  = new ChannelModel(freq_*1e-3, 80.0, 10.0, 20.0, Ts, false);
	}
}

OFDM_80216::~OFDM_80216()
{
//	printf("%d OFDM_80216::~OFDM_80216()\n", get_nid());

	delete channelCoding;
	delete channelModel;
}

void OFDM_80216::scheduleTimer(timerObj * timer, double microSecond)
{
	u_int64_t timeInTick;

	MICRO_TO_TICK(timeInTick, microSecond);
	timer->start(timeInTick, 0);

//      printf("\tAt %lld, schedule a timer which will be triggered at %lld (%lf us later)\n", 
//              GetCurrentTime(), GetCurrentTime()+timeInTick, microSecond);
}

void OFDM_80216::dumpByteString(char *prompt, char *byteString, int len)
{
	int i;

	printf("%s: ", prompt);
	for (i = 0; i < len; i++) {
		if (i % 24 == 0)
			printf("\n");
		printf(" %02X", byteString[i] & 0xFF);
	}
	printf("\n");
}
