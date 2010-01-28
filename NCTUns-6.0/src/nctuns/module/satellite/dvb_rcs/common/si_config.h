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

#ifndef __NCTUNS_si_config_h__
#define __NCTUNS_si_config_h__

#define	DVBS2_NETWORK_ID	0
#define TS_ID			0
#define INIT_VER_NUM            0
#define IP_MAC_PLAT_NUM		1
#define IP_MAC_PLAT_ID		0
#define IP_MAC_PLAT_ID_HASH	0
#define UNKNOWN_LEN		0

/*
 * which when set to '1' indicates that the table sent is currently applicable.
 * when the bit is set to '0', it indicates the table sent is not yet applicable
 * and shall be the next table to become valid.
 */
enum CUR_NEXT_INDICATOR {

        NEXT_INDICATOR          = 0,
        CUR_INDICATOR           = 1
};

enum STREAM_TYPE {

	PRIVATE_SECTION		= 0x05,
	MPE_STREAM		= 0x0D
};

enum SI_TABLE_ID {

        PAT_TABLE_ID		= 0x00,
        CAT_TABLE_ID		= 0x01,
        PMT_TABLE_ID		= 0x02,
        DGM_TABLE_ID		= 0x3E,
        NIT_ACTIVE_TABLE_ID	= 0x40,
        RMT_TABLE_ID		= 0x41,
	INT_TABLE_ID		= 0x4C,
        SCT_TABLE_ID		= 0xA0,
        FCT_TABLE_ID		= 0xA1,
        TCT_TABLE_ID		= 0xA2,
        SPT_TABLE_ID		= 0xA3,
        CMT_TABLE_ID		= 0xA4,
        TBTP_TABLE_ID		= 0xA5,
        PCR_PACKET_PAYLOAD	= 0xA6,
        TIM_TABLE_ID		= 0xB0
};

enum SI_PRO_NUM {

	NIT_PRO_NUM		= 0,
	INT_PRO_NUM		= 1,
	/* indicate TCP/IP traffic */
	TRF_PRO_NUM		= 2
};

enum SI_PID {

	PAT_PID		= 0x0000,
	CAT_PID		= 0x0001,
	TSDT_PID	= 0x0002,

	/* 0x00010 ~ 0x1FFE may be assigned as network_PID, program_map_PID,
	 * elementary_PID, or for other purposes.
	 */
	NIT_PID		= 0x0010,
	SDT_PID		= 0x0011,
	BAT_PID		= 0x0011,
	EIT_PID		= 0x0012,
	CIT_PID		= 0x0012,
	RST_PID		= 0x0013,
	TDT_PID		= 0x0014,
	TOT_PID		= 0x0014,
	RNT_PID		= 0x0016,
	DIT_PID		= 0x001E,
	SIT_PID		= 0x001F,

	/*
	 * user defined
	 * between 0x0030 and 0x0040 reserved for PID of tables of return link
	 */
	RMT_PID		= 0x0030,
	SCT_PID		= 0x0031,
	FCT_PID		= 0x0032,
	TCT_PID		= 0x0033,
	SPT_PID		= 0x0034,
	CMT_PID		= 0x0035,
	TBTP_PID	= 0x0036,
	PCR_PKT_PID	= 0x0037,
	TX_TABLE_PID	= 0x0038,
	TIM_PID		= 0x0039,
	
	/*
         * user defined
	 * beyond 0x0100, defined as PID of each RCST 
         */
	INT_PID		= 0x0050,
	INT_PMT_PID	= 0x0051,
	TRF_PMT_PID	= 0x0052,
	
	NIL_PKT_PID 	= 0x1FFF
};

#endif /* __NCTUNS_si_config_h__ */
