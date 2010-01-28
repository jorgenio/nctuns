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

#ifndef __NCTUNS_mac_802_11_e_h__
#define __NCTUNS_mac_802_11_e_h__

#include <object.h>
#include <tcp.h>
#include <udp.h>
#include <event.h>
#include <timer.h>
#include <nctuns_api.h>
#include <mac-802_11e.h>
#include <math.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>

#include <mac/PriQueue.h>

#define MAX_QUEUE_LEN	50	/* Max 50 packets in a queue */
#define MAX_MSDU_SIZE   2304	/* Max MSDU size */

#define NO_ACTION	0x0
#define DATA_FLOW	0x1

#define AC_BE	0
#define AC_BK	1
#define AC_VI	2
#define AC_VO	3

#define AC_NUM	4	/* Total number of Access Categories */

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

	MAC_BEACON	= 0x0200,
	MAC_HCCA_SEND	= 0x0040,

	MAC_RESET	= 0xffff	
};

/*========================================================================
   Define HCCA State
  ========================================================================*/
enum HCCA_State {
        HCCA_LOST_CONTROL   = 0,
        HCCA_HAS_CONTROL    = 1,
        HCCA_SUCCESS        = 2,
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
 
struct PHY_MIB {
	// agPhyOperationGroup
	u_char			aPHYType;
	u_char			*aRegDomainsSupported;
	u_int32_t		acurrentRegdomain;
	u_int32_t		aSlotTime;
	u_int32_t		aCCATime;
	u_int32_t		aTxRxTurnaroundTime;
	u_int32_t		aTxPLCPDelay;
	u_int32_t		aRxTxSwitchTime;
	u_int32_t		aTxRampOnTime;
	u_int32_t		aTxRFDelay;
	u_int32_t		aSIFSTime;
	u_int32_t		aRxRFDelay;
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

#define MAC80211E_HDR_LEN						\
	 (sizeof(struct hdr_mac802_11e))

#define MAC80211_RTS_LEN	 					\
	 (sizeof(struct rts_frame))
	 
#define MAC80211_CTS_LEN						\
	 (sizeof(struct cts_frame))
	 
#define MAC80211E_ACK_LEN						\
	 (sizeof(struct ack_frame))

/*
 * Note XXX:
 *	- Time unit here is microsecond.
 */
#define PRM_Time							\
	((u_int32_t)rintf((MAC80211_PREAMBLE*8.0)/1.0))
#define RTS_Time	 						\
	((u_int32_t)(rintf(((MAC80211_RTS_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define CTS_Time							\
	((u_int32_t)(rintf(((MAC80211_CTS_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define ACK_Time							\
	((u_int32_t)(rintf(((MAC80211E_ACK_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define RX_Time(len, senderBW)						\
	((u_int32_t)(rintf((len*8000000.0)/(senderBW)+.5))+PRM_Time) 


/*
 * IEEE 802.11 Spec, section 9.2.5.7
 *	- After transmitting an RTS, a node waits CTSTimeout
 *
 * Note XXX:
 *	- Time unit here is microsecond.
 */
#define PRM_Time							\
	((u_int32_t)rintf((MAC80211_PREAMBLE*8.0)/1.0))
#define RTS_Time	 						\
	((u_int32_t)(rintf(((MAC80211_RTS_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define CTS_Time							\
	((u_int32_t)(rintf(((MAC80211_CTS_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define ACK_Time							\
	((u_int32_t)(rintf(((MAC80211E_ACK_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define RX_Time(len, senderBW)						\
	((u_int32_t)(rintf((len*8000000.0)/(senderBW)+.5))+PRM_Time) 


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
#define CTSTimeout	((RTS_Time + CTS_Time) + 2 * sifs)
#define NAVTimeout   	(2 * sifs + phymib->aSIFSTime + CTS_Time + 2 * phymib->aSlotTime)	//check
#define ACKTimeout(len,pri) 	(TX_Time(len) + ACK_Time + sifs + aifs[pri])

#define RTS_DURATION(len)  (sifs + CTS_Time + sifs + TX_Time(len) + sifs + ACK_Time)
#define CTS_DURATION(d)	   (d - (CTS_Time + sifs))
#define DATA_DURATION()	   (ACK_Time + sifs)

/*========================================================================
   Cache structure for duplicated detection
  ========================================================================*/
struct dup_cache {
	int			aclevel; /* TID */
	u_int64_t		timestamp;
	u_char			src[ETHER_ADDR_LEN];
	u_int16_t		sequno; /* sequence NO. and fragment NO. */
};
#define CACHE_SIZE		20

struct fragBuf {
	ePacket_		*ep; 
	struct fragBuf		*nextf; 
}; 


/*========================================================================
   Internal HCCA Status 
  ========================================================================*/
struct HCCA_Status {

   bool 	hcca_on;	/* hcca is on */
   bool 	has_control;	/* the QSTA or QAP has the control of the medium */
   bool		need_ack;	/* receiving a DATA frame, an ACK is needed. */
   bool         cap_expire;	
   int 		ack_tid;	/* TID of the next received ACK */
   u_char 	ack_dst[6];	/* destination of the next received ACK */
   bool 	hcca_tx_active; /* distinguish between Contention-based and Contention-free transmissions */
   u_int64_t	sense_idle;	/* the time for which the medium must be sensed idle before transmitting */
   u_int64_t 	idle_time;	/* last time that the medium was sensed idle */

};

class HCCA_TxP {
  public:
	bool		getP;	/* success: get packet */
   	int 		tsid;	/* Traffic Stream Identifier */
   	int 		qSize;  /* Queue Size QSTA only in bytes */
 	bool 		eosp;
   	u_int64_t 	txop;  	/* QAP Only */
   	u_int16_t 	dura; 	/* A QSTA uses it to override the NAV setting */
   	u_char 		subtype;
   	u_char 		ackP;
   	u_char 		dst[6];
   	ePacket_ 	*txp;	/* packet to transmit */

	HCCA_TxP() {
		getP 	= false;
      		eosp 	= false;
     	 	ackP 	= 0;
     	 	subtype = 0;
      		tsid 	= 0;
      		txop 	= 0;
      		qSize 	= 0;
      		dura 	= 0;
      		txp 	= 0;
   	}

 	~HCCA_TxP() {
		if(txp)
			freePacket(txp);
   	}

   	void reset() {
		getP 	= false;
      		eosp 	= false;
     	 	ackP 	= 0;
     	 	subtype = 0;
      		tsid 	= 0;
      		txop 	= 0;
      		qSize 	= 0;
      		dura 	= 0;
		if(txp)
			freePacket(txp);
		txp = 0;
  	}

};

enum dir_{
        UPLINK          = 0x01,
        DOWNLINK        = 0x02
};

enum pro_{
        UDP             = 0x01,
        TCP             = 0x02
};

enum ADDTS_status{
        INACTIVE        = 0x00,
        REQUESTING      = 0x01,
        ACTIVE          = 0x02,
        DECLINED        = 0x03
};

struct TSPEC {
        u_char		tsid;                   /* Traffic Stream Identifier */
        dir_		direction;              /* TS Direction */
        pro_		protocol;               /* Transmission Protocol */
        char		qsta[6];
        u_int32_t	mean_data_rate;       /* Mean Data Rate (KB/s)*/
        u_int32_t	nominal_MSDU_size;    /* Nominal MSDU Size (bytes) */
        u_int32_t	max_MSDU_size;        /* Maximum MSDU Size (bytes) */
        u_int32_t	max_SI;               /* Maximum Service Interval (ms) */
	u_int32_t	delay_bound;		/* Delay Bound */
        u_int32_t	stime;                /* Start Time (sec) */
        u_int32_t	etime;                /* End Time (sec) */
        u_int32_t	sport;                /* Source Port */
        u_int32_t	dport;                /* Destination Port */
        struct TSPEC 	*next;
        enum ADDTS_status Active;       /* register Traffic Stream to HC */
};

class QosScheduler;

/*========================================================================
   Class IEEE 802.11e 
  ========================================================================*/
class mac802_11e : public NslObject {

 private :
	int			*ch_;
        int			*isQAP;
	bool			QBSS;

	u_char			_ptrlog;
	u_char			action;
	
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

	/* mac timer used by IEEE 802.11
	 */
	timerObj		*mhNav;		/* NAV timer */
	timerObj		*mhSend;	/* send timer */
	timerObj		*mhSIFS;	/* ACK timer */
	timerObj		*mhDefer[AC_NUM];	/* defer timer */
	timerObj		*mhBackoff[AC_NUM];	/* backoff timer */
	timerObj		*mhIF;		/* interface timer */
	timerObj		*mhRecv;  	/* receive timer */

	/* HCCA timers */
	timerObj		*mhCAP;
	timerObj		*mhHCCATx;

	u_int32_t		ssrc; 			/* Short Retry Count */ 
	u_int32_t		slrc;			/* Long Retry Count */

	u_int64_t		edca_txop;	/* A QSTA gets CP's txop, edca_txop = now + txopLimit[ac] */
	
	u_int64_t		nav;		/* network allocation vector */
	u_int16_t		sifs; 		/* Short Interface Frame */
	u_int16_t		pifs; 		/* PCF Interface Frame */
	u_int16_t		difs;		/* DCF Interface Frame */
	u_int16_t		eifs; 		/* Extended Interface Frame */
	u_int16_t		aifs[AC_NUM];	/* Arbitration interframe space */
	u_int16_t		aifsn[AC_NUM];	/* AIFS Number */

	struct MAC_MIB		*macmib; 	/* MAC MIB of IEEE 802.11 */
	
	struct PriQueue		pq_[AC_NUM];	/* Queues for Priority traffic */
	struct HCCA_Status	hcca_stat;	/* Current HCCA Status */

	QosScheduler		*qs;		/* Scheduler for Parameter traffic */
	HCCA_TxP		hccaTx;		/* HCCA Transmission buffer */
		
	ePacket_		*epktRx;		/* incoming packet buffer */
	ePacket_		*epktTx;		/* outgoing packet buffer */
	ePacket_		*epktCTRL;		/* CTS/ACK send buffer */
	ePacket_	 	*epktRTS[AC_NUM];    	/* RTS send buffer */
	ePacket_		*epktDATA[AC_NUM]; 	/* tmp outgoing packet buffer of every AC */
	struct fragBuf		*epktBuf; 		/* fragmentation buffer */
	
	double			*CPThreshold_; 
	
	enum MacState		rx_state; 
	enum MacState		tx_state;
	u_char			tx_active;

	u_int16_t 		sta_seqno; 
	struct dup_cache	cache[CACHE_SIZE];

	char			*qosmode;
	int 			plevel;			/* User's QoS priority */
	int			now_ac;			/* Access category of current transmitting packet, in EDCA */
	u_int8_t		txopLimit[AC_NUM];	/* TxopLimit  unit is 32-usec. */	
        u_int16_t       	cw[AC_NUM];            	/* Contention Window */
	u_int16_t       	CWmin[AC_NUM];         	/* Contention Window Min (from PHY) */
        u_int16_t      		CWmax[AC_NUM];         	/* Contention Window Max (from PHY) */
	
	struct TSPEC		*TShead; 	
                                                                                             
	int			init_PHYMIB();
	int			init_MACMIB();
	int			init_Timer();
	void			init_TSPEC();
	int			calcul_IFS();
	
	void 			inc_CW(int);	
	void			rst_CW(int); 
	inline void 		set_NAV(u_int32_t _nav);

	/* functions called by timer */
	int			check_pktCTRL(int);
	int 			check_pktRTS(int);
	int			check_pktTx(int);

	/* timer handler */
	int			navHandler(Event_ *e);
	int			sendHandler(Event_ *e);
	int			sifsHandler(Event_ *e);
	int 			BE_deferHandler(Event_ *e);
	int 			BK_deferHandler(Event_ *e);
	int 			VI_deferHandler(Event_ *e);
	int 			VO_deferHandler(Event_ *e);
	int 			BE_backoffHandler(Event_ *e);
	int 			BK_backoffHandler(Event_ *e);
	int 			VI_backoffHandler(Event_ *e);
	int 			VO_backoffHandler(Event_ *e);
	int			txHandler(Event_ *e);
	int			recvHandler(Event_ *e); 
	void			capHandler(Event_ *e);
	void			hccaTxHandler(Event_ *e);
	
	/* Transmission functions */
	void 			sendRTS(int, u_char *dst);
	void 			sendCTS(u_char *dst, u_int16_t d);
	void			sendACK(u_char *dst);
	void 			sendDATA(int, u_char *dst);
	void			retransmitRTS();
	void			retransmitDATA();
	void			transmit(ePacket_ *p, double t);
	ePacket_*		sendQoSACK(u_char *dst);
	
	/* Reception functions */
	void			recvRTS(ePacket_ *p);
	void			recvCTS(ePacket_ *p);
	void			recvACK(ePacket_ *p); 
	void			recvDATA(ePacket_ *p);

	inline int		is_idle(); 
	void			tx_resume();
	void			rx_resume();
	void			collision(ePacket_ *p);
	void			capture(ePacket_ *p);

	TSPEC*			decide_TID(int proto, struct tcphdr *tcph, struct udphdr *udph, dir_ direction, u_char *addr);
	TSPEC*			find_TSPEC(u_char tsid, dir_ direction);

	void			start_CAP();
	void			start_backoff(int); 
	inline void		start_defer(int,u_int32_t us); 
	inline void		start_hccatx(u_int32_t us); 
	inline void		start_sifs(u_int32_t us); 
	inline void		start_sendtimer(u_int32_t us);
	inline void  		start_recvtimer(u_int32_t us);
	inline void  		start_IFtimer(u_int32_t us);


	inline int UPtoAC(int pri){                    
        	if(pri > 3)       return (pri >> 1);  
        	else if(pri == 0 || pri == 3)   return 1;                              
        	else return  0;                                              
	}
	int			update_dcache(u_char *mac, u_int16_t sc); 
	int			Fragment(ePacket_ *pkt, u_char *dst);

	void			control_medium(bool set);
	void			check_idle(void);

	void			GenerateADDTSReq(TSPEC *ts);	/* Handle Traffic Stream Function */
	void			recvADDTSResp(ePacket_ *pkt);
	void			ProcessADDTS(ePacket_ *pkt);
	void			ProcessDELTS(ePacket_ *pkt);

	u_int8_t		bytes_to_queue(int bytes);

 public:
	struct PHY_MIB		*phymib; 	/* PHY MIB of IEEE 802.11 */
	u_char			mac_[6]; 
	u_char			ap_mac[6];
	u_int64_t		last_beacon;	
	int			near_beacon;
        int			*Beacon_Timeval;
	double			*bw_; 
 
	mac802_11e(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~mac802_11e();
	
	void			drop(ePacket_ *p, char *why); 
	int			init();
	int			recv(ePacket_ *pkt);
	int			send(ePacket_ *pkt);
	int			log();
	u_int64_t 		TX_Time(int len){ return (u_int64_t)(rintf((len*8000000.0)/(*bw_)+.5))+PRM_Time; }
}; 

#endif /* __NCTUNS_mac_802_11_e_h__ */
