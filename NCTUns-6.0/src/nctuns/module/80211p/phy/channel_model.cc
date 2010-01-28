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
#include <math.h>
#include <nctuns_api.h>
#include "ofdm_80211p.h"
#include "channel_model.h"

#define VERBOSE_LEVEL MSG_WARNING
//#include "../mac/verbose.h"

ChannelModel_80211p::ChannelModel_80211p(double frequency, double bandwidth,double symbolDuration, bool isEnabled)
{
	//printf("== Initializing Channel Model ==\n");

	isBypassing = !isEnabled;

	_frequency = frequency;
	_bandwidth = bandwidth;
							//hychen modified
	_dataRate[BPSK_1_2] = 3*8  / symbolDuration;
        _dataRate[BPSK_3_4] = 4.5*8  / symbolDuration;
        _dataRate[QPSK_1_2] = 6*8  / symbolDuration;
        _dataRate[QPSK_3_4] = 9*8  / symbolDuration;
        _dataRate[QAM16_1_2] = 12*8  / symbolDuration;
        _dataRate[QAM16_3_4] = 18*8  / symbolDuration;
        _dataRate[QAM64_2_3] = 24*8  / symbolDuration;
        _dataRate[QAM64_3_4] = 27*8  / symbolDuration;
}

ChannelModel_80211p::~ChannelModel_80211p()
{
	//printf("ChannelModel_80211p::~ChannelModel_80211p()\n");
}

double ChannelModel_80211p::computeBER(int codeType, double SNR)
{
	double ENR = SNRToENR(SNR, _dataRate[codeType]);
	double BER;

	/*printf("ChannelModel_80211p::computeBER()\n");
	printf("\tcodeType = %d\n", codeType);
	printf("\tSNR      = %lf\n", SNR);
	printf("\tdataRate = %lf\n", _dataRate[codeType]);
	printf("\tENR      = %lf dB\n", ENR);*/

	switch (codeType) {
	case BPSK_1_2:
	case BPSK_3_4:
		BER = BPSK(ENR);
		break;
	case QPSK_1_2:
	case QPSK_3_4:
		BER = BPSK(ENR);
		break;
	case QAM16_1_2:
	case QAM16_3_4:
		BER = QAM(16, ENR);
		break;
	case QAM64_2_3:
	case QAM64_3_4:
		BER = QAM(64, ENR);
		break;
	default:
		break;
	}
	//printf("\tBER      = %lf\n", BER);

	return BER;
}

double ChannelModel_80211p::computeBERbyENR(int codeType, double ENR)
{
	double BER;

	switch (codeType) {
	case BPSK_1_2:
	case BPSK_3_4:
		BER = BPSK(ENR);
		break;
	case QPSK_1_2:
	case QPSK_3_4:
		BER = BPSK(ENR);
		break;
	case QAM16_1_2:
	case QAM16_3_4:
		BER = QAM(16, ENR);
		break;
	case QAM64_2_3:
	case QAM64_3_4:
		BER = QAM(64, ENR);
		break;
	default:
		break;
	}
	//printf("\tBER      = %lf\n", BER);

	return BER;
}

void ChannelModel_80211p::makeBitError(char *input, int inputLen, double BER)
{
	int nbits = inputLen * 8;
	int nerrors = (int) (nbits * BER);
	int errorPos;
	int bytePos;
	int bitPos;
	char bitMap;
	int i;

	/*printf("ChannelModel_80211p::makeBitError()\n");
	printf("\tinputLen = %d, nerrors = %d, BER = %lf\n", inputLen,
	       nerrors, BER);*/

	if (isBypassing) {
		return;
	}

	for (i = 0; i < nerrors; i++) {
		errorPos = random() % nbits;
		bytePos = errorPos / 8;
		bitPos = errorPos % 8;
		bitMap = 1 << (7 - bitPos);
		input[bytePos] ^= bitMap;

		//printf("\t%dth bit\n", errorPos);
	}
}

//
// Reference:
//
//   Theodore S. Rappaport, "Wireless Communications - Principles & Practice,"
//   Prentice Hall, 1996, p.286.
//
#if 0
double ChannelModel_80211p::BPSK(double ENR)
{
	double BER = 0.5 / (1 + pow(10.0, ENR / 10.0));

	return BER;
}
#endif

double ChannelModel_80211p::BPSK(double ENR)
{
	double r = pow(10.0, ENR / 10.0);
	double BER = (1.0 - sqrt(r / (1.0 + r))) / 2.0;

	return BER;
}

double ChannelModel_80211p::QPSK(double ENR)
{
	return BPSK(ENR);
}

//
// Reference:
//
//   Marvin K. Simon, Mohamed-Slim Alouini, "Digital Communication over Fading
//   Channels - A Unified Approach to Performance Analysis," John Wiley & Sons,
//   2000, p.223.
//
double ChannelModel_80211p::QAM(int M, double ENR)
{
	double BER;
	double d, logm;
	double sum = 0;
	int n = (int) (sqrt(M) / 2);
	int i;

	ENR = pow(10.0, ENR / 10.0);
	logm = log2(M);

	for (i = 1; i <= n; i++) {
		d = 1.5 * (2 * i - 1) * (2 * i - 1) * ENR * logm;
		sum += (1.0 - sqrt(d / (M - 1.0 + d)));
	}

	BER = 2 * (sqrt(M) - 1) / sqrt(M) / logm * sum;

	return BER;
}

double ChannelModel_80211p::powerToSNR(double power)
{
	// SNR = S(dBm) - N,  where N = k*T*B or N = 10log(k*T*B) in dbW
	//     = S - 10*logk - 10*logT - 10*logB -30    (Assume T = 290K)
	//     = S + 228.6 - 24.6 - 10*logB -30
	//     = S - 10*logB + 174;
	return power - 10 * log10(_bandwidth * 1000000) + 174;
}

//
// Reference:
//
//   William Stallings, "Data & Computer Communications," Prentice Hall, 2000,
//   p.97.
//
double ChannelModel_80211p::SNRToENR(double SNR, double dataRate)
{
	// Eb/N0 = (S/R) / (k*T)
	//       = (S/R) / (N/B)
	//       = (S/N) / (R/B)
	//       = SNR - 10*log(R/B)
	return SNR - 10 * log10(dataRate / _bandwidth);
}
