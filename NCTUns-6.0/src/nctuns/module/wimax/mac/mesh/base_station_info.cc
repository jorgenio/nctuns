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

#include <stdio.h>
#include "base_station_info.h"

#if 0
#include <cassert>
#include <string>
#endif
#include "msh_ncfg.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


/*
 * Member function definitions of class `Base_station_info'.
 */

/*
 * Constructor
 */
Base_station_info::Base_station_info()
: _cur_used_bs_node_id(0xffff)
{
	_bs_list.clear();
}


/*
 * Destructor
 */
Base_station_info::~Base_station_info()
{
}

int
Base_station_info::update(MSH_NCFG* ncfg, uint16_t tx_node_id)
{
    std::list<Base_station_entry*>::iterator it;

    DEBUG("bs_list size: %u \n", _bs_list.size());

    if (_bs_list.empty()) {
        return 0;
    }

    int num_bs_entry = ncfg->getNumBSEntries();

    int whole_list_changed_flag = 0;

    for (int i=0; i<num_bs_entry ;++i) {

        /*
         * FIXME: The entry_p should be delete somewhere.
         */
        assert(0);
        Base_station_entry* entry_p = ncfg->getBSEntry(i);

        entry_p->reporting_peer_nid = tx_node_id;
        FATAL("Getting base station entry failed.")

        int found_flag = 0;


        for (it = _bs_list.begin(); it != _bs_list.end(); it++) {

            bs_info_change_level_t change_flag = none;

            if ((*it)->nid == entry_p->nid) {

                Base_station_entry* tmp_entry_p = (*it);

                if ( tmp_entry_p->hop_cnt > entry_p->hop_cnt) {

                    tmp_entry_p->hop_cnt = entry_p->hop_cnt;

                    change_flag = static_cast<bs_info_change_level_t> (change_flag | change_bs_hop_cnt);

                }

                if ( change_flag & change_bs_hop_cnt) {

                    tmp_entry_p->hop_cnt = entry_p->hop_cnt;

                    change_flag = static_cast<bs_info_change_level_t> (change_flag | change_bs_energy_level);

                }
                else {

                    if ( tmp_entry_p->tx_energy_level < entry_p->tx_energy_level) {

                        tmp_entry_p->hop_cnt = entry_p->hop_cnt;

                        change_flag = static_cast<bs_info_change_level_t> ( change_flag | change_bs_energy_level);

                    }

                }

                if ( tmp_entry_p->reporting_peer_nid != entry_p->reporting_peer_nid ) {

                    if ( change_flag != none ) {

                        tmp_entry_p->reporting_peer_nid = entry_p->reporting_peer_nid;
                        change_flag = static_cast<bs_info_change_level_t> ( change_flag | change_reporting_nid);

                    }

                }

                if ( change_flag != none) {

                    tmp_entry_p->update_timestamp = entry_p->update_timestamp;
                    whole_list_changed_flag = 1;

                }

                found_flag = 1;
                break;

            }
        }


        if (!found_flag) {

            _bs_list.push_back(entry_p);
            whole_list_changed_flag = 1;
        }

    }

    if (whole_list_changed_flag) {

        /* doing sorting is unimplemented. */
        /* do_bs_info_list_sort();*/
        ;
    }

    return 1;

}

int
Base_station_info::fill_in_msgncfg_bsentries(MSH_NCFG* msg_ncfg_p)
{
    ASSERT(msg_ncfg_p, "msg_ncfg_p is null.\n");

    u_int32_t bs_cnt = _bs_list.size();

    /* Adjust the number of base stations that are going to be reported. */
    if ( bs_cnt > 3)
        bs_cnt = 3;

    if ( !bs_cnt) {

        return 0;

    }

    Base_station_entry* entry_p = _get_cur_used_entry();
    msg_ncfg_p->addBSentry(entry_p->nid,entry_p->hop_cnt,entry_p->tx_energy_level);

    u_int16_t cur_bs_nid = entry_p->nid;

    /* choose the first two base stations other that the currently used one
     * to report, if they exist.
     */

    std::list<Base_station_entry*>::iterator it = _bs_list.begin();
    int max_ind=bs_cnt-1;
    for (int i=0; i<max_ind ;++i) {

        if (it == _bs_list.end() )
            break;

        if ( (*it)->nid != cur_bs_nid) {

            msg_ncfg_p->addBSentry(entry_p->nid,entry_p->hop_cnt,entry_p->tx_energy_level);

        }

        ++it;
    }

    return bs_cnt;
}

Base_station_entry*
Base_station_info::_get_cur_used_entry()
{
    std::list<Base_station_entry*>::iterator it;

    for (it=_bs_list.begin(); it!= _bs_list.end(); ++it) {

        Base_station_entry* entry_p = (*it);
        if (entry_p->nid == _cur_used_bs_node_id)
            return entry_p;

    }

    return NULL;
}
