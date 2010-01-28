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

#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <ip.h>
#include <gbind.h>
#include <packet.h>
#include <random.h>
#include <ethernet.h>
#include <wphyInfo.h>
#include <phy/wphy.h>
#include <mac/mac-802_11e.h>
#include <mbinder.h>

#include <mac/qos-scheduler.h>
#include <mac/sta-scheduler.h>
#include <mac/hc-scheduler.h>

MODULE_GENERATOR(mac802_11e); 

extern u_char etherbroadcastaddr[ETHER_ADDR_LEN];

/*=======================================================================  
   Define Macros
  ========================================================================*/
#define CHECK_BACKOFF_TIMER()								\
{											\
	if(is_idle() && !hcca_stat.has_control && !mhHCCATx->busy_) { 			\
		for (int i = AC_VO; i >= AC_BE; i--) {					\
			if (mhBackoff[i]->paused_) {					\
				u_int64_t tm;						\
				MICRO_TO_TICK(tm, aifs[i]);				\
				mhBackoff[i]->resume(tm);				\
			} 								\
		}									\
	} else {									\
		for (int i = AC_VO; i >= AC_BE; i--) {					\
			if ( mhBackoff[i]->busy_ && !mhBackoff[i]->paused_ )		\
				mhBackoff[i]->pause(); 					\
		}									\
	}										\
}

#define SET_TX_STATE(x)									\
{											\
	tx_state = (x);  								\
	CHECK_BACKOFF_TIMER(); 								\
}

#define SET_RX_STATE(x)									\
{											\
	rx_state = (x);  								\
	CHECK_BACKOFF_TIMER();	 							\
}

  
mac802_11e::mac802_11e(u_int32_t type, u_int32_t id, struct plist* pl, const char *name): NslObject(type, id, pl, name)
{
	bw_ = 0;
	_ptrlog = 0;
	Beacon_Timeval = 0;


	for (int i = AC_VO; i >= AC_BE; i--) {		
		mhBackoff[i] = NULL;
		mhDefer[i] = NULL;
	}

	macmib = NULL;
	phymib = NULL;

  	/* bind variable */
	vBind_mac("mac", mac_);
	vBind("QoS_Priority", &plevel);
	vBind("PromisOpt", &promiscuous_bind);
	vBind("QoSmode",&qosmode);

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

        // Changed by Prof. Wang on 08/06/2003 here. Previously there is a Randomize()
        // function called here. However, this should not happen. Otherwise, the random
        // number seed will be reset every time when a new mobile node is created.
	/* init sequence number */
 	sta_seqno = Random() % 4096;

	/* init MAC state */
	rx_state 	= MAC_IDLE;
	tx_state 	= MAC_IDLE;
	tx_active 	= 0;  
	
	/* init EDCA 4 AC parameters */
	CWmin[AC_BE] = phymib->aCWmin;
	CWmin[AC_BK] = phymib->aCWmin;
	CWmin[AC_VI] = (phymib->aCWmin+1)/2 - 1;
	CWmin[AC_VO] = (phymib->aCWmin+1)/4 - 1;	

	CWmax[AC_BE] = phymib->aCWmax;
	CWmax[AC_BK] = phymib->aCWmax;
	CWmax[AC_VI] = phymib->aCWmin;
	CWmax[AC_VO] = (phymib->aCWmin+1)/2 - 1;	
	
	/* based on WME spec. */
	edca_txop = 0;
	txopLimit[AC_BE] = 0;
	txopLimit[AC_BK] = 0;
	txopLimit[AC_VI] = 188;  /* 188 << 5, unit is 32-usec */  
	txopLimit[AC_VO] = 102;  /* 102 << 5, unit is 32-usec */

	/* initialize buffers */
	epktRx 		= 0;
	epktTx		= 0;
	epktBuf 	= 0;  
	epktCTRL	= 0;

        for (int i = AC_VO; i >= AC_BE; i--) {
		epktDATA[i] = 0;
		epktRTS[i] = 0;
                cw[i] = CWmin[i];
        }

   	/* initialize others */
	ssrc = 0;
	slrc = 0;
	nav = 0;
	
	isQAP 	= 0;
	action 	= NO_ACTION;
	now_ac  = -1;

	last_beacon = 0;
	near_beacon = 0;
	
	bzero(ap_mac,ETHER_ADDR_LEN);

	/* init cache */
	bzero(cache, sizeof(struct dup_cache)*CACHE_SIZE); 

	/* register variable */
	REG_VAR("promiscuous", &promiscuous);
}


mac802_11e::~mac802_11e()
{
	if (NULL != phymib)
		free(phymib);

	if (NULL != macmib)
		free(macmib);

	if (NULL != epktRx)
		freePacket(epktRx);

	if (NULL != epktCTRL)
		freePacket(epktCTRL);

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

	if (NULL != mhCAP)
		delete mhCAP;

	if (NULL != mhHCCATx)
		delete mhHCCATx;

	for (int i = AC_VO; i >= AC_BE; i--) 
	{
		if (NULL != epktRTS[i])
			freePacket(epktRTS[i]);
		if (NULL != epktDATA[i])
			freePacket(epktDATA[i]);
		if (NULL != mhBackoff[i])
			delete mhBackoff[i];
		if (NULL != mhDefer[i])
			delete mhDefer[i];
	}
}


int mac802_11e::init_PHYMIB()
{
	phymib = (struct PHY_MIB *)malloc(sizeof(struct PHY_MIB));
 	assert(phymib);

	/* IEEE 802.11 Spec, Page 237 */
	phymib->aCWmin = 31; 
	phymib->aCWmax = 1023;
 	phymib->aSlotTime = 20; 		/* 20 us */
   	phymib->aCCATime = 15;			/* 15 us */
  	phymib->aTxRxTurnaroundTime = 15;	/* 15 us */
	phymib->aSIFSTime = 10;  		/* 10 us */
	phymib->aPreambleLength = 144;		/* 144 bits */
  	phymib->aPLCPHeaderLength = 48;  	/* 48 bits */

	return(1); 
}

int mac802_11e::init_MACMIB() {

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


int mac802_11e::init_Timer() {

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
	mhCAP	  = new timerObj;
	mhHCCATx  = new timerObj;
	
	mhDefer[0] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, BE_deferHandler);
	mhDefer[0]->setCallOutObj(this, type);

	mhDefer[1] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, BK_deferHandler);
	mhDefer[1]->setCallOutObj(this, type);

	mhDefer[2] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, VI_deferHandler);
	mhDefer[2]->setCallOutObj(this, type);

	mhDefer[3] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, VO_deferHandler);
	mhDefer[3]->setCallOutObj(this, type);

	mhBackoff[0] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, BE_backoffHandler);
   	mhBackoff[0]->setCallOutObj(this, type);

	mhBackoff[1] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, BK_backoffHandler);
   	mhBackoff[1]->setCallOutObj(this, type);

	mhBackoff[2] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, VI_backoffHandler);
   	mhBackoff[2]->setCallOutObj(this, type);

	mhBackoff[3] = new timerObj;
	type = POINTER_TO_MEMBER(mac802_11e, VO_backoffHandler);
   	mhBackoff[3]->setCallOutObj(this, type);

	/* 
	 * Associate timer with corresponding 
	 * time-handler method.
	 */
	type = POINTER_TO_MEMBER(mac802_11e, navHandler);
	mhNav->setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(mac802_11e, sendHandler);
 	mhSend->setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(mac802_11e, txHandler);
 	mhIF->setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(mac802_11e, recvHandler);
 	mhRecv->setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(mac802_11e, sifsHandler); 
	mhSIFS->setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(mac802_11e, capHandler); 
	mhCAP->setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(mac802_11e, hccaTxHandler); 
	mhHCCATx->setCallOutObj(this, type);

	return(1); 
}

void mac802_11e::init_TSPEC()
{
        FILE    	*fd;
        char    	buf[150], *ptr;
	u_int32_t 	num, self_id;
        u_int32_t 	nid = 0, tfc = 0;
        struct 	TSPEC   *ts;
        char    	*FILEPATH = (char *)malloc(strlen(GetScriptName())+5);

	/* initialize HCCA Status */
	hcca_stat.hcca_on = 0;
	hcca_stat.has_control = 0;
	hcca_stat.need_ack = 0;
	hcca_stat.cap_expire = 0;
	hcca_stat.ack_tid = 0;	
	hcca_stat.hcca_tx_active = 0;
	hcca_stat.sense_idle = 0;
	hcca_stat.idle_time = 0;
	bzero(hcca_stat.ack_dst, ETHER_ADDR_LEN);

        strcpy(FILEPATH, GetScriptName());
        sprintf(FILEPATH,"%s%s",FILEPATH,".tsc");

      	if((fd = fopen(FILEPATH,"r")) == NULL){
                printf("Warning: can't open file %s.\n",FILEPATH);
                return;
        }

        free(FILEPATH);

	self_id = get_nid();
	nid = tfc = 0;

        while(!feof(fd)) {
                buf[0] = '\0'; fgets(buf, 130, fd);
                if ((buf[0]=='\0')||(buf[0]=='#'))
                        continue;

                if(nid == 0)
                {
                        nid = (u_int32_t)atoi(buf);
                        buf[0] = '\0'; fgets(buf, 130, fd);
                        //buf[strlen(buf)-1] = '\0';
                        tfc = atoi(buf);
			if(tfc == 0) nid = 0;
			continue;
		}else{	
		
			if(nid != self_id) {
				tfc--;
				if(tfc==0)
					nid = 0;
                        	continue;
			}
                }
                buf[strlen(buf)-1] = '\0';

                ts = (struct TSPEC*)malloc(sizeof(struct TSPEC));
                assert(ts);

		ts->next = NULL;
		ts->Active = INACTIVE;

                /*
                 * The format of each entry is as follows:
                 *   {node id}
                 *   {tspec number}
                 *   {start time} {end time} {protocol} {src port} {dst port} {tsid} {mean data rate}
                 *   {nominal MSDU size} {delay bound} {max SI}
                 */
                ptr = strtok(buf, " ");  // start time
                sscanf(ptr, "%u", &num);
                ts->stime = num;

                ptr = strtok(NULL, " ");  // end time
                sscanf(ptr, "%u", &num);
                ts->etime = num;

                ptr = strtok(NULL, " ");  // downlink/uplink
                if(!strcmp(ptr,"Uplink"))
                        ts->direction = UPLINK;
                else if(!strcmp(ptr,"Downlink"))
                        ts->direction = DOWNLINK;

                ptr = strtok(NULL, " "); // UDP/TCP
                if(!strcmp(ptr,"TCP"))
                        ts->protocol = TCP;
                else
                        ts->protocol = UDP;

                ptr = strtok(NULL, " "); // src port
                sscanf(ptr, "%u", &num);
                ts->sport = num;

                ptr = strtok(NULL, " "); // dst port
                sscanf(ptr, "%u", &num);
                ts->dport = num;

                ptr = strtok(NULL, " "); // tsid
                sscanf(ptr, "%u", &num);
                ts->tsid = num;

                ptr = strtok(NULL, " "); // mean data rate
                sscanf(ptr, "%u", &num);
                ts->mean_data_rate = num;

                ptr = strtok(NULL, " "); // nominal MSDU size
                sscanf(ptr, "%u", &num);

		if(ts->protocol == TCP)
                	ts->nominal_MSDU_size = num + 50 + 40;
		else if(ts->protocol == UDP)
                	ts->nominal_MSDU_size = num + 50 + 28;

                ptr = strtok(NULL, " "); // delaybound
                sscanf(ptr, "%u", &num);
		ts->delay_bound = num;

                ptr = strtok(NULL, " "); // maximum service intreval in ms
                sscanf(ptr, "%u", &num);
                ts->max_SI = num;

		ts->max_MSDU_size = MAX_MSDU_SIZE;

		//if(TShead != NULL)
		ts->next = TShead;
		TShead = ts;

                if((--tfc)==0)
                        nid = 0;
        }
        fclose(fd);
	return; 
}

int mac802_11e::calcul_IFS() {

	aifsn[AC_BE] = 7;
	aifsn[AC_BK] = 3;
	aifsn[AC_VI] = 2;
	aifsn[AC_VO] = 2;	

	sifs = phymib->aSIFSTime;
	pifs = sifs + phymib->aSlotTime;   
  	difs = sifs + 2 * phymib->aSlotTime;
 	eifs = sifs + difs + TX_Time(MAC80211E_ACK_LEN); 
	for(int i = AC_VO ; i >= AC_BE; i--)
		aifs[i] = sifs + aifsn[i] * phymib->aSlotTime;

	return(1); 
}


int mac802_11e::init() {
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
				sprintf(ptrFile,"%s%s", GetConfigFileDir(),ptrlogFileName);
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}
			else {
				ptrFile = (char *)malloc(strlen(GetScriptName())+5);
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
		type = POINTER_TO_MEMBER(mac802_11e, log);
		logTimer.setCallOutObj(this, type);
		logTimer.start(logIntervalTick, logIntervalTick);		
	}

	bw_ = GET_REG_VAR1(get_portls(), "BW", double *);
	ch_ = GET_REG_VAR1(get_portls(), "CHANNEL", int *);
	CPThreshold_ = GET_REG_VAR(get_portls(), "CPThresh", double *);	
	isQAP = GET_REG_VAR(get_port(), "ISQAP", int *);

        /* check if the value of BEACON_TIMEVAL is reasonable, if not
           give it a reasonable value */
	if(*isQAP){
		Beacon_Timeval = GET_REG_VAR1(get_portls(), "BEACON_INTERVAL", int *);
        	if(*Beacon_Timeval == 0)
                	*Beacon_Timeval = 100;
        	else if(*Beacon_Timeval < 10)
                	*Beacon_Timeval = 10;
	}
	/* initialize TSPEC record */
	TShead = NULL;
	init_TSPEC();

	/* initialize 802.11e scheduler */
	if (*isQAP){
		plevel = 0; 	//AC_BE
		qs = new QosScheduler_HC(this);
		action = DATA_FLOW;
	}else
		qs = new QosScheduler_STA(this);

	for(int i = 0; i < AC_NUM; i++) {
                pq_[i]._head = pq_[i]._tail = 0;
                pq_[i]._length = 0;
                pq_[i]._drops = 0;
                pq_[i]._pri = i;
		pq_[i]._size = 0;
                pq_[i]._maxLength = MAX_QUEUE_LEN;
        }
	
	calcul_IFS();
	hcca_stat.hcca_on = 1;

	if(*isQAP)
		start_CAP();
	
	return(1); 
}

int mac802_11e::recv(ePacket_ *pkt) {

	Packet			*p;
	struct wphyInfo		*pinfo, *pinfo1;
	struct hdr_mac802_11e	*mh, *hm;
	u_int64_t		time_period;
	u_int32_t		t,ipdst,ipsrc;

	char			*iph;
	struct mac802_11_log	*log1,*log2;
	struct logEvent		*logep;
	struct ip		*iphdr;
	struct udphdr		*udph;
	struct tcphdr		*tcph;
	enum pro_		proto;
	TSPEC			*ts = NULL;
	
	GET_PKT(p, pkt); 
	assert(p);

       if ((hcca_stat.hcca_tx_active || tx_active) && (p->pkt_err_ == 0)) {

                p->pkt_err_ = 1;

                /* log "StartRX" and "DropRX" event */
                if(_ptrlog == 1){
                        GET_PKT(p, pkt);
                        pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
                        mh = (struct hdr_mac802_11e *)p->pkt_get();
			u_int32_t *txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");
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

	hm  = (struct hdr_mac802_11e *)p->pkt_get();

	/* Decide if this packet is belong to a register traffic stream */
	iphdr = (struct ip *)p->pkt_sget();

	 if( !(*isQAP) && !bcmp(hm->dh_addr1,mac_,ETHER_ADDR_LEN) && 
			(hm->dh_fc.fc_type == MAC_Type_Data) && (iphdr != NULL)) {
		
		if(iphdr->ip_p==IPPROTO_TCP){
			tcph = (struct tcphdr *)((char *)iphdr+sizeof(struct ip));
			proto = TCP;
		}else if(iphdr->ip_p==IPPROTO_UDP){
			udph = (struct udphdr *)((char *)iphdr+sizeof(struct ip));
			proto = UDP;
		}
		ts = decide_TID(proto,tcph,udph, DOWNLINK, hm->dh_addr1);
	
		if(ts!=NULL){	
			if(ts->Active == INACTIVE){
				ts->protocol = proto;
				GenerateADDTSReq(ts);
			}
		}	
	}

	/* count log metric */
	GET_PKT(p, pkt); 
	assert(p);

	if( p->pkt_err_ == 1 ) {
		if( _log && !strcasecmp(_log, "on") ) {
			if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
				mh = (struct hdr_mac802_11e *)p->pkt_get();
				if( mh->dh_fc.fc_type == MAC_Type_Data )
					NumDrop++;
			}
		}
	}

	mh = (struct hdr_mac802_11e *)p->pkt_get();

       	if (rx_state == MAC_IDLE) {
                SET_RX_STATE(MAC_RECV);
                assert(!epktRx);  // just make sure
                epktRx = pkt;

                /*
                 * Schedule a timer to simulate receive time
                 */
                GET_PKT(p, pkt);
                assert(p);
                pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
                t = RX_Time(p->pkt_getlen(), pinfo->bw_);
                start_recvtimer(t);
        }else {
		/*
		 * If the power of the incoming packet is smaller
		 * than the power of the packet currently being 
		 * received by at least the capture threshold,
		 * then we ignore the new packet.
		 */
		GET_PKT(p, pkt); 
	  	pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
		GET_PKT(p, epktRx); 
  		pinfo1 = (struct wphyInfo *)p->pkt_getinfo("WPHY");

		if (pinfo1->RxPr_ / pinfo->RxPr_ >= (double)*CPThreshold_) {

			/* log "StartRX" and "DropRX" event */
			if(_ptrlog == 1){
				GET_PKT(p, pkt);
				pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
				mh = (struct hdr_mac802_11e *)p->pkt_get();
				u_int32_t *txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");


				if(!bcmp(mh->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN) ||
				   !bcmp(mh->dh_addr1, mac_, ETHER_ADDR_LEN)) {

					iph = p->pkt_sget();

					GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

					log1 = (struct mac802_11_log*)malloc
						(sizeof(struct mac802_11_log));
					LOG_802_11(mh,log1,p,GetCurrentTime(),
						GetCurrentTime(),get_type(),get_nid(),
						StartRX,ipsrc,ipdst,*ch_,*txnid);
					INSERT_TO_HEAP(logep,log1->PROTO,
						log1->Time+START,log1);

					log2 = (struct mac802_11_log*)malloc
						(sizeof(struct mac802_11_log));
					MICRO_TO_TICK(time_period,RX_Time(p->pkt_getlen(), pinfo->bw_));
					DROP_LOG_802_11(log1,log2,GetCurrentTime()+
						time_period,DropRX,DROP_CAP);
					INSERT_TO_HEAP(logep,log2->PROTO,
						log2->Time+ENDING,log2);
				}
			}

			/* count log metric */
			if( _log && !strcasecmp(_log, "on") ) {
				if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
					GET_PKT(p, pkt);
					mh = (struct hdr_mac802_11e *)p->pkt_get();
					if( mh->dh_fc.fc_type == MAC_Type_Data )
						NumDrop++;
				}
			}
			capture(pkt);
		}
 		else {
			/* count log metric */
			if( _log && !strcasecmp(_log, "on") ) {
				if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
					GET_PKT(p, pkt);
					mh = (struct hdr_mac802_11e *)p->pkt_get();
					if( mh->dh_fc.fc_type == MAC_Type_Data ) {
						NumColl++;
						if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
							NumDrop++;
					}

					GET_PKT(p, epktRx);
					mh = (struct hdr_mac802_11e *)p->pkt_get();
					if( mh->dh_fc.fc_type == MAC_Type_Data ) {
						 NumColl++;
						if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
							NumDrop++;
					}
				}
			}
			collision(pkt);
		}
	}

	if (hcca_stat.has_control) {
		if (*isQAP) {
	 	/*     The station is a QAP and it has just sent a QoS(+)CF-Poll frame,
    	 	 *     whose 'TXOP limit' field is not 0, and that is not a self poll.
     	 	 *     In this case the control is passed to the QSTA as soon as the QAP
    	 	 *     receive the PHY-CCA.indication(busy).
		 */
      			if ((hccaTx.subtype == MAC_Subtype_QoS_Poll) && (hccaTx.dura != 0) &&
					(bcmp(hccaTx.dst,mac_,ETHER_ADDR_LEN))) 
         			control_medium(false);
		
		} else {
	 	/*     The station is a QSTA and it has just sent a QoS(+)Data frame whose
    	 	 *     duration field is equal to the time required to transmit a QoS CF-Ack
    	 	 *     frame plus SIFS. In this case, if the PHY-CCA.indication(busy) is
    	 	 *     received, then the TXOP ends and the control is passed to the peer.
    	 	 */
			if (hccaTx.dura == TX_Time(MAC80211E_ACK_LEN)+sifs) 
         			control_medium(false);
		}
      	}

     	if(mhHCCATx->busy_)
		mhHCCATx->cancel();
	
	return(1);  
}

/*  
 * put pkt in priority queue, then get dpkt from queue 	
 *  send the oldest pkt in current queue 		
 */
int mac802_11e::send(ePacket_ *pkt) {

	Packet			*p, *pkt_;
 	struct hdr_mac802_11e	*hm; 
	u_char			*dst;
	ePacket_		*dpkt;  
	int			ac = -1;
	int			prev_len = 0,size;
	struct ip		*iph;
	struct udphdr		*udph;
	struct tcphdr		*tcph;
	enum pro_		proto;
	TSPEC			*ts = NULL;

	pkt_ = (Packet *)pkt->DataInfo_;

	GET_PKT(p, pkt); 
	hm = (struct hdr_mac802_11e *)p->pkt_get();
	size = p->pkt_getlen();

	if(hm->dh_fc.fc_type == MAC_Type_Management 
		&& hm->dh_fc.fc_subtype == MAC_Subtype_Beacon){
		near_beacon = 0;
		last_beacon = GetCurrentTime();
	}

	if ( action == NO_ACTION || hm->dh_fc.fc_type == MAC_Type_Management ) {
		/* 
		 * Mangagement type frame should be "AC_VO", according to 802.11e spec. 
	 	 * Management Packets before connection; do not need to put in queue  
		 */		
		if (epktDATA[AC_VO]) 
		{
			// Associating with AP
			if(hm->dh_fc.fc_subtype==MAC_Subtype_Action){
				IF_ENQUEUE(&pq_[AC_VO], pkt, size);
				return (1);
			}	

			return(-1); /* reject upper module's request */
		}
		epktDATA[AC_VO] = pkt;
		ac = AC_VO;
		goto send;
		
	} else if(hcca_stat.hcca_on){
		/* Decide if this packet is belong to a register traffic stream */
		iph = (struct ip *)p->pkt_sget();
		if(hm->dh_fc.fc_type == MAC_Type_Data && iph!=NULL){
		
			if(iph->ip_p==IPPROTO_TCP){
				tcph = (struct tcphdr *)((char *)iph+sizeof(struct ip));
				proto = TCP;
			}else if(iph->ip_p==IPPROTO_UDP){
				udph = (struct udphdr *)((char *)iph+sizeof(struct ip));
				proto = UDP;
			}

			if(proto == TCP && tcph->th_flags == TH_SYN)
				goto outside;

			if(*isQAP)
				ts = decide_TID(proto,tcph,udph, DOWNLINK, hm->dh_addr1);
			else
				ts = decide_TID(proto,tcph,udph, UPLINK, hm->dh_addr1);
	
			if(ts!=NULL){
				if(ts->Active == INACTIVE){	
					ts->protocol = proto;
					GenerateADDTSReq(ts);
					return (-1);
				}else if(ts->Active == ACTIVE || ts->Active == REQUESTING){
					hm->dh_qos.tid = ts->tsid;
					int ret = qs->pkt_enque(pkt);
					if(ret){
						return (1);
					}else if(proto == UDP){
						freePacket(pkt);
						return (1);
					}else
						return (-1);
				}
			}
		}
	}
outside:
	/* calculate the access category of packets */		
	ac = UPtoAC(plevel);
	
	/* record queue length before IF_ENQUEUE */
	prev_len = pq_[ac]._length;

	IF_ENQUEUE(&pq_[ac], pkt, size);	
		
	/* 
	 * NAV is setting right now, so put pkt in queue then return. 	
	 * edca_txop > 0 means that the station is busy sending a series of packets
   	 */
	assert(pkt);

	if (epktDATA[ac] || epktBuf || edca_txop > GetCurrentTime()) {
		if(prev_len == MAX_QUEUE_LEN)
			freePacket(pkt);
		return (1);
	}
	IF_DEQUEUE(&pq_[ac], dpkt);
	epktDATA[ac] = dpkt;
	GET_PKT(p, dpkt);
	//pq_[ac]._size -= p->pkt_getlen(); 

	hm = (struct hdr_mac802_11e *)p->pkt_get();

send:
	/* Get destination MAC address */
  	dst = hm->dh_addr1;  

	/* Generate IEEE 802.11 Data frmae */
	sendDATA(ac,dst);

	/* Generate IEEE 802.11 RTS frame */
	sendRTS(ac,dst);

 	/*
	 * Sense the medium state, if idle we should wait a AIFS time
	 * before transmitting; otherwise, we should start backoff timer.
	 */
	if(mhBackoff[ac]->busy_ == 0 ) {
		if (is_idle()) {
			/*
			 * Medium is idle, we should start defer
			 * timer. If the defer timer is starting, there
			 * is no need to reset defer timer.
			 */
			if (mhDefer[ac]->busy_ == 0)
				start_defer(ac,aifs[ac]); 
		}
		else {
			/*
			 * Medium is not idle, we should start
			 * backoff timer.
			 */
			start_backoff(ac); 
		}
	}  

	if(prev_len == MAX_QUEUE_LEN){	//important! to solve memory increasing problem
		freePacket(pkt);
		return(1);
	}

	assert(epktDATA[ac]);
  	return(1); 
}


/*------------------------------------------------------------------------*
 * IEEE MAC 802.11e Implementation.
 *------------------------------------------------------------------------*/ 
inline void mac802_11e::set_NAV(u_int32_t _nav) {

	u_int64_t       curTime, new_nav;

	curTime = GetCurrentTime();
	MICRO_TO_TICK(new_nav, _nav);
	new_nav += curTime;

	nav = new_nav;
	if (mhNav->busy_)
		mhNav->cancel();
	mhNav->start((nav-curTime), 0);
}

inline void mac802_11e::start_hccatx(u_int32_t us) {

	u_int64_t		inTicks;	
	
	for(int i=0; i <=AC_VO; i++){
		if(mhBackoff[i]->busy_ && !mhBackoff[i]->paused_)
			mhBackoff[i]->pause();
	}

	if(mhHCCATx->busy_)
		mhHCCATx->cancel();

 	MICRO_TO_TICK(inTicks, us);
 	mhHCCATx->start(inTicks, 0); 
}

inline void mac802_11e::start_defer(int pri,u_int32_t us) {

	u_int64_t		inTicks;	
	
 	MICRO_TO_TICK(inTicks, us);
 	mhDefer[pri]->start(inTicks, 0); 
}

inline void mac802_11e::start_sifs(u_int32_t us) {

	u_int64_t		inTicks;	
	
 	MICRO_TO_TICK(inTicks, us);
 	mhSIFS->start(inTicks, 0); 
}

inline void mac802_11e::start_sendtimer(u_int32_t us) {

	u_int64_t		inTicks;
 	MICRO_TO_TICK(inTicks, us);
	mhSend->start(inTicks, 0);
}


inline void mac802_11e::start_recvtimer(u_int32_t us) {

	u_int64_t		inTicks;
	MICRO_TO_TICK(inTicks, us);
	mhRecv->start(inTicks, 0);
}


inline void mac802_11e::start_IFtimer(u_int32_t us) {

	u_int64_t		inTicks;
 	MICRO_TO_TICK(inTicks, us);
	mhIF->start(inTicks, 0);
}

void mac802_11e::start_backoff(int pri) {

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
	timeout = (Random() % cw[pri]) * phymib->aSlotTime;
	MICRO_TO_TICK(timeInTick, timeout); 
  	mhBackoff[pri]->start(timeInTick, 0);

 	/* 
	 * Sense the medium, if busy, pause 
	 * the timer immediately.
	 */
	if (!is_idle() || (hcca_stat.cap_expire == true) || mhHCCATx->busy_)
		mhBackoff[pri]->pause(); 
}

int mac802_11e::navHandler(Event_ *e)
{
	/*
	 * IEEE 802.11 Spec, /Figure 52
	 * 	- If medium idle, we should defer 
	 *	  difs time.
	 */
	u_int32_t	tick;

	if (is_idle()) {					
		for (int i = AC_VO; i >= AC_BE; i--) {
 			if(mhBackoff[i]->paused_){
				MICRO_TO_TICK(tick,aifs[i]);
				mhBackoff[i]->resume(tick);
			}
	
		}
 	} 

	return 1;
}


int mac802_11e::sendHandler(Event_ *e)
{
	struct hdr_mac802_11e		*hm;
	Packet				*p;
	u_int64_t			now;

	now = GetCurrentTime();

	switch(tx_state) {
		case MAC_RTS:
 			/*
			 * sent a RTS, but did not receive a CTS.
			 */
			//edca_txop = 0;
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

		case MAC_BEACON:
			freePacket(epktDATA[now_ac]);
 			epktDATA[now_ac] = 0;  
			rst_CW(now_ac);
			start_backoff(now_ac);

			hccaTx.reset();
			mhCAP->start(0,0);
			if(pq_[now_ac]._length > 0)
				IF_DEQUEUE(&pq_[now_ac], epktDATA[now_ac]);
			now_ac = -1;
			break;
		
		case MAC_SEND:
 			/*
			 * sent DATA, but did not receive an ACK
			 */
			GET_PKT(p, epktDATA[now_ac]); 
 			hm = (struct hdr_mac802_11e *)p->pkt_get();
  			
			if (!bcmp(hm->dh_addr1, etherbroadcastaddr,
				ETHER_ADDR_LEN))
			{	/*
				 * IEEE 802.11 Spec, section 9.2.7
				 *	- In Broadcast and Multicast,
				 *	  no ACK should be received.
				 */
				freePacket(epktDATA[now_ac]);
 				epktDATA[now_ac] = 0;  
				rst_CW(now_ac);
 				start_backoff(now_ac);	

				if(pq_[now_ac]._length > 0)
					IF_DEQUEUE(&pq_[now_ac], epktDATA[now_ac]);

				now_ac = -1;

			}else{
				retransmitDATA();
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

int mac802_11e::sifsHandler(Event_ *e)
{
	assert( epktDATA[now_ac]|| epktRTS[now_ac] || epktCTRL );

	if (check_pktCTRL(now_ac) == 0)		
		return(1); 

	assert(mhBackoff[now_ac]->busy_==0);  

	if (check_pktRTS(now_ac) == 0)
  		return(1);

	if (check_pktTx(now_ac) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::BE_deferHandler(Event_ *e)
{
	assert( epktDATA[AC_BE] || epktRTS[AC_BE]);

	if (check_pktCTRL(AC_BE) == 0)		
		return(1); 

	assert(mhBackoff[AC_BE]->busy_==0);  

	if (check_pktRTS(AC_BE) == 0)
  		return(1);

	if (check_pktTx(AC_BE) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::BK_deferHandler(Event_ *e)
{
	assert(epktDATA[AC_BK] || epktRTS[AC_BK]);

	if (check_pktCTRL(AC_BK) == 0)		
		return(1); 

	assert(mhBackoff[AC_BK]->busy_==0);     

	if (check_pktRTS(AC_BK) == 0)
  		return(1);

	if (check_pktTx(AC_BK) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::VI_deferHandler(Event_ *e)
{
	assert(epktDATA[AC_VI] || epktRTS[AC_VI]);

	if (check_pktCTRL(AC_VI) == 0)		
		return(1); 

	assert(mhBackoff[AC_VI]->busy_==0);     

	if (check_pktRTS(AC_VI) == 0)
  		return(1);

	if (check_pktTx(AC_VI) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::VO_deferHandler(Event_ *e)
{
	assert(epktCTRL || epktDATA[AC_VO] || epktRTS[AC_VO]);

	if (check_pktCTRL(AC_VO) == 0)		
		return(1); 

	assert(mhBackoff[AC_VO]->busy_==0);    /* assert */

	if (check_pktRTS(AC_VO) == 0)
  		return(1);

	if (check_pktTx(AC_VO) == 0)
  		return(1); 

	return 0;
}


int mac802_11e::BE_backoffHandler(Event_ *e)
{

	if (epktCTRL) {
		assert(mhSend->busy_ || mhDefer[AC_BE]->busy_);
 		return(1);
	}

	if (check_pktRTS(AC_BE) == 0)
  		return(1);

	if (check_pktTx(AC_BE) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::BK_backoffHandler(Event_ *e)
{

	if (epktCTRL) {
		assert(mhSend->busy_ || mhDefer[AC_BK]->busy_);
 		return(1);
	}

	if (check_pktRTS(AC_BK) == 0)
  		return(1);

	if (check_pktTx(AC_BK) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::VI_backoffHandler(Event_ *e)
{

	if (epktCTRL) {
		assert(mhSend->busy_ || mhDefer[AC_VI]->busy_);
 		return(1);
	}

	if (check_pktRTS(AC_VI) == 0)
  		return(1);

	if (check_pktTx(AC_VI) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::VO_backoffHandler(Event_ *e)
{

	if (epktCTRL) {
		assert(mhSend->busy_ || mhDefer[AC_VO]->busy_);
 		return(1);
	}

	if (check_pktRTS(AC_VO) == 0)
  		return(1);

	if (check_pktTx(AC_VO) == 0)
  		return(1); 

	return 0;
}

int mac802_11e::txHandler(Event_ *e)
{
	/* If current polled station has no response,
	 * eAP polls next station after pifs */
	if(hcca_stat.has_control && (*isQAP)) {
		if(rx_state == MAC_IDLE)
			start_hccatx(pifs);
 	}

	if(tx_state == MAC_HCCA_SEND){
  		SET_TX_STATE(MAC_IDLE);
		check_idle();
	}

  	// Set the MAC state to idle and the transmitter to inactive
	tx_active = 0;  
 	hcca_stat.hcca_tx_active = 0;
	return(1); 
}

int mac802_11e::recvHandler(Event_ *e)
{
	struct 		hdr_mac802_11e 	*hm;
	Packet 		*p;
	char 		*iph;
    	u_long 		ipsrc,ipdst;  
 	struct 		logEvent 	*logep;
    	struct 		mac802_11_log 	*log1,*log2;
	struct		wphyInfo	*pinfo;

	assert(epktRx && (rx_state == MAC_RECV || rx_state == MAC_COLL));

	/*
	 * If the interface is in TRANSMIT mode when this packet
	 * "arrives", then I would never have seen it and should
	 * do a silent discard without adjusting the NAV.
	 */
	if (tx_active || hcca_stat.hcca_tx_active) {
		freePacket(epktRx);
 		goto done; 
	}

   	if( rx_state == MAC_COLL )
	{
		if(hcca_stat.has_control) {

			start_hccatx(sifs);

			if(!*isQAP)
         			hccaTx.reset();
		}

		if(now_ac > -1)
 			set_NAV(eifs-difs+aifs[now_ac]);
		else
			set_NAV(eifs);

		freePacket(epktRx);
		epktRx = 0;
 		goto done;
   	} 

	/*
	 * Get packet TSheader and txinfo from wphy
	 */
	GET_PKT(p, epktRx); 
 	hm = (struct hdr_mac802_11e *)p->pkt_get();  
	pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");

	if(!hcca_stat.hcca_on || action!=DATA_FLOW)
		goto edca;

   	/* 
	 * When the station has the control of the medium, if it is detected an error, 
	 * a collision, or it is received a frame not address to this station, then 
	 * a frame may be transmitted after SIFS 
	 */
   	if( hcca_stat.has_control && ( p->pkt_err_ || bcmp(hm->dh_addr1, mac_, ETHER_ADDR_LEN )))
	{
		start_hccatx(sifs);

		if(!*isQAP)
         		hccaTx.reset();
   	} 

   	// Notify the event about the reception of a frame

    edca:
	if (  !bcmp(hm->dh_addr1,mac_,ETHER_ADDR_LEN) && 
			hm->dh_fc.fc_type == MAC_Type_Management ) {

		if( hm->dh_fc.fc_subtype == MAC_Subtype_Asso_Resp 
		  	|| hm->dh_fc.fc_subtype == MAC_Subtype_ReAsso_Resp)
		{
			action = DATA_FLOW;
                	memcpy(ap_mac,hm->dh_addr2, ETHER_ADDR_LEN);
		}

		if( hm->dh_fc.fc_subtype == MAC_Subtype_DisAsso)		
		{
			action = NO_ACTION;
			bzero(ap_mac,ETHER_ADDR_LEN);
		}
	}

	if ( p->pkt_err_ ) {

		freePacket(epktRx);
		epktRx = 0;

		if(now_ac > -1)
 			set_NAV(eifs-difs+aifs[now_ac]);
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

//	if( hm->dh_fc.fc_type == MAC_Type_Data && hm->dh_fc.fc_subtype == MAC_Subtype_QoS_Poll)
//		set_NAV(hm->dh_duration);

 	/*
	 * Address Filtering
	 */
	if (bcmp(hm->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN) &&
	    bcmp(hm->dh_addr1, mac_, ETHER_ADDR_LEN)) {
		if (!promiscuous) {
		   freePacket(epktRx); 
		   goto done; 
		}
	}

	/* log "StartRX" and "SuccessRX" event */
	if(_ptrlog == 1){
		GET_PKT(p, epktRx);
		//struct wphyInfo *pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
		iph = p->pkt_sget();
		u_int32_t *txnid = (u_int32_t *)p->pkt_getinfo("LOG_TX_NID");

		GET_IP_SRC_DST(hm,iph,ipsrc,ipdst);

		log1 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));

		u_int64_t txtime = RX_Time(p->pkt_getlen(), pinfo->bw_);  // assert fail
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
		if(hm->dh_fc.fc_subtype == MAC_Subtype_Action){
			if(*isQAP)			//ADDTS Request	
				ProcessADDTS(epktRx);
			else
				recvADDTSResp(epktRx);	//ADDTS Response
		}
		recvDATA(epktRx);
		break; 

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
		}
		break;  

	case MAC_Type_Data: 
		switch(hm->dh_fc.fc_subtype) {
			
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
			if(hcca_stat.hcca_on){
 
				hcca_stat.need_ack = 1;
				hcca_stat.ack_tid = hm->dh_qos.tid;
				memcpy(hcca_stat.ack_dst, hm->dh_addr2, ETHER_ADDR_LEN);

				start_hccatx(sifs);
				assert(put(epktRx, recvtarget_));
			}else
				recvDATA(epktRx);
			break;

		case MAC_Subtype_Data:
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
	
		case MAC_Subtype_QoS_Poll:
		{
			/* If epktRx is a self poll, it can be discarded after updating NAV */
			if(!*isQAP)
			{
				if(hm->dh_qos.txop_queue != 0)
					control_medium(true);	 
			}

			TSPEC *ts = find_TSPEC(hm->dh_qos.tid, UPLINK);

			if(ts->Active != ACTIVE)
				ts->Active = ACTIVE;

			start_hccatx(sifs);
			qs->set_poll_tsid(hm->dh_qos.tid);   

			freePacket(epktRx);
			break;

		}
		case MAC_Subtype_QoS_Ack:
   			/* 
	 	 	 * When the station receives a QoS(+)CF-Ack frame and it has 
			 * sent a QoS(+)Data frame, the scheduler is notified of the 
			 * successful transmission.
	 	 	 */
   			if( hccaTx.txp && (hccaTx.subtype == MAC_Subtype_QoS_Data)) {
       
	 			GET_PKT(p, hccaTx.txp);
       	 			//int pktlen = p->pkt_getlen() - MAC80211E_HDR_LEN;
        			hm = (struct hdr_mac802_11e *)p->pkt_get();

        			if( _log && !strcasecmp(_log, "on") ) {
                			if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) )
                        			NumUniOut++;
                			if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
                        			NumUniInOut++;
                			if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
                        			OutThrput += (double)p->pkt_getlen() / 1000.0;
                			if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
                        			InOutThrput += (double)p->pkt_getlen() / 1000.0;
        			}

 	 			qs->change_state(HCCA_SUCCESS,epktRx);

				if(hcca_stat.has_control)
					start_hccatx(sifs);

         			hccaTx.reset();
				freePacket(epktRx);
			} else
				recvACK(epktRx);
 			break; 

		case MAC_Subtype_QoS_Null:
		/* It is notified the scheduler of the reception of a QoS(+)Null frame */
			assert(*isQAP);
			freePacket(epktRx);
			start_hccatx(pifs);
			break;

		default: 
			ErrorMesg("Invalid MAC Data Subtype");  
		}
		break;
 
	default:
		ErrorMesg("Invalid MAC Type");   
	}

	done:
 		epktRx = 0;
		rx_resume(); 

	return 1;
}

int mac802_11e::check_pktCTRL(int pri) {

	struct hdr_mac802_11e		*hm; 
	u_int32_t			timeout;
	Packet				*p;

	if (epktCTRL == 0)
   		return(-1);

  	if (tx_state == MAC_CTS || tx_state == MAC_ACK)
    		return(-1);

	GET_PKT(p, epktCTRL); 
 	hm = (struct hdr_mac802_11e *)p->pkt_get();
  	
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
 			timeout = hm->dh_duration + CTS_Time;
 			break;

 		/*
		 * IEEE 802.11 Spec, section 9.2.8
		 * Acknowledgments are sent after an SIFS ,without
		 * regard to the busy/idle state of the medium.
		 */
		case MAC_Subtype_ACK:
			SET_TX_STATE(MAC_ACK);
 			timeout = ACK_Time;
  			break;

		default:
 			ErrorMesg("Invalid MAC Control Type !");
	}

	now_ac = pri;
	transmit(epktCTRL, timeout);
 	return(0); 
}


int mac802_11e::check_pktRTS(int pri) {
        struct hdr_mac802_11e	*hm;
        u_int32_t             	timeout;
        Packet                 	*p;

	assert(mhBackoff[pri]->busy_==0);

	if (epktRTS[pri] == 0)
  		return(-1);

	GET_PKT(p, epktRTS[pri]);
        hm = (struct hdr_mac802_11e *)p->pkt_get();
        switch(hm->dh_fc.fc_subtype) {
                case MAC_Subtype_RTS:
                        if (!is_idle()) {
                                inc_CW(pri);
                                start_backoff(pri);
                                return(0);
                        }

                        SET_TX_STATE(MAC_RTS);
                        timeout = CTSTimeout;
                        break;

                default:
                        ErrorMesg("Invalid MAC Control Type !");
        }

	now_ac = pri;
        transmit(epktRTS[pri], timeout);
 	return(0);
}


int mac802_11e::check_pktTx(int pri) {

	struct hdr_mac802_11e		*hm;
	u_int32_t			timeout;
 	Packet				*p = NULL;
 
	assert(mhBackoff[pri]->busy_ == 0); 

	if (epktDATA[pri] == 0)
  		return(-1); 

	GET_PKT(p, epktDATA[pri]); 
        hm = (struct hdr_mac802_11e *)p->pkt_get();

        switch(hm->dh_fc.fc_subtype){
                case MAC_Subtype_Asso_Req:
                case MAC_Subtype_Asso_Resp:
                case MAC_Subtype_ReAsso_Req:
                case MAC_Subtype_ReAsso_Resp:
                case MAC_Subtype_Probe_Req:
                case MAC_Subtype_Probe_Resp:
                case MAC_Subtype_Beacon:
                case MAC_Subtype_DisAsso:
                case MAC_Subtype_Action:
                case MAC_Subtype_ApInfo:
                case MAC_Subtype_Data:

			if (!is_idle()) {
				sendRTS(pri,hm->dh_addr1);
				inc_CW(pri);
				start_backoff(pri);	
				return(0); 
			}
			SET_TX_STATE(MAC_SEND);

			if (bcmp(hm->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN))
 				timeout = ACKTimeout(p->pkt_getlen(),pri);
 			else {	
				/* broadcasat 
				 * sent a braodcast packet and no ack need. 
				 */
				timeout = TX_Time(p->pkt_getlen()); 
			}
			break;
		default:
			ErrorMesg("Invalid MAC Subtype !");
	}

	/* Associated with AP, In contention period, with higher priority */
	if ( (!*isQAP) && action == DATA_FLOW && edca_txop == 0 && now_ac > 1) {
		u_int64_t time_period;
		MICRO_TO_TICK(time_period,(txopLimit[now_ac] << 5));	
		edca_txop = GetCurrentTime() + time_period;
	}

	now_ac = pri;
	transmit(epktDATA[pri], timeout); 
	return(0); 
}


/* In CFP, QAP uses this function to decide which station to poll */
void mac802_11e::sendRTS(int pri, u_char *dst) {

	int				pktlen; 
	Packet				*p;
	struct rts_frame		*rf;

	assert(epktDATA[pri] && !epktRTS[pri]);
	GET_PKT(p, epktDATA[pri]); 
	pktlen = p->pkt_getlen() - MAC80211E_HDR_LEN; 

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
	assert(epktRTS[pri] == NULL);
	epktRTS[pri] = createEvent();
	p = new Packet; assert(p); 
	rf = (struct rts_frame *)p->pkt_malloc(sizeof(struct rts_frame));  
 	assert(rf);
	ATTACH_PKT(p, epktRTS[pri]); 

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

	rf->rf_duration = RTS_DURATION(pktlen);
 	memcpy(rf->rf_ta, mac_, ETHER_ADDR_LEN); /* source addr. */
  	memcpy(rf->rf_ra, dst, ETHER_ADDR_LEN);  /* destination addr. */
}


void mac802_11e::sendCTS(u_char *dst, u_int16_t d) {

	struct cts_frame		*cf;
	Packet				*p;
 	u_int32_t			nid;
 

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

	nid = get_nid();
	cf->cf_duration = CTS_DURATION(d);
	(void)memcpy((char *)cf->cf_fcs, (char *)&nid, 4);
	(void)memcpy(cf->cf_ra, dst, ETHER_ADDR_LEN); /* destination addr. */ 
}


void mac802_11e::sendACK(u_char *dst) {

	struct ack_frame		*af;
	Packet				*p;
 	u_int32_t			nid;
 
	assert( epktCTRL==0 );

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

	nid = get_nid();
	af->af_duration = 0;
	(void)memcpy((char *)af->af_fcs, (char *)&nid, 4);
	(void)memcpy(af->af_ra, dst, ETHER_ADDR_LEN); /* destination addr. */
}

ePacket_* mac802_11e::sendQoSACK(u_char *dst) {

	ePacket_			*pkt;
	struct hdr_mac802_11e		*af;
	Packet				*p;
 	u_int32_t			nid;

	/* Generate ACK Frame */
	pkt = createEvent();
	p = new Packet; assert(p); 
  	af = (struct hdr_mac802_11e *)p->pkt_malloc(sizeof(struct hdr_mac802_11e)); 
	assert(af);
 	ATTACH_PKT(p, pkt); 

  	af->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	af->dh_fc.fc_type       = MAC_Type_Data;
	af->dh_fc.fc_subtype    = MAC_Subtype_QoS_Ack;
	af->dh_fc.fc_to_ds      = 0;
	af->dh_fc.fc_from_ds    = 0;
	af->dh_fc.fc_more_frag  = 0;
	af->dh_fc.fc_retry      = 0;
	af->dh_fc.fc_pwr_mgt    = 0;
	af->dh_fc.fc_more_data  = 0;
	af->dh_fc.fc_wep        = 0;
	af->dh_fc.fc_order      = 0;

	af->dh_qos.tid = hcca_stat.ack_tid;

	nid = get_nid();
	af->dh_duration = 0;
	(void)memcpy((char *)af->dh_fcs, (char *)&nid, 4);
	(void)memcpy(af->dh_addr1, dst, ETHER_ADDR_LEN); /* destination addr. */
	(void)memcpy(af->dh_addr2, mac_, ETHER_ADDR_LEN); /* destination addr. */

	return pkt;
}


void mac802_11e::sendDATA(int pri, u_char *dst) {

	struct hdr_mac802_11e		*mh;
  	Packet				*p;

	assert(epktDATA[pri]);
	GET_PKT(p, epktDATA[pri]); 
	mh = (struct hdr_mac802_11e *)p->pkt_get();

#if 0
        /* MoblNode and AP modules would set the following values */

        /* Encapsulate IEEE 802.11 header. */
        mh = (struct hdr_mac802_11 *)
              p->pkt_malloc(sizeof(struct hdr_mac802_11));
        assert(mh);

        /* fill control field */
        mh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
        mh->dh_fc.fc_type       = MAC_Type_Data;
        mh->dh_fc.fc_subtype    = MAC_Subtype_Data;
        mh->dh_fc.fc_to_ds      = 0;
        mh->dh_fc.fc_from_ds    = 0;
        mh->dh_fc.fc_more_frag  = 0;
        mh->dh_fc.fc_retry      = 0;
        mh->dh_fc.fc_pwr_mgt    = 0;
        mh->dh_fc.fc_more_data  = 0;
        mh->dh_fc.fc_wep        = 0;
        mh->dh_fc.fc_order      = 0;
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
			mh->dh_duration = DATA_DURATION();
		}

		if(mh->dh_fc.fc_type == MAC_Type_Data && 
				mh->dh_fc.fc_subtype == MAC_Subtype_QoS_Data){

			mh->dh_qos.tid = plevel;
			mh->dh_qos.ack_policy = 0; 
			mh->dh_qos.reserved = 0;
			mh->dh_qos.eosp = 0;
			//rounded up to the nearest 256 octets
			mh->dh_qos.txop_queue = bytes_to_queue(pq_[pri]._size);  
		}

		memcpy(mh->dh_addr1, dst, ETHER_ADDR_LEN);
 		memcpy(mh->dh_addr2, mac_, ETHER_ADDR_LEN); 

	} else {	/* broadcast address */
		mh->dh_duration = 0;
   
		if(mh->dh_fc.fc_type == MAC_Type_Data && 
				mh->dh_fc.fc_subtype == MAC_Subtype_QoS_Data){

			mh->dh_qos.tid = 0;
			mh->dh_qos.ack_policy = 1;
			mh->dh_qos.reserved = 0;
			mh->dh_qos.eosp = 0;
			//rounded up to the nearest 256 octets
			mh->dh_qos.txop_queue = bytes_to_queue(pq_[pri]._size);  
		}

		memcpy(mh->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN);
 		memcpy(mh->dh_addr2, mac_, ETHER_ADDR_LEN); 

	}
}

void mac802_11e::transmit(ePacket_ *pkt, double timeout)
{
	struct hdr_mac802_11e 	*mh = NULL;
	Packet 			*p; 
	Event 			*ep;

   	struct 			mac802_11_log *log1,*log2;
    	struct 			logEvent *logep;
    	char 			*iph;
	u_int64_t 		time_period;
    	u_long 			ipdst,ipsrc;
	u_int32_t txnid = get_nid();

	if(hcca_stat.hcca_tx_active){	//mwhsu
		return;
	}

	if(mhHCCATx->busy_)
		mhHCCATx->cancel();

	tx_active = 1;
	GET_PKT(p, pkt);

 	mh = (struct hdr_mac802_11e *)p->pkt_get();

	if (mh->dh_fc.fc_type == MAC_Type_Management && 
			mh->dh_fc.fc_subtype == MAC_Subtype_Beacon)
		SET_TX_STATE(MAC_BEACON);
	/*
	 * If I'm transmitting without doing CS, such as when
	 * sending an ACK, any incoming packet will be "missed"
	 * and hence, must be discared.
	 */
 	if (rx_state != MAC_IDLE) {
 		mh = (struct hdr_mac802_11e *)p->pkt_get();

 		assert(mh->dh_fc.fc_type == MAC_Type_Control);  
		assert(mh->dh_fc.fc_subtype == MAC_Subtype_ACK);
   		assert(epktRx);

 		/* force packet discard */
  		p->pkt_err_ = 1;  
	}

	/* start send timer */
	start_sendtimer((u_int32_t)timeout); 

	/* start Interface timer */
	timeout = TX_Time(p->pkt_getlen());  
	start_IFtimer((u_int32_t)timeout); 

	/* count log metric : 
	 *
	 * Note:
	 *	Broadcast packets don't expect to receive ACK packets,
	 * 	therefore the NumBroOut metric is counted here.
	 *	Unicast packets expect to receive ACK packets, therefore
	 *	the NumUniOut metric is counted in recvACK(). 
	 */
	if( _log && !strcasecmp(_log, "on") ) {
		mh = (struct hdr_mac802_11e *)p->pkt_get();
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


	/* log "StartTX" and "SuccessTX" event */
	if(_ptrlog == 1){
		if(mh == NULL)
			mh = (struct hdr_mac802_11e *)p->pkt_get();
		p->pkt_addinfo("LOG_TX_NID",(char*)&txnid , sizeof(int));
		iph = p->pkt_sget();

		GET_IP_SRC_DST(mh,iph,ipsrc,ipdst);

		log1 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));
		LOG_802_11(mh,log1,p,GetCurrentTime(),GetCurrentTime(),
			get_type(),get_nid(),StartTX,ipsrc,ipdst,*ch_,txnid);
		INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

		log2 = (struct mac802_11_log*)malloc
			(sizeof(struct mac802_11_log));
		MICRO_TO_TICK(time_period,timeout);
		LOG_802_11(mh,log2,p,GetCurrentTime()+time_period,
			GetCurrentTime(),get_type(),get_nid(),SuccessTX,
			ipsrc,ipdst,*ch_,txnid);
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

//	freePacket(pkt);
	assert(put(ep, sendtarget_)); 
}


void mac802_11e::retransmitRTS() {

	struct rts_frame		*rf;
	Packet				*p;
	ePacket_			*dupP_;

	assert(epktDATA[now_ac] && epktRTS[now_ac]);
 	assert(mhBackoff[now_ac]->busy_ == 0);

   	macmib->aRTSFailureCount ++;
   	ssrc++;	// STA Short Retry Count

   	if (ssrc >= macmib->aShortRetryLimit) {
        /*
         * For mobile node routing protocol.
		 * If packet be dropped due to retry count
		 * >= retry limit, we call an error handler.
		 */
		dupP_ = pkt_copy(epktDATA[now_ac]);  // for dropping message 
       		GET_PKT(p, dupP_);
		p->pkt_seek(MAC80211E_HDR_LEN + sizeof(struct ether_header));

		if (-1 == p->pkt_callout(dupP_)) 
			freePacket(dupP_);

		/*
		 * IEEE 802.11 Spec, section 11.4.4.2.16
		 * IEEE 802.11 Spec, section 9.2.5.3
		 */
		drop(epktRTS[now_ac], NULL);
 		drop(epktDATA[now_ac], NULL);
 		epktRTS[now_ac] = epktDATA[now_ac] = 0;   
		ssrc = 0; 

		rst_CW(now_ac);
		now_ac = -1;
 
	} else {
		GET_PKT(p, epktRTS[now_ac]); 
 		rf = (struct rts_frame *)p->pkt_get();
   		rf->rf_fc.fc_retry = 1;
  
		inc_CW(now_ac); 

		/* IEEE 802.11 Spec, section 9.2.5.7 */
		start_backoff(now_ac); 
	}
}


void mac802_11e::retransmitDATA() {

	struct hdr_mac802_11e	*hm;
	Packet			*p;
	ePacket_		*dupP_;
 	u_int32_t		*rcount;
	u_int16_t		*thresh;
	int			pktlen; 

   	assert(epktDATA[now_ac] && epktRTS[now_ac] == 0);
   
	macmib->aACKFailureCount++;
   	
	GET_PKT(p, epktDATA[now_ac]); 
	hm = (struct hdr_mac802_11e *)p->pkt_get();  
 	pktlen = p->pkt_getlen() - MAC80211E_HDR_LEN; 

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
		dupP_ = pkt_copy(epktDATA[now_ac]); // for dropping message 
        	GET_PKT(p, dupP_);
		p->pkt_seek(MAC80211E_HDR_LEN + sizeof(struct ether_header));
        	if (-1 == p->pkt_callout(dupP_)) {
			freePacket(dupP_);
		}

		macmib->aFailedCount++;
   		drop(epktDATA[now_ac], NULL); 
		epktDATA[now_ac] = 0;
		*rcount = 0;

  		rst_CW(now_ac);
		edca_txop = 0;
		now_ac = -1; 
	}
	else {
		hm->dh_fc.fc_retry = 1;
  		sendRTS(now_ac,hm->dh_addr1);
 		inc_CW(now_ac);

		/* IEEE 802.11 Spec, section 9.2.5.2 */
 		start_backoff(now_ac); 
	}
}


void mac802_11e::recvRTS(ePacket_ *p) {

	struct rts_frame	*rf;
	Packet			*pkt;

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
  
	sendCTS(rf->rf_ta, rf->rf_duration);
 	
	/* stop deffering */
	for(int i=AC_VO; i>=AC_BE; i--)
	{
		if (mhDefer[i]->busy_)
			mhDefer[i]->cancel();
	}
	freePacket(p); 
 	tx_resume();
}


void mac802_11e::recvCTS(ePacket_ *p) {


	if (tx_state != MAC_RTS) {
 		drop(p, NULL);
		p = NULL;
 		return;
 	}

	assert(now_ac > -1);
	assert(epktRTS[now_ac] && epktDATA[now_ac]);
 	freePacket(epktRTS[now_ac]); 
	epktRTS[now_ac] = 0;
   
	mhSend->cancel();
	freePacket(p); 

 	/* 
	 * The successful reception of this CTS packet implies
	 * that our RTS was successful. Hence, we can reset
	 * the Short Retry Count and the CW.
	 */
	ssrc = 0;
  	rst_CW(now_ac);

 	tx_resume(); 
}


void mac802_11e::recvACK(ePacket_ *p) {

	Packet				*pkt;
	struct hdr_mac802_11e		*hm;
 	int				pktlen; 
	u_int64_t			inTicks;
	u_int32_t			ttime;

	if ( tx_state != MAC_SEND ) {
 		drop(p, NULL);
		p = NULL;
 		return;
	}

 	assert(epktDATA[now_ac]);
	GET_PKT(pkt, epktDATA[now_ac]);
	pktlen = pkt->pkt_getlen() - MAC80211E_HDR_LEN;  
	
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
		GET_PKT(pkt, epktDATA[now_ac]);
		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
			OutThrput += (double)pkt->pkt_getlen() / 1000.0;
		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
			InOutThrput += (double)pkt->pkt_getlen() / 1000.0;
	}

   	mhSend->cancel();
 	freePacket(epktDATA[now_ac]);
	epktDATA[now_ac] = 0; 

	/* 
	 * If successful reception of this ACK packet implies
	 * that our DATA transmission was successful. Hence,
	 * we can reset the Short/Long Retry Count and CW.
	 */
        if (pktlen < (int) macmib-> aRTSThreshold)
        	ssrc = 0;
      	else
               	slrc = 0;

	freePacket(p);
	rst_CW(now_ac);

  	/* Backoff before sending again */
 	assert(mhBackoff[now_ac]->busy_ == 0);  

	if(pq_[now_ac]._length > 0) {

		IF_DEQUEUE(&pq_[now_ac], epktDATA[now_ac]);
		assert(epktDATA[now_ac]);
		//pq_[now_ac]._size -= pkt->pkt_getlen();
        	GET_PKT(pkt, epktDATA[now_ac]);


		if (edca_txop > GetCurrentTime()){

			//SET_TX_STATE(MAC_IDLE);
			//check_idle();
	 		ttime = 2*sifs + TX_Time(MAC80211E_ACK_LEN) + TX_Time(pkt->pkt_getlen());
			MICRO_TO_TICK(inTicks, ttime);

			if((edca_txop - GetCurrentTime()) < inTicks) {

				mhNav->start(edca_txop-GetCurrentTime(),0);  //mwhsu
				start_backoff(now_ac);
				edca_txop = 0;
				now_ac = -1;
				tx_resume();
				return;
			}

        		hm = (struct hdr_mac802_11e *)pkt->pkt_get();
			sendDATA(now_ac,hm->dh_addr1);	
			sendRTS(now_ac,hm->dh_addr1);	

			MICRO_TO_TICK(inTicks, sifs);
			start_sifs(sifs);
			return;
		}else{
			edca_txop = 0;
		}
	}

	start_backoff(now_ac);	
	edca_txop = 0;
	now_ac = -1;
	tx_resume();
}

void mac802_11e::recvDATA(ePacket_ *p) {

	struct hdr_mac802_11e	*hm;
	Packet			*pkt;
	int			match; 

	GET_PKT(pkt, p); 
 	hm = (struct hdr_mac802_11e *)pkt->pkt_get();  

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
  				rst_CW(now_ac); 
			} else {
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
			if (epktCTRL) {
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
	    if ((hm->dh_fc.fc_retry==1) && match) {
			drop(p, NULL);
			p = NULL;
			return;
	    } 
	}

	if((hm->dh_fc.fc_type == MAC_Type_Management) && 
			(hm->dh_fc.fc_subtype == MAC_Subtype_Action)) {
		freePacket(p);
		return;
	}

	/* Pass the received packet to upper layer. */
	assert(put(p, recvtarget_)); 
}

int mac802_11e::is_idle() {

	if (rx_state != MAC_IDLE){
 		return(0);
 	}
	if (tx_state != MAC_IDLE){
 		return(0);
	}
 	if (nav > GetCurrentTime()){
		return(0);
	}
 	return(1);  
}


void mac802_11e::tx_resume() {

	u_int32_t		pktlen, dt;
	Packet			*p; 

	assert(mhSend->busy_==0);

	if(hcca_stat.cap_expire)
		goto state_idle;

	if (epktCTRL) {
		/*
		 * need to send a CTS or ACK.
		 */
		assert(mhDefer[AC_VO]->busy_==0);
		start_sifs(sifs); 
		
	} else if (now_ac > -1) {

		assert( mhDefer[now_ac]->busy_==0);
		
		if (epktRTS[now_ac]) { 

			if (mhBackoff[now_ac]->busy_==0)
				start_defer(now_ac,aifs[now_ac]);

		} else if (epktDATA[now_ac]) {
			/*
                 	 * If we transmit MPDU using RTS/CTS, then we should defer
                	 * sifs time before MPDU is sent; contrarily, we should
               		 * defer difs time before MPDU is sent when we transmit
               		 * MPDU without RTS/CTS.
                 	 */	

                	GET_PKT(p, epktDATA[now_ac]);
                	pktlen = p->pkt_getlen() - MAC80211E_HDR_LEN;
                                                                                               
                	if (pktlen > macmib->aRTSThreshold)
                		dt = sifs;
			else 	
                        	dt = aifs[now_ac];
                                                                                               
                	if (mhBackoff[now_ac]->busy_==0)  
                        	start_defer(now_ac,dt);
		} 
	}

state_idle:
	SET_TX_STATE(MAC_IDLE);
	check_idle();

}

void mac802_11e::rx_resume() {

	assert( (epktRx==0) && (mhRecv->busy_==0) );

	SET_RX_STATE(MAC_IDLE);
	check_idle(); 
}


void mac802_11e::collision(ePacket_ *p)
{
	u_int64_t dt, expired, t;
	Packet *pkt, *pkt1, *pkt2; 
    	struct hdr_mac802_11e *mh1,*mh2;
   	struct mac802_11_log *log1,*log2;
    	char *iph;
    	u_long ipdst,ipsrc;
    	struct logEvent *logep;
	u_char LOG_flag;
	struct wphyInfo *pinfo;

	switch(rx_state) {
	case MAC_RECV:
 		SET_RX_STATE(MAC_COLL);
		/* fall through */
	
	case MAC_COLL:
 		assert(epktRx && mhRecv->busy_); 
		
		GET_PKT(pkt, p);
		pinfo = (struct wphyInfo *)pkt->pkt_getinfo("WPHY"); 
		expired = mhRecv->expire() - GetCurrentTime();
 		t = RX_Time(pkt->pkt_getlen(), pinfo->bw_); 
		MICRO_TO_TICK(dt, t);

		if(_ptrlog == 1) {
			LOG_flag = 0;

			GET_PKT(pkt1, epktRx);
			mh1 = (struct hdr_mac802_11e *)pkt1->pkt_get();

			GET_PKT(pkt2, p);
			mh2 = (struct hdr_mac802_11e *)pkt2->pkt_get();		

			if (!bcmp(mh1->dh_addr1, mac_, ETHER_ADDR_LEN) ||
			    !bcmp(mh2->dh_addr1, mac_, ETHER_ADDR_LEN)) {
				LOG_flag = 1;
			}
			else if(!bcmp(mh1->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN) ||
				!bcmp(mh2->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN)) {
					if(pkt1->pkt_err_ != 1 && pkt2->pkt_err_ != 1)
						LOG_flag = 1;
			}				

			/* log "StartRX" and "DropRX" event for the former pkt */
			if( LOG_flag == 1) {
				iph = pkt1->pkt_sget();
				u_int32_t *txnid = (u_int32_t *)pkt1->pkt_getinfo("LOG_TX_NID");
				GET_IP_SRC_DST(mh1,iph,ipsrc,ipdst);

				log1 = (struct mac802_11_log*)malloc
					(sizeof(struct mac802_11_log));
				LOG_802_11(mh1,log1,pkt1,mhRecv->expire()-dt,
					mhRecv->expire()-dt,get_type(),get_nid(),
					StartRX,ipsrc,ipdst,*ch_,*txnid);
				INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

				log2 = (struct mac802_11_log*)malloc
					(sizeof(struct mac802_11_log));
				DROP_LOG_802_11(log1,log2,mhRecv->expire(),
					DropRX,DROP_COLL);
				INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);

				/* log "StartRX" and "DropRX" event for the latter pkt */

				iph = pkt2->pkt_sget();
				txnid = (u_int32_t *)pkt2->pkt_getinfo("LOG_TX_NID");
				GET_IP_SRC_DST(mh2,iph,ipsrc,ipdst);
			
				log1 = (struct mac802_11_log*)malloc
					(sizeof(struct mac802_11_log));
				LOG_802_11(mh2,log1,pkt2,GetCurrentTime(),
					GetCurrentTime(),get_type(),get_nid(),
					StartRX,ipsrc,ipdst,*ch_,*txnid);
				INSERT_TO_HEAP(logep,log1->PROTO,log1->Time+START,log1);

				log2 = (struct mac802_11_log*)malloc
					(sizeof(struct mac802_11_log));
				DROP_LOG_802_11(log1,log2,GetCurrentTime()+dt,
					DropRX,DROP_COLL);
				INSERT_TO_HEAP(logep,log2->PROTO,log2->Time+ENDING,log2);
			}
		}

 		if (dt > expired) {
			mhRecv->cancel();
 			drop(epktRx, NULL);
			epktRx = p;
   			mhRecv->start(dt, 0); 
		}
		else drop(p, NULL); 
		break;

	default:
		;
		//assert(0);
	}
}


void mac802_11e::capture(ePacket_ *p)
{
	u_int32_t		t;
	Packet			*pkt;

	GET_PKT(pkt, p); 
	struct wphyInfo *pinfo = (struct wphyInfo *)pkt->pkt_getinfo("WPHY");
 
	if(now_ac > -1)
 		t = RX_Time(pkt->pkt_getlen(), pinfo->bw_) + eifs - difs + aifs[now_ac];
	else
 		t = RX_Time(pkt->pkt_getlen(), pinfo->bw_) + eifs;
   
	/*
	 * Update the NAV so that this does not screw
	 * up carrier sense.
	 */
	set_NAV(t);
 	freePacket(p); 
}


int mac802_11e::update_dcache(u_char *mac, u_int16_t sc)
{
	int			i, cidx;
	u_int64_t		expire;
 

	/* find matching tuple */
 	for(i=0, cidx=-1 ; i < CACHE_SIZE; i++) {
	    if ((cache[i].timestamp != 0 ) && (sc == cache[i].sequno)&&
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


int mac802_11e::Fragment(ePacket_ *pkt, u_char *dst)
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

void mac802_11e::drop(ePacket_ *p, char *why)
{
	freePacket(p); 
}

int mac802_11e::log()
{
	double		cur_time;
    
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


void mac802_11e::inc_CW(int pri) {

	cw[pri] = (cw[pri] << 1) + 1;

	if(cw[pri] > CWmax[pri])
		cw[pri] = CWmax[pri];
}
                                                                                                                         
void mac802_11e::rst_CW (int pri) {

	cw[pri] = CWmin[pri];
}

void mac802_11e::start_CAP() {

	u_int64_t	ticks;
	u_int64_t 	next_cap = (u_int64_t)qs->next_CAP();

	assert(*isQAP);

	if(next_cap < 0)
		return;

      	/* If a Poll frame has been sent, CAP cannot start before 	*
   	 * the end of current TXOP. Therefore, The CAP timer is set 	*
    	 * to expire at the end of the TXOP in this case 		*/	
	if(hccaTx.subtype == MAC_Subtype_QoS_Poll) {
		MICRO_TO_TICK(ticks,(hccaTx.txop-sifs));
		mhCAP->start(ticks,0);
	} else
		mhCAP->start(next_cap,0);
}

void mac802_11e::capHandler(Event_ *e)
{
	u_int64_t ts;
   	u_int64_t now = GetCurrentTime();

	assert(*isQAP && hcca_stat.hcca_on);

	hcca_stat.cap_expire = true;	
	MICRO_TO_TICK(ts,pifs);
 
  	if(ts < hcca_stat.sense_idle)
     	 	return;

	/*if(mhHCCATx->busy_){
                mhHCCATx->cancel();
		hcca_stat.need_ack = 0;
		hccaTx.reset();
	}*/
 
   	hcca_stat.sense_idle = ts;

	if(rx_state == MAC_IDLE && tx_state == MAC_IDLE) {
      		if((now - hcca_stat.idle_time >= ts)) {
			mhHCCATx->start(0,0);	/* not sure, handle immediately */
      		} else {
         		if(mhHCCATx->expire() > ts-(now-hcca_stat.idle_time))
				mhHCCATx->start(ts-(now-hcca_stat.idle_time),0); 
      		}
	}
}

void mac802_11e::hccaTxHandler(Event_* e)
{
	hdr_mac802_11e *mh;
	Event *ep;
      	ePacket_ *pkt;
	Packet	*p;
      	u_int64_t timeout;

	if (check_pktCTRL(now_ac) == 0)	//mwhsu
		return; 	

   	assert(rx_state == MAC_IDLE);

	if(hcca_stat.need_ack){

		pkt = sendQoSACK(hcca_stat.ack_dst);	
		hcca_stat.need_ack = 0;
		hccaTx.reset();

	}else{
		if(hcca_stat.cap_expire){

			hcca_stat.cap_expire = false;
			hcca_stat.sense_idle = 0;
			control_medium(true);

		}

   		hccaTx.reset();
   		qs->pkt_deque(hccaTx);
	
   		if(hccaTx.getP == false){
         		control_medium(false);
			return;
		}
   		
		if(hccaTx.txp == 0) {		// Poll packet

      			hccaTx.txp = createEvent();
			p = new Packet; assert(p);

			mh = (struct hdr_mac802_11e *)p->pkt_malloc(MAC80211E_HDR_LEN);
               	 	assert(mh);
                	ATTACH_PKT(p,hccaTx.txp);
      			mh->dh_qos.tid = hccaTx.tsid;

   		} else {
			GET_PKT(p,hccaTx.txp); 
			assert(p);
			mh = (struct hdr_mac802_11e *)p->pkt_get();
      			hccaTx.tsid = mh->dh_qos.tid;
   		}

		mh->dh_duration = hccaTx.dura;  
   		mh->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
   		mh->dh_fc.fc_type             = MAC_Type_Data;
   		mh->dh_fc.fc_subtype          = hccaTx.subtype;
   		mh->dh_fc.fc_more_frag  = 0;
   		mh->dh_fc.fc_retry      = 0;
   		mh->dh_fc.fc_pwr_mgt    = 0;
   		mh->dh_fc.fc_more_data  = 0;
   		mh->dh_fc.fc_wep        = 0;
   		mh->dh_fc.fc_order      = 0;

		// QoS Control Field
		if(hccaTx.subtype != MAC_Subtype_Data){
  
 			mh->dh_qos.ack_policy   = hccaTx.ackP;
   			mh->dh_qos.eosp       	= hccaTx.eosp;
   			mh->dh_qos.reserved    	= 0;

   			if(*isQAP)
      				mh->dh_qos.txop_queue = (hccaTx.txop >> 5);
   			else
      				mh->dh_qos.txop_queue = bytes_to_queue(hccaTx.qSize);
		}

		if(hccaTx.subtype == MAC_Subtype_QoS_Null)
			control_medium(false);

   		// If it is a self poll the QAP has no more the control of the medium
   		if((hccaTx.subtype == MAC_Subtype_QoS_Poll) && 
				(!bcmp(hccaTx.dst, ap_mac, ETHER_ADDR_LEN))){
      			control_medium(false);
		}

		// transmitter/receiver address
		memcpy(mh->dh_addr2,mac_,ETHER_ADDR_LEN);
		memcpy(mh->dh_addr1,hccaTx.dst,ETHER_ADDR_LEN);

   		pkt = hccaTx.txp;
	}

      	/* Get the next packet to be transmitted */
      	if(pkt) {
		GET_PKT(p,pkt);
		int len = p->pkt_getlen();
         	tx_active = 1;

         	/* the current transmission is performed by the HCCA function */
         	hcca_stat.hcca_tx_active = 1;
         	timeout = TX_Time(len);
		start_IFtimer(timeout);
		SET_TX_STATE(MAC_HCCA_SEND);   

		if(mhSend->busy_)
			mhSend->cancel();

		ep = pkt_copy(pkt);
		assert(put(ep, sendtarget_));
	}
}

/* Change the medium access state relative to hcca_stat.has_control	*
 * which tells whether the station has or not the control of the medium */
void mac802_11e::control_medium(bool set)
{
      	hcca_stat.has_control = set;
	CHECK_BACKOFF_TIMER();
 
  	if(set) {
      		if(*isQAP && mhCAP->busy_)
			mhCAP->cancel(); 
		else if(!*isQAP)
   			qs->change_state(HCCA_HAS_CONTROL,epktRx);
	
   	} else {
		if(*isQAP)
      			start_CAP();
		else
       			qs->change_state(HCCA_LOST_CONTROL,NULL);
   	}
}


void mac802_11e::check_idle()
{
   	if(rx_state == MAC_IDLE && tx_state == MAC_IDLE) {
		hcca_stat.idle_time = GetCurrentTime();

         	if ((*isQAP) && hcca_stat.cap_expire && !mhHCCATx->busy_) 
			mhHCCATx->start(hcca_stat.sense_idle,0);                           
      	} else {
		if(*isQAP && mhHCCATx->busy_)
			mhHCCATx->cancel();
	}
}

u_int8_t mac802_11e::bytes_to_queue (int bytes)
{
        if ( bytes > 64768 ) return 254;
        if ( bytes < 0 ) return 255;
        if ( bytes > 0 && bytes < 256 ) return 1;   
        return u_int8_t(round (double(bytes) / 256));
}

TSPEC* mac802_11e::decide_TID (int proto, struct tcphdr *tcph, struct udphdr *udph, dir_ direction, u_char* addr)
{
	struct TSPEC		*tmp;
	u_int64_t		now, stime, etime;


	now = GetCurrentTime();
	tmp = TShead;


	while(tmp!=NULL){
		SEC_TO_TICK(stime,tmp->stime);			
		SEC_TO_TICK(etime,tmp->etime);	
		
		if(tmp->protocol == proto)
		{
			if(proto == TCP){
				if((tmp->sport == 0 || tcph->th_sport == htons(tmp->sport)) &&
				   (tmp->dport == 0 || tcph->th_dport == htons(tmp->dport)) &&
				   (tmp->direction == direction)){
					if(*isQAP){
						if(!bcmp((void *)addr, (void *)tmp->qsta, 6))
							return tmp;
					}else if(tmp->Active != DECLINED)	
						return tmp;
				}
			}else{
				if((tmp->sport == 0 || udph->uh_sport == htons(tmp->sport)) &&
				   (tmp->dport == 0 || udph->uh_dport == htons(tmp->dport)) && 
				   (tmp->direction == direction )) {
					if(*isQAP){
						if(!bcmp((char *)addr, tmp->qsta,ETHER_ADDR_LEN))
							return tmp;
					}else if(tmp->Active != DECLINED)
						return tmp;
				}	
			}
		}
		tmp = tmp->next;
	}
	return NULL;
}

void mac802_11e::GenerateADDTSReq(TSPEC *ts)
{
        hdr_mac802_11e_manage          	*mac_hdr;
        Packet                  	*addts_pkt;
        Event_                  	*ep;
        char                    	*s_buf;

	u_char				optional_info_flag;
	bool				WithTCLAS;
	enum TCLAS_FrameClassifierType	TCLASFrameClassifierType;
	bool				WithTCLASProcessing;
	
	unsigned int			framebody_len;
	char				*framebody;

	u_char 				Category;
	u_char 				Action;

	bool 				TSInfo_TrafficType = false;
	u_char 				TSInfo_TSID; // 4-bits 
	bool 				TSInfo_Direction_Bit_1;
	bool 				TSInfo_Direction_Bit_2;
	bool 				TSInfo_AccessPolicy_Bit_1 = 0;		// May not be used now
	bool 				TSInfo_AccessPolicy_Bit_2 = 0;		// May not be used now
	bool 				TSInfo_Aggregation = 0;			// May not be used now
	bool 				TSInfo_APSD = 0;			// May not be used now
	u_char				TSInfo_UserPriority; // 0~7
	bool 				TSInfo_TSInfoAckPolicy_Bit_1 = 0;	// May not be used now
	bool 				TSInfo_TSInfoAckPolicy_Bit_2 = 0;	// May not be used now
	bool 				TSInfo_Schedule = 0;			// May not be used now
	unsigned short 			NominalMSDUSize;
	bool 				NominalMSDUSize_Fixed = 0;		// May not be used now
	unsigned short 			MaximumMSDUSize;
	unsigned int 			MinimumServiceInterval;
	unsigned int 			MaximumServiceInterval;
	unsigned int 			InactivityInterval;
	unsigned int 			SyspensionInterval;
	unsigned int 			ServiceStartTime;
	unsigned int 			MinimumDataRate;
	unsigned int 			MeanDataRate;
	unsigned int 			PeakDataRate;
	unsigned int 			BurstSize;
	unsigned int 			DelayBound;
	unsigned int 			MinimumPHYRate;
	unsigned short 			SurplusBandwidthAllowance_IntegerPart; // 3-bits
	unsigned short 			SurplusBandwidthAllowance_DecimalPart; // 13-bits
	unsigned short 			MediumTime;

	u_char 				UserPriority = 0; // 0~7		// May not be used now
	u_char 				ClassifierMask = 0; 			// May not be used now
	u_char 				Version = 0;				// MAy not be used now
	unsigned short 			SourcePort;
	unsigned short 			DestinationPort;
	unsigned int	 		SourceIPAddressV4;
	unsigned int 			DestinationIPAddressV4;
	u_char 				DSCP = 0; // 6-bits			// May not be used now
	u_char 				Protocol;

        addts_pkt = new Packet;
        addts_pkt->pkt_setflow(PF_SEND);

        mac_hdr = (struct hdr_mac802_11e_manage *)
                addts_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));

        mac_hdr->dh_fc.fc_type = MAC_Type_Management;
        mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Action;
        mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
        mac_hdr->dh_fc.fc_from_ds = BIT_0;
        mac_hdr->dh_fc.fc_to_ds = BIT_1;

        memcpy((char *)mac_hdr->dh_addr1,ap_mac,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
        memcpy((char *)mac_hdr->dh_addr3,(char *)ap_mac,6);

        /*
         * Fill the frame body of ADDTS Request frame.
         */
	WithTCLAS = true;
	TCLASFrameClassifierType = ClassifierType_1_IPv4;
	WithTCLASProcessing = false;	

	optional_info_flag = SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSRequestFrameBody(WithTCLAS, TCLASFrameClassifierType, WithTCLASProcessing);

	framebody_len = Calculate80211eADDTSRequestFrameBodyLength(optional_info_flag);

        s_buf =(char *)addts_pkt->pkt_sattach(framebody_len);
        addts_pkt->pkt_sprepend(s_buf, framebody_len);

        framebody = s_buf;
	Initialize80211eFrameBodyWithZero(framebody, framebody_len);

	Category = 0x01;
	Action = 0x00;
	SetCategoryInto80211eActionFrameBody(framebody, Category);
	SetActionInto80211eActionFrameBody(framebody, Action);


	TSInfo_TSID = ts->tsid; 
	if(ts->direction == UPLINK) {
		TSInfo_Direction_Bit_1 = false;
		TSInfo_Direction_Bit_2 = false;
	}
	else if(ts->direction == DOWNLINK) {
		TSInfo_Direction_Bit_1 = true;
		TSInfo_Direction_Bit_2 = false;
	}
	TSInfo_UserPriority = (u_char)plevel; // 0~7
	NominalMSDUSize = (unsigned short)ts->nominal_MSDU_size;
	MaximumMSDUSize = (unsigned short)ts->max_MSDU_size;
	MaximumServiceInterval = (unsigned int)ts->max_SI;
	MeanDataRate = (unsigned int)ts->mean_data_rate;
	DelayBound = (unsigned int)ts->delay_bound;

	SetTSPECInto80211eADDTSRequestFrameBody(framebody, TSInfo_TrafficType, TSInfo_TSID, TSInfo_Direction_Bit_1, TSInfo_Direction_Bit_2, TSInfo_AccessPolicy_Bit_1, TSInfo_AccessPolicy_Bit_2, TSInfo_Aggregation, TSInfo_APSD, TSInfo_UserPriority, TSInfo_TSInfoAckPolicy_Bit_1, TSInfo_TSInfoAckPolicy_Bit_2, TSInfo_Schedule, &NominalMSDUSize, NominalMSDUSize_Fixed, &MaximumMSDUSize, &MinimumServiceInterval, &MaximumServiceInterval, &InactivityInterval, &SyspensionInterval, &ServiceStartTime, &MinimumDataRate, &MeanDataRate, &PeakDataRate, &BurstSize, &DelayBound, &MinimumPHYRate, &SurplusBandwidthAllowance_IntegerPart, &SurplusBandwidthAllowance_DecimalPart, &MediumTime);

	SourcePort = ts->sport;
	DestinationPort = ts->dport;
	if(ts->protocol == UDP)
		Protocol = IPPROTO_UDP;
	else if(ts->protocol == TCP)
		Protocol = IPPROTO_TCP;

	SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSRequestFrameBody(framebody, optional_info_flag, UserPriority, ClassifierMask, Version, (void *)&SourceIPAddressV4, (void *)&DestinationIPAddressV4, &SourcePort, &DestinationPort, DSCP, Protocol);


        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(mac802_11e,send);
        
	ep = createEvent();
        setObjEvent(ep,GetNodeCurrentTime(get_nid()),
                0,this,type,(void *)addts_pkt);

        SET_MLEVEL_3(ep);

	ts->Active = REQUESTING;

	return;
}


void mac802_11e::ProcessADDTS(ePacket_ *pkt)
{
        hdr_mac802_11e_manage   	*mac_hdr, *mh;
        Packet                  	*p, *resp_pkt;
        Event_                  	*ep;
	int 				result;
        char                    	*s_buf;

	char				*ADDTS_req_framebody;

	char 				*TSPEC;
	char 				*TSInfo;
	u_char 				TSInfo_TSID;
	bool				TSInfo_Direction_Bit_1;
	bool				TSInfo_Direction_Bit_2;
	u_char 				FrameClassifierType;
	char 				*TCLAS;
	unsigned short 			SourcePort;
	unsigned short 			DestinationPort;

	char				*ADDTS_resp_framebody;

	u_char				optional_info_flag;
	bool				WithTCLAS;
	enum TCLAS_FrameClassifierType	TCLASFrameClassifierType;
	bool				WithTCLASProcessing;
	
	unsigned int			framebody_len;
	u_char 				Category;
	u_char 				Action;
	char				*NewTSPEC;
	unsigned short			StatusCode;
		
	u_char 				UserPriority = 0; // 0~7	// Not used currently
	u_char 				ClassifierMask = 0; 		// Not used cuurently
	u_char 				Version = 0;			// Not used currently
	unsigned int 			SourceIPAddressV4;
	unsigned int 			DestinationIPAddressV4;
	u_char 				DSCP = 0; // 6-bits		// Not used currently
	u_char 				Protocol;
	enum pro_			proto;

	GET_PKT(p,pkt);     
	assert(p); 

	mh = (struct hdr_mac802_11e_manage *)p->pkt_get();

	ADDTS_req_framebody = p->pkt_sget();

	TSPEC = GetTSPECFrom80211eADDTSRequestFrameBody(ADDTS_req_framebody);	
	assert(TSPEC);

	TSInfo = GetTSInfoFromTSPEC(TSPEC);
	assert(TSInfo);

	assert(GetTSIDFromTSInfo(TSInfo, &TSInfo_TSID));

	if(GetDirectionBit1FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_1 = true;
	else
		TSInfo_Direction_Bit_1 = false;

	if(GetDirectionBit2FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_2 = true;
	else
		TSInfo_Direction_Bit_2 = false;


	FrameClassifierType = 0xff; // initial value, not equal to 0x00, 0x01 or 0x02
	TCLAS = GetTCLASFrom80211eADDTSRequestFrameBody(ADDTS_req_framebody, &FrameClassifierType);
	assert(TCLAS);
	if(FrameClassifierType == 0x01) {
		assert(GetSourcePortFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &SourcePort));
		assert(GetDestinationPortFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &DestinationPort));
		assert(GetProtocolFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &Protocol));
		if(Protocol == IPPROTO_UDP)		proto = UDP;
		else if(Protocol == IPPROTO_TCP)	proto = TCP;
	}

	result = qs->reg_TSPEC(mh->dh_addr2, TSPEC, proto);

        struct TSPEC *ts = (struct TSPEC*)malloc(sizeof(struct TSPEC));
        assert(ts);
	ts->Active = ACTIVE;
        ts->stime = GetCurrentTime();
        ts->etime = 0;
	ts->protocol = proto;
        ts->sport = SourcePort; 
        ts->dport = DestinationPort; 
        ts->tsid = TSInfo_TSID;
	if(TSInfo_Direction_Bit_1 == false && TSInfo_Direction_Bit_2 == false)
		ts->direction = UPLINK;
	else if(TSInfo_Direction_Bit_1 == true && TSInfo_Direction_Bit_2 == false)
		ts->direction = DOWNLINK;
	memcpy(ts->qsta,mh->dh_addr2,ETHER_ADDR_LEN);

	ts->next = TShead;
	TShead = ts;


	resp_pkt = new Packet;
        resp_pkt->pkt_setflow(PF_SEND);

        mac_hdr = (struct hdr_mac802_11e_manage *)
                resp_pkt->pkt_malloc(sizeof(struct hdr_mac802_11e_manage));

        mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Action;
        mac_hdr->dh_fc.fc_type = MAC_Type_Management;
        mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
        mac_hdr->dh_fc.fc_from_ds = BIT_1;
        mac_hdr->dh_fc.fc_to_ds = BIT_0;

        memcpy((char *)mac_hdr->dh_addr1,(char *)mh->dh_addr2,6);
        memcpy((char *)mac_hdr->dh_addr2,(char *)mac_,6);
        memcpy((char *)mac_hdr->dh_addr3,(char *)mac_,6);

        /*
         * Fill the frame body of ADDTS Response frame.
         */

	WithTCLAS = true;
	TCLASFrameClassifierType = ClassifierType_1_IPv4;
	WithTCLASProcessing = false;	

	optional_info_flag = SpecifyWithOrWithoutSomeOptionalInfoFlagFor80211eADDTSResponseFrameBody(WithTCLAS, TCLASFrameClassifierType, WithTCLASProcessing);

	framebody_len = Calculate80211eADDTSResponseFrameBodyLength(optional_info_flag);

        s_buf =(char *)resp_pkt->pkt_sattach(framebody_len);
        resp_pkt->pkt_sprepend(s_buf, framebody_len);

        ADDTS_resp_framebody = s_buf;
	Initialize80211eFrameBodyWithZero(ADDTS_resp_framebody, framebody_len);

	Category = 0x01;
	Action = 0x01;
	SetCategoryInto80211eActionFrameBody(ADDTS_resp_framebody, Category);
	SetActionInto80211eActionFrameBody(ADDTS_resp_framebody, Action);

	NewTSPEC = GetTSPECFrom80211eADDTSResponseFrameBody(ADDTS_resp_framebody);
	assert(NewTSPEC);
        memcpy((void *)NewTSPEC, (void *)TSPEC, 57); // the length of TSPEC is 57 octets

	StatusCode = (unsigned short)result;	
	SetStatusCodeInto80211eADDTSResponseFrameBody(ADDTS_resp_framebody, &StatusCode);

	SetTCLASwithFrameClassifierType1ForIPv4Into80211eADDTSResponseFrameBody(ADDTS_resp_framebody, optional_info_flag, UserPriority, ClassifierMask, Version, (void *)&SourceIPAddressV4, (void *)&DestinationIPAddressV4, &SourcePort, &DestinationPort, DSCP, Protocol);

        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(mac802_11e,send);
        
	ep = createEvent();
        setObjEvent(ep,GetNodeCurrentTime(get_nid()),
                0,this,type,(void *)resp_pkt);
        SET_MLEVEL_3(ep);

	return;
}

void mac802_11e::recvADDTSResp(ePacket_ *pkt)
{
        hdr_mac802_11e_manage   *mh;
        Packet                  *p;
	int 			result;
	TSPEC			*ts;

	char			*ADDTS_resp_framebody;
	unsigned short		StatusCode;
	char			*TSPEC;
	char			*TSInfo;
	u_char			TSInfo_TSID;
	bool			TSInfo_Direction_Bit_1;
	bool			TSInfo_Direction_Bit_2;
	dir_			Direction;
	u_char 			FrameClassifierType;
	char 			*TCLAS;
	u_char			Protocol;
	enum pro_		proto;

	GET_PKT(p,pkt);     
	assert(p); 

	mh = (struct hdr_mac802_11e_manage *)p->pkt_get();

	ADDTS_resp_framebody = p->pkt_sget();

	/*if(respfram->action == 2){
		ProcessDELTS(pkt);
		return;
	}*/

	GetStatusCodeFrom80211eADDTSResponseFrameBody(ADDTS_resp_framebody, &StatusCode);
	result = (int)StatusCode; 

	TSPEC = GetTSPECFrom80211eADDTSResponseFrameBody(ADDTS_resp_framebody);
	assert(TSPEC);
	TSInfo = GetTSInfoFromTSPEC(TSPEC);
	assert(TSInfo);

	assert(GetTSIDFromTSInfo(TSInfo, &TSInfo_TSID));

	if(GetDirectionBit1FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_1 = true;
	else
		TSInfo_Direction_Bit_1 = false;

	if(GetDirectionBit2FromTSInfo(TSInfo))
		TSInfo_Direction_Bit_2 = true;
	else
		TSInfo_Direction_Bit_2 = false;

	if(TSInfo_Direction_Bit_1 == false && TSInfo_Direction_Bit_2 == false)
		Direction = UPLINK;
	else if(TSInfo_Direction_Bit_1 == true && TSInfo_Direction_Bit_2 == false)
		Direction = DOWNLINK;

	ts = find_TSPEC(TSInfo_TSID, Direction);
	assert(ts);

	if(result && ts->Active != ACTIVE){	// TS already exist
		ts->Active = DECLINED;
		return;
	}else
		ts->Active = ACTIVE;

	FrameClassifierType = 0xff; // initial value, not equal to 0x00, 0x01 or 0x02
	TCLAS = GetTCLASFrom80211eADDTSResponseFrameBody(ADDTS_resp_framebody, &FrameClassifierType);
	assert(TCLAS);
	if(FrameClassifierType == 0x01) {
		assert(GetProtocolFromTCLASwithFrameClassifierType1ForIPv4(TCLAS, &Protocol));
		if(Protocol == IPPROTO_UDP)		proto = UDP;
		else if(Protocol == IPPROTO_TCP)	proto = TCP;
	}
	
	qs->reg_TSPEC(mac_, TSPEC, proto);
}

TSPEC* mac802_11e::find_TSPEC(u_char tsid, dir_ direction)
{
	TSPEC *tmp = TShead;

	while(tmp!=NULL){
		if(tmp->tsid == tsid && tmp->direction == direction)
			return tmp;
		tmp = tmp->next;
	}
	return NULL;
}

/*
void mac802_11e::ProcessDELTS(ePacket_ *pkt){
        hdr_mac802_11e          *mh;
        Packet                  *p;
        DELTSFramBody           *deltsfram;
        TSPEC                   *ts;

        GET_PKT(p,pkt);
        assert(p);

        mh = (struct hdr_mac802_11e *)p->pkt_get();

        deltsfram =  (struct DELTSFramBody *)p->pkt_sget();

        ts = find_TSPEC(deltsfram->ts_info.TSID,
                        deltsfram->ts_info.Direction);

        assert(put(pkt,recvtarget_));

}*/

