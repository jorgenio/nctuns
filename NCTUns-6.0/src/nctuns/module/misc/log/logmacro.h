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

#ifndef __NCTUNS_log_macro_h__
#define __NCTUNS_log_mcaro_h__

/* define macro for logging packet trace event */
#define GET_IP_SRC_DST(MAC_HEADER,IP_HEADER,IPSRC,IPDST)		\
{									\
	if(MAC_HEADER->dh_fc.fc_subtype == MAC_Subtype_Data		\
		&& IP_HEADER != NULL) {					\
		IP_DST(IPDST,IP_HEADER);				\
		IP_SRC(IPSRC,IP_HEADER);				\
									\
		if(is_ipv4_broadcast(get_nid(),IPDST))			\
			IPDST = 0;					\
		if(is_ipv4_broadcast(get_nid(),IPSRC))			\
			IPSRC = 0;					\
	}								\
	if(MAC_HEADER->dh_fc.fc_subtype == MAC_Subtype_QoS_Data             \
                && IP_HEADER != NULL) {                                 \
                IP_DST(IPDST,IP_HEADER);                                \
                IP_SRC(IPSRC,IP_HEADER);                                \
                                                                        \
                if(is_ipv4_broadcast(get_nid(),IPDST))                  \
                        IPDST = 0;                                      \
                if(is_ipv4_broadcast(get_nid(),IPSRC))                  \
                        IPSRC = 0;                                      \
        }                                                               \
	else								\
		IPDST = IPSRC = 0;					\
}

#define INSERT_TO_HEAP(EVENT_P,PROTO_,TIME,LOG)				\
{									\
	EVENT_P = (struct logEvent*)malloc(sizeof(struct logEvent));	\
	EVENT_P->PROTO = PROTO_;					\
	EVENT_P->Time = TIME;						\
	EVENT_P->log = (char *)LOG;					\
	logQ.push(EVENT_P);						\
}

#define LOG_802_11(MAC_HEADER,LOG,PACKET,TIME,STIME,NODE_TYPE,NODE_ID,	\
			EVENT,IP_SRC,IP_DST,CHANNEL,TXNID)			\
{									\
	LOG->PROTO 	= PROTO_802_11;					\
	LOG->Time	= TIME;						\
	LOG->sTime	= STIME;					\
	LOG->NodeType	= NODE_TYPE;					\
	LOG->NodeID	= NODE_ID;					\
	LOG->Event	= EVENT;					\
	LOG->FrameID	= PACKET->pkt_getpid();				\
	LOG->FrameLen	= PACKET->pkt_getlen();				\
	LOG->DropReason	= DROP_NONE;					\
	LOG->Channel	= (u_char)CHANNEL;				\
									\
	if(LOG->Event == StartRX || LOG->Event == SuccessRX) {		\
		LOG->PHY_Src = macaddr_to_nodeid(MAC_HEADER->dh_addr2);	\
									\
		if (!bcmp(MAC_HEADER->dh_addr1, etherbroadcastaddr,	\
			ETHER_ADDR_LEN)) {				\
			LOG->PHY_Dst = NODE_ID + PHY_BROADCAST_ID;	\
			LOG->MAC_Dst = 0;				\
		}							\
		else {							\
			LOG->PHY_Dst = NODE_ID;				\
			LOG->MAC_Dst = macaddr_to_nodeid(MAC_HEADER->dh_addr1);\
		}							\
	}								\
	else if(LOG->Event == StartTX || LOG->Event == SuccessTX) {	\
		LOG->PHY_Src = NODE_ID;					\
									\
		if (!bcmp(MAC_HEADER->dh_addr1, etherbroadcastaddr,	\
			ETHER_ADDR_LEN))				\
			LOG->PHY_Dst = PHY_BROADCAST_ID;		\
		else							\
			LOG->PHY_Dst = macaddr_to_nodeid(		\
				MAC_HEADER->dh_addr1);			\
									\
		LOG->MAC_Dst = LOG->PHY_Dst;				\
	}								\
									\
	if(IP_SRC == 0)							\
		LOG->IP_Src = 0;					\
	else								\
		LOG->IP_Src = ipv4addr_to_nodeid(IP_SRC);		\
									\
	if(IP_DST == 0)							\
		LOG->IP_Dst = 0;					\
	else								\
		LOG->IP_Dst = ipv4addr_to_nodeid(IP_DST);		\
									\
	switch(MAC_HEADER->dh_fc.fc_type){				\
		case MAC_Type_Management:				\
			switch(MAC_HEADER->dh_fc.fc_subtype) {		\
				case MAC_Subtype_Asso_Req:		\
					LOG->FrameType = FRAMETYPE_ASSQ;\
					break;				\
				case MAC_Subtype_Asso_Resp:		\
					LOG->FrameType = FRAMETYPE_ASSR;\
					break;				\
				case MAC_Subtype_ReAsso_Req:		\
					LOG->FrameType = FRAMETYPE_REASSQ;\
					break;				\
				case MAC_Subtype_ReAsso_Resp:		\
					LOG->FrameType = FRAMETYPE_REASSR;\
					break;				\
				case MAC_Subtype_Probe_Req:		\
					LOG->FrameType = FRAMETYPE_PROBQ;\
					break;				\
				case MAC_Subtype_Probe_Resp:		\
					LOG->FrameType = FRAMETYPE_PROBR;\
					break;				\
				case MAC_Subtype_Beacon:		\
					LOG->FrameType = FRAMETYPE_BEACON;\
					break;				\
				case MAC_Subtype_DisAsso:		\
					LOG->FrameType = FRAMETYPE_DISASS;\
					break;				\
				case MAC_Subtype_ApInfo:		\
					LOG->FrameType = FRAMETYPE_APINFO;\
					break;				\
				case MAC_Subtype_Action:		\
					LOG->FrameType = FRAMETYPE_ACTION;\
					break;				\
				default:				\
				ErrorMesg("Log : Invalid MAC Management Subtype!");\
			}						\
			break;						\
		case MAC_Type_Control:					\
			switch(MAC_HEADER->dh_fc.fc_subtype) {		\
				case MAC_Subtype_RTS:			\
					LOG->FrameType = FRAMETYPE_RTS;	\
					break;				\
				case MAC_Subtype_CTS:			\
					LOG->FrameType = FRAMETYPE_CTS;	\
					LOG->PHY_Src = TXNID;		\
					break;				\
				case MAC_Subtype_ACK:			\
					LOG->FrameType = FRAMETYPE_ACK;	\
					LOG->PHY_Src = TXNID;		\
					break;				\
				default:				\
				ErrorMesg("Log : Invalid MAC Control Subtype!");\
			}						\
			break;						\
		case MAC_Type_Data:					\
			switch(MAC_HEADER->dh_fc.fc_subtype) {		\
				case MAC_Subtype_Data:			\
					LOG->FrameType = FRAMETYPE_DATA;\
					break;				\
				case MAC_Subtype_QoS_Data:			\
					LOG->FrameType = FRAMETYPE_QOS_DATA;\
					break;				\
				case MAC_Subtype_QoS_Ack:			\
					LOG->FrameType = FRAMETYPE_QOS_ACK;\
					break;				\
				case MAC_Subtype_QoS_Poll:			\
					LOG->FrameType = FRAMETYPE_QOS_POLL;\
					break;				\
				case MAC_Subtype_QoS_Null:			\
					LOG->FrameType = FRAMETYPE_QOS_NULL;\
					break;				\
				default:				\
				ErrorMesg("Log : Invalid MAC Data Subtype");	\
			}						\
			break;						\
		default:ErrorMesg("Log : Invalid MAC Type");		\
	}								\
									\
	if (LOG->FrameLen < macmib->aRTSThreshold) 			\
		LOG->RetryCount = ssrc;					\
	else								\
		LOG->RetryCount = slrc;					\
}

#define DROP_LOG_802_11(LOG1,LOG2,TIME,EVENT,REASON)			\
{									\
	LOG2->PROTO		= PROTO_802_11;				\
	LOG2->Time		= TIME;					\
	LOG2->sTime		= LOG1->sTime;				\
	LOG2->NodeType		= LOG1->NodeType;			\
	LOG2->NodeID		= LOG1->NodeID;				\
	LOG2->Event		= EVENT;				\
	LOG2->IP_Src		= LOG1->IP_Src;				\
	LOG2->IP_Dst		= LOG1->IP_Dst;				\
	LOG2->PHY_Src		= LOG1->PHY_Src;			\
	LOG2->PHY_Dst		= LOG1->PHY_Dst;			\
	LOG2->MAC_Dst		= LOG1->MAC_Dst;			\
	LOG2->FrameID		= LOG1->FrameID;			\
	LOG2->FrameLen		= LOG1->FrameLen;			\
	LOG2->DropReason	= REASON;				\
	LOG2->Channel		= LOG1->Channel;			\
	LOG2->FrameType		= LOG1->FrameType;			\
	LOG2->RetryCount	= LOG1->RetryCount;			\
}

#ifdef CONFIG_GPRS
#define LOG_GPRS(LOG,TIME,STIME,NODE_TYPE,NODE_ID,EVENT,SRC_ID,DST_ID,  \
                        B_ID,B_TYPE,B_LEN,CHANNEL)                      \
{                                                                       \
	LOG->PROTO      = PROTO_GPRS;                                   \
	LOG->Time       = TIME;                                         \
	LOG->sTime      = STIME;                                        \
	LOG->NodeType   = NODE_TYPE;                                    \
	LOG->NodeID     = NODE_ID;                                      \
	LOG->Event      = EVENT;                                        \
	LOG->IP_Src     = 0;                                            \
	LOG->IP_Dst     = 0;                                            \
	LOG->PHY_Src    = SRC_ID;                                       \
	LOG->PHY_Dst    = DST_ID;                                       \
	LOG->RetryCount = 0;                                            \
	LOG->BurstID    = B_ID;                                         \
	LOG->BurstType  = B_TYPE;                                       \
	LOG->BurstLen   = B_LEN;                                        \
	LOG->DropReason = DROP_NONE;                                    \
	LOG->Channel    = (u_char)CHANNEL;                              \
}
#endif	/* CONFIG_GPRS */


#ifdef CONFIG_WIMAX
#define LOG_MAC_802_16(LOG,TIME,STIME,NODE_TYPE,NODE_ID,EVENT,SRC_ID,DST_ID,  \
                        UsedConnID, B_TYPE,B_LEN,CHANNEL)                      \
{                                                                       \
	(LOG)->PROTO      = PROTO_802_16;                                   \
	(LOG)->Time       = (TIME);                                         \
	(LOG)->sTime      = (STIME);                                        \
	(LOG)->NodeType   = (NODE_TYPE);                                    \
	(LOG)->NodeID     = (NODE_ID);                                      \
	(LOG)->Event      = (EVENT);                                        \
	(LOG)->IP_Src     = 0;                                            \
	(LOG)->IP_Dst     = 0;                                            \
	(LOG)->PHY_Src    = (SRC_ID);                                       \
	(LOG)->PHY_Dst    = (DST_ID);                                       \
	(LOG)->RetryCount = 0;                                            \
	(LOG)->ConnID     = (u_int64_t)(UsedConnID);                                         \
	(LOG)->BurstType  = (B_TYPE);                                       \
	(LOG)->BurstLen   = (B_LEN);                                        \
	(LOG)->DropReason = DROP_NONE;                                \
	(LOG)->Channel    = (CHANNEL);                                      \
}

#define DROP_LOG_802_16(LOG1,LOG2,TIME1,EVENT,REASON)            \
{                                                               \
	(LOG2)->PROTO         = PROTO_802_16;             \
	(LOG2)->Time          = (TIME1);                     \
	(LOG2)->sTime         = (LOG1)->sTime;              \
	(LOG2)->NodeType      = (LOG1)->NodeType;           \
	(LOG2)->NodeID        = (LOG1)->NodeID;             \
	(LOG2)->Event         = (EVENT);                    \
	(LOG2)->IP_Src        = (LOG1)->IP_Src;             \
	(LOG2)->IP_Dst        = (LOG1)->IP_Dst;             \
	(LOG2)->PHY_Src       = (LOG1)->PHY_Src;            \
	(LOG2)->PHY_Dst       = (LOG1)->PHY_Dst;            \
	(LOG2)->ConnID        = (u_int64_t)(LOG1)->ConnID;             \
	(LOG2)->BurstLen      = (LOG1)->BurstLen;           \
	(LOG2)->DropReason    = (REASON);                 \
	(LOG2)->Channel       = (LOG1)->Channel;            \
	(LOG2)->BurstType     = (LOG1)->BurstType;          \
	(LOG2)->RetryCount    = (LOG1)->RetryCount;         \
}
#endif	/* CONFIG_WIMAX */

#ifdef CONFIG_MobileWIMAX
#define LOG_MAC_802_16e(LOG,TIME,STIME,NODE_TYPE,NODE_ID,EVENT,SRC_ID,DST_ID,  \
                        B_TYPE,B_LEN,CHANNEL)                      \
{                                                                       \
	(LOG)->PROTO      = PROTO_802_16e;                                   \
	(LOG)->Time       = (TIME);                                         \
	(LOG)->sTime      = (STIME);                                        \
	(LOG)->NodeType   = (NODE_TYPE);                                    \
	(LOG)->NodeID     = (NODE_ID);                                      \
	(LOG)->Event      = (EVENT);                                        \
	(LOG)->IP_Src     = 0;                                            \
	(LOG)->IP_Dst     = 0;                                            \
	(LOG)->PHY_Src    = (SRC_ID);                                       \
	(LOG)->PHY_Dst    = (DST_ID);                                       \
	(LOG)->RetryCount = 0;                                            \
	(LOG)->BurstType  = (B_TYPE);                                       \
	(LOG)->BurstLen   = (B_LEN);                                        \
	(LOG)->DropReason = DROP_NONE;                                \
	(LOG)->Channel    = (CHANNEL);                                      \
}

#define DROP_LOG_802_16e(LOG1,LOG2,TIME1,EVENT,REASON)            \
{                                                               \
	(LOG2)->PROTO         = PROTO_802_16e;             \
	(LOG2)->Time          = (TIME1);                     \
	(LOG2)->sTime         = (LOG1)->sTime;              \
	(LOG2)->NodeType      = (LOG1)->NodeType;           \
	(LOG2)->NodeID        = (LOG1)->NodeID;             \
	(LOG2)->Event         = (EVENT);                    \
	(LOG2)->IP_Src        = (LOG1)->IP_Src;             \
	(LOG2)->IP_Dst        = (LOG1)->IP_Dst;             \
	(LOG2)->PHY_Src       = (LOG1)->PHY_Src;            \
	(LOG2)->PHY_Dst       = (LOG1)->PHY_Dst;            \
	(LOG2)->BurstLen      = (LOG1)->BurstLen;           \
	(LOG2)->DropReason    = (REASON);                 \
	(LOG2)->Channel       = (LOG1)->Channel;            \
	(LOG2)->BurstType     = (LOG1)->BurstType;          \
	(LOG2)->RetryCount    = (LOG1)->RetryCount;         \
}
#endif	/* CONFIG_MobileWIMAX */

#ifdef CONFIG_MobileRelayWIMAX
#define LOG_MAC_802_16j(LOG,TIME,STIME,NODE_TYPE,NODE_ID,EVENT,SRC_ID,DST_ID,  \
                        B_TYPE,B_LEN,CHANNEL)                      \
{                                                                       \
	(LOG)->PROTO      = PROTO_802_16j;                                   \
	(LOG)->Time       = (TIME);                                         \
	(LOG)->sTime      = (STIME);                                        \
	(LOG)->NodeType   = (NODE_TYPE);                                    \
	(LOG)->NodeID     = (NODE_ID);                                      \
	(LOG)->Event      = (EVENT);                                        \
	(LOG)->IP_Src     = 0;                                            \
	(LOG)->IP_Dst     = 0;                                            \
	(LOG)->PHY_Src    = (SRC_ID);                                       \
	(LOG)->PHY_Dst    = (DST_ID);                                       \
	(LOG)->RetryCount = 0;                                            \
	(LOG)->BurstType  = (B_TYPE);                                       \
	(LOG)->BurstLen   = (B_LEN);                                        \
	(LOG)->DropReason = DROP_NONE;                                \
	(LOG)->Channel    = (CHANNEL);                                      \
}

#define DROP_LOG_802_16j(LOG1,LOG2,TIME1,EVENT,REASON)            \
{                                                               \
	(LOG2)->PROTO         = PROTO_802_16j;             \
	(LOG2)->Time          = (TIME1);                     \
	(LOG2)->sTime         = (LOG1)->sTime;              \
	(LOG2)->NodeType      = (LOG1)->NodeType;           \
	(LOG2)->NodeID        = (LOG1)->NodeID;             \
	(LOG2)->Event         = (EVENT);                    \
	(LOG2)->IP_Src        = (LOG1)->IP_Src;             \
	(LOG2)->IP_Dst        = (LOG1)->IP_Dst;             \
	(LOG2)->PHY_Src       = (LOG1)->PHY_Src;            \
	(LOG2)->PHY_Dst       = (LOG1)->PHY_Dst;            \
	(LOG2)->BurstLen      = (LOG1)->BurstLen;           \
	(LOG2)->DropReason    = (REASON);                 \
	(LOG2)->Channel       = (LOG1)->Channel;            \
	(LOG2)->BurstType     = (LOG1)->BurstType;          \
	(LOG2)->RetryCount    = (LOG1)->RetryCount;         \
}
#endif	/* CONFIG_MobileRelayWIMAX */

#ifdef CONFIG_MR_WIMAX_NT
#define LOG_MAC_802_16j_NT(LOG,TIME,STIME,NODE_TYPE,NODE_ID,EVENT,SRC_ID,DST_ID,  \
                        B_TYPE,B_LEN,CHANNEL)                      \
{                                                                       \
	(LOG)->PROTO      = PROTO_802_16j_NT;                                   \
	(LOG)->Time       = (TIME);                                         \
	(LOG)->sTime      = (STIME);                                        \
	(LOG)->NodeType   = (NODE_TYPE);                                    \
	(LOG)->NodeID     = (NODE_ID);                                      \
	(LOG)->Event      = (EVENT);                                        \
	(LOG)->IP_Src     = 0;                                            \
	(LOG)->IP_Dst     = 0;                                            \
	(LOG)->PHY_Src    = (SRC_ID);                                       \
	(LOG)->PHY_Dst    = (DST_ID);                                       \
	(LOG)->RetryCount = 0;                                            \
	(LOG)->BurstType  = (B_TYPE);                                       \
	(LOG)->BurstLen   = (B_LEN);                                        \
	(LOG)->DropReason = DROP_NONE;                                \
	(LOG)->Channel    = (CHANNEL);                                      \
}

#define DROP_LOG_802_16j_NT(LOG1,LOG2,TIME1,EVENT,REASON)            \
{                                                               \
	(LOG2)->PROTO         = PROTO_802_16j_NT;             \
	(LOG2)->Time          = (TIME1);                     \
	(LOG2)->sTime         = (LOG1)->sTime;              \
	(LOG2)->NodeType      = (LOG1)->NodeType;           \
	(LOG2)->NodeID        = (LOG1)->NodeID;             \
	(LOG2)->Event         = (EVENT);                    \
	(LOG2)->IP_Src        = (LOG1)->IP_Src;             \
	(LOG2)->IP_Dst        = (LOG1)->IP_Dst;             \
	(LOG2)->PHY_Src       = (LOG1)->PHY_Src;            \
	(LOG2)->PHY_Dst       = (LOG1)->PHY_Dst;            \
	(LOG2)->BurstLen      = (LOG1)->BurstLen;           \
	(LOG2)->DropReason    = (REASON);                 \
	(LOG2)->Channel       = (LOG1)->Channel;            \
	(LOG2)->BurstType     = (LOG1)->BurstType;          \
	(LOG2)->RetryCount    = (LOG1)->RetryCount;         \
}
#endif        /* CONFIG_MR_WIMAX_NT */

#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
#define LOG_SAT_DVBRCS(LOG,TIME,STIME,NODE_TYPE,NODE_ID,EVENT,SRC_ID,DST_ID,  \
                        B_ID,B_TYPE,B_LEN,CHANNEL,DROP_REA)                      \
{                                                                       \
        LOG->PROTO      = PROTO_SAT_DVBRCS;                                   \
        LOG->Time       = (TIME);                                         \
        LOG->sTime      = (STIME);                                        \
        LOG->NodeType   = (NODE_TYPE);                                    \
        LOG->NodeID     = (NODE_ID);                                      \
        LOG->Event      = (EVENT);                                        \
        LOG->IP_Src     = 0;                                            \
        LOG->IP_Dst     = 0;                                            \
        LOG->PHY_Src    = (SRC_ID);                                       \
        LOG->PHY_Dst    = (DST_ID);                                       \
        LOG->RetryCount = 0;                                            \
        LOG->BurstID    = (B_ID);                                         \
        LOG->BurstType  = (B_TYPE);                                       \
        LOG->BurstLen   = (B_LEN);                                        \
        LOG->DropReason = DROP_REA;                                    \
        LOG->Channel    = (u_char)CHANNEL;                              \
}
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */


#endif /*__NCTUNS_log_mcaro_h__*/
