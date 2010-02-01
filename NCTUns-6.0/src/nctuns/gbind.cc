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

#include <nctuns_api.h>
#include <gbind.h>


/*
 * Global Binding for TCL Script.
 */
int     RanSeed = 0;
int	TICK;
char	*SimSpeed;
int	pcluster;
char	*WireLogFlag;		// on/off
char	*WirelessLogFlag;	// on/off
char	*OphyLogFlag;		// on/off
char	*GPRSLogFlag;		// on/off
char    *WiMAXLogFlag;          // on/off
char    *MobileWIMAXLogFlag;    // on/off
char    *MobileRelayWIMAXLogFlag;    // on/off
char	*MR_WIMAX_NT_LogFlag;
char	*SatLogFlag;		// on/off
FILE	*fptr;
bool	ptrlogFileOpenFlag;	
char	*ptrlogFileName;
int	dumptunfd = -1;
char	*ObstacleFlag;		// on/off
bool    obstacleFileOpenFlag;	// on/off
char	*dynamicLocLogFlag;	// on/off
char	*dynamicPtrLogFlag;	// on/off
char	*WiMAXChannelCoding;    // on/off
char	*MobileWIMAXChannelCoding;    // on/off
char    *MobileRelayWIMAXChannelCoding;    // on/off
char	*MR_WIMAXChannelCoding_NT;    // on/off
char	*DVBChannelCoding;	// on/off
char	*WAVEChannelCoding;	// on/off
char	*WiFiChannelCoding;	// on/off

char	*GdbStart;		// on/off


int gBind_Init() {

	gBind("RandomNumberSeed", &RanSeed);
	gBind("TickToNanoSec", &TICK);
	gBind("SimSpeed", &SimSpeed);
	gBind("PCluster", &pcluster);
	gBind("WireLogFlag", &WireLogFlag);
	gBind("WirelessLogFlag", &WirelessLogFlag);
	gBind("OphyLogFlag",&OphyLogFlag);
	gBind("GPRSLogFlag",&GPRSLogFlag);
	gBind("WiMAXLogFlag",&WiMAXLogFlag);
	gBind("MobileWIMAXLogFlag",&MobileWIMAXLogFlag);
	gBind("MobileRelayWIMAXLogFlag",&MobileRelayWIMAXLogFlag);
	gBind("MR_WIMAX_NT_LogFlag", &MR_WIMAX_NT_LogFlag);
	gBind("SatLogFlag",&SatLogFlag);
	gBind("ptrlogFileName", &ptrlogFileName);
	gBind("ObstacleFlag", &ObstacleFlag);
	gBind("DynamicMovingPath", &dynamicLocLogFlag);
	gBind("OnlinePacketTransmission", &dynamicPtrLogFlag);
	gBind("WiMAXChannelCoding", &WiMAXChannelCoding);
	gBind("MobileWIMAXChannelCoding", &MobileWIMAXChannelCoding);
	gBind("MobileRelayWIMAXChannelCoding", &MobileRelayWIMAXChannelCoding);
	gBind("MR_WIMAXChannelCoding_NT", &MR_WIMAXChannelCoding_NT);
	gBind("DVBChannelCoding", &DVBChannelCoding);
	gBind("WAVEChannelCoding",&WAVEChannelCoding);
	gBind("WiFiChannelCoding",&WiFiChannelCoding);
	
	gBind("GdbStart", &GdbStart);

	TICK = 100;		// 1 tick = 100ns by default
	SimSpeed = NULL;
	pcluster = 1024;	// by default

	WireLogFlag 		= NULL;
	WirelessLogFlag 	= NULL;
	OphyLogFlag 		= NULL;
	GPRSLogFlag 		= NULL;
    	WiMAXLogFlag 		= NULL;
    	MobileWIMAXLogFlag  	= NULL;
	MobileRelayWIMAXLogFlag = NULL;
	SatLogFlag 		= NULL;
	ptrlogFileOpenFlag 	= false;
	ptrlogFileName 		= NULL;
	fptr 			= NULL;
	obstacleFileOpenFlag 	= false;
	dynamicLocLogFlag 	= NULL;
	dynamicPtrLogFlag 	= NULL;
	WiMAXChannelCoding 	= NULL;
	MobileWIMAXChannelCoding= NULL;
	MobileRelayWIMAXChannelCoding= NULL;
	DVBChannelCoding 	= NULL;
	WAVEChannelCoding	= NULL;
	WiFiChannelCoding	= NULL;	

	GdbStart		= NULL;

	return(1);
}



