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
#include <stdlib.h>

#include <gbind.h>
#include <maptable.h>
#include <mbinder.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include <object.h>
#include <packet.h>
#include <regcom.h>
#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>
#include "hub.h"

extern RegTable                 RegTable_;

MODULE_GENERATOR(hub);

hub::hub(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	_log = 0;

	/* initialize port */
	PortList = NULL;
	num_port = 0;

	/* hub should not buffer any pkt in s_queue or r_queue */
        s_flowctl = DISABLED;
        r_flowctl = DISABLED;

}


hub::~hub() {

}

/* main behaviors of hub are in send function */
int hub::send(ePacket_ *pkt, MBinder *frm) {

        ePacket_ 	*pkt_;
	u_int32_t	i;
	HubPort		*tmpHubPort;


	for ( i = 1, tmpHubPort = PortList;
	      //tmpHubPort != NULL, i <= num_port;
	      tmpHubPort != NULL && i <= num_port;
	      tmpHubPort = tmpHubPort->nextport, i++ ) { 
		if ( tmpHubPort->port && 
		     (frm->myModule() == tmpHubPort->port->bindModule()) ) {
			if ( _log ) {
				/* recv start log */
				rslog(pkt, (u_int32_t)tmpHubPort->portNum);
			}
		}
		else if ( tmpHubPort->port &&
			  (frm->myModule() != tmpHubPort->port->bindModule())) {
			pkt_ = pkt_copy(pkt);
			if ( _log ) {
				/* send start log */
				sslog(pkt_, (u_int32_t)tmpHubPort->portNum);
			}
			put(pkt_, tmpHubPort->port);
		}
	}

	freePacket(pkt);
		
        return 1;
}


int hub::recv(ePacket_ *pkt) {
	return 1;
}


int hub::get(ePacket_ *pkt, MBinder *frm) {

	/* change pkt flow to SEND_FLOW since hub is the top module */
	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	pkt_->pkt_setflow(PF_SEND);

	return send(pkt, frm);
}


int hub::init() {

	if ( WireLogFlag && !strcasecmp(WireLogFlag, "on") ) {
		_log = 1;
	}

	return(1);  
}

int hub::sslog(ePacket_ *pkt, u_int32_t portNum)
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

	/* get log information: logchktbl pointer
	 *     txtime:		in order to count back-check time
	 */
	char		*__frmLogRcd, *__txtime;
	__frmLogRcd = pkt_->pkt_getinfo("LOG");
	__txtime = pkt_->pkt_getinfo("TXTIM");

	/* if the incoming pkt doesn't have "LOG", it means
	 * that sending host doesn't turn log option on. So
	 * it won't have any log check table for the hub to
	 * look up, hence skip the incoming pkt.
	 */
	if ( !__frmLogRcd )
		return 1;

	chkEntry	*frmLogRcd;
	u_int64_t	txtime;
	memcpy(&frmLogRcd, __frmLogRcd, 4);
	memcpy(&txtime, __txtime, 8);

        logEvent *sslogEvent = new logEvent[1], *selogEvent = new logEvent[1];
        sslogEvent->PROTO = PROTO_802_3;
        sslogEvent->Time = GetCurrentTime();
	sslogEvent->Time += START;
	memcpy(selogEvent, sslogEvent, sizeof(logEvent));

        struct mac802_3_log *ss8023log = new struct mac802_3_log[1],
			 *se8023log = new struct mac802_3_log[1];
        ss8023log->PROTO = PROTO_802_3;
        ss8023log->Time = ss8023log->sTime = GetCurrentTime();
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
        ss8023log->PHY_Src = get_nid();
        int     *ConnNodeID;
	//printf("enter....\n");
        ConnNodeID = (int *)get_regvar(get_nid(), portNum, "ConnNodeID");
	assert(ConnNodeID);
	//GET_REG_VAR(portNum, "ConnNodeID", int *);
        ss8023log->PHY_Dst = *ConnNodeID;
        ss8023log->RetryCount = frmLogRcd->rtxcnt;
        ss8023log->FrameID = pkt_->pkt_getpid();
        ss8023log->FrameType = FRAMETYPE_DATA;
        ss8023log->FrameLen = pkt_->pkt_getlen();
        ss8023log->DropReason = DROP_NONE;

	memcpy(se8023log, ss8023log, sizeof(struct mac802_3_log));

        sslogEvent->log = (char *)ss8023log;
        logQ.push(sslogEvent);
	selogEvent->log = (char *)se8023log;
	selogEvent->log2 = frmLogRcd;

	/* create the event that handle back-check */
	ePacket_ *pkt__ = createEvent();
	u_int64_t lookupTime = GetCurrentTime() + txtime + 100;
	setEventTimeStamp(pkt__, lookupTime, 0);

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(hub, selog);
	setEventCallOutObj(pkt__, this, type, selogEvent);
	scheduleInsertEvent(pkt__);

	return 1;
}


int hub::rslog(ePacket_ *pkt, u_int32_t portNum)
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

	/* get log information: logchktbl pointer
	 *     txtime:		in order to count back-check time
	 */
	char		*__frmLogRcd, *__txtime;
	__frmLogRcd = pkt_->pkt_getinfo("LOG");
	__txtime = pkt_->pkt_getinfo("TXTIM");

	/* if the incoming pkt doesn't have "LOG", it means
	 * that sending host doesn't turn log option on. So
	 * it won't have any log check table for the hub to
	 * look up, hence skip the incoming pkt.
	 */
	if ( !__frmLogRcd )
		return 1;

	chkEntry	*frmLogRcd;
	u_int64_t	txtime;
	memcpy(&frmLogRcd, __frmLogRcd, 4);
	memcpy(&txtime, __txtime, 8);

        logEvent *rslogEvent = new logEvent[1], *relogEvent = new logEvent[1];
        rslogEvent->PROTO = PROTO_802_3;
        rslogEvent->Time = GetCurrentTime();
	rslogEvent->Time += START;
	memcpy(relogEvent, rslogEvent, sizeof(logEvent));

        struct mac802_3_log *rs8023log = new struct mac802_3_log[1],
			 *re8023log = new struct mac802_3_log[1];
        rs8023log->PROTO = PROTO_802_3;
        rs8023log->Time = rs8023log->sTime = GetCurrentTime();
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
        int     *ConnNodeID;
        ConnNodeID = (int *)get_regvar(get_nid(), portNum, "ConnNodeID");
	assert(ConnNodeID);
	//GET_REG_VAR(portNum, "ConnNodeID", int *);
        rs8023log->PHY_Src = *ConnNodeID;
        rs8023log->PHY_Dst = get_nid();
        rs8023log->RetryCount = frmLogRcd->rtxcnt;
        rs8023log->FrameID = pkt_->pkt_getpid();
        rs8023log->FrameType = FRAMETYPE_DATA;
        rs8023log->FrameLen = pkt_->pkt_getlen();
        rs8023log->DropReason = DROP_NONE;

	memcpy(re8023log, rs8023log, sizeof(struct mac802_3_log));

        rslogEvent->log = (char *)rs8023log;

        logQ.push(rslogEvent);

	relogEvent->log = (char *)re8023log;
	relogEvent->log2 = frmLogRcd;

	/* create the event that handle back-check */
	ePacket_ *pkt__ = createEvent();
	u_int64_t lookupTime = GetCurrentTime() + txtime + 100;
	setEventTimeStamp(pkt__, lookupTime, 0);

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(hub, relog);
	setEventCallOutObj(pkt__, this, type, relogEvent);
	scheduleInsertEvent(pkt__);

	return 1;
}

int hub::selog(ePacket_ *pkt)
{
	logEvent *selogEvent = (logEvent *)pkt->DataInfo_;
	chkEntry *frmLogRcd = (chkEntry *)selogEvent->log2;
	selogEvent->Time += frmLogRcd->etime - frmLogRcd->stime;
	selogEvent->Time += ENDING;

	struct mac802_3_log *se8023log = (struct mac802_3_log *)selogEvent->log;
	se8023log->Time += frmLogRcd->etime - frmLogRcd->stime;
	se8023log->Event = frmLogRcd->etype;
	se8023log->RetryCount = frmLogRcd->rtxcnt;
	se8023log->DropReason = frmLogRcd->dreason;

	logQ.push(selogEvent);
	free(pkt);

	return 1;
}

int hub::relog(ePacket_ *pkt)
{

	logEvent *relogEvent = (logEvent *)pkt->DataInfo_;
	chkEntry *frmLogRcd = (chkEntry *)relogEvent->log2;
	relogEvent->Time += frmLogRcd->etime - frmLogRcd->stime;
	relogEvent->Time += ENDING;

	struct mac802_3_log *re8023log = (struct mac802_3_log *)relogEvent->log;
	re8023log->Time += frmLogRcd->etime - frmLogRcd->stime;
	if(frmLogRcd->etype == SuccessTX)
		re8023log->Event = SuccessRX;
	else if(frmLogRcd->etype == DropTX)
		re8023log->Event = DropRX;
	re8023log->RetryCount = frmLogRcd->rtxcnt;
	re8023log->DropReason = frmLogRcd->dreason;

	logQ.push(relogEvent);

	free(pkt);

	return 1;
}

  
int hub::command(int argc, const char *argv[]) {

    NslObject                       *obj;

    if (argc != 4) {
        printf("Syntax:  Set variable = value\n\n");
        return(-1);
    }

    bool uncanonical_module_name_flag = 0;

    /* The "." sign is not allowed as part of a module name. */
    if ( strstr(argv[3],".") ) {
        uncanonical_module_name_flag = 1;
    }
        /* The RegTable->lookup_Instance() with two parameter version
        * should use the canonical name of a module. If an illegal
        * module name is found, we should return immediately.
    * A canonical module name should start with
    * a prefix "NODE." As such, for any module names
    * without this prefix, this function simply returns
    * the original string as its output because the
    * name translation process may fail with incorrect
    * input names.
    */
    if (strncasecmp("NODE", argv[3], 4))
        uncanonical_module_name_flag = 1;

    /* A canonical module name should start with
    * a prefix "NODE." As such, for any module names
    * without this prefix, this function simply returns
    * the original string as its output because the
    * name translation process may fail with incorrect
    * input names.
    */

    /* Connectivity */
    if (!uncanonical_module_name_flag) {

        obj = RegTable_.lookup_Instance(get_nid(), const_cast<char*>(argv[3]));
        if (!obj) {
            printf("The instance of %s does not exist.\n\n", argv[3]);
            return(-1);
        }
    }
    /* support port should be added here */
    MBinder		*tmpMBinder;
    HubPort		*tmpHubPort;
    u_int32_t	portNum;

    if (!strncmp(argv[1], "port", 4)) {
        sscanf(argv[1], "port%d", &portNum);

        num_port++;

        tmpMBinder = new MBinder(this);
        assert(tmpMBinder);
        tmpMBinder->bind_to(obj);

        tmpHubPort = (HubPort *)malloc(sizeof(struct HubPort));
        tmpHubPort->portNum = (u_int8_t)portNum;
        tmpHubPort->port = tmpMBinder;
        tmpHubPort->nextport = PortList;

        PortList = tmpHubPort;
    }
    else if (!strcmp(argv[1], "sendtarget"))
        sendtarget_->bind_to(obj);
    else if(!strcmp(argv[1], "recvtarget"))
        recvtarget_->bind_to(obj);
    else {
        printf("hub::command(): Invalid command: %s %s %s %s.\n",argv[0],argv[1],argv[2],argv[3]);
        return(-1);
    }
    return(1);
}
