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

#ifndef __NCTUNS_80216E_STRUCTURE_H__
#define __NCTUNS_80216E_STRUCTURE_H__

#include <sys/types.h>
#include "ofdma_map_ie.h"

/*
 * Spec 16e. Table 14. MAC Management message's Type
 */
#define MG_UCD              0
#define MG_DCD              1
#define MG_DLMAP            2
#define MG_ULMAP            3
#define MG_RNGREQ           4
#define MG_RNGRSP           5
#define MG_REGREQ           6
#define MG_REGRSP           7

#define MG_PKMREQ           9
#define MG_PKMRSP           10
#define MG_DSAREQ           11
#define MG_DSARSP           12
#define MG_DSAACK           13
#define MG_DSCREQ           14
#define MG_DSCRSP           15
#define MG_DSCACK           16
#define MG_DSDREQ           17
#define MG_DSDRSP           18

#define MG_MCAREQ           21
#define MG_MCARSP           22
#define MG_DBPCREQ          23
#define MG_DBPCRSP          24
#define MG_RESCMD           25
#define MG_SBCREQ           26
#define MG_SBCRSP           27
#define MG_CLKCMP           28
#define MG_DREGCMD          29
#define MG_DSXRVD           30
#define MG_TFTPCPLT         31
#define MG_TFTPRSP          32
#define MG_ARQFeedback      33
#define MG_ARQDiscard       34
#define MG_ARQReset         35
#define MG_REPREQ           36
#define MG_REPRSP           37
#define MG_FPC              38
#define MG_MSHNCFG          39
#define MG_MSHNENT          40
#define MG_MSHDSCH          41
#define MG_MSHCSCH          42
#define MG_MSHCSCF          43
#define MG_AASFBCKREQ       44
#define MG_AASFBCKRSP       45
#define MG_AAS_Beam_Select  46
#define MG_AAS_BEAM_REQ     47
#define MG_AAS_BEAM_RSP     48
#define MG_DREGREQ          49
#define MG_SLP_REQ          50
#define MG_MOB_SLPRSP       51
#define MG_MOB_TRFIND       52
#define MG_MOB_NBRADV       53
#define MG_MOB_SCNREQ       54
#define MG_MOB_SCNRSP       55
#define MG_MOB_BSHOREQ      56
#define MG_MOB_MSHOREQ      57
#define MG_MOB_BSHORSP      58
#define MG_MOB_HOIND        59
#define MG_MOB_SCNREP       60
#define MG_MOB_PAGADV       61
#define MG_MBS_MAP          62
#define MG_PMC_REQ          63
#define MG_PMC_RSP          64
#define MG_PRC_LTCTRL       65
#define MG_MOB_ASCREP       66

#define MG_COMPOUND         255

using namespace mobileOFDMAMapIE;

/*
 * Spec 16e. Table 6.
 */
enum {
	tyMesh      = 1 << 5,
	tyARQ       = 1 << 4,
	tyExtend    = 1 << 3,
	tyFragment  = 1 << 2,
	tyPacking   = 1 << 1,
	tyFEEDBACK  = 1,     // Downlink
	tyGrant     = 1      // Uplink
};

/*
 * Spec 16e. Figure 19.
 */
struct hdr_generic {
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:6;
	uint8_t esf:1;
	uint8_t ci:1;
	uint8_t eks:2;
	uint8_t rsv:1;
	uint8_t len_msb:3;
	uint8_t len_lsb:8;
	uint8_t cid_msb:8;
	uint8_t cid_lsb:8;
	uint8_t hcs:8;
};

struct hdr_type1 {
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:3;
	uint16_t hc_msb:11;
	uint8_t hc_lsb:8;
	uint8_t cid_msb:8;
	uint8_t cid_lsb:8;
	uint8_t hcs:8;
};

struct hdr_type2 {
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:1;
	uint16_t hc_msb:13;
	uint16_t hc:16;
	uint8_t hc_lsb:8;
	uint8_t hcs:8;
};

struct subhdr_fragment {
	uint8_t fc:2;
	uint8_t fsn:3;
	uint8_t rsv:3;
};

struct subhdr_grant_mgmt {
	struct {
		uint8_t si:1;
		uint8_t pm:1;
		uint8_t fli:1;
		uint8_t fl:4;
		uint16_t rsv:9;
	} ugs;

	struct {
		uint16_t ext_piggyback:11;
		uint8_t fli:1;
		uint8_t fl:4;
	} ertps;

	uint16_t piggyback:16;
};

struct subhdr_packing {
	uint8_t fc:2;
	uint8_t fsn:3;
	uint16_t len:11;
};

/*
 * Spec 16e. Figure 21.
 */
struct mgmt_msg {
	uint8_t type;
	uint8_t msg[256];
};

/*
 * Spec 16e. Table 304a. 304b.
 * Both Downlink Burst Profile and Uplink Burst Profile
 */
struct OFDMA_BurstProfile {
	uint8_t type:8;
	uint8_t length:8;
	uint8_t rsv:2;
	uint8_t coding_type:2;
	uint8_t usagecode:4; // diuc, uiuc
};

/*
 * Spec 16e. Table 363.
 */
struct DCDBurstProfile {
	char used;
	char fec;
};

/*
 * Spec 16e. Table 357.
 */
struct UCDBurstProfile {
	char used;
	char fec;
	char rangRatio;
};

/*
 * PHY Information
 */
struct PHYInfo {
	double SNR;                 // Used between PHY and MAC
	double power;               // Used between PHY peers
	int fec;                    // FEC type
	int uiuc;                   // UIUC
	int ChannelID;              // Channel ID
	int chOffset;               // Subchannel offset
	int symOffset;              // Symbol offset
	uint64_t frameStartTime;    // In ticks
	ULMAP_IE_u ulmap_ie;        // UL-MAP IE
};

// Spec 6.3.2.1.1
inline int GHDR_GET_LEN(struct hdr_generic *msg)
{
	return msg->len_msb * 256 + msg->len_lsb;
}

inline int GHDR_GET_CID(struct hdr_generic *msg)
{
	return msg->cid_msb * 256 + msg->cid_lsb;
}

#endif 
