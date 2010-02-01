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

#include "ofdma_channel_coding_MR.h"

bool OFDMA_ChannelCoding_MR::_CC_EncoderTableInitiated   = false;
bool OFDMA_ChannelCoding_MR::_CC_EncoderTableDeleted     = false;
bool OFDMA_ChannelCoding_MR::_CC_DecoderTableInitiated   = false;
bool OFDMA_ChannelCoding_MR::_CC_DecoderTableDeleted     = false;

struct EncoderLookupEntry *OFDMA_ChannelCoding_MR::_CC_1_2_EncoderTable = NULL;
struct EncoderLookupEntry *OFDMA_ChannelCoding_MR::_CC_2_3_EncoderTable = NULL;
struct EncoderLookupEntry *OFDMA_ChannelCoding_MR::_CC_3_4_EncoderTable = NULL;
struct DecoderLookupEntry *OFDMA_ChannelCoding_MR::_CC_1_2_DecoderTable = NULL;
struct DecoderLookupEntry *OFDMA_ChannelCoding_MR::_CC_2_3_DecoderTable = NULL;
struct DecoderLookupEntry *OFDMA_ChannelCoding_MR::_CC_3_4_DecoderTable = NULL;

OFDMA_ChannelCoding_MR::OFDMA_ChannelCoding_MR(bool isEnabled)
{
	_isBypassing = !isEnabled;

	/* Spec. 16e: Table 318 */ 
	_concatenation_j[0] = 6;
	_concatenation_j[1] = 4;
	_concatenation_j[2] = 3;
	_concatenation_j[3] = 2;
	_concatenation_j[4] = 2;
	_concatenation_j[5] = 1;
	_concatenation_j[6] = 1;

	if (_isBypassing)
	{
		// Mandatory channel coding
		initCoding( 6, 12, NULL, 2, QPSK_1_2);
		initCoding( 9, 12, NULL, 2, QPSK_3_4);
		initCoding(12, 24, NULL, 4, QAM16_1_2);
		initCoding(18, 24, NULL, 4, QAM16_3_4);
		initCoding(18, 36, NULL, 6, QAM64_1_2);
		initCoding(24, 36, NULL, 6, QAM64_2_3);
		initCoding(27, 36, NULL, 6, QAM64_3_4);
		return;
	}

	//printf("Initializing OFDMA Convolutional Coder/Decoder...\n");
	_CC_1_2 = new OFDMA_ConvoCode_MR(0171, 0133, 7, 01, 01, 1);
	_CC_2_3 = new OFDMA_ConvoCode_MR(0171, 0133, 7, 02, 03, 2);
	_CC_3_4 = new OFDMA_ConvoCode_MR(0171, 0133, 7, 05, 06, 3);

	if (!_CC_EncoderTableInitiated)
	{
		//printf("Generating CC encoder table...\n");
		_CC_1_2_EncoderTable = _CC_1_2->genEncoderLookupTable();
		_CC_2_3_EncoderTable = _CC_2_3->genEncoderLookupTable();
		_CC_3_4_EncoderTable = _CC_3_4->genEncoderLookupTable();
		_CC_EncoderTableInitiated = true;
	}

	_CC_1_2->setEncoderLookupTable(_CC_1_2_EncoderTable);
	_CC_2_3->setEncoderLookupTable(_CC_2_3_EncoderTable);
	_CC_3_4->setEncoderLookupTable(_CC_3_4_EncoderTable);

	if (!_CC_DecoderTableInitiated)
	{
		//printf("Generating CC decoder table...\n");
		_CC_1_2_DecoderTable = _CC_1_2->genDecoderLookupTable();
		_CC_2_3_DecoderTable = _CC_2_3->genDecoderLookupTable();
		_CC_3_4_DecoderTable = _CC_3_4->genDecoderLookupTable();
		_CC_DecoderTableInitiated = true;
	}

	_CC_1_2->setDecoderLookupTable(_CC_1_2_DecoderTable);
	_CC_2_3->setDecoderLookupTable(_CC_2_3_DecoderTable);
	_CC_3_4->setDecoderLookupTable(_CC_3_4_DecoderTable);

	// Mandatory channel coding
	initCoding( 6, 12, _CC_1_2, 2, QPSK_1_2);
	initCoding( 9, 12, _CC_3_4, 2, QPSK_3_4);
	initCoding(12, 24, _CC_1_2, 4, QAM16_1_2);
	initCoding(18, 24, _CC_3_4, 4, QAM16_3_4);
	initCoding(18, 36, _CC_1_2, 6, QAM64_1_2);
	initCoding(24, 36, _CC_2_3, 6, QAM64_2_3);
	initCoding(27, 36, _CC_3_4, 6, QAM64_3_4);
}

OFDMA_ChannelCoding_MR::~OFDMA_ChannelCoding_MR()
{
	if (_isBypassing)
		return;

	delete _CC_1_2;
	delete _CC_2_3;
	delete _CC_3_4;

	if (!_CC_EncoderTableDeleted)
	{
		delete _CC_1_2_EncoderTable;
		delete _CC_2_3_EncoderTable;
		delete _CC_3_4_EncoderTable;
		_CC_EncoderTableDeleted = true;
	}

	if (!_CC_DecoderTableDeleted)
	{
		delete _CC_1_2_DecoderTable;
		delete _CC_2_3_DecoderTable;
		delete _CC_3_4_DecoderTable;
		_CC_DecoderTableDeleted = true;
	}
}

int OFDMA_ChannelCoding_MR::setEncoderState(char *input, int inputBytes, int j, int n, int k, int m, int codeType)
{
	int index               = 0;
	char state              = 0;
	int uncodedBlockSize    = 0;
	char *output = NULL;

	// compute the last block of input data
	if (n <= j)
	{
		index = n;
	}
	else
	{
		if (m == 0)
		{
			index = j;
		}
		else
		{
			index = (m + j) / 2;
		}
	}

	uncodedBlockSize = _coding[codeType].uncodedPayload[index - 1];
	output = new char [uncodedBlockSize];
	memset(output, 0, uncodedBlockSize);

	randomize(input + inputBytes - uncodedBlockSize, output, uncodedBlockSize * 8);

	// Inverse the last 6 bits to be the tailbiting enoder's initial state
	for (int j = 0;j < 6;j++)
	{
		state <<= 1;
		state |= (output[uncodedBlockSize - 1] >> j) & 0x01;
	}

	_coding[codeType].CC->resetEncoderState(state);
	delete output;

	return state;
}

// repeatTimes just use to compute some parameter, (1, 2, 4, 6)
// not repeat here
int OFDMA_ChannelCoding_MR::encode(char *input, char *output, int inputBytes, int inputSlots, int repeatTimes, int codeType)
{
	int codedLen    = 0;
	int paddingLen  = 0;
	int uncodedSize = _coding[codeType].uncodedPayload[0];// per slot
	int codedSize   = _coding[codeType].codedPayload[0];// per slot
	int j           = _concatenation_j[codeType];
	int n           = inputSlots / repeatTimes;
	int k           = n / j;
	int m           = n % j;
	int state       = 0;
	char tmp[n * uncodedSize];

	if (_isBypassing)
	{
		memcpy(output, &inputBytes, sizeof(short));
		memcpy(output + sizeof(short), input, inputBytes);
		return inputSlots * codedSize;
	}

	//clear
	memset(tmp, 0, n * uncodedSize);
	memcpy(tmp, input, inputBytes);

	//padding 0xFF if needed
	paddingLen = n * uncodedSize - inputBytes;
	if (paddingLen != 0)
		memset(tmp + inputBytes, 0xFF, paddingLen);

	//init encoder's state
	state = setEncoderState(tmp, inputBytes + paddingLen, j, n, k, m, codeType);

	/* Spec. 16e: 8.4.9.2 and Table 317 */
	if (n <= j)
	{
		codedLen = encodeBlock(tmp, output, 1, n, codeType);
	}
	else
	{
		if (m == 0)
		{
			codedLen = encodeBlock(tmp, output, k, j, codeType);
		}
		else
		{
			if (k - 1 != 0)
			{
				codedLen = encodeBlock(tmp, output, k - 1, j, codeType);
			}
			else
			{
				codedLen = 0;
			}
			codedLen += encodeBlock(tmp + codedLen * uncodedSize / codedSize,
					output + codedLen, 1, (m + j + 1) / 2, codeType);
			codedLen += encodeBlock(tmp + codedLen * uncodedSize / codedSize, 
					output + codedLen, 1, (m + j) / 2, codeType);
		}
	}

	return inputSlots * codedSize;
}

int OFDMA_ChannelCoding_MR::encodeBlock(char *input, char *output, int numBlocks, int usedSlot, int codeType)
{
	int uncodedSize     = _coding[codeType].uncodedPayload[usedSlot - 1]; // per block
	int codedSize       = _coding[codeType].codedPayload[usedSlot - 1];   // per block
	char *inp           = input;
	char *outp          = output;
	char buf1[codedSize];
	char buf2[codedSize];

	// Encode each block and copy the coded block to the output buffer
	for (int i = 0;i < numBlocks;i++)
	{
		// Clear buffer
		memset(buf1, 0, codedSize);
		memset(buf2, 0, codedSize);

		// Randomization
		randomize(inp, buf1, uncodedSize * 8);

		// FEC encode
		_coding[codeType].CC->encode(buf1, buf2, uncodedSize * 8);

		// Interleaving
		interleave(buf2, buf1, usedSlot, codeType);

		// Copy to output buffer
		memcpy(outp, buf1, codedSize);

		// Update data offset
		inp += uncodedSize;
		outp += codedSize;
	}

	return numBlocks * codedSize;
}

int OFDMA_ChannelCoding_MR::decode(char *input, char *output, int inputBytes, int inputSlots, int repeatTimes, int codeType)
{
	int uncodedLen  = 0;
	int uncodedSize = _coding[codeType].uncodedPayload[0]; // per slot
	int codedSize   = _coding[codeType].codedPayload[0];   // per slot
	int j           = _concatenation_j[codeType];
	int n           = inputSlots / repeatTimes;
	int k           = n / j;
	int m           = n % j;
	int lastBytes   = 0;
	int totalBytes  = codedSize * inputSlots;
	char *lastBlock = NULL;

	if (_isBypassing)
	{
		// To make the CRC comfortable
		memset(output, 0xFF, uncodedSize * (inputSlots / repeatTimes));
		memcpy(&uncodedLen, input, sizeof(short));
		memcpy(output, input + sizeof(short), uncodedLen);
		return uncodedLen;
	}

	/*
	 * In order to insert the final block to CC decoder (tailbiting),
	 * we need to compute the number of slots of final block,
	 * before we start decoding the original data.
	 */

	/* Spec. 16e: 8.4.9.2 and Table 317 */
	if (n <= j)
	{
		lastBytes = _coding[codeType].codedPayload[n - 1];
		lastBlock = new char [lastBytes];

		memcpy(lastBlock, input, lastBytes);
		uncodedLen = decodeBlock(input, output, lastBlock, n, 0, 1, n, codeType);
	}
	else
	{
		if (m == 0)
		{
			lastBytes = _coding[codeType].codedPayload[j - 1];
			lastBlock = new char [lastBytes];

			memcpy(lastBlock, input + totalBytes - lastBytes, lastBytes);
			uncodedLen = decodeBlock(input, output, lastBlock, j, 0, k, j, codeType);
		}
		else
		{
			int offset = 0;
			int ceilSlots = (m + j + 1) / 2;
			int floorSlots = (m + j) / 2; // numSlots of last block

			lastBytes = _coding[codeType].codedPayload[floorSlots - 1];
			lastBlock = new char [lastBytes];

			memcpy(lastBlock, input  + totalBytes - lastBytes, lastBytes);

			// Start decoding Steps:
			uncodedLen = decodeBlock(input, output, lastBlock, floorSlots,
					ceilSlots, k - 1, j, codeType); // k-1 maybe zero

			if (k - 1 == 0)
			{
				// update input pointer
				offset = (uncodedLen * codedSize) / uncodedSize;
				uncodedLen += decodeBlock(input + offset, output + uncodedLen, lastBlock, floorSlots,
						floorSlots, 1, ceilSlots, codeType);
			}
			else
			{
				// update input pointer
				offset = (uncodedLen * codedSize) / uncodedSize;
				uncodedLen += decodeBlock(input + offset, output + uncodedLen, NULL, 0,
						floorSlots, 1, ceilSlots, codeType);
			}
			// update input pointer
			offset = (uncodedLen * codedSize) / uncodedSize;
			uncodedLen += decodeBlock(input + offset, output + uncodedLen, NULL, 0,
					0, 1, floorSlots, codeType);
		}
	}

	delete [] lastBlock;

	return uncodedLen;
}

int OFDMA_ChannelCoding_MR::decodeBlock(char *input, char *output, char *lastBlock, int lastSlots,
		int lookaheadSlots, int numBlocks, int numSlots, int codeType)
{
	int uncodedSize     = _coding[codeType].uncodedPayload[numSlots - 1];   // per block
	int codedSize       = _coding[codeType].codedPayload[numSlots - 1];     // per block
	int lookahead       = 0;
	char *inp           = input;
	char *outp          = output;
	char buf1[codedSize * 2];
	char buf2[codedSize * 2];

	// no data
	if (numBlocks == 0)
		return 0;

	if (lookaheadSlots != 0)
		lookahead = _coding[codeType].codedPayload[lookaheadSlots - 1];

	// Clear buffer
	memset(buf1, 0, codedSize * 2);
	memset(buf2, 0, codedSize * 2);

	// Set decoder's initial state if it's the first block
	// Tailbite CC Decoding need to input the last bits at first for training.
	if (lastBlock != NULL)
	{
		int lastBlockSize = _coding[codeType].codedPayload[lastSlots - 1];

		// Reset decoder state
		_coding[codeType].CC->resetDecoderState();

		// Deinterleaving
		deinterleave(lastBlock, buf1, lastSlots, codeType);

		// FEC decode
		_coding[codeType].CC->decode(buf1, buf2, lastBlockSize * 8, 0);
	}

	// Decode each block ( + lookahead) and copy the decoded block to the output buffer
	for (int i = 0; i < numBlocks - 1; i++)
	{
		// Deinterleaving
		deinterleave(inp, buf1, numSlots, codeType);
		deinterleave(inp + codedSize, buf1 + codedSize, numSlots, codeType);

		// FEC decode
		_coding[codeType].CC->decode(buf1, buf2, codedSize * 8, codedSize * 8 / 2);

		// Derandomization
		derandomize(buf2, buf1, uncodedSize * 8);

		// Copy to output buffer
		memcpy(outp, buf1, uncodedSize);

		inp += codedSize;
		outp += uncodedSize;
	}

	// Deinterleaving
	deinterleave(inp, buf1, numSlots, codeType);

	if (lookahead != 0)
	{
		deinterleave(inp + codedSize, buf1 + codedSize, lookaheadSlots, codeType);
	}

	// FEC decode
	_coding[codeType].CC->decode(buf1, buf2, codedSize * 8, lookahead * 8 / 2);

	// Derandomization
	derandomize(buf2, buf1, uncodedSize * 8);

	// Copy to output buffer
	memcpy(outp, buf1, uncodedSize);

	return numBlocks * uncodedSize;
}

// Initialize channel coding
void OFDMA_ChannelCoding_MR::initCoding(int uncodedSize, int codedSize, OFDMA_ConvoCode_MR * CC, int Ncpc, int codeType)
{
	/*
	 * Spec. 16e: 8.4.9.2.1 and Table 320
	 * Uncoded(Coded) Bytes
	 *
	 * j = 6: QPSK_1_2 :  6(12), 12(24), 18(36), 24(48), 30(60), 36(72)
	 * j = 4: QPSK_3_4 :  9(12), 18(24), 27(36), 36(48)
	 * j = 3: QAM16_1_2: 12(24), 24(48), 36(72)
	 * j = 2: QAM16_3_4: 18(24), 36(48)
	 * j = 2: QAM64_1_2: 18(36), 36(72)
	 * j = 1: QAM64_2_3: 24(36)
	 * j = 1: QAM64_3_4: 27(36)
	 */
	for (int i = 0;i < _concatenation_j[codeType];i++)
	{
		_coding[codeType].uncodedPayload[i]   = (i + 1) * uncodedSize;
		_coding[codeType].codedPayload[i]     = (i + 1) * codedSize;
		_coding[codeType].Ncbps[i]            = (i + 1) * codedSize * 8;
	}

	_coding[codeType].CC            = CC;
	_coding[codeType].Ncpc          = Ncpc;
}

// Uncoded Bytes ==> Number of Slots with Repetition coding
int OFDMA_ChannelCoding_MR::getNumUncodedSlotWithRepe(int numBytes, int repeatTimes, int codeType)
{
	int numSlotsNoRepe = numBytes / _coding[codeType].uncodedPayload[0];

	if ((numBytes % _coding[codeType].uncodedPayload[0]) != 0)
		numSlotsNoRepe++;

	return numSlotsNoRepe * repeatTimes;
}

// Coded Bytes ==> Number of Slots with Repetition coding
int OFDMA_ChannelCoding_MR::getNumCodedSlotWithRepe(int numBytes, int repeatTimes, int codeType)
{
	int numSlotsNoRepe = numBytes / _coding[codeType].codedPayload[0];

	if ((numBytes % _coding[codeType].codedPayload[0]) != 0)
		numSlotsNoRepe++;

	return numSlotsNoRepe * repeatTimes;
}

// Coded Slot Bytes
int OFDMA_ChannelCoding_MR::getCodedSlotLen(int codeType)
{
	return _coding[codeType].codedPayload[0];
}

// Uncoded Slot Bytes
int OFDMA_ChannelCoding_MR::getUncodedSlotLen(int codeType)
{
	return _coding[codeType].uncodedPayload[0];
}

// Uncoded Bytes ==> Coded Burst Length after apply repetition coding
int OFDMA_ChannelCoding_MR::getCodedBurstLen(int numBytes, int repeatTimes, int codeType)
{
	int numSlotsWithRepe = getNumUncodedSlotWithRepe(numBytes, repeatTimes, codeType);

	return numSlotsWithRepe * _coding[codeType].codedPayload[0];
}

// Coded Bytes ==> Uncoded Burst Length
int OFDMA_ChannelCoding_MR::getUncodedBurstLen(int numBytes, int codeType)
{
	int numSlots = numBytes / _coding[codeType].codedPayload[0];

	if (numBytes % _coding[codeType].codedPayload[0] != 0)
	{
		fprintf(stderr, "%s: Error Status: %d is not multiple of %d\n",
				__func__, numBytes, _coding[codeType].codedPayload[0]);
		exit(1);
	}

	return numSlots * _coding[codeType].uncodedPayload[0];
}

// Perform on each burst of data on the DL and UL.
void OFDMA_ChannelCoding_MR::randomize(char *input, char *output, int inputBits)
{
	/* Spec. 16e: 8.4.9.1 */
	int seed    = 0x3715;
	int in      = 0;
	int out     = 0;
	int tmp     = 0;

	memset(output, 0, inputBits / 8);

	for (int i = 0; i < inputBits; i++) 
	{
		// Extract 1-bit input
		in      = ((input[i / 8] << (i % 8)) & 0x80) >> 7;

		tmp     = (seed & 0x1) ^ ((seed & 0x2) >> 1);
		out     = in ^ tmp;
		seed    = (seed >> 1) | (tmp << 14);

		output[i / 8] <<= 1;
		output[i / 8] |= out;
	}
}

void OFDMA_ChannelCoding_MR::derandomize(char *input, char *output, int inputBits)
{
	randomize(input, output, inputBits);
}

void OFDMA_ChannelCoding_MR::interleave(char *input, char *output, int numSlots, int codeType)
{
	int Ncbps   = _coding[codeType].Ncbps[numSlots - 1];
	int Ncpc    = _coding[codeType].Ncpc;
	int s       = Ncpc / 2;
	int in      = 0;
	int j       = 0;
	int m       = 0;
	int d       = 16;

	memset(output, 0, Ncbps / 8);

	for (int k = 0;k < Ncbps;k++) 
	{
		// Extract 1-bit input
		in  = ((input[k / 8] << (k % 8)) & 0x80) >> 7;

		m   = (Ncbps / d) * (k % d) + k / d;
		j   = s * (m / s) + (m + Ncbps - (d * m / Ncbps)) % s;

		output[j / 8] |= (in << (7 - j % 8));
	}
}

void OFDMA_ChannelCoding_MR::deinterleave(char *input, char *output, int numSlots, int codeType)
{
	int Ncbps   = _coding[codeType].Ncbps[numSlots - 1];
	int Ncpc    = _coding[codeType].Ncpc;
	int s       = Ncpc / 2;
	int in      = 0;
	int k       = 0;
	int m       = 0;
	int d       = 16;

	memset(output, 0, Ncbps / 8);

	for (int j = 0;j < Ncbps;j++)
	{
		// Extract 1-bit input
		in  = ((input[j / 8] << (j % 8)) & 0x80) >> 7;

		m   = s * (j / s) + (j + (d * j / Ncbps)) % s;
		k   = d * m - (Ncbps - 1) * (d * m / Ncbps);

		output[k / 8] |= (in << (7 - k % 8));
	}
}

void OFDMA_ChannelCoding_MR::dumpByteString(char *prompt, char *byteString, int len)
{
	printf("%s:\t", prompt);
	for (int i = 0; i < len; i++) 
	{
		printf("%02X ", byteString[i] & 0xFF);
	}
	printf("\n");
}

void OFDMA_ChannelCoding_MR::dumpByteByBits(char b)
{
	for (int i = 0;i < 8;i++)
	{
		printf("%X", ((b << i) & 0x80) >> 7);
	}
	printf("\n");
}
