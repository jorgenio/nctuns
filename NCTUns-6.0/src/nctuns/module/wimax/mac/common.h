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

#ifndef __NCTUNS_WIMAX_COMMON_H__
#define __NCTUNS_WIMAX_COMMON_H__

#include <stdio.h>
#include <vector>
#include "structure.h"
#include "burst.h"
#include <packet.h>

class Pkthdr {
      public:
	struct p_hdr p_hdr;
	struct s_exthdr exthdr;
	short flag;		/*  PF_SEND , PF_RECV */
	short type;		// Command Type;
};

class dlmapinfo {
      public:
	short start;
	short end;
	char usage;
	int byteOffset;
};

class ntfyDLMAP:public Pkthdr {
      public:
	std::vector < dlmapinfo > info;
	ntfyDLMAP() {
		type = 2;
}};

class BurstInfo:public Pkthdr {
      public:
	std::vector < WiMaxBurst * >*Collection;

	BurstInfo() {
		type = 1;
		Collection = NULL;
	} ~BurstInfo() {
		if (Collection) {
			while (!Collection->empty()) {
				delete *(Collection->begin());
				Collection->erase(Collection->begin());
			}
			delete Collection;
		}
	}
	int appendBurst(int diuc);	// May move GetCorrespondBurst() to here?
};

#define CTLMSG_TYPE_MSH_NCFG 39
#define CTLMSG_TYPE_MSH_NENT 40
#define CTLMSG_TYPE_MSH_DSCH 41
#define CTLMSG_TYPE_MSH_CSCH 42
#define CTLMSG_TYPE_MSH_CSCF 43

typedef enum Mac80216BurstType {

	UNDEFINED = 0,
	BT_DATA = 240,
	BT_NETWORK_CFG = 39,
	BT_NETWORK_ENTRY = 40,

	BT_DSCHED = 41,
	BT_CSCHED = 42,
	BT_CSCF = 43,
	BT_SBRREG = 241,
	BT_REQREG = 242,
	BT_SPONSOR = 243
} mac80216_burst_t;

typedef struct LogInfo {

	u_int32_t src_nid;
	u_int32_t dst_nid;
	mac80216_burst_t burst_type;
	int nsymbols;
	int burst_len;
	int connection_id;
	int channel_id;


} loginfo_t;

#endif				/* __NCTUNS_WIMAX_COMMON_H__    */
