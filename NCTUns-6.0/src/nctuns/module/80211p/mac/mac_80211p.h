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

#ifndef __NCTUNS_mac_80211p_h__
#define __NCTUNS_mac_80211p_h__

#include <tcp.h>
#include <udp.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <nctuns_api.h>
#include <mac-802_11.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>
/*for cross-module function call*/
#include <mbinder.h>
#include <80211p/phy/ofdm_80211p.h>

#define MAX_QUEUE_LEN	50	/* Max 50 packets in a queue */
#define MAX_MSDU_SIZE   2304	/* Max MSDU size */
#define AC_BE	0
#define AC_BK	1
#define AC_VI	2
#define AC_VO	3

#define AC_NUM 4 /* Total number of Access Categories */ 
#define CH_NUM 2 /* 2 channels ( CCH & SCH ) */
#define IS_CH_SWITCH_PAUSED CH_status
/* kcliu added
   queue implementaiton
*/
/* Define Interface Queue for every Interface */
struct ifqueue {
	ePacket_		*ifq_head; /* head of ifq */
	ePacket_		*ifq_tail; /* tail of ifq */
	int			ifq_pri;
	int			ifq_len;   /* current queue length */
	int			ifq_maxlen;/* max queue length */
	int			ifq_drops; /* drops count */
};
/* Define Macros for IFq */
#define IF_QFULL(ifq)           ((ifq)->ifq_len >= (ifq)->ifq_maxlen)
#define IF_DROP(ifq)            ((ifq)->ifq_drops++)
#define IF_ENQUEUE(ifq, m) { \
        if ((ifq)->ifq_tail == 0) \
                (ifq)->ifq_head = m; \
        else \
                (ifq)->ifq_tail->next_ep = m; \
        (ifq)->ifq_tail = m; \
        (ifq)->ifq_len++; \
}
#define IF_PREPEND(ifq, m) { \
        (m)->next_ep = (ifq)->ifq_head; \
        if ((ifq)->ifq_tail == 0) \
                (ifq)->ifq_tail = (m); \
        (ifq)->ifq_head = (m); \
        (ifq)->ifq_len++; \
}
#define IF_DEQUEUE(ifq, m) { \
        (m) = (ifq)->ifq_head; \
        if (m) { \
                if (((ifq)->ifq_head = (m)->next_ep) == 0) \
                        (ifq)->ifq_tail = 0; \
                (m)->next_ep = 0; \
                (ifq)->ifq_len--; \
        } \
}
//end

/*========================================================================
   Define Misc. Data Structure
  ========================================================================*/
enum MacState {
	MAC_IDLE	= 0x0000,
	MAC_RECV	= 0x0001,
	MAC_SEND	= 0x0002,
	MAC_RTS		= 0x0004,
	MAC_CTS		= 0x0008,
	MAC_ACK 	= 0x0010,
	MAC_COLL	= 0x0020,
};
enum Channel_State 
{
	state_CCH = 0,
	state_SCH = 1,
};
enum PHY_State 
{
	state_IDLE = 0,
	state_BUSY = 1,
};
	

/*========================================================================
   Define MAC/PHY MIB Data Structure
  ========================================================================*/
struct Mag_Attr {
	// agStationConfiggrp
	u_char			*aStationID;
	u_int32_t		aMediumOccupancyLimit;
	u_char			aCFPollable;
	u_int32_t		aAuthenticationType;
	u_int32_t		aAuthenticationAlgorithms;
	u_int32_t		aCFPPeriod;
	u_int32_t		aCFPMaxDuration;
	u_int32_t		aAuthenticationResponseTimeout;
	u_int32_t		aWEPUndecryptableCount;
	u_int32_t		aReceiveDTIMs;
	
	// agPrivacygrp
	u_char			aPrivacyOptionImplemented;
	u_char 			aPrivacyInvoked;
	u_int32_t 		aWEPDefaultKeys;
	u_int32_t 		aWEPDefaultKeyID;
	u_int32_t		aWEPKeyMappings;
	u_int32_t 		aWEPKeyMappingLength;
	u_char 			aExcludeUnencrypted;
	u_int32_t 		aWEPICVErrorCount;
	u_int32_t 		aWEPExcludedCount;
};  

struct MAC_Attr {
	// agOperationgrp
	u_char			aMACAddress[6];
	//u_char			aGroupAddress[6];
	u_int32_t		aRTSThreshold;
	u_int16_t 		aShortRetryLimit;
	u_int16_t 		aLongRetryLimit;
	u_int32_t 		aFragmentationThreshold;
	u_int32_t 		aMaxTransmitMSDULifetime;
	u_int32_t 		aMaxReceiveLifetime;
	//u_char			*aManufacturerID;
	//u_char			*aProductID;
	
	//agCountersgrp
	u_int32_t		aTransmittedFragmentCount;
	u_int32_t 		aMulticastTransmittedFramecount;
	u_int32_t 		aFailedCount;
	u_int32_t 		aRetryCount;
	u_int32_t 		aMultipleRetryCount;
	u_int32_t		aFrameDuplicateCount;
	u_int32_t	 	aRTSSuccessCount;
	u_int32_t		aRTSFailureCount;
	u_int32_t 		aACKFailureCount;
	u_int32_t 		aReceivedFragmentCount;
	u_int32_t 		aMulticastReceivedFrameCount;
	u_int32_t 		aFCSErrorCount; 
};  

struct ResType_Attr {
	u_char			*aResourceTypeIDName;
	u_char			*aResourceInfo;
};

struct Notify_Attr {
	u_char			*nDisassociate; 
};



struct MAC_MIB {
	struct Mag_Attr		Mag_Attr_;
	struct MAC_Attr 	MAC_Attr_;
	//struct ResType_Attr	RegType_Attr_; 
	//struct Notify_Attr	Notify_Attr_;  
};
 
struct PHY_MIB_IN_MAC {
	// agPhyOperationGroup
	u_char			aPHYType;
	u_char			*aRegDomainsSupported;
	u_int32_t		acurrentRegdomain;
	u_int32_t		aSlotTime;
	u_int32_t		aCCATime;
	u_int32_t		aRxTxTurnaroundTime;
	u_int32_t		aTxPLCPDelay;
	u_int32_t		aRxTxSwitchTime;
	u_int32_t		aTxRampOnTime;
	u_int32_t		aTxRFDelay;
	u_int32_t		aSIFSTime;
	u_int32_t 		aRxRFDelay;
	u_int32_t 		aRxPLCPDelay;
	u_int32_t		aMACProcessingDelay;
	u_int32_t		aTxRampOffTime;
	u_int32_t		aPreambleLength;
	u_int32_t		aPLCPHeaderLength;
	u_int32_t		aMPDUDurationFactor;
	u_int32_t 		aAirPropagationTime;
	u_int32_t		aTempType;
	u_int16_t 		aCWmin;
	u_int16_t 		aCWmax;
	
	// agPhyRateGroup
	u_char			*aSupportedDataRatesTx;
	u_char			*aSupportedDataRatesRx;
	u_int32_t 		aMPDUMaxLength;
	
	// agPhyAntennaGroup
	u_int32_t 		aCurrentTxAntenna;
	u_int32_t		aDiversitySupport;
	
	// agPhyTxPowerGroup
	u_int32_t 		aNumberSupportedPowerLevels;
	u_int32_t 		aTxPowerLevel1;
	u_int32_t               aTxPowerLevel2;
	u_int32_t               aTxPowerLevel3;
	u_int32_t               aTxPowerLevel4;
	u_int32_t               aTxPowerLevel5;
	u_int32_t               aTxPowerLevel6;
	u_int32_t               aTxPowerLevel7;
	u_int32_t               aTxPowerLevel8; 
	u_int32_t		aCurrentTxPowerLevel;
	
	// agPhyFHSSGroup
	//u_int32_t		aHopTime;
	//u_int32_t 		aCurrentChannelNumber;
	//u_int32_t		aMaxDwellTime;
	//u_int32_t 		aCurrentSet;
	//u_int32_t 		aCurrentPattern;
	//u_int32_t 		aCurrentIndex;

	// agPhyDSSSGroup
	u_int32_t		aCurrentChannel;
	u_int32_t 		aCCAModeSupported;
	u_int32_t 		aCurrentCCAMode;
	u_int32_t		aEDThreshold;
	
	// agPhyIRGroup
	//u_int32_t		aCCAWatchdogTimerMax;
	//u_int32_t 		aCCAWatchdogCountMax;
	//u_int32_t 		aCCAWatchdogTimerMin;
	//u_int32_t 		aCCAWatchdogCountMin;
	
	// agPhyStatusGroup
	u_int32_t		aSynthesizerLocked;
	
	// agPhyPowerSavingGroup
	u_int32_t 		aCurrentPowerState;
	u_int32_t 		aDozeTurnonTime;
	
	// agAntennasList
	//u_int32_t 		*aSupportedTxAntennas;
	//u_int32_t 		*aSupportedRxAntennas;
	//u_int32_t	 	*aDiversitySelectionRx; 
};  
 
 
#define aStationID			Mag_Attr_.aStationID
#define aMediumOccupancyLimit		Mag_Attr_.aMediumOccupancyLimit
#define aCFPollable			Mag_Attr_.aCFPollable
#define aAuthenticationType		Mag_Attr_.aAuthenticationType
#define aAuthenticationAlgorithms	Mag_Attr_.aAuthenticationAlgorithms
#define aCFPPeriod			Mag_Attr_.aCFPPeriod
#define aCFPMaxDuration			Mag_Attr_.aCFPMaxDuration
#define aAuthenticationResponseTimeout	Mag_Attr_.aAuthenticationResponseTimeout
#define aWEPUndecryptableCount		Mag_Attr_.aWEPUndecryptableCount
#define aReceiveDTIMs			Mag_Attr_.aReceiveDTIMs
#define aPrivacyOptionImplemented	Mag_Attr_.aPrivacyOptionImplemented
#define aPrivacyInvoked			Mag_Attr_.aPrivacyInvoked
#define aWEPDefaultKeys			Mag_Attr_.aWEPDefaultKeys
#define aWEPDefaultKeyID		Mag_Attr_.aWEPDefaultKeyID
#define aWEPKeyMappings			Mag_Attr_.aWEPKeyMappings
#define aWEPKeyMappingLength		Mag_Attr_.aWEPKeyMappingLength
#define aExcludeUnencrypted		Mag_Attr_.aExcludeUnencrypted
#define aWEPICVErrorCount		Mag_Attr_.aWEPICVErrorCount
#define aWEPExcludedCount		Mag_Attr_.aWEPExcludedCount

#define aMACAddress			MAC_Attr_.aMACAddress
#define aGroupAddress			MAC_Attr_.aGroupAddress
#define aRTSThreshold			MAC_Attr_.aRTSThreshold
#define aShortRetryLimit		MAC_Attr_.aShortRetryLimit
#define aLongRetryLimit			MAC_Attr_.aLongRetryLimit
#define aFragmentationThreshold		MAC_Attr_.aFragmentationThreshold
#define aMaxTransmitMSDULifetime	MAC_Attr_.aMaxTransmitMSDULifetime
#define aMaxReceiveLifetime		MAC_Attr_.aMaxReceiveLifetime
#define aManufacturerID			MAC_Attr_.aManufacturerID
#define aProductID			MAC_Attr_.aProductID
#define aTransmittedFragmentCount	MAC_Attr_.aTransmittedFragmentCount
#define aMulticastTransmittedFramecount	MAC_Attr_.aMulticastTransmittedFramecount
#define aFailedCount			MAC_Attr_.aFailedCount
#define aRetryCount			MAC_Attr_.aRetryCount
#define aMultipleRetryCount		MAC_Attr_.aMultipleRetryCount
#define aFrameDuplicateCount		MAC_Attr_.aFrameDuplicateCount
#define aRTSSuccessCount		MAC_Attr_.aRTSSuccessCount
#define aRTSFailureCount		MAC_Attr_.aRTSFailureCount
#define aACKFailureCount		MAC_Attr_.aACKFailureCount
#define aReceivedFragmentCount		MAC_Attr_.aReceivedFragmentCount
#define aMulticastReceivedFrameCount	MAC_Attr_.aMulticastReceivedFrameCount
#define aFCSErrorCount			MAC_Attr_.aFCSErrorCount
#define aResourceTypeIDName		ResType_Attr_.aResourceTypeIDName
#define aResourceInfo			ResType_Attr_.aResourceInfo
#define nDisassociate			Notify_Attr_.nDisassociate




  
/*========================================================================
   Define Macros
  ========================================================================*/
#define MAC80211_PREAMBLE						\
	((phymib->aPreambleLength >> 3) +				\
	 (phymib->aPLCPHeaderLength >> 3))

#define MAC80211_HDR_LEN						\
	 (sizeof(struct hdr_mac802_11))

#define MAC80211_RTS_LEN	 					\
	 (sizeof(struct rts_frame))
	 
#define MAC80211_CTS_LEN						\
	 (sizeof(struct cts_frame))
	 
#define MAC80211_ACK_LEN						\
	 (sizeof(struct ack_frame))
	 
/*
 * Note XXX:
 *	- Time unit here is microsecond.
 */

//hychen modified
#define PRM_Time							\
	(u_int32_t)(32+8)
#define RTS_Time(nid,pid)                                                       \
        (u_int32_t)(rintf(((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->TXTIME(MAC80211_RTS_LEN,*bw_)+0.5))
#define CTS_Time(nid,pid)                                                       \
        (u_int32_t)(rintf(((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->TXTIME(MAC80211_CTS_LEN,*bw_)+0.5))
#define ACK_Time(nid,pid)                                               \
        (u_int32_t)(rintf(((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->TXTIME(MAC80211_ACK_LEN,*bw_)+0.5))
#define TX_Time(len,bw,nid,pid)                                                 \
        (u_int32_t)(rintf(((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->TXTIME(len,bw)+0.5))
#define RX_Time(len, senderBW)                                          \
        ((u_int32_t)(rintf((len*8000000.0+22)/(senderBW)+.5))+PRM_Time)



/*
 * IEEE 802.11 Spec, section 9.2.5.7
 *	- After transmitting an RTS, a node waits CTSTimeout
 *	  for a CTS.
 *
 * IEEE 802.11 Spec, section 9.2.8
 *	- After transmitting an DATA, a node waits ACKTimeout
 *	  for a ACK.
 *
 * IEEE 802.11 Spec, section 9.2.5.4
 *	- After hearing an RTS, a node waits NAVTimeout
 *	  before resetting its NAV.
 *
 */
//hychen modified
#define CTSTimeout(nid,pid)     ((RTS_Time(nid,pid) + CTS_Time(nid,pid)) + 2 * sifs)
#define ACKTimeout(len,pri,nid,pid)     (TX_Time(len,(*bw_),nid,pid) + ACK_Time(nid,pid) + sifs+aifs[pri])
#define NAVTimeout      (2 * sifs + phymib->aSIFSTime + CTS_Time + 2 * phymib->aSlotTime)


#define RTS_DURATION(len,nid,pid)  (sifs + CTS_Time(nid,pid) + sifs + TX_Time(len,(*bw_),nid,pid) + sifs + ACK_Time(nid,pid))
#define CTS_DURATION(d,nid,pid)    (d - (CTS_Time(nid,pid) + sifs))
#define DATA_DURATION(nid,pid)     (ACK_Time(nid,pid) + sifs)
#define ACK_DURATION()     0x00


/*========================================================================
   Cache structure for duplicated detection
  ========================================================================*/
struct dup_cache {
	u_int64_t		timestamp;
	u_char			src[ETHER_ADDR_LEN];
	u_int16_t		sequno; /* sequence NO. and fragment NO. */
};
#define CACHE_SIZE		20

struct fragBuf {
	ePacket_			*ep; 
	struct fragBuf		*nextf; 
}; 

/*========================================================================
   Class IEEE 802.11p
  ========================================================================*/
class mac_80211p : public NslObject {

 private :
	bool			interrupt_CH;
	struct ifqueue		if_snd[AC_NUM][CH_NUM]; /* output interface queue */
	
	u_char etherbroadcastaddr[6];
	u_long		*mytunidp;
	double			*bw_; 
	u_char			mac_[6]; 
	int			*ch_;
	u_char			_ptrlog;

	char			*promiscuous_bind;
	char			promiscuous;

	char			*_log;
	double			logInterval;
	u_int64_t		logIntervalTick;
	timerObj		logTimer;
	
	char			*LogUniIn;
	char			*UniInLogFileName;
	FILE			*UniInLogFILE;
	u_int64_t		NumUniIn;

	char			*LogUniOut;
	char			*UniOutLogFileName;
	FILE			*UniOutLogFILE;
	u_int64_t		NumUniOut;

	char			*LogUniInOut;
	char			*UniInOutLogFileName;
	FILE			*UniInOutLogFILE;
	u_int64_t		NumUniInOut;

	char			*LogBroIn;
	char			*BroInLogFileName;
	FILE			*BroInLogFILE;
	u_int64_t		NumBroIn;

	char			*LogBroOut;
	char			*BroOutLogFileName;
	FILE			*BroOutLogFILE;
	u_int64_t		NumBroOut;

	char			*LogBroInOut;
	char			*BroInOutLogFileName;
	FILE			*BroInOutLogFILE;
	u_int64_t		NumBroInOut;

	char			*LogColl;
	char			*CollLogFileName;
	FILE			*CollLogFILE;
	u_int64_t		NumColl;

	char			*LogDrop;
	char			*DropLogFileName;
	FILE			*DropLogFILE;
	u_int64_t		NumDrop;

	char			*LogInThrput;
	char			*InThrputLogFileName;
	FILE			*InThrputLogFILE;
	double			InThrput;

	char			*LogOutThrput;
	char			*OutThrputLogFileName;
	FILE			*OutThrputLogFILE;
	double			OutThrput;

	char			*LogInOutThrput;
	char			*InOutThrputLogFileName;
	FILE			*InOutThrputLogFILE;
	double			InOutThrput;

	int 			plevel[8];		/* User's QoS priority */

	bool			deferFlag[AC_NUM];
	
	/* mac timer used by IEEE 802.11
	 */
	timerObj		*mhNav;		/* NAV timer */
	timerObj		*mhSend;	/* send timer */
	timerObj		*mhDefer[AC_NUM];	/* defer timer */
	timerObj		*mhBackoff[AC_NUM][CH_NUM];	/* backoff timer */
	timerObj		*mhIF;		/* interface timer */
	timerObj		*mhRecv;  	/* receive timer */
	timerObj		*mhSIFS;
	timerObj		*mhCHstate;	//SSH & CCH switch timer

	u_int64_t		edca_txop;	/* A QSTA gets CP's txop, edca_txop = now + txopLimit[ac] */
	u_int64_t		nav;		/* network allocation vector */
	u_int16_t		ssrc;		/* STA Short Retry Count */
	u_int16_t		slrc;		/* STA Long Retry Count */
	u_int16_t		sifs; 		/* Short Interface Frame */
	u_int16_t		pifs; 		/* PCF Interface Frame */
	u_int16_t		difs;		/* DCF Interface Frame */
	u_int16_t		eifs; 		/* Extended Interface Frame */
	u_int16_t		aifs[AC_NUM];	/* Arbitration interframe space */
	u_int16_t		aifsn[AC_NUM][CH_NUM];	/* AIFS Number */
	int				now_ac[CH_NUM];		/* Access category of current transmitting packet, in EDCA */
	u_int8_t		txopLimit[AC_NUM];	/* TxopLimit  unit is 32-usec. */	
    u_int16_t       cw[AC_NUM];            	/* Contention Window */
	u_int16_t       CWmin[AC_NUM][CH_NUM];         	/* Contention Window Min (from PHY) */
    u_int16_t      	CWmax[AC_NUM][CH_NUM];         	/* Contention Window Max (from PHY) */
	struct MAC_MIB		*macmib; 	/* MAC MIB of IEEE 802.11 */
	struct PHY_MIB_IN_MAC		*phymib; 	/* PHY MIB of IEEE 802.11 */
	
	ePacket_		*epktRTS[AC_NUM][CH_NUM];	/* RTS send buffer */
	ePacket_		*epktDATA[AC_NUM][CH_NUM];
	ePacket_		*epktRx;	/* incoming packet buffer */
	struct fragBuf		*epktBuf; 	/* fragmentation buffer */
	
	double			*CPThreshold_; 
	
	enum MacState		tx_state[CH_NUM];
	u_char			tx_active[CH_NUM];
	int 			service_channel_num;
	bool 			tx_suspend;
	bool			CH_state;	//real CCH or SCH flag
	bool 			CH_background_state; // should be which state 
	bool			CH_pause;   //pause channel switching timer flag
	bool			SCH_norecv; //SCH pkt flow count flag
	bool 			CH_status;
	bool			cancel_WBSS;//WBSS cancel mechanism flag(on or off)
	u_int32_t		current_provider; //store provider's PSID

	u_int8_t		channel_interval;
	u_int64_t		channel_interval_tick;
	
	u_int16_t 		sta_seqno; 
	struct dup_cache	cache[CACHE_SIZE];
	
	int				init_PHYMIB();
	int				init_MACMIB();
	int				init_Timer();
	int				calcul_IFS();
	
	void 			inc_CW(int);
	void 			rst_CW(int); 
	inline void 	set_NAV(u_int32_t _nav); 

	/* functions called by timer */
	int				check_pktCTRL(int);
	int 			check_pktRTS(int);
	int				check_pktTx(int);

	/* timer handler */
	int				navHandler(Event_ *e);
	int				sendHandler(Event_ *e);
	int				sifsHandler(Event_ *e);
	int				BE_deferHandler(Event_ *e);  //EDCA start
	int				BK_deferHandler(Event_ *e);  
	int				VI_deferHandler(Event_ *e);  
	int				VO_deferHandler(Event_ *e);  
	int				SCH_BE_backoffHandler(Event_ *e); 
	int				SCH_BK_backoffHandler(Event_ *e);
	int				SCH_VI_backoffHandler(Event_ *e);
	int				SCH_VO_backoffHandler(Event_ *e); //EDCA end
	int				CCH_BE_backoffHandler(Event_ *e); 
	int				CCH_BK_backoffHandler(Event_ *e);
	int				CCH_VI_backoffHandler(Event_ *e);
	int				CCH_VO_backoffHandler(Event_ *e); //EDCA end
	int				txHandler(Event_ *e);
	int				recvHandler(Event_ *e); 
	int				CHstateHandler(Event_ *e);
		
	/* Transmission functions */
	void 			sendRTS(int, u_char *dst);
	void 			sendCTS(u_char *dst, u_int16_t d);
	void			sendACK(u_char *dst);
	void 			sendDATA(int, u_char *dst);
	void			retransmitRTS();
	void			retransmitDATA();
	void			transmit(ePacket_ *p, u_int32_t); 
	
	/* Reception functions */
	void			recvRTS(ePacket_ *p);
	void			recvCTS(ePacket_ *p);
	void			recvACK(ePacket_ *p); 
	void			recvDATA(ePacket_ *p);
	
	inline int		is_idle(); 
	void			tx_resume();
	void			drop(ePacket_ *p, char *why); 
	void			start_backoff(int); 
	inline void		start_defer(int,u_int32_t us); 
	inline void		start_sifs(u_int32_t us); 
	inline void		start_sendtimer(u_int32_t us);
	inline void  	start_recvtimer(u_int32_t us);
	inline void  	start_IFtimer(u_int32_t us); 
	
	int 			whichAC(int port);	//EDCA              
	int 			PORTtoUP(int port); //EDCA
	int				update_dcache(u_char *mac, u_int16_t sc); 
	int				Fragment(ePacket_ *pkt, u_char *dst);
	int 			push_buf(ePacket_ *pkt, int ac);//added
	void			triggerSend();
	void 			queue_dump();
	void			channel_dump();
	bool 			PHY_state;
	void			check_if_cancel_WBSS();
	void			check_if_SCH_recv(u_int32_t*);

	unsigned int crc32(unsigned int crc, char *buf, int len);

 public :
 
	mac_80211p(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~mac_80211p();
	
	enum MacState		rx_state; 
	int			init();
	int			recv(ePacket_ *pkt);
	int			send(ePacket_ *pkt);
	int			command(int argc, const char *argv[]);
	int			log();
	void 		set_PHY_state(int st);
	bool 		is_PHY_idle();
	void 		triggerMAC(); // API for PHY module
	void 		start_channel_switch(int, int, u_int32_t);//API for WME module 
	void 		pause_channel_switch();
	void		set_mac_rxstate();
	void			rx_resume();
	ePacket_		*epktCTRL[CH_NUM];	/* CTS/ACK send buffer */
}; 

#endif /* __NCTUNS_mac_802_11_dcf_h__ */
