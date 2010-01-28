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
#include "ns.h"
#include <maptable.h>
#include <gbind.h>
#include <phyInfo.h>
#include <iostream>
#include <gprs/include/bss_message.h>
#include <mbinder.h>

extern RegTable                 RegTable_;

MODULE_GENERATOR(NS);

NS::NS(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : GprsObject(type, id, pl, name)
{
	
	/* bind variable */
	vBind("NS", NS_);
	vBind("mode", &mode_);

	/* initiate log variables */
	bw_ = new double;
	*bw_ = 2000000;
	txState = rxState = false;
	txBuf = rxBuf = NULL;

	MaxVC = 0xffff;
	
	/* register variable */
	REG_VAR("MAC", NS_);

	ptr_log = 0;
}


NS::~NS() {


}


u_int64_t NS::scheduleTime(ePacket_ *pkt, double bw, NSState NSState ) {

	double			time, dt;
	u_int64_t		retime;
	//Packet			*p;
	bss_message* p;

	p = (bss_message *)pkt->DataInfo_;   
	time = ((( p->rlc_option_len + p->llc_option_len + p->sndcp_option_len + p->rlc_header_len + p->mac_header_len + p->llc_header_len + p->sndcp_header_len + p->bssgp_header_len + p->user_data_len  )*8.0 )*(1000000/(bw))) * (1000.0/TICK);
	retime = (u_int64_t)time;
   
	dt = (time - retime) * 10.0;
	if(dt >= 5)	retime++;    

	/* If recv jam pkt(pktlen == 4), we mustn't calculate
	 * random delay since jam pkt doesn't bring random
	 * delay when it is sent.
	 */
	//if ( p->pkt_getlen() == 4 )
	//	return(retime);
	
	/* XXX
	 * We add random delay to pkt txtime because of timing
	 * problem. Without the delay, it won't be fair when
	 * nodes comtend the network bandwidth.
	 */
	int	txRandomDelay = 0;
	//char	*tmp;
	if ( NSState == NS_SEND ) {
		txRandomDelay = random() % (retime / DELAYRATIO);
		//p->pkt_addinfo("RAND", (char *)&txRandomDelay, 4);
	}
	else
	if ( NSState == NS_RECV ) {
		/* take out the random delay when receving */
		//tmp = p->pkt_getinfo("RAND");
		//memcpy(&txRandomDelay, tmp, 4);
	}
	else {
		/* don't wanna use random delay */
	}

	return(retime + txRandomDelay);
}

			
u_int64_t NS::scheduleTime(int bits, double bw) {

	double			time, dt;
	u_int64_t		retime;

	time = ( bits * ( 1000000 / bw ) ) * ( 1000.0 / TICK );
	retime = (u_int64_t)time;

	dt = (time - retime) * 10.0;
	if(dt >= 5)     retime++;

	return(retime);
}


u_int64_t NS::scheduleTime(double microSec) {

	u_int64_t		delayTick;

	MICRO_TO_TICK(delayTick, microSec);
	return delayTick;
}


int NS::recv(ePacket_ *pkt) {
	//Packet			*p;

	//assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	bss_message* bp;
	assert(pkt&&(bp=(bss_message *)pkt->DataInfo_));



	if ( !rxState ) {
		rxState = true;

		u_int64_t rxtime = scheduleTime(pkt, *bw_, NS_RECV);

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(NS, NSRxHandler);
		rxTimer.setCallOutObj(this, type);
		rxTimer.start(rxtime, 0);
		rxBuf = pkt;

		if (ptr_log){
			rslog(pkt);
		}

		return(1);
	}

	return(0);
}


int NS::send(ePacket_ *pkt) {

	if ( !txState ) {
		txState = true;
		u_int64_t txtime = scheduleTime(pkt, *bw_, NS_SEND);

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(NS, NSTxHandler);
		txTimer.setCallOutObj(this, type);
		txTimer.start(txtime, 0);
		txBuf = pkt;
		if (ptr_log){
			bss_message* p = (bss_message *)pkt->DataInfo_;
			bss_message* newp = (bss_message*)malloc(sizeof(bss_message));
			bcopy(p,newp,sizeof(bss_message));

			sslog(pkt);
			txBuf = createEvent();
			txBuf->timeStamp_ = pkt->timeStamp_;
			txBuf->perio_ = pkt->perio_;
			txBuf->priority_ = pkt->priority_;
			txBuf->calloutObj_ = pkt->calloutObj_;
			txBuf->func_ = pkt->func_;
			txBuf->DataInfo_ = newp;
			txBuf->flag = pkt->flag;
		}

		return(GprsObject::send(pkt));
	}
	return(-1);
}

void NS::NSRxHandler() {

	rxState = false;
	/* momodadreasonlo test */
	relog(rxBuf);
	assert(put(rxBuf, recvtarget_));
}

void NS::NSTxHandler() {

	txState = false;
	if (ptr_log){
		bss_message* p = (bss_message *)txBuf->DataInfo_;
		selog(txBuf);
		txBuf->DataInfo_ = p->packet;
		free(txBuf);
		free(p);
	}

	return;
}


int NS::init() {

	//bw_ = GET_REG_VAR(get_port(), "BW", double *);
	ConnNodeID_ = GET_REG_VAR(get_port(), "ConnNodeID", int *);

	if ( WireLogFlag && !strcasecmp(WireLogFlag, "on") ) {

		if ( !ptrlogFileOpenFlag ) {
			ptrlogFileOpenFlag = true;

 			char	*ptrFile;
			if( ptrlogFileName ) {
				ptrFile = (char *)malloc(strlen
					(GetConfigFileDir())+strlen
					(ptrlogFileName)+1);
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
		type = POINTER_TO_MEMBER(NS, logFlush);
		ptrlogTimer.setCallOutObj(this, type); 
		ptrlogTimer.start(execint, execint);
	}


	return(GprsObject::init());  
}

int NS::logFlush()
{
	logTbl->flush();
	return 0;
}

int NS::sslog(ePacket_ *pkt)
{
	/* insert this record to logchktbl */
        //chkEntry *thisRcd = logTbl->insertRecord(pkt);
	logTbl->insertRecord(pkt);

	/* prepare logEvent */
        logEvent *sslogEvent = new logEvent[1];
        sslogEvent->PROTO = PROTO_802_3;
        sslogEvent->Time = GetCurrentTime();

	bss_message* p;

	p = (bss_message *)pkt->DataInfo_;   
	/* prepare mac802_3_log */
        struct mac802_3_log *ss8023log = new struct mac802_3_log[1];
        ss8023log->PROTO = PROTO_802_3;
        ss8023log->Time = ss8023log->sTime = ssTime = GetCurrentTime();
        ss8023log->NodeType = get_type();
        ss8023log->NodeID = get_nid();
        ss8023log->Event = StartTX;
        ss8023log->IP_Src = 0;
        ss8023log->IP_Dst = 0;

	ss8023log->PHY_Src = get_nid();
	
	NslObject* module = sendtarget_->bindModule();
	while (ss8023log->PHY_Src == module->get_nid()) module = module->sendtarget_->bindModule();
	ss8023log->PHY_Dst = module->get_nid();
        
	ss8023log->RetryCount = 0;
	//if (p ->packet) 
        //ss8023log->FrameID = p->packet->pkt_getpid();
	//else 
		ss8023log->FrameID = 0;
        ss8023log->FrameType = FRAMETYPE_DATA;
        ss8023log->FrameLen =  p->rlc_option_len + p->llc_option_len + p->sndcp_option_len + p->rlc_header_len + p->mac_header_len + p->llc_header_len + p->sndcp_header_len + p->bssgp_header_len + p->user_data_len;
        ss8023log->DropReason = DROP_NONE;

        sslogEvent->log = (char *)ss8023log;
        logQ.push(sslogEvent);
	return 0;
}

int NS::selog(ePacket_ *pkt)
{
	/* ending this record in logchktbl */
        //chkEntry *thisRcd = logTbl->endingRecord(pkt, SuccessTX, DROP_NONE, 0);
        logTbl->endingRecord(pkt, SuccessTX, DROP_NONE, 0);

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
        se8023log->Event = SuccessTX;
        se8023log->IP_Src = 0;
        se8023log->IP_Dst = 0;
	se8023log->PHY_Src = get_nid();

	NslObject* module = sendtarget_->bindModule();
	while (se8023log->PHY_Src == module->get_nid()) module = module->sendtarget_->bindModule();
	se8023log->PHY_Dst = module->get_nid();
        
	bss_message* p = (bss_message *)pkt->DataInfo_;   

        se8023log->RetryCount = 0;
	//if (p ->packet) 
        //se8023log->FrameID = p->packet->pkt_getpid();
	//else 
		se8023log->FrameID = 0;
        se8023log->FrameType = FRAMETYPE_DATA;
        se8023log->FrameLen = p->rlc_option_len + p->llc_option_len + p->sndcp_option_len + p->rlc_header_len + p->mac_header_len + p->llc_header_len + p->sndcp_header_len + p->bssgp_header_len + p->user_data_len;
        se8023log->DropReason = DROP_NONE;

        selogEvent->log = (char *)se8023log;
        logQ.push(selogEvent);
	return 0;
}

int NS::rslog(ePacket_ *pkt)
{
        logEvent *rslogEvent = new logEvent[1];
        rslogEvent->PROTO = PROTO_802_3;
        rslogEvent->Time = GetCurrentTime();
	rslogEvent->Time += START;

	bss_message* p = (bss_message *)pkt->DataInfo_;   

        struct mac802_3_log *rs8023log = new struct mac802_3_log[1];
        rs8023log->PROTO = PROTO_802_3;
        rs8023log->Time = rs8023log->sTime = rsTime = GetCurrentTime();
        rs8023log->NodeType = get_type();
        rs8023log->NodeID = get_nid();
        rs8023log->Event = StartRX;
        rs8023log->IP_Src = 0;
        rs8023log->IP_Dst = 0;

		
	NslObject* module = sendtarget_->bindModule();
	while (rs8023log->NodeID == module->get_nid()) module = module->sendtarget_->bindModule();
	rs8023log->PHY_Src = module->get_nid();
	
	rs8023log->PHY_Dst = get_nid();

        rs8023log->RetryCount = 0;
	//if (p ->packet) 
        //rs8023log->FrameID = p->packet->pkt_getpid();
	//else 
		rs8023log->FrameID = 0;
        rs8023log->FrameType = FRAMETYPE_DATA;
        rs8023log->FrameLen = p->rlc_option_len + p->llc_option_len + p->sndcp_option_len + p->rlc_header_len + p->mac_header_len + p->llc_header_len + p->sndcp_header_len + p->bssgp_header_len + p->user_data_len;
        rs8023log->DropReason = DROP_NONE;

        rslogEvent->log = (char *)rs8023log;
        logQ.push(rslogEvent);
	return 0;
}

int NS::relog(ePacket_ *pkt)
{
	bss_message* p = (bss_message *)pkt->DataInfo_;   

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
        re8023log->Event = SuccessRX;
        re8023log->IP_Src = 0;
        re8023log->IP_Dst = 0;

	NslObject* module = sendtarget_->bindModule();
	while (re8023log->NodeID == module->get_nid()) module = module->sendtarget_->bindModule();
	re8023log->PHY_Src = module->get_nid();
	
	re8023log->PHY_Dst = get_nid();

        re8023log->RetryCount = 0;
	//if (p ->packet) 
        //re8023log->FrameID = p->packet->pkt_getpid();
	//else 
		re8023log->FrameID = 0;
        re8023log->FrameType = FRAMETYPE_DATA;
        re8023log->FrameLen = p->rlc_option_len + p->llc_option_len + p->sndcp_option_len + p->rlc_header_len + p->mac_header_len + p->llc_header_len + p->sndcp_header_len + p->bssgp_header_len + p->user_data_len;
        re8023log->DropReason = DROP_NONE;

        relogEvent->log = (char *)re8023log;
        logQ.push(relogEvent);
	return 0;
}
