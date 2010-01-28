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
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "ofdm_80211a.h"

#define DEBUG_CHANNEL_CODING    0
#define DEBUG_CHANNEL_DECODING  0


bool ChannelCoding_80211a::CC_EncoderTableInitiated = false;
bool ChannelCoding_80211a::CC_EncoderTableDeleted = false;
bool ChannelCoding_80211a::CC_DecoderTableInitiated = false;
bool ChannelCoding_80211a::CC_DecoderTableDeleted = false;
struct EncoderLookupEntry *ChannelCoding_80211a::CC_1_2_EncoderTable = NULL;
struct EncoderLookupEntry *ChannelCoding_80211a::CC_2_3_EncoderTable = NULL;
struct EncoderLookupEntry *ChannelCoding_80211a::CC_3_4_EncoderTable = NULL;
struct DecoderLookupEntry *ChannelCoding_80211a::CC_1_2_DecoderTable = NULL;
struct DecoderLookupEntry *ChannelCoding_80211a::CC_2_3_DecoderTable = NULL;
struct DecoderLookupEntry *ChannelCoding_80211a::CC_3_4_DecoderTable = NULL;

ChannelCoding_80211a::ChannelCoding_80211a(bool isEnabled)
{
	printf("== Initializing Channel Coding ==\n");

	isBypassing = !isEnabled;

	if (isBypassing) {
//		printf("Bypassing Channel Coding...\n");

		// Mandatory channel coding
		// hychen modified
		initCoding(3, 6, NULL, 48, 1, BPSK_1_2);
		initCoding(5, 6, NULL, 48, 1, BPSK_3_4);
		initCoding(6, 12, NULL, 96, 2, QPSK_1_2);
		initCoding(9, 12, NULL, 96, 2, QPSK_3_4);
		initCoding(12, 24, NULL, 192, 4, QAM16_1_2);
		initCoding(18, 24, NULL, 192, 4, QAM16_3_4);
		initCoding(24, 36, NULL, 288, 6, QAM64_2_3);
		initCoding(27, 36, NULL, 288, 6, QAM64_3_4);
		return;
	}


	printf("Initializing Convolutional Coder/Decoder...\n");
	//hychen modified  , for 802.11a OFDM Go = 133 , G1 = 171
	CC_1_2 = new ConvoCode_11a(0133, 0171, 7, 01, 01, 1);
	CC_2_3 = new ConvoCode_11a(0133, 0171, 7, 02, 03, 2);
	CC_3_4 = new ConvoCode_11a(0133, 0171, 7, 05, 06, 3);

	if (!CC_EncoderTableInitiated) {
		printf("Generating CC encoder table...\n");

		CC_1_2_EncoderTable = CC_1_2->genEncoderLookupTable();
		CC_2_3_EncoderTable = CC_2_3->genEncoderLookupTable();
		CC_3_4_EncoderTable = CC_3_4->genEncoderLookupTable();
		CC_EncoderTableInitiated = true;

		printf("%p %p %p\n", CC_1_2_EncoderTable, CC_2_3_EncoderTable, CC_3_4_EncoderTable);
	}

	CC_1_2->setEncoderLookupTable(CC_1_2_EncoderTable);
	CC_2_3->setEncoderLookupTable(CC_2_3_EncoderTable);
	CC_3_4->setEncoderLookupTable(CC_3_4_EncoderTable);

	if (!CC_DecoderTableInitiated) {
		printf("Generating CC decoder table...\n");

		CC_1_2_DecoderTable = CC_1_2->genDecoderLookupTable();
		CC_2_3_DecoderTable = CC_2_3->genDecoderLookupTable();
		CC_3_4_DecoderTable = CC_3_4->genDecoderLookupTable();
		CC_DecoderTableInitiated = true;
	}

	CC_1_2->setDecoderLookupTable(CC_1_2_DecoderTable);
	CC_2_3->setDecoderLookupTable(CC_2_3_DecoderTable);
	CC_3_4->setDecoderLookupTable(CC_3_4_DecoderTable);

	// Mandatory channel coding
	// hychen modified
	initCoding(3, 6, CC_1_2, 48, 1, BPSK_1_2);
        initCoding(5, 6, CC_3_4, 48, 1, BPSK_3_4);
        initCoding(6, 12, CC_1_2, 96, 2, QPSK_1_2);
        initCoding(9, 12, CC_3_4, 96, 2, QPSK_3_4);
        initCoding(12, 24, CC_1_2, 192, 4, QAM16_1_2);
        initCoding(18, 24, CC_3_4, 192, 4, QAM16_3_4);
        initCoding(24, 36, CC_2_3, 288, 6, QAM64_2_3);
        initCoding(27, 36, CC_3_4, 288, 6, QAM64_3_4);
	
}

ChannelCoding_80211a::~ChannelCoding_80211a()
{
	printf("ChannelCoding_80211a::~ChannelCoding_80211a()\n");

	if (isBypassing)
		return;

	//delete RS;
	delete CC_1_2;
	delete CC_2_3;
	delete CC_3_4;

	if (!CC_EncoderTableDeleted) {
		delete CC_1_2_EncoderTable;
		delete CC_2_3_EncoderTable;
		delete CC_3_4_EncoderTable;
		CC_EncoderTableDeleted = true;
	}

	if (!CC_DecoderTableDeleted) {
		delete CC_1_2_DecoderTable;
		delete CC_2_3_DecoderTable;
		delete CC_3_4_DecoderTable;
		CC_DecoderTableDeleted = true;
	}
}

int ChannelCoding_80211a::max(int a,int b)
{
	if (a >= b)
	return a;
	else	return b;
}
int ChannelCoding_80211a::encode(char *input, char *output, int inputLen,
			  int codeType)
{
	int uncodedBlockSize = coding[codeType].uncodedBlockSize;
	int codedBlockSize = coding[codeType].codedBlockSize;
	//hychen modified
	int numBlocks = (inputLen / uncodedBlockSize) + 1;
	char *inp = input;
	char *outp = output;
	char buf1[codedBlockSize];
	char buf2[codedBlockSize];
	char tmp[uncodedBlockSize];
	int payloadLen;
	int paddingLen;
	int i;

	if (isBypassing) {
		memcpy(output, &inputLen, sizeof(int));
		memcpy(output + sizeof(int), input, inputLen);
		return numBlocks * codedBlockSize ;
	}
#if DEBUG_CHANNEL_CODING
	coding[codeType].CC->resetDecoderState();
#endif

#if DEBUG_CHANNEL_CODING
	printf("===========================================\n");
	printf("ChannelCoding_80211a::encode()\n");
	printf("\tinputLen = %d\n", inputLen);
	printf("\tuncoded = %d, coded = %d\n", uncodedBlockSize,
	       codedBlockSize);
	printf("\tnumBlocks = %d\n", numBlocks);
	printf("\tinput %p, output %p\n", input, output);
#endif

	// Encode each block and copy the coded block to the output buffer
	for (i = 0; i < numBlocks; i++) {
		if (i == numBlocks - 1) {
			payloadLen = inputLen - i * uncodedBlockSize;
                        paddingLen = uncodedBlockSize - 1 - payloadLen;
		} else {
			payloadLen = uncodedBlockSize;
			paddingLen = 0;
		}

#if DEBUG_CHANNEL_CODING
		printf("\ninput[%d - %d]\n", i * uncodedBlockSize,
		       i * uncodedBlockSize + payloadLen - 1);
		printf("payloadLen = %d, paddingLen = %d\n", payloadLen,
		       paddingLen);
		dumpByteString("Before", inp, payloadLen);
#endif

		// Padding (if necessary)
		if (paddingLen != 0) {
			memcpy(tmp, inp, payloadLen);
			memset(tmp + payloadLen, 0xFF, paddingLen);	//hychen modified
			inp = tmp;

#if DEBUG_CHANNEL_CODING
			dumpByteString("Padd", tmp, uncodedBlockSize - 1);
#endif 
		}
		// Randomization , hychen modified
		randomize(inp, buf2 ,(payloadLen + paddingLen) * 8);

#if DEBUG_CHANNEL_CODING
		dumpByteString("Rand", buf2, payloadLen + paddingLen);
#endif

		// Add 0x00 tail byte to reset the CC encoder (if necessary) , hychen modified
		if (i == numBlocks - 1) {
			buf2[uncodedBlockSize - 1] = 0x00;

#if DEBUG_CHANNEL_CODING
                      dumpByteString("Tail", buf1, uncodedBlockSize);
#endif
		}
		// Convolutional code	, hychen modified
		coding[codeType].CC->encode(buf2, buf1,uncodedBlockSize*8);

#if DEBUG_CHANNEL_CODING
		dumpByteString("CC", buf1, codedBlockSize);
#endif

		// Interleaving
		interleave(buf1, buf2, codeType);

#if DEBUG_CHANNEL_CODING
		dumpByteString("Inter", buf2, codedBlockSize);
#endif

		memcpy(outp, buf2, codedBlockSize);
		//memcpy(outp, buf1, codedBlockSize);	//testing
		inp += uncodedBlockSize;
		outp += codedBlockSize;
	}
#if DEBUG_CHANNEL_CODING
	printf("===========================================\n");
#endif

	return numBlocks * codedBlockSize;	
}

int ChannelCoding_80211a::decode(char *input, char *output, int inputLen,
			  int codeType)
{
	int uncodedBlockSize = coding[codeType].uncodedBlockSize;
	int codedBlockSize = coding[codeType].codedBlockSize;

	int numBlocks = inputLen / codedBlockSize;
	char *inp = input;
	char *outp = output;
	char buf1[codedBlockSize * 2];
	char buf2[codedBlockSize];
	int len;
	int i;

	if (isBypassing) {
		int outputLen;

		// To make the CRC comfortable
		memset(output, 0xFF,getUncodedBurstLen(inputLen, codeType));

		memcpy(&outputLen, input, sizeof(int));
		memcpy(output, input + sizeof(int), outputLen);
		return outputLen;
	}

	coding[codeType].CC->resetDecoderState();

#if DEBUG_CHANNEL_DECODING
	printf("===========================================\n");
	printf("ChannelCoding_80211a::decode()\n");
	printf("\tinputLen = %d\n", inputLen);
	printf("\tuncoded = %d, coded = %d\n", uncodedBlockSize,
	       codedBlockSize);
	printf("\tnumBlocks = %d\n", numBlocks);
	printf("\tinput %p, output %p\n", input, output);
#endif

	// Decode each block and copy the decoded block to the output buffer
	for (i = 0; i < numBlocks - 1; i++) {
		// Deinterleaving
		deinterleave(inp, buf1, codeType);
		deinterleave(inp + codedBlockSize, buf1 + codedBlockSize,codeType);

#if DEBUG_CHANNEL_DECODING
		dumpByteString("Deint", buf1, codedBlockSize);
		interleave(buf1, buf2, codeType);
		dumpByteString("Inter", buf2, codedBlockSize);
#endif

		// Convolutional decode ,hychen modified
		coding[codeType].CC->decode(buf1, buf2, codedBlockSize*8 ,8*codedBlockSize / 2);

#if DEBUG_CHANNEL_DECODING
		dumpByteString("deCC", buf2, uncodedBlockSize);
		//coding[codeType].CC->encode(buf2, buf1, (uncodedBlockSize)*8);
		//dumpByteString("CC", buf1, codedBlockSize);
#endif
		len = uncodedBlockSize;

		// Derandomizationi , hychen modified
		derandomize(buf2, buf1, len*8);

#if DEBUG_CHANNEL_DECODING
		dumpByteString("DeRand", buf1, len);
		//randomize(buf2, buf1, len*8);
		//dumpByteString("Rand", buf1, len);
#endif

		memcpy(outp, buf1, len);
		inp += codedBlockSize;
		outp += uncodedBlockSize;
	}

	deinterleave(inp, buf1, codeType);
#if DEBUG_CHANNEL_DECODING
	dumpByteString("Deint", buf1, codedBlockSize);
#endif
	coding[codeType].CC->decode(buf1, buf2, codedBlockSize*8);

#if DEBUG_CHANNEL_DECODING
	dumpByteString("deCC", buf2, uncodedBlockSize);
#endif 
	// Remove 0x00 tail byte , hychen modified
	len = uncodedBlockSize - 1;
	derandomize(buf2, buf1, len * 8);

#if DEBUG_CHANNEL_DECODING
	dumpByteString("DeRand", buf1, len);
#endif
	memcpy(outp, buf1, len);

#if DEBUG_CHANNEL_DECODING
	printf("===========================================\n");
#endif

	// We do not remove the padding bytes here, so the returned length is
	// simply (numBlocks*uncodedBlockSize - 1).
	// hychen modified
	return numBlocks * uncodedBlockSize - 1;
	
}


int ChannelCoding_80211a::getUncodedBlockSize(int codeType)
{
	return coding[codeType].uncodedBlockSize;
}

int ChannelCoding_80211a::getCodedBlockSize(int codeType)
{
	return coding[codeType].codedBlockSize;
}

int ChannelCoding_80211a::getUncodedBurstLen(int codedBurstLen, int codeType)
{
	int uncodedBlockSize = coding[codeType].uncodedBlockSize;
	int codedBlockSize = coding[codeType].codedBlockSize;
	int numBlocks = codedBurstLen / codedBlockSize;

        return numBlocks * uncodedBlockSize - 1;
}

int ChannelCoding_80211a::getCodedBurstLen(int uncodedBurstLen, int codeType)
{
	int uncodedBlockSize = coding[codeType].uncodedBlockSize;
	int codedBlockSize = coding[codeType].codedBlockSize;
	int numBlocks = (uncodedBurstLen / uncodedBlockSize) + 1;

        return numBlocks * codedBlockSize;
}

// Initialize channel coding
void ChannelCoding_80211a::initCoding(int uncodedBlockSize, int codedBlockSize,
			       ConvoCode_11a * CC, int Ncbps, int Nbpsc,
			       int codeType)
{
	coding[codeType].uncodedBlockSize = uncodedBlockSize;
	coding[codeType].codedBlockSize = codedBlockSize;
	coding[codeType].CC = CC;
	coding[codeType].Ncbps = Ncbps;
	coding[codeType].Nbpsc = Nbpsc;
}

// Perform on each burst of data on the DL and UL.
void ChannelCoding_80211a::randomize(char *input, char *output, int inputLen)
{
	int seed = 0x7f;	//initiate state with all ones, hychen modified
	int in, out, tmp;
	int i;

	memset(output, 0, inputLen / 8);

	for (i = 0; i < inputLen; i++) {
		// Extract 1-bit input
		in = ((input[i / 8] << (i % 8)) & 0x80) >> 7;

		tmp = (seed & 0x1) ^ ((seed & 0x8) >> 3);	//S(x)= x^7+x^4+1
		out = in ^ tmp;
		seed = (seed >> 1) | (tmp << 7);

		output[i / 8] <<= 1;
		output[i / 8] |= out;
	}
}

void ChannelCoding_80211a::derandomize(char *input, char *output, int inputLen)
{
	randomize(input, output, inputLen);
}

void ChannelCoding_80211a::interleave(char *input, char *output, int codeType)
{
	int Ncbps = coding[codeType].Ncbps;
	int Nbpsc = coding[codeType].Nbpsc;
	int s = max(Nbpsc/2 ,1);	//hychen modified
	int in;
	int j, k, m;

	memset(output, 0, Ncbps / 8);

	for (k = 0; k < Ncbps; k++) {
		// Extract 1-bit input
		in = ((input[k / 8] << (k % 8)) & 0x80) >> 7;

		m = (Ncbps / 16) * (k % 16) + (k / 16);
		j = s * (m / s) + (m + Ncbps - (16 * m / Ncbps)) % s;
		output[j / 8] |= (in << (7 - j % 8));
	}
}

void ChannelCoding_80211a::deinterleave(char *input, char *output, int codeType)
{
	int Ncbps = coding[codeType].Ncbps;
	int Nbpsc = coding[codeType].Nbpsc;
	int s = max(Nbpsc/2 ,1);        //hychen modified
	int in;
	int j, k, m;

	memset(output, 0, Ncbps / 8);

	for (j = 0; j < Ncbps; j++) {
		// Extract 1-bit input
		in = ((input[j / 8] << (j % 8)) & 0x80) >> 7;

		m = s * (j / s) + (j + (16 * j / Ncbps)) % s;
		k = 16 * m - (Ncbps - 1) * (16 * m / Ncbps);
		output[k / 8] |= (in << (7 - k % 8));
	}
}

void ChannelCoding_80211a::dumpByteString(char *prompt, char *byteString, int len)
{
	int i;

	printf("%s:\t", prompt);
	for (i = 0; i < len; i++) {
		printf("%02X ", byteString[i] & 0xFF);
	}
	printf("\n");
}
