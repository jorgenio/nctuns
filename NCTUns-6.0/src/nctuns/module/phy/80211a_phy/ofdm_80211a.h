#ifndef __NCTUNS_phy_80211a_h__
#define __NCTUNS_phy_80211a_h__

#include <stdio.h>
#include <string.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include "channel_model.h"
#include "channel_coding.h"
#include <tcpdump/wtcpdump.h>
/*for cross-module function call*/
#include <mbinder.h>
#include <mac/mac-80211a-dcf.h>

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
	int		frame_len;
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
        double RSSI;            //WAVE receive signal strength  , CHECK
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
	u_int32_t		aRSSIMode;	//Reveive strength signal information
	
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

class phy_80211a:public NslObject {
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
	int _ChannelID;  //  Channel number 34,36,38,40,42,44,46,48,52,56,60,64,149,153,157,161
	double bw,_datarate;
	int    fec;
	int    len;	//MPDU length
	int    channelSpacing;
        double symbolDuration;
        double transPower;
	double _RecvSensitivity;	//RxPowThreshold
	double CPThresh_;      // Capture threshold (db)
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
	u_int16_t		*ssrc_p;
	u_int16_t               *slrc_p;
	u_int32_t               *aRTSThreshold_p;
	u_int16_t               ssrc;           /* STA Short Retry Count */
        u_int16_t               slrc;           /* STA Long Retry Count */
	struct MAC_MIB          *macmib;
	char			*mac_;

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
	void PMD_RSSI_indicate(double);
	double PLME_TXTIME_request();
	double PLME_TXTIME_confirm();
	 
	
        ChannelCoding_80211a *channelCoding;
        ChannelModel_80211a *channelModel;

	// For channel model
	int             beamwidth;
	double		freq_; // MHz
	double          pointingDirection;
	double          angularSpeed;   // degrees per second
	double		Gain_;
	bool		cancel_MAC_recv;

      public:
         phy_80211a(u_int32_t type, u_int32_t id, struct plist *pl,const char *name);
        ~phy_80211a();

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
};


#endif /* __NCTUNS_phy_802_11_dcf_h__ */
