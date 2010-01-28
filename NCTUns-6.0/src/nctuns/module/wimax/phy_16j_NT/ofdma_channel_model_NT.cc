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

#include "ofdma_channel_model_NT.h"

OFDMA_ChannelModel_NT::OFDMA_ChannelModel_NT(double bandwidth, double symbolDuration, bool isEnabled)
{
	isBypassing = !isEnabled;
	_bandwidth  = bandwidth;

	_dataRate[QPSK_1_2]  =  6 * 8 / symbolDuration;
	_dataRate[QPSK_3_4]  =  9 * 8 / symbolDuration;
	_dataRate[QAM16_1_2] = 12 * 8 / symbolDuration;
	_dataRate[QAM16_3_4] = 18 * 8 / symbolDuration;
	_dataRate[QAM64_1_2] = 18 * 8 / symbolDuration;
	_dataRate[QAM64_2_3] = 24 * 8 / symbolDuration;
	_dataRate[QAM64_3_4] = 27 * 8 / symbolDuration;
}

OFDMA_ChannelModel_NT::~OFDMA_ChannelModel_NT()
{
	;
}

double OFDMA_ChannelModel_NT::computeBER(int codeType, double SNR)
{
	double ENR = SNRToENR(SNR, _dataRate[codeType]);
	double BER = 0.0;

	switch (codeType) {
		case BPSK_:
		case QPSK_1_2:
		case QPSK_3_4:
			BER = BPSK(ENR);
			break;

		case QAM16_1_2:
		case QAM16_3_4:
			BER = QAM(16, ENR);
			break;

		case QAM64_1_2:
		case QAM64_2_3:
		case QAM64_3_4:
			BER = QAM(64, ENR);
			break;

		default:
			break;
	}

	return BER;
}

double OFDMA_ChannelModel_NT::computeBERbyENR(int codeType, double ENR)
{
	double BER = 0.0;

	switch (codeType) {
		case BPSK_:
		case QPSK_1_2:
		case QPSK_3_4:
			BER = BPSK(ENR);
			break;

		case QAM16_1_2:
		case QAM16_3_4:
			BER = QAM(16, ENR);
			break;

		case QAM64_1_2:
		case QAM64_2_3:
		case QAM64_3_4:
			BER = QAM(64, ENR);
			break;

		default:
			break;
	}

	return BER;
}

void OFDMA_ChannelModel_NT::makeBitError(char *input, int inputLen, double BER)
{
	int nbits    = inputLen * 8;
	int nerrors  = (int) (nbits * BER);
	int errorPos = 0;
	int bytePos  = 0;
	int bitPos   = 0;
	char bitMap  = 0;

	if (isBypassing)
	{
		return;
	}

	for (int i = 0; i < nerrors; i++)
	{
		errorPos = random() % nbits;
		bytePos = errorPos / 8;
		bitPos = errorPos % 8;
		bitMap = 1 << (7 - bitPos);
		input[bytePos] ^= bitMap;
	}
}

/*
 * Reference:
 *
 *   Theodore S. Rappaport, "Wireless Communications - Principles & Practice,"
 *   Prentice Hall, 1996, p.286.
 */
double OFDMA_ChannelModel_NT::BPSK(double ENR)
{
	double r   = pow(10.0, ENR / 10.0);
	double BER = (1.0 - sqrt(r / (1.0 + r))) / 2.0;

	return BER;
}

double OFDMA_ChannelModel_NT::QPSK(double ENR)
{
	return BPSK(ENR);
}

/*
 * Reference:
 *
 *   Marvin K. Simon, Mohamed-Slim Alouini, "Digital Communication over Fading
 *   Channels - A Unified Approach to Performance Analysis," John Wiley & Sons,
 *   2000, p.223.
 */
double OFDMA_ChannelModel_NT::QAM(int M, double ENR)
{
	double BER  = 0.0;
	double d    = 0.0;
	double logm = 0.0;
	double sum  = 0.0;
	int n       = (int) (sqrt(M) / 2);

	ENR = pow(10.0, ENR / 10.0);
	logm = log2(M);

	for (int i = 1; i <= n; i++)
	{
		d = 1.5 * (2 * i - 1) * (2 * i - 1) * ENR * logm;
		sum += (1.0 - sqrt(d / (M - 1.0 + d)));
	}

	BER = 2 * (sqrt(M) - 1) / sqrt(M) / logm * sum;

	return BER;
}

double OFDMA_ChannelModel_NT::powerToSNR(double power)
{
	return power - 10 * log10(11.2 * 840 / 1024) + 114 - 5 - 8; // Spec 16e. 8.4.13.1
}

/*
 * Reference:
 *
 *   William Stallings, "Data & Computer Communications," Prentice Hall, 2000,
 *   p.97.
 */
double OFDMA_ChannelModel_NT::SNRToENR(double SNR, double dataRate)
{
	return SNR - 10 * log10(dataRate / _bandwidth);
}
