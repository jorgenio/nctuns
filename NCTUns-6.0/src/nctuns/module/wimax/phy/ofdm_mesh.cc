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
#include <vector>
#include <nctuns_api.h>
#include <con_list.h>
#include <nodetype.h>
#include "ofdm_mesh.h"

#include <misc/log/logHeap.h>
#include <misc/log/logmacro.h>

#define VERBOSE_LEVEL MSG_INFO
#include "../mac/verbose.h"

MODULE_GENERATOR(OFDM_Mesh);

extern SLIST_HEAD(wimax_head, con_list) headOfWiMAX_;
extern typeTable*                       typeTable_;

u_char _ptrlog = 0;

int OFDM_Mesh::get_start_to_function_timepoint() {

    u_int64_t* tp = GET_REG_VAR(get_port(), "START_TIME", u_int64_t *);
    start_to_function_time = (*tp);
    
    NSLOBJ_INFO("Receive MAC Layer Command: start to function at %lf Sec.\n",
        (double)(start_to_function_time/10000000) );

    return 1;
}

OFDM_Mesh::OFDM_Mesh(u_int32_t type, u_int32_t id, struct plist *pl, const char *name)
: OFDM_80216(type, id, pl, name)
{
    start_to_function_time = 0xffffffffffffffffllu;
    timerInit              = new timerObj();
    
    BASE_OBJTYPE(obj);
    obj = POINTER_TO_MEMBER(OFDM_Mesh, get_start_to_function_timepoint );
    timerInit->setCallOutObj(this, obj);

    
    
    frameDuration = 10.0;	// 10 ms
    CPratio = 1.0 / 4.0;	// Tg/Tb
    Tb = 11.11;		// us
    Tg = Tb * CPratio;	// us
    Ts = Tg + Tb;		// us
    symbolsPerFrame = (int) (frameDuration * 1000 / Ts);
    transPower      = 37.0;	// dBm
    recvState       = Idle;
    ptr_log_flag    = 1; /* by default, it is turned on. */

#if 0
    printf("\tframeDuration=%lf, symbolsPerFrame=%d\n", frameDuration,
        symbolsPerFrame);
#endif
}

OFDM_Mesh::~OFDM_Mesh()
{
	if (timerInit)
		delete timerInit;
}

int OFDM_Mesh::init(void)
{
//	printf("%d OFDM_Mesh::init()\n", get_nid());
    /*_RecvSensitivity = -130;*//* jclin: temporary value for testing. This value will be
    * disabled if $CASE.tcl has properly specified this field.
    */
    u_int64_t init_tick = 1;
    timerInit->start(init_tick,0);


    /* Mac 802.16 Mesh mode log */
    if (WirelessLogFlag && !strcasecmp(WiMAXLogFlag, "on")) {

        ptr_log_flag = 1;
  
    }
    else {

        ptr_log_flag = 0;

    }

    if ( ptr_log_flag ) {

        if (!ptrlogFileOpenFlag) {

            ptrlogFileOpenFlag = true;

            char *ptrFile = NULL;

            if (ptrlogFileName) {

                ptrFile = (char *) malloc(strlen(GetConfigFileDir()) + strlen(ptrlogFileName) + 1);

                sprintf(ptrFile, "%s%s", GetConfigFileDir(), ptrlogFileName);

                fptr = fopen(ptrFile, "w+");

                free(ptrFile);

            } else {
                ptrFile = (char *) malloc(strlen(GetScriptName()) + 5);
                sprintf(ptrFile, "%s.ptr", GetScriptName());
                fptr = fopen(ptrFile, "w+");
                free(ptrFile);
            }

            if (fptr == NULL) {
                
                NSLOBJ_FATAL("Error : Cannot create packe trace file %s\n", ptrFile);
                exit(-1);

            }

            Event_ *heapHandle = createEvent();
            u_int64_t time;
            MILLI_TO_TICK(time, 100);
            u_int64_t chkInt = GetCurrentTime() + time;
            setEventTimeStamp(heapHandle, chkInt, 0);

            int (*__fun)(Event_*) = (int(*)(Event_*))&DequeueLogFromHeap;

            setEventCallOutFunc(heapHandle, __fun, heapHandle);
            scheduleInsertEvent(heapHandle);
        }

        _ptrlog = 1;
    }

    return NslObject::init();
}

int OFDM_Mesh::recv(Event * ep) {

    BASE_OBJTYPE(type);
    Packet *p;
    struct PHYInfo *phyInfo;
    struct LogInfo *log_info;
    struct wphyInfo *wphyinfo;

    u_int64_t timeInTick;
    double recvPower, *dist;

    p = (Packet *) ep->DataInfo_;
    phyInfo = (struct PHYInfo *) p->pkt_getinfo("phyInfo");
    dist = (double *) p->pkt_getinfo("dist");
    wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");

    assert(p && phyInfo && dist && wphyinfo);

    //printf("%d OFDM_Mesh::recv() (len=%d) @ %lld\n",
    //      get_nid(), p->pkt_getlen(), GetCurrentTime());

    //printf("\tphyInfo: power=%lf, nsymbols=%d, fec=%d\n",
    //      phyInfo->power, phyInfo->nsymbols, phyInfo->fec);


    if ( GetCurrentTime() < start_to_function_time ) {
    
        freePacket(ep);
        return 1;
    
    }

    if (phyInfo->ChannelID != _ChannelID) {
//          freePacket(ep);
//          return 1;
    }
    // Convey additional PHY information.
    //recvPower = channelModel->receivePower(phyInfo->power, *dist);

    /* The power is already calculated in CM module */
    recvPower = wphyinfo->Pr_;

    phyInfo->SNR = channelModel->powerToSNR(recvPower);
    phyInfo->frameStartTime = GetCurrentTime();


    /* always obtaining log_info now to print the type of dropped packets. */
    if ( 1 || ptr_log_flag ) {
    
        log_info = (struct LogInfo *) p->pkt_getinfo("loginfo");

        if (!log_info) {
 
            NSLOBJ_FATAL("OFDM_Mesh::recv(): missing log information. \n");
            exit(1);

        }

    }

    if (recvState != Idle) {

        recvState = Collided;

        /* log StartRX packets */
        u_char burst_type = wimax_burst_type_mapping(log_info->burst_type);

        NSLOBJ_INFO("\e[1;31mdrop a packet(%s) from node %u due to collision.\e[m\n",
                    get_wimax_burst_type_mapping_str(log_info->burst_type),
                    log_info->src_nid );
	INFO_FUNC(fflush(stdout));
        

        if (ptr_log_flag) {

            struct logEvent *logep;

            mac80216_log_t *mac80216_log_p1 =
                (mac80216_log_t *) malloc(sizeof(mac80216_log_t));
            
            mac80216_log_t *mac80216_log_p2 =
                (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

            double trans_time = (log_info->nsymbols) * Ts;
            u_int64_t trans_time_in_tick;
            MICRO_TO_TICK(trans_time_in_tick, trans_time);

            /* log StartRX packets in .ptr file. */

            LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
                    GetCurrentTime(), get_type(),
                    get_nid(), StartRX,
                    log_info->src_nid, get_nid(),
                    log_info->connection_id, burst_type,
                    log_info->burst_len,
                    log_info->channel_id);

            INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
                    mac80216_log_p1->Time + START,
                    mac80216_log_p1);


            if (recvState == Collided) {

                DROP_LOG_802_16(mac80216_log_p1,
                        mac80216_log_p2,
                        GetCurrentTime() +
                        trans_time_in_tick, DropRX,
                        DROP_COLL);
                
#ifdef __JCLIN_OFDM_MESH_LOG_DEBUG
                dump_mac80216_logep(mac80216_log_p1);
                dump_mac80216_logep(mac80216_log_p2);
#endif

            } 
            else {

                LOG_MAC_802_16(mac80216_log_p2, GetCurrentTime() + trans_time_in_tick,
                            GetCurrentTime(), get_type(), get_nid(),
                            SuccessRX, log_info->src_nid, get_nid(),
                            log_info->connection_id, burst_type, log_info->burst_len,
                            log_info->channel_id);


            }


            INSERT_TO_HEAP(logep, mac80216_log_p2->PROTO,
                    mac80216_log_p2->Time + ENDING,
                    mac80216_log_p2);

        }


        freePacket(ep);
        return 1;
    }
    
    /*
     * Simulate the transmission time of the burst.
     */

    MICRO_TO_TICK(timeInTick, (phyInfo->nsymbols) * Ts);

    timeInTick += GetCurrentTime();
    timeInTick -= 60;	// To avoid unnecessary collision.
    type = POINTER_TO_MEMBER(OFDM_Mesh, recvHandler);

    setObjEvent(ep, timeInTick, 0, this, type, ep->DataInfo_);
    recvState = Busy;


    /* C.C. Lin:
    * Since  OFDM_Mesh module automatically decrements the receive transmission
    * time, add the information of correct time point that the module starts
    * to receive this burst into the packet. In such way, the events representing
    * START_XXX can match its corresponding END_XXX events correctly.
    */


    u_int64_t start_recv_time = GetCurrentTime();
    p->pkt_addinfo("srecv_t", (const char *) &start_recv_time,
            sizeof(u_int64_t));

    struct logEvent *logep = NULL;

    /* log StartRX packets */

    if ( ptr_log_flag ) {

        mac80216_log_t *mac80216_log_p1 =
            (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

    
        u_char burst_type = wimax_burst_type_mapping(log_info->burst_type);

        LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(), GetCurrentTime(),
            get_type(), get_nid(), StartRX, log_info->src_nid,
            get_nid(), log_info->connection_id, burst_type,
            log_info->burst_len, log_info->channel_id);


        INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
            mac80216_log_p1->Time + START, mac80216_log_p1);

    }

    //printf("\tAt %lld, schedule a event which will be triggered at %lld (%lf us later)\n",
    //      GetCurrentTime(), timeInTick, (phyInfo->nsymbols)*Ts);

    return 1;
}

int OFDM_Mesh::send(Event * ep) {

    struct PHYInfo *phyInfo;
    struct LogInfo *log_info;
    Packet *p;
    int burstLen;		// Burst length in bytes
    int nsymbols;		// Burst length in symbols
    char *input, *output;
    int fec;
    int len;


    if ( GetCurrentTime() < start_to_function_time ) {
    
        freePacket(ep);
        return 1;
    
    }

    p = (Packet *) ep->DataInfo_;
    phyInfo = (struct PHYInfo *) p->pkt_getinfo("phyInfo");

    log_info = (struct LogInfo *) p->pkt_getinfo("loginfo");

    // Compute the burst length.
    fec = phyInfo->fec;
    len = p->pkt_getlen();
    input = p->pkt_sget();
    burstLen = channelCoding->getCodedBurstLen(len, fec);
    nsymbols = phyInfo->nsymbols;

    //printf("%d OFDM_Mesh::send() @ %lld fec=%d, burstLen=%d, nsymbols=%d\n",
    //      get_nid(), GetCurrentTime(), fec, burstLen, nsymbols);

    // Allocate a new Packet object.
    p = new Packet;
    //p->pkt_setflow(PF_RECV);
    output = p->pkt_sattach(burstLen);
    p->pkt_sprepend(output, burstLen);

    // Convey additional PHY information.
    phyInfo->power = transPower;
    phyInfo->fec = fec;
    phyInfo->ChannelID = _ChannelID;
    p->pkt_addinfo("phyInfo", (const char *) phyInfo,
            sizeof(struct PHYInfo));


    // Encode the burst.
    channelCoding->encode(input, output, len, fec);

    /* Packet Animation Log */


    // Send the burst.

    if (log_info) {

        log_info->nsymbols = phyInfo->nsymbols;
        log_info->burst_len = burstLen;
        log_info->channel_id = _ChannelID;
        p->pkt_addinfo("loginfo", (const char *) log_info,
                sizeof(struct LogInfo));

    }


    sendToPeers(p, log_info);
    delete p;

    // If the upper module wants to temporarily hold outgoing packets for
    // retransmission, it is the upper module's responsibility to duplicate
    // another copy of this packet.
    freePacket(ep);

    return 1;
}

void OFDM_Mesh::sendToPeers(Packet * packet, struct LogInfo *log_info)
{
    Event *ep;
    struct con_list *wi;
    //double delay;		// in micro second
    double dist;
    //u_int64_t ticks;

    struct wphyInfo         *wphyinfo;
    double currAzimuthAngle;
    double T_locX, T_locY, T_locZ;

    double		recvPower = 0.0;
    double		*rxCSThresh_; // carrier sense threshold of receiver
    cm			*cmobj;

    //printf("%d OFDM_Mesh::sendToPeers()\n", get_nid());

    wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
    assert(wphyinfo);

    GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
    currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

    wphyinfo->nid           = get_nid();
    wphyinfo->pid           = get_port();
    wphyinfo->bw_           = 0;
    wphyinfo->channel_      = _ChannelID;
    wphyinfo->TxPr_         = pow(10, transPower / 10) * 1e-3; // watt
    //wphyinfo->TxPr_         = transPower * 1e-3;
    wphyinfo->RxPr_         = 0.0;
    wphyinfo->srcX_         = T_locX;
    wphyinfo->srcY_         = T_locY;
    wphyinfo->srcZ_         = T_locZ;
    wphyinfo->currAzimuthAngle_     = currAzimuthAngle;
    wphyinfo->Pr_	    = 0.0;

    packet->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
    free(wphyinfo);

    /*
     * Index of using pre-computed received power.
     * We'll examine this index in cm module
     */
    int *uPrecompute = new(int);
    *uPrecompute = 1;
    packet->pkt_addinfo("uPre", (char *)uPrecompute, sizeof(int));

    packet->pkt_setflow(PF_SEND);
    //BASE_OBJTYPE(type);
    //type = POINTER_TO_MEMBER(OFDM_80216, get);

    SLIST_FOREACH(wi, &headOfWiMAX_, nextLoc) {

	struct wphyInfo         *tmpWphyinfo;
	char cm_obj_name[100];

        // The receiver should not be myself
        if (get_nid() == wi->obj->get_nid() &&
            get_port() == wi->obj->get_port())
            continue;

        dist = GetNodeDistance(get_nid(), wi->obj->get_nid());

	/* Get receiver's CSThresh and use it to compare the pre-computed power */
	rxCSThresh_ = (double *)get_regvar(wi->obj->get_nid(),
			wi->obj->get_port(),
			"CSThresh");
	if(rxCSThresh_ == NULL){
		/* The peer is not using ofdm_mesh module. */
		printf("Node %d:: No CSThresh!!\n\n", wi->obj->get_nid());
		continue;
	}

	/* call receiver's computePr function in channel model module */
	sprintf(cm_obj_name, "Node%d_CM_LINK_%d", wi->obj->get_nid(), wi->obj->get_port());
	cmobj = (cm*)RegTable_.lookup_Instance(wi->obj->get_nid(), cm_obj_name);
	if( !cmobj ){
		// The peer is not using the cm module.
		printf("Node %d:: No CM this Instance!!\n\n", wi->obj->get_nid());
		continue;
	}

	/*
	 * Pre-compute the received power when receiver receives this packet:
	 *  An optimization to reduce the number
	 *  of unnecessary packet duplication and transmission
	 */
	tmpWphyinfo = (struct wphyInfo *)packet->pkt_getinfo("WPHY");
	recvPower = cmobj->computePr(tmpWphyinfo);

	if(recvPower < *rxCSThresh_)
	{
		/* The pre-computed recvPower is lower than the peer node's receive power sensitivity.
		 * Therefore, no need to send this packet to the peer node.
		 */
		//printf("Node %d: can't send to node %d!! recvP %lf, recvS %lf\n\n", get_nid(), wi->obj->get_nid(), recvPower, *rxCSThresh_);
		continue;
	}

        ep = createEvent();
        ep->DataInfo_ = packet->copy();

        ((Packet *) ep->DataInfo_)->pkt_addinfo("dist",
                            (char *) &dist,
                            sizeof(double));
        // Simulate propagation
	/*
        delay = (dist / SPEED_OF_LIGHT) * 1000000;
        MICRO_TO_TICK(ticks, delay);
        ticks += GetCurrentTime();

        setObjEvent(ep, ticks, 0, wi->obj, type, ep->DataInfo_);
	*/
	ep->timeStamp_ = 0;
	ep->perio_ = 0;
	ep->calloutObj_ = wi->obj;
	ep->memfun_ = NULL;
	ep->func_ = NULL;
	NslObject::send(ep);

        if (ptr_log_flag) {

            u_char burst_type =
                wimax_burst_type_mapping(log_info->burst_type);

            if (log_info->dst_nid != PHY_BROADCAST_ID) {

                if (log_info->dst_nid ==
                    wi->obj->get_nid()) {

                    struct logEvent *logep;

                    mac80216_log_t *mac80216_log_p1 =
                        (mac80216_log_t *)
                        malloc(sizeof(mac80216_log_t));

                    /* log StartTX event */
                    LOG_MAC_802_16(mac80216_log_p1,
                            GetCurrentTime(),
                            GetCurrentTime(),
                            get_type(),
                            get_nid(), StartTX,
                            log_info->src_nid,
                            log_info->dst_nid,
                            log_info->
                            connection_id,
                            burst_type,
                            log_info->burst_len,
                            log_info->
                            channel_id);

                    INSERT_TO_HEAP(logep,
                            mac80216_log_p1->
                            PROTO,
                            mac80216_log_p1->
                            Time + START,
                            mac80216_log_p1);

                    double trans_time =
                        (log_info->nsymbols) * Ts;

                    u_int64_t trans_time_in_tick;
                    MICRO_TO_TICK(trans_time_in_tick,
                            trans_time);

                    mac80216_log_t *mac80216_log_p2 =
                        (mac80216_log_t *)
                        malloc(sizeof(mac80216_log_t));

                    /* log SuccessTX event */
                    LOG_MAC_802_16(mac80216_log_p2,
                            GetCurrentTime() +
                            trans_time_in_tick,
                            GetCurrentTime(),
                            get_type(),
                            get_nid(),
                            SuccessTX,
                            log_info->src_nid,
                            log_info->dst_nid,
                            log_info->
                            connection_id,
                            burst_type,
                            log_info->burst_len,
                            log_info->
                            channel_id);

#ifdef __JCLIN_OFDM_MESH_LOG_DEBUG

                    printf
                        ("OFDM_MESH::sendToPeers(): insert log event into heap.\n");

                    dump_mac80216_logep(mac80216_log_p1);
                    dump_mac80216_logep(mac80216_log_p2);

#endif

                    INSERT_TO_HEAP(logep,
                            mac80216_log_p2->
                            PROTO,
                            mac80216_log_p2->
                            Time + ENDING,
                            mac80216_log_p2);
                }
            } else {

                struct logEvent *logep;

                mac80216_log_t *mac80216_log_p1 =
                    (mac80216_log_t *)
                    malloc(sizeof(mac80216_log_t));

                /* log StartTX event */
                LOG_MAC_802_16(mac80216_log_p1,
                        GetCurrentTime(),
                        GetCurrentTime(),
                        get_type(), get_nid(),
                        StartTX, log_info->src_nid,
                        wi->obj->get_nid(),
                        log_info->connection_id,
                        burst_type,
                        log_info->burst_len,
                        log_info->channel_id);

                //printf("OFDM_MESH::sendToPeers(): insert log event into heap.\n");

                INSERT_TO_HEAP(logep,
                        mac80216_log_p1->PROTO,
                        mac80216_log_p1->Time +
                        START, mac80216_log_p1);

                double trans_time =
                    (log_info->nsymbols) * Ts;
                u_int64_t trans_time_in_tick;
                MICRO_TO_TICK(trans_time_in_tick,
                        trans_time);

                mac80216_log_t *mac80216_log_p2 =
                    (mac80216_log_t *)
                    malloc(sizeof(mac80216_log_t));

                /* log SuccessTX event */
                LOG_MAC_802_16(mac80216_log_p2,
                        GetCurrentTime() +
                        trans_time_in_tick,
                        GetCurrentTime(),
                        get_type(), get_nid(),
                        SuccessTX,
                        log_info->src_nid,
                        wi->obj->get_nid(),
                        log_info->connection_id,
                        burst_type,
                        log_info->burst_len,
                        log_info->channel_id);

#ifdef __JCLIN_OFDM_MESH_LOG_DEBUG

                dump_mac80216_logep(mac80216_log_p1);
                dump_mac80216_logep(mac80216_log_p2);

#endif

                INSERT_TO_HEAP(logep,
                        mac80216_log_p2->PROTO,
                        mac80216_log_p2->Time +
                        ENDING, mac80216_log_p2);

            }

        }
        //printf("\tdistance(%d->%d)=%lf, delay=%lf\n", get_nid(), wi->obj->get_nid(), dist, delay);
        //printf("\twill arrive at %lld\n", ticks);
    }
}

void OFDM_Mesh::recvHandler(Event * pkt) {

    Event_ *ep;
    Packet *p;

    struct wphyInfo         *wphyinfo;
    struct PHYInfo *phyInfo;
    struct LogInfo *log_info;

    double *dist;
    char *input, *output;
    int fec, len, burstLen;
    double avgSNR, SNR, saveSNR;
    double BER;
    int i;

    //printf("%d OFDM_Mesh::recvHandler() At %lld\n", get_nid(), GetCurrentTime());

    p = (Packet *) pkt->DataInfo_;
    wphyinfo = (struct wphyInfo *) p->pkt_getinfo("WPHY");
    phyInfo = (struct PHYInfo *) p->pkt_getinfo("phyInfo");

    dist = (double *) p->pkt_getinfo("dist");

    /* The acquisition of log_info is required to print
     * the type of dropped packet. So we turn it on always now.
     * This action can be turned off if we don't need to print
     * the type of dropped packet anymore.
     */

    if ( 1 || !strcasecmp(WiMAXLogFlag, "on")) {

        log_info = (struct LogInfo *) p->pkt_getinfo("loginfo");

        if (!log_info) {

            NSLOBJ_INFO("OFDM_Mesh::recvHandler(): missing log information. \n");
            exit(1);

        }
    }

    if (ptr_log_flag) {
    
        struct logEvent *logep;

        mac80216_log_t *mac80216_log_p1 =
            (mac80216_log_t *) malloc(sizeof(mac80216_log_t));

        u_char burst_type =
            wimax_burst_type_mapping(log_info->burst_type);

        u_int64_t *start_recv_time =
            (u_int64_t *) p->pkt_getinfo("srecv_t");

        if (recvState == Collided) {

            LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
                    *start_recv_time, get_type(),
                    get_nid(), DropRX,
                    log_info->src_nid, get_nid(),
                    log_info->connection_id, burst_type,
                    log_info->burst_len,
                    log_info->channel_id);

            mac80216_log_p1->DropReason = DROP_COLL;

    //        dump_mac80216_logep(mac80216_log_p1);
    //   dump_mac80216_logep(mac80216_log_p2);
    

        } else {

            LOG_MAC_802_16(mac80216_log_p1, GetCurrentTime(),
                    *start_recv_time, get_type(),
                    get_nid(), SuccessRX,
                    log_info->src_nid, get_nid(),
                    log_info->connection_id, burst_type,
                    log_info->burst_len,
                    log_info->channel_id);


        }


        INSERT_TO_HEAP(logep, mac80216_log_p1->PROTO,
                mac80216_log_p1->Time + ENDING,
                mac80216_log_p1);

    }


    if (recvState == Collided) {

        NSLOBJ_INFO("\e[1;31mdrop a packet(%s) from node %u due to collision.\e[m\n",
                    get_wimax_burst_type_mapping_str(log_info->burst_type),
                    log_info->src_nid );
	INFO_FUNC(fflush(stdout));
        
        recvState = Idle;
        
        
        
        freePacket(pkt);
        return;

    }
    //
    // Decode the burst and deliver it to the MAC layer.
    //
    input = p->pkt_sget();
    fec = phyInfo->fec;
    len = p->pkt_getlen();

    burstLen = channelCoding->getUncodedBurstLen(len, fec);

    p = new Packet;
    p->pkt_setflow(PF_RECV);
    output = p->pkt_sattach(burstLen);
    p->pkt_sprepend(output, burstLen);

    saveSNR = phyInfo->SNR;
    BER = channelModel->computeBER(fec, saveSNR);
    channelModel->makeBitError(input, len, BER);
    channelCoding->decode(input, output, len, fec);



    // We obtain avgSNR by averaging 100 samples. This facilitates the
    // profile selection in MAC, otherwise MAC will probably receive
    // some extreme values and make bad selection.
    char	cm_obj_name[100];
    cm		*cmobj;

    sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
    cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
    if(!cmobj){
	    fprintf(stderr, "[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
	    exit(0);
    }
    for (i = 1, avgSNR = 0; i <= 100; i++) {
        SNR = channelModel->powerToSNR(cmobj->computePr(wphyinfo));
        avgSNR += ((SNR - avgSNR) / i);
    }

    // Convey additional PHY information.
    phyInfo->SNR = avgSNR;
    p->pkt_addinfo("phyInfo", (const char *) phyInfo,
            sizeof(struct PHYInfo));

    ep = createEvent();
    ep->DataInfo_ = p;
    put(ep, recvtarget_);

    recvState = Idle;
    freePacket(pkt);
}



u_char wimax_burst_type_mapping(mac80216_burst_t log_burst_type)
{

    if (log_burst_type == BT_DATA)
        return FRAMETYPE_WIMAX_MESH_DATA;	//    0x1c   /* WiMAX Mesh mode data burst */

    else if (log_burst_type == BT_NETWORK_ENTRY)
        return FRAMETYPE_WIMAX_MESH_NENT;	//    0x1d   /* WiMAX Mesh mode netowrk Entry burst */

    else if (log_burst_type == BT_NETWORK_CFG)
        return FRAMETYPE_WIMAX_MESH_NCFG;	//    0x1e   /* WiMAX Mesh mode network config burst */

    else if (log_burst_type == BT_DSCHED)
        return FRAMETYPE_WIMAX_MESH_DSCH;	//    0x20   /* WiMAX Mesh mode distributed schedule burst */

    else if (log_burst_type == BT_CSCHED)
        return UNDEFINED;

    else if (log_burst_type == BT_CSCF)
        return UNDEFINED;

    else if (log_burst_type == BT_SBRREG)
        return FRAMETYPE_WIMAX_MESH_SBCREQ;	//     0x21   /* WiMAX Mesh mode sbc request burst */

    else if (log_burst_type == BT_REQREG)
        return FRAMETYPE_WIMAX_MESH_REGREQ;	//     0x22   /* WiMAX Mesh mode register request burst */

    else if (log_burst_type == BT_SPONSOR)
        return FRAMETYPE_WIMAX_MESH_SPONSOR;	//    0x23   /* WiMAX Mesh mode sponsor burst */

    else
        return UNDEFINED;


}

const char* get_wimax_burst_type_mapping_str(mac80216_burst_t log_burst_type)
{

    if (log_burst_type == BT_DATA)
        return "FRAMETYPE_WIMAX_MESH_DATA";   //    0x1c   /* WiMAX Mesh mode data burst */

    else if (log_burst_type == BT_NETWORK_ENTRY)
        return "FRAMETYPE_WIMAX_MESH_NENT";   //    0x1d   /* WiMAX Mesh mode netowrk Entry burst */

    else if (log_burst_type == BT_NETWORK_CFG)
        return "FRAMETYPE_WIMAX_MESH_NCFG";   //    0x1e   /* WiMAX Mesh mode network config burst */

    else if (log_burst_type == BT_DSCHED)
        return "FRAMETYPE_WIMAX_MESH_DSCH";   //    0x20   /* WiMAX Mesh mode distributed schedule burst */

    else if (log_burst_type == BT_CSCHED)
        return "UNSUPPORTED(BT_CSCHED)";

    else if (log_burst_type == BT_CSCF)
        return "UNSUPPORTED(BT_CSCF)";

    else if (log_burst_type == BT_SBRREG)
        return "FRAMETYPE_WIMAX_MESH_SBCREQ"; //     0x21   /* WiMAX Mesh mode sbc request burst */

    else if (log_burst_type == BT_REQREG)
        return "FRAMETYPE_WIMAX_MESH_REGREQ"; //     0x22   /* WiMAX Mesh mode register request burst */

    else if (log_burst_type == BT_SPONSOR)
        return "FRAMETYPE_WIMAX_MESH_SPONSOR";    //    0x23   /* WiMAX Mesh mode sponsor burst */

    else
        return "UNDEFINED";


}
