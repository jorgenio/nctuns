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

#ifndef _NCTUNS_LOGHEAP_H_
#define _NCTUNS_LOGHEAP_H_

#include <sys/types.h>
#include <list>
#include <queue>
#include <functional>
#include <stdio.h>
#include <event.h>


/*=======================================*
 *	define log data structure	 *
 *=======================================*/
/* define variables for special purposes */

/* This value must great than the
 * maximum node id in any topology.
 */
#define PHY_BROADCAST_ID	50000

/* define protocol */
#define PROTO_802_3		0x01
#define PROTO_802_11		0x02
#define PROTO_OPHY		0x03
#define PROTO_GPRS		0x04
#define PROTO_802_16		0x05
#define PROTO_SAT_DVBRCS	0x06
#define PROTO_802_16e		0x07
#define PROTO_802_16j		0x08
#define PROTO_802_16j_NT	0x09

/* define event types */
#define StartTX         0x01
#define StartRX         0x02
#define SuccessTX       0x03
#define SuccessRX       0x04
#define DropTX          0x05
#define DropRX          0x06
#define EndRX		0x07

/* define event types for GUI */
#define EVENTTYPE_TX    0x01
#define EVENTTYPE_RTX   0x02
#define EVENTTYPE_RX    0x03
#define EVENTTYPE_BTX   0x04
#define EVENTTYPE_BRX   0x05
#define EVENTTYPE_DROP  0x06

/* define frame types */
#define FRAMETYPE_DATA  		0x01
#define FRAMETYPE_RTS   		0x02
#define FRAMETYPE_CTS   		0x03
#define FRAMETYPE_ACK   		0x04
#define FRAMETYPE_BEACON		0x05	/* Beacon */
#define FRAMETYPE_ASSQ			0x06	/* Asso request */
#define FRAMETYPE_ASSR			0x07	/* Asso reply */
#define FRAMETYPE_REASSQ		0x08	/* ReAsso request */
#define FRAMETYPE_REASSR		0x09	/* ReAsso reply */
#define FRAMETYPE_DISASS		0x0a	/* DisAsso request */
#define FRAMETYPE_PROBQ			0x0b	/* Probe request */
#define FRAMETYPE_PROBR			0x0c	/* Probe reply */
#define FRAMETYPE_APINFO		0x0d	/* ap information */
#define FRAMETYPE_OBS_DATA		0x0e	/* OBS data packet */
#define FRAMETYPE_OBS_CTRL_NORMAL	0x0f	/* OBS normal control packet */
#define FRAMETYPE_OBS_CTRL_SWITCH	0x10	/* OBS switching control packet */
#define FRAMETYPE_OPTICAL_LP		0x11	/* Light path configuration packet */
#define FRAMETYPE_OPTICAL_DATA		0x12	/* Light path data packet */
#define FRAMETYPE_GPRS_DATA		0x13	/* GPRS user-data burst */
#define FRAMETYPE_GPRS_ACCESS		0x14	/* GPRS access burst */
#define FRAMETYPE_GPRS_DUMMY		0x15	/* GPRS dummy burst */
#define FRAMETYPE_GPRS_CTL		0x16	/* GPRS control burst */
#define FRAMETYPE_ACTION		0x17	/* QoS action field */
#define FRAMETYPE_QOS_DATA		0x18	/* QoS data packet*/
#define FRAMETYPE_QOS_ACK		0x19	/* QoS ack packet */
#define FRAMETYPE_QOS_POLL		0x1a	/* QoS poll packet */
#define FRAMETYPE_QOS_NULL		0x1b	/* QoS null packet */
#define FRAMETYPE_WIMAX_MESH_DATA	0x1c   /* WiMAX Mesh mode data burst */
#define FRAMETYPE_WIMAX_MESH_NENT	0x1d   /* WiMAX Mesh mode netowrk Entry burst */
#define FRAMETYPE_WIMAX_MESH_NCFG	0x1e   /* WiMAX Mesh mode network config burst */
#define FRAMETYPE_WIMAX_MESH_DSCH	0x20   /* WiMAX Mesh mode distributed schedule burst */
#define FRAMETYPE_WIMAX_MESH_SBCREQ	0x21   /* WiMAX Mesh mode sbc request burst */
#define FRAMETYPE_WIMAX_MESH_REGREQ	0x22   /* WiMAX Mesh mode register request burst */
#define FRAMETYPE_WIMAX_MESH_SPONSOR	0x23   /* WiMAX Mesh mode sponsor burst */
#define FRAMETYPE_WIMAX_PMP_DBURST	0x24   /* WiMAX PMP mode downlink burst */
#define FRAMETYPE_WIMAX_PMP_UBURST	0x25   /* WiMAX PMP mode uplink burst */
#define FRAMETYPE_WIMAX_PMP_DLMAP	0x26   /* WiMAX PMP mode downlink map */
#define FRAMETYPE_WIMAX_PMP_DLFP	0x27   /* WiMAX PMP mode downlink frame prefix */
#define FRAMETYPE_WIMAX_PMP_ULMAP	0x28   /* WiMAX PMP mode uplink map */

#define FRAMETYPE_PKT_TABLE		0x29	/* ncc control table */
#define FRAMETYPE_PKT_SECTION		0x2a	/* section frame */
#define FRAMETYPE_PKT_MPEG2_TS		0x2b	/* mpeg2-ts frame */
#define FRAMETYPE_PKT_DVB_S2		0x2c	/* dvb-s2 frame */
#define FRAMETYPE_PKT_RCSSYNC		0x2d	/* dvs-rcs sync control burst */
#define FRAMETYPE_PKT_RCSCSC		0x2e	/* dvs-rcs csc control burst */
#define FRAMETYPE_PKT_ATM		0x2f	/* atm frame */
#define FRAMETYPE_PKT_RCSMAC		0x30	/* dvs-rcs mac burst */
#define FRAMETYPE_PKT_DVBRCS		0x31	/* dvb-rcs frame */
#define FRAMETYPE_PKT_RAWDATA		0x32	/* raw data */
#define FRAMETYPE_PKT_NONE		0x33	/* none */

#define FRAMETYPE_MobileWIMAX_PMP_DBURST    0x34 /* MobileWiMAX PMP mode downlink burst */
#define FRAMETYPE_MobileWIMAX_PMP_UBURST    0x35 /* MobileWiMAX PMP mode uplink burst */
#define FRAMETYPE_MobileWIMAX_PMP_DLMAP     0x36 /* MobileWiMAX PMP mode DL-MAP message */
#define FRAMETYPE_MobileWIMAX_PMP_DLFP      0x37 /* MobileWiMAX PMP mode DLFP */

#define FRAMETYPE_MobileRelayWIMAX_PMP_DABURST     0x38   /* MobileRelayWiMAX PMP mode downlink access burst */
#define FRAMETYPE_MobileRelayWIMAX_PMP_UABURST     0x39   /* MobileRelayWiMAX PMP mode uplink access burst */
#define FRAMETYPE_MobileRelayWIMAX_PMP_DTBURST     0x3a   /* MobileRelayWiMAX PMP mode downlink transparent burst */
#define FRAMETYPE_MobileRelayWIMAX_PMP_URBURST     0x3b   /* MobileRelayWiMAX PMP mode uplink relay burst */
#define FRAMETYPE_MobileRelayWIMAX_PMP_DLMAP       0x3c   /* MobileRelayWiMAX PMP mode DL-MAP message */
#define FRAMETYPE_MobileRelayWIMAX_PMP_DLFP        0x3d   /* MobileRelayWiMAX PMP mode DLFP */

#define FRAMETYPE_MR_WIMAX_NT_PMP_DLAccessBURST     0x3e /* MR_WIMAX_NT PMP mode downlink burst */
#define FRAMETYPE_MR_WIMAX_NT_PMP_ULAccessBURST     0x3f /* MR_WIMAX_NT PMP mode uplink burst */
#define FRAMETYPE_MR_WIMAX_NT_PMP_DLRelayBURST      0x40 /* MR_WIMAX_NT PMP mode downlink burst */
#define FRAMETYPE_MR_WIMAX_NT_PMP_ULRelayBURST      0x41 /* MR_WIMAX_NT PMP mode uplink burst */
#define FRAMETYPE_MR_WIMAX_NT_PMP_DLMAP             0x42 /* MR_WIMAX_NT PMP mode DL-MAP message */
#define FRAMETYPE_MR_WIMAX_NT_PMP_DLFP              0x43 /* MR_WIMAX_NT PMP mode DLFP */
#define FRAMETYPE_MR_WIMAX_NT_PMP_RMAP              0x44 /* MR_WIMAX_NT PMP mode R-MAP message */
#define FRAMETYPE_MR_WIMAX_NT_PMP_RzonePrefix       0x45 /* MR_WIMAX_NT PMP mode RP */

/* define drop reason */
#define DROP_NONE	0x00
#define DROP_COLL       0x01
#define DROP_CAP        0x02
#define DROP_DUPX       0x03
#define DROP_BER        0x04
#define DROP_RXERR      0x05
#define DROP_POW_CS     0x06
#define DROP_POW_RX     0x07
#define DROP_RX_MAC_HEADER_ERROR 0x08


typedef struct logEvent {
	u_char		PROTO;
/* add to Time, because when time is the same, if ending record
 * is inserted first, it won't work correctly.
 */
#define START		0
#define ENDING		1
	u_int64_t	Time;
	char		*log;
	void		*log2;
} logEvent;

struct mac802_3_log {
        u_char          PROTO;
	u_int64_t	sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t       IP_Src;
        u_int32_t       IP_Dst;
        u_int32_t       PHY_Src;
        u_int32_t       PHY_Dst;
        u_int32_t       RetryCount;
        u_int64_t       FrameID;
        u_char          FrameType;
        u_int32_t       FrameLen;
        u_char          DropReason;
};

struct mac802_11_log{
        u_char          PROTO;
	u_int64_t	sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t       IP_Src;
        u_int32_t       IP_Dst;
        u_int32_t       PHY_Src;
        u_int32_t       PHY_Dst;
	u_int32_t	MAC_Dst;
        u_int32_t       RetryCount;
        u_int64_t       FrameID;
        u_char          FrameType;
        u_int32_t       FrameLen;
        u_char          DropReason;
        u_char		Channel;
};

#ifdef CONFIG_OPTICAL
struct ophy_log {
        u_char          PROTO;
	u_int64_t	sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t       IP_Src;
        u_int32_t       IP_Dst;
        u_int32_t       PHY_Src;
        u_int32_t       PHY_Dst;
        u_int32_t       RetryCount;
        u_int64_t       FrameID;
        u_char          FrameType;
        u_int32_t       FrameLen;
        u_char          DropReason;
	u_char		Channel;
};
#endif	/* CONFIG_OPTICAL */

#ifdef CONFIG_GPRS
struct gprs_log{
        u_char          PROTO;
	u_int64_t	sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t	IP_Src;
        u_int32_t	IP_Dst;
        u_int32_t	PHY_Src;
        u_int32_t	PHY_Dst;
        u_int32_t	RetryCount;
        u_int64_t       BurstID;
        u_char          BurstType;
        u_int32_t       BurstLen;
        u_char		DropReason;
        u_char		Channel;
};
#endif	/* CONFIG_GPRS */

#ifdef CONFIG_WIMAX
typedef struct mac802_16_log {

        u_char          PROTO;
        u_int64_t       sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t	    IP_Src;
        u_int32_t	    IP_Dst;
        u_int32_t	    PHY_Src;
        u_int32_t       PHY_Dst;
        u_int32_t       RetryCount;
        u_int64_t       ConnID;
        u_char          BurstType;
        u_int32_t       BurstLen;
        u_char		    DropReason;
        u_char		    Channel;

} mac80216_log_t;

int dump_mac80216_logep(mac80216_log_t* logep);
#endif	/* CONFIG_WIMAX */

#ifdef CONFIG_MobileWIMAX
typedef struct mac802_16e_log {

        u_char          PROTO;
        u_int64_t       sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t	    IP_Src;
        u_int32_t	    IP_Dst;
        u_int32_t	    PHY_Src;
        u_int32_t       PHY_Dst;
        u_int32_t       RetryCount;
        u_char          BurstType;
        u_int32_t       BurstLen;
        u_char		    DropReason;
        u_char		    Channel;
   
} mac80216e_log_t;

#endif  /* CONFIG_MobileWIMAX */

#ifdef CONFIG_MobileRelayWIMAX
typedef struct mac802_16j_log {

        u_char          PROTO;
        u_int64_t       sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
        u_int32_t	    IP_Src;
        u_int32_t	    IP_Dst;
        u_int32_t	    PHY_Src;
        u_int32_t       PHY_Dst;
        u_int32_t       RetryCount;
        u_char          BurstType;
        u_int32_t       BurstLen;
        u_char		    DropReason;
        u_char		    Channel;
   
} mac80216j_log_t;

#endif  /* CONFIG_MobileRelayWIMAX */

#ifdef CONFIG_MR_WIMAX_NT
typedef struct mac802_16j_NT_log {
	u_char		PROTO;
	u_int64_t	sTime;
	u_int64_t	Time;
	u_int32_t	NodeType;
	u_int32_t	NodeID;
	u_char		Event;
	u_int32_t	IP_Src;
	u_int32_t	IP_Dst;
	u_int32_t	PHY_Src;
	u_int32_t	PHY_Dst;
	u_int32_t	RetryCount;
	u_char		BurstType;
	u_int32_t	BurstLen;
	u_char		DropReason;
	u_char		Channel;

} mac80216j_NT_log_t;
#endif	/* CONFIG_MR_WIMAX_NT */



#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
struct dvbrcs_log{
        u_char          PROTO;
	u_int64_t	sTime;
        u_int64_t       Time;
        u_int32_t       NodeType;
        u_int32_t       NodeID;
        u_char          Event;
         u_int32_t	IP_Src;
        u_int32_t	IP_Dst;
       u_int32_t	PHY_Src;
        u_int32_t	PHY_Dst;
        u_int32_t	RetryCount;
        u_int64_t       BurstID;
        u_char          BurstType;
        u_int32_t       BurstLen;
        u_char		DropReason;
        u_char		Channel;
};
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */


using namespace std;

typedef struct logChain {
	u_char	PROTO;
	void	*log;
	void	*log_end;
} logChain;


class logComp : public binary_function<logEvent *, logEvent *, bool>
{
public:
        bool operator()(logEvent *x, logEvent *y) {
                return x->Time > y->Time;
        };
};

typedef list<logChain *>		logList;
typedef list<logChain *>::iterator	liter;
extern	priority_queue<logEvent *, vector<logEvent *>, logComp>  logQ;

int	DequeueLogFromHeap(Event_ *);
void	pkt_trace_log(logEvent * logep);

#endif /* !_NCTUNS_LOGHEAP_H_ */
