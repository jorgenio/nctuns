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

#include "emb_ie_net_entry_open.h"

#include "../frame_mngr.h"
#include "../mac802_16_meshss.h"
#include "../neighbor_mngr.h"
#include "../net_entry_mngr.h"
#include "../sch/data/scheduler.h"
#include "../../mac_address.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../../verbose.h"


using namespace Msh_ncfg;


Net_entry_open_ie::Net_entry_open_ie(Net_entry_open_ie* ie_p) 
: Embedded_data_hdr(ie_p)
{
      _fields = new fields_t;
      *_fields = *ie_p->_fields;
}

Net_entry_open_ie::Net_entry_open_ie(u_char pStart, u_char pRange, u_int32_t pFn,
        u_char pXmtCh, u_char pRcvCh, u_int32_t pValid, u_char pDelay)
: Embedded_data_hdr(TYPE_NETWORK_ENTRY_OPEN)
{
    setLength(sizeof(fields_t));

    _fields = new fields_t;
    bzero(_fields, sizeof(fields_t));
    _fields->msStart = pStart;
    _fields->msRange = pRange;
    _fields->msbFnum = pFn >> 4;
    _fields->lsbFnum = pFn & 0xff;
    _fields->XmtChannel = pXmtCh;
    _fields->RcvChannel = pRcvCh;
    _fields->msbSchValidity = pValid >> 4;
    _fields->lsbSchValidity = pValid & 0xff;
    _fields->EstPropDelay = pDelay;
}
        
Net_entry_open_ie::Net_entry_open_ie(void* pBuffer, int pLength)
: Embedded_data_hdr(pBuffer) {

    _fields = (fields_t*)((char*)pBuffer + hdr_len());
}

Net_entry_open_ie::~Net_entry_open_ie()
{
    if (_flag)
        delete _fields;
}

int
Net_entry_open_ie::copyTo(u_char* dstbuf)
{
    int pos;
    memcpy(dstbuf, Embedded_data_hdr::_fields, hdr_len());
    pos = hdr_len();
    memcpy(dstbuf + pos, _fields, sizeof(fields_t));
    pos += sizeof(fields_t);
    return pos;
}

void
Net_entry_open_ie::proc(mac802_16_MeshSS* mac, uint16_t tx_node_id)
{
    Neighbor* tx_node = mac->nbr_mngr()->neighbor(tx_node_id);
    
    if (mac->is_operational()) {
        /*
         * For other opertaional nodes which are nearby sponsor node.
         */
        schedule_alloc_t sch_alloc;
        _get_sch_info(sch_alloc);
        VINFO("%s: \e[0;31mOperational node: OpenIE from node %03u:\e[m\n",
                __func__, tx_node_id);
        VINFO_FUNC(sch_alloc.dump());
        if (!tx_node ||
                (!mac->data_scheduler()->set_nent_alloc_and_no_consider_busy_status(tx_node, sch_alloc, mac->fr_mngr()->cur_frame()))) {

            WARN("%s: Fully-functional node cannot find a corresponding allocation, tx_node_id = %d\n",
                    __func__, tx_node_id);
            /*
             * In new bitmap scheduler, we consider the situation will not occur any more.
             * FIXME?
             assert(0);
             */
        }
    } else {
        /*
         * For the new node.
         */
        Mac_address net_entry_mac_addr;
        _ncfg->getEntryAddress(net_entry_mac_addr);
        if (net_entry_mac_addr != *mac->address())
            return;

        if (!mac->net_entry_mngr()->proc_ncfg_network_entry_open(tx_node_id))
            return;

        mac->net_entry_mngr()->send_sbcreq();

        schedule_alloc_t sch_alloc;
        _get_sch_info(sch_alloc);
        VINFO("%s: msStart = %d, msRange = %d, Fnum = %#x, validity = %d \n",
                __func__,
                sch_alloc.slot_start(), sch_alloc.slot_range(),
                sch_alloc.frame_start(), sch_alloc.frame_range());

        if (mac->data_scheduler()->set_nent_alloc_and_consider_busy_status(
                    tx_node, sch_alloc, mac->fr_mngr()->cur_frame()) == false) {

            FATAL("%s: New node cannot find a corresponding allocation.\n",
                    __func__);
        }

        /*
         * Perform fine time synchronization.
         */
        uint16_t    dummy_frame_no;
        uint8_t     slot_no;
        uint8_t     dummy_sync_hop_cnt;
        _ncfg->getTimeStamp(dummy_frame_no, slot_no, dummy_sync_hop_cnt);
        mac->find_time_sync(slot_no, getDelay());
    }
}

void
Net_entry_open_ie::dump()
{
    /*
     * FIXME
     */
    assert(0);
}

void
Net_entry_open_ie::_get_sch_info(class Alloc_base& sch_alloc)
{
    sch_alloc.slot_start(_fields->msStart);
    sch_alloc.slot_range(_fields->msRange);
    sch_alloc.frame_start(_fields->msbFnum << 4 | _fields->lsbFnum);
    sch_alloc.frame_range(_fields->msbSchValidity << 4 | _fields->lsbSchValidity);
}
