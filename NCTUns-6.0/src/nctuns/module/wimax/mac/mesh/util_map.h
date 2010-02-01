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

#ifndef __UTIL_MAP__H
#define __UTIL_MAP__H

#include <cstdio>
#include <sys/types.h>
#include <list>
#include <map>
#include "neighbor.h"
#include "mac802_16_mesh_mode_sys_param.h"


class Neighbor_mngr;

typedef class UtilizationMap {

    typedef enum UtilMapState {
    
        none        = 0,
        initialized = 1,
        in_use      = 2,
        checked     = 3,
    
    } util_map_state_t; 

    private:
    
    util_map_state_t    state;
    u_int32_t           size;
    u_int32_t           node_num;
    u_int32_t           start_txopp_num;
    u_int32_t           end_txopp_num;
    u_int32_t           max_txopp_num;
    char*               node_checking_flag;
    char**              map;

    public:
    UtilizationMap(u_int32_t size1, u_int32_t node_num1, u_int32_t start_txopp_num1, 
            u_int32_t end_txopp_num1, u_int32_t max_txopp_num1);
    ~UtilizationMap();
    int reset();
    u_int32_t get_start_txopp_num();
    u_int32_t get_end_txopp_num();
    u_int32_t get_size();
    
    int check_range(u_int32_t txopp_num);
    int set_use_of_the_given_txopp(u_int32_t nid, u_int32_t txopp_num);
    int check_use_of_the_given_txopp(u_int32_t nid, u_int32_t txopp_num);

} utilization_map_t;

typedef class NodeStatBlock {

    public:

    typedef enum NodeStatusType {

        none                = 0,
        synchronized        = 1,
        functional          = 2,
        link_established    = 4,
        functional_link_est = functional | link_established,

    } node_status_t;


    private:
    node_status_t       node_status;
    u_int32_t           nid;
    Neighbor_mngr*      _nbr_mngr;
    u_int32_t           idle_txopps;
    u_int32_t           won_txopps;
    u_int32_t           total_txopps;

    public:
    NodeStatBlock();
    int set_status(NodeStatBlock::node_status_t status1);
    int set_nid(u_int32_t nid1);
    int set_neighbor_mngr(Neighbor_mngr*);
    int inc_idle_txopps();
    int inc_won_txopps();
    int inc_total_txopps();

    inline NodeStatBlock::node_status_t get_status() { return node_status; }
    inline Neighbor_mngr* get_neighbor_mngr() { return _nbr_mngr; }
    inline uint32_t get_nid() { return nid; }
    inline uint32_t get_idle_txopps() { return idle_txopps; }
    inline uint32_t get_won_txopps() { return won_txopps; }
    inline uint32_t get_total_txopps() { return total_txopps; }

    int dump();
    int dump_with_type(const char* type_str);

} node_stat_blk_t;

typedef class TxOppUtilizationCounter {

    public:
    
    typedef enum TxOppType {
    
        not_defined = 0,
        msh_ncfg = 1,
        msh_dsch = 2,
    
    } txopp_type_t;

    private:
    txopp_type_t                    opp_type;
    u_int32_t                       max_txopp_num;
    u_int32_t                       cur_bs_txopp_num;
    u_int32_t                       cur_max_blk_txopp_num;
    u_int32_t                       num_of_ss_nodes;
    u_int32_t                       num_of_map;
    std::vector<utilization_map_t*> utilization_map;
    node_stat_blk_t*                node_stat_blk;
    
    int start_flag;
    int map_size;
    u_int32_t start_txopp_num;
    u_int32_t end_txopp_num;
    
    u_int64_t start_to_function_time_in_tick;
    
    private:
    
    int initialize_map(u_int32_t txopp_num);
    int compute_txopp_use_in_the_first_map();
    int iterate_map_range();
    int range_test(u_int32_t txopp_num);

    
    public:
    TxOppUtilizationCounter(txopp_type_t opp_type1, u_int32_t num_of_ss_nodes1, u_int32_t max_txopp_num1 );
    ~TxOppUtilizationCounter();
    
    int update_node_status(u_int32_t nid, node_stat_blk_t::node_status_t status1 );
    int record_used_txopp(u_int32_t nid, u_int32_t txopp_num);
    int register_neighbor_list(u_int32_t nid, Neighbor_mngr*);

    u_int32_t get_idle_txopps(u_int32_t nid1);
    u_int32_t get_won_txopps(u_int32_t nid1);
    u_int32_t get_total_txopps(u_int32_t nid1);
    
    int dump_node(u_int32_t nid1);
    int dump();
    int dump_debugging_info();

} txopp_util_counter_t;

int record_used_txopp(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, u_int32_t txopp_num);
int register_neighbor_list(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, Neighbor_mngr*);
int update_node_status(TxOppUtilizationCounter::txopp_type_t txopp_type,
        u_int32_t nid, node_stat_blk_t::node_status_t status1 );

int dump_node_txopp_utilization(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid);
int dump_all_txopp_utilization();

u_int32_t get_total_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid);
u_int32_t get_idle_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid);
u_int32_t get_won_txopps(TxOppUtilizationCounter::txopp_type_t txopp_type, u_int32_t nid);

#endif /* __UTIL_MAP__H */
