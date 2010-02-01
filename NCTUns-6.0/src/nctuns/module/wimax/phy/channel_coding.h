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
#include "fec/rs/rs_code.h"

class ChannelCoding {
      private:
	bool isBypassing;

	struct {
		int uncodedBlockSize;
		int codedBlockSize;
		int T;		// Error-correction capability of RS
		ConvoCode *CC;
		int Ncbps;
		int Ncpc;
	} coding[7];

	/*
	 * Reed-Solomon code (N=255, K=239, T=8)
	 *
	 * This code is shortened and punctured to enable variable block
	 * sizes and variable error-correction capability.
	 */
	ReedSolomonCode *RS;

	/*
	 * Convolutional code (1/2, 2/3, 3/4, 5/6)
	 *
	 * Note that each encoder/decoder lookup table is only generated
	 * once for all class instances to save memory space.
	 */
	ConvoCode *CC_1_2;
	ConvoCode *CC_2_3;
	ConvoCode *CC_3_4;
	ConvoCode *CC_5_6;
	static bool CC_EncoderTableInitiated;
	static bool CC_EncoderTableDeleted;
	static bool CC_DecoderTableInitiated;
	static bool CC_DecoderTableDeleted;
	static struct EncoderLookupEntry *CC_1_2_EncoderTable;
	static struct EncoderLookupEntry *CC_2_3_EncoderTable;
	static struct EncoderLookupEntry *CC_3_4_EncoderTable;
	static struct EncoderLookupEntry *CC_5_6_EncoderTable;
	static struct DecoderLookupEntry *CC_1_2_DecoderTable;
	static struct DecoderLookupEntry *CC_2_3_DecoderTable;
	static struct DecoderLookupEntry *CC_3_4_DecoderTable;
	static struct DecoderLookupEntry *CC_5_6_DecoderTable;

	void initCoding(int uncodedBlockSize, int codedBlockSize,
			int T, ConvoCode * CC, int Ncbps, int Ncpc,
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
	 ChannelCoding(bool isEnabled);
	~ChannelCoding();

	int encode(char *input, char *output, int inputLen, int codeType);
	int decode(char *input, char *output, int inputLen, int codeType);
	int getUncodedBlockSize(int codeType);
	int getCodedBlockSize(int codeType);
	int getUncodedBurstLen(int uncodedBurstLen, int codeType);
	int getCodedBurstLen(int uncodedBurstLen, int codeType);
};

#endif				/* __NCTUNS_CHANNEL_CODING_H__ */
