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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <ethernet.h>
#include <sys/types.h>

#include <gbind.h>
#include <mac/mac-80211a-dcf.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>
#include <packet.h>
#include <phy/wphy.h>
#include <random.h>
#include <wphyInfo.h>
#include <tcpdump/wtcpdump.h>
#include <arpa/inet.h>
#include <arp/if_arp.h>
/*For Ad-hoc routing protocol information*/
#include<route/dsr/dsr_info.h>

/* for DEBUG */
#define VERBOSE_LEVEL   MSG_DEBUG
#include <wimax/mac/verbose.h>
/* for DEBUG end */

#define PKT_FLOW_DEBUG 0
#define PORT_MAPPING_DEBUG 0

MODULE_GENERATOR(mac802_11a_dcf); 

unsigned int crc_table_11a[256] = {
        0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
        0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
        0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
        0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
        0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
        0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
        0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
        0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
        0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
        0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
        0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
        0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
        0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
        0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
        0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
        0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
        0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
        0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
        0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
        0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
        0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
        0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
        0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
	0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
        0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
        0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
        0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
        0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
        0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
        0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
        0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
        0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
        0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
        0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
        0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
        0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
        0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
        0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
        0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
        0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
        0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
        0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
        0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
        0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
        0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
        0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
	0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
        0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
        0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
        0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
        0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
        0x2d02ef8dL
};

/* ========================================================================= */
#define DO1(buf) crc = crc_table_11a[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
/*========================================================================  
   Define Macros
  ========================================================================*/
#define CHECK_BACKOFF_TIMER()					\
{								\
	if (is_idle() && mhBackoff->paused_) {			\
		u_int64_t tm;					\
								\
 		MICRO_TO_TICK(tm, difs); 			\
		mhBackoff->resume(tm); 				\
	}							\
								\
	if (!is_idle() && mhBackoff->busy_ &&			\
	    !mhBackoff->paused_) {				\
		mhBackoff->pause(); 				\
	}							\
}

#define SET_TX_STATE(x)						\
{								\
	tx_state = (x);  					\
	CHECK_BACKOFF_TIMER(); 					\
}

#define SET_RX_STATE(x)						\
{								\
	rx_state = (x);  					\
	CHECK_BACKOFF_TIMER();	 				\
}


mac802_11a_dcf::mac802_11a_dcf(u_int32_t type, u_int32_t id, struct plist* pl, const char *name): NslObject(type, id, pl, name)
{
	bw_ = 0;
	_ptrlog = 0;
	mhNav = mhSend = mhDefer = mhBackoff = mhIF = mhRecv = NULL;
	macmib = NULL;
	phymib = NULL;

  	/* bind variable */
	vBind_mac("mac", mac_);
	vBind("PromisOpt", &promiscuous_bind);

	vBind("log", &_log);
	vBind("logInterval", &logInterval);
	vBind("NumUniInPkt", &LogUniIn);
	vBind("NumUniOutPkt", &LogUniOut);
	vBind("NumUniInOutPkt", &LogUniInOut);
	vBind("NumBroInPkt", &LogBroIn);
	vBind("NumBroOutPkt", &LogBroOut);
	vBind("NumBroInOutPkt", &LogBroInOut);
	vBind("NumCollision", &LogColl);
	vBind("NumDrop", &LogDrop);
	vBind("InThrput", &LogInThrput);
	vBind("OutThrput", &LogOutThrput);
	vBind("InOutThrput", &LogInOutThrput);

	vBind("UniInLogFile", &UniInLogFileName);
	vBind("UniOutLogFile", &UniOutLogFileName);
	vBind("UniInOutLogFile", &UniInOutLogFileName);
	vBind("BroInLogFile", &BroInLogFileName);
	vBind("BroOutLogFile", &BroOutLogFileName);
	vBind("BroInOutLogFile", &BroInOutLogFileName);
	vBind("CollLogFile", &CollLogFileName);
	vBind("DropLogFile", &DropLogFileName);
	vBind("InThrputLogFile", &InThrputLogFileName);
	vBind("OutThrputLogFile", &OutThrputLogFileName);
	vBind("InOutThrputLogFile", &InOutThrputLogFileName);

	/* initiate log variables */
	NumUniIn = 0;
	NumUniOut = 0;
	NumUniInOut = 0;
	NumBroIn = 0;
	NumBroOut = 0;
	NumBroInOut = 0;
	NumColl = 0;
	NumDrop = 0;
	InThrput = 0.0;
	OutThrput = 0.0;
	InOutThrput = 0.0;

	_log = 0;
	logInterval = 1;
	LogUniIn = 0;
	LogUniOut = 0;
	LogUniInOut = 0;
	LogBroIn = 0;
	LogBroOut = 0;
	LogBroInOut = 0;
	LogColl = 0;
	LogDrop = 0;
	LogInThrput = 0;
	LogOutThrput = 0;
	LogInOutThrput = 0;

	UniInLogFileName = NULL;
	UniOutLogFileName = NULL;
	UniInOutLogFileName = NULL;
	BroInLogFileName = NULL;
	BroOutLogFileName = NULL;
	BroInOutLogFileName = NULL;
	CollLogFileName = NULL;
	DropLogFileName = NULL;
	InThrputLogFileName = NULL;
	OutThrputLogFileName = NULL;
	InOutThrputLogFileName = NULL;

	/* register variable */
	REG_VAR("MAC", mac_); 
 
	/* initialize PHY-MIB paramaters */
	init_PHYMIB();

	/* initialize MAC-MIB paramaters */
	init_MACMIB();

	/* initialize timer used by MAC */
	init_Timer();

	/* promiscuous mode is set to off */
	promiscuous_bind = NULL;
	promiscuous = 0;

 	/* initialize buffers */
	epktCTRL = 0; epktRTS = 0;
  	epktTx = 0; epktRx = 0;
	epktBuf = 0; epktBEACON = 0;

        // Changed by Prof. Wang on 08/06/2003 here. Previously there is a Randomize()
        // function called here. However, this should not happen. Otherwise, the random
        // number seed will be reset every time when a new mobile node is created.


	/* init sequence number */
 	sta_seqno = Random() % 4096;

	/* init MAC state */
	rx_state = tx_state = MAC_IDLE;
	tx_active = 0;  

   	/* initialize others */
	nav = 0;  ssrc = slrc = 0; 
	cw = phymib->aCWmin;  

	/* init cache */
	bzero(cache, sizeof(struct dup_cache)*CACHE_SIZE); 

	for(int i=0;i<6;i++)
        {
                etherbroadcastaddr[i] = 0xff;
        }

	/* register variable  hychen added*/
	REG_VAR("promiscuous", &promiscuous);
	REG_VAR("RTS_THRESHOLD",&(macmib->aRTSThreshold));
	REG_VAR("SSRC",&ssrc);
        REG_VAR("SLRC",&slrc);
}


mac802_11a_dcf::~mac802_11a_dcf()
{
	if (NULL != phymib)
		free(phymib);

	if (NULL != macmib)
		free(macmib);

	if (NULL != epktCTRL)
		freePacket(epktCTRL);

	if (NULL != epktRTS)
		freePacket(epktRTS);

	if (NULL != epktTx)
		freePacket(epktTx);

	if (NULL != epktRx)
		freePacket(epktRx);
	
	if (NULL != epktBEACON)
                freePacket(epktBEACON);

	if (NULL != mhNav)
		delete mhNav;

	if (NULL != mhSend)
		delete mhSend;

	if (NULL != mhDefer)
		delete mhDefer;

	if (NULL != mhBackoff)
		delete mhBackoff;

	if (NULL != mhIF)
		delete mhIF;

	if (NULL != mhRecv)
		delete mhRecv;
	//delete CPThreshold_;
}


int mac802_11a_dcf::init_PHYMIB()
{
	phymib = (struct PHY_MIB_IN_MAC *)malloc(sizeof(struct PHY_MIB_IN_MAC));
 	assert(phymib);

	//initilize with channel spacing = 20Mz , hychen  , CHECK  channel spacing should be 20Mhz default
	phymib->aSlotTime = 9;  //us
        phymib->aSIFSTime = 16;
        phymib->aCCATime = 4;   //< 4 us
        phymib->aRxTxTurnaroundTime = 2;        // < 2us
        phymib->aAirPropagationTime = 1;
        phymib->aMACProcessingDelay = 2;
        phymib->aPreambleLength = 16;
        phymib->aPLCPHeaderLength = 4;
        phymib->aMPDUMaxLength = 4095;
        phymib->aCWmin = 15;
        phymib->aCWmax = 1023;

	return(1); 
}

int mac802_11a_dcf::init_MACMIB() {

	macmib = (struct MAC_MIB *)malloc(sizeof(struct MAC_MIB));
	assert(macmib); 

	/* bind variable */
	vBind("RTS_Threshold", (int *)&(macmib->aRTSThreshold));

	/* IEEE 802.11 Spec, section 11.4.4.2.15 */
	macmib->aRTSThreshold = 3000;

  	/* IEEE 802.11 Spec, section 11.4.4.2.16 */
	macmib->aShortRetryLimit = 7;

  	/* IEEE 802.11 Spec, section 11.4.4.2.17 */
	macmib->aLongRetryLimit = 4;

  	/* IEEE 802.11 Spec, section 11.4.4.2.18 */
	macmib->aFragmentationThreshold = 2346;

	return(1); 
}


int mac802_11a_dcf::init_Timer() {

	BASE_OBJTYPE(type);
 
	/* 
	 * Generate timer needed in 
	 * IEEE 802.11 MAC.
	 */
	mhNav     = new timerObj;
  	mhSend    = new timerObj;
  	mhDefer   = new timerObj;
  	mhBackoff = new timerObj;
  	mhIF      = new timerObj;
  	mhRecv    = new timerObj;
	assert( mhNav && mhSend && mhDefer &&
		mhBackoff && mhIF && mhRecv );

	/* 
	 * Associate timer with corresponding 
	 * time-handler method.
	 */
	type = POINTER_TO_MEMBER(mac802_11a_dcf, navHandler);
	mhNav->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11a_dcf, sendHandler);
 	mhSend->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11a_dcf, deferHandler);
	mhDefer->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11a_dcf, backoffHandler);
   	mhBackoff->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11a_dcf, txHandler);
 	mhIF->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11a_dcf, recvHandler);
 	mhRecv->setCallOutObj(this, type);

 	return(1); 
}


int mac802_11a_dcf::calcul_IFS() {
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();

	sifs = phymib->aSIFSTime;
	pifs = sifs + phymib->aSlotTime;   
  	difs = sifs + 2 * phymib->aSlotTime;
 	eifs = sifs + difs + TX_Time(MAC80211_ACK_LEN,(*bw_),nid,pid); 
 
	return(1); 
}


int mac802_11a_dcf::init() {

	/*
	 * Note:
	 * 	The variable "promiscuous" is a registerd variable,
	 *		REG_VAR("promiscuous", &promiscuous);
	 *	it's value might be modified by other modules.
         */

	if ( promiscuous_bind && !strcmp(promiscuous_bind, "on") )
		promiscuous = 1;
	else
		promiscuous = 0;

	if ( WirelessLogFlag && !strcasecmp(WirelessLogFlag, "on") ) {
		if ( !ptrlogFileOpenFlag ) {
			ptrlogFileOpenFlag = true;
			
			char	*ptrFile;
			if( ptrlogFileName ) {
				ptrFile = (char *)malloc(strlen
					(GetConfigFileDir())+strlen
					(ptrlogFileName) + 1);
				sprintf(ptrFile,"%s%s",GetConfigFileDir(),
							ptrlogFileName);
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}
			else {
				ptrFile = (char *)malloc(strlen
					(GetScriptName())+5);
				sprintf(ptrFile, "%s.ptr", GetScriptName());
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}

			if( fptr == NULL ) {
				printf("Error : Can't create file %s\n",ptrFile);
				exit(-1);
			}
	
			Event_ *heapHandle = createEvent();
			u_int64_t time;
			MILLI_TO_TICK(time, 100);
			u_int64_t chkInt = GetCurrentTime() + time;
			setEventTimeStamp(heapHandle, chkInt, 0);

			int (*__fun)(Event_ *) = 
			(int (*)(Event_ *))&DequeueLogFromHeap;;
			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}
		_ptrlog = 1;
	}

	char	*FILEPATH;

	if ( _log && (!strcasecmp(_log, "on")) ) {

		/* open log files */
		if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(UniInLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						UniInLogFileName);
			UniInLogFILE = fopen(FILEPATH,"w+");
			assert(UniInLogFILE);
			free(FILEPATH);
		}

		if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(UniOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						UniOutLogFileName);
			UniOutLogFILE = fopen(FILEPATH,"w+");
			assert(UniOutLogFILE);
			free(FILEPATH);
		}

		if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(UniInOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						UniInOutLogFileName);
			UniInOutLogFILE = fopen(FILEPATH,"w+");
			assert(UniInOutLogFILE);
			free(FILEPATH);
		}

		if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(BroInLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						BroInLogFileName);
			BroInLogFILE = fopen(FILEPATH,"w+");
			assert(BroInLogFILE);
			free(FILEPATH);
		}

		if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(BroOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						BroOutLogFileName);
			BroOutLogFILE = fopen(FILEPATH,"w+");
			assert(BroOutLogFILE);
			free(FILEPATH);
		}

		if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(BroInOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						BroInOutLogFileName);
			BroInOutLogFILE = fopen(FILEPATH,"w+");
			assert(BroInOutLogFILE);
			free(FILEPATH);
		}

		if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(CollLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						CollLogFileName);
			CollLogFILE = fopen(FILEPATH,"w+");
			assert(CollLogFILE);
			free(FILEPATH);
		}

		if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(DropLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						DropLogFileName);
			DropLogFILE = fopen(FILEPATH,"w+");
			assert(DropLogFILE);
			free(FILEPATH);
		}

		if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(InThrputLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						InThrputLogFileName);
			InThrputLogFILE = fopen(FILEPATH,"w+");
			assert(InThrputLogFILE);
			free(FILEPATH);
		}

		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(OutThrputLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						OutThrputLogFileName);
			OutThrputLogFILE = fopen(FILEPATH,"w+");
			assert(OutThrputLogFILE);
			free(FILEPATH);
		}

		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(InOutThrputLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						InOutThrputLogFileName);
			InOutThrputLogFILE = fopen(FILEPATH,"w+");
			assert(InOutThrputLogFILE);
			free(FILEPATH);
		}

		/* convert log interval to tick */
		SEC_TO_TICK(logIntervalTick, logInterval);

		/* set timer to log information periodically */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(mac802_11a_dcf, log);
		logTimer.setCallOutObj(this, type);
		logTimer.start(logIntervalTick, logIntervalTick);		
	}

	//hychen modified
	bw_ = GET_REG_VAR1(get_portls(), "bw", double *);
	ch_ = GET_REG_VAR1(get_portls(), "CHANNEL", int *);
	//CPThreshold_ = new double;

	*bw_ = (*bw_)*1000000;
        /*802.11a channels supported 34,36,38,40,42,44,46,48,52,56,60,64,149,153,157,161*/
        //*CPThreshold_ = 10;
        tx_suspend = 0;
	calcul_IFS();
	return(1); 

}



int mac802_11a_dcf::recv(ePacket_ *pkt) {

	Packet			*p;
	struct PHYInfo		*pinfo;
	u_int32_t		t; 

	struct hdr_mac802_11	*mh;
	struct mac802_11_log	*log1,*log2;
	char			*iph;
	u_long			ipdst,ipsrc;
        struct logEvent         *logep;
        u_int64_t               time_period;

	GET_PKT(p, pkt);
        assert(p);
        if (tx_active && p->pkt_err_ == 0) {
                p->pkt_err_ = 1;
		/* log "StartRX" and "DropRX" event */
		if(_ptrlog == 1){
			GET_PKT(p, pkt);
			u_int32_t *txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");
			pinfo = (struct PHYInfo *)p->pkt_getinfo("WPHY_80211a");
			mh = (struct hdr_mac802_11 *)p->pkt_get();
			iph = p->pkt_sget();

			GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);	

			log1 = (struct mac802_11_log*)malloc
				(sizeof(struct mac802_11_log));
			LOG_802_11(mh,log1,p,GetCurrentTime(),GetCurrentTime(),
				get_type(),get_nid(),StartRX,ipsrc,ipdst,*ch_,*txnid);
			INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

			log2 = (struct mac802_11_log*)malloc
				(sizeof(struct mac802_11_log));
			MICRO_TO_TICK(time_period,RX_Time(p->pkt_getlen(),
					pinfo->bw_));
			DROP_LOG_802_11(log1,log2,GetCurrentTime()+
				time_period,DropRX,DROP_RXERR);
			INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);
		}
	}

	/* count log metric */
	if( p->pkt_err_ == 1 ) {
		if( _log && !strcasecmp(_log, "on") ) {
			if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
				mh = (struct hdr_mac802_11 *)p->pkt_get();
				if( mh->dh_fc.fc_type == MAC_Type_Data )
					NumDrop++;
			}
		}
	}

		assert(!epktRx);  // just make sure 
 		epktRx = pkt;

		/* 
		 * Schedule a timer to simulate receive time
		 */
		GET_PKT(p, pkt); 
		assert(p);
		pinfo = (struct PHYInfo *)p->pkt_getinfo("WPHY_80211a");
		/*recv delay had been modified to PHY*/
		t = 0; 
		start_recvtimer(t);
		return(1);  
}


int mac802_11a_dcf::send(ePacket_ *pkt) {

	u_char			*dst;	/* dst. MAC */
	Packet			*p; 
 	struct hdr_mac802_11	*hm; 
	struct frame_control    *dh_fc;

	/*
	 * We check buffer, epktTx, to see if there is 
 	 * any packet in the MAC to be send.
	 */
	if (epktTx||epktBuf||epktBEACON)
        {
                return(-1); /* reject upper module's request */
        }
	GET_PKT(p, pkt); 

	assert(pkt && epktTx == NULL && epktBEACON ==NULL);
 
	/* Get destination MAC address */
	//ETHER_DHOST(dst, pkt); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();
	dh_fc = &(hm->dh_fc);
  	dst = hm->dh_addr1;  	
	if(hm->dh_fc.fc_type == MAC_Type_Management && hm->dh_fc.fc_subtype == MAC_Subtype_Beacon)
        {
                epktBEACON = pkt;       /* hold this packet into send Beacon buffer */
        }
        else
                epktTx = pkt;   /* hold this packet into send buffer */


	/* Generate IEEE 802.11 Data frmae */
	sendDATA(dst);

	/* Generate IEEE 802.11 RTS frame */
	sendRTS(dst);

 	/*
	 * Sense the medium state, if idle we should wait a DIFS time
	 * before transmitting; otherwise, we should start backoff timer.
	 */
	if (mhBackoff->busy_ == 0) {
		if (is_idle()) {
			/* 
			 * Medium is idle, we should start defer
			 * timer. If the defer timer is starting, there
			 * is no need to reset defer timer.
			 */
			if (mhDefer->busy_ == 0)
                                {
                                        if(dh_fc->fc_type == MAC_Type_Management)
                                        {
                                                u_int32_t timeout;
                                                timeout = ((Random() % cw) * phymib->aSlotTime);
                                                timeout += sifs;
                                                start_defer(timeout);

                                        }
                                        else
                                        {
                                                u_int32_t timeout;
                                                timeout = ((Random() % cw) * phymib->aSlotTime);
                                                timeout += difs;
                                                start_defer(timeout);
                                        }
                                }
		}
		else {	/* 
			 * Medium is not idle, we should start
			 * backoff timer.
			 */
			 start_backoff(dh_fc);
		}
	}
  	return(1); 
}


int mac802_11a_dcf::command(int argc, const char *argv[]) {

	return(NslObject::command(argc, argv));   
}



/*------------------------------------------------------------------------*
 * IEEE MAC 802.11 Implementation.
 *------------------------------------------------------------------------*/ 
inline void mac802_11a_dcf::set_NAV(u_int32_t _nav) {

	u_int64_t       curTime;
        u_int64_t       new_nav;

	curTime = GetCurrentTime();
	MICRO_TO_TICK(new_nav, _nav);
	new_nav += curTime;
	if (new_nav > nav) {
		nav = new_nav;
		if (mhNav->busy_)
			mhNav->cancel();
		mhNav->start((nav-GetCurrentTime()), 0);
	}
}


inline void mac802_11a_dcf::start_defer(u_int32_t us) {

	u_int64_t		inTicks;

 	MICRO_TO_TICK(inTicks, us);
 	mhDefer->start(inTicks, 0); 
}


inline void mac802_11a_dcf::start_sendtimer(u_int32_t us) {

	u_int64_t		inTicks;

 	MICRO_TO_TICK(inTicks, us);
	mhSend->start(inTicks, 0);
}


inline void mac802_11a_dcf::start_recvtimer(u_int32_t us) {

	u_int64_t		inTicks;
 
	MICRO_TO_TICK(inTicks, us);
	mhRecv->start(inTicks, 0);
}


inline void mac802_11a_dcf::start_IFtimer(u_int32_t us) {

	u_int64_t		inTicks;

 	MICRO_TO_TICK(inTicks, us);
	mhIF->start(inTicks, 0);
}


void mac802_11a_dcf::start_backoff(struct frame_control* dh_fc) {

	u_int32_t	timeout;	/* microsecond */
	u_int64_t	timeInTick; 


 	/*
	 * IEEE 802.11 Spec, section 9.2.4
	 *	
	 *	- Backoff Time = Random() x aSlotTime
	 *  	- Random() : Pseudorandom interger drawn from
	 * 		     a uniform distribution over the 
	 *		     interval [0, CW] where
	 * 		     aCWmin <=  CW <= aCWmax
	 */
	
	if(dh_fc->fc_type == MAC_Type_Management)
        {
                timeout = ((Random() % cw) * phymib->aSlotTime);
                timeout += sifs;
                MICRO_TO_TICK(timeInTick, timeout);
                mhBackoff->start(timeInTick, 0);
        }
        else
        {
                timeout = ((Random() % cw) * phymib->aSlotTime);
                timeout += difs;
                MICRO_TO_TICK(timeInTick, timeout);
                mhBackoff->start(timeInTick, 0);
        }

 	/* 
	 * Sense the medium, if busy, pause 
	 * the timer immediately.
	 */
	if (!is_idle())	{
		mhBackoff->pause(); 
	}
}


int mac802_11a_dcf::navHandler(Event_ *e)
{
	u_int32_t		timeInTick;
 
	/*
	 * IEEE 802.11 Spec, Figure 52
	 * 	- If medium idle, we should defer 
	 *	  difs time.
	 */
	if (is_idle() && mhBackoff->paused_) {
		MICRO_TO_TICK(timeInTick, difs); 
		mhBackoff->resume(timeInTick);
 	}

	return 1;
}


int mac802_11a_dcf::sendHandler(Event_ *e)
{
	struct hdr_mac802_11		*hm;
	Packet				*p;
	struct frame_control    *dh_fc;
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("tx_state = %d\n",tx_state);
#endif 
	switch(tx_state) {
		case MAC_RTS:
 			/*
			 * sent a RTS, but did not receive a CTS.
			 */
			retransmitRTS();
			break;

 		case MAC_CTS:
 			/*
			 * sent a CTS, but did not receive DATA packet
			 */
			assert(epktCTRL);
 			freePacket(epktCTRL);
 			epktCTRL = 0;
  			break;

		case MAC_SEND:
 			/*
			 * sent DATA, but did not receive an ACK
			 */
			if(epktTx!= NULL)
                                GET_PKT(p, epktTx);
                        else
                                GET_PKT(p, epktBEACON);
 			hm = (struct hdr_mac802_11 *)p->pkt_get();
  			dh_fc =  &(hm->dh_fc);
			if (bcmp(hm->dh_addr1, etherbroadcastaddr,
				ETHER_ADDR_LEN))
			{
				/* unicast address */
				retransmitDATA();
			}
			else {	/*
				 * IEEE 802.11 Spec, section 9.2.7
				 *	- In Broadcast and Multicast,
				 *	  no ACK should be received.
				 */
				if(epktTx!=NULL)
                                {
                                        freePacket(epktTx);
                                        epktTx = 0;
                                }
                                else
                                {
                                        freePacket(epktBEACON);
                                        epktBEACON = 0;
                                }

				rst_CW();
 				start_backoff(dh_fc); 
			}
 			break;

 		case MAC_ACK:
 			/*
			 * Sent an ACK, and now ready to 
			 * resume transmission
			 */
			assert(epktCTRL);
 			freePacket(epktCTRL);
 			epktCTRL = 0;
  			break;

		case MAC_IDLE:
			break;

		default:
    			assert(0); 
	}
	tx_resume(); 

	return 1;
}


int mac802_11a_dcf::deferHandler(Event_ *e)
{
	assert(epktCTRL || epktTx || epktRTS ||epktBEACON);
	
	if (check_pktCTRL() == 0)
		return(1); 

	assert(mhBackoff->busy_ == 0);   

	if (check_pktRTS() == 0)
  		return(1);

	if (check_pktBEACON() == 0)
                return(1);

	if (check_pktTx() == 0)
  		return(1); 

	return 0;
}


int mac802_11a_dcf::backoffHandler(Event_ *e)
{
	if (epktCTRL) {
		assert(mhSend->busy_ || mhDefer->busy_);
 		return(1);
	}
 
	if (check_pktRTS() == 0)
                return(1);

        if (check_pktBEACON() == 0)
                return(1);

	if (check_pktTx() == 0)
  		return(1); 

	return 0;
}


int mac802_11a_dcf::txHandler(Event_ *e)
{
	tx_active = 0;  

	return(1); 
}

int mac802_11a_dcf::recvHandler(Event_ *e)
{
	Packet *p;
	struct hdr_mac802_11 *hm;
    struct logEvent *logep;
    struct mac802_11_log *log1,*log2;
    char *iph;
    u_long ipsrc,ipdst;  

	assert(epktRx && 
	      (rx_state == MAC_RECV || rx_state == MAC_COLL));

	/*
	 * If the interface is in TRANSMIT mode when this packet
	 * "arrives", then I would never have seen it and should
	 * do a silent discard without adjusting the NAV.
	 */
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("1\n");
        NSLOBJ_DEBUG("tx_state = %d\n",tx_state);
#endif
	if (tx_active) {
		freePacket(epktRx);
 		goto done; 
	}
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("2\n");
#endif
	/*
	 * handle collisions.
	 */
	if (rx_state == MAC_COLL) {
		drop(epktRx, NULL);
		epktRx = NULL;
 		set_NAV(eifs);
 		goto done;
	}
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("3\n");
#endif
	/*
	 * Get packet header and txinfo from wphy
	 */
	GET_PKT(p, epktRx); 
 	hm = (struct hdr_mac802_11 *)p->pkt_get();  

	/*
	 * Check to see if his packet was received with enough
	 * bit errors.
	 */
	if (p->pkt_err_) {
		freePacket(epktRx);
 		set_NAV(eifs);
 		goto done; 
	}

	/*
	 * IEEE 802.11 Spec, section 9.2.5.6
	 * 	- update the Nav (Network Allocation Vector)
	 */
	if (bcmp(hm->dh_addr1, mac_, ETHER_ADDR_LEN))
		set_NAV(hm->dh_duration);

 	/*
	 * Address Filtering
	 */
	if (bcmp(hm->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN)&&
	    bcmp(hm->dh_addr1, mac_, ETHER_ADDR_LEN)) {
		if (!promiscuous) {
		   freePacket(epktRx); 
		   goto done; 
		}
	}


	/* log "StartRX" and "SuccessRX" event */
	if(_ptrlog == 1){
		GET_PKT(p, epktRx);
		struct PHYInfo *pinfo = (struct PHYInfo *)p->pkt_getinfo("WPHY_80211a");
		u_int32_t *txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");

		iph = p->pkt_sget();

		GET_IP_SRC_DST(hm,iph,ipsrc,ipdst);

		log1 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));

		u_int64_t txtime = RX_Time(p->pkt_getlen(), pinfo->bw_);
		u_int64_t ticktime;
		MICRO_TO_TICK(ticktime,txtime);	

		LOG_802_11(hm,log1,p,GetCurrentTime()-ticktime,
			GetCurrentTime()-ticktime,get_type(),
			get_nid(),StartRX,ipsrc,ipdst,*ch_,*txnid);
		INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

		log2 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));
		LOG_802_11(hm,log2,p,GetCurrentTime(),GetCurrentTime()-ticktime,
			get_type(),get_nid(),SuccessRX,ipsrc,ipdst,*ch_,*txnid);
		INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);
	}
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("4\n");
#endif
	switch(hm->dh_fc.fc_type) {

	case MAC_Type_Management:
		recvDATA(epktRx);
		goto done; 

	case MAC_Type_Control:
 		switch(hm->dh_fc.fc_subtype) {

		case MAC_Subtype_RTS:
			recvRTS(epktRx);
 			break; 

 		case MAC_Subtype_CTS:
			recvCTS(epktRx);
 			break; 

		case MAC_Subtype_ACK:
			recvACK(epktRx);
 			break; 

		default: 
			ErrorMesg("Invalid MAC Control Subtype !");
			//NSLOBJ_DEBUG("Invalid MAC Control Subtype !\n");
		}
		break;  

	case MAC_Type_Data: 
		switch(hm->dh_fc.fc_subtype) {
			
		case MAC_Subtype_Data:
		case MAC_Subtype_QoS_Data:
			/* count log metric */
			if( _log && !strcasecmp(_log, "on") ) {
				if( !bcmp(hm->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN) ) {
					if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) )
						NumBroIn++;
					if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) )
						NumBroInOut++;
				}
				else {
					if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) )
						NumUniIn++;
					if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
						NumUniInOut++;
				}
				
				GET_PKT(p, epktRx);	
				if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) )
					InThrput += (double)p->pkt_getlen() / 1000.0;
				if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
					InOutThrput += (double)p->pkt_getlen() / 1000.0;
			}

 			recvDATA(epktRx);
 			break;

		default: ErrorMesg("Invalid MAC Data Subtype");
		//default: NSLOBJ_DEBUG("Invalid MAC Data Subtype\n");
		}
		break;
 
	default: ErrorMesg("Invalid MAC Type");   
	//default: NSLOBJ_DEBUG("Invalid MAC Type\n");
	}

	done:
	
 	epktRx = 0;
  	rx_resume(); 

	return 1;
}

int mac802_11a_dcf::check_pktCTRL() {

	struct hdr_mac802_11		*hm; 
	u_int32_t			timeout;
	Packet				*p;
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();
 
	
	if (epktCTRL == 0)
   		return(-1);
  	if (tx_state == MAC_CTS || tx_state == MAC_ACK)
    		return(-1);
 	
	GET_PKT(p, epktCTRL); 
 	hm = (struct hdr_mac802_11 *)p->pkt_get();
  	
	switch(hm->dh_fc.fc_subtype) {
		case MAC_Subtype_CTS:
 			/* If medium is not idle,
			 * don't send CTS.
			 */
			if (!is_idle()) {
				drop(epktCTRL, NULL);
 				epktCTRL = 0;
  				return(0); 
			}
			SET_TX_STATE(MAC_CTS);
			// ????????? why needs duration ??????????
			// hychen added
 			timeout = hm->dh_duration + CTS_Time(nid,pid);
 			break;

 		/*
		 * IEEE 802.11 Spec, section 9.2.8
		 * Acknowledgments are sent after an SIFS ,without
		 * regard to the busy/idle state of the medium.
		 */
		case MAC_Subtype_ACK:
 			SET_TX_STATE(MAC_ACK);
 			timeout = ACK_Time(nid,pid);
  			break;

		default:
 			ErrorMesg("Invalid MAC Control Type !");
			//NSLOBJ_DEBUG("Invalid MAC Control Type !\n");
	}
	transmit(epktCTRL, timeout);
 	return(0); 
}


int mac802_11a_dcf::check_pktRTS() {

	struct hdr_mac802_11		*hm;
	struct frame_control            *dh_fc;
	u_int32_t			timeout; 
	Packet				*p;
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();
 

	assert(mhBackoff->busy_ == 0);
	if (epktRTS == 0)
  		return(-1);

	GET_PKT(p, epktRTS); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();
 	dh_fc =  &(hm->dh_fc);
	switch(hm->dh_fc.fc_subtype) {
		case MAC_Subtype_RTS:
			if (!is_idle()) {
				inc_CW();
 				start_backoff(dh_fc);
 				return(0); 
			}
			SET_TX_STATE(MAC_RTS);
			//hychen added
 			timeout = CTSTimeout(nid,pid);  
			break;

 		default:
 			ErrorMesg("Invalid MAC Control Type !"); 
	}
	transmit(epktRTS, timeout);
 	return(0);
}

int mac802_11a_dcf::check_pktBEACON()
{

        struct hdr_mac802_11            *hm;
        struct frame_control            *dh_fc;
        u_int32_t                       timeout;
        Packet                          *p;
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();

        assert(mhBackoff->busy_ == 0);
        if (epktBEACON == 0)
                return(-1);

        GET_PKT(p, epktBEACON);
        hm = (struct hdr_mac802_11 *)p->pkt_get();
        dh_fc =  &(hm->dh_fc);

        switch(hm->dh_fc.fc_subtype) {
        case MAC_Subtype_Beacon:
                if(! is_idle()) {
                        inc_CW();
                        start_backoff(dh_fc);
                        return 0;
                }
                SET_TX_STATE(MAC_SEND);
                timeout = TX_Time(p->pkt_getlen(),(*bw_),nid,pid);
                break;
        default:
                        ErrorMesg("Invalid MAC Management BEACON Subtype !");
        }
        transmit(epktBEACON, timeout);

        return 0;
}

int mac802_11a_dcf::check_pktTx() {

	struct hdr_mac802_11		*hm;
	u_int32_t			timeout;
 	Packet				*p;
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();
	struct frame_control            *dh_fc;
 
 
	assert(mhBackoff->busy_ == 0);
 
	if (epktTx == 0)
  		return(-1); 

	GET_PKT(p, epktTx); 
 	hm = (struct hdr_mac802_11 *)p->pkt_get();
	dh_fc =  &(hm->dh_fc);
  	switch(hm->dh_fc.fc_subtype) {
		case MAC_Subtype_Data:
		case MAC_Subtype_Asso_Req:
		case MAC_Subtype_Asso_Resp:
		case MAC_Subtype_ReAsso_Req:
		case MAC_Subtype_ReAsso_Resp:
		case MAC_Subtype_Probe_Req:
		case MAC_Subtype_Probe_Resp:
		case MAC_Subtype_Beacon:
		case MAC_Subtype_DisAsso:
		case MAC_Subtype_ApInfo:
			/*
			 * If the medium is busy, we should
			 * redo RTS/CTS/DATA/ACK.
			 */
			if (!is_idle()) {
				sendRTS(hm->dh_addr1);
 				inc_CW();
 				start_backoff(dh_fc); 
				return(0); 
			}
			SET_TX_STATE(MAC_SEND);

			if (bcmp(hm->dh_addr1, etherbroadcastaddr, 
				ETHER_ADDR_LEN))
 				timeout = ACKTimeout(p->pkt_getlen(),nid,pid);
 			else {	
				/* XXX: broadcasat 
				 * sent a braodcast packet and no
				 * ack need.  ???????????????????
				 */
				//timeout = DATA_Time(p->pkt_getlen()); 
				//hychen modified
				timeout = TX_Time(p->pkt_getlen(),(*bw_),nid,pid);
#if PKT_FLOW_DEBUG

                                NSLOBJ_DEBUG("TX_time = %u(tick)\n", timeout*10);
#endif
			}
			break; 

 		default:
 			ErrorMesg("Invalid MAC Control Subtype !"); 
 			//NSLOBJ_DEBUG("Invalid MAC Control Subtype !\n");
	}
	transmit(epktTx, timeout); 
	return(0); 
}


void mac802_11a_dcf::sendRTS(u_char *dst) {

	int				pktlen; 
	Packet				*p;
	struct rts_frame		*rf;
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();
 

	if(epktTx!=NULL)
        {
                assert(epktTx && !epktRTS);
                GET_PKT(p, epktTx);
        }
        else
        {
                assert(epktBEACON && !epktRTS);
                GET_PKT(p, epktBEACON);
        }

	pktlen = p->pkt_getlen() - sizeof(struct hdr_mac802_11); 

	/*
	 * IEEE 802.11 Spec, section 9.2
	 *	- The RTS/CTS mechanism cannot be used for MPDUS with
	 *	  broadcast and multicast immediate address.
	 *
	 * IEEE 802.11 Spec, section 11.4.4.2.15
	 *	- If the size of packet is large than or equal to 
	 *	  RTSThreshold, we should perform RTS/CTS handshake.
	 */
	if ((pktlen < (int)macmib->aRTSThreshold) || 
	    !bcmp(dst, etherbroadcastaddr, ETHER_ADDR_LEN)) 
	{
		/* shouldn't perform RTS/CTS */
		return; 
	}

	/* Generate an IEEE 802.11 RTS frame */
	assert(epktRTS == NULL);
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("Generate RTS packet!!\n");
#endif
	epktRTS = createEvent();
	p = new Packet; assert(p); 
	rf = (struct rts_frame *)p->pkt_malloc(sizeof(struct rts_frame));  
 	assert(rf);
	ATTACH_PKT(p, epktRTS); 

	rf->rf_fc.fc_protocol_version = MAC_ProtocolVersion;
 	rf->rf_fc.fc_type       = MAC_Type_Control;
	rf->rf_fc.fc_subtype    = MAC_Subtype_RTS;
	rf->rf_fc.fc_to_ds      = 0;
	rf->rf_fc.fc_from_ds    = 0;
	rf->rf_fc.fc_more_frag  = 0;
	rf->rf_fc.fc_retry      = 0;
	rf->rf_fc.fc_pwr_mgt    = 0;
	rf->rf_fc.fc_more_data  = 0;
	rf->rf_fc.fc_wep        = 0;
	rf->rf_fc.fc_order      = 0;

	rf->rf_duration = RTS_DURATION(pktlen,nid,pid);
 	memcpy(rf->rf_ta, mac_, ETHER_ADDR_LEN); /* source addr. */
  	memcpy(rf->rf_ra, dst, ETHER_ADDR_LEN);  /* destination addr. */
	
}


void mac802_11a_dcf::sendCTS(u_char *dst, u_int16_t d) {

	struct cts_frame		*cf;
	Packet				*p;
 	u_int32_t			nid = get_nid();
	u_int32_t			pid = get_port();
 	

 	assert(epktCTRL == 0); 

	/* Generate a CTS frame */
	epktCTRL = createEvent();
	p = new Packet; assert(p); 
  	cf = (struct cts_frame *)p->pkt_malloc(sizeof(struct cts_frame));
	assert(cf); 
	ATTACH_PKT(p, epktCTRL); 

	cf->cf_fc.fc_protocol_version = MAC_ProtocolVersion;
	cf->cf_fc.fc_type       = MAC_Type_Control;
	cf->cf_fc.fc_subtype    = MAC_Subtype_CTS;
	cf->cf_fc.fc_to_ds      = 0;
	cf->cf_fc.fc_from_ds    = 0;
	cf->cf_fc.fc_more_frag  = 0;
	cf->cf_fc.fc_retry      = 0;
	cf->cf_fc.fc_pwr_mgt    = 0;
	cf->cf_fc.fc_more_data  = 0;
	cf->cf_fc.fc_wep        = 0;
	cf->cf_fc.fc_order      = 0;

	cf->cf_duration = CTS_DURATION(d,nid,pid);
	(void)memcpy(cf->cf_ra, dst, ETHER_ADDR_LEN); /* destination addr. */ 

}


void mac802_11a_dcf::sendACK(u_char *dst) {

	struct ack_frame		*af;
	Packet				*p;
 	//u_int32_t			nid;
 

	assert(epktCTRL == 0);
	
	/* Generate ACK Frame */
	epktCTRL = createEvent();
	p = new Packet; assert(p); 
  	af = (struct ack_frame *)p->pkt_malloc(sizeof(struct ack_frame)); 
	assert(af);
 	ATTACH_PKT(p, epktCTRL); 

  	af->af_fc.fc_protocol_version = MAC_ProtocolVersion;
	af->af_fc.fc_type       = MAC_Type_Control;
	af->af_fc.fc_subtype    = MAC_Subtype_ACK;
	af->af_fc.fc_to_ds      = 0;
	af->af_fc.fc_from_ds    = 0;
	af->af_fc.fc_more_frag  = 0;
	af->af_fc.fc_retry      = 0;
	af->af_fc.fc_pwr_mgt    = 0;
	af->af_fc.fc_more_data  = 0;
	af->af_fc.fc_wep        = 0;
	af->af_fc.fc_order      = 0;

	af->af_duration = ACK_DURATION();
	(void)memcpy(af->af_ra, dst, ETHER_ADDR_LEN); /* destination addr. */

}


void mac802_11a_dcf::sendDATA(u_char *dst) {

	struct hdr_mac802_11		*mh;
  	Packet				*p;
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();

 	
	if(epktTx != NULL)
                GET_PKT(p, epktTx);
        else
                GET_PKT(p, epktBEACON);
	mh = (struct hdr_mac802_11 *)p->pkt_get();
  
#if 0
	/* MoblNode and AP modules would set the following values */

	/* Encapsulate IEEE 802.11 header. */
	mh = (struct hdr_mac802_11 *)
	      p->pkt_malloc(sizeof(struct hdr_mac802_11)); 
	assert(mh); 

	/* fill control field */
	mh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
 	mh->dh_fc.fc_type 	= MAC_Type_Data;
 	mh->dh_fc.fc_subtype	= MAC_Subtype_Data;
   	mh->dh_fc.fc_to_ds	= 0;
 	mh->dh_fc.fc_from_ds	= 0;
  	mh->dh_fc.fc_more_frag	= 0;
  	mh->dh_fc.fc_retry	= 0;
  	mh->dh_fc.fc_pwr_mgt	= 0;
	mh->dh_fc.fc_more_data	= 0;
	mh->dh_fc.fc_wep	= 0;
  	mh->dh_fc.fc_order	= 0;
#endif

	/*
	 * IEEE 802.11 Spec, section 7.1.3.4.1
	 *	- sequence number field
	 */
	sta_seqno = (sta_seqno+1) % 4096;   
	mh->dh_scontrol = sta_seqno;
    	
	if (bcmp(etherbroadcastaddr, dst, ETHER_ADDR_LEN)) {		
		/* unicast address */
		mh->dh_duration = DATA_DURATION(nid,pid); 
		memcpy(mh->dh_addr1, dst, ETHER_ADDR_LEN);
 		memcpy(mh->dh_addr2, mac_, ETHER_ADDR_LEN); 
	}
	else {	/* broadcast address */
		mh->dh_duration = 0;   
		memcpy(mh->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN);
 		memcpy(mh->dh_addr2, mac_, ETHER_ADDR_LEN); 
	}
}


void mac802_11a_dcf::transmit(ePacket_ *pkt, u_int32_t timeout)
{
	Packet *p, *RxPkt; 
	Event *ep; 
	struct mac802_11_log *log1,*log2;
	char *iph;
	u_long ipdst,ipsrc;
	struct logEvent *logep;
	u_int64_t time_period;

	struct TXVECTOR *tx;
	struct hdr_mac802_11 *mh=NULL;

	tx_active = 1;
	GET_PKT(p, pkt);

	//DSR_Info *dsr_info = (DSR_Info*)p->pkt_getinfo( "dsr" );
 	
	struct frame_control    *fc;
        struct ether_header     *eh;
        struct ack_frame        *af;
        struct rts_frame        *rf;
        struct cts_frame        *cf;
        int     f_len,pl_len;
        unsigned int crc;
        char    *tmp,*buf;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();

        fc = (struct frame_control*)p->pkt_get();
	//hychen added for CRC calculation
        if ((fc->fc_type == MAC_Type_Control) && (fc->fc_subtype ==MAC_Subtype_ACK))	//Calculate CRC for ACK
        {
                af = (struct ack_frame*)p->pkt_get();
                f_len = sizeof(struct ack_frame) - ETHER_FCS_LEN;
                buf = (char*)malloc(f_len);
                memset(buf,0,f_len);
                memcpy(buf,af,f_len);
                crc = crc32(0,buf,f_len);
#if PKT_FLOW_DEBUG
                NSLOBJ_DEBUG("CRC = %08x\n",crc);
#endif
                memset(af->af_fcs,0,4);
                memcpy(af->af_fcs,&crc,4);
                free(buf);
	}else if ((fc->fc_type == MAC_Type_Control) && (fc->fc_subtype ==MAC_Subtype_RTS))	//Calculate CRC for RTS
	{
		rf = (struct rts_frame*)p->pkt_get();
		f_len = sizeof(struct rts_frame) - ETHER_FCS_LEN;
                buf = (char*)malloc(f_len);
                memset(buf,0,f_len);
                memcpy(buf,rf,f_len);
                crc = crc32(0,buf,f_len);
#if PKT_FLOW_DEBUG
                NSLOBJ_DEBUG("CRC = %08x\n",crc);
#endif
                memset(rf->rf_fcs,0,4);
                memcpy(rf->rf_fcs,&crc,4);
                free(buf);
        }
        else if ((fc->fc_type == MAC_Type_Control) && (fc->fc_subtype ==MAC_Subtype_CTS))	//Calculate CRC for CTS
        {
                cf = (struct cts_frame*)p->pkt_get();
                f_len = sizeof(struct cts_frame) - ETHER_FCS_LEN;
                buf = (char*)malloc(f_len);
                memset(buf,0,f_len);
                memcpy(buf,cf,f_len);
                crc = crc32(0,buf,f_len);
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC = %08x\n",crc);
#endif
                memset(cf->cf_fcs,0,4);
                memcpy(cf->cf_fcs,&crc,4);
                free(buf);
        }
	else if(fc->fc_type == MAC_Type_Management)
	{
		mh = (struct hdr_mac802_11 *)p->pkt_get();
		tmp = p->pkt_sget();
                pl_len = p->pkt_getlen() - sizeof(struct hdr_mac802_11);
                f_len = p->pkt_getlen() - ETHER_FCS_LEN;
                buf = (char*)malloc(f_len);
		memset(buf,0,f_len);
		memcpy(buf,mh,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
		memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,tmp,pl_len);
		crc = crc32(0,buf,f_len);
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC = %08x\n",crc);
#endif
                memset(mh->dh_fcs,0,4);
                memcpy(mh->dh_fcs,&crc,4);
                free(buf);
		
	}
	else	//Calculate CRC for DATA frame
	{
		mh = (struct hdr_mac802_11 *)p->pkt_get();
                eh=(struct ether_header *)(p->pkt_get()+sizeof(struct hdr_mac802_11));
                tmp = p->pkt_sget();
		f_len = p->pkt_getlen() - ETHER_FCS_LEN;
		pl_len = p->pkt_getlen() - sizeof(struct hdr_mac802_11);
		buf = (char*)malloc(f_len);
		if(tmp !=NULL)
		{
			char* pt_ptr = p->pkt_get()+sizeof(struct hdr_mac802_11);
                        int pt_len = p->pkt_get_pt_data_len() - sizeof(struct hdr_mac802_11);
                        pl_len = p->pkt_getlen() - p->pkt_get_pt_data_len();
                        memset(buf,0,f_len);
                        memcpy(buf,mh,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
                        memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,pt_ptr,pt_len);
                        memcpy(buf+p->pkt_get_pt_data_len()-ETHER_FCS_LEN,tmp,pl_len);
			crc = crc32(0,buf,f_len);
		}
		else
		{
			tmp = p->pkt_get()+sizeof(struct hdr_mac802_11);
                        memset(buf,0,f_len);
                        memcpy(buf,mh,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
                        memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,tmp,pl_len);
                        crc = crc32(0,buf,f_len);
		}
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC = %08x\n",crc);
#endif
		memset(mh->dh_fcs,0,4);
		memcpy(mh->dh_fcs,&crc,4);
		free(buf);
	}
	/*
	 * If I'm transmitting without doing CS, such as when
	 * sending an ACK, any incoming packet will be "missed"
	 * and hence, must be discared.
	 */
 	if (rx_state != MAC_IDLE) {
 		mh = (struct hdr_mac802_11 *)p->pkt_get();

  		assert(mh->dh_fc.fc_type == MAC_Type_Control);
		assert(mh->dh_fc.fc_subtype == MAC_Subtype_ACK);
   		//assert(epktRx);
   		//hychen modified
   		if (epktRx) {   //XXX : should be log ?
                        GET_PKT(RxPkt, epktRx);
                        /* force packet discard */
                        RxPkt->pkt_err_ = 1;
                } else if (((phy_80211a*)InstanceLookup(nid,pid,"phy_80211a"))->mhRecv->busy_ ) {
                        ((phy_80211a*)InstanceLookup(nid,pid,"phy_80211a"))->mhRecv->cancel();	
			((phy_80211a*)InstanceLookup(nid,pid,"phy_80211a"))->PHY_CCARESET_request();
                }

	}
#if PKT_FLOW_DEBUG
	printf("\t\ttx_state = %d, timeout = %u,pkt size = [%d]\n", tx_state, timeout*10,p->pkt_getlen());
#endif
	//hychen added
	 if( is_PHY_idle() )  //if PHY idle
        {
                tx = new TXVECTOR;
		memset(tx,0,sizeof(struct TXVECTOR));
                tx->LENGTH = p->pkt_getlen();
                tx->DATARATE = *bw_/1000000;
                ((phy_80211a*)InstanceLookup(nid,pid,"phy_80211a"))->PHY_TXSTART_request(*tx);
		delete tx;
        }
        else
        {
                printf("Busy\n");
                tx_suspend = 1;
		return;
	}

	
	/* start send timer */
	start_sendtimer(timeout); 
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("ACKTIMEOUT_time = %u(tick)\n", timeout*10);
#endif

	/* start Interface timer */
	timeout = TX_Time(p->pkt_getlen(),(*bw_),nid,pid);
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("TX_time = %u(tick)\n", timeout*10);  
#endif
	start_IFtimer(timeout); 

	/* count log metric : 
	 *
	 * Note:
	 *	Broadcast packets don't expect to receive ACK packets,
	 * 	therefore the NumBroOut metric is counted here.
	 *	Unicast packets expect to receive ACK packets, therefore
	 *	the NumUniOut metric is counted in recvACK(). 
	 */
	if( _log && !strcasecmp(_log, "on") ) {
		mh = (struct hdr_mac802_11 *)p->pkt_get();
		if ( tx_state == MAC_SEND	/* means send data packets */ 
	             && !bcmp(mh->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN)) {
			if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) )
				NumBroOut++;
			if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) )
				NumBroInOut++;
		
			GET_PKT(p, pkt);
			if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
				OutThrput += (double)p->pkt_getlen() / 1000.0;
			if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
				InOutThrput += (double)p->pkt_getlen() / 1000.0;
		}
	}	

	p->pkt_addinfo("LOG_TX_NID",(char*)&nid , sizeof(int));
	/* log "StartTX" and "SuccessTX" event */
	if(_ptrlog == 1){
		if(mh == NULL)
			mh = (struct hdr_mac802_11 *)p->pkt_get();
		/*Record node id for Log purpose at Rx side*/

		iph = p->pkt_sget();

		GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

		log1 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));
		LOG_802_11(mh,log1,p,GetCurrentTime(),GetCurrentTime(),
			get_type(),get_nid(),StartTX,ipsrc,ipdst,*ch_,nid);
		INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

		log2 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));
		MICRO_TO_TICK(time_period,timeout);
		LOG_802_11(mh,log2,p,GetCurrentTime()+time_period,
			GetCurrentTime(),get_type(),get_nid(),SuccessTX,
			ipsrc,ipdst,*ch_,nid);
		INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);
	}

	/*
	 * Note XXX:
	 *	We should duplicate a packet and pass this 
	 * duplicated packet to next module. Because the
	 * MAC will temporarily hold the sent packet for 
	 * retransmission and the duplicated packet will be
	 * release in lowe module.
	 *
	 * BUG BUG:
	 * What about pkt?? No one to handle it!
	 */
	ep = pkt_copy(pkt);

	//freePacket(pkt);

	assert(put(ep, sendtarget_)); 
}


void mac802_11a_dcf::retransmitRTS() {

	struct rts_frame		*rf;
	Packet				*p;
	ePacket_			*dupP_;
	struct frame_control            *dh_fc;

	
	if(epktTx!=NULL)
                assert(epktTx && epktRTS);
        else
                assert(epktBEACON && epktRTS);
 	assert(mhBackoff->busy_ == 0);

   	macmib->aRTSFailureCount ++;
   	ssrc ++;	// STA Short Retry Count

   	if (ssrc >= macmib->aShortRetryLimit) {
        /*
         * For mobile node routing protocol.
		 * If packet be dropped due to retry count
		 * >= retry limit, we call an error handler.
		 */
		if(epktTx!=NULL)
                {
                        dupP_ = pkt_copy(epktTx);  // for dropping message 
                        GET_PKT(p, dupP_);
                }
                else
                {
                        dupP_ = pkt_copy(epktBEACON);  // for dropping message 
                        GET_PKT(p, dupP_);
                }

		p->pkt_seek(sizeof(struct hdr_mac802_11) +
		            sizeof(struct ether_header));
		if (-1 == p->pkt_callout(dupP_)) {
//			printf("mac802_11a_dcf::retransmitRTS: pkt_callout failed\n");
			freePacket(dupP_);
		}

		/*
		 * IEEE 802.11 Spec, section 11.4.4.2.16
		 * IEEE 802.11 Spec, section 9.2.5.3
		 */
		drop(epktRTS, NULL);
                if(epktTx!=NULL)
                {
                        drop(epktTx, NULL);
                        epktRTS = epktTx = 0;
                }
                else
                {
                        drop(epktBEACON, NULL);
                        epktRTS = epktBEACON = 0;
                }
		ssrc = 0; rst_CW();   
	}
 	else {
		GET_PKT(p, epktRTS); 
 		rf = (struct rts_frame *)p->pkt_get();
   		rf->rf_fc.fc_retry = 1;
  		dh_fc = &(rf->rf_fc);
		inc_CW(); 

		/* IEEE 802.11 Spec, section 9.2.5.7 */
		start_backoff(dh_fc); 
	}
}


void mac802_11a_dcf::retransmitDATA() {

	struct hdr_mac802_11	*hm;
	Packet			*p;
	ePacket_		*dupP_;
	int			pktlen; 
 	u_int16_t		*rcount, *thresh;
	struct frame_control    *dh_fc;
 
	assert(mhBackoff->busy_ == 0);
   	assert(epktTx && epktRTS == 0);
   
	macmib->aACKFailureCount++;
   	
	GET_PKT(p, epktTx); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();  
	dh_fc = &(hm->dh_fc);
 	pktlen = p->pkt_getlen() - sizeof(struct hdr_mac802_11); 

	if (pktlen < (int)macmib->aRTSThreshold) {
		rcount = &ssrc;
  		thresh = &macmib->aShortRetryLimit;  
	}
	else {
		rcount = &slrc;
  		thresh = &macmib->aLongRetryLimit;  
	}

	(*rcount) ++;
   	
	if (*rcount > *thresh) {
		/*
         * For mobile node routing protocol.
		 * If packet be dropped due to retry count
		 * >= retry limit, we call an error handler.
		 */
		dupP_ = pkt_copy(epktTx); // for dropping message 
        GET_PKT(p, dupP_);
		p->pkt_seek(sizeof(struct hdr_mac802_11) +
		            sizeof(struct ether_header));
        if (-1 == p->pkt_callout(dupP_)) {
//			printf("mac802_11a_dcf::retransmitDATA, pkt_callout failed\n");
			freePacket(dupP_);
		}

		macmib->aFailedCount++;
   		drop(epktTx, NULL); epktTx = 0;
		*rcount = 0;
  		rst_CW(); 
	}
	else {
		hm->dh_fc.fc_retry = 1;
  		sendRTS(hm->dh_addr1);
 		inc_CW();

		/* IEEE 802.11 Spec, section 9.2.5.2 */
 		start_backoff(dh_fc); 
	}
}


void mac802_11a_dcf::recvRTS(ePacket_ *p) {

	struct rts_frame		*rf;
	Packet				*pkt;

	int f_len;
        unsigned int crc,fcs;
        char *buf;


 	if (tx_state != MAC_IDLE) {
		drop(p, NULL);
		p = NULL;
 		return; 
	} 

	/*
	 * If I'm responding to someone else, discard the RTS.
	 */
	if (epktCTRL) {
		drop(p, NULL);
		p = NULL;
 		return;
 	}

	GET_PKT(pkt, p); 
 	rf = (struct rts_frame *)pkt->pkt_get();
  
	//hychen added for CRC32 check
	f_len = sizeof(struct rts_frame) - ETHER_FCS_LEN;
        buf = (char*)malloc(f_len);
        memset(buf,0,f_len);
        memcpy(buf,rf,f_len);
        crc = crc32(0,buf,f_len);
        memcpy(&fcs,rf->rf_fcs, 4);
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("CRC = %08x , FCS = %08x\n",crc,fcs);
#endif
        free(buf);

        if (memcmp(&crc, rf->rf_fcs, 4) != 0) {
                NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
                drop(p, NULL);
                p = NULL;
                return;
        }
	sendCTS(rf->rf_ta, rf->rf_duration);
 	
	/* stop deffering */
	if (mhDefer->busy_)
		mhDefer->cancel();

	freePacket(p); 
 	tx_resume();
}


void mac802_11a_dcf::recvCTS(ePacket_ *p) {

	struct cts_frame                *cf;
        Packet                          *pkt;

        int f_len;
        unsigned int crc,fcs;
        char *buf;

#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("tx_state = %d\n",tx_state);
#endif

	if (tx_state != MAC_RTS) {
 		drop(p, NULL);
		p = NULL;
 		return;
 	}

	//hychen added for CRC32 check
	GET_PKT(pkt, p);
        cf = (struct cts_frame *)pkt->pkt_get();
        f_len = sizeof(struct cts_frame) - ETHER_FCS_LEN;
        buf = (char*)malloc(f_len);
        memset(buf,0,f_len);
        memcpy(buf,cf,f_len);
        crc = crc32(0,buf,f_len);
        memcpy(&fcs,cf->cf_fcs, 4);
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("CRC = %08x , FCS = %08x\n",crc,fcs);
#endif
        free(buf);

        if (memcmp(&crc, cf->cf_fcs, 4) != 0) {
                NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
                drop(p, NULL);
                p = NULL;
                return;
        }
	
	if(epktTx!=NULL)
                assert(epktRTS && epktTx);
        else
                assert(epktRTS && epktBEACON);
 	freePacket(epktRTS); epktRTS = 0;
   
	mhSend->cancel();
	freePacket(p); 

 	/* 
	 * The successful reception of this CTS packet implies
	 * that our RTS was successful. Hence, we can reset
	 * the Short Retry Count and the CW.
	 */
	ssrc = 0;
  	rst_CW();

 	tx_resume(); 
}


void mac802_11a_dcf::recvACK(ePacket_ *p) {

	Packet				*pkt;
 	int				pktlen; 
	struct frame_control    *dh_fc;

	//u_int64_t                       inTicks;
        //u_int32_t                       ttime;
        //struct hdr_mac802_11            *hm;

        int f_len;
        unsigned int crc,fcs;
        char *buf;
        struct ack_frame *af;
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("tx_state = %d\n",tx_state);
#endif
 
	if (tx_state != MAC_SEND) {
 		drop(p, NULL);
		p = NULL;
 		return;
	}

	GET_PKT(pkt,p);
        af = (struct ack_frame*)pkt->pkt_get();
	dh_fc = &(af->af_fc);
        f_len = sizeof(struct ack_frame) - ETHER_FCS_LEN;
        buf = (char*)malloc(f_len);
        memset(buf,0,f_len);
        memcpy(buf,af,f_len);
        crc = crc32(0,buf,f_len);
        memcpy(&fcs,af->af_fcs, 4);
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("CRC = %08x , FCS = %08x\n",crc,fcs);
#endif
        free(buf);

        if (memcmp(&crc, af->af_fcs, 4) != 0) {
                NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
                drop(p, NULL);
                p = NULL;
                return;
        }


 	assert(epktTx);
	GET_PKT(pkt, epktTx);
 	pktlen = pkt->pkt_getlen() - sizeof(struct hdr_mac802_11);  

	/* count log metric : 
	 *
	 * Note:
	 *	Unicast packets are sent successfully until receiving
	 *	ACK packets, therefore the NumUniOut metric is counted
	 *	here.
	 */
	if( _log && !strcasecmp(_log, "on") ) { 
		if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) )
			NumUniOut++;
		if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
			NumUniInOut++;
	
		GET_PKT(pkt, epktTx);
		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
			OutThrput += (double)pkt->pkt_getlen() / 1000.0;
		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
			InOutThrput += (double)pkt->pkt_getlen() / 1000.0;
	}

 	freePacket(epktTx); epktTx = 0;

   	mhSend->cancel();

 	/* 
	 * If successful reception of this ACK packet implies
	 * that our DATA transmission was successful. Hence,
	 * we can reset the Short/Long Retry Count and CW.
	 */
	if (pktlen < (int)macmib->aRTSThreshold)
		ssrc = 0;
  	else
		slrc = 0;

	freePacket(p);
 
  	/* Backoff before sending again */
	rst_CW();
 	assert(mhBackoff->busy_ == 0);
   	start_backoff(dh_fc);

 	tx_resume();
}


void mac802_11a_dcf::recvDATA(ePacket_ *p) {

	struct hdr_mac802_11		*hm;
	Packet				*pkt;
	int				match; 
	
	int f_len,pl_len;
        unsigned int crc,fcs;
        char *tmp,*buf;
        struct PHYInfo          *phyInfo;
	struct ether_header	*eh;

	GET_PKT(pkt, p); 

	//DSR_Info *dsr_info = (DSR_Info*)pkt->pkt_getinfo( "dsr" );

 	hm = (struct hdr_mac802_11 *)pkt->pkt_get();
	eh=(struct ether_header *)(pkt->pkt_get()+sizeof(struct hdr_mac802_11));
	
	if(eh->ether_type == (htons(ETHERTYPE_ARP)))	//Calculate ARPRequest CRC , ARP mode = RunARP
	{
		tmp = pkt->pkt_get()+sizeof(struct hdr_mac802_11);
		f_len = sizeof(struct arpPkt)+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN+sizeof(struct ether_header);
		pl_len = sizeof(struct arpPkt)+sizeof(struct ether_header);
		buf = (char*)malloc(f_len);
		memset(buf,0,f_len);
		memcpy(buf,hm,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
		memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,tmp,pl_len);
		crc = crc32(0,buf,f_len);
	}
	else if(hm->dh_fc.fc_type == MAC_Type_Management)
        {
		phyInfo =(struct PHYInfo*) pkt->pkt_getinfo("WPHY_80211a");
                tmp = pkt->pkt_sget();
		f_len = phyInfo->payload_len +sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN;
                pl_len = phyInfo->payload_len;
                buf = (char*)malloc(f_len);
                memset(buf,0,f_len);
                memcpy(buf,hm,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
                memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,tmp,pl_len);
                crc = crc32(0,buf,f_len);
        }
	else						//MAC_Type_DATA
	{
		phyInfo =(struct PHYInfo*) pkt->pkt_getinfo("WPHY_80211a");
		tmp = pkt->pkt_sget();
		if(phyInfo->payload_len !=0)
		{	
			char *pt_ptr = pkt->pkt_get()+sizeof(struct hdr_mac802_11);
                        f_len =  phyInfo->frame_len - ETHER_FCS_LEN;
                        int pt_len = phyInfo->mac_hdr_length - sizeof(struct hdr_mac802_11);
                        pl_len = phyInfo->payload_len;

                        buf = (char*)malloc(f_len);
                        memset(buf,0,f_len);
                        memcpy(buf,hm,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
                        memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,pt_ptr,pt_len);
                        memcpy(buf+phyInfo->mac_hdr_length-ETHER_FCS_LEN,tmp,pl_len);
			crc = crc32(0,buf,f_len);
		}
		else
		{
			tmp = pkt->pkt_get()+sizeof(struct hdr_mac802_11);
                        f_len =  phyInfo->mac_hdr_length -ETHER_FCS_LEN;
                        pl_len = phyInfo->frame_len - sizeof(struct hdr_mac802_11);
                        buf = (char*)malloc(f_len);
                        memset(buf,0,f_len);
                        memcpy(buf,hm,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
                        memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,tmp,pl_len);
                        crc = crc32(0,buf,f_len);	
		}	
	}
        memcpy(&fcs,hm->dh_fcs, 4);
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("CRC = %08x , FCS = %08x\n",crc,fcs);
#endif
        free(buf);

        if (memcmp(&crc, hm->dh_fcs, 4) != 0) {
                NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
                drop(p, NULL);
                p = NULL;
                return;
        }


	/* MoblNode module and AP module would decapsulate the 802.11 header */
	/* decapsulate 802.11 header */
	//CHECK
	//pkt->pkt_seek(sizeof(struct hdr_mac802_11)); 

 	if (bcmp(hm->dh_addr1, etherbroadcastaddr, 
		ETHER_ADDR_LEN))
	{	/* unicast address */
		if (pkt->pkt_getlen() >= (int)macmib->aRTSThreshold) {
			if (tx_state == MAC_CTS) {
				assert(epktCTRL);
 				freePacket(epktCTRL); 
			 	epktCTRL = 0;  

				mhSend->cancel();
 				ssrc = 0;
  				rst_CW(); 
			}
			else {
				drop(p, NULL);
				p = NULL;
 				return; 
			}
#if PKT_FLOW_DEBUG
			NSLOBJ_DEBUG("Send ACK\n");
#endif
			sendACK(hm->dh_addr2);
 			tx_resume(); 
		}
 		/*
		 * We did not send a CTS and there is no 
		 * room to buffer an ACK.
		 */
		else {
			if (epktCTRL) {
				drop(p, NULL);
				p = NULL;
 				return; 
			}
#if PKT_FLOW_DEBUG
                        NSLOBJ_DEBUG("Send ACK\n");
#endif
			sendACK(hm->dh_addr2); 
			if (mhSend->busy_ == 0)
  				tx_resume(); 
		}
	}
	
	/*===================================================
	   Make/Update an entry in sequence number cache
	  ===================================================*/
	if (bcmp(hm->dh_addr1, etherbroadcastaddr, 
	    ETHER_ADDR_LEN)) {
	    /*
	     * IEEE 802.11 Spec, section 9.2.9
	     */
	    match = update_dcache(hm->dh_addr2, hm->dh_scontrol); 
	    if ((hm->dh_fc.fc_retry==1)&&match) {
			drop(p, NULL);
			p = NULL;
			return;
	    } 
	}
#if PKT_FLOW_DEBUG
        NSLOBJ_DEBUG("put to upper layer\n");
#endif
	/* Pass the received packet to upper layer. */
	assert(put(p, recvtarget_)); 
}


int mac802_11a_dcf::is_idle() {

	if (rx_state != MAC_IDLE)
 		return(0);

 	if (tx_state != MAC_IDLE)
 		return(0);

 	if (nav > GetCurrentTime())
		return(0);
 	return(1);  
}


void mac802_11a_dcf::tx_resume() {

	u_int32_t		pktlen, dt;
	Packet			*p; 

	
	assert(mhSend->busy_==0);
	
	if (epktCTRL) {
		/*
		 * need to send a CTS or ACK.
		 */
		start_defer(sifs); 
	} 
	else if (epktRTS) {
		if (mhBackoff->busy_ == 0)
			start_defer(difs); 
	} 
	else if (epktTx || epktBEACON) {
		/*
		 * If we transmit MPDU using RTS/CTS, then we should defer
		 * sifs time before MPDU is sent; contrarily, we should
		 * defer difs time before MPDU is sent when we transmit
		 * MPDU without RTS/CTS.
		 */
		if(epktTx!=NULL)
                        GET_PKT(p, epktTx);
                else
                        GET_PKT(p, epktBEACON);
 		pktlen = p->pkt_getlen() - sizeof(struct hdr_mac802_11);
  		
		if (pktlen < macmib->aRTSThreshold)
			dt = difs;
		else 	dt = sifs;  

		if (mhBackoff->busy_ == 0)
			start_defer(dt); 
	}
	SET_TX_STATE(MAC_IDLE); 
}


void mac802_11a_dcf::rx_resume() {
	
	if (PHY_state == 1) {
		return;
	}

	assert((epktRx==0)&&(mhRecv->busy_==0));
	SET_RX_STATE(MAC_IDLE); 
}



int mac802_11a_dcf::update_dcache(u_char *mac, u_int16_t sc)
{
	int			i, cidx;
	u_int64_t		expire;
 

	/* find matching tuple */
 	for(i=0, cidx=-1 ; i<CACHE_SIZE; i++) {
	    if ((cache[i].timestamp!=0)&&(sc==cache[i].sequno)&&
		(!bcmp(mac, cache[i].src, ETHER_ADDR_LEN)))
		return(1); /* matching */

	    /* find the first unused entry */
	    if ((cidx==-1)&&(cache[i].timestamp==0))
  		cidx = i;  
	}     

	/* 
	 * If out of unused entry, update all tuple's
	 * timestamp and mark expire tuples.
	 */
	if (cidx == -1) {  
		MILLI_TO_TICK(expire, 2); 
		for(i=0,cidx=-1; i<CACHE_SIZE; i++) {
		    if ((cache[i].timestamp!=0)&&
			((GetCurrentTime()-cache[i].timestamp) > expire)) {
  			cache[i].timestamp = 0;  
			if (cidx==-1) cidx = i;    
		    }
		}       
	}
	
	/* 
	 * If no maching tuple found, find a unused
	 * tuple.
	 */
	if (cidx > -1) {
 		cache[cidx].timestamp = GetCurrentTime();
 		cache[cidx].sequno = sc;
                (void)memcpy((char *)cache[cidx].src,
                             (char *)mac, ETHER_ADDR_LEN);
                return(0); /* no matching tuple found */
        }
	return(0);
}


int mac802_11a_dcf::Fragment(ePacket_ *pkt, u_char *dst)
{
	Packet			*p;
	char			*k; 

	/*
     * IEEE 802.11 Spec, section 9.1.4
     *
     * - MSDUs/MMPDUs with an unicast receiver shall be fragmented
     *   if their length exceeds aFragmentation Threshold.
     * - MSDUs/MMPDUS with Broadcast/Multicast should not be
     *   fragmented even if their length exceeds a Fragmentation
     *   Threshold.
     */
	GET_PKT(p, pkt); 
	if ((p->pkt_getlen() > (int)macmib->aFragmentationThreshold)||
	    (!bcmp(dst, etherbroadcastaddr, ETHER_ADDR_LEN)))
		return(-1); 

	k = p->pkt_aggregate();
 	
	return 0;
}

void mac802_11a_dcf::drop(ePacket_ *p, char *why)
{
	freePacket(p); 
}

int mac802_11a_dcf::log()
{
	double		cur_time;
//printf("mac802_11a_dcf::log %d\n",get_nid());
    
	cur_time = (double)(GetCurrentTime() * TICK) / 1000000000.0;

	if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) ) {
		fprintf(UniInLogFILE, "%.3f\t%llu\n", cur_time, NumUniIn);
		fflush(UniInLogFILE);
		NumUniIn = 0;
	}

	if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) ) {
		fprintf(UniOutLogFILE, "%.3f\t%llu\n", cur_time, NumUniOut);
		fflush(UniOutLogFILE);
		NumUniOut = 0;
	}

	if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) ) {
		fprintf(UniInOutLogFILE, "%.3f\t%llu\n", cur_time, NumUniInOut);
		fflush(UniInOutLogFILE);
		NumUniInOut = 0;
	}

	if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) ) {
		fprintf(BroInLogFILE, "%.3f\t%llu\n", cur_time, NumBroIn);
		fflush(BroInLogFILE);
		NumBroIn = 0;
	}

	if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) ) {
		fprintf(BroOutLogFILE, "%.3f\t%llu\n", cur_time, NumBroOut);
		fflush(BroOutLogFILE);
		NumBroOut = 0;
	}

	if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) ) {
		fprintf(BroInOutLogFILE, "%.3f\t%llu\n", cur_time, NumBroInOut);
		fflush(BroInOutLogFILE);
		NumBroInOut = 0;
	}

	if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
		fprintf(CollLogFILE, "%.3f\t%llu\n", cur_time, NumColl);
		fflush(CollLogFILE);
		NumColl = 0;
	}

	if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
		fprintf(DropLogFILE, "%.3f\t%llu\n", cur_time, NumDrop);
		fflush(DropLogFILE);
		NumDrop = 0;
	}

	InThrput /= logInterval;	
	OutThrput /= logInterval;	
	InOutThrput /= logInterval;	

	if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) ) {
		fprintf(InThrputLogFILE, "%.3f\t%.3f\n", cur_time, InThrput);
		fflush(InThrputLogFILE);	
		InThrput = 0;
	}

	if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) ) {
		fprintf(OutThrputLogFILE, "%.3f\t%.3f\n", cur_time, OutThrput);
		fflush(OutThrputLogFILE);
		OutThrput = 0;
	}

	if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) ) {
	    	fprintf(InOutThrputLogFILE, "%.3f\t%.3f\n", cur_time, InOutThrput);
		fflush(InOutThrputLogFILE);
		InOutThrput = 0;
	}

	return(1);
}

//hychen added
// BUSY: 1
// IDLE: 0

void mac802_11a_dcf::set_PHY_state(int st)
{
        PHY_state = (bool)st;
}

bool mac802_11a_dcf::is_PHY_idle()
{
        return !PHY_state;
}

void mac802_11a_dcf::triggerMAC()
{
	u_int32_t nid = get_nid();
        u_int32_t pid = get_port();
        if ( !tx_suspend ){
                return;
        }
        u_int32_t                       timeout;
        struct hdr_mac802_11            *hm;
        Packet                          *p;
        tx_suspend = 0;
        GET_PKT(p, epktTx);
	hm = (struct hdr_mac802_11 *)p->pkt_get();

	if (bcmp(hm->dh_addr1, etherbroadcastaddr,ETHER_ADDR_LEN))
	{
		timeout = ACKTimeout(p->pkt_getlen(),nid,pid);
	}
	else {
		// XXX: broadcasat
		//   sent a braodcast packet and no
		//   ack need.  ???????????????????
		timeout = TX_Time(p->pkt_getlen(),(*bw_),nid,pid);
		NSLOBJ_DEBUG("TX_time = %u(tick)\n", timeout*10);
	}
	transmit(epktTx, timeout);
}

void mac802_11a_dcf::set_mac_rxstate()
{
	if (rx_state == MAC_IDLE) {
                SET_RX_STATE(MAC_RECV);
        }
}

/* ========================================================================= */

unsigned int mac802_11a_dcf::crc32(unsigned int crc, char *buf, int len)
{
        if (buf == NULL)
                return 0L;
        crc = crc ^ 0xffffffffL;
        while (len >= 8) {
                DO8(buf);
                len -= 8;
        }
        if (len)
                do {
                        DO1(buf);
                } while (--len);
        return crc ^ 0xffffffffL;
}

