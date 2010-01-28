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
#include <string.h>
#include <stdlib.h>
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <mac/mac.h>
#include <maptable.h>
#include <gbind.h>
#include <phyInfo.h>
#include <iostream>

extern RegTable                 RegTable_;

MODULE_GENERATOR(mac);

mac::mac(u_int32_t type, u_int32_t id, struct plist *pl, const char *name)
                : NslObject(type, id, pl, name)
{
	
	/* bind variable */
	vBind_mac("mac", mac_);
	vBind("PromisOpt", &promiscuous_bind);
	vBind("mode", &mode_);

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
	bw_ = NULL;
	txState = rxState = false;
	txBuf = rxBuf = NULL;
	ptr_log = 0;

	promiscuous_bind = NULL;
	promiscuous = 0;
	_log = NULL;
	
	NumUniIn = NumUniOut = NumUniInOut = 0;
	NumBroIn = NumBroOut = NumBroInOut = 0;
	NumColl = NumDrop = 0;
	InThrput = OutThrput = InOutThrput = 0.0;

	logInterval = 1;
	LogUniIn = LogUniOut = LogUniInOut = 0;
	LogBroIn = LogBroOut = LogBroInOut = 0;
	LogColl = LogDrop = 0;
	LogInThrput = LogOutThrput = LogInOutThrput = 0;

	UniInLogFileName = UniOutLogFileName = UniInOutLogFileName = NULL;
	BroInLogFileName = BroOutLogFileName = BroInOutLogFileName = NULL;
	CollLogFileName = DropLogFileName = NULL;
	InThrputLogFileName = OutThrputLogFileName = InOutThrputLogFileName = NULL;

	/* register variable */
	REG_VAR("MAC", mac_);
}


mac::~mac() {


}

//u_int64_t mac::scheduleTime(ePacket_ *pkt, double bw, MacState macState = MAC_IDLE) {
u_int64_t mac::scheduleTime(ePacket_ *pkt, double bw, MacState macState) {

	double			time, dt;
	u_int64_t		retime;
	Packet			*p;

	p = (Packet *)pkt->DataInfo_;   
	time = ((p->pkt_getlen()*8.0)*(1000000/(bw))) * (1000.0/TICK);
	retime = (u_int64_t)time;
   
	dt = (time - retime) * 10.0;
	if(dt >= 5)	retime++;    

	/* If recv jam pkt(pktlen == 4), we mustn't calculate
	 * random delay since jam pkt doesn't bring random
	 * delay when it is sent.
	 */
	if ( p->pkt_getlen() == 4 )
		return(retime);
	
	/* XXX
	 * We add random delay to pkt txtime because of timing
	 * problem. Without the delay, it won't be fair when
	 * nodes comtend the network bandwidth.
	 */
	int	txRandomDelay = 0;
	char	*tmp;
	if ( macState == MAC_SEND ) {
		txRandomDelay = random() % (retime / DELAYRATIO);
		p->pkt_addinfo("RAND", (char *)&txRandomDelay, 4);
	}
	else
	if ( macState == MAC_RECV ) {
		/* take out the random delay when receving */
		tmp = p->pkt_getinfo("RAND");
		memcpy(&txRandomDelay, tmp, 4);
	}
	else {
		/* don't wanna use random delay */
	}

	return(retime + txRandomDelay);
}

			
u_int64_t mac::scheduleTime(int bits, double bw) {

	double			time, dt;
	u_int64_t		retime;

	time = ( bits * ( 1000000 / bw ) ) * ( 1000.0 / TICK );
	retime = (u_int64_t)time;

	dt = (time - retime) * 10.0;
	if(dt >= 5)     retime++;

	return(retime);
}


u_int64_t mac::scheduleTime(double microSec) {

	u_int64_t		delayTick;
		
	MICRO_TO_TICK(delayTick, microSec);
	return delayTick;
}


bool mac::pktFilter(ePacket_ *pkt) {

	assert(pkt);
	Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);

	struct ether_header *eh =
		(struct ether_header *)pkt_->pkt_get();

        /* check mac address, if the same or broadcast, receive it;
	 * otherwise, discard
	 */
	if( bcmp(eh->ether_dhost, mac_, 6) &&
	    bcmp(eh->ether_dhost, ETHER_BROADCAST, 6) )
		return true;
	else
		return false;
}


int mac::recv(ePacket_ *pkt) {
	Packet			*p;
	struct phyInfo		*phyinfo;
	
	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	if ( !rxState ) {
		rxState = true;

		phyinfo = (struct phyInfo *)p->pkt_getinfo("PHY");
		assert(phyinfo);
		u_int64_t rxtime = scheduleTime(pkt, phyinfo->TxBW, MAC_RECV);

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(mac, macRxHandler);
		rxTimer.setCallOutObj(this, type);
		rxTimer.start(rxtime, 0);
		rxBuf = pkt;

		if ( ptr_log ) {
			/* recv start log */
			rslog(pkt);
		}

		return(1);
	}

	return(-1);
}
       

int mac::send(ePacket_ *pkt) {

	if ( !txState ) {
		txState = true;
		u_int64_t txtime = scheduleTime(pkt, *bw_, MAC_SEND);
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(mac, macTxHandler);
		txTimer.setCallOutObj(this, type);
		txTimer.start(txtime, 0);

		if (ptr_log || (_log && !strcasecmp(_log, "on")))
			txBuf = pkt_copy(pkt);

		if ( ptr_log ) {
			/* send start log */
			sslog(pkt, 0, txtime);
		}

		return(NslObject::send(pkt));
	}
	return(-1);
}

void mac::macRxHandler() {

	rxState = false;

	if ( CRCerror(rxBuf) ) {
		if ( ptr_log ) {
			/* recv end log with bit error */
			relog(rxBuf, DropRX, DROP_BER);
		}
		
		if( _log && !strcasecmp(_log, "on") ) {
			if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
				NumDrop++;
		}

		freePacket(rxBuf);
	}
	else {

		if ( ptr_log ) {
			/* recv end log */
			relog(rxBuf, SuccessRX, DROP_NONE);
		}

		Packet			*pkt_;
		struct ether_header	*eh;

		if ( pktFilterOpt && !strcasecmp(pktFilterOpt, "off") ) {
			if( _log && !strcasecmp(_log, "on") ) {
				pkt_ = (Packet *)rxBuf->DataInfo_;
				eh = (struct ether_header *)pkt_->pkt_get();

				if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) ) {
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

				if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) )
					InThrput += (double)pkt_->pkt_getlen() / 1000.0;
				if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
					InOutThrput += (double)pkt_->pkt_getlen() / 1000.0;
			}
			
			/* 
			 * hwchu: 2005/06/13
			 *   There is a frame check sequence field (FCS) and an imposed minimum
			 *   frame length in the IEEE 802.3 standard, which we do not consider
			 *   before. As a remedy, we treat both as padding bytes and manually
			 *   adjust the frame length.
 			*/
			Packet *p = (Packet *)rxBuf->DataInfo_;
			int *padding = (int *)p->pkt_getinfo("PADDING");

			if ((padding != NULL) && (*padding != 0)) {
				/* Padding bytes can only be calculated once */
				int pktlen = p->pkt_getlen() - (*padding);

				*padding = 0;
				p->pkt_setlen(pktlen);
				//printf("recv pktlen = %d\n", p->pkt_getlen());
			}
			// end of modification: 2005/06/13

			assert(put(rxBuf, recvtarget_));
		}
		else if ( !pktFilter(rxBuf) ) {
			if( _log && !strcasecmp(_log, "on") ) {
				pkt_ = (Packet *)rxBuf->DataInfo_;
				eh = (struct ether_header *)pkt_->pkt_get();

				if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) ) {
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

				if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) )
					InThrput += (double)pkt_->pkt_getlen() / 1000.0;
				if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
					InOutThrput += (double)pkt_->pkt_getlen() / 1000.0;
			}

			/* 
			 * hwchu: 2005/06/13
			 *   There is a frame check sequence field (FCS) and an imposed minimum
			 *   frame length in the IEEE 802.3 standard, which we do not consider
			 *   before. As a remedy, we treat both as padding bytes and manually
			 *   adjust the frame length.
 			*/
			Packet *p = (Packet *)rxBuf->DataInfo_;
			int *padding = (int *)p->pkt_getinfo("PADDING");

			if ((padding != NULL) && (*padding != 0)) {
				/* Padding bytes can only be calculated once */
				int pktlen = p->pkt_getlen() - (*padding);

				*padding = 0;
				p->pkt_setlen(pktlen);
				//printf("recv pktlen = %d\n", p->pkt_getlen());
			}
			// end of modification: 2005/06/13

			assert(put(rxBuf, recvtarget_));
		}
		else
			freePacket(rxBuf);
	}
}

void mac::macTxHandler() {

	txState = false;

	if ( ptr_log ) {
		/* send end log */
		selog(txBuf, SuccessTX, DROP_NONE, 0);
	}

	if( _log && !strcasecmp(_log, "on") ) {
		Packet *pkt_ = (Packet *)txBuf->DataInfo_;
		struct ether_header *eh = 
			(struct ether_header *)pkt_->pkt_get();

		if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) ) {			
			if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) )
				NumBroOut++;
			if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) )
				NumBroInOut++;
		}
		else {
			if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) )
				NumUniOut++;
			if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
				NumUniInOut++;
		}

		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
			OutThrput += (double)pkt_->pkt_getlen() / 1000.0;
		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
			InOutThrput += (double)pkt_->pkt_getlen() / 1000.0;
	}
	if (ptr_log || (_log && !strcasecmp(_log, "on"))) {
		freePacket(txBuf);
		txBuf = NULL;
	}

	return;
}


int mac::init() {

	bw_ = GET_REG_VAR1(get_portls(), "BW", double*);
	ConnNodeID_ = GET_REG_VAR1(get_portls(), "ConnNodeID", int *);

	if( !promiscuous_bind )
		strcpy(pktFilterOpt, "on");
	else if( !strcasecmp(promiscuous_bind, "on") )
		strcpy(pktFilterOpt, "off");
	else
		strcpy(pktFilterOpt, "on");


	if ( WireLogFlag && !strcasecmp(WireLogFlag, "on") ) {

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
	
			/* triggler the heapHandle event */
			Event_ *heapHandle = createEvent();
			u_int64_t chkInt;
			MILLI_TO_TICK(chkInt, 100);
			chkInt += GetCurrentTime();
			setEventTimeStamp(heapHandle, chkInt, 0);

			/* heapHandle is handled by the function
			 * in logHeap.cc : DequeueLogFromHeap
			 */
			int (*__fun)(Event_ *) = (int (*)(Event_ *))&DequeueLogFromHeap;
			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}

		ptr_log = 1;

		/* every a interval, flush the logchktbl */
		u_int64_t life;
		MILLI_TO_TICK(life,10);
		int execint = life / 6;

		logTbl = new logChkTbl(life);
			
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(mac, logFlush);
		ptrlogTimer.setCallOutObj(this, type); 
		ptrlogTimer.start(execint, execint);
	}

	return(NslObject::init());  
}

int mac::logFlush()
{
	logTbl->flush();

	return 1;
}

int mac::sslog(ePacket_ *pkt, int rtxCnt, u_int64_t txtime)
{
	/* insert this record to logchktbl */
        chkEntry *thisRcd = logTbl->insertRecord(pkt);

        Packet *pkt_;
        GET_PKT(pkt_, pkt);

	/* try to get log information: logchktbl pointer
	 * , if no, insert it. If having, get it for using.
	 */
	pkt_->pkt_addinfo("LOG", (char *)&thisRcd, 4);

	/* the same action with txtime */
	pkt_->pkt_addinfo("TXTIM", (char *)&txtime, 8);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

	/* prepare logEvent */
        logEvent *sslogEvent = new logEvent[1];
        sslogEvent->PROTO = PROTO_802_3;
        sslogEvent->Time = GetCurrentTime();
	sslogEvent->Time += START;

	/* prepare mac802_3_log */
        struct mac802_3_log *ss8023log = new struct mac802_3_log[1];
        ss8023log->PROTO = PROTO_802_3;
        ss8023log->Time = ss8023log->sTime = ssTime = GetCurrentTime();
        ss8023log->NodeType = get_type();
        ss8023log->NodeID = get_nid();
        ss8023log->Event = StartTX;
        if ( __ip ) {
                ss8023log->IP_Src = mtbl_iptonid(ipSrc);
                ss8023log->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                ss8023log->IP_Src = 0;
                ss8023log->IP_Dst = 0;
        }

	struct ether_header	*eh;
	eh = (struct ether_header *)pkt_->pkt_get();
		
	ss8023log->PHY_Src = get_nid();
	
	if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) )
		ss8023log->PHY_Dst = *ConnNodeID_ + PHY_BROADCAST_ID;
	else	    
		ss8023log->PHY_Dst = *ConnNodeID_;
        
	ss8023log->RetryCount = rtxCnt;
        ss8023log->FrameID = pkt_->pkt_getpid();
        ss8023log->FrameType = FRAMETYPE_DATA;
        ss8023log->FrameLen = pkt_->pkt_getlen();
        ss8023log->DropReason = DROP_NONE;

        sslogEvent->log = (char *)ss8023log;
        logQ.push(sslogEvent);
	
	return 1;
}

int mac::selog(ePacket_ *pkt, u_char etype, u_char dreason, int rtxCnt)
{
	/* ending this record in logchktbl */
        //chkEntry *thisRcd = logTbl->endingRecord(pkt, etype, dreason, rtxCnt);
        logTbl->endingRecord(pkt, etype, dreason, rtxCnt);

        Packet *pkt_;
        GET_PKT(pkt_, pkt);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

	/* prepare logEvent */
        logEvent *selogEvent = new logEvent[1];
        selogEvent->PROTO = PROTO_802_3;
        selogEvent->Time = GetCurrentTime();
	selogEvent->Time += ENDING;

	/* prepare mac802_3_log */
        struct mac802_3_log *se8023log = new struct mac802_3_log[1];
        se8023log->PROTO = PROTO_802_3;
	se8023log->sTime = ssTime;
        se8023log->Time = GetCurrentTime();
        se8023log->NodeType = get_type();
        se8023log->NodeID = get_nid();
        se8023log->Event = etype;
        if ( __ip ) {
                se8023log->IP_Src = mtbl_iptonid(ipSrc);
                se8023log->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                se8023log->IP_Src = 0;
                se8023log->IP_Dst = 0;
        }

	struct ether_header	*eh;
	eh = (struct ether_header *)pkt_->pkt_get();
		
	se8023log->PHY_Src = get_nid();
	
	if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) )
		se8023log->PHY_Dst = *ConnNodeID_ + PHY_BROADCAST_ID;
	else	    
		se8023log->PHY_Dst = *ConnNodeID_;
        
        se8023log->RetryCount = rtxCnt;
        se8023log->FrameID = pkt_->pkt_getpid();
        se8023log->FrameType = FRAMETYPE_DATA;
        se8023log->FrameLen = pkt_->pkt_getlen();
        se8023log->DropReason = dreason;

        selogEvent->log = (char *)se8023log;
        logQ.push(selogEvent);
	
	return 1;
}

int mac::rslog(ePacket_ *pkt)
{
        Packet *pkt_;
        GET_PKT(pkt_, pkt);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

        logEvent *rslogEvent = new logEvent[1];
        rslogEvent->PROTO = PROTO_802_3;
        rslogEvent->Time = GetCurrentTime();
	rslogEvent->Time += START;

        struct mac802_3_log *rs8023log = new struct mac802_3_log[1];
        rs8023log->PROTO = PROTO_802_3;
        rs8023log->Time = rs8023log->sTime = rsTime = GetCurrentTime();
        rs8023log->NodeType = get_type();
        rs8023log->NodeID = get_nid();
        rs8023log->Event = StartRX;
        if ( __ip ) {
                rs8023log->IP_Src = mtbl_iptonid(ipSrc);
                rs8023log->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                rs8023log->IP_Src = 0;
                rs8023log->IP_Dst = 0;
        }

	struct ether_header	*eh;
	eh = (struct ether_header *)pkt_->pkt_get();
		
	rs8023log->PHY_Src = *ConnNodeID_;
	
	if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) )
		rs8023log->PHY_Dst = get_nid() + PHY_BROADCAST_ID;
	else	    
		rs8023log->PHY_Dst = get_nid();

        rs8023log->RetryCount = 0;
        rs8023log->FrameID = pkt_->pkt_getpid();
        rs8023log->FrameType = FRAMETYPE_DATA;
        rs8023log->FrameLen = pkt_->pkt_getlen();
        rs8023log->DropReason = DROP_NONE;

        rslogEvent->log = (char *)rs8023log;
        logQ.push(rslogEvent);
	
	return 1;
}

int mac::relog(ePacket_ *pkt, u_char etype, u_char dreason)
{
        Packet *pkt_;
        GET_PKT(pkt_, pkt);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

        logEvent *relogEvent = new logEvent[1];
        relogEvent->PROTO = PROTO_802_3;
        relogEvent->Time = GetCurrentTime();
	relogEvent->Time += ENDING;

        struct mac802_3_log *re8023log = new struct mac802_3_log[1];
        re8023log->PROTO = PROTO_802_3;
	re8023log->sTime = rsTime;
        re8023log->Time = GetCurrentTime();
        re8023log->NodeType = get_type();
        re8023log->NodeID = get_nid();
        re8023log->Event = etype;
        if ( __ip ) {
                re8023log->IP_Src = mtbl_iptonid(ipSrc);
                re8023log->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                re8023log->IP_Src = 0;
                re8023log->IP_Dst = 0;
        }

	struct ether_header	*eh;
	eh = (struct ether_header *)pkt_->pkt_get();
		
	re8023log->PHY_Src = *ConnNodeID_;
	
	if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) )
		re8023log->PHY_Dst = get_nid() + PHY_BROADCAST_ID;
	else	    
		re8023log->PHY_Dst = get_nid();

        re8023log->RetryCount = 0;
        re8023log->FrameID = pkt_->pkt_getpid();
        re8023log->FrameType = FRAMETYPE_DATA;
        re8023log->FrameLen = pkt_->pkt_getlen();
        re8023log->DropReason = dreason;

        relogEvent->log = (char *)re8023log;
        logQ.push(relogEvent);
	
	return 1;
}
 
int mac::Debugger() {

	char		mac[20];

 
	NslObject::Debugger();
	macaddr_to_str(mac_, mac); 
	printf("   mac addr: %s\n", mac); 
	printf("   bw: %10.3f\n", *bw_);  
	return(1);      
}

bool mac::CRCerror(ePacket_ *pkt) {
	Packet		*p;

	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	if( p->pkt_err_ == 1)
		return true;
	else
        	return false;
}

