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

#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <assert.h>
#include <packet.h>
#include <timer.h>
#include <nctuns_api.h>
#include <sys/stat.h>
#include "radiolink.h"
#include <unistd.h>
#include <con_list.h>
#include <wphyInfo.h>
#include "radiolink.h"
#include <misc/obs/obstacle.h>
#include <gprs/include/burst_type.h>
#include <gprs/include/GPRS_rlcmac_message.h>

//#define __RL_CHANNEL_MISMATCH_DEBUG

MODULE_GENERATOR(radiolink);

extern SLIST_HEAD(headOfLink, con_list)          headOfGPRSRF_;

int remove_upper_layer_pkt(Packet* pkt);

radiolink::radiolink(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
        /* disable all flow control */
        r_flowctl = DISABLED;
        s_flowctl = DISABLED;

        /* assign propagation model and
         * modulation model
         */

        /* bind variable */
        vBind("Bw", &bw_Mbps);
        vBind("freq", &freq_);
        vBind("Noise", &Noise);
        vBind("TransPower",&TransPower);
        //vBind("Gain",&Gain);
        //vBind("CSOpt",&CSOpt);
        vBind("CSThresh",&CSThresh_);
        //vBind("CSRange",&CSRange);

        vBind("log", &_log);
        vBind("logInterval", &_logInterval);
        vBind("inOpt", &_inOpt);
        vBind("outOpt", &_outOpt);
        vBind("inoutOpt", &_inoutOpt);

        vBind("logInFileName", &logInFileName);
        vBind("logOutFileName", &logOutFileName);
        vBind("logInOutFileName", &logInOutFileName);

        vBind("linkfail", &_linkfail);
        vBind("linkfailFileName", &linkfailFileName);

        /* register variable */
        REG_VAR("BW", &bw_);
        REG_VAR("CHANNEL", &freq_);
        REG_VAR("CPThresh", &CPThresh_);

        //REG_VAR("Gain", &Gain);
	REG_VAR("FREQ", &freq_);
	REG_VAR("beamwidth", &beamwidth);
	REG_VAR("pointingDirection", &pointingDirection);
	REG_VAR("angularSpeed", &angularSpeed);
	/*
	 *  We have to register this variable, because GPRS uses different packet structure 
	 *  It will be used in cm.cc
	 */
	REG_VAR("uGPRS", &uGPRS);	

        /* initial variable */
	uGPRS = 1;
	beamwidth = 360;
	pointingDirection = 0;
	angularSpeed = 0;

        TransPower = 5.0;                 // dbm

        Noise =  10.0;
        //Gain = 1.0;
        //CSOpt = 1;

	freq_ = 900; //MHz
        CPThresh_ = 10.0;
        LinkFailFlag = 0;

        bw_Mbps = 11;
        inKb = outKb = inoutKb = 0;
        _in = _out = _inout = 0;

        _log = 0;
        _logInterval = 1;
        _inOpt = 0;
        _outOpt = 0;
        _inoutOpt = 0;

        logInFileName = NULL;
        logOutFileName = NULL;
        logInOutFileName = NULL;
}

radiolink::~radiolink() {

}

int radiolink::init() {
        NslObject::init();

        /* set bandwidth */
        bw_ = bw_Mbps * 1000000.0;
        TransPower = pow(10,TransPower / 10) * 1e-3; // Watt

        errorModel = new GMSK();
        assert(errorModel);

        char    *FILEPATH;

        if ( (_log)&&(!strcasecmp(_log, "on")) ) {
                if ( (_inOpt)&&(!strcasecmp(_inOpt, "on")) ) {
                        _in = 1;
                        FILEPATH = (char *)malloc(strlen(GetConfigFileDir())
                                        + strlen(logInFileName)+1);
                        sprintf(FILEPATH,"%s%s", GetConfigFileDir(),
                                        logInFileName);
                        logInFile = fopen(FILEPATH,"w+");
                        assert(logInFile);
                        free(FILEPATH);
                }

                if ( (_outOpt)&&(!strcasecmp(_outOpt, "on")) ) {
                        _out = 1;
                        FILEPATH = (char *)malloc(strlen(GetConfigFileDir())
                                        + strlen(logOutFileName)+1);
                        sprintf(FILEPATH,"%s%s", GetConfigFileDir(),
                                        logOutFileName);
                        logOutFile = fopen(FILEPATH,"w+");
                        assert(logOutFile);
                        free(FILEPATH);
                }

                if ( (_inoutOpt)&&(!strcasecmp(_inoutOpt, "on")) ) {
                        _inout = 1;
                        FILEPATH = (char *)malloc(strlen(GetConfigFileDir())
                                        + strlen(logInOutFileName)+1);
                        sprintf(FILEPATH,"%s%s", GetConfigFileDir(),
                                        logInOutFileName);
                        logInOutFile = fopen(FILEPATH,"w+");
                        assert(logInOutFile);
                        free(FILEPATH);
                }

                /* convert log interval to tick */
                MILLI_TO_TICK(_logIntervalTick, _logInterval);

                /* set timer to log information periodically */
                BASE_OBJTYPE(type);
                type = POINTER_TO_MEMBER(radiolink, log);
                logTimer.setCallOutObj(this, type);
                logTimer.start(_logIntervalTick, _logIntervalTick);
        }

        char            line[128];
        if( _linkfail && !strcmp(_linkfail, "on") ) {
                FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
                                                strlen(linkfailFileName)+1);
                sprintf(FILEPATH,"%s%s", GetConfigFileDir(), linkfailFileName);

                linkfailFile = fopen(FILEPATH,"r");

                if( linkfailFile == NULL ) {
                        printf("Warning : Can't read file %s\n", FILEPATH);
                }
                else {
                        double          StartTime, StopTime;
                        Event_          *start_ep;
                        Event_          *stop_ep;
                        u_int64_t       StartTimeTick, StopTimeTick;
                        BASE_OBJTYPE(typeStart);
                        BASE_OBJTYPE(typeStop);

                        typeStart = POINTER_TO_MEMBER(radiolink, TurnOnLinkFailFlag);
                        typeStop  = POINTER_TO_MEMBER(radiolink, TurnOffLinkFailFlag);

                        while( !feof(linkfailFile) ) {
                                line[0] = '\0';
                                fgets(line, 127, linkfailFile);
                                if ((line[0]=='\0')||(line[0]=='#'))
                                        continue;
                                if ( 2 == sscanf(line, "%lf %lf",
                                        &StartTime, &StopTime) ) {

                                        if( StartTime >= StopTime )
                                                continue;
                                        /* handle start evnet */
                                        SEC_TO_TICK(StartTimeTick, StartTime);
                                        start_ep =  createEvent();
                                        setObjEvent(start_ep,
                                                    StartTimeTick,
                                                    0,this,typeStart,
                                                    (void *)NULL);

                                        /* handle stop event */
                                        SEC_TO_TICK(StopTimeTick, StopTime);
                                        stop_ep =  createEvent();
                                        setObjEvent(stop_ep,
                                                    StopTimeTick,
                                                    0,this,typeStop,
                                                    (void *)NULL);
                                }
                        }
                }

		fclose(linkfailFile);
                free(FILEPATH);
        }

        if ( ObstacleFlag && !strcasecmp(ObstacleFlag, "on") ) {
                if ( !obstacleFileOpenFlag ) {
                        obstacleFileOpenFlag = true;

                        FILEPATH = (char *)malloc(strlen(GetScriptName())+5);
                        sprintf(FILEPATH, "%s.obs", GetScriptName());
                        obstacleFile = fopen(FILEPATH, "r");

                        if( obstacleFile == NULL ) {
                                printf("Warning : Can't read file %s\n", FILEPATH);
                        }
                        else {
                                Insert_obstacles(obstacleFile);
                        }

			fclose(obstacleFile);
                        free(FILEPATH);
                }
        }

        return(1);
}

int radiolink::send(ePacket_ *pkt) {

    struct wphyInfo*    wphyinfo;
    struct con_list*    wi;
    double              T_locX, T_locY, T_locZ;
    bss_message*        bs;
    ePacket_*           ep;

    assert(pkt&&(bs=(bss_message *)pkt->DataInfo_));

    if ( LinkFailFlag > 0 ) {

        pkt->DataInfo_ = NULL;

        /* Do not release packet carried in the packet field in radiolink::send(). */
        free_pkt(bs->packet);
        bs->packet = NULL;

        mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);
        if ( bs_mac_opt ) {

            if ( bs_mac_opt->msg ) {

                shared_obj_dec_refcnt( bs_mac_opt->msg_type , bs_mac_opt->msg );
                int res = shared_obj_release( bs_mac_opt->msg_type , bs_mac_opt->msg );
                if ( res ) {
                    bs_mac_opt->msg = 0;
                }

            }

            if ( (bs_mac_opt->burst_type == NORMAL_BURST) || (bs_mac_opt->burst_type == CTL_BURST) ) {
                NB* nb_p = reinterpret_cast<NB*> (bs->user_data);
                delete nb_p;
                bs->user_data = NULL;
                bs->user_data_len = 0;
            }
        }

        free_bss_msg_elem(bs);
        delete bs;

        FREE_EVENT(pkt);
        return(1);
    }

    /* add up the outgoing packet's length (throughput) */
    outKb += (double)bssall_len(bs) / 1000.0;
    inoutKb += (double)bssall_len(bs) / 1000.0;

    wphyinfo = (struct wphyInfo *)
        malloc(sizeof(struct wphyInfo));
    assert(wphyinfo);

    /* get my location */
    assert(GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ) > 0);

    /* fill phy-information
    * eg, sender location, frequency ...... etc,.
    */
    wphyinfo->nid      = get_nid();
    wphyinfo->pid      = get_port();
    wphyinfo->bw_      = bw_;
    wphyinfo->channel_ = ((mac_option*)bs->mac_option)->channel_id;
    wphyinfo->TxPr_    = TransPower; // Watt
    wphyinfo->RxPr_    = 0.0;
    wphyinfo->srcX_    = T_locX;
    wphyinfo->srcY_    = T_locY;
    wphyinfo->srcZ_    = T_locZ;
    wphyinfo->currAzimuthAngle_ = 0;
    wphyinfo->Pr_      = 0.0;

    bs->wphyinfo = wphyinfo;
    //bs->flag = PF_RECV;
    bs->flag = PF_SEND;

    bss_message* nbs;

    /* Duplicate this frame (N-1) times, where N is the number of
     * GPRS_RadioLink modules. The sucesses of frame deliveries are
     * determined by the radiolink module in each GPRS node that receives
     * this packet.
     */

    SLIST_FOREACH(wi, &headOfGPRSRF_, nextLoc) {

        /* if myself, just skip it */
        if( get_nid() == wi->obj->get_nid() &&
            get_port() == wi->obj->get_port() )
            continue;

        nbs = (bss_message*) malloc(sizeof(bss_message));
        bzero(nbs,sizeof(bss_message));
        //nbs->flag = PF_RECV;
        nbs->flag = PF_SEND;
        copy_bss_msg_options(nbs,bs);

        {
            mac_option* src_macopt = reinterpret_cast<mac_option*> (bs->mac_option);
            if ( src_macopt ) {
                if ( src_macopt->msg ) {
                    insert_macopt_msg( (mac_option*)(nbs->mac_option) , src_macopt->msg_type , src_macopt->msg );
                    shared_obj_inc_refcnt( src_macopt->msg_type , src_macopt->msg );
                }
            }

            /* copying user_data should depend on the type of burst_type indicated in mac option. */
            if ( src_macopt->burst_type == NORMAL_BURST || src_macopt->burst_type == CTL_BURST ) {
                NormalBurst* nb1         = new NormalBurst( (uchar*)(1) , 0 , 0 , NORMAL_BURST);
                NormalBurst* nb          = reinterpret_cast<NB*> (bs->user_data);
                nb1->copy_nb(nb);
                insert_user_data(nbs, nb1 , sizeof(NormalBurst) );
            }
            else if ( src_macopt->burst_type == DUMMY_BURST ) {
                nbs->user_data      = bs->user_data;
                nbs->user_data_len  = bs->user_data_len;
            }
            else
                copy_bss_msg_userdata(nbs,bs);

        }
        copy_bss_msg_headers(nbs,bs);

        //if(bs->packet) nbs->packet = bs->packet->pkt_copy();
        if ( bs->packet ) copy_bss_msg_packet_field(nbs,bs);

        /*  duplicate packet */
        ep               = createEvent();
        ep->created_nid_ = wi->obj->get_nid();
        ep->timeStamp_   = pkt->timeStamp_;
        ep->perio_       = pkt->perio_;
        ep->priority_    = pkt->priority_;
        ep->calloutObj_  = pkt->calloutObj_;
        ep->func_        = pkt->func_;
        ep->DataInfo_    = nbs;
        ep->flag         = pkt->flag;

        //#define __EVENT_REC_DEBUG
        #ifdef __EVENT_REC_DEBUG
        if ( GetCurrentTime() >= 20000000 ) {
            int rec_res = rec_created_event(ep);
            if ( rec_res<0) {
                printf("radiolink::send(): record ep_rec failed.\n");
            }
        }
        #endif

        /* simulate propagation delay */
	ep->timeStamp_ = 0;
	ep->perio_ = 0;
	ep->calloutObj_ = wi->obj;
	ep->memfun_ = NULL;
	ep->func_ = NULL;
	NslObject::send(ep);
    }

    /* added by jclin on 10/02/2004 */
    {
        bss_message* pkt_bs_tmp = reinterpret_cast<bss_message*> (pkt->DataInfo_);
        if ( pkt_bs_tmp ) {

            if ( pkt_bs_tmp->user_data ) {

                if ( pkt_bs_tmp->user_data_len > 0 ) {

                    mac_option* macopt = (mac_option*)( pkt_bs_tmp->mac_option);
                    if ( (macopt->burst_type == NORMAL_BURST) || (macopt->burst_type == CTL_BURST) ) {
                        NB* nb_p = reinterpret_cast<NB*> (pkt_bs_tmp->user_data);
                        delete nb_p;
                        pkt_bs_tmp->user_data = NULL;
                        pkt_bs_tmp->user_data_len = 0;
                    }
                }
                else {
                    printf("radiolink::send(): Assertion failed: user_data field is not null with zero length");
                    exit(1);
                }
            }

            if ( pkt_bs_tmp->packet ) {
                free_pkt(pkt_bs_tmp->packet);
                pkt_bs_tmp->packet = NULL;
            }
        }
    }

    /* Release the event given as the argument. Events standing for bursts
     * need not be backuped.
     */

    FREE_BSS_EVENT( pkt );

    return(1);
}

int radiolink::recv(ePacket_ *pkt) {

    bss_message*        bs;
    double              Pr;
    struct wphyInfo*    wphyinfo;

    assert(pkt&&(bs=(bss_message *)pkt->DataInfo_));

    if ( LinkFailFlag > 0 ) {

        /* In the case of link failure. */
        //pkt->DataInfo_ = bs->packet;
        pkt->DataInfo_ = NULL;

        /* added by jclin on 10/02/2004 */

        if ( bs->packet ) {
            remove_upper_layer_pkt(bs->packet);
            free_pkt(bs->packet);
            bs->packet     = NULL;
        }
        FREE_EVENT(pkt);

        /* added by jclin on 10/02/2004 */
        {
            bss_message* pkt_bs_tmp = bs;
            if ( pkt_bs_tmp ) {

                if ( pkt_bs_tmp->user_data ) {

                    if ( pkt_bs_tmp->user_data_len > 0 ) {
                        mac_option* macopt = (mac_option*)( pkt_bs_tmp->mac_option);
                        if ( (macopt->burst_type == NORMAL_BURST) || (macopt->burst_type == CTL_BURST) ) {
                            NB* nb_p = reinterpret_cast<NB*> (pkt_bs_tmp->user_data);
                            delete nb_p;
                            pkt_bs_tmp->user_data = NULL;
                            pkt_bs_tmp->user_data_len = 0;
                        }
                    }
                    else {
                        printf("radiolink::recv(): Assertion failed: user_data field is not null with zero length");
                        exit(1);
                    }
                }
            }
        }

        mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);

        if ( bs_mac_opt ) {

            if ( bs_mac_opt->msg ) {

                shared_obj_dec_refcnt( bs_mac_opt->msg_type , bs_mac_opt->msg );

                int res = shared_obj_release( bs_mac_opt->msg_type , bs_mac_opt->msg );

                if ( res ) {
                    bs_mac_opt->msg = 0;
                }
            }
        }

        free_bss_msg_elem(bs);
        delete bs;
        return(1);
    }

    wphyinfo = bs->wphyinfo;

    if (!wphyinfo) {

        printf("radionlink::recv(): wphyinfo is null.\n");
        exit(1);

        //pkt->DataInfo_ = bs->packet;
        pkt->DataInfo_ = NULL;

        /* added by jclin */
        if ( bs->packet ) {
            remove_upper_layer_pkt(bs->packet);
            free_pkt(bs->packet);
            bs->packet     = NULL;
        }
        FREE_EVENT(pkt);

        /* added by jclin on 10/02/2004 */
        {
            bss_message* pkt_bs_tmp = bs;
            if ( pkt_bs_tmp ) {

                if ( pkt_bs_tmp->user_data ) {

                    if ( pkt_bs_tmp->user_data_len > 0 ) {
                        mac_option* macopt = (mac_option*)( pkt_bs_tmp->mac_option);
                        if ( (macopt->burst_type == NORMAL_BURST) || (macopt->burst_type == CTL_BURST) ) {
                            NB* nb_p = reinterpret_cast<NB*> (pkt_bs_tmp->user_data);
                            delete nb_p;
                            pkt_bs_tmp->user_data = NULL;
                            pkt_bs_tmp->user_data_len = 0;
                        }
                    }
                    else {
                        printf("radiolink::recv(): Assertion failed: user_data field is not null with zero length");
                        exit(1);
                    }
                }
            }
        }

        mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);

        if ( bs_mac_opt ) {
            if ( bs_mac_opt->msg ) {

                shared_obj_dec_refcnt( bs_mac_opt->msg_type , bs_mac_opt->msg );
                int res = shared_obj_release( bs_mac_opt->msg_type , bs_mac_opt->msg );
                if ( res ) {
                    bs_mac_opt->msg = 0;
                }
            }
        }

        free_bss_msg_elem(bs);
        delete bs;
        return(1);
    }

    /*
     * Check channel, if the channel of incoming packet is
     * not equal to channel of receiver, we should drop it
     * to simulate the rf can't listen this signal.
     */

    if (channels[wphyinfo->channel_] == 0) {

        /* modified by jclin on 03/24/2004 */
        #ifdef __RL_CHANNEL_MISMATCH_DEBUG
            printf("RL(nid %u): Drop Packet due to frequency channel mismatching, wphyinfo_channel_ = %d \n",
                get_nid(), wphyinfo->channel_);
        #endif

        #ifdef __RADIOLINK_DETAILED_DEBUG
        {
            mac_option* mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);
            printf("Radiolink: burst_type = %d mac_opt->channel_id = %ld \n",mac_opt->burst_type,mac_opt->channel_id);
        }
        #endif

        /* added by jclin on 10/02/2004 */
        pkt->DataInfo_ = NULL;
        if ( bs->packet ) {
            remove_upper_layer_pkt(bs->packet);
            free_pkt(bs->packet);
            bs->packet     = NULL;
        }

        FREE_EVENT(pkt);

        {
            bss_message* pkt_bs_tmp = bs;
            if ( pkt_bs_tmp ) {

                if ( pkt_bs_tmp->user_data ) {

                    if ( pkt_bs_tmp->user_data_len > 0 ) {
                        mac_option* macopt = (mac_option*)( pkt_bs_tmp->mac_option);
                        if ( (macopt->burst_type == NORMAL_BURST) || (macopt->burst_type == CTL_BURST) ) {
                            NB* nb_p = reinterpret_cast<NB*> (pkt_bs_tmp->user_data);
                            delete nb_p;
                            pkt_bs_tmp->user_data = NULL;
                            pkt_bs_tmp->user_data_len = 0;
                        }
                    }
                    else {
                        printf("radiolink::recv(): Assertion failed: user_data field is not null with zero length");
                        exit(1);
                    }
                }
            }
        }

        mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);
        if ( bs_mac_opt ) {
            if ( bs_mac_opt->msg ) {

                shared_obj_dec_refcnt( bs_mac_opt->msg_type , bs_mac_opt->msg );
                int res = shared_obj_release( bs_mac_opt->msg_type , bs_mac_opt->msg );
                if ( res ) {
                    bs_mac_opt->msg = 0;
                }

            }
        }

        free_bss_msg_elem(bs);
        delete bs;
        return(1);
    }

    /*
     * Check obstacles. If there is at least an obstacle located
     * between the source node and destination node, just drop
     * this packet.
     */

    if ( ObstacleFlag && !strcasecmp(ObstacleFlag, "on") ) {

        if ( Check_obstacles(wphyinfo->nid, get_nid()) > 0 ) {

            /* added by jclin on 10/02/2004 */
            pkt->DataInfo_ = NULL;
            if ( bs->packet ) {
                remove_upper_layer_pkt(bs->packet);
                free_pkt(bs->packet);
                bs->packet     = NULL;
            }
            FREE_EVENT(pkt);

            {
                bss_message* pkt_bs_tmp = bs;
                if ( pkt_bs_tmp ) {

                    if ( pkt_bs_tmp->user_data ) {

                        if ( pkt_bs_tmp->user_data_len > 0 ) {
                            mac_option* macopt = (mac_option*)( pkt_bs_tmp->mac_option);
                            if ( (macopt->burst_type == NORMAL_BURST) || (macopt->burst_type == CTL_BURST) ) {
                                NB* nb_p = reinterpret_cast<NB*> (pkt_bs_tmp->user_data);
                                delete nb_p;
                                pkt_bs_tmp->user_data = NULL;
                                pkt_bs_tmp->user_data_len = 0;
                            }
                        }
                        else {
                            printf("radiolink::recv(): Assertion failed: user_data field is not null with zero length");
                            exit(1);
                        }
                    }
                }
            }

            mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);

            if ( bs_mac_opt ) {
                if ( bs_mac_opt->msg ) {

                    shared_obj_dec_refcnt( bs_mac_opt->msg_type , bs_mac_opt->msg );
                    int res = shared_obj_release( bs_mac_opt->msg_type , bs_mac_opt->msg );
                    if ( res ) {
                        bs_mac_opt->msg = 0;
                    }

                }
            }

            free_bss_msg_elem(bs);
            delete bs;
            return(1);
        }
    }

    /* 
     * Otherwise, we should process this packet.
     * Calculate power level, BER of the packet.
     */
    Pr = wphyinfo->Pr_; // dbm

    if (bs->user_data_len == 0) 
        return NslObject::recv(pkt);

    mac_option* bs_mac_opt = reinterpret_cast<mac_option*> (bs->mac_option);
    if ( !bs_mac_opt ) {

	    printf("no mac_option field is found in a GPRS BSS packet.\n");
            assert(0);

    }

    /* set Modulation args */

    errorModel->Noise = Noise;

    /* packet error Model */

    /* cclin: the error model is only applied to only normal bursts */
    if (bs_mac_opt->burst_type == NORMAL_BURST ) {

	    /* add up the incoming packets' length (throughput) */
	    u_int32_t data_len = 14; /* 14 bytes for a normal burst payload */
	    inKb += (double)data_len / 1000.0;
	    inoutKb += (double)data_len / 1000.0;

	    double ber = errorModel->ProbBitError(errorModel->PowerToENR(Pr));
	    double perror;
	    
	    /* ensures that the range of BER is between 0 and 1 */
	    if (ber <0.0)
	        ber = 0.0;
	    else if (ber >1.0)
	        ber = 1.0;
	    else
		;

	    if (ber == 1)
		    perror = data_len*8;
	    else
		    perror = (ber>0) ? (ber*data_len*8) : 0;

	    NB* nb_p = reinterpret_cast<NB*> (bs->user_data);
	    assert(nb_p);

	    char* buf = reinterpret_cast<char*>(nb_p->get_data_ptr1());

	    while (perror >= 1){
		    buf[rand()%data_len] ^= 1;
		    perror--;
	    }
    }
    else {

	    /* add up the incoming packets' length (throughput) */
	    inKb += (double)bs->user_data_len / 1000.0;
	    inoutKb += (double)bs->user_data_len / 1000.0;
    }

    /*
     * Bring receving power calculated in
     * WPHY module to MAC module.
     */

    wphyinfo->RxPr_ = pow(10, Pr/10) * 1e-3; // Watt

    /* Record the received signal strength indicator (RSSI)
     * of the incoming packet.
     */

    /* push this packet to upper module */

    return(put(pkt, recvtarget_));
}

int radiolink::log() {
        double  intervalTmp;

        inKb /= (_logInterval / 1000.0);
        outKb /= (_logInterval / 1000.0);
        inoutKb /= (_logInterval / 1000.0);

        intervalTmp = GetCurrentTime() * TICK / 1000000000.0;

        if ( _in ) {
                fprintf(logInFile, "%.3f\t%.3f\n", intervalTmp, inKb);
                inKb = 0;
        }

        if ( _out ) {
                fprintf(logOutFile, "%.3f\t%.3f\n", intervalTmp, outKb);
                outKb = 0;
        }

        if ( _inout ) {
                fprintf(logInOutFile, "%.3f\t%.3f\n", intervalTmp, inoutKb);
                inoutKb = 0;
        }

        return(1);
}

int radiolink::command(int argc, const char *argv[]) {
        return(NslObject::command(argc, argv));
}

void radiolink::TurnOnLinkFailFlag(Event_ *ep){
        LinkFailFlag++;
}

void radiolink::TurnOffLinkFailFlag(Event_ *ep){
        LinkFailFlag--;
}

int radiolink::set_listen_channel(u_char* listen_chmap) {
        /* range definition:
         * 0-124  : uplink channel,
         * 125-249: downlink channel.
         *
         * the value of this map:
         * 0: unset, and radiolink shall not listen to this channel.
         * 1: set, and radiolink shall listen to this channel.             */
        bcopy(listen_chmap,channels,sizeof(char[250]));
        return 1;
}
