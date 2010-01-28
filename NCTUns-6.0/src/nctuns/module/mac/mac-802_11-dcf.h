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

/* -*-	Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t -*-
 *
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Header: /nfs/jade/vint/CVSROOT/ns-2/mac/mac-802_11.cc,v 1.39 2002/03/14 01:12:53 haldar Exp $
 *
 * Ported from CMU/Monarch's code, nov'98 -Padma.
 */


#ifndef __NCTUNS_mac_802_11_dcf_h__
#define __NCTUNS_mac_802_11_dcf_h__

#include <object.h>
#include <event.h>
#include <timer.h>
#include <nctuns_api.h>
#include <mac-802_11.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>

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
#define PRM_Time							\
	((u_int32_t)rintf((MAC80211_PREAMBLE*8.0)/1.0))
#define RTS_Time	 						\
	((u_int32_t)(rintf(((MAC80211_RTS_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define CTS_Time							\
	((u_int32_t)(rintf(((MAC80211_CTS_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define ACK_Time							\
	((u_int32_t)(rintf(((MAC80211_ACK_LEN*8000000.0)/(*bw_))+.5))+PRM_Time) 
#define DATA_Time(len)							\
	((u_int32_t)(rintf((len*8000000.0)/(*bw_)+.5))+PRM_Time) 
#define TX_Time(len)							\
	((u_int32_t)(rintf((len*8000000.0)/(*bw_)+.5))+PRM_Time) 
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
#define ACKTimeout(len) (DATA_Time(len) + ACK_Time + sifs + difs)
#define NAVTimeout   	(2 * sifs + phymib->aSIFSTime + CTS_Time + 2 * phymib->aSlotTime)
 

#define RTS_DURATION(len)  (sifs + CTS_Time + sifs + DATA_Time(len) + sifs + ACK_Time)
#define CTS_DURATION(d)	   (d - (CTS_Time + sifs))
#define DATA_DURATION()	   (ACK_Time + sifs)
#define ACK_DURATION()	   0x00 


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
   Class IEEE 802.11 DCF
  ========================================================================*/
class mac802_11dcf : public NslObject {

 private :
	
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


	
	/* mac timer used by IEEE 802.11
	 */
	timerObj		*mhNav;		/* NAV timer */
	timerObj		*mhSend;	/* send timer */
	timerObj		*mhDefer;	/* defer timer */
	timerObj		*mhBackoff;	/* backoff timer */
	timerObj		*mhIF;		/* interface timer */
	timerObj		*mhRecv;  	/* receive timer */
	
	u_int64_t		nav;		/* network allocation vector */
	u_int16_t		cw;		/* Contention window */
	u_int16_t		ssrc;		/* STA Short Retry Count */
	u_int16_t		slrc;		/* STA Long Retry Count */
	u_int16_t		sifs; 		/* Short Interface Frame */
	u_int16_t		pifs; 		/* PCF Interface Frame */
	u_int16_t		difs;		/* DCF Interface Frame */
	u_int16_t		eifs; 		/* Extended Interface Frame */
	
	struct MAC_MIB		*macmib; 	/* MAC MIB of IEEE 802.11 */
	struct PHY_MIB		*phymib; 	/* PHY MIB of IEEE 802.11 */
	
	ePacket_		*epktCTRL;	/* CTS/ACK send buffer */
	ePacket_		*epktRTS;	/* RTS send buffer */
	ePacket_		*epktTx;	/* outgoing pcaket buffer */
	ePacket_		*epktRx;	/* incoming packet buffer */
	struct fragBuf		*epktBuf; 	/* fragmentation buffer */
	ePacket_                *epktBEACON;        /* incoming Beacon packet buffer */
	
	double			*CPThreshold_; 
	
	enum MacState		rx_state; 
	enum MacState		tx_state;
	u_char			tx_active;
	
	u_int16_t 		sta_seqno; 
	struct dup_cache	cache[CACHE_SIZE];
	
	int			init_PHYMIB();
	int			init_MACMIB();
	int			init_Timer();
	int			calcul_IFS();
	
	inline void inc_CW() {
		cw = (cw << 1) + 1;
		if (cw > phymib->aCWmax)
			cw = phymib->aCWmax;
	}; 
	
	inline void rst_CW() { cw = phymib->aCWmin; }; 
	inline void set_NAV(u_int32_t _nav); 

	/* functions called by timer */
	int			check_pktCTRL();
	int 			check_pktRTS();
	int			check_pktTx();
	int			check_pktBEACON();

	/* timer handler */
	int			navHandler(Event_ *e);
	int			sendHandler(Event_ *e);
	int			deferHandler(Event_ *e);
	int			backoffHandler(Event_ *e);
	int			txHandler(Event_ *e);
	int			recvHandler(Event_ *e); 
		
	/* Transmission functions */
	void 			sendRTS(u_char *dst);
	void 			sendCTS(u_char *dst, u_int16_t d);
	void			sendACK(u_char *dst);
	void 			sendDATA(u_char *dst);
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
	void			rx_resume();
	void			collision(ePacket_ *p);
	void			capture(ePacket_ *p);
	void			drop(ePacket_ *p, char *why); 
	void			start_backoff(struct frame_control*); 
	inline void		start_defer(u_int32_t us); 
	inline void		start_sendtimer(u_int32_t us);
	inline void  		start_recvtimer(u_int32_t us);
	inline void  		start_IFtimer(u_int32_t us); 
	int			update_dcache(u_char *mac, u_int16_t sc); 
	int			Fragment(ePacket_ *pkt, u_char *dst);

 public :
 
	mac802_11dcf(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~mac802_11dcf();
	
	int			init();
	int			recv(ePacket_ *pkt);
	int			send(ePacket_ *pkt);
	int			log();
}; 

#endif /* __NCTUNS_mac_802_11_dcf_h__ */
