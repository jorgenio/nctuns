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

#ifndef __NCTUNS_WIMAX_BASE_STATION_INFO_H__
#define __NCTUNS_WIMAX_BASE_STATION_INFO_H__


#include <stdint.h>
#include <list>


class Base_station_entry {

public:
    uint16_t nid;
    uint8_t  hop_cnt;
    uint8_t  tx_energy_level;
    uint16_t reporting_peer_nid;
    uint64_t update_timestamp;
};

class MSH_NCFG;
class Base_station_info {

private:
    typedef enum {

        none                    = 0x0000,
        change_reporting_nid    = 0x0001,
        change_bs_hop_cnt       = 0x0002,
        change_bs_energy_level  = 0x0004,

        change_reporting_nid_and_bs_hop_cnt         = change_reporting_nid | change_bs_hop_cnt,
        change_reporting_nid_and_bs_energy_level    = change_reporting_nid | change_bs_energy_level,
        change_bs_hop_cnt_and_bs_energy_level       = change_bs_hop_cnt | change_bs_energy_level,
        change_all = change_reporting_nid | change_bs_hop_cnt | change_bs_energy_level,

    } bs_info_change_level_t;


private:
    uint16_t                        _cur_used_bs_node_id;
    std::list<Base_station_entry*>  _bs_list;

protected:

public:
    explicit Base_station_info();
    virtual ~Base_station_info();

    int update(MSH_NCFG*, uint16_t);
    int fill_in_msgncfg_bsentries(MSH_NCFG*);

private:
    Base_station_entry* _get_cur_used_entry();
};


#endif /* __NCTUNS_WIMAX_BASE_STATION_INFO_H__ */
