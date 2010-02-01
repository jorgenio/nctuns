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

#ifndef __NCTUNS_Neighbor_h__
#define __NCTUNS_Neighbor_h__

#include "mesh_link.h"


class Nent_and_link_est_log;

class Neighbor {

public:

private:
    enum {
        UNKNOWN_HOP_COUNT   = 0xff,
        UNKNOWN_PROP_DELAY  = 0xff,
    };

    typedef enum {
        LINK_INIT,
        LINK_WAIT_CHALLENGE,
        LINK_SENT_CHALLENGE,
        LINK_WAIT_RESPONSE,
        LINK_SENT_RESPONSE,
        LINK_WAIT_ACCEPT,
        LINK_SENT_ACCEPT,
        LINK_ACTIVE,

    } link_state_t;

private:
    uint16_t                _node_id;
    uint8_t                 _hop_count;
    uint8_t                 _sync_hop_count;
    uint8_t                 _est_prop_delay;
    bool                    _has_been_selected_as_a_cs_node;
    link_state_t            _link_state;
    Nent_and_link_est_log*  _link_est_log;

    /*
     * The _tx_link is the link for the neighbor to transmit data to the local node.
     * The _rx_link is the link for the neighbor to receive data from the local node.
     */
    Mesh_link*              _tx_link;
    Mesh_link*              _rx_link;
    Mesh_link_rx::hash_t    _rx_links;

public:
    Neighbor(uint16_t, uint8_t hop_count = UNKNOWN_HOP_COUNT);
    virtual ~Neighbor();

    Mesh_link_rx* create_tx_link(uint8_t, uint16_t);
    Mesh_link_rx* tx_link(uint8_t);
    void dump() const;
    void dump_link_state(const char* trailing_str = NULL) const;

    inline uint16_t node_id() const { return _node_id; }
    inline uint8_t hop_count() const { return _hop_count; }
    inline uint8_t sync_hop_count() { return _sync_hop_count; }
    inline uint8_t est_prop_delay() const { return _est_prop_delay; }
    inline Nent_and_link_est_log* link_est_log() const { return _link_est_log; }
    inline Mesh_link* tx_link() const { return _tx_link; }
    inline Mesh_link* rx_link() const { return _rx_link; }
    inline void set_hop_count(uint8_t count) { _hop_count = count; }
    inline void set_sync_hop_count(uint8_t count) { _sync_hop_count = count; }
    inline void set_tx_link(Mesh_link* link) { _tx_link = link; }
    inline void set_rx_link(Mesh_link* link) { _rx_link = link; }

    inline bool has_been_selected_as_a_cs_node() const { return _has_been_selected_as_a_cs_node; }
    inline void select_as_a_cs_node() { _has_been_selected_as_a_cs_node = true; }

    /*
     * State access functions.
     */
    inline bool is_link_init() const { return _link_state == LINK_INIT; }
    inline bool is_link_wait_challenge() const { return _link_state == LINK_WAIT_CHALLENGE; }
    inline bool is_link_sent_challenge() const { return _link_state == LINK_SENT_CHALLENGE; }
    inline bool is_link_wait_response() const { return _link_state == LINK_WAIT_RESPONSE; }
    inline bool is_link_sent_response() const { return _link_state == LINK_SENT_RESPONSE; }
    inline bool is_link_wait_accept() const { return _link_state == LINK_WAIT_ACCEPT; }
    inline bool is_link_sent_accept() const { return _link_state == LINK_SENT_ACCEPT; }
    inline bool is_link_active() const { return _link_state == LINK_ACTIVE; }

    inline void set_link_init() { _link_state = LINK_INIT; }
    inline void set_link_wait_challenge() { _link_state = LINK_WAIT_CHALLENGE; }
    inline void set_link_sent_challenge() { _link_state = LINK_SENT_CHALLENGE; }
    inline void set_link_wait_response() { _link_state = LINK_WAIT_RESPONSE; }
    inline void set_link_sent_response() { _link_state = LINK_SENT_RESPONSE; }
    inline void set_link_wait_accept() { _link_state = LINK_WAIT_ACCEPT; }
    inline void set_link_sent_accept() { _link_state = LINK_SENT_ACCEPT; }
    inline void set_link_active() { _link_state = LINK_ACTIVE; }
};


#endif /* __NCTUNS_Neighbor_h__ */
