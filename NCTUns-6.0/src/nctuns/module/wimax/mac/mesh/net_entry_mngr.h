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

#ifndef __NCTUNS_WIMAX_NETWORK_ENTRY_MNGR_H__
#define __NCTUNS_WIMAX_NETWORK_ENTRY_MNGR_H__


#include <netinet/in.h>
#include <queue>

#include "mesh_link.h"
#include "msh_ncfg/emb_data_hdr.h"


namespace Msh_ncfg {
    class Net_entry_open_ie;
    class Link_est_ie;
}

struct mgmt_msg;
class Data_scheduler;
class Frame_mngr;
class Neighbor;
class Neighbor_mngr;
class Nent_and_link_est_log;
class Mac_address;
class mac802_16_MeshSS;
class MSH_NCFG;
class MSH_NENT;
class Packet;
class Timer_mngr;
class timerObj;

class Net_entry_mngr {

private:
    typedef enum network_entry_state {

        NET_ENTRY_INIT,
        NET_ENTRY_OPEN,
        NET_ENTRY_NEG_CAP,
        NET_ENTRY_REGISTER,

    } network_entry_state_t;

    typedef enum sponsor_state {

        sprNone,
        sprUnavailable,
        sprAvailable,
        sprPolling,
        sprBusy,
        sprSendOpenGrant,   // Sponsor Node received MSH-NENT::NetEntryRequest, and need to send Grant
        sprSendAck,         // Sponsor Node received MSH-NENT::EntryClose, and need to send Ack
        sprRunning          // Sponsor Node is keeping upper MAC entry

    } sponsor_state_t;

public:
    typedef enum NetworkEntryProcessMode {

        original_nent_process_mode = 1,
        enhanced_nent_process_mode = 2

    } network_entry_process_mode_t;

    typedef enum LinkEstablishementProtocolMode {

        original_link_est_mode = 1,
        extended_link_est_mode = 2,

    } link_est_protocol_mode_t;

private:
    mac802_16_MeshSS*               _mac;
    Msh_ncfg::link_est_ie_format_t  _link_est_ie_format;
    network_entry_process_mode_t    _network_entry_process_mode;
    link_est_protocol_mode_t        _link_est_protocol_mode;
    network_entry_state_t           _state;
    uint16_t                        _sync_hop_count;

    sponsor_state_t                 sponsoringState; // Merge with sponsorState
    uint32_t                        _next_nent_attempt_opp;

    /* For the Sponsoring Node. */
    bool                            isSponsoring;
    bool                            isEntryOpenGranted;
    bool                            isEntryCloseGranted;
    u_char                          sponsoredMacAddr[6];
    double                          estPropDelay;
    Msh_ncfg::Net_entry_open_ie*    ncfg_entry_open_ie_p;
    uint32_t                        sponsoring_retry_cnt;
    uint32_t                        default_max_sponsoring_retry_cnt;
    uint16_t                        sponsoredNodeID;

    /* For the Candidate Node. */
    sponsor_state_t                 sponsorsState;
    sponsor_state_t                 othersState;
    uint64_t                        othersMaxMacAddr;
    uint64_t                        othersMinMacAddr;
    uint8_t                         othersMaxMacAddr1[6];
    uint8_t                         othersMinMacAddr1[6];
    const Neighbor*                 targetSponsor;
    uint32_t                        seek_for_sponsor_retry_cnt;
    uint32_t                        default_max_seek_for_sponsor_retry_cnt;
    std::queue<MSH_NENT*>           _nent_tx_queue;

    /*
     * Links for transmission.
     */
    Mesh_link_tx::hash_t            _tx_links;

    bool                            _use_nexthop_to_bs_as_sponsor_node_flag;
    bool                            _change_route_to_bs_flag;

    Nent_and_link_est_log*          _nent_log;
    Nent_and_link_est_log*          _link_est_log;

public:
    explicit Net_entry_mngr(mac802_16_MeshSS*);
    virtual ~Net_entry_mngr();

    bool is_sponsoring();

    size_t show_link_statues(std::vector<Neighbor*>&);
    void show_net_entry_mode();
    void show_link_est_mode();

    Msh_ncfg::link_est_ie_format_t set_link_est_ie_format();

    void t25();

    const MSH_NENT* dequeue_nent();

    void stop_sponsoring_due_to_recv_node_id(uint16_t);

    void proc_ncfg_network_entry_sync(const MSH_NCFG&, uint16_t);
    void proc_ncfg_network_entry_contend(const MSH_NCFG&, uint16_t);

    bool proc_ncfg_network_entry_open(uint16_t);
    bool proc_ncfg_network_entry_ack(uint16_t);
    void proc_ncfg_neighbor_link_est(Neighbor*, Msh_ncfg::Link_est_ie*);

    void proc_nent(mgmt_msg*, uint16_t, int, timerObj*);

    ifmgmt* proc_sbcreq(mgmt_msg*, int);
    bool proc_sbcrsp();
    Packet* proc_regreq(mgmt_msg*, int);
    uint16_t proc_regrsp(mgmt_msg*, int);

    void send_regreq();
    void send_sbcreq();

    Packet* tunnel(ifmgmt&, in_addr_t, in_addr_t);
    mgmt_msg* extract(char*, int&, in_addr_t&, in_addr_t);

    void append_sponsoring_info(MSH_NCFG*);
    void link_establish(MSH_NCFG*);

    inline uint16_t sync_hop_count() { return _sync_hop_count; }
    inline void set_sync_hop_count(uint16_t count) { _sync_hop_count = count; }

    inline Mesh_link_tx::hash_t& tx_links() { return _tx_links; }
    Mesh_link_tx* tx_link(uint8_t);

    inline bool is_negotiate_cap() const { return _state == NET_ENTRY_NEG_CAP; }
    inline bool is_register() const { return _state == NET_ENTRY_REGISTER; }

    inline Msh_ncfg::link_est_ie_format_t link_est_ie_format() const { return _link_est_ie_format; }
    inline const Neighbor* sponsor() const { return targetSponsor; }

    inline const Nent_and_link_est_log* nent_log() const { return _nent_log; }
    inline const Nent_and_link_est_log* link_est_log() const { return _link_est_log; }

private:
    void _reset_next_nent_attempt_opp();
    void _cancel_next_nent_attempt_opp();
    const Neighbor* _select_sponsor() const;
    void _change_route_to_bs() const;
    void _enqueue_nent(uint8_t);

    void _proc_nent_network_entry_ack(const MSH_NENT&);
    void _proc_nent_network_entry_req(const MSH_NENT&);
    void _proc_nent_network_entry_close(const MSH_NENT&, uint16_t);

    void _create_rx_link(class Neighbor*, uint8_t);
    void _create_tx_link(class Neighbor*);

    void _set_link_est_finished();
};


/*
 * for statistic purpose of entry process and dolink protocol
 */
class Nent_and_link_est_log {

private:
    enum {
        UNKNOWN_TICK    = 0xffffffffffffffffull,
    };

private:
    uint64_t    _start_tick;
    uint64_t    _end_tick;

public:
    explicit Nent_and_link_est_log();
    virtual ~Nent_and_link_est_log();

    double start_sec() const;
    double end_sec() const;
    double period_in_sec() const;

    inline void set_start_tick(uint64_t tick) { _start_tick = tick; }
    inline void set_end_tick(uint64_t tick) { _end_tick = tick; }

    inline uint64_t start_tick() const { return _start_tick; }
    inline uint64_t end_tick() const { return _end_tick; }

    inline bool is_start() const { return _start_tick != UNKNOWN_TICK; }
    inline bool is_end() const { return _end_tick != UNKNOWN_TICK; }
};


#endif /* __NCTUNS_WIMAX_NETWORK_ENTRY_MNGR_H__ */
