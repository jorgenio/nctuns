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

#include <stdio.h>
#include <stdlib.h>
#include <nctuns_api.h>
#include <con_list.h>
#include <packet.h>
#include <gbind.h>
#include <assert.h>
#include <string.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include <math.h>
#include <80211p/phy/ofdm_80211p.h>
/* added by kcliu */
#define VERBOSE_LEVEL	MSG_DEBUG
#include <wimax/mac/verbose.h>
/* for DEBUG */
/* or linkfail testing*/
#ifdef LINUX
#include <sys/ioctl.h>
#include <if_tun.h>
#endif
#include <mac-802_11.h>
#include <ethernet.h>
#include <ip.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>
#include <80211p/mac/mac_80211p.h>
/*Ad_hoc routing protocol infomation*/
#include<route/dsr/dsr_info.h>


#define DEBUG_PHY 0

MODULE_GENERATOR(phy_80211p);
extern SLIST_HEAD(WAVE_head, con_list) headOfWAVE_;
extern char *WAVEChannelCoding;

phy_80211p::phy_80211p(u_int32_t type, u_int32_t id, struct plist *pl,
		const char *name):NslObject(type, id, pl, name)
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("Constructor...\n");
#endif
	/* initialize PHY MIB used by PHY */
	init_PHYMIB();

	/* initialize timer used by PHY */
	init_Timer();

	/* Bind variables */

	vBind("bw" , &_datarate);
	vBind("TransPower", &transPower);
	vBind("CSThresh", &_RecvSensitivity);
	vBind("freq", &freq_);
	vBind("Gain", &Gain_);
	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);

	/* Register variables */
	REG_VAR("bw" , &_datarate);
	REG_VAR("transPower", &transPower);
	REG_VAR("CHANNEL", &_ChannelID);
	REG_VAR("RecvSensitivity", &_RecvSensitivity);

	REG_VAR("FREQ", &freq_);
	REG_VAR("beamwidth", &beamwidth);
	REG_VAR("pointingDirection", &pointingDirection);
	REG_VAR("angularSpeed", &angularSpeed);

	freq_ = 5890; //Mhz
	beamwidth = 360;
	pointingDirection = 0;
	angularSpeed = 0;
	Gain_ = 1;

	epktRx = 0;

	State = IDLE;
	PLCP = new plcp_t;
	_datarate = 6;
	bw = _datarate; // Mbps , data rate 3 ,6 and 9 Mb/s are madatory, Default = 6 Mbps
	fec = QPSK_1_2;

	LinkFailFlag = 0;

	/* In North America, RSU sholud generate Beacons on the CCH 178.Most communications will be initiated on the CCH. Chanel 172 
	 * & 184 shall be reserved for communications of safety-related applications.*/

	_ChannelID = 178;	
	deviceType = 'D';
	channelSpacing = 10;    //MHz
	transPower = 28.8;   // dBm , Device class D , Maximum device output power
	_RecvSensitivity = -82; //dBm , Minimum sensitivity of Date rate = 6Mbps
	CPThresh_ = 10.0;


	T_FFT = 6.4;    //us
	T_GI = T_FFT / 4;
	T_short  = 16;  //us
	T_long = 16;    //us
	T_preamble = T_short + T_long;
	T_signal = T_GI + T_FFT;
	T_SYM = T_GI + T_FFT;
	symbolDuration = T_SYM;

	if (WAVEChannelCoding && !strcasecmp(WAVEChannelCoding, "on")) {

		/*	hychen added , assume Tx/Rx on the same height and frequency = 5.89 uses for control channel(channel = 178 ) , based on 802.16 channel_model parameter atm.
		*/
		channelCoding = new ChannelCoding_80211p(true);
		channelModel = new ChannelModel_80211p(5.89,10.0, T_SYM, true);   
	} else {
		channelCoding = new ChannelCoding_80211p(false);
		channelModel =  new ChannelModel_80211p(5.89,10.0, T_SYM, false);
	}
	for(int i=0;i<6;i++)
	{
		etherbroadcastaddr[i] = 0xff;
	}

	macmib = (struct MAC_MIB *)malloc(sizeof(struct MAC_MIB));
	assert(macmib);

}

phy_80211p::~phy_80211p()
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("Destructor...\n");
#endif
	delete phymib;
	delete PLCP;	
	delete channelCoding;
	delete channelModel;
	delete resetStateTimer;
	delete macmib;
}

int phy_80211p::init()
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("Initialize...\n");
#endif

	switch((int)bw)
	{
		case 3:
			{
				fec = BPSK_1_2;
				if(_RecvSensitivity != -85)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		case 4 :	//4.5
			{
				fec = BPSK_3_4;
				if(_RecvSensitivity != -84)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		case 6:
			{
				fec = QPSK_1_2;
				if(_RecvSensitivity != -82)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		case 9:
			{
				fec = QPSK_3_4;
				if(_RecvSensitivity != -80)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		case 12:
			{
				fec = QAM16_1_2;
				if(_RecvSensitivity != -77)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		case 18:
			{
				fec = QAM16_3_4;
				if(_RecvSensitivity != -73)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		case 24:
			{
				fec = QAM64_2_3;
				if(_RecvSensitivity != -69)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");					break;
			}
		case 27:
			{
				fec = QAM64_3_4;
				if(_RecvSensitivity != -68)
					NSLOBJ_DEBUG("Current _RecvSensitivity does not follow the SPEC regulation!\n");
				break;
			}
		default:
			NSLOBJ_DEBUG("Unsupported data rate!\n");

	}

	char    *FILEPATH;
	char    line[128];

	if( _linkfail && !strcmp(_linkfail, "on") ) {

		tunfd_ = GET_REG_VAR1(get_portls(), "TUNFD", int *);
		/*
		 * if tunfd_ equal to NULL, that means
		 * this is a layer-1 or layer-2 device.
		 * NO TYNFD is supposed to be got.
		 */

		FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
				strlen(linkfailFileName) + 1);
		sprintf(FILEPATH,"%s%s", GetConfigFileDir(), linkfailFileName);

		linkfailFile = fopen(FILEPATH,"r");

		if( linkfailFile == NULL ) {
			printf("Warning : Can't read file %s\n", FILEPATH);
		}
		else {
			double		StartTime, StopTime;
			Event_		*start_ep;
			Event_		*stop_ep;
			u_int64_t	StartTimeTick, StopTimeTick;
			BASE_OBJTYPE(typeStart);
			BASE_OBJTYPE(typeStop);

			typeStart = POINTER_TO_MEMBER(phy_80211p, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(phy_80211p, TurnOffLinkFailFlag);

			while( !feof(linkfailFile) ) {
				line[0] = '\0';
				fgets(line, 127, linkfailFile);
				if ((line[0]=='\0')||(line[0]=='#'))
					continue;
				if ( 2 == sscanf(line, "%lf %lf",
							&StartTime, &StopTime) ) {

					if( StartTime >= StopTime )
						continue;
					/* handle start evnet */
					SEC_TO_TICK(StartTimeTick, StartTime);
					start_ep =  createEvent();
					setObjEvent(start_ep,
							StartTimeTick,
							0,this,typeStart,
							(void *)NULL);

					/* handle stop event */
					SEC_TO_TICK(StopTimeTick, StopTime);
					stop_ep =  createEvent();
					setObjEvent(stop_ep,
							StopTimeTick,
							0,this,typeStop,
							(void *)NULL);
				}
			}
		}

		free(FILEPATH);
	}
	/*Register for Log purpose*/
	ssrc_p = GET_REG_VAR1(get_portls(), "SSRC", u_int16_t*);
	slrc_p = GET_REG_VAR1(get_portls(), "SLRC", u_int16_t*);
	aRTSThreshold_p = GET_REG_VAR1(get_portls(), "RTS_THRESHOLD", u_int32_t*);
	mac_ = GET_REG_VAR1(get_portls(),"MAC",char*);

	cancel_MAC_recv = false;

	return NslObject::init();
}

void phy_80211p::buildPLCP(double datarate , int length)
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("Building PLCP header for transmitting PPDU...\n");
#endif
	memset(PLCP , 0 , sizeof(plcp_t));	
	switch((int)datarate)
	{
		case 3:
			PLCP->RATE = 13;		// 1101
			fec =  BPSK_1_2;
			_RecvSensitivity = -85;
			break;
		case 4:				//4.5Mbps
			PLCP->RATE = 15;
			fec =  BPSK_3_4;
			_RecvSensitivity = -84;
			break;
		case 6:
			PLCP->RATE = 5;
			fec =  QPSK_1_2;
			break;
		case 9:
			PLCP->RATE = 7;
			fec =  QPSK_3_4;
			break;
		case 12:
			PLCP->RATE = 9;
			fec =  QAM16_1_2;
			break;
		case 18:
			PLCP->RATE = 11;
			fec =  QAM16_3_4;
			break;
		case 24:
			PLCP->RATE = 1;
			fec =  QAM64_2_3;
			break;
		case 27:
			PLCP->RATE = 3;
			fec =  QAM64_3_4;
			break;
		default:
			NSLOBJ_DEBUG("Unsupported data rate!\n");
	}
	PLCP->LENGTH = length;
	PLCP->Reserved = 0;
	PLCP->Parity = 0;
	PLCP->Tail = 0;
	PLCP->SERVICE = 0;

}
/*	This primitive will be issued by the MAC sublayer to the PHY entity when the MAC sublayer needs to begin
 *	the transmission of an MPDU.
 */
void phy_80211p::PHY_TXSTART_request(struct TXVECTOR txvector)
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("MAC sends PHY_TXSTART_request primitive to PHY\n");
#endif
	bw = txvector.DATARATE;
	len = txvector.LENGTH;
	//transPower = txvector.TXPWR_LEVEL;
	//CHECK!!
	PMD_TXPWRLVL_request(txvector.TXPWR_LEVEL);
	PMD_RATE_request(txvector.DATARATE);
	PMD_TXSTART_request();
	PHY_TXSTART_confirm();
}
/*	This primitive will be issued by the PHY to the MAC entity when the PHY has received a PHYTXSTART.
 *	request from the MAC entity and is ready to begin accepting outgoing data octets from the
 *	MAC.
 */
void phy_80211p::PHY_TXSTART_confirm()
{
	return;
}
/*	This primitive will be generated when the MAC sublayer has received the last PHY-DATA.confirm from
 *	the local PHY entity for the MPDU currently being transferred.
 */
void phy_80211p::PHY_TXEND_request()
{

	PHY_TXEND_confirm();
	return;
}
/*	This primitive is issued by the PHY to the local MAC entity to confirm the completion of a transmission.
 *	The PHY issues this primitive in response to every PHY-TXEND.request primitive issued by the MAC
 *	sublayer.
 *	This primitive will be issued by the PHY to the MAC entity when the PHY has received a PHYTXEND.
 *	request immediately after transmitting the end of the last bit of the last data octet indicating that the
 *	symbol containing the last data octet has been transferred.
 */
void phy_80211p::PHY_TXEND_confirm()
{
	return;
}
/*	The primitive provides the following parameter:
 *	PHY-CCA.indication (STATE)
 *	The STATE parameter can be one of two values: BUSY or IDLE. The parameter value is BUSY if the
 *	channel assessment by the PHY determines that the channel is not available. Otherwise, the value of the
 *	parameter is IDLE.
 *	The primitive provides the following parameter:
 *	PHY-CCA.indication (STATE)
 *	The STATE parameter can be one of two values: BUSY or IDLE. The parameter value is BUSY if the
 *	channel assessment by the PHY determines that the channel is not available. Otherwise, the value of the
 *	parameter is IDLE.
 */
void phy_80211p::PHY_CCA_indication(int state)
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("State=[%d], PHY sends PHY_CCA_indication primitive to MAC\n" ,state);
#endif
	if (state == IDLE)
		State = IDLE;
	else
		State = BUSY;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();
	((mac_80211p *)InstanceLookup(nid,pid,"MAC80211p"))->set_PHY_state(state);
	if(cancel_MAC_recv == true && State == IDLE)
	{
		((mac_80211p *)InstanceLookup(nid,pid,"MAC80211p"))->rx_resume();
		cancel_MAC_recv = false;
	}
}
/*	This primitive is a request by the MAC sublayer to the local PHY entity to reset the CCA state machine.	
 *	This primitive is generated by the MAC sublayer for the local PHY entity at the end of a NAV timer. This
 *	request can be used by some PHY implementations that may synchronize antenna diversity with slot timings.
 *	The effect of receipt of this primitive by the PHY entity is to reset the PLCP CS/CCA timers to the state
 *	appropriate for the end of a received frame.
 */
void phy_80211p::PHY_CCARESET_request()
{
	PHY_CCA_indication(IDLE);
	epktRx = NULL;	
#if DEBUG_PHY
	NSLOBJ_DEBUG("State=[%d], Resetting its medium state!!\n" ,State);
#endif
	PHY_CCARESET_confirm();
	return;
}
/*	This primitive is issued by the PHY to the MAC entity when the PHY has received a PHY-CCARESET.
 *	request.
 */
void phy_80211p::PHY_CCARESET_confirm()
{
	return;
}
/*	This primitive is an indication by the PHY to the local MAC entity that the PLCP has received a valid start
 *	frame delimiter (SFD) and PLCP header.
 *	The primitive provides the following parameter:
 *	PHY-RXSTART.indication (RXVECTOR)
 *	The RXVECTOR represents a list of parameters that the PHY provides the local MAC entity upon receipt of
 *	a valid PLCP header. This vector may contain both MAC and MAC management parameters.
 *	This primitive is generated by the local PHY entity to the MAC sublayer when the PHY has successfully
 *	validated the PLCP header at the start of a new PPDU.
 *	After generating a PHYRXSTART.indication, the PHY is expected to maintain physical medium busy
 *	status (not generating PHY-CCA.indication(IDLE)) during the period required by that PHY to transfer a
 *	frame of the indicated LENGTH at the indicated DATARATE. This physical medium busy condition should
 *	be maintained even if a PHY-RXEND.indication(CarrierLost) or a PHYRXEND.indication(Format-
 *	Violation) is generated by the PHY prior to the end of this period.
 */
void phy_80211p::PHY_RXSTART_indication(struct RXVECTOR _rxvector )
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("PHY send PHY_RXSTART_indication primitive to MAC\n");
#endif
	PHY_CCA_indication(BUSY);
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();
	((mac_80211p *)InstanceLookup(nid,pid,"MAC80211p"))->set_mac_rxstate();
	return;	
}
/*	This primitive is an indication by the PHY to the local MAC entity that the MPDU currently being received
 *	is complete.
 *	The primitive provides the following parameter:
 *	PHY-RXEND.indication (RXERROR)
 *	The RXERROR parameter can convey one or more of the following values: NoError, FormatViolation,
 *	CarrierLost, or UnsupportedRate. A number of error conditions may occur after the PLCP’s receive state
 *	machine has detected what appears to be a valid preamble and SFD. The following describes the parameter
 *	returned for each of those error conditions.
 *	— NoError. This value is used to indicate that no error occurred during the receive process in the
 *	PLCP.
 *	— FormatViolation. This value is used to indicate that the format of the received PPDU was in error.
 *	— CarrierLost. This value is used to indicate that during the reception of the incoming MPDU, the
 *	q
 *	carrier was lost and no further processing of the MPDU can be accomplished.
 *	— UnsupportedRate. This value is used to indicate that during the reception of the incoming PPDU, a
 *	nonsupported date rate was detected.
 *	This primitive is generated by the PHY for the local MAC entity to indicate that the receive state machine
 *	has completed a reception with or without errors.
 *	In the case of an RXERROR value of NoError, the MAC uses the PHY-RXEND.Indication as reference for
 *	channel access timing, as shown in Figure 9-12 (in 9.2.10).
 *	The effect of receipt of this primitive is for the MAC to begin inter-frame space processing, as described in
 *	9.2.10.
 */
void phy_80211p::PHY_RXEND_indication(int RXERROR)
{
	if (RXERROR == NoError)
	{
#if DEBUG_PHY
		NSLOBJ_DEBUG("Transmission successful!\n");
#endif
	}
	else if (RXERROR == FormatViolation)
	{
		NSLOBJ_DEBUG("Parity check failed!\n");
	}
	else if (RXERROR == CarrierLost)
	{
		NSLOBJ_DEBUG("A change in the WRSS causes the status of the CCA to return to the IDLE state before the complete reception of the PSDU!\n");
	}
	else
	{
		NSLOBJ_DEBUG("The rate in the SIGNAL field is not receivable!\n");
	}
}
/*	This primitive shall be generated by the PLCP sublayer to initiate the PMD layer transmission of the PPDU.
 *	The PHY-TXSTART.request primitive shall be provided to the PLCP sublayer prior to issuing the
 *	PMD_TXSTART command.
 */
void phy_80211p::PMD_TXSTART_request()
{
	return;
}
/*	This primitive shall be generated by the PLCP sublayer to terminate the PMD layer transmission of
 *	the PPDU.
 */
void phy_80211p::PMD_TXEND_request()
{
	return;
}
/*	The receipt of this primitive by the PMD entity will cause the transmit power level to be modify. */
void phy_80211p::PMD_TXPWRLVL_request(double TXPWR)
{	
	return;
}
/*	This primitive shall be generated by the PLCP sublayer to change or set the current OFDM PHY modulation
 *	rate used for the MPDU portion of a PPDU.
 */
void phy_80211p::PMD_RATE_request(double RATE)
{
	return;
}
/*	This primitive shall provide the following parameters:
 *	PMD_RSSI.indicate(RSSI)
 *	The RSSI shall be a measure of the RF energy received by the OFDM PHY. RSSIs of up to 8 bits
 *	(256 levels) are supported.
 *	This primitive shall be generated by the PMD when the OFDM PHY is in the receive state. It shall be
 *	available continuously to the PLCP which, in turn, shall provide the parameter to the MAC entity.
 *	This parameter shall be provided to the PLCP layer for information only. The RSSI may be used as part of a
 *	CCA scheme.
 */
void phy_80211p::PMD_WRSS_indicate(double WRSS)
{
	return;
}
/*	Calculate Transmission Time
 *	This primitive is a request for the PHY to calculate the time that will be required to transmit onto the WM a
 *	PPDU containing a specified length MPDU, and using a specified format, data rate, and signalling.
 *	This primitive provides the following parameters:
 *	PLME-TXTIME.request(TXVECTOR)
 *	The TXVECTOR represents a list of parameters that the MAC sublayer provides to the local PHY entity in
 *	order to transmit an MPDU, as further described in 12.3.4.4 and 17.4 (which defines the local PHY entity).
 *	This primitive is issued by the MAC sublayer to the PHY entity when the MAC sublayer needs to determine
 *	the time required to transmit a particular MPDU.
 */
double phy_80211p::PLME_TXTIME_request()
{
#if DEBUG_PHY
	NSLOBJ_DEBUG("Calculating TXTIME of currently transmit!\n");
#endif
	double TXTIME;
	//TXTIME = T_preamble + T_signal +(16+8*length+6) / DATARATE + T_sym/2;
	//TXTIME = T_preamble + T_signal + T_sym*ceil((16+8*length+6)/N_dbps);
	TXTIME = PLME_TXTIME_confirm();	
	return TXTIME;		
}
/*	This primitive provides the time that will be required to transmit the PPDU described in the corresponding
 *	PLME-TXTIME.request.
 *	This primitive provides the following parameters:
 *	PLME-TXTIME.confirm(TXTIME)
 *	The TXTIME represents the time, in microseconds, required to transmit the PPDU described in the
 *	corresponding PLME-TXTIME.request. If the calculated time includes a fractional microsecond, the
 *	TXTIME value is rounded up to the next higher integer.
 */
double phy_80211p::PLME_TXTIME_confirm()
{
	int temp;
	/*T_preamble + T_signal + T_sym*ceil((16+8*length+6)/N_dbps)*/
	if ( (2+PLCP->LENGTH+1) % channelCoding->getUncodedBlockSize(fec) != 0)
		temp = (2+PLCP->LENGTH+1)/channelCoding->getUncodedBlockSize(fec)+1;
	else
		temp = (2+PLCP->LENGTH+1)/channelCoding->getUncodedBlockSize(fec);
	return T_preamble + T_signal + T_SYM*temp;
}
void phy_80211p::TX_StateHandler()
{
	PHY_CCA_indication(IDLE);
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();
	((mac_80211p *)InstanceLookup(nid,pid,"MAC80211p"))->triggerMAC();
}
double phy_80211p::TXTIME(int length , double rate)
{
	int fec;
	int temp;
	rate = rate/1000000;
	switch((int)rate)
	{
		case 3:
			fec = 0;
			break;
		case 4:
			fec = 1;
			break;
		case 6:	
			fec = 2;
			break;
		case 9:
			fec = 3;
			break;
		case 12:
			fec = 4;
			break;
		case 18:
			fec = 5;
			break;
		case 24:
			fec = 6;
			break;
		case 27:
			fec = 7;
			break;
		default:
			printf("Unsupported data rate!!!\n");
			return (-1);
	}
	if ( (2+length+1) % channelCoding->getUncodedBlockSize(fec) != 0)
		temp = (2+length+1)/channelCoding->getUncodedBlockSize(fec)+1;
	else
		temp = (2+length+1)/channelCoding->getUncodedBlockSize(fec);
	return T_preamble + T_signal + T_SYM*temp;
}

inline void phy_80211p::start_ResetStatetimer(u_int32_t ticks) {

	resetStateTimer->start(ticks, 0);
}
inline void phy_80211p::start_recvtimer(u_int32_t ticks) {

	mhRecv->start(ticks, 0);
}
inline void phy_80211p::Rx_Statetimer(u_int32_t ticks){
	RxStateTimer->start(ticks, 0);
}

int phy_80211p::switch_Channel_and_Power(int ch)
{
	_ChannelID = ch;
	switch(ch)
	{       
		case 172:
			transPower = 28.8;
			break;
		case 174 :
			transPower = 28.8;
			break;
		case 175:
			transPower = 10;
			break;
		case 176:
			transPower = 28.8;
			break;
		case 178:
			transPower = 28.8;
			break;
		case 180:
			transPower = 10;
			break;
		case 181:
			transPower = 10;
			break;
		case 182:
			transPower = 10;
			break;
		case 184:
			transPower = 28.8;
			break;
		default:
			NSLOBJ_DEBUG("Unsupported Channel Number : %d\n",ch);
			return (-1);
	}
	return 1;

}

int phy_80211p::init_PHYMIB()
{
	phymib = (struct PHY_MIB *)malloc(sizeof(struct PHY_MIB));
	assert(phymib);

	phymib->aSlotTime = 13;  //us
	phymib->aSIFSTime = 32;
	phymib->aCCATime = 8;	//< 8 us
	phymib->aRxTxTurnaroundTime = 2;	// < 2us
	phymib->aAirPropagationTime = 1;
	phymib->aMACProcessingDelay = 2;
	phymib->aPreambleLength = 32;
	phymib->aPLCPHeaderLength = 8;
	phymib->aMPDUMaxLength = 4095;
	phymib->aCWmin = 15;
	phymib->aCWmax = 1023;

	return(1);
}

int phy_80211p::init_Timer()
{

	BASE_OBJTYPE(type);

	/*
	 * Generate timer needed in
	 * IEEE 802.11 PHY.
	 */
	resetStateTimer = new timerObj;
	mhRecv    = new timerObj;
	RxStateTimer = new timerObj;
	assert( resetStateTimer && mhRecv && RxStateTimer );

	/*
	 * Associate timer with corresponding
	 * time-handler method.
	 */
	type = POINTER_TO_MEMBER(phy_80211p, TX_StateHandler);
	resetStateTimer->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(phy_80211p, recvHandler);
	mhRecv->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(phy_80211p, PHY_CCARESET_request);
	RxStateTimer->setCallOutObj(this, type);

	return(1);

}
int phy_80211p::send(ePacket_ *pkt)
{
	Event 			*ep;
	struct PHYInfo 		phyInfo;
	Packet			*p,*DLframe;
	struct con_list		*WAVE;
	int			signal_len,framelen;	//byte
	int			PT_DATA_header_len;
	double			TXTIME;
	double			dist;
	char			*plcp_input,*input;
	char			*plcp_output,*output;

	u_int64_t		ticks;
	u_int32_t		*psid;

	/*Log*/
	struct hdr_mac802_11    *mh;
	struct ip               *iph;
	u_int32_t		*txnid;

	if ( State == IDLE)
	{
#if DEBUG_PHY
		NSLOBJ_DEBUG("Processing Transmission!!\n");
#endif
		p = (Packet*)pkt->DataInfo_;
		txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");
		assert(p && txnid);
		psid = (u_int32_t*) p->pkt_getinfo("PSID");
		/*For Ad-hoc routing protocol purpose*/
                DSR_Info *dsr_info = (DSR_Info*)p->pkt_getinfo( "dsr" );	

		if ( LinkFailFlag > 0 ) {
			freePacket(pkt);
			return(1);
		}

		mh = (struct hdr_mac802_11*)p->pkt_get();
		assert(mh);
		//build PLCP header
		len = p->pkt_getlen();
		buildPLCP( bw , len);
		char SIGNAL[3];		//establish SIGNAL field
		memset(SIGNAL , 0 , 3);
		memcpy(SIGNAL , PLCP , 3);

		signal_len = channelCoding->getCodedBurstLen(3 , BPSK_1_2);

		if ( p->pkt_sget() == NULL )
		{
			iph = (struct ip*)p->pkt_get()+(sizeof(struct hdr_mac802_11)+sizeof(struct ether_header));
			PT_DATA_header_len = p->pkt_getlen();
		}
		else
		{
			iph = (struct ip*)p->pkt_sget();
			PT_DATA_header_len = p->pkt_get_pt_data_len();
		}

		char *tmp = (char*) malloc(p->pkt_getlen());
		memset(tmp , 0 , p->pkt_getlen());
		input = p->pkt_aggregate();
		if(p->pkt_use_cluster())// yoy
                {
                        memcpy(tmp,p->pkt_get(),p->pkt_get_pt_data_len());
                        memcpy(tmp+p->pkt_get_pt_data_len(),input,p->pkt_getlen()-p->pkt_get_pt_data_len());
                }
                else
                {
                        memcpy(tmp , input , p->pkt_getlen());
                }

		input = tmp;
		framelen = channelCoding->getCodedBurstLen(p->pkt_getlen() , fec);
		
		u_long gateway = p->rt_gateway();
		DLframe = new Packet;
		DLframe->pkt_setflow(PF_SEND);
		DLframe->rt_setgw(gateway);
		plcp_input = SIGNAL;
		plcp_output = DLframe->pkt_malloc(signal_len);
		output =  DLframe->pkt_sattach(framelen);
		DLframe->pkt_sprepend(output,framelen);

		//Encode SIGNAL & DATA field
		channelCoding->encode(plcp_input , plcp_output , 3 , BPSK_1_2);
		channelCoding->encode(input , output , p->pkt_getlen() , fec);

		//Calculate TXTIME , simulation transmission time . Reset the phy medium state after transmission complete. 
		TXTIME = PLME_TXTIME_request();		//us

		memset(&phyInfo , 0 , sizeof(PHYInfo));
		phyInfo.bw_ = bw*1000000;
		phyInfo.power = transPower;
		phyInfo.fec = fec;
		phyInfo.frame_len = p->pkt_getlen();
		phyInfo.payload_len = p->pkt_getlen()-PT_DATA_header_len;
		phyInfo.mac_hdr_length = PT_DATA_header_len;
		phyInfo.ChannelID = _ChannelID;
		phyInfo.RX_Thresh = _RecvSensitivity;
		phyInfo.txtime = TXTIME;
		phyInfo.PLCPInfo = PLCP;
		DLframe->pkt_addinfo("WAVEPhyinfo",(char*)&phyInfo , sizeof(struct PHYInfo));
		DLframe->pkt_addinfo("MAC_HEADER",(char*)mh , sizeof(struct hdr_mac802_11));    //For comparing MAC_TYPE correction after doing FEC
		DLframe->pkt_addinfo("IP_HEADER",(char*)iph , sizeof(struct ip));
		DLframe->pkt_addinfo("LOG_TX_NID",(char*)txnid , sizeof(int));

		if( psid != NULL)
			DLframe->pkt_addinfo("PSID",(char*)psid , sizeof(u_int32_t));
		 /*For Ad-hoc routing protocol purpose*/
                if(dsr_info != NULL)
                        DLframe->pkt_addinfo( "dsr", (const char*)dsr_info, sizeof( DSR_Info ));

		MICRO_TO_TICK(ticks , TXTIME);
#if DEBUG_PHY
		NSLOBJ_DEBUG("Resetting TX state after %llu ticks\n" ,ticks);
#endif
		//scheduleTimer(resetStateTimer, ticks);
		start_ResetStatetimer(ticks);
		PHY_CCA_indication(BUSY);

#if DEBUG_PHY
		NSLOBJ_DEBUG("Send funciton:: before send to the air\n");
#endif

		// wphyinfo start
		struct wphyInfo         *wphyinfo;
		double currAzimuthAngle;
		double T_locX, T_locY, T_locZ;

		wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
		assert(wphyinfo);

		GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
		currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

		wphyinfo->nid           = get_nid();
		wphyinfo->pid           = get_port();
		wphyinfo->bw_           = 0;
		wphyinfo->channel_      = _ChannelID;
		wphyinfo->TxPr_         = pow(10, transPower/10) * 1e-3;
		wphyinfo->RxPr_         = 0.0;
		wphyinfo->srcX_         = T_locX;
		wphyinfo->srcY_         = T_locY;
		wphyinfo->srcZ_         = T_locZ;
		wphyinfo->currAzimuthAngle_     = currAzimuthAngle;
		wphyinfo->Pr_		= 0.0;

		DLframe->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
		free(wphyinfo);
		// wphyinfo end

		SLIST_FOREACH(WAVE, &headOfWAVE_, nextLoc) {


			// The receiver should not be myself
			if (get_nid() == WAVE->obj->get_nid() && get_port() == WAVE->obj->get_port())
				continue;

			ep = createEvent();
			ep->DataInfo_ = DLframe->copy();

			// Simulate propagation
			dist = GetNodeDistance(get_nid(), WAVE->obj->get_nid());
			((Packet *) ep->DataInfo_)->pkt_addinfo("dist",(char *) &dist,sizeof(double));
			ep->timeStamp_ = 0;
			ep->perio_ = 0;
			ep->calloutObj_ = WAVE->obj;
			ep->memfun_ = NULL;
			ep->func_ = NULL;
			NslObject::send(ep);
		}


		PMD_TXEND_request();
		freePacket(pkt);
		free(tmp);
		delete DLframe;
		return (1);

	}
	else
	{
#if DEBUG_PHY
		NSLOBJ_DEBUG("Transmiter:The WAVE_PHY medium is busying currently!!!\n");
#endif
		freePacket(pkt);
		return (1);
	}
}

int phy_80211p::recv(ePacket_ *pkt)
{
	Packet *p,*p_Rx;

	struct PHYInfo		*phyInfo,*phyInfo1;
	double			*dist;
	double 			recvPower;
	struct wphyInfo *wphyinfo;
	plcp_t                  plcp_hdr;
	char			*plcp_input,*plcp_output;
	struct RXVECTOR         _rxvector;
	double			BER;
	int			len;
	struct hdr_mac802_11    *mh,*mh_epktRx;
	struct mac802_11_log    *log1,*log2;
	struct ip                    *iph;
	u_long                  ipdst,ipsrc;
	struct logEvent         *logep;
	u_int64_t               time_period;

	u_int64_t               ticks;
	u_int32_t		*txnid;

	p = (Packet*) pkt->DataInfo_;
	phyInfo =(struct PHYInfo*) p->pkt_getinfo("WAVEPhyinfo");
	dist = (double *) p->pkt_getinfo("dist");
	wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
	mh = (struct hdr_mac802_11*)p->pkt_getinfo("MAC_HEADER");
	txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");

	assert(p && phyInfo && dist && wphyinfo && txnid);

	if ( LinkFailFlag > 0 ) {
		freePacket(pkt);
		return(1);
	}
#if DEBUG_PHY
	NSLOBJ_DEBUG("Processing Receive!!!\n");
#endif
	if (phyInfo->ChannelID != _ChannelID) {
#if DEBUG_PHY
		NSLOBJ_DEBUG("pro_ChannelID = %d,user_ChannelID = %d ,Channel number incorrectly\n",phyInfo->ChannelID,_ChannelID);
#endif
		freePacket(pkt);
		return (1);
	}	

	//recvPower = channelModel->receivePower(phyInfo->power, *dist);
	recvPower = wphyinfo->Pr_;
#if DEBUG_PHY
	NSLOBJ_DEBUG(">>>recvPower = %f\n",recvPower);
#endif
	phyInfo->SNR = channelModel->powerToSNR(recvPower);
	phyInfo->RxPr_ = pow(10, recvPower/10) * 1e-3; // Watt
	p->pkt_addinfo("WRSS", (char *)&phyInfo->RxPr_ ,8);
	PMD_WRSS_indicate(recvPower);

	if (recvPower < _RecvSensitivity) {
#if DEBUG_PHY
		NSLOBJ_DEBUG("Receive power lower than RX threshold!! recvPower : %f , RxThreshold : %f\n", recvPower ,_RecvSensitivity);
#endif
		freePacket(pkt);
		return (1);
	}

	/* Collisioned*/	
	if (State == BUSY) {
#if DEBUG_PHY
		NSLOBJ_DEBUG("Receiver:The WAVE_PHY  medium  is busying!\n");
#endif
		/* Log and Drop receive packet while doing transmitting*/
		if(!epktRx)
		{
			iph = (struct ip*)p->pkt_getinfo("IP_HEADER");
			GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

			log1 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
			LOG_802_11(mh,log1,p,GetCurrentTime(),GetCurrentTime(),get_type(),get_nid(),StartRX,ipsrc,ipdst,_ChannelID,*txnid);
			INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);
			log2 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
			MICRO_TO_TICK(time_period,phyInfo->txtime);
			DROP_LOG_802_11(log1,log2,GetCurrentTime()+time_period,DropRX,DROP_RXERR);
			INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);
			freePacket(pkt);
			return(1);
		}
		else    /* Log collisioned packets*/
		{
			/*
			 * If the power of the incoming packet is smaller
			 * than the power of the packet currently being 
			 * received by at least the capture threshold,
			 * then we ignore the new packet.
			 */
			GET_PKT(p, epktRx);
			phyInfo1 = (struct PHYInfo *)p->pkt_getinfo("WAVEPhyinfo");
			
			if (phyInfo1->RxPr_ / phyInfo->RxPr_ >= CPThresh_)
			{
				GET_PKT(p, pkt);
				phyInfo = (struct PHYInfo *)p->pkt_getinfo("WAVEPhyinfo");
				mh = (struct hdr_mac802_11*)p->pkt_getinfo("MAC_HEADER");

				/* log "StartRX" and "DropRX" event */
				if(!bcmp(mh->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN) ||
						!bcmp(mh->dh_addr1, mac_, ETHER_ADDR_LEN)) {

					iph = (struct ip*)p->pkt_getinfo("IP_HEADER");

					GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

					log1 = (struct mac802_11_log*)malloc
						(sizeof(struct mac802_11_log));
					LOG_802_11(mh,log1,p,GetCurrentTime(),
							GetCurrentTime(),get_type(),get_nid(),
							StartRX,ipsrc,ipdst,_ChannelID,*txnid);
					INSERT_TO_HEAP(logep,log1->PROTO,
							log1->Time+START,log1);

					log2 = (struct mac802_11_log*)malloc
						(sizeof(struct mac802_11_log));
					MICRO_TO_TICK(time_period,phyInfo->txtime);
					DROP_LOG_802_11(log1,log2,GetCurrentTime()+
							time_period,DropRX,DROP_CAP);
					INSERT_TO_HEAP(logep,log2->PROTO,
							log2->Time+ENDING,log2);
				}
				freePacket(pkt);
				return(1);

			}
			else
			{
				u_int64_t dt, expired;

				assert(epktRx && mhRecv->busy_);
				expired = mhRecv->expire() - GetCurrentTime();

				GET_PKT(p_Rx, epktRx);
				mh_epktRx = (struct hdr_mac802_11*)p_Rx->pkt_getinfo("MAC_HEADER");
				phyInfo1 = (struct PHYInfo *)p_Rx->pkt_getinfo("WAVEPhyinfo");

				GET_PKT(p, pkt);
				mh = (struct hdr_mac802_11*)p->pkt_getinfo("MAC_HEADER");
				phyInfo = (struct PHYInfo *)p->pkt_getinfo("WAVEPhyinfo");
				MICRO_TO_TICK(dt, phyInfo->txtime);

				iph = (struct ip*)p_Rx->pkt_getinfo("IP_HEADER");

				GET_IP_SRC_DST(mh_epktRx,iph,ipsrc,ipdst);

				log1 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
				LOG_802_11(mh_epktRx,log1,p_Rx,mhRecv->expire()-dt,
						mhRecv->expire()-dt,get_type(),get_nid(),
						StartRX,ipsrc,ipdst,_ChannelID,*txnid);
				INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

				log2 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
				DROP_LOG_802_11(log1,log2,mhRecv->expire(),DropRX,DROP_COLL);
				INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);
				/* log "StartRX" and "DropRX" event for the latter pkt */

				iph =  (struct ip*)p->pkt_getinfo("IP_HEADER");

				GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

				log1 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
				LOG_802_11(mh,log1,p,GetCurrentTime(),
						GetCurrentTime(),get_type(),get_nid(),
						StartRX,ipsrc,ipdst,_ChannelID,*txnid);
				INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

				log2 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
				DROP_LOG_802_11(log1,log2,GetCurrentTime()+dt,DropRX,DROP_COLL);
				INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);

				if (dt > expired) {
					cancel_MAC_recv = true;
					mhRecv->cancel();
					freePacket(epktRx);
					epktRx = NULL;
					freePacket(pkt);
					Rx_Statetimer(dt);
					return(1);
				}
				else {
					cancel_MAC_recv = true;
					mhRecv->cancel();
					freePacket(epktRx);
					epktRx = NULL;
					freePacket(pkt);
					Rx_Statetimer(expired);
					return(1);
				}
			}
		}
	}
	assert(!epktRx);	
	epktRx = pkt;           /* record currect Rx packet */
	//Decode PLCP SIGNEL field
	plcp_input = p->pkt_get();
	plcp_output = (char*) &plcp_hdr;

	len = channelCoding->getCodedBurstLen(3 , BPSK_1_2);
	// Make bit error
	BER = channelModel->computeBER(BPSK_1_2, phyInfo->SNR);
	channelModel->makeBitError(plcp_input, len , BER);

	channelCoding->decode(plcp_input , plcp_output, len , BPSK_1_2);

	_rxvector.LENGTH = plcp_hdr.LENGTH;
	_rxvector.WRSS = phyInfo->RxPr_;
	_rxvector.DATARATE = plcp_hdr.RATE;
	_rxvector.SERVICE = 0;

	/* Check if datarate corresponds with 802.11p supported*/
	switch(_rxvector.DATARATE)
	{
		case 13 :
			break;
		case 15 :
			break;
		case 5  :
			break;
		case 7  :
			break;
		case 9  :
			break;
		case 11 :
			break;
		case 1  :
			break;
		case 3  :
			break;
		default :
			PHY_RXEND_indication(UnsupportedRate);
			freePacket(pkt);
			return(1);
	}

	MICRO_TO_TICK(ticks,phyInfo->txtime);
	start_recvtimer(ticks);

	PHY_RXSTART_indication(_rxvector);
	cancel_MAC_recv = false;
	return (1);
}

void phy_80211p::recvHandler(Event *pkt)
{
	Event_  *ep;
	Packet *p;

	struct PHYInfo          *phyInfo;
	double                  *dist;
	char                    *input;
	char                    *tmp,*buffer,*output_PT , *output_PTS;
	int                     frameLen,mac_hdr_len,payload_len;
	double                  BER;
	double			*WRSS;

	u_int32_t		*psid;

	struct hdr_mac802_11    *mh,*mh_after;
	struct logEvent *logep;
	struct mac802_11_log *log1,*log2;
	char *iph;
	u_long ipsrc,ipdst;
	u_int32_t       *txnid;


	assert(epktRx);

	p = (Packet*) epktRx->DataInfo_;
	phyInfo =(struct PHYInfo*) p->pkt_getinfo("WAVEPhyinfo");
	dist = (double *) p->pkt_getinfo("dist");
	psid = (u_int32_t*) p->pkt_getinfo("PSID");
	WRSS = (double*) p->pkt_getinfo("WRSS");
	mh = (struct hdr_mac802_11*)p->pkt_getinfo("MAC_HEADER");
	txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");
	
	/*For Ad-hoc routing protocol purpose*/
        DSR_Info *dsr_info = (DSR_Info*)p->pkt_getinfo( "dsr" );

	assert(p && phyInfo && dist && WRSS && txnid);		

	//Decode DATA field
	input = p->pkt_sget();
	fec = phyInfo->fec;
	len = p->pkt_getlen() - channelCoding->getCodedBurstLen(3 , BPSK_1_2);	//subtract encoded SIGNAL length


	frameLen = channelCoding->getUncodedBurstLen(len, fec);

	mac_hdr_len = phyInfo->mac_hdr_length;
	payload_len = phyInfo->payload_len;

	//Decode 
	BER = channelModel->computeBER(fec, phyInfo->SNR);
#if DEBUG_PHY
	NSLOBJ_DEBUG("Receiver BER = %f\n",BER);
#endif	
	buffer =  (char*)malloc(frameLen);
	memset(buffer , 0 , frameLen);
	//Make bit errors & remove padding byte
	channelModel->makeBitError(input, len, BER);
	channelCoding->decode(input, buffer, len, fec);
	output_PT =  (char*)malloc(phyInfo->frame_len);
	memset(output_PT , 0 , phyInfo->frame_len);
	memcpy(output_PT,buffer,phyInfo->frame_len);
	free(buffer);
	tmp = output_PT +  mac_hdr_len;	
	
	u_long gateway = p->rt_gateway();
	p = new Packet;
	p->pkt_setflow(PF_RECV);
	p->rt_setgw(gateway);
	p->pkt_prepend(output_PT , mac_hdr_len);
	output_PTS = p->pkt_sattach(payload_len);
	p->pkt_sprepend(output_PTS, payload_len);
	memcpy(output_PTS,tmp,payload_len);

	/*hychen added for MAC_HEADER checking*/
	mh_after = (struct hdr_mac802_11*)output_PT;
	if(!bcmp(&(mh->dh_fc),&(mh_after->dh_fc),sizeof(struct frame_control)))
	{
		// Convey additional PHY information.
		p->pkt_addinfo("WAVEPhyinfo", (char *) phyInfo,sizeof(struct PHYInfo));
		p->pkt_addinfo("WRSS", (char *)WRSS ,sizeof(double));
		p->pkt_addinfo("LOG_TX_NID",(char*)txnid , sizeof(int));
		if (psid != NULL)
			p->pkt_addinfo("PSID", (char *) psid,sizeof(u_int32_t));
		/*For Ad-hoc routing protocol purpose*/
                if(dsr_info != NULL)
                        p->pkt_addinfo( "dsr", (const char*)dsr_info, sizeof( DSR_Info ));
		//simulation transmission time in Rx PHY to MAC
		ep = createEvent();
		ep->DataInfo_ = p->copy();

		put(ep, recvtarget_);

		PHY_RXEND_indication(NoError);
		PHY_CCA_indication(IDLE);
		delete (p);
		free(output_PT);
		freePacket(pkt);
		freePacket(epktRx);
		epktRx = NULL;
	}
	else	/*Log StartRx and DROPRx_MAC_HEADER_ERROR after foward error correction failed*/
	{
#if DEBUG_PHY
                NSLOBJ_DEBUG("DROP MAC HEADER ERROR\n");
#endif

		ssrc = *ssrc_p;
		slrc = *slrc_p;
		macmib->aRTSThreshold = *aRTSThreshold_p;
		iph = output_PTS;

		assert(mh && iph);

		GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

		log1 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));

		LOG_802_11(mh,log1,p,GetCurrentTime(),GetCurrentTime(),get_type(),get_nid(),StartRX,ipsrc,ipdst,_ChannelID,*txnid);
		INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

		log2 = (struct mac802_11_log*)malloc(sizeof(struct mac802_11_log));
		u_int64_t ticktime;
		MICRO_TO_TICK(ticktime,phyInfo->txtime);
		DROP_LOG_802_11(log1,log2,GetCurrentTime()+ticktime,DropRX,DROP_RX_MAC_HEADER_ERROR);
		INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);

		cancel_MAC_recv = true;
		PHY_CCA_indication(IDLE);
		delete p;
		free(output_PT);
		freePacket(pkt);
		freePacket(epktRx);
		epktRx = NULL;

	}
}


void phy_80211p::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void phy_80211p::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}


