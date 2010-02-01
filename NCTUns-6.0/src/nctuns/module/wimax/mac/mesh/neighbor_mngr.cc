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

#include "neighbor_mngr.h"

#include "frame_mngr.h"
#include "mac802_16_meshss.h"
#include "msh_dsch.h"
#include "msh_ncfg.h"
#include "sch/ctrl/info.h"
#include "sch/ctrl/scheduler.h"
#include "util_map.h"


#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


/*
 * Member function definitions of class `Neighbor_mngr'.
 */

/*
 * Constructor
 */
Neighbor_mngr::Neighbor_mngr()
{
	_neighbors.clear();
}


/*
 * Destructor
 */
Neighbor_mngr::~Neighbor_mngr()
{
    for (map_t::iterator it = _neighbors.begin(); it != _neighbors.end(); it++)
        delete it->second;
}

void
Neighbor_mngr::init(uint32_t tcl_node_id)
{
    /*
     * For statistics of txopp uses.
     */
    register_neighbor_list(
            TxOppUtilizationCounter::msh_ncfg, tcl_node_id, this);
    register_neighbor_list(
            TxOppUtilizationCounter::msh_dsch, tcl_node_id, this);
}

bool
Neighbor_mngr::add_neighbor(Neighbor* nbr)
{
    if (neighbor(nbr->node_id()))
        return false;

    _neighbors[nbr->node_id()] = nbr;

    return true;
}

Neighbor*
Neighbor_mngr::neighbor(uint16_t node_id)
{
	map_t::const_iterator it = _neighbors.find(node_id);

	if (it == _neighbors.end())
		return NULL;

	return it->second;
}

Neighbor*
Neighbor_mngr::neighbor_with_link_rx_id(uint8_t link_id)
{
    for (map_t::iterator it = _neighbors.begin();
            it != _neighbors.end(); it++) {

        Neighbor* nbr = it->second;
        if (nbr->rx_link() && nbr->rx_link()->id() == link_id)
            return nbr;
    }
    return NULL;
}

void
Neighbor_mngr::update_neighbors(mac802_16_MeshSS* mac,
        Ctrl_sch::Entry_interface& sch_info_if, uint16_t tx_node_id)
{
    mac->fr_mngr()->fix_prop_delay();

    /*
     * FIXME: So ugly...
     */
    uint32_t cur_tx_opp;
    uint32_t max_tx_opp;
    MSH_NCFG* ncfg = NULL;
    MSH_DSCH* dsch = NULL;
    Ctrl_scheduler* scheduler;
    u_char sync_hop_count;
    if ((ncfg = dynamic_cast<MSH_NCFG*>(&sch_info_if))) {

        uint16_t dummy_frame_no;
        u_char slot_no;
        ncfg->getTimeStamp(dummy_frame_no, slot_no, sync_hop_count);

        cur_tx_opp = mac->fr_mngr()->cur_ncfg_tx_opp(slot_no);
        max_tx_opp = mac->fr_mngr()->max_ncfg_tx_opp();
        scheduler = mac->ncfg_scheduler();

    } else if ((dsch = dynamic_cast<MSH_DSCH*>(&sch_info_if))) {

        cur_tx_opp = mac->fr_mngr()->cur_dsch_tx_opp();
        max_tx_opp = mac->fr_mngr()->max_dsch_tx_opp();
        scheduler = mac->dsch_scheduler();

    } else assert(0);

    DEBUG("[%03u]%s: cur_tx_opp=%d, max_tx_opp=%d, ncfg_opp_seq_end = %u\n",
            mac->node_id(), __func__,
            cur_tx_opp, max_tx_opp,
            mac->fr_mngr()->ncfg_opp_seq_end());

    /*
     * Update the transmitting node's information.
     */
    Neighbor* tx_node = neighbor(tx_node_id);

    if (tx_node)
        tx_node->set_hop_count(1);

    else {

        tx_node = new Neighbor(tx_node_id, 1);

        tx_node->set_link_sent_challenge();

        assert(add_neighbor(tx_node));

        INFO("[%03u]%s: add 1-hop neighbor with ID: %u\n",
                mac->node_id(), __func__, tx_node->node_id());
    }

    if (ncfg)
        tx_node->set_sync_hop_count(sync_hop_count);

    Ctrl_sch::Entry sch_entry;
    sch_info_if.get_sch_param(sch_entry);

    Ctrl_sch::Info_nbr* sch_info = scheduler->nbr_sch_info(tx_node_id);

    sch_info->set_reported(false);

    if (mac->is_op_params_obtained())
        sch_info->update(sch_entry, cur_tx_opp, max_tx_opp);

#if 0
    if (mac->is_op_params_obtained() && max_tx_opp < 20000 &&
            (mac->node_id() == 1 || mac->node_id() == 8)) {

        INFO("[%03u]%s: tx_node = %u: mx = %u, exp = %u, ", 
                mac->node_id(), __func__,
                tx_node_id,
                sch_entry.next_tx_mx(),
                sch_entry.tx_holdoff_exp());
        INFO_FUNC(sch_info->dump());
    }
#endif

    /*
     * Update the neighborhood information contained in MSH-NCFG.
     */
    for (int i = 0; i < sch_info_if.get_nf_nbr_sch_entry(); i++) {

        Ctrl_sch::Entry nbr_sch_entry;
        sch_info_if.get_nbr_sch_entry(nbr_sch_entry, i);

        /*
         * By-pass the neighbor entry standing for the node itself.
         */
        if (nbr_sch_entry.node_id() == mac->node_id())
            continue;

        Neighbor* nbr = neighbor(nbr_sch_entry.node_id());
        if (!nbr) {

            nbr = new Neighbor(nbr_sch_entry.node_id());

            nbr->set_link_sent_challenge();

            assert(add_neighbor(nbr));

            INFO("[%03u]%s: add neighbor with ID: %u\n",
                    mac->node_id(), __func__, nbr->node_id());
        }

        if (nbr_sch_entry.is_valid_hops_to_nbr() &&
                nbr_sch_entry.hops_to_nbr() + 2 < nbr->hop_count())
            nbr->set_hop_count(nbr_sch_entry.hops_to_nbr() + 2);

        Ctrl_sch::Info_nbr* sch_info = scheduler->nbr_sch_info(nbr_sch_entry.node_id());
        if (mac->is_op_params_obtained())
            sch_info->update(nbr_sch_entry, cur_tx_opp, max_tx_opp);
#if 0
        if (mac->is_op_params_obtained() && max_tx_opp < 20000 &&
                (mac->node_id() == 1 || mac->node_id() == 8)) {

                INFO("[%03u]%s: Nbr(%03u): mx = %u, exp = %u, ", 
                        mac->node_id(), __func__,
                        nbr_sch_entry.node_id(),
                        nbr_sch_entry.next_tx_mx(),
                        nbr_sch_entry.tx_holdoff_exp());
                INFO_FUNC(sch_info->dump());
            }
#endif

        sch_info->set_reported(false);
    }
}


uint8_t
Neighbor_mngr::max_rx_link_id()
{
    uint8_t id_max = 0;
    for (map_t::iterator it = _neighbors.begin();
            it != _neighbors.end(); it++) {

        Mesh_link* link = it->second->rx_link();
        if (link && link->id() > id_max)
            id_max = link->id();
    }

    return id_max;
}

size_t
Neighbor_mngr::nf_active_link()
{
    size_t nf_active_link = 0;

    for (map_t::iterator it = _neighbors.begin();
            it != _neighbors.end(); it++) {

        Neighbor* nbr = it->second;

        if (nbr->is_link_active())
            nf_active_link++;
    }
    return nf_active_link;
}

size_t
Neighbor_mngr::nf_one_hop_nbrs()
{
    size_t nf_nbrs = 0;

    for (map_t::iterator it = _neighbors.begin();
            it != _neighbors.end(); it++) {

	    if (it->second->hop_count() == 1)
		nf_nbrs++;
    }
    return nf_nbrs;
}

void
Neighbor_mngr::dump()
{
    if (_neighbors.empty())
        return;

    printf("\tNeighbor_mngr::%s:\n", __func__);
    for (map_t::iterator it = _neighbors.begin();
            it != _neighbors.end(); it++)
        it->second->dump();
}
