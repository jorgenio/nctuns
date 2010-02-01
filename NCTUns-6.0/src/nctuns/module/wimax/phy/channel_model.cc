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
#include "ofdm_80216.h"
#include "channel_model.h"

#define VERBOSE_LEVEL MSG_WARNING
#include "../mac/verbose.h"

ChannelModel::ChannelModel(double frequency, double txHeight,
			   double rxHeight, double bandwidth,
			   double symbolDuration, bool isEnabled)
{
	DEBUG("== Initializing Channel Model ==\n");

	isBypassing = !isEnabled;

	_frequency = frequency;
	_txHeight = txHeight;
	_rxHeight = rxHeight;
	_bandwidth = bandwidth;

	_dataRate[BPSK_1_2] = 24 * 8 / symbolDuration;
	_dataRate[QPSK_1_2] = 48 * 8 / symbolDuration;
	_dataRate[QPSK_3_4] = 48 * 8 / symbolDuration;
	_dataRate[QAM16_1_2] = 96 * 8 / symbolDuration;
	_dataRate[QAM16_3_4] = 96 * 8 / symbolDuration;
	_dataRate[QAM64_2_3] = 144 * 8 / symbolDuration;
	_dataRate[QAM64_3_4] = 144 * 8 / symbolDuration;
}

ChannelModel::~ChannelModel()
{
	DEBUG("ChannelModel::~ChannelModel()\n");
}

double ChannelModel::receivePower(double transPower, double dist)
{
	if (dist == 0.0)
		dist = 10;
	return transPower - pathLoss(dist);
}


double ChannelModel::computeSNR(double transPower, double dist)
{
//      double pLoss = pathLoss(dist);
//      double power = transPower - pLoss;
//      double SNR   = powerToSNR(power);
	return powerToSNR(receivePower(transPower, dist));
}

double ChannelModel::computeBER(int codeType, double SNR)
{
	double ENR = SNRToENR(SNR, _dataRate[codeType]);
	double BER;

	DEBUG("ChannelModel::computeBER()\n");
	DEBUG("\tcodeType = %d\n", codeType);
	DEBUG("\tSNR      = %lf\n", SNR);
	DEBUG("\tdataRate = %lf\n", _dataRate[codeType]);
	DEBUG("\tENR      = %lf dB\n", ENR);

	switch (codeType) {
	case BPSK_1_2:
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
	DEBUG("\tBER      = %lf\n", BER);

//      BER = QAM(64, SNRToENR(SNR, _dataRate[QAM64_2_3]));
	return BER;
}

double ChannelModel::computeBERbyENR(int codeType, double ENR)
{
	double BER;

	switch (codeType) {
	case BPSK_1_2:
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
	DEBUG("\tBER      = %lf\n", BER);

	return BER;
}

void ChannelModel::makeBitError(char *input, int inputLen, double BER)
{
	int nbits = inputLen * 8;
	int nerrors = (int) (nbits * BER);
	int errorPos;
	int bytePos;
	int bitPos;
	char bitMap;
	int i;

	DEBUG("ChannelModel::makeBitError()\n");
	DEBUG("\tinputLen = %d, nerrors = %d, BER = %lf\n", inputLen,
	       nerrors, BER);

	if (isBypassing) {
		return;
	}

	for (i = 0; i < nerrors; i++) {
		errorPos = random() % nbits;
		bytePos = errorPos / 8;
		bitPos = errorPos % 8;
		bitMap = 1 << (7 - bitPos);
		input[bytePos] ^= bitMap;

		//DEBUG("\t%dth bit\n", errorPos);
	}
}

//
// Reference:
//
//   V. Erceg et. al, "An empirically based path loss model for wireless
//   channels in suburban environments," IEEE JSAC, vol. 17, no. 7, July 1999,
//   pp. 1205-1211.
//
// Median path loss:
//
//   PL = A + 10*r*log(d/d0) + s; d>=d0
//
// Freqyence correction term:
//
//   PLf = 6*log(f/2000); f is the frequency in MHz
//
// Receive antenna height correction term:
//
//   PLh = -10.8 * log(h/2); for category B
// 
// Note: (Am i right?)
//
//   10 <= BS antenna height <= 80
//    2 <= SS antenna height <= 10
//
double ChannelModel::pathLoss(double dist)
{
	/*
	 * We adopt terrain category B.
	 */
	double A = 78.0;
	double a = 4.0;
	double b = 0.0065;
	double c = 17.1;
	double r = a - b * _txHeight + c / _txHeight;
	double d0 = 100.0;
	double x = gaussian(-1.5, 1.5);
	double y = gaussian(-2.0, 2.0);
	double z = gaussian(-2.0, 2.0);
	double mean = 9.6;
	double stdDev = 3.0;
	double s = 10.0 * x * log10(dist / d0) + y * mean + y * z * stdDev;
	double PLf = 6 * log10(_frequency * 1000.0 / 2000.0);
	double PLh = -10.8 * log10(_rxHeight / 2.0);
	double PL = A + 10.0 * r * log10(dist / d0) + s + PLf + PLh;

	return PL;
}

//
// Reference:
//
//   Theodore S. Rappaport, "Wireless Communications - Principles & Practice,"
//   Prentice Hall, 1996, p.286.
//
#if 0
double ChannelModel::BPSK(double ENR)
{
	double BER = 0.5 / (1 + pow(10.0, ENR / 10.0));

	return BER;
}
#endif

double ChannelModel::BPSK(double ENR)
{
	double r = pow(10.0, ENR / 10.0);
	double BER = (1.0 - sqrt(r / (1.0 + r))) / 2.0;

	return BER;
}

double ChannelModel::QPSK(double ENR)
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
double ChannelModel::QAM(int M, double ENR)
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

double ChannelModel::powerToSNR(double power)
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
double ChannelModel::SNRToENR(double SNR, double dataRate)
{
	// Eb/N0 = (S/R) / (k*T)
	//       = (S/R) / (N/B)
	//       = (S/N) / (R/B)
	//       = SNR - 10*log(R/B)
	return SNR - 10 * log10(dataRate / _bandwidth);
}
