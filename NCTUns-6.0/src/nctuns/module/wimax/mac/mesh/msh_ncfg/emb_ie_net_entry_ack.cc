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

#include "emb_ie_net_entry_ack.h"

#include "../frame_mngr.h"
#include "../mac802_16_meshss.h"
#include "../neighbor_mngr.h"
#include "../net_entry_mngr.h"
#include "../sch/ctrl/ht_assigner.h"
#include "../sch/ctrl/info.h"
#include "../sch/ctrl/scheduler.h"
#include "../sch/data/scheduler.h"
#include "../util_map.h"
#include "../../mac_address.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../../verbose.h"


using namespace Msh_ncfg;


Net_entry_ack_ie::Net_entry_ack_ie()
: Embedded_data_hdr(TYPE_NETWORK_ENTRY_ACK)
{
    setLength(0);
}

Net_entry_ack_ie::Net_entry_ack_ie(void* pBuffer, int pLength)
: Embedded_data_hdr(pBuffer)
{
}

Net_entry_ack_ie::~Net_entry_ack_ie()
{
}

int
Net_entry_ack_ie::copyTo(u_char* dstbuf)
{
    memcpy(dstbuf, Embedded_data_hdr::_fields, hdr_len());
    return hdr_len();
}

void
Net_entry_ack_ie::proc(mac802_16_MeshSS* mac, uint16_t tx_node_id)
{
    if (mac->is_operational()) {

        Neighbor* tx_node = mac->nbr_mngr()->neighbor(tx_node_id);

        if (!tx_node || !mac->data_scheduler()->free_nent_alloc(
                    tx_node_id, Data_scheduler::RECV_NET_ENTRY) ) {

            WARN("One of the neighbors cannot find a corresponding allocation to free\n");
        }
    } else {

        Mac_address net_entry_mac_addr;
        _ncfg->getEntryAddress(net_entry_mac_addr);
        if (net_entry_mac_addr != *mac->address())
            return;

        if (!mac->net_entry_mngr()->proc_ncfg_network_entry_ack(tx_node_id))
            return;

        mac->set_operational();

        STAT("[%03u]Net_entry_ack_ie::%s: recv NetEntryAck, Set operational, "
                "Sponsor Channel Close.\n",
                mac->node_id(), __func__);

        if (mac->data_scheduler()->free_nent_alloc(
                    tx_node_id, Data_scheduler::RECV_NET_ENTRY) == false) {

            FATAL("New node cannot find a corresponding allocation to free (Entry ACK)\n");
        }

        ASSERT((!mac->net_entry_mngr()->is_register()),
                "Node's registration procedure has not yet finished "
                "but the sponsor channel has been closed.\n");

        /*
         * If the registration procedure is finished,
         * SS should start to schedule the transmissions of
         * its MSH-NCFG and MSH-DSCH messages.
         */

        Frame_mngr* fr_mngr = mac->fr_mngr();

        /*
         * Schedule the first MSH-NCFG transmission.
         */
        mac->ncfg_scheduler()->start(fr_mngr->ncfg_opp_seq_end(), fr_mngr->max_ncfg_tx_opp());
        mac->ncfg_scheduler()->update_next_tx_opp();

        /*
         * Schedule the first MSH-DSCH transmission.
         */
        mac->dsch_scheduler()->start(fr_mngr->dsch_opp_seq_end(), fr_mngr->max_dsch_tx_opp());
        mac->dsch_scheduler()->update_next_tx_opp();

        update_node_status(TxOppUtilizationCounter::msh_ncfg,
                mac->node_id(), node_stat_blk_t::functional);
        update_node_status(TxOppUtilizationCounter::msh_dsch,
                mac->node_id(), node_stat_blk_t::functional);

        STAT("[%03u]Net_entry_ack_ie::%s: "
                "\e[1;35mfinishes the network entry procedure\e[m, "
                "1st NCFG tx_opp = %u, 1st DSCH tx_opp = %u\n",
                mac->node_id(), __func__,
                mac->ncfg_scheduler()->next_tx_opp(),
                mac->dsch_scheduler()->next_tx_opp());
    }
}

void
Net_entry_ack_ie::dump()
{
    /*
     * FIXME: implement me.
     */
    assert(0);
}
