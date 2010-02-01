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

#include "scheduler.h"

#include "entry_interface.h"
#include "ht_assigner.h"
#include "info.h"
#include "../../mac802_16_meshss.h"
#include "../../neighbor.h"
#include "../../neighbor_mngr.h"

#define VERBOSE_LEVEL           MSG_INFO
#include "../../../verbose.h"


/*
 * Only for debug, don't use this to determine which type the scheduler
 * belongs to via this macro.
 */
#define IS_MSH_NCFG         (max_tx_opp < 20000)
#define PRINF_SCHED_INFO    0


/*
 * Member function definitions of class `Ctrl_scheduler'.
 */

/*
 * Constructor
 */
Ctrl_scheduler::Ctrl_scheduler(mac802_16_MeshSS* mac)
: _mac(mac)
, _ht_assigner(NULL)
, _collect_node(NULL)
, _next_tx_opp(Ctrl_sch::Info::invalid_tx_opp())
, _max_tx_opp(0)
{
    _nbr_sch_infos.clear();
}

/*
 * Destructor
 */
Ctrl_scheduler::~Ctrl_scheduler()
{
    for (nbr_sch_map_t::iterator it = _nbr_sch_infos.begin();
            it != _nbr_sch_infos.end(); it++)
        delete it->second;

    if (_ht_assigner)
        delete _ht_assigner;
}

void
Ctrl_scheduler::init()
{
    _init_ht_assigner();
}

void
Ctrl_scheduler::start(uint32_t first_tx_opp, uint32_t max_tx_opp)
{
    assert(_ht_assigner && !_collect_node);
    _collect_node = &Ctrl_scheduler::_collect_all_node;

    _next_tx_opp = first_tx_opp;
    _max_tx_opp = max_tx_opp;
}

uint32_t
Ctrl_scheduler::update_next_tx_opp()
{
    _next_tx_opp = get_next_tx_opp(_ht_assigner->sch_info());
    return _next_tx_opp;
}

uint32_t
Ctrl_scheduler::get_next_tx_opp(const Ctrl_sch::Info* sch_info)
{
    /*
     * We disable the following function call because:
     *   (1) it may change the order of entries in neighborList, which will
     *       in turn complicate the link establishment process.
     *   (2) it seems O.K. to be removed.
     *
     *  Order the physical neighbor table by the Next Xmt Time.
     *  _sort(neighbor_list);
     */

    uint32_t tmp_tx_opp = _next_tx_opp + sch_info->tx_holdoff_opp();

#if PRINF_SCHED_INFO
    if (IS_MSH_NCFG && (_mac->node_id() == 1 || _mac->node_id() == 8)) {
        INFO("[%03u]%s: max_tx_opp = %u, tmp_tx_opp = %u, old next_tx_opp = %u, tx_holdoff_opp = %u\n",
                _mac->node_id(), __func__,
                _max_tx_opp, tmp_tx_opp,
                sch_info->next_tx_opp(), sch_info->tx_holdoff_opp());

        INFO("[%03u]%s: Dump potential competing nodes:\n",
                _mac->node_id(), __func__);
        INFO_FUNC(_mac->nbr_mngr()->dump());
        INFO("end of potential competing node list.\n\n");
    }
#endif

    node_id_list_t node_id_list;
    bool success = false;
    while (1) {

        if (tmp_tx_opp > _max_tx_opp)
            tmp_tx_opp %= _max_tx_opp;

        node_id_list.clear();

        (this->*(_collect_node))(node_id_list, tmp_tx_opp, _max_tx_opp);

#if PRINF_SCHED_INFO
        if (IS_MSH_NCFG && (_mac->node_id() == 1 || _mac->node_id() == 8)) {
            INFO("[%03u]%s: Competing for tmp_tx_opp = %u node_id_list: (size = %d) ",
                    _mac->node_id(), __func__, tmp_tx_opp, node_id_list.size());

            for (node_id_list_t::iterator it = node_id_list.begin();
                    it != node_id_list.end(); it++) {

                printf("%d ", *it);
            }
            printf("\n");
        }
#endif

        // Hold a Mesh Election.
        success = _mesh_election(tmp_tx_opp, node_id_list);

        if (success) {


#if PRINF_SCHED_INFO
            if (IS_MSH_NCFG && (_mac->node_id() == 1 || _mac->node_id() == 8))
                INFO("[%03u]%s: chosen tmp_tx_opp = \e[1;36m%u\e[m\n",
                        _mac->node_id(), __func__, tmp_tx_opp);
#endif
            return tmp_tx_opp;
        }

        tmp_tx_opp++;
    }
    return _next_tx_opp;
}

Ctrl_sch::Info_nbr*
Ctrl_scheduler::nbr_sch_info(uint16_t tx_node_id)
{
    Ctrl_sch::Info_nbr* sch_info = _nbr_sch_info(tx_node_id);
    if (!sch_info) {

        Neighbor* tx_node = _mac->nbr_mngr()->neighbor(tx_node_id);

        if (!tx_node) {

            tx_node = new Neighbor(tx_node_id, 1);

            tx_node->set_link_sent_challenge();

            assert(_mac->nbr_mngr()->add_neighbor(tx_node));

            INFO("[%03u]%s: Add neighbor (ID = %u)\n",
                    _mac->node_id(), __func__, tx_node_id);
        }

        sch_info = new Ctrl_sch::Info_nbr(
                _ht_assigner->sch_info()->tx_holdoff_exp_base(),
                _ht_assigner->sch_info()->tx_holdoff_exp(),
                tx_node);

        _nbr_sch_infos[tx_node_id] = sch_info;
    }
    return sch_info;
}

bool
Ctrl_scheduler::is_valid_next_tx_opp() const
{
    return Ctrl_sch::Info::is_valid_tx_opp(_next_tx_opp);
}

/*
 * TODO: Following parameters should be read from tcl file.
 */
#include "../../mac802_16_mesh_mode_sys_param.h"
void
Ctrl_scheduler::_init_ht_assigner()
{
    Ctrl_sch::Ht_assigner::ht_assignment_mode_t holdoff_time_assignment_mode;
    Ctrl_sch::Ht_assigner::ht_function_mode_t holdoff_time_func_mode;

    holdoff_time_assignment_mode    = Ctrl_sch::Ht_assigner::assigned_by_default_value;
    holdoff_time_func_mode	    = Ctrl_sch::Ht_assigner::undefined;

    const Ctrl_sch::Info sch_info(DEFAULT_HOLDOFF_EXP_BASE, DEFAULT_TX_HOLDOFF_EXP);

    _ht_assigner = new Ctrl_sch::Ht_assigner(
		    this, holdoff_time_assignment_mode, holdoff_time_func_mode, sch_info);
}

void
Ctrl_scheduler::_collect_all_node(
        node_id_list_t& node_id_list, uint32_t tmp_tx_opp, uint32_t max_tx_opp)
{
    const Neighbor_mngr::map_t& neighbors = _mac->nbr_mngr()->neighbors();

    for (Neighbor_mngr::map_t::const_iterator it = neighbors.begin();
            it != neighbors.end(); it++) {
        node_id_list.push_back(it->second->node_id());
    }
    _collect_node = &Ctrl_scheduler::_collect_eligible_node;
}

/*
 * Determine the eligible competing nodes.
 */
void
Ctrl_scheduler::_collect_eligible_node(
        node_id_list_t& node_id_list, uint32_t tmp_tx_opp, uint32_t max_tx_opp)
{
    const Neighbor_mngr::map_t& neighbors = _mac->nbr_mngr()->neighbors();

    for (Neighbor_mngr::map_t::const_iterator it = neighbors.begin();
            it != neighbors.end(); it++) {

        Neighbor* nbr = it->second;

        if (nbr_sch_info(nbr->node_id())->is_eligible(tmp_tx_opp, max_tx_opp))
            node_id_list.push_back(nbr->node_id());
    }
}

#if 0
void Ctrl_scheduler::_sort(vector<Neighbor*> &neighbor_list)
{
    sort(neighbor_list.begin(), neighbor_list.end(), compareNCFG);
}
#endif

/*
 * The Mesh Election procedure determines whether the local node is
 * the winner for a specific TempXmtTime among all the competing nodes.
 * It returns TRUE, if the local node wins, or otherwise FALSE.
 */
bool
Ctrl_scheduler::_mesh_election(
        uint32_t tx_opp, const node_id_list_t& node_id_list)
{
    uint16_t node_id = _mac->node_id();

    uint32_t smear_val1 = _smear(node_id ^ tx_opp);
    uint32_t smear_val2 = _smear(node_id + tx_opp);

    for (node_id_list_t::const_iterator it = node_id_list.begin();
            it != node_id_list.end(); it++) {

        uint16_t nbrsNodeID = *it;
        uint32_t nbr_smear_val = _smear(nbrsNodeID ^ tx_opp);

        if (nbr_smear_val > smear_val1) {

            DEBUG("_mesh_election(): nbr %u wins txopp %u. (nbr_smear=%u, my_smear1=%u)\n",
                    nbrsNodeID, tx_opp, nbr_smear_val, smear_val1);

            return false;	// This node loses.

        } else if (nbr_smear_val == smear_val1) {
            /*
             * 1st tie-breaker
             */
            nbr_smear_val = _smear(nbrsNodeID + tx_opp);

            if (nbr_smear_val > smear_val2) {

                DEBUG("%s: nbr %u wins txopp %u. (nbr_smear=%u, my_smear2=%u)\n",
                        __func__, nbrsNodeID, tx_opp, nbr_smear_val, smear_val2);

                return false;	// This node loses.

            } else if (nbr_smear_val == smear_val2) {
                /*
                 * If we still collide at this point, break
                 * the tie based on MacAddr.
                 */
                if ((tx_opp % 2 == 0 && nbrsNodeID > node_id) ||
                    (tx_opp % 2 == 1 && nbrsNodeID < node_id)) {

                    DEBUG("%s: nbr %u wins txopp %u by tie_break."
                            " (nbr_smear=%u, my_smear2=%u.)\n",
                            __func__, nbrsNodeID, tx_opp, nbr_smear_val, smear_val2);

                    return false;	// This node loses.
                }
            }
        }
        // This node won over this competing node.

    }// End for all competing nodes.

    //  This node is winner, it won over all competing nodes.

    DEBUG("%s: node %u wins txopp %u. (my_smear1=%u, mysmear2=%u)\n",
            __func__, node_id, tx_opp, smear_val1, smear_val2);

    return true;
}

uint32_t
Ctrl_scheduler::_smear(uint16_t val)
{
    val += (val << 12);
    val ^= (val >> 22);
    val += (val << 4);
    val ^= (val >> 9);
    val += (val << 10);
    val ^= (val >> 2);
    val += (val << 7);
    val ^= (val >> 12);

    return val;
}

Ctrl_sch::Info_nbr*
Ctrl_scheduler::_nbr_sch_info(uint16_t node_id)
{
	nbr_sch_map_t::const_iterator it = _nbr_sch_infos.find(node_id);

	if (it == _nbr_sch_infos.end())
		return NULL;

	return it->second;
}
