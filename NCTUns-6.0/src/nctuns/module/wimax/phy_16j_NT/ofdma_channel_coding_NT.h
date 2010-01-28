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

#ifndef __NCTUNS_80216E_OFDMA_CHANNEL_CODING_H__
#define __NCTUNS_80216E_OFDMA_CHANNEL_CODING_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "ofdma_80216j_NT.h"
#include "fec/cc/ofdma_convo_code_NT.h"

class OFDMA_ChannelCoding_NT {
	private:
		bool    _isBypassing;
		int     _concatenation_j[7];

		struct {
			int             uncodedPayload[6];
			int             codedPayload[6];
			int             preDecodeBit;
			int             addDecodeBit;
			OFDMA_ConvoCode_NT *CC;
			int             Ncbps[6];
			int             Ncpc;
		} _coding[7];

		/*
		 * Convolutional code (1/2, 2/3, 3/4)
		 *
		 * Note that each encoder/decoder lookup table is only generated
		 * once for all class instances to save memory space.
		 */
		OFDMA_ConvoCode_NT   *_CC_1_2;
		OFDMA_ConvoCode_NT   *_CC_2_3;
		OFDMA_ConvoCode_NT   *_CC_3_4;

		static bool _CC_EncoderTableInitiated;
		static bool _CC_EncoderTableDeleted;
		static bool _CC_DecoderTableInitiated;
		static bool _CC_DecoderTableDeleted;

		static struct EncoderLookupEntry *_CC_1_2_EncoderTable;
		static struct EncoderLookupEntry *_CC_2_3_EncoderTable;
		static struct EncoderLookupEntry *_CC_3_4_EncoderTable;
		static struct DecoderLookupEntry *_CC_1_2_DecoderTable;
		static struct DecoderLookupEntry *_CC_2_3_DecoderTable;
		static struct DecoderLookupEntry *_CC_3_4_DecoderTable;

		void    initCoding      (int, int, OFDMA_ConvoCode_NT *, int, int);
		void    randomize       (char *, char *, int);
		void    derandomize     (char *, char *, int);
		void    interleave      (char *, char *, int, int);
		void    deinterleave    (char *, char *, int, int);
		int     setEncoderState (char *, int, int, int, int, int, int);
		int     encodeBlock     (char *, char *, int, int, int);
		int     decodeBlock     (char *, char *, char *, int, int, int, int, int);

	public:
		OFDMA_ChannelCoding_NT        (bool);
		~OFDMA_ChannelCoding_NT       ();

		int     encode                      (char *, char *, int, int, int, int);
		int     decode                      (char *, char *, int, int, int, int);
		int     getNumUncodedSlotWithRepe   (int, int, int);
		int     getNumCodedSlotWithRepe     (int, int, int);
		int     getCodedSlotLen             (int);
		int     getUncodedSlotLen           (int);
		int     getCodedBurstLen            (int, int, int);
		int     getUncodedBurstLen          (int, int);
		void    dumpByteString              (char *, char *, int);
		void    dumpByteByBits              (char);
};

#endif                /* __NCTUNS_80216E_OFDMA_CHANNEL_CODING_H__ */
