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

/* This file contains the implementation of class Rlc.
* RLC acknowledge mode is first implemented because
* in our simulator, LLC works under unacknowledge mode.
* And RLC/MAC provides a reliable radio link between
* MS and BSS.
*
* CreateDate:  08/12/2003
* Author:       Chih-che Lin
* Email:        jclin@csie.nctu.edu.tw
*/

#include "rlc.h"
#include <stdlib.h>
#include <sys/types.h>
#include <packet.h>
#include <event.h>
#include <timer.h>
#include <nctuns_api.h>
#include <gprs/llc/llc.h>
#include <gprs/include/GPRS_rlcmac_message.h>
#include <gprs/include/channel_coding.h>
#include <gprs/SndcpGmm/GPRS_mm_message.h>
#include <gprs/mac/GPRS_bts_mac.h>
#include <gprs/mac/mac_shared_def.h>
#include <gprs/rlc/rlc_shared_def.h>
#include "tcb.h"

using namespace std;
//#define __TCB_DUMP
#define __DECODE_CORRECT
//#define __DATA_SENT_DEBUG
//#define __ACK_DEBUG

        MODULE_GENERATOR(MsRlc);
        MODULE_GENERATOR(BtsRlc);


        #ifdef __BTSRLC_SGSNLLC_DIRECT_CONNECTED__
        /* The piece of source codes is used to directly connect RLC in BTS and
         * LLC in SGSN if necessary.
         */

        int Rlc::ori_recv(Event* ep) {
            if ( node_type == gprs::bts ) {

                NslObject* sgsn_llc = InstanceLookup( 3 , 1, "LLC");
                if ( !sgsn_llc ) {
                    cout << "sgsn_llc is NULL in directly-connected mode." << endl;
                    exit(1);
                }
                else {
                    return ( sgsn_llc->recv(ep) );
                }
            }
            else
                return NslObject::recv(ep);
        }

        #endif

        /**** RLC module ****/
        Rlc::Rlc(u_int32_t type, u_int32_t id, struct plist* pl, const char *name) : NslObject(type, id, pl, name) {
            /* internal variables */
            state             = IDLE_STATE;
            window_size       = GPRS_SNS;
            retry_cnt         = 0; /* used for uplink tbf */
            max_retry_num     = 10;
            user_specified_cs = 2;

            temp_frame_buf    = new SList<Event>;

            vBind("channel_coding_scheme", &user_specified_cs);
        }

        Rlc::~Rlc() {

            if (temp_frame_buf)
                delete temp_frame_buf;
        }

        MsRlc::MsRlc (u_int32_t type, u_int32_t id, struct plist* pl, const char *name) : Rlc(type, id, pl, name) {
            node_type       = gprs::ms ; /* default value, and it may changed in init() */
            utbfmap         = new UTbfMap;
            dtbfmap         = new DTbfMap;
            cout << "MsRlc MsRlc(): Construction OK." << endl;
        }

        MsRlc::~MsRlc() {
            if (utbfmap) delete utbfmap;
            if (dtbfmap) delete dtbfmap;
        }

        BtsRlc::BtsRlc (u_int32_t type, u_int32_t id, struct plist* pl, const char *name) : Rlc(type, id, pl, name) {

            node_type       = gprs::bts ; /* default value, and it may changed in init() */

            for (int i=0 ; i<125 ; ++i) {
                utbfmap[i]         = new UTbfMap;
                dtbfmap[i]         = new DTbfMap;
            }

        }

        BtsRlc::~BtsRlc() {
            for (int i=0 ; i<125 ;++i) {
                if (utbfmap[i]) delete utbfmap[i];
                if (dtbfmap[i]) delete dtbfmap[i];
            }
        }

        int MsRlc::send(Event* e_p) {

            IS_NULL_STR(e_p, "MsRlc send(): ep is null \n",0);
            IS_NULL_STR(e_p, "MsRlc send(): ep->DataInfo_ is null \n" ,0);

	    /* create a local copy for the sending pkt */
	    //Event* new_ep = copy_gprs_pkt(e_p);
	    //destroy_gprs_pkt(e_p); 
	    //e_p = new_ep;

            /* 1. Check the tlli provided by LLC layer */
            bss_message* bssmsg     = reinterpret_cast<bss_message*> (e_p->DataInfo_);
            llc_option*  llc_opt    = reinterpret_cast<llc_option*> (bssmsg->llc_option);
            rlc_option*  rlc_opt    = reinterpret_cast<rlc_option*>(bssmsg->rlc_option);

            IS_NULL_STR(llc_opt, "MsRlc send(): llc_opt is null \n" , 0);

            UTCB* utcb = NULL;
            DTCB* dtcb = NULL;

            /* Close the connection. */
            if ( llc_opt->addtional_info == LLGMM_RESET ) {

                UTCB* utcb = utbfmap->search_by_tlli(llc_opt->tlli);
                DTCB* dtcb = dtbfmap->search_by_tlli(llc_opt->tlli);

                uchar remove_flag = false;
                if (utcb) {

                    utcb->llgmm_reset_flag = true;

                    //if ( utcb->detach_flag ) {
                    if ( true ) {
                        utbfmap->remove( llc_opt->tlli );
                        remove_flag = true;
                    }

                }
                if ( dtcb ) {

                    dtcb->llgmm_reset_flag = true;

                    //if ( dtcb->detach_flag ) {

                    if ( true ) {
                        dtbfmap->remove( llc_opt->tlli );
                        remove_flag = true;
                    }

                }

                /* Transfer this command down to mac layer. Since this is LLC command, it's not
                 * proper to use command_mac() function to conduct MAC layer.
                 */
                NslObject::send( e_p );

                if ( remove_flag )
                    cout << "MsRlc send(): remove TCBs with tlli = " << llc_opt->tlli << endl;

                return 1;
            }

            if ( (llc_opt->req_ch_no>=0) ) {

                create_rlc_option(bssmsg);
                rlc_option* rlc_opt = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
                rlc_opt->cmd = GMM_ROAMING_REQUEST;
                return NslObject::send(e_p);
            }

            /* if llc_opt->cmd == LLGMM_ASSIGN, the tlli of the corresponding TCB should be updated */

            dtcb = dtbfmap->search_by_tlli(llc_opt->oldtlli);
            if (dtcb) {
                dtbfmap->update_tlli(dtcb,llc_opt->tlli);
            }


            utcb = utbfmap->search_by_tlli(llc_opt->oldtlli);
            if (utcb)
                utbfmap->update_tlli(utcb,llc_opt->tlli);


            /* check whether this event carries paging response */
            if (rlc_opt) {
                if ( rlc_opt->cmd == PAGING_RESPONSE ) {
                    //temp_frame_buf->insert_tail(e_p);
                    //command_mac(PAGING_RESPONSE,gmm_opt->imsi, llc_opt->tlli , rlc_opt->tfi , -1 , NULL);
                    return NslObject::send(e_p);
                }
            }


            /* 2. Check whether an uplink TBF is established.
            *    If so, go to step 4.
            *    Else, drive MAC layer to initialize the procedure for TBF establishment, turn
            *          RLC's state to PACKET_TRANSFER and go to step 3.
            */

            if ( !(utbfmap->utcb_list->get_num_of_tbfs()) ) {

                if (retry_cnt < max_retry_num ) {

                    temp_frame_buf->insert_tail(e_p);
		    printf("rlc::send(): insert into temp_frame_buf \n");
                    Rlc::command_mac(UPLINK_TBF_ESTABLISH, ulong(0), llc_opt->oldtlli, llc_opt->tlli,255, 0 , NULL);
                    return 1;
                
		}
                else {
                
		    printf("The number of requests to establish a TBF exceeds the maximum of retries\n");
                    return 0;
                
		}
            }
            else {

                /* A/Gb mode only supports single TBF at a time */
                ListElem1<UTCB>* elem = utbfmap->utcb_list->get1();
                UTCB* utcb = NULL;
                if (elem)
                    utcb = elem->get_elem();

                if ( utcb ) 
                    return (utcb->send(e_p));
                else
                    return 0;
            }
        }

        int BtsRlc::send(Event* ep) {

            IS_NULL_STR(ep, "BtsRlc send(): ep is null" ,0);
            IS_NULL_STR(ep->DataInfo_, "BtsRlc send(): ep->DataInfo_ is null" ,0);

	    //printf("BtsRlc[%u]: send a pkt. \n", get_nid());

#if 0
	    /* create a local copy for the sending pkt */
	    Event* new_ep = copy_gprs_pkt(ep);
            bss_message* bmsg_p   = reinterpret_cast<bss_message*>(ep->DataInfo_);
	    bmsg_p->user_data = 0;
	    bmsg_p->user_data_len = 0;
	    destroy_gprs_pkt(ep);
	    ep = new_ep;
            /****/
#endif

            bss_message* bmsg   = reinterpret_cast<bss_message*>(ep->DataInfo_);

	    /****/
#if 0
	    if (bmsg->packet) {
                bmsg->user_data     = (char*)((bmsg->packet)->pkt_sget());
	        bmsg->user_data_len = (bmsg->packet)->pkt_getlen();
	    }
	    else {
                bmsg->user_data     = 0;
	        bmsg->user_data_len = 0;
	    }
            /****/
#endif
            llc_option* llc_opt = reinterpret_cast<llc_option*> (bmsg->llc_option);
            IS_NULL_STR(llc_opt,"BtsRlc send(): bssmsg->llc_opt is null\n",0);

            DTCB* dtcb = NULL;
            UTCB* utcb = NULL;

            /* Close the connection. */
            if ( llc_opt->addtional_info == LLGMM_RESET ) {

                /* Transfer this command down to mac layer. Since this is LLC command, it's not
                * proper to use command_mac() function to conduct MAC layer.
                */
                ulong removed_tlli = llc_opt->tlli;
                NslObject::send( ep );

                uchar remove_flag = false;

                for ( int i = 0 ; i<125 ; ++i ) {

                    UTCB* marked_utcb = utbfmap[i]->search_by_tlli( removed_tlli );

                    if ( marked_utcb ) {
                        marked_utcb->llgmm_reset_flag = true;

                        if ( marked_utcb->detach_flag ) {
                            utbfmap[i]->remove( removed_tlli );
                            remove_flag = true;
                        }
                    }

                    DTCB* marked_dtcb = dtbfmap[i]->search_by_tlli( removed_tlli );

                    if ( marked_dtcb ) {
                        marked_dtcb->llgmm_reset_flag = true;

                        if ( marked_dtcb->detach_flag ) {
                            dtbfmap[i]->remove( removed_tlli );
                            remove_flag = true;
                        }
                    }
                }

                if ( remove_flag )
                    cout << "BtsRlc send(): remove TCBs with tlli = " << removed_tlli << endl;

                return 1;
            }
            else if ( llc_opt->addtional_info == LLGMM_DETACH ) {

                ;/* corresponding actions are left in TCB::partition_upper_layer_pdu(); */

            }
            else
                ;


            gmm_option* gmm_opt = reinterpret_cast<gmm_option*> (bmsg->sndcp_option);
            IS_NULL_STR(gmm_opt,"BtsRlc send(): bssmsg->sndcp_option is null\n",0);


            /* if llc_opt->cmd == LLGMM_ASSIGN, the tlli of the corresponding TCB should be updated */

            for (int i=0 ; i<125 ;++i) {
                dtcb = dtbfmap[i]->search_by_tlli(llc_opt->oldtlli);
                if (dtcb) {
                    dtbfmap[i]->update_tlli(dtcb,llc_opt->tlli);
                }
            }
            for (int i=0 ; i<125 ;++i) {
                utcb = utbfmap[i]->search_by_tlli(llc_opt->oldtlli);
                if (utcb)
                    utbfmap[i]->update_tlli(utcb,llc_opt->tlli);
            }

            /* look for the corresponding TCB */
            dtcb = NULL;
            for (int i=0 ; i<125 ;++i) {
                dtcb = dtbfmap[i]->search_by_tlli(llc_opt->tlli);
                if (dtcb)
                    break;
            }

            utcb = NULL;
            for (int i=0 ; i<125 ;++i) {
                utcb = utbfmap[i]->search_by_tlli(llc_opt->tlli);
                if (utcb)
                    break;
            }

            if ( dtcb ) 
                return (dtcb->send(ep));
            
            else if ( utcb ) {

                /* In this case, MS is in Transfer state and has uplink TBF. The Packet Downlink
                 * Assignment message can be distributed over PACCH of the uplink TBF allocated
                 * to MS.
                 */


                Rlc::command_mac(DOWNLINK_TBF_ESTABLISH , gmm_opt->imsi ,
                    llc_opt->oldtlli, llc_opt->tlli , utcb->tfi, CORRESPONDING_DOWNLINK_CH(utcb->uplink_freq) , NULL);

		/* check if the length of temp_frame_buf is too large.
		 * If so, the BTS rlc layer should discard the packet.
		 */

#if 0
		int tf_buf_len = temp_frame_buf->get_list_num();
	        if (tf_buf_len >= 50 ) {
                    
		    destroy_gprs_pkt(ep);

		}
                else {	
                    temp_frame_buf->insert_tail(ep);
		    printf("BTSrlc::send(): insert into temp_frame_buf \n");
		}
#endif
	
                temp_frame_buf->insert_tail(ep);
		printf("BTSrlc::send(): insert into temp_frame_buf \n");
                return 1;
            
	    }
            else {
               
	        /* In this case, MS is in Idle state( may be corresponding to the standby state defined by
                 * GMM ). The sequece of exchanged messages for paging procedure is elaborated in the figure.
                 */
               

		/* check if the length of temp_frame_buf is too large.
		 * If so, the BTS rlc layer should discard the packet.
		 */

                command_mac(SEND_PAGING_REQUEST,gmm_opt->imsi, llc_opt->oldtlli, llc_opt->tlli,255, 0 , NULL);

#if 0
		int tf_buf_len = temp_frame_buf->get_list_num();
	        if (tf_buf_len >= 50 ) {
                    
		    destroy_gprs_pkt(ep);

		}
                else {	
                    temp_frame_buf->insert_tail(ep);
		    printf("BTSrlc::send(): insert into temp_frame_buf \n");
		}
#endif
                temp_frame_buf->insert_tail(ep);
		printf("BTSrlc::send(): insert into temp_frame_buf \n");
                return 1;

            }
        }


        int MsRlc::recv(Event* ep) {

            IS_NULL_STR(ep,"Rlc recv(): Exceptions: recv an event pointing to address zero \n",0);
            IS_NULL_STR(ep->DataInfo_, "Rlc recv(): Exceptions: DataInfo_ points to address zero \n",0 );

            /* For real system, MAC is responsible for determining the type of information that a block carries.
             * MAC layer can determine the block type according to the frame number corresponding to the logical
             * channel. In NCTUns GPRS package, MAC layer determines the message type and then fill the field,
             * "ctl_msg_type". The msg type can be filled by the BTS RLC layer to achieve double-protection.
             * Second, the field, "user_data_bsn" also can be provided by RLC sender in advance to perform
             * fast decoding process.
             */
            uchar* decoded_data     = NULL;
            bss_message* bssmsg     = reinterpret_cast<bss_message*> (ep->DataInfo_);
            //gmm_option* gmm_opt     = reinterpret_cast<gmm_option*> (bssmsg->sndcp_option);
            //llc_option* llc_opt     = reinterpret_cast<llc_option*> (bssmsg->llc_option);
            rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

            //IS_NULL_STR(gmm_opt   , "Rlc recv(): Exceptions: gmm_opt is null \n",0 );
            //IS_NULL_STR(llc_opt   , "Rlc recv(): Exceptions: llc_opt is null \n",0 );
            IS_NULL_STR(rlc_opt   , "MsRlc recv(): Exceptions: rlc_opt is null \n",0 );

            /* cmd field identifies communication primitives used by RLC and MAC layers */
            int res = 1;

            if ( rlc_opt->cmd == UPLINK_TBF_ESTABLISHED) {

                if ( bssmsg->user_data ) {

                    TbfDes*     utbfdes;
                    UTCB*       utcb;

                    utbfdes = reinterpret_cast<TbfDes*> (bssmsg->user_data);
                    IS_NULL_STR(utbfdes,
                        "MsRlc recv(): Uplink Tbf Established but the corresponding structure from MAC is not found.\n" , 0);
                    uchar tmptfi = utbfdes->get_tfi();
                    utcb = new UTCB(SENDER,tmptfi, this, &Rlc::ori_send , &Rlc::ori_recv );

                    utcb->transform_tcb_descriptions(utbfdes);

                    if (!utcb->tfi_range_test())
                        res = 0;

                    else {

                        if (utcb->get_state() != READY )
                            res = 0;

                        else {

                            if ( node_type == gprs::ms ) {
                                res = utbfmap->insert(utcb, (utbfdes->get_imsi()),(utbfdes->get_tlli()) );
                                if ( res < 0 )
                                    exit(1);
                            }

                        }
                    }

                    if (res>0) {
                        trigger_send(utcb);
                    }

                }
                else {
                    cout << "Rlc recv(): assertion failed: tbfdes is not carried \n" << endl;
                    res = 0;
                }

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT(ep);

                return res;
            }
            else if ( rlc_opt->cmd == DOWNLINK_TBF_ESTABLISHED ) {

                if ( bssmsg->user_data ) {

                    TbfDes*     dtbfdes;
                    DTCB*       dtcb;

                    dtbfdes         = reinterpret_cast<TbfDes*> (bssmsg->user_data);
                    IS_NULL_STR(dtbfdes,
                        "MsRlc recv(): Downlink TBF Established but the corresponding structure from MAC is not found.\n" , 0);
                    uchar tmptfi    = dtbfdes->get_tfi();
                    dtcb            = new DTCB(RECEIVER, tmptfi , this, &Rlc::ori_send , &Rlc::ori_recv);

                    dtcb->transform_tcb_descriptions(dtbfdes);
                    bssmsg->user_data       = NULL;
                    bssmsg->user_data_len   = 0;

                    if (!dtcb->tfi_range_test())
                        res = 0;

                    else {

                        if (dtcb->get_state() != READY )
                            res = 0;

                        else {

                            if ( node_type == gprs::ms ) {
                                res = dtbfmap->insert(dtcb,(dtbfdes->get_imsi()),(dtbfdes->get_tlli()));

                                if ( res < 0 ) {
                                    DTCB* tmp = dtbfmap->get( (dtcb->get_tfi()) );
                                    if ( tmp ) {
                                        delete tmp;
                                        dtbfmap->remove_entry_by_tfi( (dtcb->get_tfi()) );
                                        res = dtbfmap->insert(dtcb, (dtbfdes->get_imsi()),(dtbfdes->get_tlli()) );
                                        if ( res < 0 )
                                            exit(1);
                                        else
                                            printf("reinsert the DTCB structure successfully. \n");
                                    }
                                    else {
                                        printf("MS RLC: Assertion failed.\n");
                                        exit(1);
                                    }
                                }

                            }
                        }
                    }

                }
                else {
                    cout << "Rlc recv(): assertion failed: tcbdes is not carried \n" << endl;
                    res = 0;
                }

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT(ep);

                return res;
            }
            else if ( rlc_opt->cmd == ROAMING_COMPLETED ) {

                if ( bssmsg->user_data ) {

                    TbfDes*     utbfdes;
                    UTCB*       utcb;

                    utbfdes             = reinterpret_cast<TbfDes*> (bssmsg->user_data);
                    bssmsg->user_data   = NULL;

                    IS_NULL_STR(utbfdes,
                        "MsRlc recv(): utbfdes is invalid while MS performs roaming procedure.\n" , 0);
                    uchar tmptfi = utbfdes->get_tfi();
                    utcb = new UTCB(SENDER,tmptfi, this, &Rlc::ori_send , &Rlc::ori_recv );

                    utcb->transform_tcb_descriptions(utbfdes);

                    if (!utcb->tfi_range_test())
                        res = 0;

                    else {

                        if (utcb->get_state() != READY )
                            res = 0;

                        else {

                            if ( node_type == gprs::ms ) {
                                res = utbfmap->insert(utcb, (utbfdes->get_imsi()),(utbfdes->get_tlli()) );

                                if ( res < 0 ) {
                                    UTCB* tmp = utbfmap->get( (utcb->get_tfi()) );
                                    if ( tmp ) {
                                        delete tmp;
                                        utbfmap->remove_entry_by_tfi( (utcb->get_tfi()) );
                                        res = utbfmap->insert(utcb, (utbfdes->get_imsi()),(utbfdes->get_tlli()) );
                                        if ( res < 0 )
                                            exit(1);
                                        else {
                                            printf("reinsert the TCB structure successfully. \n");
                                            printf("TCB downlink_freq = %d uplink_freq = %d \n\n",
                                                utcb->downlink_freq , utcb->uplink_freq );
                                        }
                                    }
                                    else {
                                        printf("MS RLC: Assertion failed.\n");
                                        exit(1);
                                    }
                                }
                                else {
                                    printf("MS RLC: reinsert TCB for roaming successfully.\n");
                                    printf("TCB downlink_freq = %d uplink_freq = %d \n\n",
                                                utcb->downlink_freq , utcb->uplink_freq );
                                }
                            }

                        }
                    }

                    if (res>0) {
                        create_llc_option(bssmsg);
                        trigger_send(utcb);
                    }

                }
                else {
                    cout << "Rlc recv(): assertion failed: tcbdes is not carried \n" << endl;
                    res = 0;
                }

                if ( res < 0 ) {
                    printf("MS RLC: roaming failed .\n");
                    exit(1);
                }
                else
                    return NslObject::recv(ep);
            }
            else if ( rlc_opt->cmd == SEND_PAGING_REQUEST ) {
                /* In MS side, this message shall be passed to LLC.
                * In BTS side, this message shall not be received.(Assertion failed)
                */

                if ( node_type == gprs::ms ) {
                    if (!bssmsg->llc_option) {
                        llc_option* tmp;
                        tmp = new llc_option;
                        (bssmsg->llc_option) = reinterpret_cast<char*> (tmp);
                    }

                    reinterpret_cast<llc_option*> (bssmsg->llc_option)->command = SEND_PAGING_REQUEST;
                    return NslObject::recv(ep);
                }
                else {


                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                        printf("assertion failed.\n");
                        exit(1);
                    }

                    FREE_BSS_EVENT(ep);
                    cout << "BtsRlc recv(): Assertion failed: receiving a Paging Request." << endl;
                    return 0;
                }
            }
            else if ( rlc_opt->cmd == PAGING_RESPONSE ) {

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                        printf("assertion failed.\n");
                        exit(1);
                    }
                    FREE_BSS_EVENT(ep);
                    cout << "MsRlc recv(): Assertion failed: receiving a Paging Response." << endl;
                    return 0;
            }

            else if ( rlc_opt->cmd == PUSH_A_RLCDATA_BLK ) {

                PollInd* poll_ind = reinterpret_cast<PollInd*> (bssmsg->user_data);
                IS_NULL_STR(poll_ind,"Rlc recv(): poll_id is null" ,0);

                DTCB* seldtcb   = NULL;
                UTCB* selutcb   = NULL;

                if ( node_type == gprs::ms ) {

                   /* Ideally, the downlink TBF tn and the uplink TBF tn are orthogonal and do not
                    * interfere with each other. However, if this case occurs, the preference principal
                    * is defined as the following clauses:
                    * In MS side, DTBF has higher priority since its messages are PKT_ACKNACKs.
                    * In BTS side, UTBF has higher priority due to the same reason.
                    */

                    dtbfmap->clear_selected();
                    utbfmap->clear_selected();


                    /* In the case that rrbp_ind is valid */
                    //poll_ind->rrbp_ind = 0; /* turn off rrbp mechanism */
                    if ( poll_ind->rrbp_ind <4 ) {

                        for ( int i=0 ; i<32; ++i ) {

                            seldtcb = dtbfmap->sel_tbf_to_send( poll_ind->polling_tn , poll_ind->polling_ch_bitmap);

                            if ( seldtcb ) {

                                res = seldtcb->create_ack();
                                if ( res > 0 ) {
                                    #ifdef __DATA_SENT_DEBUG
                                    cout << "MsRlc::recv(): push a Downlink ACK down to MAC."<< endl;
                                    #endif
                                    break;
                                }
                                else
                                    printf("MS RLC: try to send dtcb ACK but no ACK to send.\n");

                            }
                        }

                    }
                    else {
                        for ( int i=0 ; i<32; ++i ) {

                            selutcb = utbfmap->sel_tbf_to_send(poll_ind->polling_tn,poll_ind->polling_ch_bitmap);

                            if ( selutcb ) {

                                /*printf("**MS RLC: nid = %ld **********\n", get_nid() );*/
                                Event* pushed_blk = selutcb->scheduler();
                                /*printf("******************************\n");*/

                                if ( pushed_blk ) {


                                    #ifdef __TCB_DUMP

                                    bss_message* pblk_bmsg = reinterpret_cast<bss_message*> (pushed_blk->DataInfo_);

                                    rlc_option* sss = reinterpret_cast<rlc_option*> pblk_bmsg->rlc_option;

                                    if ( sss )
                                        printf("MS TCB:: try to send block with bsn = %d \n", sss->user_data_bsn );
                                    else
                                        printf("MS TCB:: try to send block with no rlc_option\n");

                                    printf ("MsRlc UTCB send a rlcblock: ta=%d ufreq=%d dfreq=%d tfi=%d tlli=%lu \n",
                                            selutcb->timeslot_allocation, selutcb->uplink_freq,
                                                selutcb->downlink_freq, selutcb->tfi, selutcb->tlli);
                                    printf("Polling_tn=%d \n",poll_ind->polling_tn);
                                    #endif

                                    res = NslObject::send(pushed_blk);

                                    if ( res > 0 ) {

                                        #ifdef __DATA_SENT_DEBUG
                                        cout << "MsRlc::recv(): push a data packet down to MAC."<< endl;
                                        #endif
                                        break;

                                    }
                                }
                                else
                                    continue; /* this TBF doesn't have data to send so far */
                            }
                        }
                    }

                    /* PUSH_A_RLC_BLOCK event shall be freed by RLC layer */
                    delete poll_ind;
                    bssmsg->user_data = NULL;

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                        printf("Assertion failed.\n");
                        exit(1);
                    }

                    FREE_BSS_EVENT(ep);
                    return 1;

                }
                else {
                    delete poll_ind;
                    bssmsg->user_data = NULL;

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                        printf("Assertion failed.\n");
                        exit(1);
                    }

                    FREE_BSS_EVENT(ep);
                    cout <<"MsRlc recv(): Assertion failed: The instance of MsRlc is created without nodetype being MS." << endl;
                    exit(1);
                }

            }

            else if ( rlc_opt->cmd == LLGMM_DETACH_SENT_COMPLETE ) {

                uchar remove_flag = false;
                UTCB* marked_utcb = utbfmap->search_by_tlli( rlc_opt->detach_tlli );
                if ( marked_utcb ) {
                    marked_utcb->detach_flag = true;

                    if ( marked_utcb->llgmm_reset_flag ) {
                        utbfmap->remove( rlc_opt->detach_tlli);
                        remove_flag = true;
                    }
                }

                DTCB* marked_dtcb = dtbfmap->search_by_tlli( rlc_opt->detach_tlli );
                if ( marked_dtcb ) {
                    marked_dtcb->detach_flag = true;

                    if ( marked_dtcb->llgmm_reset_flag ) {
                        dtbfmap->remove( rlc_opt->detach_tlli);
                        remove_flag = true;
                    }
                }

                if ( remove_flag )
                    cout << "MS RLC: remove TCBs with TLLI = " << rlc_opt->detach_tlli << endl;

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("Assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT( ep );
                return 1;

            }
            else if ( rlc_opt->cmd == GMM_UPDATE ) {

                create_llc_option(bssmsg);
                return NslObject::recv(ep);

            }
            else if (!rlc_opt->cmd) {
                ;/* user data */
            }
            else {
                cout << "Rlc recv(): unknown cmd value" << endl;

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("Assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT(ep);
                return 0;
            }

            /* ctlmsg differentiates RLCMAC messages exchanged in BSS mac layer should
            * perform decoding because control message is parsed and identified in mac layer
            */

            decoded_data = reinterpret_cast<uchar*> (bssmsg->user_data);
            bssmsg->user_data = NULL;

            #ifndef __DECODE_CORRECT
            uchar* fake_data = new uchar[8];
            bzero( fake_data , 8 );
            decoded_data = fake_data;
            #endif

            if (!decoded_data) {

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("Assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT(ep);
                return 0;
            }
            /* The error detection is performed by channel decoding function. The encoded data
            * is partitioned into four bursts. Each burst is protected by CRC checking.
            * CRC checking at this point is disabled thus far.
            *
            *
            */

            /* received a data packet: For a MS, the received data block
            * is expected as a downlink data block. Contrarily, for a BTS the
            * received data block is expected as an uplink data block.
            */

        if ( node_type == gprs::ms ) {

            /* If receiving a PACKET ACK/NACK,
            * Then, invoke process_ack().
            *
            * Else if receiving a data block,
            * Then, enqueue the data block and invoke reassemble_upper_layer_pdu().
            *
            * Else if receiving a control block,
            * Then, invoke process_control_block().
            *
            * Else exception occurs.
            */

            if ( rlc_opt->ctl_msg_type<0 ) { /* data block */

                #ifdef __DECODE_CORRECT

                RDDBH* rddbh;
                if (bssmsg->rlc_header)
                    rddbh = reinterpret_cast<RDDBH*> (bssmsg->rlc_header);
                else {
                    rddbh = new RDDBH;
                    rddbh->unpack_header(decoded_data+1);
                }

                if ( rddbh->bsn != rlc_opt->user_data_bsn ) {
                    printf("Rlc(ms side) recv(): bsn mismatch!\n");
                    exit(1);
                }



                DTCB* dtcb = dtbfmap->get(rddbh->tfi);
                if ( !dtcb ) {
                    printf("assert.");
                    IS_NULL_STR(dtcb,"Debug: rlc recv(): Downlink TFI mismatches.\n",0);
                }

                #else

                DTCB* dtcb = dtbfmap->get(rlc_opt->tfi);
                IS_NULL_STR(dtcb,"Debug: rlc recv(): Downlink TFI mismatches.\n",0);

                #endif

                dtcb->recv_data( ep , decoded_data );
            }
            else if ( rlc_opt->ctl_msg_type == PKT_POLLING_REQUEST ) {

                /* examine necessary fields and invoke TCB->create_ack */
                RDCBH* rdcbh;
                if (bssmsg->rlc_header) {
                    rdcbh = reinterpret_cast<RDCBH*> (bssmsg->rlc_header);
                    bssmsg->rlc_header      = NULL;
                    bssmsg->rlc_header_len  = 0;
                }
                else {
                    rdcbh = new RDCBH;
                    rdcbh->unpack_header(decoded_data+1);
                }

                if ( rdcbh->tfi >=32 ) {
                    cout <<"MsRlc recv(): tfi is out of range." << endl;
                    if ( rdcbh )
                        delete rdcbh;

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                        printf("Assertion failed.\n");
                        exit(1);
                    }

                    FREE_BSS_EVENT(ep);
                    return 0;
                }

                DTCB* dtcb = dtbfmap->get(rdcbh->tfi);
                IS_NULL_STR(dtcb,"Debug: rlc recv(): Downlink TFI mismatches.\n",0);

                /*Event* eack = dtcb->create_ctl_ack();
                dtcb->send_ctl_ack(eack);*/
            }

            else if ( rlc_opt->ctl_msg_type == PKT_UPLINK_ACKNACK)  { /* uplink ack */

                RDCBH* rdcbh;
                NonDistributedMsg* u_ack_msg;
                if (bssmsg->rlc_header) {
                    rdcbh = reinterpret_cast<RDCBH*> (bssmsg->rlc_header);
                    u_ack_msg = reinterpret_cast<NonDistributedMsg*>(decoded_data);
                    bssmsg->rlc_header      = NULL;
                    bssmsg->rlc_header_len  = 0;
                }
                else {
                    rdcbh = new RDCBH;
                    rdcbh->unpack_header(decoded_data+1);
                    u_ack_msg = reinterpret_cast<NonDistributedMsg*>(decoded_data+3);
                }


                UTCB*  utcb = NULL;

                if ( utbfmap->utcb_list->get(rdcbh->tfi) )
                    utcb = utbfmap->utcb_list->get(rdcbh->tfi)->get_elem();
                else {

                    utcb = NULL;

                    shared_obj_dec_refcnt( NDMSG , u_ack_msg );

                    if ( (shared_obj_release( NDMSG , u_ack_msg )) ) {
                        u_ack_msg = NULL;
                    }

                    if ( rdcbh )
                        delete rdcbh;

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                        printf("Assertion failed.\n");
                        exit(1);
                    }

                    FREE_BSS_EVENT(ep);
                    return 1;
                }

                //IS_NULL_STR(utcb,"Debug: rlc(ms side) recv(): Uplink TFI mismatches.\n",0);

                PUACK* pua = reinterpret_cast<PUACK*>(u_ack_msg->get_nondistr_msg());

                #ifdef __ACK_DEBUG
                cout << "MS MAC: receive Uplink ACKNACK "<< endl;
                #endif

                utcb->recv_ack(pua);
                shared_obj_dec_refcnt( NDMSG , u_ack_msg );
                if ( (shared_obj_release( NDMSG , u_ack_msg )) ) {
                    u_ack_msg = NULL;
                }

                if ( rdcbh )
                        delete rdcbh;


                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("Assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT(ep);
            }
            else {
                cout << "rlc(ms side) recv(): unknown ctl block type" << endl;

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                    printf("Assertion failed.\n");
                    exit(1);
                }

                FREE_BSS_EVENT(ep);
                exit(1);
            }
        }

        return 1;
    }


    int BtsRlc::recv(Event* ep) {

        IS_NULL_STR(ep,"BtsRlc recv(): Exception: recv an event pointing to address zero \n",0);
        IS_NULL_STR(ep->DataInfo_, "BtsRlc recv(): Exception: DataInfo_ points to address zero \n",0 );

        /* For real system, MAC is responsible for determining the type of information that a block carries.
        * MAC layer can determine the block type according to the frame number corresponding to the logical
        * channel. In NCTUns GPRS package, MAC layer determines the message type and then fill the field,
        * "ctl_msg_type". The msg type can be filled by the BTS RLC layer to achieve double-protection.
        * Second, the field, "user_data_bsn" also can be provided by RLC sender in advance to perform
        * fast decoding process.
        */
        uchar* decoded_data     = NULL;
        bss_message* bssmsg     = reinterpret_cast<bss_message*> (ep->DataInfo_);
        gmm_option* gmm_opt     = reinterpret_cast<gmm_option*> (bssmsg->sndcp_option);
        llc_option* llc_opt     = reinterpret_cast<llc_option*> (bssmsg->llc_option);
        rlc_option* rlc_opt     = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);

        //IS_NULL_STR(gmm_opt   , "Rlc recv(): Exceptions: gmm_opt is null \n",0 );
        //IS_NULL_STR(llc_opt   , "Rlc recv(): Exceptions: llc_opt is null \n",0 );
        IS_NULL_STR(rlc_opt   , "BtsRlc recv(): Exceptions: rlc_opt is null \n",0 );

        /* cmd field identifies communication primitives used by RLC and MAC layers */
        int res = 0;

        if ( rlc_opt->cmd == UPLINK_TBF_ESTABLISHED) {

            if ( bssmsg->user_data ) {

                TbfDes*     utbfdes;
                UTCB*       utcb;

                utbfdes = reinterpret_cast<TbfDes*> (bssmsg->user_data);
                IS_NULL_STR(utbfdes,
                    "BtsRlc recv(): Uplink Tbf Established but the corresponding structure from MAC is not found.\n" , 0);
                uchar tmptfi = utbfdes->get_tfi();
                utcb = new UTCB(RECEIVER, tmptfi , this , &Rlc::ori_send , &Rlc::ori_recv );

                utcb->transform_tcb_descriptions(utbfdes);


                if (!utcb->tfi_range_test())
                    res = 0;
                else {

                    if (utcb->get_state() != READY )
                        res = 0;
                    else {
                        utbfmap[utcb->uplink_freq]->insert(utcb, 0 ,utbfdes->get_tlli());
                        res = 1;
                    }
                }

                if (res>0) {
                    utcb->init_ack_timer();
                    utcb->start_ack_timer();
                }
            }

            else {
                cout << "BtsRlc recv(): assertion failed: tcbdes is not carried \n" << endl;
                res = 0;
            }

            if ( bssmsg->packet) {
                remove_upper_layer_pkt(bssmsg->packet);
                free_pkt(bssmsg->packet);
                bssmsg->packet = NULL;
            }

            FREE_BSS_EVENT(ep);

            return res;
        }
        else if ( rlc_opt->cmd == DOWNLINK_TBF_ESTABLISHED ) {

            if ( bssmsg->user_data ) {

                TbfDes*     dtbfdes;
                DTCB*       dtcb;

                dtbfdes = reinterpret_cast<TbfDes*> (bssmsg->user_data);
                IS_NULL_STR(dtbfdes,
                    "BtsRlc recv(): Downlink Tbf established but the corresponding data structure from MAC is not found.\n" , 0);
                uchar tmptfi = dtbfdes->get_tfi();
                dtcb = new DTCB(SENDER, tmptfi , this, &Rlc::ori_send , &Rlc::ori_recv );

                dtcb->transform_tcb_descriptions(dtbfdes);

                if (!dtcb->tfi_range_test())
                    res = 0;

                else {

                    if (dtcb->get_state() != READY )
                        res = 0;

                    else {


                            dtbfmap[CORRESPONDING_UPLINK_CH(dtcb->downlink_freq)]->
                                insert(dtcb,(dtbfdes->get_imsi()),dtcb->tlli);
                            Event* blk = temp_frame_buf->get_head()->get_elem();

                            if ( blk ) {
                                dtcb->send(blk);
                                temp_frame_buf->remove_head();
                            }

                        res = 1;
                    }
                }

                if (res>0) {
                    dtcb->init_ack_timer();
                    dtcb->start_ack_timer();
                    trigger_send(dtcb);
                }

            }
            else {
                cout << "BtsRlc recv(): assertion failed: tbfdes is not carried \n" << endl;
                res = 0;
            }

            if ( bssmsg->packet) {
                remove_upper_layer_pkt(bssmsg->packet);
                free_pkt(bssmsg->packet);
                bssmsg->packet = NULL;
            }

            FREE_BSS_EVENT(ep);

            return res;
        }
        else if ( rlc_opt->cmd == SEND_PAGING_REQUEST ) {
            /* In MS side, this message shall be passed to LLC.
            * In BTS side, this message shall not be received.(Assertion failed)
            */

            if ( node_type == gprs::ms ) {
                if (!bssmsg->llc_option) {
                    llc_option* tmp;
            tmp = new llc_option;
            (bssmsg->llc_option) = reinterpret_cast<char*> (tmp);
        }

            reinterpret_cast<llc_option*> (bssmsg->llc_option)->command = SEND_PAGING_REQUEST;
                return NslObject::recv(ep);
            }
            else {

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                }

                FREE_BSS_EVENT(ep);
                cout << "BtsRlc recv(): Assertion failed: receiving a Paging Request." << endl;
                return 0;
            }
        }
        else if ( rlc_opt->cmd == PAGING_RESPONSE ) {
            if ( node_type == gprs::bts ) {

                command_mac(PAGING_RESPONSE,gmm_opt->imsi, llc_opt->oldtlli, llc_opt->tlli,255,-1,NULL);
                return NslObject::recv(ep);
            }
            else {

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                }

                FREE_BSS_EVENT(ep);
                cout << "BtsRlc recv(): Assertion failed: receiving a Paging Response." << endl;
                return 0;
            }
        }

        else if ( rlc_opt->cmd == PUSH_A_RLCDATA_BLK ) {

            PollInd* poll_ind = reinterpret_cast<PollInd*> (bssmsg->user_data);
            IS_NULL_STR(poll_ind,"BtsRlc recv(): poll_id is null" ,0);

            DTCB* seldtcb   = NULL;
            UTCB* selutcb   = NULL;

            /* trigger the sending of ACKs in uplink TBFs */

            uchar utbf_send_ack_flag_vector[125];
            bzero(utbf_send_ack_flag_vector , 125 );


            for ( int i = 1 ; i<125 ; ++i ) {

                if ( utbfmap[i] ) {

                    utbfmap[i]->clear_selected();

                    if ( poll_ind->polling_ch_bitmap[i] ) {


                        for ( int j=0 ; j<32; ++j ) {

                            selutcb = utbfmap[i]->sel_tbf_to_send(poll_ind->polling_tn,poll_ind->polling_ch_bitmap);

                            if ( selutcb ) {

                                res = selutcb->create_ack();
                                if ( res > 0 ) {

                                    utbf_send_ack_flag_vector[i] = true;

                                    #ifdef __DATA_SENT_DEBUG
                                    cout << "BtsRlc::recv(): push an Uplink ACK down to MAC."<< endl;
                                    #endif
                                    break;

                                }

                            }
                        }

                    }
                }

                /* trigger the sending of downlink TBFs */
                if ( dtbfmap[i]) {

                    dtbfmap[i]->clear_selected();

                    if ( poll_ind->polling_ch_bitmap[i+125] ) {


                        for ( int j=0 ; j<32; ++j ) {

                            seldtcb = dtbfmap[i]->sel_tbf_to_send(poll_ind->polling_tn,poll_ind->polling_ch_bitmap);

                            if ( seldtcb ) {

                                Event* pushed_blk = seldtcb->scheduler();
                                if ( pushed_blk ) {
                                    rlc_option* rlc_opt =
                                        reinterpret_cast<rlc_option*>((reinterpret_cast<bss_message*> (pushed_blk->DataInfo_))->rlc_option);

                                    #ifdef __ACK_DEBUG
                                    cout << "BTS RLC: seldtcb->ack_req = " <<  (int)(seldtcb->ack_req)
                                        << " and clear to false." << endl;
                                    #endif

                                    if ( seldtcb->ack_req ) {

                                        rlc_opt->rrbp       = seldtcb->rrbp_value;
                                        seldtcb->rrbp_value = gprs::rrbp_unused;
                                        seldtcb->ack_req    = false;

                                        /* Send a RRBP_REQUEST to BTS MAC to make it
                                        * allocate an uplink block for PACKET DOWNLINK
                                        * ACKNACK.
                                        */
                                        //command_mac(RRBP_REQUEST);

                                    }

                                    #ifdef __ACK_DEBUG
                                    cout << "BTS RLC: add rrbp value = " <<  (int)(rlc_opt->rrbp)
                                        << " and clear to unused." << endl;
                                    #endif

                                    res = NslObject::send(pushed_blk);
                                    if ( res > 0 ) {
                                        #ifdef __DATA_SENT_DEBUG
                                        cout << "BtsRlc::recv(): push a data packet down to MAC."<< endl;
                                        #endif
                                        break;
                                    }
                                }
                                else
                                    continue; /* this TBF doesn't have data to send so far */
                            }
                        }

                    }

                }


            }

            delete poll_ind;
            bssmsg->user_data = NULL;

            if ( bssmsg->packet) {
                remove_upper_layer_pkt(bssmsg->packet);
                free_pkt(bssmsg->packet);
                bssmsg->packet = NULL;
            }

            FREE_BSS_EVENT(ep);
            return 1;
        }

        else if ( rlc_opt->cmd == LLGMM_DETACH_SENT_COMPLETE ) {

            uchar remove_flag = false;
            for (int i=0 ; i<125 ;++i ) {

                if ( utbfmap[i] ) {
                    UTCB* marked_tcb = utbfmap[i]->search_by_tlli( rlc_opt->detach_tlli );
                    if ( marked_tcb ) {
                        marked_tcb->detach_flag = true;

                        if ( marked_tcb->llgmm_reset_flag ) {
                            utbfmap[i]->remove( rlc_opt->detach_tlli);
                            remove_flag = true;
                        }
                    }
                }

                if ( dtbfmap[i] ) {
                    DTCB* marked_tcb = dtbfmap[i]->search_by_tlli( rlc_opt->detach_tlli );
                    if ( marked_tcb ) {
                        marked_tcb->detach_flag = true;

                        if ( marked_tcb->llgmm_reset_flag ) {
                            dtbfmap[i]->remove( rlc_opt->detach_tlli);
                            remove_flag = true;
                        }
                    }
                }
            }

            if ( remove_flag )
                cout << "BTS RLC: remove TCBs with TLLI = " << rlc_opt->detach_tlli << endl;


            if ( bssmsg->packet) {
                remove_upper_layer_pkt(bssmsg->packet);
                free_pkt(bssmsg->packet);
                bssmsg->packet = NULL;
            }
            FREE_BSS_EVENT( ep );
            return 1;

        }

        else if (!rlc_opt->cmd) {
            /* user data */
        }
        else {
            cout << "Rlc recv(): unknown cmd value" << endl;

            if ( bssmsg->packet) {
                remove_upper_layer_pkt(bssmsg->packet);
                free_pkt(bssmsg->packet);
                bssmsg->packet = NULL;
            }
            FREE_BSS_EVENT(ep);
            return 0;
        }

        /* ctlmsg differentiates RLCMAC messages exchanged in BSS
        * mac layer should perform decoding because control message is parsed and identified
        * in mac layer
        */

        decoded_data = reinterpret_cast<uchar*> (bssmsg->user_data);
        bssmsg->user_data = NULL;

        #ifndef __DECODE_CORRECT
        uchar* fake_data = new uchar[8];
        bzero( fake_data , 8 );
        decoded_data = fake_data;
        #endif

        if (!decoded_data) {

            if ( bssmsg->packet) {
                remove_upper_layer_pkt(bssmsg->packet);
                free_pkt(bssmsg->packet);
                bssmsg->packet = NULL;
            }

            FREE_BSS_EVENT(ep);
            return 1;
        }
        /* The error detection is performed by channel decoding function. The encoded data
        * is partitioned into four bursts. Each burst is protected by CRC checking.
        * CRC checking at this point is disabled thus far.
        *
        *
        */

        if(rlc_opt->cmd > 0)
            return res;

        /* received a data packet: For a MS, the received data block
        * is expected as a downlink data block. Contrarily, for a BTS the
        * received data block is expected as an uplink data block.
        */


        if ( node_type == gprs::bts ) {
            /* received a data packet: For a gprs::ms, the received data block
            * is expected as a downlink data block. Contrarily, for a BTS the
            * received data block is expected as an uplink data block.
            */
            /* User Data */
            if ( rlc_opt->ctl_msg_type<0 ) {

                #ifdef  __DECODE_CORRECT

                RUDBH* rudbh;

                if (bssmsg->rlc_header)
                    rudbh = reinterpret_cast<RUDBH*> (bssmsg->rlc_header);
                else {
                    rudbh = new RUDBH;
                    rudbh->unpack_header(decoded_data+1);
                }

                if ( rudbh->bsn != rlc_opt->user_data_bsn ) {
                    printf("Rlc(bts side) recv(): bsn mismatch! rudbh->bsn = %d rlc_opt->bsn = %d \n",
                        rudbh->bsn , rlc_opt->user_data_bsn);
                    return 0;
                }



                mac_option* mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
                UTCB* utcb = NULL;

                if ( (mac_opt->channel_id >=0) && (mac_opt->channel_id < 125) && (rudbh->tfi < 32) ) {

                    if ( utbfmap[mac_opt->channel_id]->utcb_list->get(rudbh->tfi) )
                        utcb = utbfmap[mac_opt->channel_id]->utcb_list->get(rudbh->tfi)->get_elem();

                }

                if (!utcb) {
                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                    }
                    FREE_BSS_EVENT(ep);
                    return 0;
                }

                #else
                mac_option* mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
                UTCB* utcb = NULL;

                if ( (mac_opt->channel_id < 125) && (rudbh->tfi < 32) ) {

                    if ( utbfmap[mac_opt->channel_id]->utcb_list->get(rudbh->tfi) )
                        utcb = utbfmap[mac_opt->channel_id]->utcb_list->get(rudbh->tfi)->get_elem();

                }

                if (!utcb) {
                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                    }
                    FREE_BSS_EVENT(ep);
                    return 0;
                }

                #endif

                /*printf("**** BTS RLC:: nid = %ld ****\n", get_nid() );*/
                    utcb->recv_data( ep , decoded_data );
                /*printf("******************************\n");*/

            }
            /* Downlink ACK */
            else if ( rlc_opt->ctl_msg_type == PKT_DOWNLINK_ACKNACK) {

                #ifdef __ACK_DEBUG
                cout << "BTS MAC: receive Downlink ACKNACK "<< endl;
                #endif

                /* Uplink Control Blk doesn't have RLC header */
                NonDistributedMsg* d_ack_msg =reinterpret_cast<NonDistributedMsg*> (decoded_data);
                GlobalTfi gtfi_ie;
                d_ack_msg->get_addr_info(&gtfi_ie);
                PDACK* pda = reinterpret_cast<PDACK*>(d_ack_msg->get_nondistr_msg());
                mac_option* mac_opt = reinterpret_cast<mac_option*> (bssmsg->mac_option);
                DTCB* dtcb;
                if ((dtcb=dtbfmap[mac_opt->channel_id]->dtcb_list->get(gtfi_ie.tfi)->get_elem()) ) {

                    dtcb->recv_ack(pda);

                    shared_obj_dec_refcnt( NDMSG , d_ack_msg );
                    if ( (shared_obj_release( NDMSG , d_ack_msg )) ) {
                        d_ack_msg = NULL;
                    }

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                    }
                    FREE_BSS_EVENT(ep);
                }
                else {
                    cout << "rlc(bts side) recv(): Assertion failed:" <<
                            " Receive a PKT Downlink ACK that does not have the corresponding DTbf" << endl;

                    shared_obj_dec_refcnt( NDMSG , d_ack_msg );
                    if ( (shared_obj_release( NDMSG , d_ack_msg )) ) {
                        d_ack_msg = NULL;
                    }

                    if ( bssmsg->packet) {
                        remove_upper_layer_pkt(bssmsg->packet);
                        free_pkt(bssmsg->packet);
                        bssmsg->packet = NULL;
                    }

                    FREE_BSS_EVENT(ep);

                    return 0;
                }
            }
            else {

                cout << "rlc(bts side) recv(): assertion failed: Unknown block type." << endl;

                if ( bssmsg->packet) {
                    remove_upper_layer_pkt(bssmsg->packet);
                    free_pkt(bssmsg->packet);
                    bssmsg->packet = NULL;
                }

                FREE_BSS_EVENT(ep);

                return 0; /* unexpected block type */
            }
        }
        else
            ;

        return 1;
    }




    int Rlc::command_mac(uchar cmd,ulong imsi, ulong tlli_old , ulong tlli_cur ,uchar tfi,long freq,void* msg) {

        try {
            Event* erlccmd;
            CREATE_BSS_EVENT(erlccmd);
            bss_message* bssmsg         = reinterpret_cast<bss_message*>(erlccmd->DataInfo_);
            bssmsg->flag                = PF_SEND;
            bssmsg->user_data           = reinterpret_cast<char*> (msg);

            create_llc_option(bssmsg);
            llc_option* llc_opt         = reinterpret_cast<llc_option*> (bssmsg->llc_option);
            llc_opt->tlli               = tlli_cur;
            llc_opt->oldtlli            = tlli_old;

            create_rlc_option(bssmsg);
            rlc_option* rlc_opt         = reinterpret_cast<rlc_option*> (bssmsg->rlc_option);
            rlc_opt->cmd                = cmd;
            rlc_opt->tfi                = tfi;
            rlc_opt->chid               = freq;

            gmm_option* gmm_opt         = new gmm_option;
            gmm_opt->imsi               = imsi;
            bssmsg->sndcp_option        = reinterpret_cast<char*> (gmm_opt);
            bssmsg->sndcp_option_len    = sizeof(gmm_option);
            return NslObject::send(erlccmd);
        }
        catch (std::exception& e) {
            cout << "Rlc command_mac(): assertion failed, reason:" << e.what() << endl;
            return 0;
        }

    }


    int Rlc::trigger_send(UTCB* tcb) {
        Event* ep;
        if ( (ep=search_unsent_event_by_tlli(tcb->get_tlli()) ) ) {
            tcb->send(ep);
            return 1;
        }
        else
            return 0;
    }

    int Rlc::trigger_send(DTCB* tcb) {
        Event* ep;
        if ( (ep=search_unsent_event_by_tlli(tcb->get_tlli()) ) ) {
            tcb->send(ep);
            tcb->restart_ack_timer();
            return 1;
        }
        else
            return 0;
    }

    Event*  Rlc::search_unsent_event_by_tlli(ulong tlli) {

        Event* res = NULL;

        if (temp_frame_buf) {

            ListElem<Event>* elem = temp_frame_buf->get_head();
            if (elem) {
                for (; elem ; elem = elem->get_next() ) {
                    Event* ptr = elem->get_elem();
                    if ( ptr ) {

                        bss_message* bmsg = reinterpret_cast<bss_message*> (ptr->DataInfo_);

                        if ( bmsg->llc_option ) {
                            if ( (reinterpret_cast<llc_option*>(bmsg->llc_option))->tlli == tlli ) {
                                res = ptr;
                                temp_frame_buf->remove(elem);
                                break;
                            }
                        }
                    }
                } /* end of for loop */
            }
        }

        return res;
    }

    /********************************************************************/
    UTCB* UTbfMap::search(uchar tfi) {
        for ( int i=0 ; i<32 ;++i ) {
            if ( utcb_list->get(i)->get_elem()->tfi == tfi)
                return utcb_list->get(i)->get_elem();
        }
        return NULL;
    }

    DTCB* DTbfMap::search(uchar tfi) {
        for ( int i=0 ; i<32 ;++i ) {
            if ( dtcb_list->get(i)->get_elem()->tfi == tfi)
                return dtcb_list->get(i)->get_elem();
        }
        return NULL;
    }

    DTCB* DTbfMap::search_by_imsi(ulong imsi1) {
        for ( int i=0 ; i<32 ;++i ) {
            if ( imsi[i] == imsi1)
                return dtcb_list->get(i)->get_elem();
        }
        return NULL;
    }

    DTCB* DTbfMap::get(uchar tfi) {
        ListElem1<DTCB>* res = dtcb_list->get(tfi);
        if (res)
            return res->get_elem();
        else
            return NULL;
    }

    UTCB* UTbfMap::get(uchar tfi) {
        ListElem1<UTCB>* res = utcb_list->get(tfi);
        if (res)
            return res->get_elem();
        else
            return NULL;
    }

    UTCB* UTbfMap::search_by_imsi(ulong imsi1) {
        for ( int i=0 ; i<32 ;++i ) {
            if ( imsi[i] == imsi1)
                return utcb_list->get(i)->get_elem();
        }
        return NULL;
    }

    DTCB* DTbfMap::search_by_tlli(ulong tlli1) {
        if ( tlli1 == 0 )
            return NULL;

        for ( int i=0 ; i<32 ;++i ) {
            if ( tlli[i] == tlli1)
                return dtcb_list->get(i)->get_elem();
        }
        return NULL;
    }

    UTCB* UTbfMap::search_by_tlli(ulong tlli1) {
        if ( tlli1 == 0 )
            return NULL;
        for ( int i=0 ; i<32 ;++i ) {
            if ( tlli[i] == tlli1)
                return utcb_list->get(i)->get_elem();
        }
        return NULL;
    }

    int DTbfMap::remove_entry( ulong tlli1 ) {

        for ( int i=0 ; i<32 ;++i ) {
            if ( tlli[i] == tlli1) {
                dtcb_list->del_entry(i);
                tlli[i] = 0;
            }
        }

        return 1;

    }

    int DTbfMap::remove_entry_by_tfi( uchar tfi ) {

        if ( tfi >= 32 ) {
            printf("Utbfmap::remove_entry_by_tfi(): tfi is out of range.\n");
            exit(1);
        }

        if ( dtcb_list->get(tfi) ) {
            dtcb_list->del_entry(tfi);
            tlli[tfi] = 0;
            imsi[tfi] = 0;
        }

        return 1;

    }

    int UTbfMap::remove_entry_by_tfi( uchar tfi ) {

        if ( tfi >= 32 ) {
            printf("Utbfmap::remove_entry_by_tfi(): tfi is out of range.\n");
            exit(1);
        }

        if ( utcb_list->get(tfi) ) {
            utcb_list->del_entry(tfi);
            tlli[tfi] = 0;
            imsi[tfi] = 0;
        }

        return 1;

    }

    int UTbfMap::remove_entry( ulong tlli1 ) {

        for ( int i=0 ; i<32 ;++i ) {
            if ( tlli[i] == tlli1) {
                utcb_list->del_entry(i);
                tlli[i] = 0;
                imsi[i] = 0;
            }
        }

        return 1;

    }

    int DTbfMap::remove( ulong tlli1 ) {

        for ( int i=0 ; i<32 ;++i ) {
            if ( tlli[i] == tlli1) {
                dtcb_list->del(i);
                tlli[i] = 0;
                imsi[i] = 0;
            }
        }

        return 1;

    }

    int UTbfMap::remove( ulong tlli1 ) {

        for ( int i=0 ; i<32 ;++i ) {
            if ( tlli[i] == tlli1) {
                utcb_list->del(i);
                tlli[i] = 0;
            }
        }

        return 1;

    }

    DTCB* DTbfMap::sel_tbf_to_send(uchar tn, uchar* ch_bitmap) {

        ListElem1<DTCB>* elem;
        ulong min_tx_cnt  = 0;
        int   candidate   = -1;

        min_tx_cnt = ~min_tx_cnt; /* maximum */

        for (int i=0 ; i<32 ;++i) {
            elem = dtcb_list->get(i);
            if (!elem)
                continue;
            else if ( selected[i] == true )
                continue;
            else {
                DTCB* dtcb = elem->get_elem();

                if ( !dtcb )
                    continue;

                uchar ta = dtcb->get_ta();

                if ( ((ta>>tn) & 0x01) && (ch_bitmap[dtcb->downlink_freq])) {
                    if ( min_tx_cnt > dtcb->get_tx_cnt() ) {
                        min_tx_cnt  = dtcb->get_tx_cnt();
                        candidate   = i;
                    }
                }
            }
        }

        if ( candidate == -1 )
            return NULL;
        else {
            selected[candidate]=true;
            return ((dtcb_list->get(candidate))->get_elem());
        }
    }

    UTCB* UTbfMap::sel_tbf_to_send(uchar tn, uchar* ch_bitmap) {

        ListElem1<UTCB>* elem;
        ulong min_tx_cnt  = 0;
        int   candidate   = -1;

        min_tx_cnt = ~min_tx_cnt; /* maximum */

        int selected_flag = false;

        /* so far , select in sequential manner */
        for ( int i=0 ; i<32 ; ++i ) {

            elem = utcb_list->get(i);
            if (!elem)
                continue;

            else {

                UTCB* utcb = elem->get_elem();
                uchar ta = utcb->get_ta();

                if ( !utcb )
                    continue;

                /* modified by jclin on 11/02/2004 to solve the problem that blocks of a TCB are triggered on
                 * the timeslots that are not assigned for that TCB.
                 */
                if ( utcb->ack_req && ((ta>>tn) & 0x01)  && (ch_bitmap[utcb->uplink_freq]) ) {

                    candidate       = i;
                    utcb->ack_req   = false;
                    selected_flag   = true;
                    break;

                }
            }
        }

        if ( !selected_flag ) {

            for (int i=0 ; i<32 ;++i ) {

                elem = utcb_list->get(i);

                if (!elem)
                    continue;

                else if ( selected[i] == true )
                    continue;

                else {

                    UTCB* utcb = elem->get_elem();
                    uchar ta = utcb->get_ta();

                    if ( ((ta>>tn) & 0x01)  && (ch_bitmap[utcb->uplink_freq]) ) {

                        if ( min_tx_cnt > utcb->get_tx_cnt() ) {
                            min_tx_cnt  = utcb->get_tx_cnt();
                            candidate   = i;
                        }
                    }
                }
            }
        }

        if ( candidate == -1 )
            return NULL;
        else {
            selected[candidate]=true;
            return ((utcb_list->get(candidate))->get_elem());
        }
    }

    UTbfMap::UTbfMap() {
        utcb_list = new Array<UTCB,static_cast<ulong> (32)>;
        bzero(imsi,sizeof(ulong)*32);
        bzero(tlli,sizeof(ulong)*32);
    }

    DTbfMap::DTbfMap() {
        dtcb_list = new Array<DTCB,static_cast<ulong> (32)>;
        bzero(imsi,sizeof(ulong)*32);
        bzero(tlli,sizeof(ulong)*32);
        bzero(selected,32);
    }

    int DTbfMap::insert(DTCB* dtcb, ulong imsi1, ulong tlli1) {

        if (dtcb->tfi>=32)
            return -1;

        else {

            int res = dtcb_list->insert(dtcb,static_cast<ulong> (dtcb->tfi));

            if ( res == -3 ) { /* not empty slot */
                printf("DTBFMAP::insert(): The slot to insert is not empty. \n");



                return res;
            }

            else if ( res < 0 )
                return res;

            else {

                imsi[dtcb->tfi] = imsi1;
                tlli[dtcb->tfi] = tlli1;
                return 1;

            }
        }
    }

    int UTbfMap::insert(UTCB* utcb, ulong imsi1, ulong tlli1) {

        if (utcb->tfi>=32)
            return -1;

        else {
            int res = utcb_list->insert(utcb,static_cast<ulong> (utcb->tfi));

            if ( res == -3 ) { /* not empty slot */
                printf("UTBFMAP::insert(): The slot to insert is not empty. \n");
                return res;
            }

            else if ( res < 0 )
                return res;

            else {
                imsi[utcb->tfi] = imsi1;
                tlli[utcb->tfi] = tlli1;
                return res;
            }
        }
    }

    int   UTbfMap::update_tlli(UTCB* utcb, ulong newtlli) {
        if (newtlli == 0 ) {
            cout <<"UTbfMap::update_tlli(): Warning: The new value of tlli is zero.\n" << endl;
            return -1;
        }
        utcb->tlli      = newtlli;
        tlli[utcb->tfi] = newtlli;
        return 1;
    }

    int   DTbfMap::update_tlli(DTCB* dtcb, ulong newtlli) {
        if (newtlli == 0 ) {
            cout <<"DTbfMap::update_tlli(): Warning: The new value of tlli is zero.\n" << endl;
            return -1;
        }
        dtcb->tlli      = newtlli;
        tlli[dtcb->tfi] = newtlli;
        return 1;
    }

    int RlcDownlinkDataBlockHeader::pack_header(uchar* ptr) {
    	if (!ptr) {
    	    printf("RLC Exception in pack_downlink_datablk_header(): ptr is null \n");
    	    return -1;
    	}

    	*ptr = ( (power_reduction << 6) | (tfi << 1) | fbi );
    	++ptr;
    	*ptr = ( (bsn << 1) | (ebit) );



    	return 1;
    }

    int RlcDownlinkDataBlockHeader::unpack_header(uchar* ptr) {
    	if (!ptr) {
    	    printf("RLC Exception in unpack_header(): a pointer pointing to header buffer is NULL\n");
    	    return -1;
    	}

    	uchar *tmp = ptr;

    	power_reduction = (*tmp >> 6)   & 0x03;
    	tfi             = (*tmp >> 1)   & 0x1f;
    	fbi             = *tmp          & 0x01;

    	++tmp;
    	bsn             = (*tmp >> 1)   & 0x7f;
    	ebit            = *tmp          & 0x01;
        return 1;
    }

    int RlcDownlinkControlBlockHeader::pack_header(uchar* ptr) {

    	if (!ptr) {
    	    printf("RLC Exception in pack_downlink_ctlblk_header(): ptr is null \n");
    	    return -1;
    	}

    	*ptr = ( (rbsn<<7) | (tfi<<2) | (fs<<1) | (ac) );
    	++ptr;
    	*ptr = ( (pr<<6) | (tfi<<1) | (dbit) );

    	return 1;
    }

    int RlcDownlinkControlBlockHeader::unpack_header(uchar* ptr) {
    	if (!ptr) {
    	    printf("RLC Exception in unpack_header(): a pointer pointing to header buffer is NULL\n");
    	    return -1;
    	}

    	uchar *tmp  = ptr;
        rbsn 	    = (*tmp >> 7)	& 0x01;
        rti		    = (*tmp >> 2)	& 0x1f;
        fs		    = (*tmp >> 1)	& 0x01;
        ac		    = *tmp 		    & 0x01;

    	++tmp;
    	pr		    = (*tmp >> 6)	& 0x03;
    	tfi		    = (*tmp >> 1)	& 0x1f;
    	dbit		= *tmp 		    & 0x01;

    	return 1;
    }

    int RlcUplinkDataBlockHeader::pack_header(uchar* ptr) {

    	if (!ptr) {
    	    printf("RLC Exception in pack_uplink_datablk_header(): ptr is null \n");
    	    return -1;
    	}
    	uchar spare_byte = 0x00;
    	*ptr = ( spare_byte | (pi<<6) | (tfi<<1) | (ti) );
    	++ptr;
    	*ptr = ( (bsn<<1 ) | ebit );

        #ifdef __PACK_UNPACK_FUNCTION_DEBUG
        cout << "RUDBH pack(): ptr[0] = " << static_cast<int> (*(ptr-1))
             << " ptr[1] = " << static_cast<int> (*ptr)
             << " bsn = " << (int)(bsn) << endl;

        #endif


    	return 1;
    }

    int RlcUplinkDataBlockHeader::unpack_header(uchar* ptr) {
    	if (!ptr) {
    	    printf("RLC Exception in unpack_header(): a pointer pointing to header buffer is NULL\n");
    	    return -1;
    	}

    	uchar *tmp = ptr;

    	pi		= (*tmp >> 6)   & 0x01;
    	tfi		= (*tmp >> 1)   & 0x1f;
    	ti		= *tmp          & 0x01;

    	++tmp;
    	bsn		= (*tmp >> 1)   & 0x7f;
    	ebit	= *tmp          & 0x01;

        #ifdef __PACK_UNPACK_FUNCTION_DEBUG
        cout << "RUDBH unpack(): ptr[0] = " << static_cast<int> (*(ptr))
             << " ptr[1] = " << static_cast<int> (*(ptr+1))
             << " bsn = " << (int)(bsn) << endl;
        #endif

    	return 1;
    }

