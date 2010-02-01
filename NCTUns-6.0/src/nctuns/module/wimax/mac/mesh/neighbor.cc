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
#include "neighbor.h"
#include "net_entry_mngr.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


Neighbor::Neighbor(uint16_t node_id, uint8_t hop_count)
: _node_id(node_id)
, _hop_count(hop_count)
, _sync_hop_count(UNKNOWN_HOP_COUNT)
, _est_prop_delay(UNKNOWN_PROP_DELAY)
, _has_been_selected_as_a_cs_node(false)
, _link_state(LINK_INIT)
, _link_est_log(new Nent_and_link_est_log())
, _tx_link(NULL)
, _rx_link(NULL)
{
    assert(node_id);

    _rx_links.clear();
}

Neighbor::~Neighbor()
{
    if (_link_est_log)
        delete _link_est_log;
}

Mesh_link_rx*
Neighbor::create_tx_link(uint8_t link_id, uint16_t dst_nid)
{
    if (_rx_links.find(link_id) == _rx_links.end()) {
        _rx_links[link_id] =
            new Mesh_link_rx(link_id, dst_nid, _node_id);
        return _rx_links[link_id];
    }

    return NULL;
}

Mesh_link_rx*
Neighbor::tx_link(uint8_t link_id)
{
    Mesh_link_rx::hash_t::const_iterator it = _rx_links.find(link_id);

    if (it == _rx_links.end())
        return NULL;

    return it->second;
}

void
Neighbor::dump() const
{
    printf("\tnode ID = %u, hop = %u, sync = %u, link = (%u,%u), state = ",
	    _node_id, _hop_count, _sync_hop_count,
	    _rx_link?_rx_link->id():Mesh_link::UNKNOWN_ID,
	    _tx_link?_tx_link->id():Mesh_link::UNKNOWN_ID);
    dump_link_state();
    printf("\n");
}

void
Neighbor::dump_link_state(const char* trailing_str) const
{
    switch (_link_state) {
    case LINK_INIT:             printf("`Init'"); break;
    case LINK_WAIT_CHALLENGE:   printf("`Wait Challenge'"); break;
    case LINK_SENT_CHALLENGE:   printf("`Sent Challenge'"); break;
    case LINK_WAIT_RESPONSE:    printf("`Wait Response'"); break;
    case LINK_SENT_RESPONSE:    printf("`Sent Response'"); break;
    case LINK_WAIT_ACCEPT:      printf("`Wait Accept'"); break;
    case LINK_SENT_ACCEPT:      printf("`Sent Accept'"); break;
    case LINK_ACTIVE:           printf("`Active'"); break;
    default: assert(0);
    }

    if (trailing_str)
        printf("%s", trailing_str);
}
