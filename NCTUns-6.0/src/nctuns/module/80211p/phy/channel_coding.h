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

#ifndef __NCTUNS_CHANNEL_CODING_H__
#define __NCTUNS_CHANNEL_CODING_H__

#include "fec/cc/conv_code.h"
//#include "fec/rs/rs_code.h"

class ChannelCoding_80211p {
      private:
	bool isBypassing;

	struct {
		int uncodedBlockSize;	//hychen modified ,  unit of bit
		int codedBlockSize;
		//int T;		// Error-correction capability of RS
		ConvoCode_ *CC;
		//int Ndbps;	//data bits per OFDM symbol
		int Ncbps;	//coded bits per OFDM symbol
		int Nbpsc;	//numbber of coded bits per subcarrier , ie, 1,2,4,or 6 for BPSK , QPSK , 16-QAM or 64-QAM
	} coding[8];

	/*
	 * Convolutional code (1/2, 2/3, 3/4)
	 *
	 * Note that each encoder/decoder lookup table is only generated
	 * once for all class instances to save memory space.
	 */
	ConvoCode_ *CC_1_2;
	ConvoCode_ *CC_2_3;
	ConvoCode_ *CC_3_4;
	static bool CC_EncoderTableInitiated;
	static bool CC_EncoderTableDeleted;
	static bool CC_DecoderTableInitiated;
	static bool CC_DecoderTableDeleted;
	static struct EncoderLookupEntry *CC_1_2_EncoderTable;
	static struct EncoderLookupEntry *CC_2_3_EncoderTable;
	static struct EncoderLookupEntry *CC_3_4_EncoderTable;
	static struct DecoderLookupEntry *CC_1_2_DecoderTable;
	static struct DecoderLookupEntry *CC_2_3_DecoderTable;
	static struct DecoderLookupEntry *CC_3_4_DecoderTable;

	void initCoding(int uncodedBlockSize, int codedBlockSize,
			ConvoCode_ * CC, int Ncbps, int Nbpsc,
			int codeType);
	void randomize(char *input, char *output, int inputLen);
	void derandomize(char *input, char *output, int inputLen);
	void interleave(char *input, char *output, int codeType);
	void deinterleave(char *input, char *output, int codeType);
	/*
	   void modulate();
	   void demodulate();
	 */

	// debug
	void dumpByteString(char *prompt, char *byteString, int len);

      public:
	 ChannelCoding_80211p(bool isEnabled);
	~ChannelCoding_80211p();

	int max(int a,int b);
	int encode(char *input, char *output, int inputLen, int codeType);
	int decode(char *input, char *output, int inputLen, int codeType);
	int getUncodedBlockSize(int codeType);
	int getCodedBlockSize(int codeType);
	int getUncodedBurstLen(int uncodedBurstLen, int codeType);
	int getCodedBurstLen(int uncodedBurstLen, int codeType);
};

#endif				/* __NCTUNS_CHANNEL_CODING_H__ */
