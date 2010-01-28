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

/* for DEBUG */
#define VERBOSE_LEVEL   MSG_DEBUG
#include <wimax/mac/verbose.h>
/* for DEBUG end */

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <ethernet.h>
#include <sys/types.h>
#include <netinet/in.h>

#include <gbind.h>
#include <mbinder.h>
#include <80211p/mac/mac_80211p.h>
#include <80211p/wme/wme.h>
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

#define PKT_FLOW_DEBUG 0
#define CHstate_DEBUG 0
#define PORT_MAPPING_DEBUG 0
//extern u_int32_t *fifoIFcurqlen;

MODULE_GENERATOR(mac_80211p); 



/*========================================================================  
  Define Macros
  ========================================================================*/
#define CHECK_BACKOFF_TIMER()					\
{								\
	if ( is_idle() ) {			\
		for (int i = AC_VO; i >= AC_BE; i--) {					\
			if (mhBackoff[i][CH_state]->paused_) {					\
				u_int64_t tm;					\
				MICRO_TO_TICK(tm, aifs[i]); 			\
				mhBackoff[i][CH_state]->resume(tm); 				\
			} \
		} \
	}							\
	\
	if (!is_idle() ){\
		for (int i = AC_VO; i >= AC_BE; i--) {					\
			if ( mhBackoff[i][CH_state]->busy_ &&	!mhBackoff[i][CH_state]->paused_) {				\
				mhBackoff[i][CH_state]->pause(); 				\
			}\
		} \
	}							\
} 

#define SET_TX_STATE(x)						\
{								\
	tx_state[CH_state] = (x);  					\
	CHECK_BACKOFF_TIMER(); 					\
}

#define SET_RX_STATE(x)						\
{								\
	rx_state = (x);  					\
	CHECK_BACKOFF_TIMER();	 				\
}

unsigned int crc_table_11p[256] = {
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
#define DO1(buf) crc = crc_table_11p[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
#define DO2(buf)  DO1(buf); DO1(buf);
#define DO4(buf)  DO2(buf); DO2(buf);
#define DO8(buf)  DO4(buf); DO4(buf);

/* ========================================================================= */
unsigned int mac_80211p::crc32(unsigned int crc, char *buf, int len)
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

mac_80211p::mac_80211p(u_int32_t type, u_int32_t id, struct plist* pl, const char *name): NslObject(type, id, pl, name)
{
	interrupt_CH = false;
	bw_ = 0;
	_ptrlog = 0;
	mhNav = mhSend = mhIF = mhRecv = NULL;
	for (int i = AC_VO; i >= AC_BE; i--) {
		for (int j = 0; j < CH_NUM;j++) {
			mhBackoff[i][j] = NULL;
		}
		mhDefer[i] = NULL;
	}
	macmib = NULL;
	phymib = NULL;

	/* bind variable */
	vBind_mac("mac", mac_);
	vBind("QoS_Pri_Port0", &plevel[0]);
	vBind("QoS_Pri_Port1", &plevel[1]);
	vBind("QoS_Pri_Port2", &plevel[2]);
	vBind("QoS_Pri_Port3", &plevel[3]);
	vBind("QoS_Pri_Port4", &plevel[4]);
	vBind("QoS_Pri_Port5", &plevel[5]);
	vBind("QoS_Pri_Port6", &plevel[6]);
	vBind("QoS_Pri_Port7", &plevel[7]);
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
	epktRx = 0;
	epktBuf = 0;  
	epktCTRL[state_SCH] = 0; 
	epktCTRL[state_CCH] = 0; 

	// Changed by Prof. Wang on 08/06/2003 here. Previously there is a Randomize()
	// function called here. However, this should not happen. Otherwise, the random
	// number seed will be reset every time when a new mobile node is created.


	/* init sequence number */
	sta_seqno = Random() % 4096;

	/* init MAC state */
	rx_state = tx_state[state_SCH] = tx_state[state_CCH]= MAC_IDLE;
	tx_active[state_SCH] = 0;  
	tx_active[state_CCH] = 0;  
	/* init EDCA 4 AC parameters 
	 *
	 * 1609.4, chapter 6.3 :
	 *
	 * EDCA parameter set used on the CCH
	 *
	 * ACI  AC    CWmin					 CWmax  			   AIFSN 
	 *			
	 *  1   BK    aCWmin				 aCWmax					 9	
	 *  0   BE   (aCWmin + 1)/2 - 1		 aCWmin					 6
	 *  2   VI   (aCWmin + 1)/4 - 1		(aCWmin + 1)/2 - 1		 3
	 *  3   VO	 (aCWmin + 1)/4 - 1		(aCWmin + 1)/2 - 1		 2
	 */
	CWmin[AC_BK][state_CCH] = phymib->aCWmin;
	CWmin[AC_BE][state_CCH] = (phymib->aCWmin+1)/2 - 1;
	CWmin[AC_VI][state_CCH] = (phymib->aCWmin+1)/4 - 1;
	CWmin[AC_VO][state_CCH] = (phymib->aCWmin+1)/4 - 1;	

	CWmax[AC_BK][state_CCH] = phymib->aCWmax;
	CWmax[AC_BE][state_CCH] = phymib->aCWmin;
	CWmax[AC_VI][state_CCH] = (phymib->aCWmin+1)/2 - 1;
	CWmax[AC_VO][state_CCH] = (phymib->aCWmin+1)/2 - 1;	

	aifsn[AC_BK][state_CCH] = 9;
	aifsn[AC_BE][state_CCH] = 6;
	aifsn[AC_VI][state_CCH] = 3;
	aifsn[AC_VO][state_CCH] = 2;	

	/* EDCA parameter set on the SCH
	 *
	 *  ACI  AC    CWmin				 CWmax		  		   AIFSN 
	 *			
	 *  1   BK    aCWmin				 aCWmax					 7	
	 *  0   BE    aCWmin				 aCWmax					 3
	 *  2   VI   (aCWmin + 1)/2 - 1		 aCWmin					 2
	 *  3   VO	 (aCWmin + 1)/4 - 1		(aCWmin + 1)/2 - 1		 2
	 */
	CWmin[AC_BK][state_SCH] = phymib->aCWmin;
	CWmin[AC_BE][state_SCH] = phymib->aCWmin;
	CWmin[AC_VI][state_SCH] = (phymib->aCWmin+1)/2 - 1;
	CWmin[AC_VO][state_SCH] = (phymib->aCWmin+1)/4 - 1;	

	CWmax[AC_BK][state_SCH] = phymib->aCWmax;
	CWmax[AC_BE][state_SCH] = phymib->aCWmax;
	CWmax[AC_VI][state_SCH] = phymib->aCWmin;
	CWmax[AC_VO][state_SCH] = (phymib->aCWmin+1)/2 - 1;	

	aifsn[AC_BK][state_SCH] = 7;
	aifsn[AC_BE][state_SCH] = 3;
	aifsn[AC_VI][state_SCH] = 2;
	aifsn[AC_VO][state_SCH] = 2;	
	/* based on WME spec. */
	edca_txop = 0;
	txopLimit[AC_BE] = 0;
	txopLimit[AC_BK] = 0;
	txopLimit[AC_VI] = 188;  /* 188 << 5, unit is 32-usec */  
	txopLimit[AC_VO] = 102;  /* 102 << 5, unit is 32-usec */

	/* init plevel( port <-> priority ) */
	for(int i = 0; i <= 7 ; i++ )
		plevel[i] = 2000 + i;

	for (int i = AC_VO; i >= AC_BE; i--) {
		for (int j = 0;j < CH_NUM;j++) {
			epktDATA[i][j] = 0;
			epktRTS[i][j] = 0;
		}
		cw[i] = CWmin[i][state_CCH]; 
	}
	/* initialize others */
	nav = 0;  ssrc = slrc = 0; 
	now_ac[state_SCH] = -1;
	now_ac[state_CCH] = -1;
	/* init cache */
	bzero(cache, sizeof(struct dup_cache)*CACHE_SIZE); 
	for(int i=0;i<6;i++)
	{
		etherbroadcastaddr[i] = 0xff;
	}
	/* register variable */
	REG_VAR("promiscuous", &promiscuous);
	REG_VAR("RTS_THRESHOLD",&(macmib->aRTSThreshold));
	REG_VAR("SSRC",&ssrc);
	REG_VAR("SLRC",&slrc);
}


mac_80211p::~mac_80211p()
{
	if (NULL != phymib)
		free(phymib);

	if (NULL != macmib)
		free(macmib);

	if (NULL != epktCTRL[state_SCH])
		freePacket(epktCTRL[state_SCH]);

	if (NULL != epktCTRL[state_CCH])
		freePacket(epktCTRL[state_CCH]);

	if (NULL != epktRx)
		freePacket(epktRx);

	if (NULL != mhNav)
		delete mhNav;

	if (NULL != mhSend)
		delete mhSend;

	if (NULL != mhIF)
		delete mhIF;

	if (NULL != mhRecv)
		delete mhRecv;

	if (NULL != mhSIFS)
		delete mhSIFS;
	if (NULL != mhCHstate)
		delete mhCHstate;

	for (int i = AC_VO; i >= AC_BE; i--) 
	{
		for (int j = 0;j < CH_NUM;j++) {
			if (NULL != epktRTS[i][j])
				freePacket(epktRTS[i][j]);
			if (NULL != epktDATA[i][j])
				freePacket(epktDATA[i][j]);
			if (NULL != mhBackoff[i][j])
				delete mhBackoff[i][j];
		}
		if (NULL != mhDefer[i])
			delete mhDefer[i];
	}
}


int mac_80211p::init_PHYMIB()
{
	phymib = (struct PHY_MIB_IN_MAC *)malloc(sizeof(struct PHY_MIB_IN_MAC));
	assert(phymib);

	/* IEEE 802.11 Spec, Page 237 */
	phymib->aCWmin = 15;
	phymib->aCWmax = 1023;
	phymib->aSlotTime = 13; 		/* 20 us */
	phymib->aCCATime = 8;			/* 8 us */
	phymib->aRxTxTurnaroundTime = 2;	/* 2 us */
	phymib->aSIFSTime = 32;  		/* 32 us */
	phymib->aPreambleLength = 32;		/* microsecond */
	phymib->aPLCPHeaderLength = 8;  	/* microsecond */

	return(1); 
}

int mac_80211p::init_MACMIB() {

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


int mac_80211p::init_Timer() {

	BASE_OBJTYPE(type);

	/* 
	 * Generate timer needed in 
	 * IEEE 802.11 MAC.
	 */
	mhNav     = new timerObj;
	mhSend    = new timerObj;
	mhIF      = new timerObj;
	mhRecv    = new timerObj;
	mhSIFS	  = new timerObj;
	mhCHstate = new timerObj; //SCH or CCH timer

	mhDefer[0] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, BE_deferHandler);
	mhDefer[0]->setCallOutObj(this, type);

	mhDefer[1] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, BK_deferHandler);
	mhDefer[1]->setCallOutObj(this, type);

	mhDefer[2] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, VI_deferHandler);
	mhDefer[2]->setCallOutObj(this, type);

	mhDefer[3] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, VO_deferHandler);
	mhDefer[3]->setCallOutObj(this, type);

	deferFlag[0] = false;
	deferFlag[1] = false;
	deferFlag[2] = false;
	deferFlag[3] = false;

	/*initialize SCH backoff Handler*/

	mhBackoff[AC_BE][state_SCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, SCH_BE_backoffHandler);
	mhBackoff[AC_BE][state_SCH]->setCallOutObj(this, type);

	mhBackoff[AC_BK][state_SCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, SCH_BK_backoffHandler);
	mhBackoff[AC_BK][state_SCH]->setCallOutObj(this, type);

	mhBackoff[AC_VI][state_SCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, SCH_VI_backoffHandler);
	mhBackoff[AC_VI][state_SCH]->setCallOutObj(this, type);

	mhBackoff[AC_VO][state_SCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, SCH_VO_backoffHandler);
	mhBackoff[AC_VO][state_SCH]->setCallOutObj(this, type);

	/*initialize CCH backoff Handler*/

	mhBackoff[AC_BE][state_CCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, CCH_BE_backoffHandler);
	mhBackoff[AC_BE][state_CCH]->setCallOutObj(this, type);

	mhBackoff[AC_BK][state_CCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, CCH_BK_backoffHandler);
	mhBackoff[AC_BK][state_CCH]->setCallOutObj(this, type);

	mhBackoff[AC_VI][state_CCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, CCH_VI_backoffHandler);
	mhBackoff[AC_VI][state_CCH]->setCallOutObj(this, type);

	mhBackoff[AC_VO][state_CCH] = new timerObj;
	type = POINTER_TO_MEMBER(mac_80211p, CCH_VO_backoffHandler);
	mhBackoff[AC_VO][state_CCH]->setCallOutObj(this, type);



	/* 
	 * Associate timer with corresponding 
	 * time-handler method.
	 */
	type = POINTER_TO_MEMBER(mac_80211p, navHandler);
	mhNav->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac_80211p, sendHandler);
	mhSend->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac_80211p, txHandler);
	mhIF->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac_80211p, recvHandler);
	mhRecv->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac_80211p, sifsHandler); 
	mhSIFS->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac_80211p, CHstateHandler);
	mhCHstate->setCallOutObj(this, type);

	return(1); 
}


int mac_80211p::calcul_IFS() {

	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();	

	sifs = phymib->aSIFSTime;
	pifs = sifs + phymib->aSlotTime;   
	difs = sifs + 2 * phymib->aSlotTime;
	eifs = sifs + difs + TX_Time(MAC80211_ACK_LEN,(*bw_),nid,pid);
	for(int i = AC_VO ; i >= AC_BE; i--)
		aifs[i] = sifs + aifsn[i][CH_state] * phymib->aSlotTime;

	return(1); 
}


int mac_80211p::init() {

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
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
		type = POINTER_TO_MEMBER(mac_80211p, log);
		logTimer.setCallOutObj(this, type);
		logTimer.start(logIntervalTick, logIntervalTick);		
	}
	//queue init
	//FIXME mytunidp = GET_REG_VAR(get_portls()->pid, "TUNID", u_long *);
	for (int i = 0; i < AC_NUM; i++) {
		for (int j = 0;j < CH_NUM; j++) { 
			if_snd[i][j].ifq_head = if_snd[i][j].ifq_tail = 0; //queue
			if_snd[i][j].ifq_len = 0;
			if_snd[i][j].ifq_drops = 0;
			if_snd[i][j].ifq_pri = i;
			if_snd[i][j].ifq_maxlen = 50; /* by default */
		}
	}
	//some flag init
	cancel_WBSS = 0;
	CH_status = 0;
	SCH_norecv = 0;
	//CH_state timer init
	channel_interval = 50;
	MILLI_TO_TICK(channel_interval_tick, channel_interval);
	mhCHstate->start(channel_interval_tick, channel_interval_tick);
	CH_background_state = state_CCH;
	CH_state = state_CCH;  //start channel is CCH.
	IS_CH_SWITCH_PAUSED = 1;
	current_provider = -1;
	//CH_state timer end
	//FIXME
	bw_ = GET_REG_VAR1(get_portls(), "bw", double *);
	ch_ = GET_REG_VAR1(get_portls(), "CHANNEL", int *);
	//bw_ = new double;
	//ch_ = new int;
	//CPThreshold_ = new double;	
	*bw_ = (*bw_)*1000000;
	//*bw_ = 6000000;
	*ch_ = 178;				//FIXME init CCH channel number
	//*CPThreshold_ = 10;
	tx_suspend = 0;
	calcul_IFS();
	return(1); 

}

int mac_80211p::recv(ePacket_ *pkt) {

	Packet			*p;
	struct PHYInfo		*pinfo;
	u_int32_t		t; 

	struct hdr_mac802_11	*mh;
	struct mac802_11_log	*log1,*log2;
	char			*iph;
	u_long			ipdst,ipsrc;
	struct logEvent		*logep;
	u_int64_t		time_period;
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	GET_PKT(p, pkt); 
	assert(p);

	check_if_SCH_recv(((u_int32_t *)(p->pkt_getinfo("PSID"))));
	if (tx_active[CH_state] && p->pkt_err_ == 0) {
		p->pkt_err_ = 1;

		/* log "StartRX" and "DropRX" event */
		if(_ptrlog == 1){
			GET_PKT(p, pkt);
			pinfo = (struct PHYInfo *)p->pkt_getinfo("WAVEPhyinfo");
			u_int32_t *txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");
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
	GET_PKT(p, pkt); 
	assert(p);
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
	pinfo = (struct PHYInfo *)p->pkt_getinfo("WAVEPhyinfo");
	/* recv delay had been modified to PHY*/
	t = 0;
	start_recvtimer(t); 

	return(1);  
}

int mac_80211p::push_buf(ePacket_ *pkt, int ac) {
	u_char			*dst;	/* dst. MAC */
	Packet			*p; 
	struct hdr_mac802_11	*hm; 
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("ac = %d\n",ac);
#endif
	if (ac == -1){
		return 1;
	}
	/*
	 * We check buffer, epktTx, to see if there is 
	 * any packet in the MAC to be send.
	 */
	if (epktDATA[ac][CH_state]||edca_txop > GetCurrentTime()){ 
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("ac = %d,CH_state = %d, epktdata = %d,exca_txop = %llu\n",ac,CH_state,epktDATA[ac][CH_state],edca_txop);
#endif
		return(1); /* reject upper module's request */
	}
	assert(pkt);
	assert((Packet *)(pkt)->DataInfo_);
	GET_PKT(p, pkt); 

	assert(pkt && epktDATA[ac][CH_state] == NULL);
	epktDATA[ac][CH_state] = pkt_copy(pkt);		/* hold this packet into send buffer */

	/* Get destination MAC address */
	//ETHER_DHOST(dst, pkt); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();
	dst = hm->dh_addr1;  

	/* Generate IEEE 802.11 Data frmae */
	sendDATA( ac, dst);

	/* Generate IEEE 802.11 RTS frame */
	sendRTS( ac, dst);

	/*
	 * Sense the medium state, if idle we should wait a DIFS time
	 * before transmitting; otherwise, we should start backoff timer.
	 */
	if (mhBackoff[ac][CH_state]->busy_ == 0) {
		if (is_idle()) {
			/* 
			 * Medium is idle, we should start defer
			 * timer. If the defer timer is starting, there
			 * is no need to reset defer timer.
			 */
			if (mhDefer[ac]->busy_ == 0)
			{
				if(hm->dh_fc.fc_type == MAC_Type_Management)
				{
					//u_int32_t timeout;
					//timeout = ((Random() % cw[ac]) * phymib->aSlotTime);
					//timeout += sifs;
					//timeout  = timeout * 10;
					deferFlag[ac] = true;
					start_defer(ac, sifs);
				}
				else
				{
					//u_int32_t timeout;
					//timeout = ((Random() % cw[ac]) * phymib->aSlotTime);
					//timeout += difs;
					//timeout  = timeout * 10;
					deferFlag[ac] = true;
					start_defer(ac, difs);
				}
			}
		} else {
			/*XXX only backoff? */
			if (mhBackoff[ac][CH_state]->paused_ == 0 )
				start_backoff(ac); 
		}
	}
	return(1); 
}

/* a. Pkt here will be enqueued in the MAC queue. 
 * b. If current channel is CCH, ip pkt should be enqueued in SCH queue and 
 *    would not be sent until SCH.
 * c. 
 * */
int mac_80211p::send(ePacket_ *pkt) {

	//u_char					*dst;	/* dst. MAC */
	Packet			*p; 
	struct hdr_mac802_11	*hm; 
	struct ip		*iph;
	struct ether_header 	*eh;
	struct tcphdr		*tcph;
	struct udphdr		*udph;
	int			port = 0;
	int 			ac = -1;

#if PKT_FLOW_DEBUG
	//NSLOBJ_DEBUG("\n");
#endif
	/*
	 * We check buffer, epktTx, to see if there is 
	 * any packet in the MAC to be send.
	 */
	//EDCA
	// get port from IP header 
	GET_PKT(p,pkt);
	hm = (struct hdr_mac802_11 *)p->pkt_get();
	eh=(struct ether_header *)(p->pkt_get()+sizeof(struct hdr_mac802_11));

#if PKT_FLOW_DEBUG
	if(ntohs(eh->ether_type) == 0x88DC)
		printf("WSM!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
	if(ntohs(eh->ether_type) == 1)
		printf("WSA!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif

	iph = (struct ip *)p->pkt_sget();
	if(hm->dh_fc.fc_type == MAC_Type_Data && ntohs(eh->ether_type) == 0x0800 && iph!=NULL)  // IP pkt
	{
		if (iph->ip_p == IPPROTO_TCP) {
			tcph = (struct tcphdr *)((char *)iph+sizeof(struct ip));
			port = ntohs(tcph->th_dport);
		} else if(iph->ip_p == IPPROTO_UDP) {
			udph = (struct udphdr *)((char *)iph+sizeof(struct ip));
			port = ntohs(udph->uh_dport);
		}
		ac = whichAC(port);
		if(port==8118)
			ac = 3;
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("NID=[%d],ip pkt, ac = %d,port = %d\n",get_nid(),ac,port);
#endif


		if( IF_QFULL(&if_snd[ac][state_SCH]) )
		{
#if PKT_FLOW_DEBUG
			NSLOBJ_DEBUG("queue full\n");
#endif
			IF_DROP(&if_snd[ac][state_SCH]);
			freePacket(pkt);
			return(1);
		}	
		IF_ENQUEUE(&if_snd[ac][state_SCH], pkt);//IP pkt can only be enqueued in SCH queue
		triggerSend();
		return (1);
	} else if (ntohs(eh->ether_type) == 1) {  //WSA
		/*
		 * Mangagement type frame should be "AC_VO", according to 802.11e spec.
		 * Management Packets before connection; do not need to put in queue
		 */
		ac =AC_VO;

#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CH_state = %d\n",CH_state);
#endif
		if( IF_QFULL(&if_snd[ac][state_CCH]) )
		{
#if PKT_FLOW_DEBUG
			NSLOBJ_DEBUG("queue full\n");
#endif
			IF_DROP(&if_snd[ac][state_CCH]);
			freePacket(pkt);
			return 1;
		}

		IF_ENQUEUE(&if_snd[ac][state_CCH], pkt);//WSA can only be sent at CCH
		triggerSend();
		/*if (CH_state == state_CCH) {
			push_buf(if_snd[ac][CH_state].ifq_head, AC_VO); //FIXME can be recv when SCH ?
		}*/
		return (1);
	}
#if PORT_MAPPING_DEBUG
	NSLOBJ_DEBUG("other pkt, port = %d, ac = %d\n",port , ac);
#endif
	ac = whichAC(port);

	if( IF_QFULL(&if_snd[ac][CH_state]) )
	{
		IF_DROP(&if_snd[ac][CH_state]);
		freePacket(pkt);
		return(1);
	}	
	IF_ENQUEUE(&if_snd[ac][CH_state], pkt);//enqueued whether in SCH or CCH queue
	/* FIXME
	   if (mytunidp != NULL) {
	   fifoIFcurqlen[*mytunidp] = if_snd.ifq_len;
	   }
	   */
	triggerSend();
	return (1);
}


int mac_80211p::command(int argc, const char *argv[]) {

	return(NslObject::command(argc, argv));   
}



/*------------------------------------------------------------------------*
 * IEEE MAC 802.11 Implementation.
 *------------------------------------------------------------------------*/ 
inline void mac_80211p::set_NAV(u_int32_t _nav) {

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


inline void mac_80211p::start_defer(int pri, u_int32_t us) {

	u_int64_t		inTicks;
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	MICRO_TO_TICK(inTicks, us);
	mhDefer[pri]->start(inTicks, 0); 
}
inline void mac_80211p::start_sifs(u_int32_t us) {

	u_int64_t		inTicks;	

	MICRO_TO_TICK(inTicks, us);
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("timeout = %llu\n",inTicks);
#endif
	mhSIFS->start(inTicks, 0); 
}


inline void mac_80211p::start_sendtimer(u_int32_t us) {

	u_int64_t		inTicks;

	MICRO_TO_TICK(inTicks, us);
	mhSend->start(inTicks, 0);
}


inline void mac_80211p::start_recvtimer(u_int32_t us) {

	u_int64_t		inTicks;

	MICRO_TO_TICK(inTicks, us);
	mhRecv->start(inTicks, 0);
}


inline void mac_80211p::start_IFtimer(u_int32_t us) {

	u_int64_t		inTicks;

	MICRO_TO_TICK(inTicks, us);
	mhIF->start(inTicks, 0);
}


void mac_80211p::start_backoff(int pri) {

	u_int32_t	timeout;	/* microsecond */
	u_int64_t	timeInTick; 

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("pri = %d\n",pri);
#endif
	/*
	 * IEEE 802.11 Spec, section 9.2.4
	 *	
	 *	- Backoff Time = Random() x aSlotTime
	 *  	- Random() : PseudoRandom interger drawn from
	 * 		     a uniform distribution over the 
	 *		     interval [0, CW] where
	 * 		     aCWmin <=  CW <= aCWmax
	 */

	timeout = (Random() % cw[pri]) * phymib->aSlotTime;
	MICRO_TO_TICK(timeInTick, timeout); 
	//DEBUG 
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("timeout = %llu, cw[pri]= %d,aSlotTime =%d\n",timeInTick,cw[pri],phymib->aSlotTime);
#endif
	mhBackoff[pri][CH_state]->start(timeInTick, 0);

	/* 
	 * Sense the medium, if busy, pause 
	 * the timer immediately.
	 */
	if (!is_idle())	{
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("medium is not idle, backoff timer paused\n");
#endif

		mhBackoff[pri][CH_state]->pause(); 
	}
}

int mac_80211p::sifsHandler(Event_ *e)
{
	assert( epktDATA[now_ac[CH_state]][CH_state]|| epktRTS[now_ac[CH_state]][CH_state] || epktCTRL[CH_state] );
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	if (check_pktCTRL(now_ac[CH_state]) == 0)		
		return(1); 

	assert(mhBackoff[now_ac[CH_state]][CH_state]->busy_==0);  

	if (check_pktRTS(now_ac[CH_state]) == 0)
		return(1);

	if (check_pktTx(now_ac[CH_state]) == 0)
		return(1); 

	return 0;
}

int mac_80211p::navHandler(Event_ *e)
{
	/*
	 * IEEE 802.11 Spec, Figure 52
	 * 	- If medium idle, we should defer 
	 *	  difs time.
	 */	

	u_int32_t	tick;

	if (is_idle()) {					
		for (int i = AC_VO; i >= AC_BE; i--) {
			if(mhBackoff[i][CH_state]->paused_){
				MICRO_TO_TICK(tick,aifs[i]);
				mhBackoff[i][CH_state]->resume(tick);
			}

		}
	} 

	return 1;
}


int mac_80211p::sendHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("tx_state = %d\n",tx_state[CH_state]);
#endif
	struct hdr_mac802_11		*hm;
	Packet				*p;

	switch(tx_state[CH_state]) {
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
			assert(epktCTRL[CH_state]);
			freePacket(epktCTRL[CH_state]);
			epktCTRL[CH_state] = 0;
			break;

		case MAC_SEND:
			/*
			 * sent DATA, but did not receive an ACK
			 */
			GET_PKT(p, epktDATA[now_ac[CH_state]][CH_state]); 
			hm = (struct hdr_mac802_11 *)p->pkt_get();

			if (bcmp(hm->dh_addr1, etherbroadcastaddr,
						ETHER_ADDR_LEN))
			{
				/* unicast address */
#if PKT_FLOW_DEBUG
				NSLOBJ_DEBUG("	retransmit pkt[%d]\n",now_ac[CH_state]);
#endif
				retransmitDATA();
			}
			else {	/*
				 * IEEE 802.11 Spec, section 9.2.7
				 *	- In Broadcast and Multicast,
				 *	  no ACK should be received.
				 */
#if PKT_FLOW_DEBUG
				NSLOBJ_DEBUG("	broadcast pkt, free epktDATA[%d]\n",now_ac[CH_state]);
#endif
				freePacket(epktDATA[now_ac[CH_state]][CH_state]);
				epktDATA[now_ac[CH_state]][CH_state] = 0;
				rst_CW(now_ac[CH_state]);
				start_backoff(now_ac[CH_state]);
				//kcliu start : push buffer
				//EDCA
				ePacket_ *tmp_pkt;
				IF_DEQUEUE(&if_snd[now_ac[CH_state]][CH_state], tmp_pkt); //dequeue
				freePacket(tmp_pkt);
				triggerSend();
				now_ac[CH_state] = -1;
				//end
			}
			break;

		case MAC_ACK:
			/*
			 * Sent an ACK, and now ready to 
			 * resume transmission
			 */
			assert(epktCTRL[CH_state]);
			freePacket(epktCTRL[CH_state]);
			epktCTRL[CH_state] = 0;
			break;

		case MAC_IDLE:
			break;

		default:
			assert(0); 
	}
	tx_resume(); 

	return 1;
}
int mac_80211p::BE_deferHandler(Event_ *e)
{
	assert( epktCTRL[CH_state] || epktDATA[AC_BE][CH_state] || epktRTS[AC_BE][CH_state]);
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	if( !deferFlag[AC_BE] )
	{
		if (check_pktCTRL(AC_BE) == 0)
			return(1); 

		assert(mhBackoff[AC_BE][CH_state]->busy_ == 0);   

		if (check_pktRTS(AC_BE) == 0)
			return(1);

		if (check_pktTx(AC_BE) == 0)
			return(1); 
	}
	else
	{
		start_backoff(AC_BE);
	}

	return 0;
}
int mac_80211p::BK_deferHandler(Event_ *e)
{
	assert( epktCTRL[CH_state] || epktDATA[AC_BK][CH_state] || epktRTS[AC_BK][CH_state]);
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	if( !deferFlag[AC_BK] )
	{
		if (check_pktCTRL(AC_BK) == 0)
			return(1); 

		assert(mhBackoff[AC_BK][CH_state]->busy_ == 0);   

		if (check_pktRTS(AC_BK) == 0)
			return(1);

		if (check_pktTx(AC_BK) == 0)
			return(1); 
	}
	else
	{
		start_backoff(AC_BK);
	}

	return 0;
}
int mac_80211p::VI_deferHandler(Event_ *e)
{
	assert( epktCTRL[CH_state] || epktDATA[AC_VI][CH_state] || epktRTS[AC_VI][CH_state]);
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	if (!deferFlag[AC_VI] )
	{
		if (check_pktCTRL(AC_VI) == 0)
			return(1); 

		assert(mhBackoff[AC_VI][CH_state]->busy_ == 0);   

		if (check_pktRTS(AC_VI) == 0)
			return(1);

		if (check_pktTx(AC_VI) == 0)
			return(1); 
	}
	else
	{
		start_backoff(AC_VI);
	}

	return 0;
}


int mac_80211p::VO_deferHandler(Event_ *e)
{
	assert( epktCTRL[CH_state] || epktDATA[AC_VO][CH_state] || epktRTS[AC_VO][CH_state]);
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	if ( !deferFlag[AC_VO] )
	{
		if (check_pktCTRL(AC_VO) == 0)
			return(1); 

		assert(mhBackoff[AC_VO][CH_state]->busy_ == 0);   

		if (check_pktRTS(AC_VO) == 0)
			return(1);

		if (check_pktTx(AC_VO) == 0)
			return(1); 
	}
	else
	{
		start_backoff(AC_VO);
	}

	return 0;
}

int mac_80211p::CCH_BE_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_CCH]) {
		assert(mhSend->busy_ || mhDefer[AC_BE]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_BE) == 0)
		return(1);

	if (check_pktTx(AC_BE) == 0)
		return(1); 

	return 0;
}
int mac_80211p::SCH_BE_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_SCH]) {
		assert(mhSend->busy_ || mhDefer[AC_BE]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_BE) == 0)
		return(1);

	if (check_pktTx(AC_BE) == 0)
		return(1); 

	return 0;
}
int mac_80211p::SCH_BK_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_SCH]) {
		assert(mhSend->busy_ || mhDefer[AC_BK]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_BK) == 0)
		return(1);

	if (check_pktTx(AC_BK) == 0)
		return(1); 

	return 0;
}
int mac_80211p::CCH_BK_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_CCH]) {
		assert(mhSend->busy_ || mhDefer[AC_BK]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_BK) == 0)
		return(1);

	if (check_pktTx(AC_BK) == 0)
		return(1); 

	return 0;
}


int mac_80211p::SCH_VI_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_SCH]) {
		assert(mhSend->busy_ || mhDefer[AC_VI]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_VI) == 0)
		return(1);

	if (check_pktTx(AC_VI) == 0)
		return(1); 

	return 0;
}

int mac_80211p::CCH_VI_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_CCH]) {
		assert(mhSend->busy_ || mhDefer[AC_VI]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_VI) == 0)
		return(1);

	if (check_pktTx(AC_VI) == 0)
		return(1); 

	return 0;
}
int mac_80211p::SCH_VO_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_SCH]) {
		assert(mhSend->busy_ || mhDefer[AC_VO]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_VO) == 0)
		return(1);

	if (check_pktTx(AC_VO) == 0)
		return(1); 

	return 0;
}

int mac_80211p::CCH_VO_backoffHandler(Event_ *e)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	if (epktCTRL[state_CCH]) {
		assert(mhSend->busy_ || mhDefer[AC_VO]->busy_);
		return(1);
	}

	if (check_pktRTS(AC_VO) == 0)
		return(1);

	if (check_pktTx(AC_VO) == 0)
		return(1); 

	return 0;
}


int mac_80211p::CHstateHandler(Event_ *e) //added by kcliu
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	CH_background_state = !CH_background_state;  //background CH flag should always switch 
	if (IS_CH_SWITCH_PAUSED) {
		if(CH_state == state_CCH)
		{

			((WME *)InstanceLookup(get_nid(), get_port(), "WME"))->to_cch();
			if(!IS_CH_SWITCH_PAUSED)
				interrupt_CH = true;
			else
				return 0;

		}
	}

	/* will switch to CCH, check background CH flag is CCH first 
	 * if background CH flag is SCH, channel cannot switch       */
	if (CH_state == state_SCH && CH_background_state == state_CCH) {   //channel will switch from SCH to CCH immediately
#if CHstate_DEBUG
		NSLOBJ_DEBUG("switches to CCH\n");
		channel_dump();
#endif
		CH_state = state_CCH;
		*ch_ = 178;
		check_if_cancel_WBSS(); //if SCH hadn't send any pkt, WBSS should be cancelled
		/* recalculate aifs */
		for(int i = AC_VO ; i >= AC_BE; i--){
			aifs[i] = sifs + aifsn[i][state_CCH] * phymib->aSlotTime;
		}
		if (epktCTRL[state_CCH]) {
			freePacket(epktCTRL[state_CCH]);
			epktCTRL[state_CCH]=0;
		}
		if (mhSend->busy_) {
#if PKT_FLOW_DEBUG
			printf("\t\tmhSend busy!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n");
#endif
			//mhSend->cancel();
		}
		if (mhNav->busy_)
		{
			mhNav->cancel();
			nav = 0;
		}
		for (int i = AC_BE;i <= AC_VO;i++)
		{
			if(mhDefer[i]->busy_){
#if PKT_FLOW_DEBUG
				printf("\t\tmhDefer[%d] busy!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",i+1);
#endif
				mhDefer[i]->cancel();
			}
			if (mhBackoff[i][state_SCH]->busy_) {
#if PKT_FLOW_DEBUG
				printf("\t\tnode[%d]:mhBackoff[%d] busy!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",get_nid(),i);
#endif
				mhBackoff[i][state_SCH]->cancel();
				freePacket(epktDATA[i][state_SCH]); //FIXME should be modified to pause mode than cancel mode
				epktDATA[i][state_SCH] = 0; //FIXME should be modified to pause mode than cancel mode
			}
			if(epktRTS[i][state_CCH]) {
				freePacket(epktRTS[i][state_CCH]);
				epktRTS[i][state_CCH] = 0;
			}
			if (epktDATA[i][state_CCH] != 0) {
				//start_backoff(i);
				freePacket(epktDATA[i][state_CCH]);
				epktDATA[i][state_CCH] = 0;
			}
		}
		SET_TX_STATE(MAC_IDLE); 
		triggerSend();
		/* will switch to SCH, check background CH flag is SCH first 
		 * if background CH flag is CCH, channel cannot switch       */
	} else if (CH_state == state_CCH && CH_background_state == state_SCH) {
#if CHstate_DEBUG 
		NSLOBJ_DEBUG("switches to SCH\n");
		channel_dump();
#endif
		*ch_ = service_channel_num;
		CH_state = state_SCH;
		if(interrupt_CH == false)
			((WME *)InstanceLookup(get_nid(), get_port(), "WME"))->to_cch();
		else
			interrupt_CH = false;

		if(CH_state == state_CCH)
			return 0;

		SCH_norecv = 1;
		/* recalculate aifs */
		for (int i = AC_VO;i >= AC_BE;i--) {
			aifs[i] = sifs + aifsn[i][state_SCH] * phymib->aSlotTime;
		}
		if (epktCTRL[state_SCH]) {
			freePacket(epktCTRL[state_SCH]);
			epktCTRL[state_SCH]=0;
		}
		if (mhSend->paused_) {
#if PKT_FLOW_DEBUG
			printf("mhSend paused!\n");
#endif
			//mhSend->cancel();
		}
		if (mhNav->busy_)
		{
			mhNav->cancel();
			nav = 0;
		}
		for (int i = AC_BE;i <= AC_VO;i++) {
			if(mhDefer[i]->busy_){
#if PKT_FLOW_DEBUG
				printf("\t\tmhDefer[%d] busy!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",i+1);
#endif
				mhDefer[i]->cancel();
			}
			if (mhBackoff[i][state_CCH]->busy_) {
#if PKT_FLOW_DEBUG
				printf("\t\tnode[%d]:mhBackoff[%d] busy!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!\n",get_nid(),i);
#endif
				mhBackoff[i][state_CCH]->cancel();
				freePacket(epktDATA[i][state_CCH]);
				epktDATA[i][state_CCH] = 0; //FIXME should be modified to pause mode than cancel mode
			}
			if(epktRTS[i][state_SCH]) {
				freePacket(epktRTS[i][state_SCH]);
				epktRTS[i][state_SCH] = 0;
			}
			if (epktDATA[i][state_SCH] != 0) {
				//start_backoff(i);
				freePacket(epktDATA[i][state_SCH]);
				epktDATA[i][state_SCH] = 0;
			}
		}

		//start_backoff(now_ac);
		SET_TX_STATE(MAC_IDLE); 
		triggerSend();

	}
	return(1);	
}

int mac_80211p::txHandler(Event_ *e)
{
	tx_active[CH_state] = 0;  

	return(1); 
}

int mac_80211p::recvHandler(Event_ *e)
{
	Packet *p;
	struct hdr_mac802_11 *hm;
	struct logEvent *logep;
	struct mac802_11_log *log1,*log2;
	char *iph;
	u_long ipsrc,ipdst;  
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	assert(epktRx && 
			(rx_state == MAC_RECV || rx_state == MAC_COLL));

	/*
	 * If the interface is in TRANSMIT mode when this packet
	 * "arrives", then I would never have seen it and should
	 * do a silent discard without adjusting the NAV.
	 */
	if (tx_active[CH_state]) {
		freePacket(epktRx);
		goto done; 
	}

	/*
	 * handle collisions.
	 */
	if (rx_state == MAC_COLL) {
		if(now_ac[CH_state] > -1)
			set_NAV(eifs-difs+aifs[now_ac[CH_state]]);
		else
			set_NAV(eifs);

		freePacket(epktRx);
		epktRx = 0;
		goto done;
	}

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
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("pkt error\n");
#endif
		freePacket(epktRx);
		epktRx = 0;

		if(now_ac[CH_state] > -1)
			set_NAV(eifs-difs+aifs[now_ac[CH_state]]);
		else
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
		struct PHYInfo *pinfo = (struct PHYInfo *)p->pkt_getinfo("WAVEPhyinfo");
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
					//ErrorMesg("Invalid MAC Control Subtype !");
					NSLOBJ_DEBUG("Invalid MAC Control Subtype !");   
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

				default: //ErrorMesg("Invalid MAC Data Subtype");  
					NSLOBJ_DEBUG("Invalid MAC Data Subtype");
			}
			break;

		default: //ErrorMesg("Invalid MAC Type");   
			NSLOBJ_DEBUG("Invalid MAC Type");
	}

done:
	epktRx = 0;
	rx_resume(); 

	return 1;
}

int mac_80211p::check_pktCTRL(int pri) {

	struct hdr_mac802_11		*hm; 
	u_int32_t			timeout;
	Packet				*p;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();


	if (epktCTRL[CH_state] == 0)
		return(-1);
	if (tx_state[CH_state] == MAC_CTS || tx_state[CH_state] == MAC_ACK)
		return(-1);

	GET_PKT(p, epktCTRL[CH_state]); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();

	switch(hm->dh_fc.fc_subtype) {
		case MAC_Subtype_CTS:
			/* If medium is not idle,
			 * don't send CTS.
			 */
			if (!is_idle()) {
				drop(epktCTRL[CH_state], NULL);
				epktCTRL[CH_state] = 0;
				return(0); 
			}
			SET_TX_STATE(MAC_CTS);
			// ????????? why needs duration ??????????
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
			//ErrorMesg("Invalid MAC Control Type !");
			NSLOBJ_DEBUG("Invalid MAC Control Type !");
	}
	now_ac[CH_state] = pri;
	transmit(epktCTRL[CH_state], timeout);
	return(0); 
}


int mac_80211p::check_pktRTS(int pri) {

	struct hdr_mac802_11		*hm;
	u_int32_t			timeout; 
	Packet				*p;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("pri = %d\n", pri);
#endif
	assert(mhBackoff[pri][CH_state]->busy_ == 0);
	if (epktRTS[pri][CH_state] == 0)
		return(-1);

	GET_PKT(p, epktRTS[pri][CH_state]); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();

	switch(hm->dh_fc.fc_subtype) {
		case MAC_Subtype_RTS:
			if (!is_idle()) {
				inc_CW(pri);
				start_backoff(pri);
				return(0); 
			}
			SET_TX_STATE(MAC_RTS);
			timeout = CTSTimeout(nid,pid);
			break;

		default:
			//ErrorMesg("Invalid MAC Control Type !"); 
			NSLOBJ_DEBUG("Invalid MAC Control Type !");
	}
	now_ac[CH_state] = pri;
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("now_ac = %d\n", pri);
#endif
	transmit(epktRTS[pri][CH_state], timeout);
	return(0);
}


int mac_80211p::check_pktTx(int pri) {

	struct hdr_mac802_11		*hm;
	u_int32_t			timeout;
	Packet				*p;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port(); 

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("pri = %d\n", pri);
#endif

	assert(mhBackoff[pri][CH_state]->busy_ == 0);
	if (epktDATA[pri][CH_state] == 0)
		return(-1); 

	GET_PKT(p, epktDATA[pri][CH_state]); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();

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
				sendRTS(pri, hm->dh_addr1);
				inc_CW(pri);
				start_backoff(pri); 
				return(0); 
			}
			SET_TX_STATE(MAC_SEND);

			if (bcmp(hm->dh_addr1, etherbroadcastaddr, 
						ETHER_ADDR_LEN))
			{
				timeout = ACKTimeout(p->pkt_getlen(), pri,nid,pid);
			}
			else {	
				/* XXX: broadcasat 
				 * sent a braodcast packet and no
				 * ack need.  ???????????????????
				 */
				timeout = TX_Time(p->pkt_getlen(),(*bw_),nid,pid); 
#if PKT_FLOW_DEBUG
				NSLOBJ_DEBUG("TX_time = %u(tick)\n", timeout*10);
#endif
			}
			break; 

		default:
			//ErrorMesg("Invalid MAC Control Subtype !"); 
			NSLOBJ_DEBUG("Invalid MAC Control Subtype !");
	}
	now_ac[CH_state] = pri;
	transmit(epktDATA[pri][CH_state], timeout); 
	return(0); 
}


void mac_80211p::sendRTS(int pri, u_char *dst) {

	int				pktlen; 
	Packet				*p;
	struct rts_frame		*rf;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();



#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	assert(epktDATA[pri][CH_state] && !epktRTS[pri][CH_state]);
	GET_PKT(p, epktDATA[pri][CH_state]); 
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
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("RTS doesn't send\n");
#endif
		return; 
	}

	/* Generate an IEEE 802.11 RTS frame */
	assert(epktRTS[pri][CH_state] == NULL);
	epktRTS[pri][CH_state] = createEvent();
	p = new Packet; assert(p); 
	rf = (struct rts_frame *)p->pkt_malloc(sizeof(struct rts_frame));  
	assert(rf);
	ATTACH_PKT(p, epktRTS[pri][CH_state]); 

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


void mac_80211p::sendCTS(u_char *dst, u_int16_t d) {

	struct cts_frame		*cf;
	Packet				*p;
	u_int32_t                       nid = get_nid();
	u_int32_t                       pid = get_port();



	assert(epktCTRL[CH_state] == 0); 

	/* Generate a CTS frame */
	epktCTRL[CH_state] = createEvent();
	p = new Packet; assert(p); 
	cf = (struct cts_frame *)p->pkt_malloc(sizeof(struct cts_frame));
	assert(cf); 
	ATTACH_PKT(p, epktCTRL[CH_state]); 

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


void mac_80211p::sendACK(u_char *dst) {

	struct ack_frame		*af;
	Packet				*p;


#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	assert(epktCTRL[CH_state] == 0);

	/* Generate ACK Frame */
	epktCTRL[CH_state] = createEvent();
	p = new Packet; assert(p); 
	af = (struct ack_frame *)p->pkt_malloc(sizeof(struct ack_frame)); 
	assert(af);
	ATTACH_PKT(p, epktCTRL[CH_state]); 

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


void mac_80211p::sendDATA(int pri, u_char *dst) {

	struct hdr_mac802_11		*mh;
	Packet				*p;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();

	assert(epktDATA[pri][CH_state]); 
	GET_PKT(p, epktDATA[pri][CH_state]); 
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

		if(edca_txop > GetCurrentTime()){
			mh->dh_duration = edca_txop - GetCurrentTime();
		}else{
			edca_txop = 0;
			mh->dh_duration = DATA_DURATION(nid,pid);
		}


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


void mac_80211p::transmit(ePacket_ *pkt, u_int32_t timeout)
{
	//struct hdr_mac802_11 *mh=NULL;
	Packet *p, *RxPkt; 
	Event *ep; 
	struct mac802_11_log *log1,*log2;
	char *iph;
	u_long ipdst,ipsrc;
	struct logEvent *logep;
	struct TXVECTOR *tx;
	u_int64_t time_period;
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
	if (CH_state == state_CCH) { 
		printf("\t\tCCH transmit!!\n");
	} else {
		printf("\t\tSCH transmit!!\n");
	}
#endif
	tx_active[CH_state] = 1;
	GET_PKT(p, pkt);
	//DSR_Info *dsr_info = (DSR_Info*)p->pkt_getinfo( "dsr" );
	//hychen added for CRC32 check
	struct frame_control	*fc;
	struct ether_header     *eh;
	struct ack_frame	*af;
	struct rts_frame	*rf;
	struct cts_frame	*cf;
	struct hdr_mac802_11 *mh=NULL;
	int     f_len,pl_len;
	unsigned int crc;
	char    *tmp,*buf;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();

	GET_PKT(p,pkt);
	fc = (struct frame_control*)p->pkt_get();
	if ((fc->fc_type == MAC_Type_Control) && (fc->fc_subtype ==MAC_Subtype_ACK))    //Calculate CRC for ACK
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
	}else if ((fc->fc_type == MAC_Type_Control) && (fc->fc_subtype ==MAC_Subtype_RTS))      //Calculate CRC for RTS
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
	else if ((fc->fc_type == MAC_Type_Control) && (fc->fc_subtype ==MAC_Subtype_CTS))       //Calculate CRC for CTS
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
		eh=(struct ether_header *)(p->pkt_get()+sizeof(struct hdr_mac802_11));
		tmp = p->pkt_sget();
		pl_len = p->pkt_getlen() - (sizeof(struct hdr_mac802_11)+sizeof(struct ether_header));
		f_len = p->pkt_getlen() - ETHER_FCS_LEN;
		buf = (char*)malloc(f_len);
		memset(buf,0,f_len);
		memcpy(buf,mh,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
		memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,eh,sizeof(struct ether_header));
		memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN+sizeof(struct ether_header),tmp,pl_len);
		crc = crc32(0,buf,f_len);
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC = %08x\n",crc);
#endif
		memset(mh->dh_fcs,0,4);
		memcpy(mh->dh_fcs,&crc,4);
		free(buf);

	}
	else    //Calculate CRC for DATA frame
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
		if (epktRx) { 	//XXX : should be log ?
			GET_PKT(RxPkt, epktRx);
			/* force packet discard */
			RxPkt->pkt_err_ = 1; 
		} else if (((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->mhRecv->busy_ ) {
			((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->mhRecv->cancel();
			((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->PHY_CCARESET_request(); 
		}
	}
	u_int64_t remainTime = (u_int64_t)(channel_interval_tick - (GetCurrentTime()%channel_interval_tick));
#if CHstate_DEBUG 
	printf("\t\ttx_state = %d, remain time = %llu, timeout = %u,pkt size = [%d]\n", tx_state[CH_state], remainTime, timeout*10,p->pkt_getlen());
#endif
	if ((timeout*10) > remainTime && tx_state[CH_state] != MAC_ACK && !IS_CH_SWITCH_PAUSED) 
	{
#if CHstate_DEBUG 
		printf("\t\tremain time = %llu, current TX should be cancelled\n",remainTime);
#endif
		/*if(remainTime/10 < (PRM_Time+ACK_Time+sifs+aifs[now_ac]))
		  {	
#if PKT_FLOW_DEBUG//temp
printf("free epktDATA[%d]\n",now_ac);
#endif
freePacket(epktDATA[now_ac]);
epktDATA[now_ac] = 0;
tx_active[CH_state] = 0;
		//temp
		printf("transmit::epktDATA = null\n");
		return;
		}*/
		//freePacket(epktDATA[now_ac[CH_state]][CH_state]);
		//epktDATA[now_ac[CH_state]][CH_state] = 0;
		tx_active[CH_state] = 0;
		return;
	}
	if( is_PHY_idle() )  //if PHY idle
	{
		tx = new TXVECTOR;
		tx->LENGTH = p->pkt_getlen();
		tx->DATARATE = *bw_/1000000;
		((phy_80211p*)InstanceLookup(nid,pid,"phy_80211p"))->PHY_TXSTART_request(*tx);
		delete tx;
	}
	else
	{
#if PKT_FLOW_DEBUG
		printf("Busy\n");
#endif
		tx_suspend = 1;
		//tx_active[CH_state] = 0;
		//epktDATA[now_ac[CH_state]][CH_state] = 0;
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
		if ( tx_state[CH_state] == MAC_SEND	/* means send data packets */ 
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


	/* log "StartTX" and "SuccessTX" event */
	if(_ptrlog == 1){
		if(mh == NULL)
			mh = (struct hdr_mac802_11 *)p->pkt_get();

		p->pkt_addinfo("LOG_TX_NID",(char*)&nid , sizeof(int));
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
	/* FIXME
	   if (mytunidp != NULL) {
	   fifoIFcurqlen[*mytunidp] = if_snd.ifq_len;
	   printf("ifq_len = %d\n",if_snd.ifq_len);
	   }
	   */
	//	freePacket(pkt);
	assert(put(ep, sendtarget_)); 
}


void mac_80211p::retransmitRTS() {

	struct rts_frame		*rf;
	Packet				*p;
	ePacket_			*dupP_;


	assert(epktDATA[now_ac[CH_state]][CH_state] && epktRTS[now_ac[CH_state]][CH_state]);
	assert(mhBackoff[now_ac[CH_state]][CH_state]->busy_ == 0);

	macmib->aRTSFailureCount ++;
	ssrc++;	// STA Short Retry Count

	if (ssrc >= macmib->aShortRetryLimit) {
		/*
		 * For mobile node routing protocol.
		 * If packet be dropped due to retry count
		 * >= retry limit, we call an error handler.
		 */
		dupP_ = pkt_copy(epktDATA[now_ac[CH_state]][CH_state]);  // for dropping message 
		GET_PKT(p, dupP_);
		p->pkt_seek(sizeof(struct hdr_mac802_11) +
				sizeof(struct ether_header));
		if (-1 == p->pkt_callout(dupP_)) {
			//			printf("mac_80211p::retransmitRTS: pkt_callout failed\n");
			freePacket(dupP_);
		}

		/*
		 * IEEE 802.11 Spec, section 11.4.4.2.16
		 * IEEE 802.11 Spec, section 9.2.5.3
		 */
		drop(epktRTS[now_ac[CH_state]][CH_state], NULL);
		drop(epktDATA[now_ac[CH_state]][CH_state], NULL);
		epktRTS[now_ac[CH_state]][CH_state] = epktDATA[now_ac[CH_state]][CH_state] = 0;   
		//kcliu start : push buffer
		triggerSend();			
		//end

		ssrc = 0; rst_CW(now_ac[CH_state]); 
		now_ac[CH_state] = -1;  
	}
	else {
		GET_PKT(p, epktRTS[now_ac[CH_state]][CH_state]); 
		rf = (struct rts_frame *)p->pkt_get();
		rf->rf_fc.fc_retry = 1;

		inc_CW(now_ac[CH_state]); 

		/* IEEE 802.11 Spec, section 9.2.5.7 */
		start_backoff(now_ac[CH_state]); 
	}
}


void mac_80211p::retransmitDATA() {

	struct hdr_mac802_11	*hm;
	Packet			*p;
	ePacket_		*dupP_;
	int			pktlen; 
	u_int16_t		*rcount, *thresh;



	//assert(mhBackoff->busy_ == 0);  /* XXX */
	assert(epktDATA[now_ac[CH_state]][CH_state] && epktRTS[now_ac[CH_state]][CH_state] == 0);

	macmib->aACKFailureCount++;

	GET_PKT(p, epktDATA[now_ac[CH_state]][CH_state]); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();  
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
		dupP_ = pkt_copy(epktDATA[now_ac[CH_state]][CH_state]); // for dropping message 
		GET_PKT(p, dupP_);
		p->pkt_seek(sizeof(struct hdr_mac802_11) +
				sizeof(struct ether_header));
		if (-1 == p->pkt_callout(dupP_)) {
			//			printf("mac_80211p::retransmitDATA, pkt_callout failed\n");
			freePacket(dupP_);
		}

		macmib->aFailedCount++;
		drop(epktDATA[now_ac[CH_state]][CH_state], NULL); 
		epktDATA[now_ac[CH_state]][CH_state] = 0;
		//kcliu start : push buffer
		triggerSend();
		//end
		*rcount = 0;
		rst_CW(now_ac[CH_state]);
		edca_txop = 0;
		now_ac[CH_state] = -1; 
	}
	else {
		hm->dh_fc.fc_retry = 1;
		sendRTS(now_ac[CH_state],hm->dh_addr1);
		inc_CW(now_ac[CH_state]);

		/* IEEE 802.11 Spec, section 9.2.5.2 */
		start_backoff(now_ac[CH_state]); 
	}
}


void mac_80211p::recvRTS(ePacket_ *p) {

	struct rts_frame		*rf;
	Packet				*pkt;

	int f_len;
	unsigned int crc,fcs;
	char *buf;


	if (tx_state[CH_state] != MAC_IDLE) {
		drop(p, NULL);
		p = NULL;
		return; 
	} 

	/*
	 * If I'm responding to someone else, discard the RTS.
	 */
	if (epktCTRL[CH_state]) {
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
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
#endif
		drop(p, NULL);
		p = NULL;
		return;
	}

	sendCTS(rf->rf_ta, rf->rf_duration);

	/* stop deffering */
	for(int i=AC_VO ; i >= AC_BE ; i--)
	{
		if (mhDefer[i]->busy_)
			mhDefer[i]->cancel();
	}

	freePacket(p); 
	tx_resume();
}


void mac_80211p::recvCTS(ePacket_ *p) {

	struct cts_frame                *cf;
	Packet                          *pkt;

	int f_len;
	unsigned int crc,fcs;
	char *buf;

	if (tx_state[CH_state] != MAC_RTS) {
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
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
#endif
		drop(p, NULL);
		p = NULL;
		return;
	}

	assert(now_ac[CH_state] > -1);
	assert(epktRTS[now_ac[CH_state]][CH_state] && epktDATA[now_ac[CH_state]][CH_state]);
	freePacket(epktRTS[now_ac[CH_state]][CH_state]); 
	epktRTS[now_ac[CH_state]][CH_state] = 0;

	mhSend->cancel();
	freePacket(p); 

	/* 
	 * The successful reception of this CTS packet implies
	 * that our RTS was successful. Hence, we can reset
	 * the Short Retry Count and the CW.
	 */
	ssrc = 0;
	rst_CW(now_ac[CH_state]);

	tx_resume(); 
}


void mac_80211p::recvACK(ePacket_ *p) {

	Packet				*pkt;
	int				pktlen; 
	u_int64_t			inTicks;
	u_int32_t			ttime;
	struct hdr_mac802_11		*hm;

	int f_len;
	unsigned int crc,fcs;
	char *buf;
	struct ack_frame *af;

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("tx_state = %d\n",tx_state[CH_state]);
#endif

	if (tx_state[CH_state] != MAC_SEND) {
		drop(p, NULL);
		p = NULL;
		return;
	}
	GET_PKT(pkt,p);
	af = (struct ack_frame*)pkt->pkt_get();
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
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("CRC Error (%08x)\n", crc);
#endif
		drop(p, NULL);
		p = NULL;
		return;
	}

	assert(epktDATA[now_ac[CH_state]][CH_state]);
	GET_PKT(pkt, epktDATA[now_ac[CH_state]][CH_state]);
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

		GET_PKT(pkt, epktDATA[now_ac[CH_state]][CH_state]);
		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
			OutThrput += (double)pkt->pkt_getlen() / 1000.0;
		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
			InOutThrput += (double)pkt->pkt_getlen() / 1000.0;
	}

	ePacket_ *tmp_pkt;
	IF_DEQUEUE(&if_snd[now_ac[CH_state]][CH_state], tmp_pkt); //dequeue
	freePacket(tmp_pkt);
	tmp_pkt = 0;

#if PKT_FLOW_DEBUG//temp
	NSLOBJ_DEBUG("epktDATA[%d]\n",now_ac[CH_state]);
#endif
	freePacket(epktDATA[now_ac[CH_state]][CH_state]); 
	epktDATA[now_ac[CH_state]][CH_state] = 0;

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
	rst_CW(now_ac[CH_state]);
	assert(mhBackoff[now_ac[CH_state]][CH_state]->busy_ == 0);

	if(if_snd[now_ac[CH_state]][CH_state].ifq_len > 0) {
		tmp_pkt = if_snd[now_ac[CH_state]][CH_state].ifq_head;
		assert(pkt && epktDATA[now_ac[CH_state]][CH_state] == NULL);
		epktDATA[now_ac[CH_state]][CH_state] = pkt_copy(tmp_pkt);		/* hold this packet into send buffer */
		assert(epktDATA[now_ac[CH_state]][CH_state]);
		//pq_[now_ac]._size -= pkt->pkt_getlen();
		GET_PKT(pkt, epktDATA[now_ac[CH_state]][CH_state]);


		if (edca_txop > GetCurrentTime()){

			//SET_TX_STATE(MAC_IDLE);
			u_int32_t nid = get_nid();
			u_int32_t pid = get_port();
			//ttime = 2*sifs + TX_Time(MAC80211_ACK_LEN,(*bw_)) + TX_Time(pkt->pkt_getlen(),(*bw_));
			ttime = 2*sifs + TX_Time(MAC80211_ACK_LEN,(*bw_),nid,pid) + TX_Time(pkt->pkt_getlen(),(*bw_),nid,pid);
			MICRO_TO_TICK(inTicks, ttime);

			if ((edca_txop - GetCurrentTime()) < inTicks) {
				mhNav->start(edca_txop-GetCurrentTime(),0);  //mwhsu
				start_backoff(now_ac[CH_state]);
				edca_txop = 0;
				now_ac[CH_state] = -1;
				tx_resume();
				return;
			}

			hm = (struct hdr_mac802_11 *)pkt->pkt_get();
			sendDATA(now_ac[CH_state],hm->dh_addr1);	
			sendRTS(now_ac[CH_state],hm->dh_addr1);	

			MICRO_TO_TICK(inTicks, sifs);
			start_sifs(sifs);
			return;
		} else {
			edca_txop = 0;
		}
	}

	start_backoff(now_ac[CH_state]);
	edca_txop = 0;
	now_ac[CH_state] = -1;
	tx_resume();
}


void mac_80211p::recvDATA(ePacket_ *p) {

	struct hdr_mac802_11		*hm;
	Packet				*pkt;
	int				match; 

	//hychen added
	int f_len,pl_len;
	unsigned int crc,fcs;
	char *tmp,*buf;
	struct PHYInfo          *phyInfo;
	struct ether_header	*eh;


#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif

	GET_PKT(pkt, p); 
	//DSR_Info *dsr_info = (DSR_Info*)pkt->pkt_getinfo( "dsr" );
	hm = (struct hdr_mac802_11 *)pkt->pkt_get();
	eh = (struct ether_header*)(pkt->pkt_get()+sizeof(struct hdr_mac802_11));
	if(eh->ether_type == (htons(ETHERTYPE_ARP)))    //Calculate ARPRequest CRC , ARP mode = RunARP
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
		phyInfo =(struct PHYInfo*) pkt->pkt_getinfo("WAVEPhyinfo");
		tmp = pkt->pkt_sget();
		pl_len = phyInfo->payload_len;
		f_len = phyInfo->payload_len +sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN+sizeof(struct ether_header);
		buf = (char*)malloc(f_len);
		memset(buf,0,f_len);
		memcpy(buf,hm,sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN);
		memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN,eh,sizeof(struct ether_header));
		memcpy(buf+sizeof(struct hdr_mac802_11)-ETHER_FCS_LEN+sizeof(struct ether_header),tmp,pl_len);
		crc = crc32(0,buf,f_len);
	}
	else                                            //MAC_Type_DATA
	{
		phyInfo =(struct PHYInfo*) pkt->pkt_getinfo("WAVEPhyinfo");
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
			f_len =  phyInfo->mac_hdr_length - ETHER_FCS_LEN;
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




	if (bcmp(hm->dh_addr1, etherbroadcastaddr, 
				ETHER_ADDR_LEN))
	{	/* unicast address */
		if (pkt->pkt_getlen() >= (int)macmib->aRTSThreshold) {
			if (tx_state[CH_state] == MAC_CTS) {
				assert(epktCTRL[CH_state]);
				freePacket(epktCTRL[CH_state]); 
				epktCTRL[CH_state] = 0;  

				mhSend->cancel();
				ssrc = 0;
				rst_CW(now_ac[CH_state]); 
			}
			else {
				drop(p, NULL);
				p = NULL;
				return; 
			}
			sendACK(hm->dh_addr2);
			tx_resume(); 
		}
		/*
		 * We did not send a CTS and there is no 
		 * room to buffer an ACK.
		 */
		else {
			if (epktCTRL[CH_state]) {
				drop(p, NULL);
				p = NULL;
				return; 
			}
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
	/* MoblNode module and AP module would decapsulate the 802.11 header */
	/* decapsulate 802.11 header */
	pkt->pkt_seek(sizeof(struct hdr_mac802_11)); 
	assert(put(p, recvtarget_)); 
}


int mac_80211p::is_idle() {

	if (rx_state != MAC_IDLE)
		return(0);

	if (tx_state[CH_state] != MAC_IDLE)
		return(0);

	if (nav > GetCurrentTime())
		return(0);
	return(1);  
}


void mac_80211p::tx_resume() {

	u_int32_t		pktlen, dt;
	Packet			*p; 

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("tx_state = %d,rx_state = %d\n",tx_state[CH_state],rx_state);
#endif

	assert((mhSend->busy_==0)/*XXX &&(mhDefer->busy_==0)*/ );

	if (epktCTRL[CH_state]) {
		/*
		 * need to send a CTS or ACK.
		 */
		assert(mhDefer[AC_VO]->busy_ == 0);
		start_sifs(sifs); 
	} else if (now_ac[CH_state] > -1) {
		assert( mhDefer[now_ac[CH_state]]->busy_ == 0);
		if (epktRTS[now_ac[CH_state]][CH_state]) {
			if (mhBackoff[now_ac[CH_state]][CH_state]->busy_ == 0) {
				start_defer(now_ac[CH_state], aifs[now_ac[CH_state]]); 
			}
		} 
		else if (epktDATA[now_ac[CH_state]][CH_state]) {
			/*
			 * If we transmit MPDU using RTS/CTS, then we should defer
			 * sifs time before MPDU is sent; contrarily, we should
			 * defer difs time before MPDU is sent when we transmit
			 * MPDU without RTS/CTS.
			 */
			GET_PKT(p, epktDATA[now_ac[CH_state]][CH_state]);
			pktlen = p->pkt_getlen() - sizeof(struct hdr_mac802_11);

			if (pktlen < macmib->aRTSThreshold)
				dt = sifs;
			else 	
				dt = aifs[now_ac[CH_state]];  

			if (mhBackoff[now_ac[CH_state]][CH_state]->busy_ == 0)
				start_defer(now_ac[CH_state],dt); 
		}
	}
	SET_TX_STATE(MAC_IDLE); 
}


void mac_80211p::rx_resume() {

#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("tx_state = %d,rx_state = %d\n",tx_state[CH_state],rx_state);
#endif
	if (PHY_state == 1) {
		return;
	}
	assert((epktRx==0)&&(mhRecv->busy_==0));
	SET_RX_STATE(MAC_IDLE); 
}

int mac_80211p::update_dcache(u_char *mac, u_int16_t sc)
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


int mac_80211p::Fragment(ePacket_ *pkt, u_char *dst)
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

void mac_80211p::drop(ePacket_ *p, char *why)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("drop reason is : %s\n", why);
#endif
	freePacket(p); 
}

int mac_80211p::log()
{
	double		cur_time;
	//printf("mac_80211p::log %d\n",get_nid());

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
void mac_80211p::inc_CW(int pri) {

	cw[pri] = (cw[pri] << 1) + 1;

	if(cw[pri] > CWmax[pri][CH_state])
		cw[pri] = CWmax[pri][CH_state];
}
void mac_80211p::rst_CW ( int pri) 
{
	cw[pri] = CWmin[pri][CH_state];
}


int mac_80211p::PORTtoUP(int port)
{
	for( int i = 0; i <= 7 ; i++ )
	{
		if( plevel[i] == port )	
			return i;
	}
	return -1;
}

int mac_80211p::whichAC(int port)
{
	int pri = PORTtoUP(port);
	if(pri > 3)       return (pri >> 1);  
	else if(pri == 0 || pri == 3)   return 1;                              
	else return  0;                                              
}

void mac_80211p::channel_dump()
{
	if (CH_state == state_SCH) {
		printf("\tnow is SCH\n");
	} else {
		printf("\tnow is CCH\n");
	}
	for (int ch = 0;ch < CH_NUM;ch++) {
		if (ch == state_SCH) {
			printf("\tSCH parameters:\n");
		} else {
			printf("\tCCH parameters:\n");
		}	
		printf("\t\tnow_ac = [%d]\n",now_ac[ch]);
		printf("\t\ttx_state = [%d]\n",tx_state[ch]);
		printf("\t\ttx_active = [%d]\n",tx_active[ch]);
		printf("\t\tepktCTRL = [%p]\n",epktCTRL[ch]);
		for (int ac = 0;ac < AC_NUM;ac++) {
			printf("\t\tcw[%d] = [%d]",ac,cw[ac]);
			printf("\tepktDATA[%d] = [%p]\n",ac,epktDATA[ac][ch]);
		}
	}
}
void mac_80211p::queue_dump()
{
	printf("node[%d]:Current Time = [%llu], Current AC = [%d]\n",get_nid(),GetCurrentTime(),now_ac[CH_state]);
	for(int i = 0; i < AC_NUM;i++ )
	{
		printf("if_snd[%d]:\n",i);
		printf("\tifq_head = \t%p\n",if_snd[i][CH_state].ifq_head);
		printf("\tifq_tail = \t%p\n",if_snd[i][CH_state].ifq_tail);
		printf("\tifq_pri = \t%d\n",if_snd[i][CH_state].ifq_pri);
		printf("\tifq_len = \t%d\n",if_snd[i][CH_state].ifq_len);
		printf("\tifq_maxlen = \t%d\n",if_snd[i][CH_state].ifq_maxlen);
		printf("\tifq_drops = \t%d\n",if_snd[i][CH_state].ifq_drops);
	}
}
// BUSY: 1
// IDLE: 0
	void 
mac_80211p::set_PHY_state(int st)
{
	PHY_state = (bool)st;
}
// If PHY is idle, return true, else false.
	bool 
mac_80211p::is_PHY_idle()
{
	return !PHY_state; 
}
	void 
mac_80211p::triggerSend()
{	
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	for (int ac = AC_BE;ac <= AC_VO;ac++){
		ePacket_ *tmp_pkt;
		if (if_snd[ac][CH_state].ifq_head){
			tmp_pkt = if_snd[ac][CH_state].ifq_head;
			push_buf(tmp_pkt,ac);
		}
	}
}
	void
mac_80211p::triggerMAC()
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("now_ac=%d,CH_state=%d\n",now_ac[CH_state],CH_state);
#endif
	if ( !tx_suspend ){
		return;
	}
	u_int32_t			timeout;
	struct hdr_mac802_11		*hm;
	Packet				*p;
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();

	tx_suspend = 0;
	GET_PKT(p, epktDATA[now_ac[CH_state]][CH_state]); 
	hm = (struct hdr_mac802_11 *)p->pkt_get();

	if (bcmp(hm->dh_addr1, etherbroadcastaddr, 
				ETHER_ADDR_LEN))
	{
		timeout = ACKTimeout(p->pkt_getlen(), now_ac[CH_state],nid,pid);
	}
	else {	
		/* XXX: broadcasat 
		 * sent a braodcast packet and no
		 * ack need.  ???????????????????
		 */
		timeout = TX_Time(p->pkt_getlen(),(*bw_),nid,pid);
		NSLOBJ_DEBUG("TX_time = %u(tick)\n", timeout*10);
	}
	transmit(epktDATA[now_ac[CH_state]][CH_state], timeout); 
	/*
	   for (int ac = AC_BE;ac <= AC_VO;ac++){
	   ePacket_ *tmp_pkt;
	   if (if_snd[ac][CH_state].ifq_head){
	   tmp_pkt = if_snd[ac][CH_state].ifq_head;
	   push_buf(tmp_pkt,ac);
	   }
	   }
	   */
}
void mac_80211p::start_channel_switch(int ch, int cancel_WBSS_flag, u_int32_t psid)
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	IS_CH_SWITCH_PAUSED = 0;
	current_provider = psid;
	cancel_WBSS = cancel_WBSS_flag; //  check if it would start pkt count mechanism, 0:off ; 1:on
	service_channel_num = ch;
	//NSLOBJ_DEBUG("nid=%d ,service_channel_num = %d\n",get_nid(),service_channel_num);
}
void mac_80211p::pause_channel_switch()
{
#if PKT_FLOW_DEBUG
	NSLOBJ_DEBUG("\n");
#endif
	IS_CH_SWITCH_PAUSED = 1;
	CH_state = state_CCH;
	*ch_ = 178;
}

void mac_80211p::check_if_cancel_WBSS()
{
	u_int32_t nid = get_nid();
	u_int32_t pid = get_port();
	if (SCH_norecv && cancel_WBSS) {
#if PKT_FLOW_DEBUG
		NSLOBJ_DEBUG("WBSS should be cancelled\n");
#endif
		//call WME WBSS inactivity
		((WME *)InstanceLookup(nid,pid,"WME"))->MLME_CHANNELINACTIVITY_indication((service_channel_num));
	}
}

/* 
 * this function should be put into transmit() in order to check if any pkt is being sent during SCH.
 */
void mac_80211p::check_if_SCH_recv(u_int32_t *psid)
{
	if (cancel_WBSS) {			// if turn on the "SCH pkt count mechanism"
		if(psid == NULL)
			return;
		if (CH_state == state_SCH && SCH_norecv && *psid == current_provider) { 	// Service Channel doesn't recv any pkt yet
			SCH_norecv = 0;	// SCH at least received one pkt, therefore, SCH_norecv flag should be set as FALSE
#if PKT_FLOW_DEBUG
			NSLOBJ_DEBUG("get one pkt\n");
#endif
		}
	}
}

void mac_80211p::set_mac_rxstate()
{
	if (rx_state == MAC_IDLE) {
		SET_RX_STATE(MAC_RECV);
	}
}
