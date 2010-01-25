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

#ifndef __NCTUNS_80216J_NT_OFDMA_MAP_IE_H__
#define __NCTUNS_80216J_NT_OFDMA_MAP_IE_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/* Except for DIUC 14 or 15 */  // (44 + numCid * 16) bits
struct OFDMA_DLMAP_IE_other {
	uint8_t diuc;
	uint8_t numCid;
	uint16_t *cid;
	uint8_t symOff;
	uint8_t chOff;
	uint8_t boosting;
	uint8_t numSym;
	uint8_t numCh;
	uint8_t repeCode;
};

/* PAPR Reduction/Safety Zone/Sounding Zone alloc IE , 36 bits , 16j table 375*/
struct OFDMA_DLMAP_IE_13 {
	uint8_t diuc;           // append 4 bits
	uint8_t symOff;         // append 8 bits
	uint8_t chOff;          // append 7 bits
	uint8_t numSym;         // append 7 bits
	uint8_t numCh;          // append 7 bits
	uint8_t papr_or_safety; // append 1 bits
	uint8_t sounding;       // append 1 bits
	uint8_t relay_zone_ind; // append 1 bits -- 1:relay zone indicator
};

/* Extended-2 DIUC IE */    // (16 + length * 8) bits
struct OFDMA_DLMAP_IE_14 {
	uint8_t diuc;		// append 4 bits
	uint8_t ext2_diuc;	// append 4 bits
	uint8_t length;		// append 8 bits
	void *data;
};

/* Extended DIUC IE */      // (12 + length * 8) bits
struct OFDMA_DLMAP_IE_15 {
	uint8_t diuc;		// append 4 bits
	uint8_t ext_diuc;	// append 4 bits
	uint8_t length;		// append 4 bits
	void *data;
};

/* STC/DL Zone IE extended ext-DIUC = 01 , 46 bits*/
struct STC_DL_Zone_Switch_IE {
	uint8_t symOff:8;
	uint16_t permut:2;
	uint16_t use_subchan:1;
	uint16_t stc:2;
	uint16_t matric_ind:2;
	uint16_t dl_permb:5;
	uint16_t amc_type:2;
	uint16_t midamb_pres:1;
	uint16_t midamb_boost:1;
	uint8_t antenna_selc;	// 1 bit
	uint8_t dedi_pilot;	// 1 bit
	uint8_t trasparent_txpwr;	// 4 bits
};
/* WiMAX 16j DL-MAP burst-based IE */
struct DL_Burst_Transmit_IE {
	uint16_t cid;
	uint8_t Nr;
	uint16_t relay_burst_len;
};
	

/* Except for UIUC 0, 11~15 */  // 32 bits
struct OFDMA_ULMAP_IE_other {
	uint16_t cid;
	uint8_t uiuc;
	uint16_t duration; // in slots
	uint8_t repeCode;
};

/* FAST-FEEDBACK Allocation IE */   // 52 bits
struct OFDMA_ULMAP_IE_0 {
	uint16_t cid;
	uint8_t uiuc;
	uint8_t symOff;
	uint8_t chOff;
	uint8_t numSym;
	uint8_t numCh;
	uint8_t rsv;
};

/* Extended-2 UIUC IE */    // (32 + length * 8) bits
struct OFDMA_ULMAP_IE_11 {
	uint16_t cid;
	uint8_t uiuc;
	uint8_t ext2_uiuc;
	uint8_t length;
	void *data;
};

/* CDMA Bandwidth-Request, CDMA Ranging IE */  // 52 bits
struct OFDMA_ULMAP_IE_12 {
	uint16_t cid;
	uint8_t uiuc;
	uint8_t symOff;
	uint8_t chOff;
	uint8_t numSym;
	uint8_t numCh;
	uint8_t rangMethod;
	uint8_t rangIndicator;
};

/* PAPR Reduction and Safety Zone Sllocation IE */  // 52 bits
/* Relay zone indicate MS does not to process in uplink relay zone*/
struct OFDMA_ULMAP_IE_13 {
	uint16_t cid;
	uint8_t uiuc;
	uint8_t symOff;
	uint8_t chOff;
	uint8_t numSym;
	uint8_t numCh;
	uint8_t papr_or_safety;
	uint8_t sounding;
	uint8_t relay_zone_ind;
};

/* CDMA Allocation IE */    // 60 bits
struct OFDMA_ULMAP_IE_14 {
	uint16_t cid;       // 16 bits
	uint8_t uiuc;       // 4  
	uint8_t duration;   // 6  
	uint8_t uiuc_trans; // 4  
	uint8_t repeCode;   // 2  
	uint8_t frameIndex; // 4  
	uint8_t rangCode;   // 8  
	uint8_t rangSym;    // 8  
	uint8_t rangCh;     // 7  
	uint8_t bwReq;      // 1  
	//uint16_t relayRScid;	//trick for record select access station
};

/* Extended UIUC IE */  // (28 + length * 8) bits
struct OFDMA_ULMAP_IE_15 {
	uint16_t cid;
	uint8_t uiuc;
	uint8_t ext_uiuc;
	uint8_t length;
	void *data;
};

/* UL_Zone_switch_IE , ext-DIUC = 04*/
struct UL_Zone_Switch_IE {
	uint16_t symoff:7;
	uint16_t permut:2;
	uint16_t ul_permb:7;
	uint8_t amc_type:2;
	uint8_t use_subchan:1;
	uint8_t rsv:5;
};

/* WiMAX 16j UL-MAP burst-based IE */
struct UL_Burst_Receive_IE {
	uint8_t	Nr;
};

struct RS_BW_Alloc_IE_0 {
    uint8_t type:2;
    uint8_t tid:4;
    uint8_t dlmap_ie_idx:8;
};

struct RS_BW_Alloc_IE_1 {
    uint8_t type:2;
    uint8_t frameIndex:4;
    uint8_t num_reject:4;
    uint8_t inc_rng_suc:1;
    uint8_t ind_dfo:1;
    uint8_t dlmap_ie_idx:8;
};    

struct RS_BW_Alloc_IE_2 {
    uint8_t type:2;
    uint8_t mess_type:2;
    uint8_t dlmap_ie_idx:8;
};
    
/* Union of DLMAP_ie and ULMAP_ie*/
typedef union DLMAP_IE_union{
	struct OFDMA_DLMAP_IE_other ie_other;
	struct OFDMA_DLMAP_IE_13    ie_13;
	struct OFDMA_DLMAP_IE_14    ie_14;
	struct OFDMA_DLMAP_IE_15    ie_15;
}DLMAP_IE_u;

typedef union ULMAP_IE_union{
	struct OFDMA_ULMAP_IE_other ie_other;
	struct OFDMA_ULMAP_IE_0     ie_0;
	struct OFDMA_ULMAP_IE_11    ie_11;
	struct OFDMA_ULMAP_IE_12    ie_12;
	struct OFDMA_ULMAP_IE_13    ie_13;
	struct OFDMA_ULMAP_IE_14    ie_14;
	struct OFDMA_ULMAP_IE_15    ie_15;
}ULMAP_IE_u;

typedef union RS_BW_Alloc_IE_union{
    struct RS_BW_Alloc_IE_0 ie_0;
    struct RS_BW_Alloc_IE_1 ie_1;
    struct RS_BW_Alloc_IE_2 ie_2;
}RS_BW_Alloc_IE_u;

namespace MR_OFDMAMapIE_NT {
	class OFDMA_DLMAP_IE {
		public:
			uint8_t *_ie_data;
			int     _ie_bits;
			int     _offset;
			uint8_t _diuc;

		public:
			OFDMA_DLMAP_IE();
			OFDMA_DLMAP_IE(int);
			OFDMA_DLMAP_IE(uint8_t *, int);
			~OFDMA_DLMAP_IE();

			void        mallocIE        (int);
			void        appendBitField  (int, uint32_t);
			void        appendBitField  (int, uint8_t *);
			void        extractField    (int, uint8_t *);
			inline      int     getBits ()          { return _ie_bits; }
			inline      uint8_t *getData()          { return _ie_data; }
			inline      uint8_t getDIUC ()          { return _diuc; }
			inline      void    setDIUC (uint8_t d) { _diuc = d; }
	};

	class OFDMA_ULMAP_IE {
		public:
			uint8_t     *_ie_data;
			int         _ie_bits;
			int         _offset;
			uint16_t    _cid;
			uint8_t     _uiuc;

		public:
			OFDMA_ULMAP_IE();
			OFDMA_ULMAP_IE(uint16_t, int);
			OFDMA_ULMAP_IE(uint8_t *, int);
			~OFDMA_ULMAP_IE();

			void        mallocIE        (int);
			void        appendBitField  (int, uint32_t);
			void        appendBitField  (int, uint8_t *);
			void        extractField    (int, uint8_t *);
			inline      int     getBits     ()              { return _ie_bits; }
			inline      uint8_t *getData    ()              { return _ie_data; }
			inline      uint8_t getUIUC     ()              { return _uiuc; }
			inline      void    setUIUC     (uint8_t u)     { _uiuc = u; }
			inline      uint8_t getCID      ()              { return _cid; }
			inline      void    setCID      (uint16_t c)    { _cid = c; }
			inline      void    resetOffset ()              { _offset = 0; }
	};
}

#endif          /*  __NCTUNS_80216J_OFDMA_MAP_IE_H__  */ 
