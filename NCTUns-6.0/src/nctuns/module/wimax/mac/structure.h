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

#ifndef __NCTUNS_structure_h__
#define __NCTUNS_structure_h__

#include <sys/types.h>

/*
 *	Spec 6.3.2
 */
#define GHDR_GET_LEN(msg, len) do {				\
	struct hdr_generic *_hg = (struct hdr_generic *) msg;	\
	len = _hg->len_msb * 256 + _hg->len_lsb;		\
} while (0)

#define GHDR_GET_CID(msg, cid) do {				\
	struct hdr_generic *_hg = (struct hdr_generic *) msg;	\
	cid = _hg->cid_msb * 256 + _hg->cid_lsb;		\
} while (0)

enum {
	tyMesh = 1 << 5,
	tyARQ = 1 << 4,
	tyExtend = 1 << 3,
	tyFragment = 1 << 2,
	tyPacking = 1 << 1,
	tyFEEDBACK = 1,
	tyGrant = 1
};

struct hdr_generic {
	u_char ht:1;
	u_char ec:1;
	u_char type:6;
	u_char rsv1:1;
	u_char ci:1;
	u_char eks:2;
	u_char rsv2:1;
	u_char len_msb:3;
	u_char len_lsb:8;
	u_char cid_msb:8;
	u_char cid_lsb:8;
	u_char hcs:8;
};

struct hdr_bwreq {
	u_char ht:1;
	u_char ec:1;
	u_char type:3;
	u_char br_msb1:3;
	u_char br_msb2:8;
	u_char br_lsb:8;
	u_char cid_msb:8;
	u_char cid_lsb:8;
	u_char hcs:8;
};

struct subhdr_fragment {
	u_char fc:2;
	u_char fsn:3;
	u_char rsv:3;
};

struct subhdr_grant_mgmt {
	struct {
		u_char si:1;
		u_char pm:1;
		u_short rsv:14;
	} ugs;
	u_short piggyback:16;
};

struct subhdr_packing {
	u_char fc:2;
	u_char fsn:3;
	u_short len:11;
};

#define	MG_UCD		0
#define	MG_DCD		1
#define	MG_DLMAP	2
#define	MG_ULMAP	3
#define	MG_RNGREQ	4
#define	MG_RNGRSP	5
#define	MG_REGREQ	6
#define	MG_REGRSP	7

#define	MG_PKMREQ	9
#define	MG_PKMRSP	10
#define	MG_DSAREQ	11
#define	MG_DSARSP	12
#define	MG_DSAACK	13
#define	MG_DSCREQ	14
#define	MG_DSCRSP	15
#define	MG_DSCACK	16
#define	MG_DSDREQ	17
#define	MG_DSDRSP	18

#define	MG_MCAREQ	21
#define	MG_MCARSP	22
#define	MG_DBPCREQ	23
#define	MG_DBPCRSP	24
#define	MG_RESCMD	25
#define	MG_SBCREQ	26
#define	MG_SBCRSP	27
#define	MG_CLKCMP	28
#define	MG_DREGCMD	29
#define	MG_DSXRVD	30
#define	MG_TFTPCPLT	31
#define	MG_TFTPRSP	32
#define	MG_ARQFeedback	33
#define	MG_ARQDiscard	34
#define	MG_ARQReset	35
#define	MG_REPREQ	36
#define	MG_REPRSP	37
#define	MG_FPC		38
#define	MG_MSHNCFG	39
#define	MG_MSHNENT	40
#define	MG_MSHDSCH	41
#define	MG_MSHCSCH	42
#define	MG_MSHCSCF	43
#define	MG_AASFBCKREQ	44
#define	MG_AASFBCKRSP	45
#define	MG_AAS_Beam_Select	46
#define	MG_AAS_BEAM_REQ	47
#define	MG_AAS_BEAM_RSP	48
#define	MG_DREGREQ	49

#define	MG_COMPOUND	255

struct mgmt_msg {
	u_char type;
	u_char msg[256];
};

struct OFDM_DLMAP_IE {
	u_short cid:16;
	u_char diuc:4;
	u_char preamble:1;
	u_short stime:11;
};

struct OFDM_ULMAP_IE {
	u_short cid:16;
	u_short stime:11;
	u_char chidx:5;
	u_char uiuc:4;
	u_short duration:10;
	u_char midamble:2;
};

struct OFDM_BurstProfile	// Both Downlink Burst Profile and Uplink Burst Profile
{
	u_char type;
	u_char Length;
	u_char resv:4;
	u_char usagecode:4;
};

struct DCDBurstProfile {
	char used;
	char frequency;		// Downlink frequency (kHz)
	char fec;		// FEC code type
	char exitThreshold;	// DIUC mandatory exit threshold
	char entryThreshold;	// DIUC minimun entry threshold
	char tcs;		// TCS_enable
};

struct UCDBurstProfile {
	char used;
	char fec;		// FEC code type
	char powerBoost;	// Focused contention power boost
	char tcs;		// TCS_enable
};

struct PHYInfo {
	//double        power;  // Pt between PHY peers or SNR between MAC and PHY
	double SNR;		// Used between PHY and MAC
	double power;		// Used between PHY peers
	int nsymbols;		// # of symbols including pre/mid/post ambles
	int fec;		// FEC type
	int ChannelID;
	u_int64_t frameStartTime;	// In ticks
	struct OFDM_ULMAP_IE ie;
};


#endif  /* __NCTUNS_structure_h__ */
