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

#include "ofdma_80216j.h"

const double    frameDuration_MR[] = {0, 2.0, 2.5, 4.0, 5.0, 8.0, 10.0, 12.5, 20.0}; // Spec 16e. Table 274.
const int       repeat_times_MR[]  = {1, 2, 4, 6};
extern char     *MobileRelayWIMAXChannelCoding;

	OFDMA_80216j::OFDMA_80216j(uint32_t type, uint32_t id, struct plist *pl, const char *name)
:NslObject(type, id, pl, name)
{
	/* Bind variables */
	vBind("ChannelID",          &_ChannelID);
	vBind("TransPower",         &_transPower);
	vBind("CSThresh",           &_RecvSensitivity);
	vBind("freq",               &freq_);
	//vBind("Gain",             	&_Gain);

	/* Register variables */
	REG_VAR("frameDuCode",      &frameDuCode);
	REG_VAR("CPratio",          &CPratio);
	REG_VAR("Ts",               &Ts);
	REG_VAR("PSratio",          &PSratio);
	REG_VAR("PS",               &PS);
	REG_VAR("symbolsPerFrame",  &_symbolsPerFrame);
	REG_VAR("DLsubchannels",    &_DLsubchannels);
	REG_VAR("ULsubchannels",    &_ULsubchannels);
	REG_VAR("transPower",       &_transPower);
	REG_VAR("TTG",              &_TTG);
	REG_VAR("RTG",              &_RTG);
	REG_VAR("RSTTG",              &_RSTTG);
	REG_VAR("RSRTG",              &_RSRTG);
	REG_VAR("FREQ",             &freq_);
	REG_VAR("beamwidth",        &beamwidth);
	REG_VAR("pointingDirection",&pointingDirection);
	REG_VAR("angularSpeed",     &angularSpeed);
	//REG_VAR("Gain",             &_Gain);

	/* Antenna parameters */
	freq_               = 2300; // (MHz)
	beamwidth           = 360;
	pointingDirection   = 0;
	angularSpeed        = 0;

	/* Spec 16e. 12.4.3.9 OFDMA_ProfP8 */
	frameDuCode     = 4;    // (5ms)
	BW              = 10.0; // (MHz)
	n               = 28.0 / 25.0;
	CPratio         = 1.0 / 8.0;

	/* Spec 16e. 8.4.6 OFDMA subcarrier allocations (DL-PUSC and UL-PUSC) */
	Nfft            = 1024; // (subcarriers)
	Nused           = 840;  // (subcarriers)
	_DLsubchannels  = 30;   // (subchannels)
	_ULsubchannels  = 35;   // (subchannels)

	Fs      = 11200000;     //       Fs = floor(n * (BW * 1000000) / 8000) * 8000;
	df      = 10937.5;      // (Hz)  df = Fs / Nfft;
	Tb      = 91.4;         // (us)  Tb = 1 / df;
	Tg      = 11.425;       // (us)  Tg = CPratio * Tb;
	Ts      = 102.825;      // (us)  Ts = Tb + Tg;
	PS      = 0.357143;     // (us)  PS = 4.0 / (Fs / 1000000);
	PSratio = 1.0 / 256.0;  //       PSration = PS / Tb

	_TTG    = 90;           // (PS)
	_RTG    = 90;           // (PS)
	_RSTTG  = 30;
	_RSRTG  = 30;

	_symbolsPerFrame = (int) ((frameDuration_MR[frameDuCode] * 1000 - (_TTG + _RTG) * PS) / Ts);

	/* Initial channel model and channel coding */
	if (MobileRelayWIMAXChannelCoding && !strncasecmp(MobileRelayWIMAXChannelCoding, "on", 2))
	{
		_channelCoding  = new OFDMA_ChannelCoding_MR(true);
		_channelModel   = new OFDMA_ChannelModel_MR(BW, Ts, true);
	}
	else
	{
		_channelCoding  = new OFDMA_ChannelCoding_MR(false);
		_channelModel   = new OFDMA_ChannelModel_MR(BW, Ts, false);
	}

	assert(_channelCoding && _channelModel);
}

OFDMA_80216j::~OFDMA_80216j()
{
	delete _channelCoding;
	delete _channelModel;
}

void OFDMA_80216j::scheduleTimer(timerObj *timer, double microSecond)
{
	uint64_t timeInTick = 0;

	MICRO_TO_TICK(timeInTick, microSecond);
	timer->start(timeInTick, 0);
}

void OFDMA_80216j::setChannelID(uint8_t chID)
{
	_ChannelID = chID;
}

uint8_t OFDMA_80216j::getChannelID()
{
	return _ChannelID;
}

void OFDMA_80216j::setDLsymbols(int Nsymbols)
{
	_DLsymbols = Nsymbols;
}

void OFDMA_80216j::setULsymbols(int Nsymbols)
{
	_ULsymbols = Nsymbols;
}

void OFDMA_80216j::setDLAccessSymbols(int Nsymbols)
{
        _DLAccessSymbols = Nsymbols;
}

void OFDMA_80216j::setDLTransparentSymbols(int Nsymbols)
{
        _DLTransparentSymbols = Nsymbols;
}

void OFDMA_80216j::setULAccessSymbols(int Nsymbols)
{
        _ULAccessSymbols = Nsymbols;
}

void OFDMA_80216j::setULRelaySymbols(int Nsymbols)
{
        _ULRelaySymbols = Nsymbols;
}

void OFDMA_80216j::dumpByteString(char *prompt, char *byteString, int len)
{
	printf("%s: ", prompt);
	for (int i = 0; i < len; i++)
	{
		if (i % 24 == 0)
			printf("\n");
		printf(" %02X", byteString[i] & 0xFF);
	}
	printf("\n");
}
