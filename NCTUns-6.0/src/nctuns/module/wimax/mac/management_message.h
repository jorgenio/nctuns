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

#ifndef __NCTUNS_ManagementMessage_h__
#define __NCTUNS_ManagementMessage_h__

#include <sys/types.h>

#define CTLMSG_TYPE_MSH_NCFG 39
#define CTLMSG_TYPE_MSH_NENT 40
#define CTLMSG_TYPE_MSH_DSCH 41
#define CTLMSG_TYPE_MSH_CSCH 42
#define CTLMSG_TYPE_MSH_CSCF 43

/*
 * All management message should inherit from this class
 */
class ManagementMessage {

protected:
	u_char Type;
	u_char flag;

public:
	explicit ManagementMessage(u_char);	// Create a new management message
	explicit ManagementMessage(u_char*);	// Read management from pBuffer with at most pLength
	virtual ~ManagementMessage();

	inline int getType() const { return Type; }
	virtual int copyTo(u_char*) const = 0;
	virtual int getLen() const = 0;
};

/* A class as an Interface help to process TLV specific encoding.
 * Here we just handle length less than 256 that can be recorded
 * in one byte.
 */
class ifTLV {
	int Len;
	u_char *Buffer, *ptr, flag;
	int tlv_len;
	u_char *tlv_value;

public:
	 ifTLV();
	 ifTLV(u_char *, int);
	~ifTLV();

	void Dump();
	int Add(int, int, unsigned int);
	int Add(int, int, void *);
	int getLen() const { return Len; }
    int copyTo(u_char*) const;
	const u_char *getBuffer();
	int getNextType();	// Get TLV Type
	int getNextLen();	// Get TLV Length
	int getNextValue(void *);	// Get TLV Value
};

class ifmgmt:public ManagementMessage {
      private:
	u_char * Field, *ptr, FieldSize;
	ifTLV *Tlv;
	int tlv, pos;
	u_char *tlv_value;

	int setField(int pStart, int pLen, int pValue);
	int setField(int pStart, int pLen, void *);

      public:
	 ifmgmt(int, int);
	 ifmgmt(u_char *, int, int);
	void Dump();
	~ifmgmt();

	inline int getFLen() { return FieldSize; }
    inline int getLen() const { return 1 + FieldSize + Tlv->getLen(); }		// Return Total Length

	int appendField(int pLen, int pValue);
	int appendField(int pLen, void *);
	int appendTLV(int, int, unsigned int);
	int appendTLV(int, int, void *);
	int copyTo(u_char* dstbuf) const;

	int getField(int, int, void *);
	int getNextType();	// Get TLV Type
	int getNextLen();	// Get TLV Length
	int getNextValue(void *);	// Get TLV Value
};


#endif				/*      __NCTUNS_ManagementMessage_h__  */
