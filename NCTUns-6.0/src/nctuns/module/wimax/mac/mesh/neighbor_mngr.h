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

#ifndef __NCTUNS_WIMAX_NEIGHBOR_MNGR_H__
#define __NCTUNS_WIMAX_NEIGHBOR_MNGR_H__


#include <stdint.h>
#include <map>


namespace Ctrl_sch {
    class Entry_interface;
}
class mac802_16_MeshSS;
class Neighbor;

class Neighbor_mngr {

public:
    typedef std::map<uint16_t, Neighbor*> map_t;

private:
    map_t   _neighbors;

public:
    explicit Neighbor_mngr();
    virtual ~Neighbor_mngr();

    void init(uint32_t);
    bool add_neighbor(Neighbor*);
    Neighbor* neighbor(uint16_t);
    Neighbor* neighbor_with_link_rx_id(uint8_t);
    void update_neighbors(mac802_16_MeshSS*, Ctrl_sch::Entry_interface&, uint16_t);
    uint8_t max_rx_link_id();
    size_t nf_active_link();
    size_t nf_one_hop_nbrs();
    void dump();

    inline const map_t& neighbors() const { return _neighbors; }
};


#endif /* __NCTUNS_WIMAX_NEIGHBOR_MNGR_H__ */
