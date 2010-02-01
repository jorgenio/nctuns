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

#ifndef __NCTUNS_80216J_MANAGEMENT_MSG_H__
#define __NCTUNS_80216J_MANAGEMENT_MSG_H__

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "structure.h"

/*
 * All management message should inherit from this class
 */
namespace mobileRelayManageMsg {
	class ManagementMessage {
		protected:
			uint8_t Type;
			uint8_t flag;

		public:
			explicit ManagementMessage(uint8_t);    // Create a new management message
			explicit ManagementMessage(uint8_t *);  // Read management from pBuffer with at most pLength
			virtual ~ManagementMessage();

			inline int getType() const { return Type; }
			virtual int copyTo(uint8_t *) const = 0;
			virtual int getLen() const = 0;
	};

	/* A class as an Interface help to process TLV specific encoding.
	 * Here we just handle length less than 256 that can be recorded
	 * in one byte.
	 */
	class ifTLV {
		private:
			uint8_t *Buffer;
			uint8_t *ptr;
			uint8_t *tlv_value;
			uint8_t flag;
			int     len;
			int     tlv_len;

		public:
			ifTLV();
			ifTLV(uint8_t *, int);
			~ifTLV();

			void Dump();
			int Add(int, int, unsigned int);
			int Add(int, int, void *);
			int getLen() const { return len; }
			int copyTo(uint8_t*) const;
			const uint8_t *getBuffer();
			int getNextType();        // Get TLV Type
			int getNextLen();         // Get TLV Length
			int getNextValue(void *); // Get TLV Value
	};

	class ifmgmt:public ManagementMessage {
		public:
			uint8_t *Field;
			uint8_t *ptr;
			ifTLV   *Tlv;
			int     FieldSize;
			int     _offset;
			
			bool relay_flag;

			int setField(int pStart, int pLen, int pValue);
			int setField(int pStart, int pLen, void *);

		public:
			ifmgmt(int, int);
			ifmgmt(uint8_t *, int, int);
			ifmgmt(uint8_t *, int);	// 16j , relay buffer purpose
			~ifmgmt();

			void Dump();
			inline int getFLen() { return FieldSize; }
			inline int getLen() const { return 1 + FieldSize + Tlv->getLen(); } // Return Total Length

			void appendField(int, int);
			void appendField(int, void *);
			void appendBitField(int, uint32_t);
			void appendBitField(int, uint8_t *);
			int appendTLV(int, int, unsigned int);
			int appendTLV(int, int, void *);
			int copyTo(uint8_t* dstbuf) const;

			int getField(int, int, void *);
			void extractBitField(int, uint8_t *);
			int getNextType();        // Get TLV Type
			int getNextLen();         // Get TLV Length
			int getNextValue(void *); // Get TLV Value
	};
}

#endif                /*   __NCTUNS_80216J_MANAGEMENT_MSG_H__  */
