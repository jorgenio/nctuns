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

#ifndef __NCTUNS_phy_80211p_h__
#define __NCTUNS_phy_80211p_h__

#include <stdio.h>
#include <string.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include "channel_model.h"
#include "channel_coding.h"

/*for cross-module function call*/
#include <mbinder.h>
//#include "../mac/mac_80211p.h"
#include <80211p/mac//mac_80211p.h>

#include <module/phy/cm.h>
#include <wphyInfo.h>

#define SPEED_OF_LIGHT 300000000.0
#define symbolsPerSecond  125000

typedef struct PLCP_hdr{
	u_int32_t RATE:4;
	u_int32_t Reserved:1;
	u_int32_t LENGTH:12;
	u_int32_t Parity:1;
	u_int32_t Tail:6;
u_int32_t :8 ;
	   u_int16_t SERVICE;
}plcp_t;

enum rate_id_encodings {
	BPSK_1_2,               // 0
	BPSK_3_4,
	QPSK_1_2,
	QPSK_3_4,
	QAM16_1_2,
	QAM16_3_4,
	QAM64_2_3,
	QAM64_3_4
};

enum RXERROR{
	NoError,
	FormatViolation,
	CarrierLost,
	UnsupportedRate
};

struct PHYInfo {
	double		bw_;
	double 		SNR;             // Used between PHY and MAC
	double 		power;           // Used between PHY peers
	double		RxPr_;
	double 		txtime;
	int             frame_len;
	int    		payload_len;	
	int    		mac_hdr_length;		//MPDU_header LENGTH
	double    	RX_Thresh;
	int 		fec;                // FEC type
	int 		ChannelID;
	plcp_t*		PLCPInfo;
};

struct TXVECTOR{
	u_int16_t LENGTH;
	double DATARATE;
	u_int16_t SERVICE;
	double TXPWR_LEVEL;	//CHECK!
};

struct RXVECTOR{
	u_int16_t LENGTH;
	double WRSS;            //WAVE receive signal strength  , CHECK
	int DATARATE;
	u_int16_t SERVICE;
};
struct PHY_MIB{
	//PhyOperationTable
	u_char                  aPHYType;
	u_int32_t               acurrentRegdomain;
	u_int32_t		acurrentFrequencyBand;
	u_int32_t		aTemp;
	u_int32_t               aSlotTime;
	u_int32_t               aSIFSTime;
	u_int32_t               aCCATime;
	u_int32_t		aPHYRxStartDelay;
	u_int32_t		aRxTxTurnaroundTime;
	u_int32_t		aTxPLCPDelay;
	u_int32_t		aRxPLCPDelay;
	u_int32_t		aRxTxSwitchTime;
	u_int32_t		aTxRampOnTime;
	u_int32_t		aTxRampOffTime;
	u_int32_t		aTxRFDelay;
	u_int32_t               aRxRFDelay;
	u_int32_t               aAirPropagationTime;
	u_int32_t               aMACProcessingDelay;
	u_int32_t               aPreambleLength;
	u_int32_t               aPLCPHeaderLength;
	u_int32_t               aMPDUMaxLength;
	u_int16_t               aCWmin;
	u_int16_t               aCWmax;
	//WAVE addition
	u_int32_t		aDeviceType;	//The device power Level : Class A =1 , Class B =2 , Class C =3 , Class D =4
	u_int32_t		aACRType;	//The Adjacent/Alternate Channel Rejection type
	u_int32_t		aWRSSMode;	//WAVE reveive strength signal information

	//PhyAntennaTable
	u_int32_t               aCurrentTxAntenna;
	u_int32_t		aCurrentRxAntenna;
	u_int32_t               aDiversitySupport;

	//PhyTxPowerTable
	u_int32_t               aNumberSupportedPowerLevels;	//CHECK!
	u_int32_t               aTxPowerLevel1;
	u_int32_t               aTxPowerLevel2;
	u_int32_t               aTxPowerLevel3;
	u_int32_t               aTxPowerLevel4;
	u_int32_t               aTxPowerLevel5;
	u_int32_t               aTxPowerLevel6;
	u_int32_t               aTxPowerLevel7;
	u_int32_t               aTxPowerLevel8;
	u_int32_t               aTxPowerLevel9;
	u_int32_t               aTxPowerLevel10;
	u_int32_t               aTxPowerLevel11;
	u_int32_t               aTxPowerLevel12;
	u_int32_t               aTxPowerLevel13;
	u_int32_t               aTxPowerLevel14;
	u_int32_t               aTxPowerLevel15;
	u_int32_t               aTxPowerLevel16;
	u_int32_t               aTxPowerLevel17;
	u_int32_t               aTxPowerLevel18;
	u_int32_t               aTxPowerLevel19;
	u_int32_t               aTxPowerLevel20;
	u_int32_t               aTxPowerLevel21;
	u_int32_t               aTxPowerLevel22;
	u_int32_t               aTxPowerLevel23;
	u_int32_t               aTxPowerLevel24;
	u_int32_t               aTxPowerLevel25;
	u_int32_t               aTxPowerLevel26;
	u_int32_t               aTxPowerLevel27;
	u_int32_t               aTxPowerLevel28;
	u_int32_t               aTxPowerLevel29;
	u_int32_t               aTxPowerLevel30;
	u_int32_t               aTxPowerLevel31;
	u_int32_t               aTxPowerLevel32;
	u_int32_t               aTxPowerLevel33;
	u_int32_t               aTxPowerLevel34;
	u_int32_t               aTxPowerLevel35;
	u_int32_t               aTxPowerLevel36;
	u_int32_t               aTxPowerLevel37;
	u_int32_t               aTxPowerLevel38;
	u_int32_t               aTxPowerLevel39;
	u_int32_t               aTxPowerLevel40;
	u_int32_t               aTxPowerLevel41;
	u_int32_t               aTxPowerLevel42;
	u_int32_t               aTxPowerLevel43;
	u_int32_t               aTxPowerLevel44;
	u_int32_t               aTxPowerLevel45;
	u_int32_t               aTxPowerLevel46;
	u_int32_t               aTxPowerLevel47;
	u_int32_t               aTxPowerLevel48;
	u_int32_t               aTxPowerLevel49;
	u_int32_t               aTxPowerLevel50;
	u_int32_t               aTxPowerLevel51;
	u_int32_t               aTxPowerLevel52;
	u_int32_t               aTxPowerLevel53;
	u_int32_t               aTxPowerLevel54;
	u_int32_t               aTxPowerLevel55;
	u_int32_t               aTxPowerLevel56;
	u_int32_t               aTxPowerLevel57;
	u_int32_t               aTxPowerLevel58;
	u_int32_t               aTxPowerLevel59;
	u_int32_t               aTxPowerLevel60;
	u_int32_t               aTxPowerLevel61;
	u_int32_t               aTxPowerLevel62;
	u_int32_t               aTxPowerLevel63;
	u_int32_t               aTxPowerLevel64;
	u_int32_t               aCurrentTxPowerLevel;

	//PhyRegDomainsSupportedTable
	u_char                  *aRegDomainsSupported;
	u_char                  *aFrequencyBandSupported;

	//PhyAntennasList
	u_int32_t             *aSupportedTxAntennas;
	u_int32_t             *aSupportedRxAntennas;
	u_int32_t             *aDiversitySelectionRx;

	//PhySupportedDataRateTable
	u_char                  *aSupportedDataRatesTx;
	u_char                  *aSupportedDataRatesRx;

	//PhyOFDMTable
	u_int32_t		aCurrentFrequency;
	u_int32_t		aTIThreshold;
	u_int32_t		aFrequencyBandsSupported;
	u_int32_t		aChannelStartingFactor;
	u_int32_t		aFiveMHzOperationImplemented;
	u_int32_t               aTenMHzOperationImplemented;
	u_int32_t               aTwentyMHzOperationImplemented;	
	u_int32_t		aPhyOFDMChannelWidth;
};

class phy_80211p:public NslObject {
	private:

		u_char etherbroadcastaddr[6];
#ifdef LINUX
		int                     *tunfd_;
#endif
		//hychen modified 2008/2/26
		enum STATUS{
			IDLE,
			BUSY
		}State;
		plcp_t* PLCP;
		int _ChannelID;  //  Channel number 172,174,175,176,178,180,181,182,184 for WAVE system , channel 178 is control channel
		double bw,_datarate;
		int    fec;
		int    len;	//MPDU length
		int    channelSpacing;
		double symbolDuration;
		char   deviceType;  //Class A , B , C , D
		double transPower;
		double _RecvSensitivity;	//RxPowThreshold
		double CPThresh_;      // Capture threshold (db)
		double Gain_;
		double T_FFT; 
		double T_preamble;	// T_short + T_long
		double T_signal;	// T_GI + T_FFT
		double T_GI;	// T_FFT/4
		double T_SYM;   // T_GI + T_FFT
		double T_short;	// short sequence duration
		double T_long;	// long sequence duration

		/*Packet buffer*/
		ePacket_                *epktRx;        /* incoming packet buffer */

		/*phy timer */
		timerObj    *resetStateTimer;		/* state timer */
		timerObj    *RxStateTimer;

		struct PHY_MIB *phymib;

		/* For linkfail testing */
		char                    *_linkfail;
		char                    *linkfailFileName;
		FILE                    *linkfailFile;
		u_int32_t               LinkFailFlag;

		/*Log purpose*/
		u_int16_t               *ssrc_p;
		u_int16_t               *slrc_p;
		u_int32_t               *aRTSThreshold_p;
		u_int16_t               ssrc;           /* STA Short Retry Count */
		u_int16_t               slrc;           /* STA Long Retry Count */
		struct MAC_MIB          *macmib;
		char                    *mac_;	

		int init_PHYMIB();
		int init_Timer();
		void buildPLCP(double , int);
		inline void start_ResetStatetimer(u_int32_t);
		inline void start_recvtimer(u_int32_t);
		inline void Rx_Statetimer(u_int32_t);
		void recvHandler(Event *pkt);
		void TX_StateHandler();

		void PHY_TXSTART_confirm();
		void PHY_TXEND_confirm();
		void PHY_CCARESET_confirm();
		void PHY_CCA_indication(int state);
		void PHY_RXSTART_indication(struct RXVECTOR);
		void PHY_RXEND_indication(int);
		void PMD_TXSTART_request();
		void PMD_TXEND_request();
		void PMD_TXPWRLVL_request(double);
		void PMD_RATE_request(double);
		void PMD_WRSS_indicate(double);
		double PLME_TXTIME_request();
		double PLME_TXTIME_confirm();


		ChannelCoding_80211p *channelCoding;
		ChannelModel_80211p *channelModel;

		// For channel model
		int             beamwidth;
		double		freq_; // MHz
		double          pointingDirection;
		double          angularSpeed;   // degrees per second

		//yoy add 2009/7/2
		bool		cancel_MAC_recv;

	public:
		phy_80211p(u_int32_t type, u_int32_t id, struct plist *pl, const char *name);
		~phy_80211p();

		timerObj    *mhRecv;        /* receive timer */	
		int init();
		int recv(ePacket_ *pkt);
		int send(ePacket_ *pkt);
		void PHY_CCARESET_request();
		void PHY_TXSTART_request(struct TXVECTOR);
		void PHY_TXEND_request();

		void                    TurnOnLinkFailFlag(Event_ *ep);
		void                    TurnOffLinkFailFlag(Event_ *ep);

		double TXTIME(int length , double rate);
		int switch_Channel_and_Power(int channel_num);
};


#endif /* __NCTUNS_phy_802_11_dcf_h__ */
