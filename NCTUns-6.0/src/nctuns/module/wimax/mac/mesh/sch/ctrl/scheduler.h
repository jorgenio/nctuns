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

#ifndef __NCTUNS_WIMAX_MESHSCHEDULING_H__
#define __NCTUNS_WIMAX_MESHSCHEDULING_H__


#include <stdint.h>
#include <list>
#include <map>


namespace Ctrl_sch {
    class Info;
    class Info_nbr;
    class Ht_assigner;
}
class mac802_16_MeshSS;
class Neighbor;
class Neighbor_mngr;

class Ctrl_scheduler {

private:
    typedef std::list<uint16_t>                     node_id_list_t;
    typedef std::map<uint16_t, Ctrl_sch::Info_nbr*> nbr_sch_map_t;
    typedef void (Ctrl_scheduler::*collect_node_func_t)(node_id_list_t&, uint32_t, uint32_t);

private:
    mac802_16_MeshSS*       _mac;
    Ctrl_sch::Ht_assigner*  _ht_assigner;
    nbr_sch_map_t           _nbr_sch_infos;
    collect_node_func_t     _collect_node;
    uint32_t                _next_tx_opp;
    uint32_t                _max_tx_opp;

public:
    explicit Ctrl_scheduler(mac802_16_MeshSS*);
    virtual ~Ctrl_scheduler();

    void init();
    void start(uint32_t, uint32_t);
    uint32_t update_next_tx_opp();
    uint32_t get_next_tx_opp(const Ctrl_sch::Info*);
    Ctrl_sch::Info_nbr* nbr_sch_info(uint16_t);
    bool is_valid_next_tx_opp() const;

    inline const mac802_16_MeshSS* mac() const { return _mac; }
    inline Ctrl_sch::Ht_assigner* ht_assigner() const { return _ht_assigner; }
    inline uint32_t next_tx_opp() const { return _next_tx_opp; }
    inline uint32_t max_tx_opp() const { return _max_tx_opp; }
    inline void set_next_tx_opp(uint32_t tx_opp) { _next_tx_opp = tx_opp; }

private:
    void _init_ht_assigner();
    void _collect_all_node(node_id_list_t&, uint32_t, uint32_t);
    void _collect_eligible_node(node_id_list_t&, uint32_t, uint32_t);
    //void _sort(std::vector<Neighbor*>&);
    bool _mesh_election(uint32_t, const node_id_list_t&);
    uint32_t _smear(uint16_t);
    Ctrl_sch::Info_nbr* _nbr_sch_info(uint16_t);
};


#endif /* __NCTUNS_WIMAX_MESHSCHEDULING_H__ */
