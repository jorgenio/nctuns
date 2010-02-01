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

#ifndef	__NCTUNS_mac_802_11e_h__
#define __NCTUNS_mac_802_11e_h__

#include <mac-802_11e_management_framebody.h>

#define ETHER_ADDR_LEN  	        6
#define ETHER_FCS_LEN          		4

#define BIT_0                   0x00
#define BIT_1                   0x01


#define MAC_ProtocolVersion		0x00

#define MAC_Type_Management		0x00
#define MAC_Type_Control		0x01
#define MAC_Type_Data			0x02
#define MAC_Type_Reserved		0x03

/* define subtypes of MAC_Type_Management */
#define MAC_Subtype_Asso_Req		0x0e //0x00
			/* different from MAC_Subtype_Data */
			/* cause the MAC layer sends the management
			   pkts as the data pkts, it is a temporal
			   solution */	
#define MAC_Subtype_Asso_Resp		0x01
#define MAC_Subtype_ReAsso_Req		0x02
#define MAC_Subtype_ReAsso_Resp		0x03
#define MAC_Subtype_Probe_Req   	0x04
#define MAC_Subtype_Probe_Resp  	0x05
#define MAC_Subtype_Beacon		0x08
#define MAC_Subtype_DisAsso		0x0a
#define MAC_Subtype_Action		0x0d
#define MAC_Subtype_ApInfo		0x0f

/* define subtypes of MAC_Type_Control */
#define MAC_Subtype_RTS			0x0B
#define MAC_Subtype_CTS			0x0C
#define MAC_Subtype_ACK         	0x0D
#define MAC_Subtype_CFEnd		0x0E	//e	
#define MAC_Subtype_CFEnd_Ack		0x0F	//e

/* define subtypes of MAC_Type_Data */
#define MAC_Subtype_Data        	0x00
#define MAC_Subtype_QoS_Data		0x08	 
#define MAC_Subtype_QoS_Data_Ack	0x09
#define MAC_Subtype_QoS_Data_Poll	0x0a
#define MAC_Subtype_QoS_Data_Ack_Poll	0x0b
#define MAC_Subtype_QoS_Null		0x0c
#define MAC_Subtype_QoS_Ack		0x0d
#define MAC_Subtype_QoS_Poll		0x0e
#define	MAC_Subtype_QoS_Ack_Poll	0x0f

/* ACK policy */
#define ACKP_NORMAL_ACK  0x0    // Normal Acknowledgement
#define ACKP_NO_ACK      0x1    // No Acknowledgement
#define ACKP_NOEXP_ACK   0x2    // No Explicit Acknowledgement
#define ACKP_BLOCK_ACK   0x3    // Block Acknowledgement


struct frame_control {
	u_char          fc_subtype              : 4;
	u_char          fc_type                 : 2;
	u_char          fc_protocol_version     : 2;

        u_char          fc_order                : 1;
        u_char          fc_wep                  : 1;
        u_char          fc_more_data            : 1;
        u_char          fc_pwr_mgt              : 1;
        u_char          fc_retry                : 1;
        u_char          fc_more_frag            : 1;
        u_char          fc_from_ds              : 1;
        u_char          fc_to_ds                : 1;
};

struct QoS_control {					//802.11e
	u_char		tid			: 4;
	u_char		eosp			: 1;
	u_char		ack_policy		: 2;
	u_char		reserved		: 1;
	u_int8_t	txop_queue		   ;
};

struct hdr_mac802_11e {
        struct frame_control    dh_fc;
        u_int16_t               dh_duration;
        u_char                  dh_addr1[ETHER_ADDR_LEN];
        u_char                  dh_addr2[ETHER_ADDR_LEN];
        u_char                  dh_addr3[ETHER_ADDR_LEN];
        u_int16_t               dh_scontrol;
//	u_char			dh_addr4[ETHER_ADDR_LEN];
	struct QoS_control	dh_qos; 
	u_char			dh_fcs[ETHER_FCS_LEN]; 
};

struct hdr_mac802_11e_manage {
        struct frame_control    dh_fc;
        u_int16_t               dh_duration;
        u_char                  dh_addr1[ETHER_ADDR_LEN];
        u_char                  dh_addr2[ETHER_ADDR_LEN];
        u_char                  dh_addr3[ETHER_ADDR_LEN];
        u_int16_t               dh_scontrol;
	u_char			dh_fcs[ETHER_FCS_LEN]; 
};

struct hdr_mac802_11 {
        struct frame_control    dh_fc;
        u_int16_t               dh_duration;
        u_char                  dh_addr1[ETHER_ADDR_LEN];
        u_char                  dh_addr2[ETHER_ADDR_LEN];
        u_char                  dh_addr3[ETHER_ADDR_LEN];
        u_int16_t               dh_scontrol;
//      u_char			dh_addr4[ETHER_ADDR_LEN];
	u_char			dh_fcs[ETHER_FCS_LEN]; 
};

/* RTS control frame */
struct rts_frame {
        struct frame_control    rf_fc;
        u_int16_t               rf_duration;
        u_char                  rf_ra[ETHER_ADDR_LEN];
        u_char                  rf_ta[ETHER_ADDR_LEN];
        u_char                  rf_fcs[ETHER_FCS_LEN];
};
                   
/* CTS control frame */                     
struct cts_frame {
        struct frame_control    cf_fc;
        u_int16_t               cf_duration;
        u_char                  cf_ra[ETHER_ADDR_LEN];
        u_char                  cf_fcs[ETHER_FCS_LEN];
};

/* ACK control frame */
struct ack_frame {
        struct frame_control    af_fc;
        u_int16_t               af_duration;
        u_char                  af_ra[ETHER_ADDR_LEN];
        u_char                  af_fcs[ETHER_FCS_LEN];
};
/* 
struct BAR_control {					//802.11e
	u_char	bar_reserve1			   ;
	u_char  bar_reserve2			: 4;
	u_char	bar_tid				: 4;
};

struct BA_control {					//802.11e
	u_char	bar_reserve1			   ;
	u_char  bar_reserve2			: 4;
	u_char	bar_tid				: 4;
};


struct BASS_control {					//802.11e
	u_int16_t 		BASS_startseq	   ;   	// b0-b3 always 0
};
*/
/* Block Ack Request control frame */
/*
struct backr_frame {				//802.11e
	struct frame_control	bf_fc;
	u_int16_t		bf_duration;
	u_char			bf_ra[ETHER_ADDR_LEN];
	u_char			bf_ta[ETHER_ADDR_LEN];
	struct BAR_control	bf_bc;
	struct BASS_control	bf_bass;
	u_char			bf_fcs[ETHER_FCS_LEN];
};
*/
/* Block Ack control frame */
/*
struct back_frame {					//802.11e
	struct frame_control	bf_fc;
	u_int16_t		bf_duration;
	u_char			bf_ra[ETHER_ADDR_LEN];
	u_char			bf_ta[ETHER_ADDR_LEN];
	struct BA_control	bf_bc;
	struct BASS_control	bf_bass;
	u_char			bf_bitmap[128];
	u_char			bf_fcs[ETHER_FCS_LEN];
};
*/
#endif	/* __NCTUNS_mac_802_11e_h__ */
