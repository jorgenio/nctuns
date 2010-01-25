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

#ifndef __NCTUNS_80216J_NT_STRUCTURE_H__
#define __NCTUNS_80216J_NT_STRUCTURE_H__

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
#define MG_TEST             67
#define MG_RMAP             68
#define MG_RCD	 	        70
#define MG_MR_NBRINFO       71
#define MG_MR_RNGREP        72
#define MG_CID_ALLOCREQ	    73
#define MG_RS_ConfigCMD     74
#define MG_RS_NBR_MEASREP   75
#define MG_MR_LOCREQ        76
#define MG_MR_LOCRSP        77
#define MG_MS_SCNINF        78
#define MG_MS_SCNCLT        79
#define MG_MS_INFODEL       80
#define MG_CLKSYNC          81
#define MG_MR_ASCREQ        82
#define MG_MR_ASCRSP        83
#define MG_RS_MOB_MEASREQ   84
#define MG_HARQ_Chase_ERREP 85
#define MG_HARQ_IR_ERREP    86
#define MG_MR_SLPINFO       87
#define MG_RS_AccessRSREQ   88
#define MG_RSSCH            89
#define MG_RS_Member_List_Update 90
#define MG_MR_PBBRINFO      91
#define MG_MR_GenericACK    92
#define MG_RS_AccessMAP     93
#define MG_RS_RelayMAP      94
#define MG_MOB_INFIND       95
#define MG_MS_ContextREQ    96
#define MG_MS_ContextRSP    97
#define MG_MT_Transfer      98
	

#define MG_COMPOUND         255

using namespace MR_OFDMAMapIE_NT;

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

/* 
 * Signaling Header
 */
enum SignalingHeaderType{
    type1,
    type2_0,
    type2_1
};

enum SignalingHeaderType2_ext{
    RS_BR_hdr,
    RS_UL_DCH_signaling_hdr,
    MR_Acknowledgment_hdr,
    MR_HARQ_error_report_hdr,
    MR_CodeREP_hdr,
    Reserved,
    Tunnel_BR_hdr,
    DL_Flow_Control_hdr
};

struct BR_header_type1{
    uint8_t type;   // 3 bits
    const void *hdr;
}; 

struct BR_header_type2_0 {
    // feedback header ; spec 16e 6.3.2.1.2.2.1
    const void *hdr;
};

struct BR_header_type2_1 {
    uint8_t type_ext;       // spec 16j Table 18c
    const void *hdr;              
};

typedef union {
    struct BR_header_type1      br_hdr_1;
    struct BR_header_type2_0    br_type2_0;
    struct BR_header_type2_1    br_type2_1;
}BR_header_u;

typedef struct BR_header{
    uint8_t type;   // type1 or type2_0 or type2_1
    BR_header_u br_hdr;
}BR_HDR;

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

// 16e 6.3.2.1.2.2 MAC signaling header type II , Figure 20g
struct hdr_type2 {
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:1;
	uint16_t hc_msb:13;
	uint16_t hc:16;
	uint8_t hc_lsb:8;
	uint8_t hcs:8;
};

// 16j/D7 6.3.2.1.2.2.2 Extended MAC signaling header type II , Figure 34a
struct hdr_type2_ext {
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:1;
    uint8_t type_ext:3; // spec 16j/D7 Table 18a.
	uint16_t hc_msb:10;
	uint16_t hc:16;
	uint8_t hc_lsb:8;
	uint8_t hcs:8;
};

// spec 16j Table 34b
struct RS_BR_header{
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:1;
    uint8_t type_ext:3; //  type = 0 
	uint8_t tid_msb:2;
	
    uint8_t tid_lsb:2;
    uint8_t diuc:4;
    uint8_t br_msb:2;
    
    uint8_t br_lsb:8;
    uint8_t cid_msb:8;
    uint8_t cid_lsb:8;
	uint8_t hcs:8;
};

// spec 16j Table 34f
struct MR_CodeREP_header{
	uint8_t ht:1;
	uint8_t ec:1;
	uint8_t type:1;
    uint8_t type_ext:3; //  type = 0 
	uint8_t fnum_idx_msb:2;
	
    uint8_t fnum_idx_lsb:2;
    uint8_t num_ir:4;       // number of received IR codes
    uint8_t num_hr_msb:2;   // number of received HR codes
    
    uint8_t num_hr_lsb:2;
    uint8_t num_br:6;
    
    uint8_t cid_msb:8;
    uint8_t cid_lsb:8;
	uint8_t hcs:8;
}; 

// spec 16j/D7 6.3.2.1.1.1 Relay MAC header format
struct hdr_relay {
	uint8_t ht:1;
    uint8_t ec:1;
	uint8_t rmi:1;
	uint8_t ash:1;
	uint8_t gmsh:1;
	uint8_t fsh:1;
	uint8_t psh:1;
	uint8_t qsh:1;
	uint8_t esf:1;
	uint8_t ci:1;
	uint8_t eks:2;
	uint8_t len_msb:4;
	uint8_t len_lsb:8;
	uint8_t cid_msb:8;
	uint8_t cid_lsb:8;
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
 * 	Spec 16j.D7.6.3.2.2.8 
 */
struct relay_subhdr_qos {
	uint8_t data_deliverservice:3;
	uint8_t pri:3;
	uint8_t rsv:2;
};

struct relay_subhdr_alloc {
	uint16_t target_trans_fra:6;
	uint16_t alloc:6;
	uint16_t no_mac_pdus:3;
	uint16_t conti:1;
};

struct relay_subhdr_fragment {
	uint8_t more_frag:1;
	uint8_t tsn:7;
	uint16_t fo:12;
	uint16_t rsv:4;
};

struct relay_subhdr_packing {
	uint32_t mf:1;
	uint32_t tsn:7;
	uint32_t fo:12;
	uint32_t frag_len:12;
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

	
	/* 16j new added */
	uint32_t nid;		    // purpose to msobject and rsobject to find out corresponding MS/RS module
	uint32_t pid;           // port id
    int  burst_type;           
    bool RS_RNGREQ_flag;    // false:RNGREQ is sent by MS / true:RNGREQ is sent by RS
    bool relay_flag; 	    // true:transmit via RS ; false:transmition direct with BS  
	int ULAccessSym;	    // Record UL access symbols
	int ULRelaySym;		    // Record UL relay symbols
	int DLAccessSym;	    // Record DL access symbols
	int DLRelaySym;	        // Record DL transparent symbols
};

struct rngcode {
    uint8_t frameIndex;
    uint8_t rangCode;
    uint8_t rangSym;
    uint8_t rangCh;
};


// Spec 6.3.2.1.1
inline int GHDR_GET_LEN(struct hdr_generic *msg)
{   // Length. The length in bytes of the MAC PDU including the MAC header and the CRC if present.
	return msg->len_msb * 256 + msg->len_lsb;
}

inline int GHDR_GET_CID(struct hdr_generic *msg)
{
	return msg->cid_msb * 256 + msg->cid_lsb;
}

#endif 
