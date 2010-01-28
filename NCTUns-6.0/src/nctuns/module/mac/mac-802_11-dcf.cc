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



#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <ethernet.h>
#include <sys/types.h>

#include <gbind.h>
#include <mac/mac-802_11-dcf.h>
#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>
#include <packet.h>
#include <phy/wphy.h>
#include <random.h>
#include <wphyInfo.h>


MODULE_GENERATOR(mac802_11dcf); 

u_char etherbroadcastaddr[] =  
        { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff  };


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


mac802_11dcf::mac802_11dcf(u_int32_t type, u_int32_t id, struct plist* pl, const char *name): NslObject(type, id, pl, name)
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
	epktBuf = 0;  epktBEACON = 0;

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

	/* register variable */
	REG_VAR("promiscuous", &promiscuous);
}


mac802_11dcf::~mac802_11dcf()
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
}


int mac802_11dcf::init_PHYMIB()
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

int mac802_11dcf::init_MACMIB() {

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


int mac802_11dcf::init_Timer() {

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
	type = POINTER_TO_MEMBER(mac802_11dcf, navHandler);
	mhNav->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11dcf, sendHandler);
 	mhSend->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11dcf, deferHandler);
	mhDefer->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11dcf, backoffHandler);
   	mhBackoff->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11dcf, txHandler);
 	mhIF->setCallOutObj(this, type);
	type = POINTER_TO_MEMBER(mac802_11dcf, recvHandler);
 	mhRecv->setCallOutObj(this, type);

 	return(1); 
}


int mac802_11dcf::calcul_IFS() {

	sifs = phymib->aSIFSTime;
	pifs = sifs + phymib->aSlotTime;   
  	difs = sifs + 2 * phymib->aSlotTime;
 	eifs = sifs + difs + TX_Time(MAC80211_ACK_LEN); 
 
	return(1); 
}


int mac802_11dcf::init() {

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
		type = POINTER_TO_MEMBER(mac802_11dcf, log);
		logTimer.setCallOutObj(this, type);
		logTimer.start(logIntervalTick, logIntervalTick);		
	}

	bw_ = GET_REG_VAR1(get_portls(), "BW", double *);
	ch_ = GET_REG_VAR1(get_portls(), "CHANNEL", int *);
	CPThreshold_ = GET_REG_VAR(get_port(), "CPThresh", double *);	

	calcul_IFS();
	return(1); 

}



int mac802_11dcf::recv(ePacket_ *pkt) {

	Packet			*p;
	struct wphyInfo		*pinfo, *pinfo1;
	u_int32_t		t; 

	struct hdr_mac802_11	*mh;
	struct mac802_11_log	*log1,*log2;
	char			*iph;
	u_long			ipdst,ipsrc;
	struct logEvent		*logep;
	u_int64_t		time_period;

	GET_PKT(p, pkt); 
	assert(p);
	mh = (struct hdr_mac802_11 *)p->pkt_get();
	if (tx_active && p->pkt_err_ == 0) {
  		p->pkt_err_ = 1;
	
		/* log "StartRX" and "DropRX" event */
		if(_ptrlog == 1){
			GET_PKT(p, pkt);
			pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
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
	}
	else {
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
				mh = (struct hdr_mac802_11 *)p->pkt_get();
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
					mh = (struct hdr_mac802_11 *)p->pkt_get();
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
					mh = (struct hdr_mac802_11 *)p->pkt_get();
					if( mh->dh_fc.fc_type == MAC_Type_Data ) {
						NumColl++;
						if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
							NumDrop++;
					}

					GET_PKT(p, epktRx);
					mh = (struct hdr_mac802_11 *)p->pkt_get();
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
	return(1);  
}


int mac802_11dcf::send(ePacket_ *pkt) {

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
		epktBEACON = pkt;	/* hold this packet into send Beacon buffer */
	}
	else
		epktTx = pkt;	/* hold this packet into send buffer */

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



/*------------------------------------------------------------------------*
 * IEEE MAC 802.11 Implementation.
 *------------------------------------------------------------------------*/ 
inline void mac802_11dcf::set_NAV(u_int32_t _nav) {

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


inline void mac802_11dcf::start_defer(u_int32_t us) {

	u_int64_t		inTicks;

 	MICRO_TO_TICK(inTicks, us);
 	mhDefer->start(inTicks, 0); 
}


inline void mac802_11dcf::start_sendtimer(u_int32_t us) {

	u_int64_t		inTicks;

 	MICRO_TO_TICK(inTicks, us);
	mhSend->start(inTicks, 0);
}


inline void mac802_11dcf::start_recvtimer(u_int32_t us) {

	u_int64_t		inTicks;
 
	MICRO_TO_TICK(inTicks, us);
	mhRecv->start(inTicks, 0);
}


inline void mac802_11dcf::start_IFtimer(u_int32_t us) {

	u_int64_t		inTicks;

 	MICRO_TO_TICK(inTicks, us);
	mhIF->start(inTicks, 0);
}


void mac802_11dcf::start_backoff(struct frame_control* dh_fc) {

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


int mac802_11dcf::navHandler(Event_ *e)
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


int mac802_11dcf::sendHandler(Event_ *e)
{
	struct hdr_mac802_11		*hm;
	Packet				*p;
	struct frame_control    *dh_fc;
 
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


int mac802_11dcf::deferHandler(Event_ *e)
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


int mac802_11dcf::backoffHandler(Event_ *e)
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


int mac802_11dcf::txHandler(Event_ *e)
{
	tx_active = 0;  

	return(1); 
}

int mac802_11dcf::recvHandler(Event_ *e)
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
	if (tx_active) {
		freePacket(epktRx);
 		goto done; 
	}

	/*
	 * handle collisions.
	 */
	if (rx_state == MAC_COLL) {
		drop(epktRx, NULL);
		epktRx = NULL;
 		set_NAV(eifs);
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
		struct wphyInfo *pinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
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
			ErrorMesg("Invalid MAC Control Subtype !");   
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
		}
		break;
 
	default: ErrorMesg("Invalid MAC Type");   
	}

	done:
	
 	epktRx = 0;
  	rx_resume(); 

	return 1;
}

int mac802_11dcf::check_pktCTRL() {

	struct hdr_mac802_11		*hm; 
	u_int32_t			timeout;
	Packet				*p;
 
	
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
	transmit(epktCTRL, timeout);
 	return(0); 
}


int mac802_11dcf::check_pktRTS() {

	struct hdr_mac802_11		*hm;
	struct frame_control		*dh_fc;
	u_int32_t			timeout; 
	Packet				*p;
 

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
 			timeout = CTSTimeout;  
			break;

 		default:
 			ErrorMesg("Invalid MAC Control Type !"); 
	}
	transmit(epktRTS, timeout);
 	return(0);
}

int mac802_11dcf::check_pktBEACON()
{

	struct hdr_mac802_11            *hm;
        struct frame_control            *dh_fc;
        u_int32_t                       timeout;
        Packet                          *p;

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
		timeout = DATA_Time(p->pkt_getlen());
                break;
        default:
                        ErrorMesg("Invalid MAC Management BEACON Subtype !");
        }
        transmit(epktBEACON, timeout);

        return 0;
}

int mac802_11dcf::check_pktTx() {

	struct hdr_mac802_11		*hm;
	u_int32_t			timeout;
 	Packet				*p;
	struct frame_control		*dh_fc;
 
 
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
 				timeout = ACKTimeout(p->pkt_getlen());
 			else {	
				/* XXX: broadcasat 
				 * sent a braodcast packet and no
				 * ack need.  ???????????????????
				 */
				timeout = DATA_Time(p->pkt_getlen()); 
			}
			break; 

 		default:
 			ErrorMesg("Invalid MAC Control Subtype !"); 
	}
	transmit(epktTx, timeout); 
	return(0); 
}


void mac802_11dcf::sendRTS(u_char *dst) {

	int				pktlen; 
	Packet				*p;
	struct rts_frame		*rf;
 
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

	rf->rf_duration = RTS_DURATION(pktlen);
 	memcpy(rf->rf_ta, mac_, ETHER_ADDR_LEN); /* source addr. */
  	memcpy(rf->rf_ra, dst, ETHER_ADDR_LEN);  /* destination addr. */
}


void mac802_11dcf::sendCTS(u_char *dst, u_int16_t d) {

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


void mac802_11dcf::sendACK(u_char *dst) {

	struct ack_frame		*af;
	Packet				*p;
 	u_int32_t			nid;
 

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

	nid = get_nid();
	af->af_duration = ACK_DURATION();
	(void)memcpy((char *)af->af_fcs, (char *)&nid, 4);
	(void)memcpy(af->af_ra, dst, ETHER_ADDR_LEN); /* destination addr. */
}


void mac802_11dcf::sendDATA(u_char *dst) {

	struct hdr_mac802_11		*mh;
  	Packet				*p;

 
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
		mh->dh_duration = DATA_DURATION(); 
		memcpy(mh->dh_addr1, dst, ETHER_ADDR_LEN);
 		memcpy(mh->dh_addr2, mac_, ETHER_ADDR_LEN); 
	}
	else {	/* broadcast address */
		mh->dh_duration = 0;   
		memcpy(mh->dh_addr1, etherbroadcastaddr, ETHER_ADDR_LEN);
 		memcpy(mh->dh_addr2, mac_, ETHER_ADDR_LEN); 
	}
}


void mac802_11dcf::transmit(ePacket_ *pkt, u_int32_t timeout)
{
	struct hdr_mac802_11 *mh=NULL;
	Packet *p;
	Event *ep; 
    struct mac802_11_log *log1,*log2;
    char *iph;
    u_long ipdst,ipsrc;
    struct logEvent *logep;
	u_int64_t time_period;
	u_int32_t txnid = get_nid();

	tx_active = 1;
	GET_PKT(p, pkt);
  
	/*
	 * If I'm transmitting without doing CS, such as when
	 * sending an ACK, any incoming packet will be "missed"
	 * and hence, must be discared.
	 */
 	if (rx_state != MAC_IDLE) {
 		mh = (struct hdr_mac802_11 *)p->pkt_get();

  		assert(mh->dh_fc.fc_type == MAC_Type_Control);
		assert(mh->dh_fc.fc_subtype == MAC_Subtype_ACK);
   		assert(epktRx);

 		/* force packet discard */
		Packet *RxPkt;
		GET_PKT(RxPkt, epktRx);
  		RxPkt->pkt_err_ = 1;  
	}

	/* start send timer */
	start_sendtimer(timeout); 

	/* start Interface timer */
	timeout = TX_Time(p->pkt_getlen());  
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


	/* log "StartTX" and "SuccessTX" event */
	if(_ptrlog == 1){
		if(mh == NULL)
			mh = (struct hdr_mac802_11 *)p->pkt_get();

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


void mac802_11dcf::retransmitRTS() {

	struct rts_frame		*rf;
	Packet				*p;
	ePacket_			*dupP_;
	struct frame_control		*dh_fc;

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
//			printf("mac802_11dcf::retransmitRTS: pkt_callout failed\n");
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


void mac802_11dcf::retransmitDATA() {

	struct hdr_mac802_11	*hm;
	Packet			*p;
	ePacket_		*dupP_;
	int			pktlen; 
 	u_int16_t		*rcount, *thresh;
	struct frame_control	*dh_fc;

 
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
//			printf("mac802_11dcf::retransmitDATA, pkt_callout failed\n");
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


void mac802_11dcf::recvRTS(ePacket_ *p) {

	struct rts_frame		*rf;
	Packet				*pkt;


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
	if (mhDefer->busy_)
		mhDefer->cancel();

	freePacket(p); 
 	tx_resume();
}


void mac802_11dcf::recvCTS(ePacket_ *p) {

	if (tx_state != MAC_RTS) {
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


void mac802_11dcf::recvACK(ePacket_ *p) {

	Packet				*pkt;
 	int				pktlen; 
	struct ack_frame *af;
	struct frame_control	*dh_fc;
	
	
 	GET_PKT(pkt,p);
        af = (struct ack_frame*)pkt->pkt_get();
	dh_fc = &(af->af_fc);
	if (tx_state != MAC_SEND) {
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


void mac802_11dcf::recvDATA(ePacket_ *p) {

	struct hdr_mac802_11		*hm;
	Packet				*pkt;
	int				match; 


	GET_PKT(pkt, p); 
 	hm = (struct hdr_mac802_11 *)pkt->pkt_get();  


	/* MoblNode module and AP module would decapsulate the 802.11 header */
	/* decapsulate 802.11 header */
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
	    if ((hm->dh_fc.fc_retry==1)&&match) {
			drop(p, NULL);
			p = NULL;
			return;
	    } 
	}

	/* Pass the received packet to upper layer. */
	assert(put(p, recvtarget_)); 
}


int mac802_11dcf::is_idle() {

	if (rx_state != MAC_IDLE)
 		return(0);

 	if (tx_state != MAC_IDLE)
 		return(0);

 	if (nav > GetCurrentTime())
		return(0);
 	return(1);  
}


void mac802_11dcf::tx_resume() {

	u_int32_t		pktlen, dt;
	Packet			*p; 

 
	//assert((mhSend->busy_==0)&&(mhDefer->busy_==0));
	assert(mhSend->busy_ == 0);
	
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


void mac802_11dcf::rx_resume() {

	assert((epktRx==0)&&(mhRecv->busy_==0));
	SET_RX_STATE(MAC_IDLE); 
}


void mac802_11dcf::collision(ePacket_ *p)
{
	u_int64_t dt, expired, t;
	Packet *pkt, *pkt1, *pkt2; 
    struct hdr_mac802_11 *mh1,*mh2;
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
			mh1 = (struct hdr_mac802_11 *)pkt1->pkt_get();

			GET_PKT(pkt2, p);
			mh2 = (struct hdr_mac802_11 *)pkt2->pkt_get();		

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
		assert(0); 
	}
}


void mac802_11dcf::capture(ePacket_ *p)
{
	u_int32_t		t;
	Packet			*pkt;

	GET_PKT(pkt, p); 
	struct wphyInfo *pinfo = (struct wphyInfo *)pkt->pkt_getinfo("WPHY"); 
 	t = RX_Time(pkt->pkt_getlen(), pinfo->bw_) + eifs;
   
	/*
	 * Update the NAV so that this does not screw
	 * up carrier sense.
	 */
	set_NAV(t);

 	freePacket(p); 
}


int mac802_11dcf::update_dcache(u_char *mac, u_int16_t sc)
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


int mac802_11dcf::Fragment(ePacket_ *pkt, u_char *dst)
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

void mac802_11dcf::drop(ePacket_ *p, char *why)
{
	freePacket(p); 
}

int mac802_11dcf::log()
{
	double		cur_time;
//printf("mac802_11dcf::log %d\n",get_nid());
    
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

