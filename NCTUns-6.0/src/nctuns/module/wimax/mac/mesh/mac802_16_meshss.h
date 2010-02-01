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

#ifndef __NCTUNS_mac802_16_MeshSS_h__
#define __NCTUNS_mac802_16_MeshSS_h__

#include <netinet/in.h>
#include <object.h>

#include "../mac802_16.h"


class Base_station_info;
class Ctrl_scheduler;
class Data_scheduler;
class Frame_mngr;
class Neighbor_mngr;
class Net_entry_mngr;
class Network_descriptor;
class Node_deg_descriptor;
class MeshRoute;
class timerObj;

class mac802_16_MeshSS: public mac802_16 {

private:
        enum {
            UNKNOWN_NODE_ID     = 0,
        };

        typedef enum {

            conform_to_standard             = 0,
            self_determine                  = 1,
            read_param_from_auxiliary_file  = 2

        } attach_mode_t;

        typedef enum {

            MSH_INIT,
            MSH_NET_ENTRY_SYNC,
            MSH_NET_ENTRY_CONTEND,
            MSH_OPERATIONAL,

        } state_t;

        class Tx_params;

        /*
         * Variable Declaration.
         */
private:
        uint16_t                _node_id;
        state_t                 _state;
        attach_mode_t           _attach_mode;
        int*                    _inet_addr;
        const Tx_params*        _tx_params;

        Frame_mngr*             _fr_mngr;
        Neighbor_mngr*          _nbr_mngr;
        Net_entry_mngr*         _net_entry_mngr;
        Base_station_info*      _bs_info;
        Network_descriptor*     _network_des;
        Node_deg_descriptor*    _node_deg_des;
        MeshRoute*              _route_module;
        Ctrl_scheduler*         _ncfg_scheduler;
        Ctrl_scheduler*         _dsch_scheduler;
        Data_scheduler*         _data_scheduler;

        uint64_t                _start_to_function_tick;

        bool                    _pseudo_sched_flag;
        bool                    _use_node_degree_file_flag;
        bool                    _use_network_description_file_flag;

        uint32_t                _nf_requested_data_schedules;
        uint32_t                _nf_granted_data_schedules;

protected:
        timerObj*               timerInit;
        timerObj*               timerFrame;
        timerObj*               timerSendNCFG;
        timerObj*               timerSendDSCH;
        timerObj*               timerSendData;
        timerObj*               timerCheckValidity;
        timerObj*               timerPseudoScheduler;

        ManagementConnection*   CtrlConnection;
        ManagementConnection*   SponsorConnection;

        /*
         * Function Declaration.
         */
public:
        explicit mac802_16_MeshSS(uint32_t, uint32_t, plist*, const char*);
        virtual ~mac802_16_MeshSS();

        int init();
        int recv(Event*);
        int send(Event*);

        void find_time_sync(uint8_t, double rtt = 0);
        void put_pkt_to_upper_layer(Packet*);
        int reschedule_next_ncfg(uint32_t);
        void start_pseudo_scheduler();

        inline void set_node_id(uint16_t id) { _node_id = id; }

        inline in_addr_t inet_address() const { return *_inet_addr; }
        inline double Ts() const { return mac802_16::Ts(); }
        inline uint16_t node_id() const { return _node_id; }
        inline Frame_mngr* fr_mngr() const { return _fr_mngr; }
        inline Neighbor_mngr* nbr_mngr() const { return _nbr_mngr; }
        inline Net_entry_mngr* net_entry_mngr() const { return _net_entry_mngr; }
        inline Network_descriptor* network_des() const { return _network_des; }
        inline Node_deg_descriptor* node_deg_des() const { return _node_deg_des; }
        inline MeshRoute* route_module() const { return _route_module; }
        inline Ctrl_scheduler* ncfg_scheduler() const { return _ncfg_scheduler; }
        inline Ctrl_scheduler* dsch_scheduler() const { return _dsch_scheduler; }
        inline Data_scheduler* data_scheduler() const { return _data_scheduler; }
        inline ManagementConnection* ctrl_connection() const { return CtrlConnection; }
        inline ManagementConnection* sponsor_connection() const { return SponsorConnection; }

        /*
         * State access functions.
         */
        inline void set_net_entry_sync() { _state = MSH_NET_ENTRY_SYNC; }
        inline void set_net_entry_contend() { _state = MSH_NET_ENTRY_CONTEND; }
        inline void set_operational() { _state = MSH_OPERATIONAL; }

        inline bool is_initialized() const { return _state != MSH_INIT; }
        inline bool is_net_entry_sync() const { return _state == MSH_NET_ENTRY_SYNC; }
        inline bool is_net_entry_contend() const { return _state == MSH_NET_ENTRY_CONTEND; }
        inline bool is_op_params_obtained() const { return is_net_entry_contend() || is_operational(); }
        inline bool is_operational() const { return _state == MSH_OPERATIONAL; }

protected:
        int procSubscriberStation(hdr_generic*);

private:
        int T6(Event_*);
        int T18(Event_*);
        int T25(Event_*);
        int StartFrame(Event_*);
        int SendMSHNCFG(Event_*);
        int SendMSHDSCH(Event_*);
        int SendData(Event_*);
        void SendCtrlMessage(mac80216_burst_t);
        void SendSponsor();

        int procMSHNCFG(mgmt_msg*, int, uint16_t, int);
        int procMSHDSCH(mgmt_msg*, int, uint16_t, int);

        int _check_ctrl_msg_len(uint32_t, uint32_t) const;
        int _self_determine_start_to_function_timepoint(Event_*);
        int _check_sched_validity(Event_*);
        int _pseudo_scheduler(Event_*);

        const char* _state_str() const;
        void _dump_log_to_file();
};

class mac802_16_MeshSS::Tx_params {

private:
    uint8_t _power;
    uint8_t _antenna;

public:
    explicit Tx_params();
    virtual ~Tx_params();

    inline uint8_t power() const { return _power; }
    inline uint8_t antenna() const { return _antenna; }
};

#endif /* __NCTUNS_mac802_16_MeshSS_h__ */
