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

#include "ofdma_pmprs_NT.h"

MODULE_GENERATOR(OFDMA_PMPRS_NT);

//extern SLIST_HEAD(wimax_head, con_list) headOfMobileWIMAX_;
extern SLIST_HEAD(wimax_head, con_list) headOfMRWIMAXNT_;
extern typeTable *typeTable_;
extern char *MR_WIMAXChannelCoding_NT;

    OFDMA_PMPRS_NT::OFDMA_PMPRS_NT(uint32_t type, uint32_t id, struct plist *pl, const char *name)
:OFDMA_80216j_NT(type, id, pl, name)
{
    /* Initial Registers */
    recvState           = Idle;
    _buffered_ePacket   = NULL;
    relay_flag          = true;
    //_Gain               = 5;

    /* Initial receive timer */
    BASE_OBJTYPE(func);
    _recvTimer = new timerObj;
    assert(_recvTimer);
    func = POINTER_TO_MEMBER(OFDMA_PMPRS_NT, recvHandler_superordinate);
    _recvTimer->setCallOutObj(this, func);
}

OFDMA_PMPRS_NT::~OFDMA_PMPRS_NT()
{
    delete _recvTimer;
}

int OFDMA_PMPRS_NT::init(void)
{
    _DCDProfile = GET_REG_VAR(get_port(), "DCDProfile", struct DCDBurstProfile *);
    _UCDProfile = GET_REG_VAR(get_port(), "UCDProfile", struct UCDBurstProfile *);

    return NslObject::init();
}

int OFDMA_PMPRS_NT::recv(ePacket_ *epkt)
{
    double          recvPower   = 0;
    double          *dist       = NULL;
    Packet          *pkt        = NULL;
    struct PHYInfo  *phyInfo    = NULL;
    struct wphyInfo *wphyinfo    = NULL;

    uint8_t Nsymbols     = 0;
    uint64_t timeInTick = 0;

    /* Get packet information */
    pkt     = (Packet *) epkt->DataInfo_;
    phyInfo = (struct PHYInfo *) pkt->pkt_getinfo("phyInfo");
    dist    = (double *) pkt->pkt_getinfo("dist");
    wphyinfo = (struct wphyInfo *)pkt->pkt_getinfo("WPHY");

    memset(&LastSignalInfo,0,sizeof(LastSignalInfo));
    memcpy(&LastSignalInfo, pkt->pkt_getinfo("phyInfo"), sizeof(LastSignalInfo));
    assert(pkt && phyInfo && dist && wphyinfo);

    //printf("Time:%llu RS[%d] recv (phyInfo->burst_type = %d)\n",GetCurrentTime(),get_nid(),phyInfo->burst_type);
    //printf("packet size : phyInfo(%d) + dist(%d) + wphyinfo(%d)\n",sizeof(PHYInfo),sizeof(double),sizeof(wphyInfo));
    // check burst type
    if(phyInfo->burst_type == DL_ACCESS && phyInfo->relay_flag == false)
    {
        //printf("Time:%llu RS[%d] recv BS DL Access Burst\n",GetCurrentTime(),get_nid());
        _frameStartTime         = GetCurrentTime();

        //epkt->DataInfo_ = NULL;
        freePacket(epkt);
        return 1;
    }

    /* Check channel ID */
    if (phyInfo->ChannelID != _ChannelID)
    {
        freePacket(epkt);
        return 1;
    }

    /* Compute recvPower and SNR */
    recvPower       = wphyinfo->Pr_;
    phyInfo->SNR    = _channelModel->powerToSNR(recvPower);

    /* Check receive power and compare with sensitivity */
    if (recvPower < _RecvSensitivity)
    {
        freePacket(epkt);
        return 1;
    }

    //printf("Time:%llu OFDMA_PMPRS_NT[%d]::%s() from Node[%d]\n",GetCurrentTime(),get_nid(),__func__,phyInfo->nid);

    //printf("RS recv time = %llu\n",GetCurrentTime());
    /* Check receive status */
    if (recvState != Idle)
    {
        fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPRS_NT(%d)::recv() PHY is busy now! Receive State (%d) Error  from node[%d]\e[0m\n", GetCurrentTime(), get_nid(), recvState,phyInfo->nid);

#if NT_DEBUG 
        printf("frameStartTime = %llu\n",_frameStartTime);
        printf("_ULAccessSymbols = %d\n",_ULAccessSymbols);
        printf("_ULRelaySymbols  = %d\n",_ULRelaySymbols);
        printf("_DLAccessSymbols = %d\n",_DLAccessSymbols);
        printf("_DLRelaySymbols  = %d\n",_DLRelaySymbols);
#endif

        freePacket(epkt);
        return 1;

        //exit(1);
    }

    /* Clear the previous bufferedPacket */
    if (_buffered_ePacket != NULL)
    {
        freePacket(_buffered_ePacket);
        _buffered_ePacket = NULL;
    }

    if(phyInfo->uiuc == CDMA_BWreq_Ranging)
    {
        //printf("Time:%llu OFDMA_PMPRS_NT::%s CDMA_BWreq_Ranging from Node[%d]\n",GetCurrentTime(),__func__,phyInfo->nid);
        if (phyInfo->ulmap_ie.ie_12.rangMethod == 0)
        {
            Nsymbols = 2;
        }
        else if (phyInfo->ulmap_ie.ie_12.rangMethod == 1)
        {
            Nsymbols = 4;
        }
        else if (phyInfo->ulmap_ie.ie_12.rangMethod == 2)
        {
            Nsymbols = 1;
        }
        else 
        {
            Nsymbols = 3;
        }

        // Save start recv time for log
        uint64_t start_recv_time = GetCurrentTime();
        pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));

        MICRO_TO_TICK(timeInTick, Nsymbols * Ts);
        timeInTick += GetCurrentTime();
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(OFDMA_PMPRS_NT, recvHandler_subordinate);
        setObjEvent(epkt, timeInTick, 0, this, type, epkt->DataInfo_);
    }
    else if(phyInfo->uiuc == CDMA_Alloc_IE)
    {
        //printf("Time:%llu OFDMA_PMPRS_NT::%s CDMA_Alloc_IE from Node[%d]\n",GetCurrentTime(),__func__,phyInfo->nid);
        int duration = phyInfo->ulmap_ie.ie_14.duration;
        if(duration * UL_PUSC > _ULAccessSymbols)
        {
            Nsymbols = _ULAccessSymbols; 
        }
        else
        {
            Nsymbols = duration * UL_PUSC;
        }

        // Save start recv time for log
        uint64_t start_recv_time = GetCurrentTime();
        pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));


        MICRO_TO_TICK(timeInTick, Nsymbols * Ts);
        timeInTick += GetCurrentTime();
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(OFDMA_PMPRS_NT, recvHandler_subordinate);
        setObjEvent(epkt, timeInTick, 0, this, type, epkt->DataInfo_);
    }
    else if(phyInfo->uiuc > 0 && phyInfo->uiuc < 11)
    {
        //printf("Time:%llu OFDMA_PMPRS_NT::%s normal from Node[%d]\n",GetCurrentTime(),__func__,phyInfo->nid);
        Nsymbols = _DLAccessSymbols;

        // Save start recv time for log
        uint64_t start_recv_time = GetCurrentTime();
        pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));


        MICRO_TO_TICK(timeInTick, Nsymbols * Ts);
        timeInTick += GetCurrentTime();
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(OFDMA_PMPRS_NT, recvHandler_subordinate);
        setObjEvent(epkt, timeInTick, 0, this, type, epkt->DataInfo_);

    }
    else
    {   // DL burst
        //printf("Time:%llu OFDMA_PMPRS_NT::%s DL burst from Node[%d] burst_type=%d\n",GetCurrentTime(),__func__,phyInfo->nid,phyInfo->burst_type);
        _ULAccessSymbols = phyInfo->ULAccessSym;
        _ULRelaySymbols = phyInfo->ULRelaySym;
        _DLAccessSymbols = phyInfo->DLAccessSym;
        _DLRelaySymbols = phyInfo->DLRelaySym;

        //uint64_t ticks = 0;
        //MICRO_TO_TICK(ticks,(_DLAccessSymbols)*Ts);

        /* Save the new arrival packet and change state to ProcRzonePrefix */
        _buffered_ePacket       = epkt;
        recvState               = ProcRzonePrefix;
        _burstPtr               = pkt->pkt_sget();
        _frameEndPtr            = _burstPtr + pkt->pkt_getlen();
        //_frameStartTime         = GetCurrentTime() - ticks;
        phyInfo->frameStartTime = _frameStartTime;

        /* Save start recv time for log */
        uint64_t start_recv_time = GetCurrentTime();
        pkt->pkt_addinfo("srecv_t", (char *) &start_recv_time, sizeof(uint64_t));

        /* Simulate the transmission time of the preceding */
        /* Preamble and RzonePrefix (include R-MAP)              */
        scheduleTimer(_recvTimer, (int)((DL_PUSC) * Ts));
    }
    return 1;
}

int OFDMA_PMPRS_NT::send(ePacket_ *epkt)
{
    struct PHYInfo phyInfo;
    vector<upBurst *> *burstCollection = NULL;
    vector<downBurst *> *DLAccessBurstCollection = NULL;

    BurstInfo *burstInfo    = NULL;
    Packet *ulburst_pkt     = NULL;
    Packet *dlFrame_pkt     = NULL;
    upBurst *uburst         = NULL;
    downBurst *burst        = NULL;
    char *input             = NULL;
    char *output            = NULL;
    char *tmp_dlfp          = NULL;
    int numBytes            = 0;
    int slots               = 0;
    int codedLen            = 0;
    int uncodedLen          = 0;
    int fec                 = 0;
    int repe                = 0;
    int uiuc                = 0;
    int len                 = 0;
    int n                   = 0;

    struct DLFP_except_128 dl_frame_prefix;
    vector<downBurst *>::iterator itd;

    /* Process the R-MAP message from upper layer */
    if (procCmdFromMAC((Pkthdr *)epkt->DataInfo_) == 1)
    {
        epkt->DataInfo_ = NULL;
        freePacket(epkt);
        return 1;
    }

    /* Process the ranging code message from upper layer */
    if (procRNGCode((Pkthdr *)epkt->DataInfo_) == 1)
    {
        epkt->DataInfo_ = NULL;
        freePacket(epkt);
        return 1;
    }

    /* Get the burst information */
    burstInfo       = (BurstInfo *)epkt->DataInfo_;

    if(burstInfo->type == DL_ACCESS)
    {
        //printf("\e[35mTime:%llu:OFDMA_PMPRS_NT(%d)::%s\e[0m\n", GetCurrentTime(),get_nid(),__func__);

        DLAccessBurstCollection = (vector <downBurst *> *)burstInfo->Collection;
        if(DLAccessBurstCollection->empty())
        {
            fprintf(stderr,"\e[1;35mTime:%llu:OFDMA_PMPRS_NT(%d)::%s DLAccessBurstCollection is empty!!!\e[0m\n",GetCurrentTime(),get_nid(),__func__);
            //exit(1);
            freePacket(epkt);
            return 1;
        }

        /* 1. Compute total length */
        // Get coded length of RzonePrefix (12 Byte * 4 repetition)
        repe        = REPETATION_CODEWORD_4;
        fec         = QPSK_1_2;
        numBytes    = _channelCoding->getCodedSlotLen(fec) * repeat_times_NT[repe];

        // Get coded length of DL-MAP
        repe        = REPETATION_CODEWORD_1;
        fec         = QPSK_1_2;
        numBytes    += _channelCoding->getCodedBurstLen(_rmapLen, repeat_times_NT[repe], fec);

        for (itd = DLAccessBurstCollection->begin();itd != DLAccessBurstCollection->end();itd++)
        {
            burst       = *itd;
            fec         = _DCDProfile[burst->ie->_diuc].fec;
            slots       = (burst->dlmap_ie.ie_other.numSym * burst->dlmap_ie.ie_other.numCh / 2);
            len         = _channelCoding->getUncodedSlotLen(fec) * slots;
            n           = _channelCoding->getCodedBurstLen(len, repeat_times_NT[repe], fec);
            numBytes    += n;
        }

        /* 2. Allocate a new Packet, packet length depends on the frame length. */
        dlFrame_pkt = new Packet;
        assert(dlFrame_pkt);
        output      = dlFrame_pkt->pkt_sattach(numBytes);
        dlFrame_pkt->pkt_sprepend(output, numBytes);
        dlFrame_pkt->pkt_setflow(PF_RECV);

        /* 3. Convey additional PHY information. */
        memset(&phyInfo, 0, sizeof(struct PHYInfo));
        phyInfo.power       = _transPower;
        phyInfo.ChannelID   = _ChannelID;
        phyInfo.ULAccessSym = _ULAccessSymbols;
        phyInfo.ULRelaySym  = _ULRelaySymbols;
        phyInfo.DLAccessSym = _DLAccessSymbols;
        phyInfo.DLRelaySym  = _DLRelaySymbols;
        phyInfo.relay_flag  = true;
        phyInfo.burst_type  = DL_ACCESS;
        phyInfo.nid         = get_nid();
        phyInfo.pid         = get_port();
#if NT_DEBUG 
        printf("----- OFDMA_PMPRS_NT[%d]::%s() -----\n",get_nid(),__func__);
        printf("phyInfo.ULAccessSym = %d\n",phyInfo.ULAccessSym);
        printf("phyInfo.ULRelaySym = %d\n",phyInfo.ULRelaySym);
        printf("phyInfo.DLAccessSym = %d\n",phyInfo.DLAccessSym);
        printf("phyInfo.DLRelaySym = %d\n",phyInfo.DLRelaySym);
        printf("---------------------------------\n\n");
#endif

        dlFrame_pkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));

        /* 4. Build and encode the DLFP message */
        buildDLFP(&dl_frame_prefix, _dlmapLen_NT);

        // Duplicate 2 times to form a FEC block and repeat 4 times
        repe        = REPETATION_CODEWORD_4;
        fec         = QPSK_1_2;
        len         = sizeof(struct DLFP_except_128);  // 3
        tmp_dlfp    = new char [len * 2];
        memcpy(tmp_dlfp, &dl_frame_prefix, len);
        memcpy(tmp_dlfp + len, &dl_frame_prefix, len);
        slots       = _channelCoding->getNumUncodedSlotWithRepe(len * 2, repeat_times_NT[repe], fec);

        for (int i = 0;i < repeat_times_NT[repe];i++)
        {
            n = _channelCoding->encode(tmp_dlfp, output + i * 12, len * 2, slots, repeat_times_NT[repe], fec);
        }
        output += n;
        delete [] tmp_dlfp;

        /* 5. Encode the DL-MAP message */
        repe    = REPETATION_CODEWORD_1;
        fec     = QPSK_1_2;
        slots   = _channelCoding->getNumUncodedSlotWithRepe(_dlmapLen_NT, repeat_times_NT[repe], fec);
        n       = _channelCoding->encode(_dlmapBuf_NT, output, _dlmapLen_NT, slots, repeat_times_NT[repe], fec);
        output += n;
        delete _dlmapBuf_NT;

        /* 6. Encode the DL bursts */
        for (itd = DLAccessBurstCollection->begin();itd != DLAccessBurstCollection->end();itd++)
        {
            burst   = *itd;
            repe    = REPETATION_CODEWORD_1;
            fec     = _DCDProfile[burst->ie->_diuc].fec;
            len     = burst->length;
            input   = burst->payload;
            slots   = burst->dlmap_ie.ie_other.numSym * burst->dlmap_ie.ie_other.numCh / DL_PUSC;
            n       = _channelCoding->encode(input, output, len, slots, repeat_times_NT[repe], fec);
            output += n;
#if  0 
            if(burst->ie)
            {
                int diuc = burst->ie->_diuc;
                if(diuc!=Extended_DIUC_2 && diuc != Extended_DIUC && diuc != 13 && burst->nCid!=0)
                {
                    delete [] burst->dlmap_ie.ie_other.cid;
                }
                delete burst->ie;
                burst->ie = NULL;
            }
            delete [] burst->payload;
            burst->payload = NULL;
#endif

        }

        /* Convey log information */
        struct LogInfo log_info;
        memset(&log_info, 0, sizeof(struct LogInfo));
        log_info.src_nid = get_nid();
        log_info.dst_nid = 0;  /* this field will be filled in sendToBS() */
        log_info.burst_len = codedLen;
        log_info.channel_id = _ChannelID;

        /* 7. Send the DL packet to SubStations. */
        sendToSubStations(dlFrame_pkt, &log_info);
        delete dlFrame_pkt;
    }
    else
    {// UL RELAY BURST 
        //printf("burstInfo->type = %d\n",burstInfo->type);
        burstCollection = (vector<upBurst *> *)burstInfo->Collection;

        /* Check the burstCollection size */
        if (burstCollection->size() != 1)
        {
            fprintf(stderr, "\e[1;31mRS(%d)::OFDMA_PMPRS_NT::%s(): burstCollection->size() != 1\e[0m\n", get_nid(), __func__);
            exit(1);
        }

        uburst = *(burstCollection->begin());

        if (uburst->ie->_uiuc == CDMA_Alloc_IE)   /* Special message of CDMA allocation */
        {
            fec     = _UCDProfile[uburst->ulmap_ie.ie_14.uiuc].fec;
            repe    = uburst->ulmap_ie.ie_14.repeCode;
            uiuc    = uburst->ulmap_ie.ie_14.uiuc;
        }
        else   /* Normal uplink burst */
        {
            fec     = _UCDProfile[uburst->ulmap_ie.ie_other.uiuc].fec;
            repe    = uburst->ulmap_ie.ie_other.repeCode;
            uiuc    = uburst->ulmap_ie.ie_other.uiuc;
        }

        /* Get the input data and compute data length */
        uncodedLen  = uburst->length;
        codedLen    = _channelCoding->getCodedBurstLen(uncodedLen, repeat_times_NT[repe], fec);
        input       = uburst->payload;

        /* Allocate a new packet */
        ulburst_pkt = new Packet;
        assert(ulburst_pkt);
        output      = ulburst_pkt->pkt_sattach(codedLen);
        ulburst_pkt->pkt_sprepend(output, codedLen);
        ulburst_pkt->pkt_setflow(PF_RECV);

        /* Encode the uplink burst */
        slots       = _channelCoding->getNumUncodedSlotWithRepe(uncodedLen, repeat_times_NT[repe], fec);
        _channelCoding->encode(input, output, uncodedLen, slots, repeat_times_NT[repe], fec);


        /* Convey additional PHY information */
        memset(&phyInfo, 0, sizeof(struct PHYInfo));
        phyInfo.power       = _transPower;
        phyInfo.fec         = fec;
        phyInfo.uiuc        = uiuc;
        phyInfo.ChannelID   = _ChannelID;
        phyInfo.nid = get_nid();
        phyInfo.pid = get_port();
        phyInfo.relay_flag = true;
        if(uburst->ie->_uiuc == CDMA_Alloc_IE)
        {
            phyInfo.RS_RNGREQ_flag = true;
        }
        else
        {
            phyInfo.RS_RNGREQ_flag = false; 
        }
        memcpy(&phyInfo.ulmap_ie, &uburst->ulmap_ie, sizeof(ULMAP_IE_u));
        ulburst_pkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));

        /* Convey log information */
        struct LogInfo log_info;
        memset(&log_info, 0, sizeof(struct LogInfo));
        log_info.src_nid = get_nid();
        log_info.dst_nid = 0;  /* this field will be filled in sendToBS() */
        log_info.burst_len = codedLen;
        log_info.channel_id = _ChannelID;

        /* Send the burst to BS */
        sendToBS(ulburst_pkt, &log_info);
        delete ulburst_pkt;

#if  0 
        /* clear upBurst */
        delete uburst->payload;
        uburst->payload = NULL;

        delete uburst->ie;
        uburst->ie = NULL;
        
        uburst = NULL;
#endif
    }

    /* Set epkt->DataInfo_ to NULL so that freePacket() won't free it again */
    epkt->DataInfo_ = NULL;
#ifdef nt 
    if(burstInfo->Collection != NULL)
    {
        WiMaxBurst *burst = NULL;
        while(!burstInfo->Collection->empty())
        {
            burst = burstInfo->Collection->back();
            if(burstInfo->type == DL_ACCESS)
            {
                ((downBurst *)burst)->Clear_downBurst();
                delete burst;
            }
            else
            {
                ((upBurst *)burst)->Clear_upBurst();
            }

            burst = NULL;
            burstInfo->Collection->pop_back();
        }

        delete burstInfo->Collection;
        burstInfo->Collection = NULL;
    }
#endif
    delete burstInfo;

    /* If the upper module wants to temporarily hold outgoing packets for   */
    /* retransmission, it is the upper module's responsibility to duplicate */
    /* another copy of this packet.                                         */
    freePacket(epkt);
    return 1;
}

void OFDMA_PMPRS_NT::buildDLFP(struct DLFP_except_128 *frame_prefix, int dlmapLen)
{
    int numSlots = _channelCoding->getNumUncodedSlotWithRepe(dlmapLen, repeat_times_NT[REPETATION_CODEWORD_1],    QPSK_1_2);

    frame_prefix->ch_bitmap = 0x3F;
    frame_prefix->rsv1      = 0;
    frame_prefix->repecode  = REPETATION_CODEWORD_1;    // no repetition coding on DL-MAP
    frame_prefix->coding    = CC;
    frame_prefix->len       = numSlots;
    frame_prefix->rsv2      = 0;
}

void OFDMA_PMPRS_NT::sendToSubStations(Packet *dlpkt,struct LogInfo *log_info)
{
    ePacket_ *epkt              = NULL;
    struct con_list *wi         = NULL;
    const char *nodeType        = NULL;
    double dist                 = 0.0;
    struct wphyInfo *wphyinfo   = NULL;
    double currAzimuthAngle     = 0.0;
    double T_locX               = 0.0;
    double T_locY               = 0.0;
    double T_locZ               = 0.0;
    int log_flag                = (!strncasecmp(MR_WIMAX_NT_LogFlag, "on", 2)) ? 1 : 0;

    //printf("\e[35mTime:%llu:OFDMA_PMPBS_NT(%d)::%s\e[0m\n", GetCurrentTime(),get_nid(),__func__);

    wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
    assert(wphyinfo);

    GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
    currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

    wphyinfo->nid               = get_nid();
    wphyinfo->pid               = get_port();
    wphyinfo->bw_               = 0;
    wphyinfo->channel_          = _ChannelID;
    wphyinfo->TxPr_             = pow(10, _transPower/10) * 1e-3; // Watt
    wphyinfo->RxPr_             = 0.0;
    wphyinfo->srcX_             = T_locX;
    wphyinfo->srcY_             = T_locY;
    wphyinfo->srcZ_             = T_locZ;
    wphyinfo->currAzimuthAngle_ = currAzimuthAngle;
    wphyinfo->Pr_               = 0.0;

    dlpkt->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
    free(wphyinfo);

    dlpkt->pkt_setflow(PF_SEND);
    SLIST_FOREACH(wi, &headOfMRWIMAXNT_, nextLoc)
    {
        nodeType = typeTable_->toName(wi->obj->get_type());

        /* The receiver should not be myself */
        if (get_nid() == wi->obj->get_nid() && get_port() == wi->obj->get_port())
            continue;

        /* We only send the frame to PMPMS module */
        if (strcmp(nodeType, "MR_WIMAX_NT_PMPMS") != 0)
            continue;

        /************ LOG START *************/
        if (log_flag)
        {
            struct logEvent *logep      = NULL;
            uint8_t burst_type          = FRAMETYPE_MR_WIMAX_NT_PMP_DLAccessBURST;
            double trans_time           = _DLAccessSymbols * Ts;
            uint64_t trans_time_in_tick = 0;

            MICRO_TO_TICK(trans_time_in_tick, trans_time);

            log_info->dst_nid = wi->obj->get_nid();

            /* log StartTX event */
            mac80216j_NT_log_t *mac80216j_NT_log_p1 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

            LOG_MAC_802_16j_NT(mac80216j_NT_log_p1, GetCurrentTime(), GetCurrentTime(), get_type(), get_nid(), StartTX,
                    log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

            INSERT_TO_HEAP(logep, mac80216j_NT_log_p1->PROTO, mac80216j_NT_log_p1->Time + START, mac80216j_NT_log_p1);
#if LOG_DEBUG
            printf("RS%d %llu ~ %llu %d->%d (StartTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime(), log_info->src_nid, log_info->dst_nid);
#endif
            /* log SuccessTX event */
            mac80216j_NT_log_t *mac80216j_NT_log_p2 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

            LOG_MAC_802_16j_NT(mac80216j_NT_log_p2, GetCurrentTime() + trans_time_in_tick, GetCurrentTime(), get_type(), get_nid(), SuccessTX,
                    log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

            INSERT_TO_HEAP(logep, mac80216j_NT_log_p2->PROTO, mac80216j_NT_log_p2->Time + ENDING, mac80216j_NT_log_p2);
#if LOG_DEBUG
            printf("RS%d %llu ~ %llu %d->%d (SuccessTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime() + trans_time_in_tick, log_info->src_nid, log_info->dst_nid);
#endif
            dlpkt->pkt_addinfo("loginfo", (char *) log_info, sizeof(struct LogInfo));
        }
        /************ LOG END *************/

        epkt = createEvent();
        epkt->DataInfo_ = dlpkt->copy();

        ((Packet *) epkt->DataInfo_)->pkt_addinfo("dist", (char *) &dist, sizeof(double));

        epkt->timeStamp_    = 0;
        epkt->perio_        = 0;
        epkt->calloutObj_   = wi->obj;
        epkt->memfun_       = NULL;
        epkt->func_         = NULL;
        NslObject::send(epkt);
    } 
}

void OFDMA_PMPRS_NT::sendToBS(Packet *ULburst, struct LogInfo *log_info)
{
    ePacket_ *epkt              = NULL;
    struct con_list *wi         = NULL;
    double dist                 = 0.0;
    const char *nodeType        = NULL;
    struct wphyInfo *wphyinfo   = NULL;
    double currAzimuthAngle     = 0.0;
    double T_locX               = 0.0;
    double T_locY               = 0.0;
    double T_locZ               = 0.0;
    int log_flag                = (!strncasecmp(MR_WIMAX_NT_LogFlag, "on", 2)) ? 1 : 0;

    //printf("Time:%llu RS(%d) send ranging code to BS\n",GetCurrentTime(),get_nid());

    wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
    assert(wphyinfo);

    GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ);
    currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

    wphyinfo->nid               = get_nid();
    wphyinfo->pid               = get_port();
    wphyinfo->bw_               = 0;
    wphyinfo->channel_          = _ChannelID;
    wphyinfo->TxPr_             = pow(10, _transPower/10) * 1e-3; // Watt
    wphyinfo->RxPr_             = 0.0;
    wphyinfo->srcX_             = T_locX;
    wphyinfo->srcY_             = T_locY;
    wphyinfo->srcZ_             = T_locZ;
    wphyinfo->currAzimuthAngle_ = currAzimuthAngle;
    wphyinfo->Pr_               = 0.0;

    ULburst->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
    free(wphyinfo);

    ULburst->pkt_setflow(PF_SEND);

    //SLIST_FOREACH(wi, &headOfMobileWIMAX_, nextLoc)
    SLIST_FOREACH(wi, &headOfMRWIMAXNT_, nextLoc)
    {
        nodeType = typeTable_->toName(wi->obj->get_type());

        /* The receiver should not be myself */
        if (get_nid() == wi->obj->get_nid() && get_port() == wi->obj->get_port())
            continue;

        /* We only send the frame to MobileWimax PMPBS module */
        if (strcmp(nodeType, "MR_WIMAX_NT_PMPBS") != 0)
            continue;

        /************ LOG START *************/
        if (log_flag)
        {
            struct logEvent *logep      = NULL;
            uint8_t burst_type          = FRAMETYPE_MR_WIMAX_NT_PMP_ULRelayBURST;
            double trans_time           = _ULRelaySymbols * Ts;
            uint64_t trans_time_in_tick = 0;

            MICRO_TO_TICK(trans_time_in_tick, trans_time);

            log_info->dst_nid = wi->obj->get_nid();

            /* log StartTX event */
            mac80216j_NT_log_t *mac80216j_NT_log_p1 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

            LOG_MAC_802_16j_NT(mac80216j_NT_log_p1, GetCurrentTime(), GetCurrentTime(), get_type(), get_nid(), StartTX,
                    log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

            INSERT_TO_HEAP(logep, mac80216j_NT_log_p1->PROTO, mac80216j_NT_log_p1->Time + START, mac80216j_NT_log_p1);
#if LOG_DEBUG
            printf("RS%d %llu ~ %llu %d->%d (StartTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime(), log_info->src_nid, log_info->dst_nid);
#endif
            /* log SuccessTX event */
            mac80216j_NT_log_t *mac80216j_NT_log_p2 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

            LOG_MAC_802_16j_NT(mac80216j_NT_log_p2, GetCurrentTime() + trans_time_in_tick, GetCurrentTime(), get_type(), get_nid(), SuccessTX,
                    log_info->src_nid, log_info->dst_nid, burst_type, log_info->burst_len, log_info->channel_id);

            INSERT_TO_HEAP(logep, mac80216j_NT_log_p2->PROTO, mac80216j_NT_log_p2->Time + ENDING, mac80216j_NT_log_p2);
#if LOG_DEBUG
            printf("RS%d %llu ~ %llu %d->%d (SuccessTX)\n", get_nid(), GetCurrentTime(), GetCurrentTime() + trans_time_in_tick, log_info->src_nid, log_info->dst_nid);
#endif
            ULburst->pkt_addinfo("loginfo", (char *) log_info, sizeof(struct LogInfo));
        }
        /************ LOG END *************/

        epkt            = createEvent();
        epkt->DataInfo_ = ULburst->copy();
        dist            = GetNodeDistance(get_nid(), wi->obj->get_nid());

        ((Packet *) epkt->DataInfo_)->pkt_addinfo("dist", (char *) &dist, sizeof(double));

        epkt->timeStamp_    = 0;
        epkt->perio_        = 0;
        epkt->calloutObj_   = wi->obj;
        epkt->memfun_       = NULL;
        epkt->func_         = NULL;
        NslObject::send(epkt);
    }
}

void OFDMA_PMPRS_NT::recvHandler_superordinate()
{
    struct PHYInfo *phyInfo     = NULL;
    struct LogInfo *src_log_info= NULL;
    struct wphyInfo *wphyinfo   = NULL;
    Packet *buffered_pkt_p      = NULL;
    Packet *pkt                 = NULL;
    ePacket_ *epkt              = NULL;
    char *output                = NULL;
    double *dist                = NULL;
    double SNR                  = 0.0;
    double avgSNR               = 0.0;
    double saveSNR              = 0.0;
    double BER                  = 0.0;
    int slots                   = 0;
    int fec                     = 0;
    int repe                    = 0;
    int len                     = 0;
    int burstLen                = 0;
    uint32_t src_nid            = 0;
    struct RzonePrefix_except_128 rzone_prefix;

    /* Get the buffered packet information */
    buffered_pkt_p  = (Packet *)_buffered_ePacket->DataInfo_;
    phyInfo         = (struct PHYInfo *) buffered_pkt_p->pkt_getinfo("phyInfo");
    dist            = (double *) buffered_pkt_p->pkt_getinfo("dist");
    src_log_info    = (struct LogInfo *) buffered_pkt_p->pkt_getinfo("loginfo");
    saveSNR         = phyInfo->SNR;

    //    printf("OFDMA_PMPRS_NT::%s() from Node[%d]\n",__func__,phyInfo->nid);
    wphyinfo = (struct wphyInfo *) buffered_pkt_p->pkt_getinfo("WPHY");

    if (wphyinfo == NULL)
    {
        printf("[%s] Error: no wphyinfo\n", __func__);
        exit(1);
    }

    /******* LOG START *******/
    int log_flag = (!strncasecmp(MR_WIMAX_NT_LogFlag, "on", 2)) ? 1 : 0;

    if (src_log_info == NULL)  /* Assertion for struct LogInfo structure */
    {
        if (log_flag == 1)
        {
            fprintf(stderr, "OFDMA_PMPRS_NT::%s(): source log information is lost.\n",__func__);
            exit(1);
        }
    }
    else
    {
        src_nid = src_log_info->src_nid;
    }

    uint64_t *start_recv_time = (uint64_t *) buffered_pkt_p->pkt_getinfo("srecv_t");

    if (start_recv_time == NULL)   /* Assertion for start_recv_time*/
    {
        fprintf(stderr, "OFDMA_PMPRS_NT::%s(): Frame receive time is lost.\n",__func__);
        exit(1);
    }

    struct LogInfo log_info;

    memset(&log_info, 0, sizeof(struct LogInfo));

    log_info.channel_id = _ChannelID;

    /****** LOG END ***********/

    uint8_t burst_type      = 0;
    char cm_obj_name[100]   = "";
    cm *cmobj               = NULL;

    sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
    cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
    if (cmobj == NULL)   /* Check channel model related information */
    {
        printf("[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
        exit(1);
    }

    if (recvState == Idle)   /* Unsafe state transformation */
    {
        fprintf(stderr, "\e[1;31mTime:%llu:RS(%d):Error! recvState is Idle.\e[0m\n", GetCurrentTime(), get_nid());
        exit(1);
    }
    else if (recvState == ProcRzonePrefix)    /* Process RzonePrefix and R-MAP */
    {
        //printf("Time:%llu RS ProcRzonePrefix\n",GetCurrentTime());
        /* Get the length of RzonePrefix */
        repe    = REPETATION_CODEWORD_4;
        fec     = QPSK_1_2;
        len     = _channelCoding->getCodedSlotLen(fec) * repeat_times_NT[repe];

        /* Make bit error and decode RzonePrefix */
        BER     = _channelModel->computeBER(fec, saveSNR);
        _channelModel->makeBitError(_burstPtr, len, BER);
        decodeRzonePrefix(_burstPtr, &rzone_prefix);
        _burstPtr += len;

        /* Get the length of R-MAP message */
        //repe        = dl_frame_prefix.repecode;
        repe        = REPETATION_CODEWORD_1;
        fec         = QPSK_1_2;
        len         = _channelCoding->getCodedSlotLen(fec) * (rzone_prefix.len_lsb + (rzone_prefix.len_msb<<2));
        burstLen    = _channelCoding->getUncodedBurstLen(len, fec);

        /********** LOG **************/
        burst_type = FRAMETYPE_MR_WIMAX_NT_PMP_RzonePrefix;
        log_info.burst_len = burstLen;
        /********** LOG **************/

        /* Allocate a new packet to deliver R-MAP message */
        pkt         = new Packet;
        assert(pkt);
        output      = pkt->pkt_sattach(burstLen);
        pkt->pkt_sprepend(output, burstLen);
        pkt->pkt_setflow(PF_RECV);

        /* Make bit error and decode R-MAP message */
        BER = _channelModel->computeBER(fec, saveSNR);
        _channelModel->makeBitError(_burstPtr, len, BER);
        slots = _channelCoding->getNumCodedSlotWithRepe(len, repeat_times_NT[repe], fec);
        _channelCoding->decode(_burstPtr, output, len, slots, repeat_times_NT[repe], fec);
        _burstPtr += len;

        /* We obtain avgSNR by averaging 300 samples. This facilitates the  */
        /* handover decision in MAC, otherwise MAC will probably receive    */
        /* some extreme values and make bad decision.                       */
        avgSNR = 0;
        for (int i = 1; i <= 100; i++) 
        {
            SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
            avgSNR  += ((SNR - avgSNR) / i);
        }

        /* Convey additional PHY information */
        phyInfo->SNR = avgSNR;
        pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

        /* Recover the original received SNR */
        phyInfo->SNR = saveSNR;

        /* Deliver the R-MAP message to upper layer */
        epkt = createEvent();
        epkt->DataInfo_ = pkt;
        put(epkt, recvtarget_);
    } 
    else if (recvState == ProcRelayIE)  /* Process the DL bursts depend on the received DL information element */
    {
        dlmapinfo *ieInfo = NULL;
        while (_DLMapInfo_relay.size())
        {
            /* Get an IE to compute the burst length */
            ieInfo      = _DLMapInfo_relay.front();
            repe        = ieInfo->repeCode;
            fec         = _DCDProfile[ieInfo->diuc].fec;
            len         = _channelCoding->getCodedSlotLen(fec) * (ieInfo->numSym * ieInfo->numCh / DL_PUSC);
            burstLen    = _channelCoding->getUncodedBurstLen(len, fec);

            /********** LOG START **************/
            burst_type = FRAMETYPE_MR_WIMAX_NT_PMP_DLRelayBURST;
            log_info.burst_len += burstLen;
            /********** LOG END **************/

            /* Allocate a new packet to deliver DL burst */
            pkt         = new Packet;
            assert(pkt);
            output      = pkt->pkt_sattach(burstLen);
            pkt->pkt_sprepend(output, burstLen);
            pkt->pkt_setflow(PF_RECV);

            /* Make bit error and decode burst */
            BER = _channelModel->computeBER(fec, saveSNR);
            _channelModel->makeBitError(_burstPtr, len, BER);
            slots = ieInfo->numSym * ieInfo->numCh / DL_PUSC;
            _channelCoding->decode(_burstPtr, output, len, slots, repeat_times_NT[repe], fec);
            _burstPtr += len;

            /* We obtain avgSNR by averaging 300 samples. This facilitates the  */
            /* profile selection in MAC, otherwise MAC will probably receive    */
            /* some extreme values and make bad selection.                      */
            avgSNR = 0;
            for (int i = 1; i <= 300; i++)
            {
                SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
                avgSNR  += ((SNR - avgSNR) / i);
            }

            /* Convey additional PHY information */
            phyInfo->SNR = avgSNR;
            pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

            /* Deliver the burst to upper layer */
            epkt = createEvent();
            epkt->DataInfo_ = pkt;
            put(epkt, recvtarget_);

            /* Remove a information element */
            delete *(_DLMapInfo_relay.begin());
            _DLMapInfo_relay.erase(_DLMapInfo_relay.begin());
        }

        /* End processing the DL burst, free memory and change state to Idle */
        //printf("Time:%llu RS[%d] End of processing DL burst , recvState = %d\n",GetCurrentTime(),get_nid(),recvState);
        recvState = Idle;

        freePacket(_buffered_ePacket);
        _buffered_ePacket = NULL;
        _burstPtr = NULL;
    }
    else    /* Unknown state transformation */
    {
        fprintf(stderr, "\e[1;31mTime:%llu:OFDMA_PMPRS_NT(%d):Unknown state\e[0m\n", GetCurrentTime(), get_nid());
        exit(1);
    }

    /********* LOG START *********/
    if (log_flag)
    {
        struct logEvent *logep      = NULL;
        double trans_time           = 0.0;
        uint64_t start_receive_time = 0;
        uint64_t trans_time_in_tick = 0;

        log_info.dst_nid = get_nid();

        if (burst_type == FRAMETYPE_MR_WIMAX_NT_PMP_RzonePrefix)
            trans_time = DL_PUSC * Ts;
        else
            trans_time= (_DLRelaySymbols - DL_PUSC) * Ts;

        MICRO_TO_TICK(trans_time_in_tick, trans_time);
        start_receive_time = GetCurrentTime() - trans_time_in_tick;

        /* log StartRX event */
        mac80216j_NT_log_t *mac80216j_NT_log_p1 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

        LOG_MAC_802_16j_NT(mac80216j_NT_log_p1, start_receive_time, start_receive_time, get_type(), get_nid(), StartRX,
                src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

        INSERT_TO_HEAP(logep, mac80216j_NT_log_p1->PROTO, mac80216j_NT_log_p1->Time + START, mac80216j_NT_log_p1);
#if LOG_DEBUG
        printf("RS%d %llu ~ %llu %d->%d (StartRX)\n", get_nid(), start_receive_time, start_receive_time, src_nid, log_info.dst_nid);
#endif
        /* log SuccessRX event */
        mac80216j_NT_log_t *mac80216j_NT_log_p2 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

        LOG_MAC_802_16j_NT(mac80216j_NT_log_p2, start_receive_time + trans_time_in_tick, start_receive_time, get_type(), get_nid(), SuccessRX,
                src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

        INSERT_TO_HEAP(logep, mac80216j_NT_log_p2->PROTO, mac80216j_NT_log_p2->Time + ENDING, mac80216j_NT_log_p2);
#if LOG_DEBUG
        printf("RS%d %llu ~ %llu %d->%d (SuccessRX)\n", get_nid(), start_receive_time, start_receive_time + trans_time_in_tick, src_nid, log_info.dst_nid);
#endif
    }
    /********* LOG END *********/
}

void OFDMA_PMPRS_NT::recvHandler_subordinate(ePacket_ *epkt)
{
    struct PHYInfo *phyInfo     = NULL;
    struct LogInfo *src_log_info= NULL;
    struct wphyInfo *wphyinfo   = NULL;
    Packet *pkt_p           = NULL;
    Packet *pkt                 = NULL;
    char *output                = NULL;
    double *dist                = NULL;
    double SNR                  = 0.0;
    double avgSNR               = 0.0;
    double saveSNR              = 0.0;
    double BER                  = 0.0;
    int slots                   = 0;
    int fec                     = 0;
    int repe                    = 0;
    uint32_t src_nid            = 0;

    int uncodedLen              = 0;
    int codedLen                = 0;
    ePacket_ *deliver_epkt      = NULL;
    char *input                 = NULL;

    //printf("OFDMA_PMPRS_NT::%s() from Node[%d]\n",__func__,phyInfo->nid);
    /* Get the buffered packet information */
    pkt_p  = (Packet *) epkt->DataInfo_;
    wphyinfo = (struct wphyInfo *) pkt_p->pkt_getinfo("WPHY");
    input = pkt_p->pkt_sget();
    phyInfo         = (struct PHYInfo *) pkt_p->pkt_getinfo("phyInfo");
    dist            = (double *) pkt_p->pkt_getinfo("dist");

    src_log_info    = (struct LogInfo *) pkt_p->pkt_getinfo("loginfo");
    saveSNR         = phyInfo->SNR;

    if (wphyinfo == NULL)
    {
        printf("[%s] Error: no wphyinfo\n", __func__);
        exit(1);
    }

    /******* LOG START *******/
    int log_flag = (!strncasecmp(MR_WIMAX_NT_LogFlag, "on", 2)) ? 1 : 0;

    if (src_log_info == NULL)  /* Assertion for struct LogInfo structure */
    {
        if (log_flag == 1)
        {
            fprintf(stderr, "OFDMA_PMPRS_MR::recvHandler_subordinate(): source log information is lost.\n");
            exit(1);
        }
    }
    else
    {
        src_nid = src_log_info->src_nid;
    }

    uint64_t *start_recv_time = (uint64_t *) pkt_p->pkt_getinfo("srecv_t");

    if (start_recv_time == NULL)   /* Assertion for start_recv_time*/
    {
        fprintf(stderr, "OFDMA_PMPRS_MR::recvHandler_subordinate(): Frame receive time is lost.\n");
        exit(1);
    }

    struct LogInfo log_info;

    memset(&log_info, 0, sizeof(struct LogInfo));

    log_info.channel_id = _ChannelID;

    /****** LOG END ***********/

    uint8_t burst_type      = 0;

    /* We obtain avgSNR by averaging 300 samples. This facilitates the  */
    /* profile selection in MAC, otherwise MAC will probably receive    */
    /* some extreme values and make bad selection.                      */
    char cm_obj_name[100]   = "";
    cm *cmobj               = NULL;
    sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
    cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
    if (cmobj == NULL)
    {
        fprintf(stderr, "[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
        exit(0);
    }

    if ( phyInfo->uiuc == CDMA_BWreq_Ranging )
    {
        fec = BPSK_;

        if (phyInfo->ulmap_ie.ie_12.rangMethod == 0)        // Initial or Handover Ranging over 2 symbols
        {
            uncodedLen = 2 * 18;
        }
        else if (phyInfo->ulmap_ie.ie_12.rangMethod == 1)
        {
            uncodedLen = 4 * 18; 
        }
        else if (phyInfo->ulmap_ie.ie_12.rangMethod == 2)
        {
            uncodedLen = 1 * 18; 
        }
        else
        {
            uncodedLen = 3 * 18; 
        }

        /* Allocate a new packet to deliver ranging code message */
        pkt     = new Packet;
        assert(pkt);
        output  = pkt->pkt_sattach(uncodedLen);
        pkt->pkt_sprepend(output, uncodedLen);
        pkt->pkt_setflow(PF_RECV);

        /********** LOG **************/
        burst_type = FRAMETYPE_MR_WIMAX_NT_PMP_ULAccessBURST;
        log_info.burst_len = uncodedLen;
        /********** LOG **************/

        /* Make bit error and copy to output */
        BER = _channelModel->computeBER(fec, phyInfo->SNR);
        _channelModel->makeBitError(input, uncodedLen, BER);
        memcpy(output, input, uncodedLen);

        /* We obtain avgSNR by averaging 300 samples. This facilitates the  */
        /* profile selection in MAC, otherwise MAC will probably receive    */
        /* some extreme values and make bad selection.                      */
        avgSNR = 0;
        for (int i = 1; i <= 300; i++)
        {
            SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
            avgSNR  += ((SNR - avgSNR) / i);
        }

        /* Convey additional PHY information */
        phyInfo->SNR = avgSNR;
        pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

        /* Deliver the ranging code message to upper layer */
        deliver_epkt            = createEvent();
        deliver_epkt->DataInfo_ = pkt;
        put(deliver_epkt, recvtarget_);

    }
    else
    {
        if (phyInfo->uiuc == CDMA_Alloc_IE)  /* Special message of CDMA allocation */
        {
            repe = phyInfo->ulmap_ie.ie_14.repeCode;
        }
        else
        {
            repe = phyInfo->ulmap_ie.ie_other.repeCode;
        }

        /* Compute the data length */
        fec         = phyInfo->fec;
        codedLen    = pkt_p->pkt_getlen();
        uncodedLen  = _channelCoding->getUncodedBurstLen(codedLen, fec);

        //##########  LOG START ###########
        burst_type = FRAMETYPE_MR_WIMAX_NT_PMP_ULAccessBURST;
        log_info.burst_len = uncodedLen;
        //##########  LOG END ###########

        /* Allocate a new packet */
        pkt         = new Packet;
        assert(pkt);
        output      = pkt->pkt_sattach(uncodedLen);
        pkt->pkt_sprepend(output, uncodedLen);
        pkt->pkt_setflow(PF_RECV);

        /* Make bit error and decode burst */
        BER = _channelModel->computeBER(fec, phyInfo->SNR);
        _channelModel->makeBitError(input, codedLen, BER);
        slots = _channelCoding->getNumCodedSlotWithRepe(codedLen, repeat_times_NT[repe], fec);
        _channelCoding->decode(input, output, codedLen, slots, repeat_times_NT[repe], fec);

        /* We obtain avgSNR by averaging 300 samples. This facilitates the  */
        /* profile selection in MAC, otherwise MAC will probably receive    */
        /* some extreme values and make bad selection.                      */
        char cm_obj_name[100]   = "";
        cm *cmobj               = NULL;

        sprintf(cm_obj_name, "Node%d_CM_LINK_%d", get_nid(), get_port());
        cmobj = (cm*)RegTable_.lookup_Instance(get_nid(), cm_obj_name);
        if (cmobj == NULL)
        {
            fprintf(stderr, "[%s] Node %d:: No CM this Instance!!\n\n", __func__, get_nid());
            exit(0);
        }

        avgSNR = 0;
        for (int i = 1;i <= 300; i++)
        {
            SNR     = _channelModel->powerToSNR(cmobj->computePr(wphyinfo));
            avgSNR  += ((SNR - avgSNR) / i);
        }

        /* Convey additional PHY information */
        phyInfo->SNR = avgSNR;
        pkt->pkt_addinfo("phyInfo", (char *) phyInfo, sizeof(struct PHYInfo));

        /* Deliver the burst to upper layer */
        deliver_epkt            = createEvent();
        deliver_epkt->DataInfo_ = pkt;
        put(deliver_epkt, recvtarget_);

    }

    /********* LOG START *********/
    if (log_flag)
    {
        struct logEvent *logep      = NULL;
        double trans_time           = 0.0;
        uint64_t start_recv_time = 0;
        uint64_t trans_time_in_tick = 0;

        log_info.dst_nid = get_nid();

        if(burst_type == FRAMETYPE_MR_WIMAX_NT_PMP_ULAccessBURST)
        {
            trans_time = _DLAccessSymbols * Ts;
        }
        else
        {
            if(phyInfo->ulmap_ie.ie_12.rangMethod == 0) 
            {
                trans_time = 2*Ts;
            }
            else if(phyInfo->ulmap_ie.ie_12.rangMethod == 1) 
            {
                trans_time = 4*Ts;
            }
            else if(phyInfo->ulmap_ie.ie_12.rangMethod == 2) 
            {
                trans_time = 1*Ts;
            }
            else
            {
                trans_time = 3*Ts;
            }

            MICRO_TO_TICK(trans_time_in_tick, trans_time);
            start_recv_time = GetCurrentTime() - trans_time_in_tick;

            /* log StartTX event */
            mac80216j_NT_log_t *mac80216j_NT_log_p1 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

            LOG_MAC_802_16j_NT(mac80216j_NT_log_p1, start_recv_time, start_recv_time, get_type(), get_nid(), StartRX,log_info.src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

            INSERT_TO_HEAP(logep, mac80216j_NT_log_p1->PROTO, mac80216j_NT_log_p1->Time + START, mac80216j_NT_log_p1);

#if LOG_DEBUG
            printf("RS: %llu ~ %llu %d->%d (StartRX)\n", start_recv_time, start_recv_time, log_info.src_nid, log_info.dst_nid);
#endif

            mac80216j_NT_log_t *mac80216j_NT_log_p2 = (mac80216j_NT_log_t *)malloc(sizeof(mac80216j_NT_log_t));

            LOG_MAC_802_16j_NT(mac80216j_NT_log_p2, start_recv_time + trans_time_in_tick, start_recv_time, get_type(), get_nid(), SuccessRX,log_info.src_nid, log_info.dst_nid, burst_type, log_info.burst_len, log_info.channel_id);

            INSERT_TO_HEAP(logep, mac80216j_NT_log_p2->PROTO, mac80216j_NT_log_p2->Time + START, mac80216j_NT_log_p2);

#if LOG_DEBUG
            printf("RS: %llu ~ %llu %d->%d (SuccessRX)\n", start_receive_time,start_recv_time + trans_time_in_tick, log_info.src_nid, log_info.dst_nid);
#endif
        }
    }

    //epkt->DataInfo_ = NULL;
    freePacket(epkt);
    //epkt = NULL;
}

void OFDMA_PMPRS_NT::decodeRzonePrefix(char *input, struct RzonePrefix_except_128 *rzone_prefix_p)
{
    char *tmp       = NULL;
    int check[8]    = {0, 0, 0, 0, 0, 0, 0, 0};
    int index       = 0;
    int max         = 0;
    int len         = 0;
    int slots       = 0;
    char dup_coded_rp[4][12];
    char uncoded_rp[8][3];

    /* Repetition decode to get the whole coded RzonePrefixs */
    for (int i = 0;i < 4;i++)
    {
        memcpy(dup_coded_rp[i], input + len, _channelCoding->getCodedSlotLen(QPSK_1_2));
        len += _channelCoding->getCodedSlotLen(QPSK_1_2);
    }

    /* Decode all of the RzonePrefixs */
    for (int i = 0;i < 4;i++)
    {
        len = sizeof(struct RzonePrefix_except_128);
        tmp = new char [len * 2];

        slots = _channelCoding->getNumCodedSlotWithRepe(12, 4, QPSK_1_2);
        _channelCoding->decode(dup_coded_rp[i], tmp, 12, slots, 4, QPSK_1_2);
        memcpy(uncoded_rp[index], tmp, len);
        memcpy(uncoded_rp[index + 1], tmp + len, len);
        index += 2;

        delete [] tmp;
    }

    /* Compare all of the RzonePrefixs */
    for (int i = 0;i < 8;i++)
    {
        for (int j = 0;j < 8;j++)
        {
            if (memcmp(uncoded_rp[i], uncoded_rp[j], 3) == 0)
            {
                check[i]++;
            }
        }
    }

    /* choose the most appropriate RzonePrefix */
    index = 0;
    for (int i = 0;i < 8;i++)
    {
        if (check[i] > max)
        {
            max   = check[i];
            index = i;
        }
    }

    /* Save the right RzonePrefix */
    memcpy(rzone_prefix_p, uncoded_rp[index], 3);
}

int OFDMA_PMPRS_NT::procRNGCode(Pkthdr *pkthdr)
{
    Packet *ulpkt           = NULL;
    char *output            = NULL;
    RangCodeInfo *codeInfo  = (RangCodeInfo *) pkthdr;
    struct PHYInfo phyInfo;


    /* It's not the ranging code message */
    if (pkthdr->type != 3)
    {
        return 0;
    }

    /* Allocate a new packet to send this ranging code message */
    ulpkt   = new Packet;
    assert(ulpkt);
    output  = ulpkt->pkt_sattach(codeInfo->codeLen);
    ulpkt->pkt_sprepend(output, codeInfo->codeLen);
    ulpkt->pkt_setflow(PF_RECV);
    memcpy(output, codeInfo->rangingCode, codeInfo->codeLen);

    /* Convey additional PHY information */
    memset(&phyInfo, 0, sizeof(struct PHYInfo));
    phyInfo.power       = _transPower;
    phyInfo.uiuc        = CDMA_BWreq_Ranging;
    phyInfo.ChannelID   = _ChannelID;
    phyInfo.chOffset    = codeInfo->rangChOffset;
    phyInfo.symOffset   = codeInfo->rangSymOffset;
    phyInfo.ulmap_ie.ie_12.rangMethod = codeInfo->rangMethod;
    phyInfo.RS_RNGREQ_flag = false;
    //    printf("Time:%llu OFDMA_PMPRS_NT::%s rangSymOffset=%d\n",GetCurrentTime(),__func__,codeInfo->rangSymOffset);
    phyInfo.relay_flag = true;
    phyInfo.nid         = get_nid();

    ulpkt->pkt_addinfo("phyInfo", (char *) &phyInfo, sizeof(struct PHYInfo));

    /* Convey log information */
    struct LogInfo log_info;
    memset(&log_info, 0, sizeof(struct LogInfo));
    log_info.src_nid = get_nid();
    log_info.dst_nid = 0;  /* this field will be filled in sendToBS() */
    log_info.burst_len = codeInfo->codeLen;
    log_info.channel_id = _ChannelID;

    /* Send the ranging code to BS */
    sendToBS(ulpkt, &log_info);

    /* Free memory */
    delete codeInfo;
    delete ulpkt;

    return 1;
}

int OFDMA_PMPRS_NT::procCmdFromMAC(Pkthdr *pkthdr)
{
    ntfyDLMAP *ntfyCmd = (ntfyDLMAP *) pkthdr;
    vector<dlmapinfo>::iterator iter;
    dlmapinfo *mapinfo = NULL;

    /* It's not the R-MAP meassage */
    if (pkthdr->type != 2)
    {
        return 0;
    }

    /* Save the information elements of R-MAP message */

    if(ntfyCmd->relay_ind == true)
    {
        while(!_DLMapInfo_relay.empty())
        {
            delete *(_DLMapInfo_relay.begin());
            _DLMapInfo_relay.erase(_DLMapInfo_relay.begin());
        }

        relay_flag = true;
        for (iter = ntfyCmd->info.begin(); iter != ntfyCmd->info.end();iter++)
        {
            mapinfo             = new dlmapinfo;
            mapinfo->diuc       = (*iter).diuc;
            mapinfo->symOff     = (*iter).symOff;
            mapinfo->chOff      = (*iter).chOff;
            mapinfo->numSym     = (*iter).numSym;
            mapinfo->numCh      = (*iter).numCh;
            mapinfo->repeCode   = (*iter).repeCode;

            _DLMapInfo_relay.push_back(mapinfo);
        }
    }
    else
    {
        ;
    }

    /* Free memory and change state to ProcIE */
    delete ntfyCmd;
    ntfyCmd   = NULL;


    /* Schedule a timer to handle the bursts.   */
    /* Here, we assume that the delay time      */
    /* is the duration of DL subframe.          */
    recvState = ProcRelayIE;
    uint64_t ticks = 0;
    MICRO_TO_TICK(ticks,( _DLRelaySymbols - DL_PUSC ) * Ts);
    scheduleTimer(_recvTimer, ( _DLRelaySymbols - DL_PUSC ) * Ts);
    return 1;
}

int OFDMA_PMPRS_NT::skipBufferedPkt()
{
    /* Clear the buffered packet and change state to Idle */
    recvState = Idle;
    freePacket(_buffered_ePacket);
    _buffered_ePacket = NULL;

    return 1;
}

void OFDMA_PMPRS_NT::saveDLMAP(char *dstbuf, int len)
{
    _dlmapBuf_NT = dstbuf;
    _dlmapLen_NT = len;
}

void OFDMA_PMPRS_NT::dump_buf(int st, int en)
{
    Packet *pkt = (Packet *)_buffered_ePacket->DataInfo_;

    for(int i = st;i < en;i++)
    {
        printf("[%3d]:%8X  ", i, pkt->pkt_sget()[i]);

        if (i % 7 == 0)
            printf("\n");
    }
    printf("\n");
}
