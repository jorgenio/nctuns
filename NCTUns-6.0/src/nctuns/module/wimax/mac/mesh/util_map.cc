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

#include <math.h>
#include <config.h>
#include <nctuns_api.h>
#include "mac802_16_mesh_mode_sys_param.h"
#include "neighbor_mngr.h"
#include "util_map.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


#if __RECORD_TXOPP_USE__

/* scheduling frame is set to 2 now. */
u_int32_t net_ctrl_frame_num = (1 + (4096)/(2*4+1));
u_int32_t num_of_ncfg_txopp_per_frame = 8-1;
u_int32_t num_of_dsch_txopp_per_frame = 8;
u_int32_t num_of_ss_node = TOTAL_MESH_SS_NODES;


txopp_util_counter_t global_ncfg_txopp_util_counter
    (TxOppUtilizationCounter::msh_ncfg, num_of_ss_node, net_ctrl_frame_num*num_of_ncfg_txopp_per_frame );

txopp_util_counter_t global_dsch_txopp_util_counter
    (TxOppUtilizationCounter::msh_dsch, num_of_ss_node, (4096-net_ctrl_frame_num)*num_of_dsch_txopp_per_frame );

int record_used_txopp(TxOppUtilizationCounter::txopp_type_t txopp_type,
    u_int32_t nid, u_int32_t txopp_num) {

    int res = 0;
    if (txopp_type == TxOppUtilizationCounter::msh_ncfg) {
        
        res = global_ncfg_txopp_util_counter.record_used_txopp(nid, txopp_num);
        VINFO("record_used_txopp: node[%u] uses MSH-NCFG txopp = %u.\n", nid, txopp_num);
    
    }
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch) {
    
        res = global_dsch_txopp_util_counter.record_used_txopp(nid, txopp_num);
        VINFO("record_used_txopp: node[%u] uses MSH-DSCH txopp = %u.\n", nid, txopp_num);

    }
    else {

        FATAL("[%u]: record_used_txopp: unknown type (%u)", nid, txopp_type);

    }
    
    return res;

}

int
register_neighbor_list(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, Neighbor_mngr* nbr_mngr)
{
    if (txopp_type == TxOppUtilizationCounter::msh_ncfg)
        return global_ncfg_txopp_util_counter.register_neighbor_list(nid, nbr_mngr);
        
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch)
        return global_dsch_txopp_util_counter.register_neighbor_list(nid, nbr_mngr);

    else {

        FATAL("[%u]: register_neighbor_list: unknown type (%u)", nid, txopp_type);

    }
    
    return 1;

}

int update_node_status(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, node_stat_blk_t::node_status_t status1 ) {

    if (txopp_type == TxOppUtilizationCounter::msh_ncfg)
        return global_ncfg_txopp_util_counter.update_node_status(nid, status1);
        
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch)
        return global_dsch_txopp_util_counter.update_node_status(nid, status1);

    else {

        FATAL("[%u]: update_node_status: unknown type (%u)", nid, txopp_type);

    }
    
    return 1;
}

int dump_node_txopp_utilization(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    if (txopp_type == TxOppUtilizationCounter::msh_ncfg)
        return global_ncfg_txopp_util_counter.dump_node(nid);
        
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch)
        return global_dsch_txopp_util_counter.dump_node(nid);

    else {

        FATAL("[%u]: dump_node_txopp_utilization: unknown type (%u)", nid, txopp_type);

    }

    return 1;

}

int dump_all_txopp_utilization() {

    global_ncfg_txopp_util_counter.dump();
    global_dsch_txopp_util_counter.dump();
    return 1;
}

u_int32_t get_total_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    if (txopp_type == TxOppUtilizationCounter::msh_ncfg)
        return global_ncfg_txopp_util_counter.get_total_txopps(nid);
        
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch)
        return global_dsch_txopp_util_counter.get_total_txopps(nid);

    else {

        FATAL("[%u]: get_total_txopps: unknown type (%u)", nid, txopp_type);

    }

}


u_int32_t get_idle_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    if (txopp_type == TxOppUtilizationCounter::msh_ncfg)
        return global_ncfg_txopp_util_counter.get_idle_txopps(nid);
        
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch)
        return global_dsch_txopp_util_counter.get_idle_txopps(nid);

    else {

        FATAL("[%u]: get_idle_txopps: unknown type (%u)", nid, txopp_type);

    }

}


u_int32_t get_won_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    if (txopp_type == TxOppUtilizationCounter::msh_ncfg)
        return global_ncfg_txopp_util_counter.get_won_txopps(nid);
        
    else if (txopp_type == TxOppUtilizationCounter::msh_dsch)
        return global_dsch_txopp_util_counter.get_won_txopps(nid);

    else {

        FATAL("[%u]: get_won_txopps: unknown type (%u)", nid, txopp_type);

    }

}

#else

int record_used_txopp(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, u_int32_t txopp_num) {
        
    return 1;
        
}
int register_neighbor_list(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, std::vector<Neighbor*> *nblist ) {
        
    return 1;
        
}
int update_node_status(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, node_stat_blk_t::node_status_t status1 ) {
        
    return 1;
        
}

int dump_node_txopp_utilization(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    return 1;

}

int dump_all_txopp_utilization() {

    return 1;

}

u_int32_t get_total_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    return 1;

}

u_int32_t get_idle_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    return 1;

}

u_int32_t get_won_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid) {

    return 1;

}


#endif

u_int32_t UtilizationMap::get_start_txopp_num() {

    return start_txopp_num;

}

u_int32_t UtilizationMap::get_end_txopp_num() {

    return end_txopp_num;

}

u_int32_t UtilizationMap::get_size() {

    return size;

}


UtilizationMap::UtilizationMap(u_int32_t size1, u_int32_t node_num1, 
    u_int32_t start_txopp_num1, u_int32_t end_txopp_num1, u_int32_t max_txopp_num1) {
    
    state           = initialized;
    size            = size1;
    node_num        = node_num1;
    start_txopp_num = start_txopp_num1;
    end_txopp_num   = end_txopp_num1;
    max_txopp_num   = max_txopp_num1;
    
    map = new char*[node_num];
    
    for (u_int32_t i=0; i<node_num ;++i) {
    
        map[i] = new char[size];
        memset(map[i], 0, sizeof(char)*size);
    
    }

}

UtilizationMap::~UtilizationMap() {

    for ( u_int32_t i=0; i<node_num ;++i) {
    
        if (map[i])
            delete[] map[i];
            
    }

    delete[] map;
}


int UtilizationMap::check_range(u_int32_t txopp_num) {

    u_int32_t tmp_end_txopp_num;
    if (end_txopp_num < start_txopp_num)
        tmp_end_txopp_num = end_txopp_num + max_txopp_num;
    else
        tmp_end_txopp_num = end_txopp_num;
        
    if (txopp_num<start_txopp_num &&
        ((start_txopp_num - txopp_num) > (max_txopp_num/2)) )
        txopp_num += max_txopp_num;
    
    
    if ( (txopp_num >= start_txopp_num) && (txopp_num <= tmp_end_txopp_num) ) {
        
        VINFO("YES: start_tx=%u, end_tx=%u, tmp_end_tx=%u, given_txopp=%u.\n", 
        start_txopp_num, end_txopp_num, tmp_end_txopp_num, txopp_num);
        return 1;
    
    }
    else {
        
        VINFO("NO: start_tx=%u, end_tx=%u, tmp_end_tx=%u, given_txopp=%u.\n", 
        start_txopp_num, end_txopp_num, tmp_end_txopp_num, txopp_num);
        return 0;
    }
}

int UtilizationMap::set_use_of_the_given_txopp(u_int32_t nid, u_int32_t txopp_num) {

    ASSERT(nid<=node_num, "illegal nid = %u (MAX nid=%u).\n", nid, node_num);

    u_int32_t adjusted_nid          = nid - 1;
    u_int32_t adjusted_txopp_num    = 0;

    if (txopp_num>= start_txopp_num )
        adjusted_txopp_num = txopp_num - start_txopp_num;

    else if ( txopp_num < (start_txopp_num/2) )
        adjusted_txopp_num = txopp_num + max_txopp_num - start_txopp_num;

    else {
        
        ERROR("[%u]: txopp_num=%u, adjusted_txopp_num=%u.\n",
        nid, txopp_num, adjusted_txopp_num);
    }
    
    map[adjusted_nid][adjusted_txopp_num] = 1;
    return 1;

}

int UtilizationMap::check_use_of_the_given_txopp(u_int32_t nid, u_int32_t txopp_num) {

    ASSERT(nid<=node_num, "illegal nid = %u (MAX nid=%u).\n", nid, node_num);
    /*ASSERT( ((txopp_num>=start_txopp_num) && (txopp_num<=start_txopp_num+size)),
        "illegal range of the given txopp = %u, (start: %u, end: %u)",
        txopp_num, start_txopp_num, end_txopp_num );*/
   
    u_int32_t adjusted_nid          = nid - 1;

    u_int32_t adjusted_txopp_num    = 0;
    
    if (txopp_num>= start_txopp_num )
        adjusted_txopp_num = txopp_num - start_txopp_num;

    else if ( txopp_num < (start_txopp_num/2) )
        adjusted_txopp_num = txopp_num + max_txopp_num - start_txopp_num;

    else {
        
        ERROR("[%u]: txopp_num=%u, adjusted_txopp_num=%u.\n",
        nid, txopp_num, adjusted_txopp_num);
    }
    
    
    return map[adjusted_nid][adjusted_txopp_num];

}



    
NodeStatBlock::NodeStatBlock()
: node_status(none)
, nid(0)
, _nbr_mngr(NULL)
, idle_txopps(0)
, won_txopps(0)
, total_txopps(0)
{
}

int NodeStatBlock::set_nid(u_int32_t nid1) {

    nid = nid1;
    return 1;

}

int
NodeStatBlock::set_neighbor_mngr(Neighbor_mngr* nbr_mngr) {

    _nbr_mngr = nbr_mngr;
    return 1;
}

int NodeStatBlock::inc_idle_txopps() {

    ++idle_txopps;
    return 1;

}

int NodeStatBlock::inc_won_txopps() {

    ++won_txopps;
    return 1;

}

int NodeStatBlock::inc_total_txopps() {

    ++total_txopps;
    return 1;

}

int NodeStatBlock::set_status(NodeStatBlock::node_status_t status1) {

    node_status = status1;
    return 1;

}

int NodeStatBlock::dump() {

    double idle_ratio = 0.0;

    if (total_txopps > 0.0) {

        idle_ratio = static_cast<double>(idle_txopps)/ static_cast<double>(total_txopps);

    }

    
    
    printf("[%u]Dump NodeStatBlock: idle_txopp_num = %u, total_txopp_num = %u, idle_ratio=%lf \n",
        nid, idle_txopps, total_txopps, idle_ratio );
    
    return 1;

}

int NodeStatBlock::dump_with_type(const char* type_str) {

    double idle_ratio = 0.0;

    if (total_txopps > 0.0) {

        idle_ratio = static_cast<double>(idle_txopps)/ static_cast<double>(total_txopps);

    }

    printf("[%u]Dump NodeStatBlock: (%s) idle_txopp_num = %u, total_txopp_num = %u, idle_ratio=%lf \n",
        nid, type_str, idle_txopps, total_txopps, idle_ratio );
    
    return 1;

}

int TxOppUtilizationCounter::dump_debugging_info() {

    const char* type_str = NULL;
    if (opp_type == msh_ncfg )
        type_str = "MSH_NCFG";
    else if (opp_type == msh_dsch)
        type_str = "MSH_DSCH";
    else
        type_str = "UNKNOWN_OPP_TYPE";
    
    
    INFO("[%s] max_txopp_num = %u, cur_bs_txopp_num = %u, cur_max_blk_txopp_num = %u \n", 
        type_str, max_txopp_num, cur_bs_txopp_num, cur_max_blk_txopp_num);
    
    INFO("[%s] num_of_ss_nodes = %u, num_of_map = %u\n", 
        type_str, num_of_ss_nodes, num_of_map);
    
    INFO("[%s] map_size = %u, start_txopp_num = %u, end_txopp_num = %u \n", 
        type_str, map_size, start_txopp_num, end_txopp_num);
        
    INFO("Dump Nodes' States:\n");
    
    for (u_int32_t i=0; i<num_of_ss_nodes ;++i) {
    
        node_stat_blk[i].dump();
    
    }
    
    return 1;

}

int TxOppUtilizationCounter::record_used_txopp(u_int32_t nid, u_int32_t txopp_num) {

    if ( GetCurrentTime() < start_to_function_time_in_tick ) 
        return 1;
    
    u_int32_t meshss_nodes_functional_cnt = 0;
    for (u_int32_t i=0; i<num_of_ss_nodes ;++i) {
    
        if ( node_stat_blk[i].get_status() == node_stat_blk_t::functional_link_est )
            meshss_nodes_functional_cnt++;
        else {
            VINFO("node_stat_blk[i].status = %u.\n", (u_int32_t)node_stat_blk[i].get_status() );
        }
    }    
    
    if ( meshss_nodes_functional_cnt < num_of_ss_nodes) {
        
        VINFO("number of functional nodes = %u.\n", meshss_nodes_functional_cnt );
        return 1;
    }
        
    VINFO("record txopp_num=%u.\n", txopp_num);
    
    std::vector<utilization_map_t*>::iterator it;
    
    if (!start_flag) {
    
        initialize_map(txopp_num);
        start_flag = 1;
        
        const char* type_str = NULL;
        if (opp_type == msh_ncfg )
            type_str = "MSH_NCFG";
        else if (opp_type == msh_dsch)
            type_str = "MSH_DSCH";
        else
            type_str = "UNKNOWN_OPP_TYPE";
            
        INFO("Start to record transmission opportunity utilization (opp_type=%s).\n",
            type_str);
    }
    
    int checked_flag = 0;
    int cnt=0;
    int list_len = utilization_map.size();
    
    for ( it=utilization_map.begin(); it!=utilization_map.end();++it) {
    
        utilization_map_t* ptr = (*it);
        
        int res = ptr->check_range(txopp_num);
        
        ++cnt;
        
        if (res) {
            
            ptr->set_use_of_the_given_txopp(nid, txopp_num);
            checked_flag = 1;
            break;
        
        }

    }

    if (cnt == list_len) {

        /* The last map being used means that we are going to run out of maps.
         * Therefore, we need to add new maps.
         */

        int new_alloc_map_cnt=0;
        
        while (1) {
        
            iterate_map_range();
        
            utilization_map_t* tmp = new utilization_map_t( map_size, num_of_ss_nodes, 
                                            start_txopp_num, end_txopp_num, max_txopp_num );
        
            utilization_map.push_back(tmp);
            
            int res = tmp->check_range(txopp_num);
            
            if (res) {
                
                tmp->set_use_of_the_given_txopp(nid, txopp_num);
                checked_flag = 1;
            
            }
            
            if (checked_flag)
                break;
            
            ++new_alloc_map_cnt;
            

            /* Detect incorrect txopp number. */
            if (new_alloc_map_cnt >=20 ) {
            
                ERROR("Too far txopp number: given txopp number = %u (last util_map:%u,%u,%u).\n",
                    txopp_num, tmp->get_start_txopp_num(), tmp->get_end_txopp_num(),
                    tmp->get_size() );

                assert(0);

            }
        
        }
   
        /* compute uses of transmission opportunities in the first map */
        if (utilization_map.size()>= num_of_map) {
            
            for (u_int32_t i=utilization_map.size(); i>num_of_map ;--i) {
            
                compute_txopp_use_in_the_first_map();
            
            }
            
        }
    }
    
    if (!checked_flag) {
    
        FATAL("[%u]: failed to set use of a txopp (num=%u).\n", nid, txopp_num);
    
    }
    
    //ERROR("[%u]: txopp_num=%u\n", nid, txopp_num);
    

    return 0;

}


int TxOppUtilizationCounter::initialize_map(u_int32_t txopp_num) {

    double times = floor( txopp_num/map_size);
    double aligned_start_txopp_num = map_size*times;
    
    start_txopp_num = static_cast<u_int32_t> (aligned_start_txopp_num);
    end_txopp_num = start_txopp_num + map_size - 1;
    
    /* initialize first three maps */
    for (u_int32_t i=0 ; i<num_of_map;++i) {
    
        if (i>0)
            iterate_map_range();
    
        utilization_map_t* tmp = new utilization_map_t( map_size, num_of_ss_nodes, 
                                    start_txopp_num, end_txopp_num, max_txopp_num );
    
        utilization_map.push_back(tmp);
    
    }
    
    return 1;

}

TxOppUtilizationCounter::TxOppUtilizationCounter(txopp_type_t opp_type1, u_int32_t num_of_ss_nodes1,
        u_int32_t max_txopp_num1 ) {

    opp_type                = opp_type1;
    num_of_ss_nodes         = num_of_ss_nodes1;
    cur_bs_txopp_num        = 0; 
    cur_max_blk_txopp_num   = 0;
    max_txopp_num           = max_txopp_num1;
    
    //ERROR("TxOppUtilizationCounter::TxOppUtilizationCounter: set max_txopp_num = %u.\n", max_txopp_num1);
    
    num_of_map              = 3;
    start_flag              = 0;
    
    map_size = 8;
    
    start_txopp_num = 0;
    end_txopp_num   = 0;
    
    u_int32_t start_time = 200;/* in second */
    start_to_function_time_in_tick = start_time*10000000;
    
    node_stat_blk = new node_stat_blk_t[num_of_ss_nodes];
    for (u_int32_t i=0; i<num_of_ss_nodes ;++i) {
    
        u_int32_t adjusted_nid = i+1;
        node_stat_blk[i].set_nid(adjusted_nid);
    
    }

}

TxOppUtilizationCounter::~TxOppUtilizationCounter()
{
    std::vector<utilization_map_t*>::iterator it;
    for ( it=utilization_map.begin() ; it!=utilization_map.end();++it) {
    
        utilization_map_t* ptr = (*it);
        
        if (ptr)
            delete ptr;
       
    }
    
    delete[] node_stat_blk;
}

int TxOppUtilizationCounter::register_neighbor_list(u_int32_t nid, Neighbor_mngr* nbr_mngr) {

    ASSERT(nbr_mngr, "nblist is null.\n");
    ASSERT(nid>0 && nid<=num_of_ss_nodes, "nid is zero.\n");
    
    u_int32_t adjusted_nid = nid-1;
    node_stat_blk[adjusted_nid].set_neighbor_mngr(nbr_mngr);
    return 1;

}

int TxOppUtilizationCounter::update_node_status(u_int32_t nid, node_stat_blk_t::node_status_t status1 ) {

    ASSERT(nid>0 && nid<=num_of_ss_nodes, "nid is zero.\n");
    
    u_int32_t adjusted_nid = nid-1;
    node_stat_blk[adjusted_nid].set_status(status1);
    return 1;

}

/* This function is used to compute uses of transmission opportunities 
 * gathered from all network nodes. Note that this computation should
 * be delayed to make sure all network nodes will have checked their 
 * uses for a specific transmission opportunity before this check is 
 * performed.
 */

int TxOppUtilizationCounter::compute_txopp_use_in_the_first_map() {

    std::vector<utilization_map_t*>::iterator map_it;
    std::vector<Neighbor*>::iterator nblist_it;
    
    map_it = utilization_map.begin();
    utilization_map_t* map_p = (*map_it);
    
    for (u_int32_t i=0; i< map_p->get_size(); ++i) {

        u_int32_t txopp_number = map_p->get_start_txopp_num() + i;
        if (txopp_number > max_txopp_num)
            txopp_number-=max_txopp_num;
        
        for ( u_int32_t j=0; j<num_of_ss_nodes ;++j ) {
        
            int use_flag=0;
            u_int32_t adjusted_nid = j;

            if ( !((node_stat_blk[adjusted_nid].get_status()) & node_stat_blk_t::functional) )
                continue;
            
            Neighbor_mngr* nbr_mngr = node_stat_blk[adjusted_nid].get_neighbor_mngr();
            
            ASSERT(nbr_mngr, "node %u's neighbor list is null.\n", j);
            
            ASSERT(nbr_mngr->neighbors().size(), "node %u's neighbor list is empty.\n", j);
            
            for (Neighbor_mngr::map_t::const_iterator it = nbr_mngr->neighbors().begin();
                    it != nbr_mngr->neighbors().end(); it++) {

                Neighbor* nbr = it->second;

                int nb_txopp_use = map_p->check_use_of_the_given_txopp(nbr->node_id(), txopp_number);
                if (nb_txopp_use)
                    use_flag = 1;
            }

            if (!use_flag)
                node_stat_blk[adjusted_nid].inc_idle_txopps();

            u_int32_t node_id = node_stat_blk[adjusted_nid].get_nid();
            
            if (map_p->check_use_of_the_given_txopp(node_id,txopp_number))
                node_stat_blk[adjusted_nid].inc_won_txopps();

            node_stat_blk[adjusted_nid].inc_total_txopps();
        
        }
    }

    VINFO("remove map with start txopp num = %u, end txopp num = %u, size = %u.\n",
        map_p->get_start_txopp_num(), map_p->get_end_txopp_num(), map_p->get_size());

    delete (*map_it);
    utilization_map.erase(map_it);

    VINFO("util_list.size = %u.\n", utilization_map.size());

    return 1;

}

int TxOppUtilizationCounter::iterate_map_range() {

    start_txopp_num += map_size;
    
    /* handle the number-wraping case */
    if ( start_txopp_num > max_txopp_num ) {
    
        start_txopp_num -= max_txopp_num;
    
    }
    
    end_txopp_num = start_txopp_num + map_size - 1;
    
    if ( end_txopp_num > max_txopp_num ) {
    
        end_txopp_num -= max_txopp_num;
    
    }
    
    return 1;

}

int TxOppUtilizationCounter::dump_node(u_int32_t nid1) {

    u_int32_t adj_nid = nid1-1;

    const char* type_str =
        (opp_type == TxOppUtilizationCounter::msh_ncfg)?"NCFG":"DSCH";
        
    return node_stat_blk[adj_nid].dump_with_type(type_str);

}

int TxOppUtilizationCounter::dump() {

    for ( u_int32_t i=0; i<num_of_ss_nodes ;++i ) {

        node_stat_blk[i].dump();

    }

    return 1;

}

int TxOppUtilizationCounter::range_test(u_int32_t txopp_num) {

    u_int32_t tmp_start_txopp_num = start_txopp_num;
    u_int32_t tmp_end_txopp_num   = end_txopp_num;
    
    tmp_start_txopp_num += map_size;
    
    /* handle the number-wraping case */
    if ( tmp_start_txopp_num > max_txopp_num ) {
    
        tmp_start_txopp_num -= max_txopp_num;
    
    }
    
    tmp_end_txopp_num = tmp_start_txopp_num + map_size - 1;
    
    if ( tmp_end_txopp_num > max_txopp_num ) {
    
        tmp_end_txopp_num -= max_txopp_num;
    
    }
    
    if (tmp_end_txopp_num < tmp_start_txopp_num)
        tmp_end_txopp_num = tmp_end_txopp_num + max_txopp_num;
    else
        tmp_end_txopp_num = tmp_end_txopp_num;
        
    if ( (txopp_num >= tmp_start_txopp_num) && (txopp_num <= tmp_end_txopp_num) )
        return 1;
    else
        return 0;

}

u_int32_t TxOppUtilizationCounter::get_idle_txopps(u_int32_t nid1) {

    u_int32_t adj_nid = nid1-1;
    return node_stat_blk[adj_nid].get_idle_txopps();

}

u_int32_t TxOppUtilizationCounter::get_won_txopps(u_int32_t nid1) {

    u_int32_t adj_nid = nid1-1;
    return node_stat_blk[adj_nid].get_won_txopps();

}

u_int32_t TxOppUtilizationCounter::get_total_txopps(u_int32_t nid1) {

    u_int32_t adj_nid = nid1-1;
    return node_stat_blk[adj_nid].get_total_txopps();

}
