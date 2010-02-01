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
#include "mac802_16_meshss.h"

#include "base_station_info.h"
#include "frame_mngr.h"
#include "mesh_connection.h"
#include "msh_dsch.h"
#include "msh_ncfg/emb_ie_network_des.h"
#include "msh_nent.h"
#include "neighbor_mngr.h"
#include "net_entry_mngr.h"
#include "network_des.h"
#include "node_deg_des.h"
#include "sch/ctrl/ht_assigner.h"
#include "sch/ctrl/info.h"
#include "sch/ctrl/scheduler.h"
#include "sch/data/scheduler.h"
#include "util_map.h"
#include "../library.h"
#include "../mac_address.h"
#include "../timer_mngr.h"
#include "../../phy/ofdm_mesh.h"
#include <misc/log/logHeap.h>
#include <netinet/udp.h>
#include <nctuns_api.h>

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


MODULE_GENERATOR(mac802_16_MeshSS);


/*
 * Member function definitions of class `mac802_16_MeshSS'.
 */

/*
 * Constructor
 */
mac802_16_MeshSS::mac802_16_MeshSS(uint32_t type, uint32_t id, plist* pl, const char* name)
: mac802_16(type, id, pl, name)
, _node_id(UNKNOWN_NODE_ID)
, _state(MSH_INIT)
#if 1
, _attach_mode(conform_to_standard)
#else
, _attach_mode(self_determine)
#endif
, _inet_addr(NULL)
, _tx_params(new Tx_params())
, _fr_mngr(new Frame_mngr(this))
, _nbr_mngr(new Neighbor_mngr())
, _net_entry_mngr(new Net_entry_mngr(this))
, _bs_info(new Base_station_info())
, _network_des(new Network_descriptor())
, _node_deg_des(new Node_deg_descriptor())
, _ncfg_scheduler(new Ctrl_scheduler(this))
, _dsch_scheduler(new Ctrl_scheduler(this))
, _data_scheduler(new Data_scheduler(this))

, _start_to_function_tick(0xffffffffffffffffllu)
/*
 * Various flags:
 */
, _pseudo_sched_flag(false)
, _use_node_degree_file_flag(false)
, _use_network_description_file_flag(true)

, _nf_requested_data_schedules(0)
, _nf_granted_data_schedules(0)

, timerInit(new timerObj())
, timerFrame(new timerObj())
, timerSendNCFG(new timerObj())
, timerSendDSCH(new timerObj())
, timerSendData(new timerObj())
, timerCheckValidity(new timerObj())
, timerPseudoScheduler(new timerObj())

, CtrlConnection(new ManagementConnection(0x00ff, true))
, SponsorConnection(new ManagementConnection(0x00fe, true))
{
    _CSType     = csIPv4;

    BASE_OBJTYPE(mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, T6);
    timer_mngr()->set_func_t(6u, this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, T18);
    timer_mngr()->set_func_t(18u, this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, T25);
    timer_mngr()->set_func_t(25u, this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, _self_determine_start_to_function_timepoint );
    timerInit->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, StartFrame);
    timerFrame->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, SendMSHNCFG);
    timerSendNCFG->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, SendMSHDSCH);
    timerSendDSCH->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, SendData);
    timerSendData->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, _check_sched_validity);
    timerCheckValidity->setCallOutObj(this, mem_func);

    mem_func = POINTER_TO_MEMBER(mac802_16_MeshSS, _pseudo_scheduler);
    timerPseudoScheduler->setCallOutObj(this, mem_func);
    
    /*
     * XXX: Issue: The default maximum value of T6 is too small for Mesh.
     *             See Table 340.
     */
    uint64_t tick_interval;
    SEC_TO_TICK(tick_interval, 10);
    timer_mngr()->set_interval_t(6u, tick_interval);
    SEC_TO_TICK(tick_interval, 3);
    timer_mngr()->set_interval_t(18u, tick_interval);

    /* Call vBind() macro to get node degree description file path. */
    vBind("node_degree_description_filename", _node_deg_des->bind_ptr());

    /* Call vBind() macro to get network description file path. */
    vBind("network_description_filename", _network_des->bind_ptr());

    /* register variable */
    REG_VAR("80216_MESHSS_MAC_MODULE", this);
    REG_VAR("START_TIME", &_start_to_function_tick);
}

/*
 * Destructor
 */
mac802_16_MeshSS::~mac802_16_MeshSS()
{
    _dump_log_to_file();

    delete CtrlConnection;
    delete SponsorConnection;

    delete timerPseudoScheduler;
    delete timerCheckValidity;
    delete timerSendData;
    delete timerSendDSCH;
    delete timerSendNCFG;
    delete timerFrame;
    delete timerInit;

    if (_data_scheduler)
        delete _data_scheduler;

    if (_dsch_scheduler)
        delete _dsch_scheduler;

    if (_ncfg_scheduler)
        delete _ncfg_scheduler;

    if (_node_deg_des)
        delete _node_deg_des;

    if (_network_des)
        delete _network_des;

    if (_bs_info)
        delete _bs_info;

    if (_net_entry_mngr)
        delete _net_entry_mngr;

    if (_nbr_mngr)
        delete _nbr_mngr;

    if (_fr_mngr)
        delete _fr_mngr;

    if (_tx_params)
        delete _tx_params;
}

int
mac802_16_MeshSS::init()
{
    mac802_16::init();

    _inet_addr = GET_REG_VAR(get_port(), "IP", int *);

    assert((_route_module = GET_REG_VAR1(
                    get_portls(), "80216_MESHSS_ROUTE_MODULE", MeshRoute*)));

    printf("Node information:\n");

    char* config_file_dir = GetConfigFileDir();

    printf("inMAC filedir: %s\n", config_file_dir);
    if (_use_network_description_file_flag)
        assert(_network_des->init(config_file_dir));
    if (_use_node_degree_file_flag)
        _node_deg_des->init(get_nid(), config_file_dir);

    //free(config_file_dir);

    _nbr_mngr->init(get_nid());
    _net_entry_mngr->set_link_est_ie_format();
    _ncfg_scheduler->init();
    _dsch_scheduler->init();

    /*
     * Show various node information.
     */
    _network_des->dump();
    _node_deg_des->dump();
    _net_entry_mngr->show_net_entry_mode();
    _net_entry_mngr->show_link_est_mode();
    NSLOBJ_STAT("NCFG ht_assigner:\n");
    _ncfg_scheduler->ht_assigner()->dump();
    NSLOBJ_STAT("DSCH ht_assigner:\n");
    _dsch_scheduler->ht_assigner()->dump();

    if (_attach_mode == conform_to_standard) {
        /*
         * The MAC function turns on at tick 0 immediately.
         */
        _start_to_function_tick = 0;

        NSLOBJ_STAT("attach_mode = conform_to_standard\n");
    }
    else if (_attach_mode == self_determine) {

        timerInit->start(1ull,0);

        NSLOBJ_STAT("attach_mode = self_determine\n");
    }
    else {
        NSLOBJ_FATAL("unrecognized value for attach mode.\n");
    }

    return 1;
}

int
mac802_16_MeshSS::recv(Event* ep)
{
    uint64_t cur_tick = GetCurrentTime();

    if (cur_tick < _start_to_function_tick) {

        freePacket(ep);
        return 1;
    }
    else if ( cur_tick == _start_to_function_tick ) {

        NSLOBJ_INFO("start to function now.");
    }

    NSLOBJ_TEST("cur_frame = %#03x\n", _fr_mngr->cur_frame());

    Packet* recvBurst;
    assert(ep && (recvBurst = (Packet*)ep->DataInfo_));

    _fr_mngr->update_last_signal_info(
            (PHYInfo*)recvBurst->pkt_getinfo("phyInfo"));

    char*           AdaptedBurst = (char*)recvBurst->pkt_sget();
    int             BurstLen = recvBurst->pkt_getlen();
    unsigned int    len;

    for (char* ptr = AdaptedBurst;
            ptr + sizeof(hdr_generic) < AdaptedBurst + BurstLen;
            ptr += len) {

        hdr_generic* hg = (hdr_generic*)ptr;
        GHDR_GET_LEN(hg, len);

        if (len == 0 || len < sizeof(hdr_generic))
            break;

        if (hcs(ptr, sizeof(hdr_generic))) {
            //NSLOBJ_WARN("[WARN] \e[1;31mHCS is not match.\e[m\n");
            break;
        }

        procSubscriberStation(hg);
    }

    freePacket(ep);
    return 0;
}

int
mac802_16_MeshSS::procSubscriberStation(hdr_generic* hg)
{
    uint16_t tx_node_id;
    Neighbor *nbr;

    int cid;
    int len;
    char* ptr = reinterpret_cast<char*>(hg);
    GHDR_GET_CID(hg, cid);
    GHDR_GET_LEN(hg, len);

    NSLOBJ_DEBUG("Extract PDU: CID=%d LEN=%d\n", cid, len);

    if (hg->ci) {

        len -= 4;
        unsigned int crc = crc32(0, ptr, len);

        if (memcmp(&crc, ptr + len, 4) != 0) {

            NSLOBJ_INFO("CRC Error (%08x)\n", crc);
            return -1;

        }
    }

    if ((hg->type & 0x20) == 0) { // if not Mesh type

        return -1;
    }

    memcpy(&tx_node_id, ptr + sizeof(hdr_generic), sizeof(uint16_t));

    Mesh_connection connection(cid);

    NSLOBJ_DEBUG("Recv a pkt from node %u. "
            "(CID: %u, LinkID: %u, LNetID: %u)\n",
            tx_node_id, cid, connection.link_id(),
            connection.logic_network_id());

    if (hg->ht == 1) {

        return 0;

    }

    if (connection.link_id() == Mesh_connection::ALL_NET_BROADCAST) {

        mgmt_msg* mm = (mgmt_msg*)(ptr + sizeof(hdr_generic) + 2);

        if (mm->type == MG_MSHNCFG) {

            NSLOBJ_DEBUG("Got MG_MSHNCFG\n");
            procMSHNCFG(mm, cid, tx_node_id, len - sizeof(hdr_generic) - 2);
        }
        else if (mm->type == MG_MSHNENT) {

            NSLOBJ_DEBUG("Got MG_MSHNENT\n");
            _net_entry_mngr->proc_nent(mm, tx_node_id, len - sizeof(hdr_generic) - 2, timerFrame);
        }
        else if (mm->type == MG_MSHDSCH) {

            NSLOBJ_DEBUG("Got MG_MSHDSCH\n");
            procMSHDSCH(mm, cid, tx_node_id, len - sizeof(hdr_generic) - 2);
        }
    } else if (connection.link_id() == 0xfe) {
        /*
         * Sponsor Channel
         */

        mgmt_msg* mm = (mgmt_msg*)(ptr + sizeof(hdr_generic) + 2);

        NSLOBJ_DEBUG("Recv message in Sponsor Channel: type = %d, state = %s\n",
                mm->type, _state_str());

        if (_net_entry_mngr->is_sponsoring()) {

            if (mm->type == MG_SBCREQ) {

                NSLOBJ_INFO("Got MG_SBCREQ from Node %u \n", tx_node_id);

                _net_entry_mngr->proc_sbcreq(mm, len - sizeof(hdr_generic) - 2);
            }
            else if (mm->type == MG_REGREQ) {

                NSLOBJ_INFO("Got MG_REGREQ from Node %u \n", tx_node_id);

		Packet* regreq_pkt_p = _net_entry_mngr->proc_regreq(mm, len - sizeof(hdr_generic) - 2);

		if (regreq_pkt_p)
                    put_pkt_to_upper_layer(regreq_pkt_p);
            }
        }
        else if (_net_entry_mngr->is_negotiate_cap()) {

            if (mm->type == MG_SBCRSP) {

                NSLOBJ_INFO("Got MG_SBCRSP from Node %u \n", tx_node_id);

                if (_net_entry_mngr->proc_sbcrsp())
                    _net_entry_mngr->send_regreq();
            }
        }
        else if (_net_entry_mngr->is_register()) {

            if (mm->type == MG_REGRSP) {

                NSLOBJ_INFO("Got MG_REGRSP from Node = %u\n", tx_node_id);

                _net_entry_mngr->proc_regrsp(mm, len - sizeof(hdr_generic) - 2);
            }
        }
    }
    else {

        if (!(nbr = _nbr_mngr->neighbor(tx_node_id)))
            return 0;

        if (!nbr->tx_link() || connection.link_id() != nbr->tx_link()->id())
            return 0;

        mgmt_msg* mm = NULL;

        if (_net_entry_mngr->is_sponsoring() &&
                connection.type() == Mesh_connection::TYPE_MAC_MANAGEMENT) {
            /*
             * Process MAC Management message.
             * Extract management message from this packet.
             */
            in_addr_t dummy_srcip;
            mm = (mgmt_msg*)_net_entry_mngr->extract(
                    ptr + sizeof(hdr_generic) + 2,
                    len, dummy_srcip, inet_address());
        }

        /*
         * process received management message.
         */
        if (mm) {
            /*
             * Send to Sponsor Channel.
             */
            ifmgmt *ifmm = new ifmgmt(mm->type, len - 1);
            ifmm->appendField(len - 1, reinterpret_cast<u_char*>(mm) + 1);
            SponsorConnection->Insert(ifmm);

        }
        else {
            /*
             * Pass to upper layer routing module.
             */
            ConnectionReceiver* connection_recv =
                nbr->tx_link(connection.link_id())->connection(cid);

            connection_recv->insert(hg, len);

            char* ptr2;
            while ((ptr2 = connection_recv->getPacket(len))) {

                Packet *UpPkt = asPacket(ptr2, len);

                UpPkt->pkt_setflow(PF_RECV);

                NSLOBJ_DEBUG("Route a pkt from node %u. "
                        "(CID: %u, LinkID: %u, LNetID: %u)\n",
                        tx_node_id, cid,
                        connection.link_id(),
                        connection.logic_network_id());

                Event* ep = createEvent();
                ep->DataInfo_ = UpPkt;
                put(ep, recvtarget_);
            }
        }
    }
    return 0;
}

int
mac802_16_MeshSS::send(Event* ep)
{
    Packet*     pkt = NULL;
    ip*         ih = NULL;
    Neighbor*   nbr = NULL;

    GET_PKT(pkt, ep);
    ep->DataInfo_ = NULL;
    freePacket(ep);

    NSLOBJ_DEBUG("Send a packet from upper module. (len = %d).\n",
            pkt->pkt_getlen() );

    int* pNextHop = (int*)pkt->pkt_getinfo("nextHop");

    if (pNextHop == NULL) {

        NSLOBJ_FATAL("It depends on MeshRoute\n");
    }
    else {

        nbr = _nbr_mngr->neighbor(*pNextHop);
    }

    if (nbr && nbr->is_link_active()) {

        char *buf = pkt->pkt_sget();

        ih = reinterpret_cast<ip*>(buf);

        uint16_t Cid;
        if (ih->ip_p == IPPROTO_UDP ) {

            udphdr* uh = reinterpret_cast<udphdr*>(buf + (ih->ip_hl << 2));

            if (uh->dest == 54707)
                Cid = Mesh_connection::gen_id(nbr->rx_link()->id(),
                    Mesh_connection::TYPE_MAC_MANAGEMENT,
                    Mesh_connection::RELIABILITY_NO_RE_XMIT,
                    Mesh_connection::CLASS_GENERAL, 0);
            else
                Cid = Mesh_connection::gen_id(nbr->rx_link()->id(),
                    Mesh_connection::TYPE_IP,
                    Mesh_connection::RELIABILITY_NO_RE_XMIT,
                    Mesh_connection::CLASS_GENERAL, 0);
        }
        else

            Cid = Mesh_connection::gen_id(nbr->rx_link()->id(),
                Mesh_connection::TYPE_IP,
                Mesh_connection::RELIABILITY_NO_RE_XMIT,
                Mesh_connection::CLASS_GENERAL, 0);
 
        /*
         * The link to transmit must exist.
         */
        Mesh_link_tx* tx_link;
        assert((tx_link = _net_entry_mngr->tx_link(nbr->rx_link()->id())));

        /*
         * The connection to transmit must exist.
         */

        Mesh_connection* connection;
        assert(connection = tx_link->connection(Cid));

        /*
         * Drop the packet if the queue of the connection is full.
         */

        if (connection->nf_pending_packet() > static_cast<size_t>(_maxqlen)) {

            NSLOBJ_DEBUG("Data Connection's queue is full, maxqlen = %u. \n", _maxqlen);
            delete pkt;
            return 1;
        }
        else {

            connection->Insert(pkt);

            NSLOBJ_DEBUG("Insert a packet (to node %d) into connection %#x, nf_pending_packet = %u\n",
                *pNextHop, Cid, connection->nf_pending_packet());

            return 0;
        }
    }
    else {

        if (!nbr) {

            NSLOBJ_INFO("Cannot find corresponding neighbor %d!\n", *pNextHop);
        }
        else {

            NSLOBJ_INFO("\e[1;31mchosen neighbor: %03u, state = ",
                    nbr->node_id());
            INFO_FUNC(nbr->dump_link_state("\e[m\n"));
        }

        delete pkt;
    }
    return 0;
}

void
mac802_16_MeshSS::find_time_sync(uint8_t slot_no, double rtt)
{
    uint64_t firstTick = _fr_mngr->find_time_sync(slot_no, rtt);

    uint64_t timeInTick;
    MILLI_TO_TICK(timeInTick, frame_duration());

    NSLOBJ_DEBUG("Schedule the frame tick = %llu, period = %llu\n",
            firstTick + GetCurrentTime(), timeInTick);

    timerFrame->cancel();
    timerFrame->start(firstTick, timeInTick);
}

int
mac802_16_MeshSS::T6(Event_*)
{
    NSLOBJ_INFO("\n");

    if (_net_entry_mngr->is_register())
        _net_entry_mngr->send_regreq();

    return 0;
}

int
mac802_16_MeshSS::T18(Event_*)
{
    NSLOBJ_INFO("\n");

    if (_net_entry_mngr->is_negotiate_cap()) {
        _net_entry_mngr->send_sbcreq();
    }
    return 0;
}

int
mac802_16_MeshSS::T25(Event_*)
{
    _net_entry_mngr->t25();

    return 0;
}

int
mac802_16_MeshSS::StartFrame(Event_*)
{
    NSLOBJ_DEBUG("frame_start_tick: \e[1;36m%llu\e[m ==> ",
            _fr_mngr->frame_start_tick());
    _fr_mngr->update_cur_frame();
    DEBUG_FUNC(printf("\e[1;36m%llu\e[m\n", _fr_mngr->frame_start_tick()));

#if 0
    if (get_nid() == 18 || get_nid() == 2) {
        NSLOBJ_INFO("cur_frame = %#03x, %s, state = %s\n",
                _fr_mngr->cur_frame(),
                _fr_mngr->is_net_ctrl_frame()?"NetCtrl":"SchCtrl",
                _state_str());
    }
#endif

    if (_fr_mngr->is_net_ctrl_frame()) {
        /*
         * Network Control Subframe: Send MSH-NENT or schedule MSH-NCFG message.
         */
        if (is_net_entry_contend()) {

            const MSH_NENT* nent = _net_entry_mngr->dequeue_nent();

            if (nent) {

                NSLOBJ_INFO("Transmit MSH-NENT message (type: %u) \n",
                        nent->getEntryType());

                CtrlConnection->Insert(nent);

                SendCtrlMessage((mac80216_burst_t)nent->getType());
            }
        } else if (is_operational()) {

            NSLOBJ_DEBUG("cur_frame = %#03x, nent_opp_seq_end = %u, ncfg_opp_seq_end = %u\n",
                    _fr_mngr->cur_frame(), _fr_mngr->nent_opp_seq_end(), _fr_mngr->ncfg_opp_seq_end());
            if (_fr_mngr->is_valid_ncfg_tx_opp_slot_no()) {

                timerSendNCFG->start(_fr_mngr->next_tx_tick(), 0);

                NSLOBJ_DEBUG("next_tx_tick = %llu, tx_opp_slot_no = %u\n",
                        _fr_mngr->next_tx_tick(), _fr_mngr->tx_opp_slot_no());
                NSLOBJ_DEBUG("next_tx_opp = %u (%lf), max_ncfg_tx_opp = %u\n",
                        _ncfg_scheduler->next_tx_opp(),
                        (double)(GetCurrentTime() + _fr_mngr->next_tx_tick()) / 10000000,
                        _fr_mngr->max_ncfg_tx_opp());
            }
        }
    }
    else {
        /*
         * Schedule control subframe: Schedule MSH-DSCH message.
         */
        if (is_operational() && _fr_mngr->is_valid_dsch_tx_opp_slot_no())
            timerSendDSCH->start(_fr_mngr->next_tx_tick(), 0);
    }

    /*
     * Process Data Subframe.
     */
    if (!_data_scheduler->job_list().empty()) {

        NSLOBJ_DEBUG("cur_frame = %#03x, %s, state = %s\n",
                _fr_mngr->cur_frame(),
                _fr_mngr->is_net_ctrl_frame()?"NetCtrl":"SchCtrl",
                _state_str());

        NSLOBJ_FATAL("Previous jobs are not finished yet!\n");
    }

    uint64_t data_subframe_start_ticks;
    MICRO_TO_TICK(data_subframe_start_ticks, _fr_mngr->nf_ctrl_symbol() * Ts());
    timerCheckValidity->start(data_subframe_start_ticks, 0);

    return 0;
}

int
mac802_16_MeshSS::SendMSHNCFG(Event_*)
{
    uint32_t cur_tx_opp = _ncfg_scheduler->next_tx_opp();
    uint32_t max_tx_opp = _fr_mngr->max_ncfg_tx_opp();

    /*
     * Schedule the next MSH-NCFG transmission.
     */
    _ncfg_scheduler->ht_assigner()->update_ncfg_sched_info();
    _ncfg_scheduler->update_next_tx_opp();


    /* Transmit current MSH-NCFG. */
    MSH_NCFG* ncfg = new MSH_NCFG();
    ncfg->setXmtParameter(_tx_params->power(), _tx_params->antenna());
    ncfg->setTimeStamp(_fr_mngr->cur_frame(), _fr_mngr->tx_opp_slot_no(), _net_entry_mngr->sync_hop_count());
    NSLOBJ_DEBUG("cur_frame = %#03x, tx_opp_slot_no = %u\n",
            _fr_mngr->cur_frame(), _fr_mngr->tx_opp_slot_no());

    ncfg->setSchedParameter(cur_tx_opp, _ncfg_scheduler->next_tx_opp(),
                _ncfg_scheduler->ht_assigner()->sch_info()->tx_holdoff_exp(), max_tx_opp);

    /* C.C. Lin: fill in Base Station Information List.
     * Note that in MSH-NCFG, it is allowed that
     * only at most three base stations are reported.
     * Due to this limitation, we put the currently attached base
     * station in the first position, and choose two other
     * proper base stations to be reported in this message if they
     * exist.
     */

    int reported_bs_cnt = _bs_info->fill_in_msgncfg_bsentries(ncfg);

    NSLOBJ_DEBUG("the number of reported base stations = %d\n", reported_bs_cnt);

    /* fill in physical neighbor list */
    for (Neighbor_mngr::map_t::const_iterator it = _nbr_mngr->neighbors().begin();
            it != _nbr_mngr->neighbors().end(); it++) {

        Neighbor* nbr = it->second;
        Ctrl_sch::Info_nbr* sch_info = _ncfg_scheduler->nbr_sch_info(nbr->node_id());

        if (!sch_info->is_reported() && nbr->hop_count() == 1) {

            sch_info->set_reported(true);

            Ctrl_sch::Entry nbr_entry;
            nbr_entry.set_node_id(nbr->node_id());
            nbr_entry.set_hops_to_nbr(nbr->hop_count() - 1);
            nbr_entry.set_est_prop_delay(nbr->est_prop_delay());
            sch_info->fill_sch_info(nbr_entry, cur_tx_opp, max_tx_opp);

            ncfg->add_nbr_entry(nbr_entry);
#if 0
            if (_node_id == 4) {
                NSLOBJ_INFO("Nbr(%03u): mx = %u, exp = %u, ", 
                        nbr_entry.node_id(),
                        nbr_entry.next_tx_mx(),
                        nbr_entry.tx_holdoff_exp());
                INFO_FUNC(sch_info->dump());
            }
#endif
        }
    }

    _net_entry_mngr->append_sponsoring_info(ncfg);


    // Network Descriptor
    Msh_ncfg::Network_Des_ie* descriptorIE = new Msh_ncfg::Network_Des_ie(_fr_mngr);
    ncfg->addEmbedded(descriptorIE);

    // Network Link Establishment Protocol
    _net_entry_mngr->link_establish(ncfg);

#if 0
    if (_node_id == 96 || _node_id == 89) {
        NSLOBJ_INFO("Send a MSH-NCFG:\n");
        NSLOBJ_INFO("dump embedded IE:\n");
        ncfg->dumpEmbedded();
    }
#endif

    CtrlConnection->Insert(ncfg);

    SendCtrlMessage((mac80216_burst_t)ncfg->getType());

    /*
     * Deal with cases where this node wins another NCFG transmission opportunities within
     * one frame. The timerSendNCFG timer should be configured here because the regular
     * configuration for this time is in frame-by-frame manner. As a result, the granularity
     * of the frame-by-frame triggering is not fast enough for all cases.
     */
    _fr_mngr->assure_ncfg_next_tx_opp_slot_distinct();
    if (_fr_mngr->is_valid_ncfg_tx_opp_slot_no())
        timerSendNCFG->start(_fr_mngr->next_tx_tick(), 0);

    /*
     * Statistics.
     */
    record_used_txopp(TxOppUtilizationCounter::msh_ncfg, get_nid(), cur_tx_opp);

    return 0;
}

/*
 * Note: Only coordinated distributed scheduling is implemented now.
 */
int
mac802_16_MeshSS::SendMSHDSCH(Event_*)
{
    uint32_t cur_tx_opp = _dsch_scheduler->next_tx_opp();

    /* Create a MSH-DSCH message. */
    MSH_DSCH* dsch = new MSH_DSCH();

    dsch->setGrantFlag(0);
    dsch->setSeqCounter(0);  // FIXME: Not implemented yet

    dsch->add_request_info(*_data_scheduler,
            _fr_mngr->cur_frame(), cur_tx_opp,
            _nf_requested_data_schedules);

    dsch->add_grant_info(*_data_scheduler, _fr_mngr->cur_frame(), cur_tx_opp);

    dsch->add_confirm_info(*_data_scheduler, _fr_mngr->cur_frame(),
            cur_tx_opp, _nf_granted_data_schedules);

    NSLOBJ_DEBUG("curTxopp: %u \n", cur_tx_opp);

    /*
     * Schedule the next MSH-DSCH transmission.
     */
    _dsch_scheduler->ht_assigner()->update_dsch_sched_info();
    _dsch_scheduler->update_next_tx_opp();

    DEBUG("next_tx_opp: %u, tx_holdoff_opp: %u\n",
            _dsch_scheduler->next_tx_opp(),
            _dsch_scheduler->ht_assigner()->sch_info()->tx_holdoff_opp());

    /* Deal with cases where this node wins another DSCH transmission
     * opportunity within one frame.
     */

    uint32_t max_tx_opp = _fr_mngr->max_dsch_tx_opp();
    uint32_t next_tx_opp = _dsch_scheduler->next_tx_opp();

    /* wrapping case */
    if ( next_tx_opp < cur_tx_opp ) {

        NSLOBJ_DEBUG("DSCH Txopp number is wrapped. (next_tx_opp=%u, cur_tx_opp=%u)\n",
            next_tx_opp, cur_tx_opp);

        next_tx_opp += max_tx_opp;

    }

    if ((next_tx_opp - cur_tx_opp) <= static_cast<uint32_t>(_fr_mngr->nf_ncfg_tx_opp())) {

        _fr_mngr->assure_dsch_next_tx_opp_slot_distinct();
        if (_fr_mngr->is_valid_dsch_tx_opp_slot_no()) {

            timerSendDSCH->start(_fr_mngr->next_tx_tick(), 0);

            NSLOBJ_DEBUG("Add a timer for MSH-DSCH with tx_opp_slot_no = %u\n",
                    _fr_mngr->tx_opp_slot_no());
            NSLOBJ_DEBUG("\tnext_tx_opp = %u, dsch_opp_seq_end = %u\n",
                    _dsch_scheduler->next_tx_opp(),
                    _fr_mngr->dsch_opp_seq_end());
        }
    }

    /* CCLin: We defer the action to fill in scheduling parameters until now
     * because we want to dynamically change the holdoff time to
     * shorten the required times of three-way handshake procedure
     * for establishing a data schedule.
     */


    dsch->addSchedParam(cur_tx_opp, _dsch_scheduler->next_tx_opp(),
                            _dsch_scheduler->ht_assigner()->sch_info()->tx_holdoff_exp(), max_tx_opp);


    for (Neighbor_mngr::map_t::const_iterator it = _nbr_mngr->neighbors().begin();
            it != _nbr_mngr->neighbors().end(); it++) {

        Neighbor* nbr = it->second;
        Ctrl_sch::Info_nbr* sch_info = _dsch_scheduler->nbr_sch_info(nbr->node_id());

        if (!sch_info->is_reported() && nbr->hop_count() == 1) {

            sch_info->set_reported(true);

            Ctrl_sch::Entry nbr_entry;
            nbr_entry.set_node_id(nbr->node_id());
            sch_info->fill_sch_info(nbr_entry, cur_tx_opp, max_tx_opp);

            dsch->add_nbr_sch_entry(nbr_entry);
        }
    }

    CtrlConnection->Insert(dsch);
    SendCtrlMessage((mac80216_burst_t)dsch->getType());

    record_used_txopp(TxOppUtilizationCounter::msh_dsch, get_nid(), cur_tx_opp);

    return 0;
}

int
mac802_16_MeshSS::SendData(Event_*)
{
    /*
     * Get a job from the job list.
     */
    const schedule_alloc_t* job = _data_scheduler->pop_head_job();
    /*
     * The job list should not be empty here.
     */
    assert(job);

    NSLOBJ_DEBUG("The job pop from the job list: (number of rest jobs: %u)\n",
                _data_scheduler->job_list().size());
    DEBUG_FUNC(job->dump());

    switch (job->status()) {
    case Data_scheduler::SENT_NET_ENTRY:
    case Data_scheduler::RECV_NET_ENTRY:
        /*
         * Transmissions in Sponsor Channel
         * cclin: this separate transmission routing increases
         *        the complexity of the logging function.
         */
        SendSponsor();
        break;

    case Data_scheduler::SENT_CONFIRM:
    {
        Packet* p = new Packet;
        p->pkt_setflow(PF_SEND);

        struct PHYInfo phyInfo;
        memset(&phyInfo, 0, sizeof(struct PHYInfo));

	/* The uncoded block size of QAM64_3_4 is 108 bytes */
        phyInfo.fec = QAM64_3_4;
        phyInfo.nsymbols = _fr_mngr->slot_size() * job->slot_range();
        p->pkt_addinfo("phyInfo", (char*)&phyInfo, sizeof(struct PHYInfo));

        int maxlen = WiMaxBurst::symbolsToBytes(phyInfo.nsymbols, phyInfo.fec);

        uint8_t* buf = reinterpret_cast<uint8_t*>(p->pkt_sattach(maxlen));
        //p->pkt_sprepend(buf, maxlen);

        /*
         * Normal data allocation.
         */
        uint16_t cid = Mesh_connection::gen_id(job->link_id(),
                Mesh_connection::TYPE_IP,
                Mesh_connection::RELIABILITY_NO_RE_XMIT,
                Mesh_connection::CLASS_GENERAL, 0);
        /*
         * Because the Std defines a Mesh CID is composed of the priority reliable and drop fields,
         * we don't maintain these difference. Only LinkID field are.
         * The link to transmit must exist.
         */
        Mesh_link_tx* tx_link;
        assert((tx_link = _net_entry_mngr->tx_link(job->link_id())));

        if (tx_link->nf_pending_packet()) {
            NSLOBJ_DEBUG("pending_data_len = %u, nf_pending_packet = %u\n",
                    tx_link->pending_data_len(),
                    tx_link->nf_pending_packet());
        }
        unsigned int sendlen = tx_link->encapsulate(buf, maxlen);

        if (sendlen) {
            NSLOBJ_DEBUG("link[%u]: send data packets.\n", job->link_id());

            Neighbor *nbr = _nbr_mngr->neighbor_with_link_rx_id(job->link_id());

            if (!nbr) {

                NSLOBJ_ERROR("Cannot find neighbor by linkID\n");

            }

            NSLOBJ_DEBUG("Send data packet(len=%d) to node %u\n", sendlen, nbr->node_id());
            p->pkt_sprepend(reinterpret_cast<char*>(buf), sendlen);


            struct LogInfo log_info;
            memset(&log_info, 0, sizeof(struct LogInfo));

            log_info.src_nid = get_nid();
            log_info.dst_nid = nbr->node_id();
            log_info.burst_type = BT_DATA;
            log_info.channel_id = 0;
            log_info.burst_len = sendlen;
            log_info.connection_id = cid;

            p->pkt_addinfo("loginfo", (char*)&log_info, sizeof(struct LogInfo));

            Event* ep = createEvent();
            ep->DataInfo_ = reinterpret_cast<void*>(p);
            put(ep, sendtarget_);
        } else
            delete p;
        break;
    }

    default:
        assert(0);
    }

    /*
     * Schedule next sending time of the next job.
     */
    if (!_data_scheduler->job_list().empty()) {

        /*
         * Save previous job sending time.
         */
        uint8_t slot_start_prev_job = job->slot_start();

        job = _data_scheduler->job_list().begin()->second;

        double sched_alloc_time =
            (job->slot_start() - slot_start_prev_job) * _fr_mngr->slot_size() * Ts();

        uint64_t firstTick;
        MICRO_TO_TICK(firstTick, sched_alloc_time);

        timerSendData->start(firstTick, 0);
        NSLOBJ_DEBUG("slot_start_prev_job = %d, "
                    "Ts = %f, sched_alloc_time = %lf\n",
                    slot_start_prev_job, Ts(), sched_alloc_time);
        NSLOBJ_DEBUG("firstTick = %llu, Sending tick: %llu, job:\n",
                    firstTick, GetCurrentTime() + firstTick);
        DEBUG_FUNC(job->dump());
    }

    return 0;
}

void
mac802_16_MeshSS::SendCtrlMessage(mac80216_burst_t msg_type)
{
    Event *ep;
    Packet *p;

    struct PHYInfo phyInfo;
    struct LogInfo log_info;

    memset(&log_info, 0, sizeof(struct LogInfo));

    char *buf;
    int len;

    p = new Packet;
    p->pkt_setflow(PF_SEND);

    len = CtrlConnection->GetInformation(NULL);

    int check_res = _check_ctrl_msg_len(msg_type, len);

    if (!check_res) {

        NSLOBJ_FATAL("The control message length is too large to fit in a transmission opportunity.\n");

    }


    buf = p->pkt_sattach(len);
    p->pkt_sprepend(buf, len);
    CtrlConnection->EncapsulateAll(buf, len);

    memset(&phyInfo, 0, sizeof(struct PHYInfo));
    phyInfo.fec = QPSK_1_2;
    phyInfo.nsymbols = _fr_mngr->nf_symbol_per_tx_opp();
    p->pkt_addinfo("phyInfo", (char *) &phyInfo,
            sizeof(struct PHYInfo));

    log_info.src_nid = get_nid();
    log_info.dst_nid = PHY_BROADCAST_ID;
    log_info.burst_type = msg_type;
    log_info.channel_id = 0;
    log_info.burst_len = len;
    log_info.connection_id = 0;
    p->pkt_addinfo("loginfo", (char *) &log_info,
            sizeof(struct LogInfo));

    ep = createEvent();
    ep->DataInfo_ = p;

    NSLOBJ_DEBUG("Send a CTRL message (type:%u) at tick = %llu.\n", msg_type , GetCurrentTime());

    put(ep, sendtarget_);
}

void
mac802_16_MeshSS::SendSponsor()
{
    Event *ep;
    Packet *p;

    struct PHYInfo phyInfo;
    struct LogInfo log_info;

    memset(&log_info, 0, sizeof(struct LogInfo));

    char *buf;
    int len;

    len = SponsorConnection->GetInformation(NULL);
    if (len) {

        p = new Packet;
        p->pkt_setflow(PF_SEND);

        buf = p->pkt_sattach(len);
        p->pkt_sprepend(buf, len);
        SponsorConnection->EncapsulateAll(buf, len);

        int cid = SponsorConnection->getCID();
        int dst_nid = SponsorConnection->GetDstNodeID();
        int src_nid = SponsorConnection->GetSrcNodeID();


        NSLOBJ_INFO("Send to sponsor connection, srcnid = %u, dstnid = %u\n",
            src_nid, dst_nid);

        log_info.src_nid = get_nid();
        log_info.dst_nid = dst_nid;
        log_info.burst_type = BT_SPONSOR;
        log_info.channel_id = 0;
        log_info.burst_len = len;
        log_info.connection_id = cid;

        p->pkt_addinfo("loginfo", (char *) &log_info,
                sizeof(struct LogInfo));

        memset(&phyInfo, 0, sizeof(struct PHYInfo));
        phyInfo.fec = QPSK_1_2;
        phyInfo.nsymbols = _fr_mngr->nf_symbol_per_tx_opp();
        p->pkt_addinfo("phyInfo", (char *) &phyInfo,
                sizeof(struct PHYInfo));

        ep = createEvent();
        ep->DataInfo_ = p;
        put(ep, sendtarget_);
    }
}

int
mac802_16_MeshSS::procMSHNCFG(struct mgmt_msg *recvmm, int cid, uint16_t tx_node_id, int len)
{
    /*
     * Receive the first NCFG from the candidate node.
     */
    _net_entry_mngr->stop_sponsoring_due_to_recv_node_id(tx_node_id);

    MSH_NCFG ncfg(reinterpret_cast<u_char*>(recvmm), len - 1, _net_entry_mngr->link_est_ie_format());

    /*
     * Processing information element.
     */
    for (int i = 0; i < ncfg.getNumEmbedded(); i++)
        ncfg.getEmbedded(i)->proc(this, tx_node_id);


    if (!is_initialized()) {

        /* C.C. Lin: This procedure should be refined with the aid of base station info list.
         * With the help of that bs_info_list, the SS is able to choose a better BS node to
         * attach. In this condition, the failure rate of attaching a huge WiMAX mesh network
         * can be reduced significantly.
         */

        /* Start entry process if we receive MSH-NCFG from the same node twice. */
        if (_nbr_mngr->neighbor(tx_node_id) != NULL) {

            set_net_entry_sync();
            NSLOBJ_INFO("MSH_INIT => MSH_NET_ENTRY_SYNC, tx_node_id = %u\n", tx_node_id);
        }
    }
    else if (is_net_entry_sync())
        _net_entry_mngr->proc_ncfg_network_entry_sync(ncfg, tx_node_id);

    else if (is_net_entry_contend())
        _net_entry_mngr->proc_ncfg_network_entry_contend(ncfg, tx_node_id);

    else if (is_operational()) {

        /* C.C. Lin:
         * After processing an MSH-NCFG message, check if this message
         * contains linkEstablishmentIE I expect.
         */

        Neighbor* nbr = _nbr_mngr->neighbor(tx_node_id);
        if (nbr) {

            NSLOBJ_DEBUG("Dump tx node:\n");
            DEBUG_FUNC(nbr->dump());

            if (nbr->is_link_wait_response()) {
                /*
                 * Retransmit a new LinkChallenge message because we don't receive an expected one.
                 */
                nbr->set_link_sent_challenge();
                NSLOBJ_DEBUG("Prepare to re-send linkSendChallenge message to node %u.\n", tx_node_id);

            } else if (nbr->is_link_wait_accept()) {
                /*
                 * Retransmit a new LinkResponse message because we don't receive an expected one.
                 */
                nbr->set_link_sent_response();
                //unnecessary action: createRecvLink(nbr->rcvLinkID, nbr->nodeID);
                NSLOBJ_DEBUG("Prepare to re-send linkSendResponse message to node %u.\n", tx_node_id);
            }
        }
    }

    _bs_info->update(&ncfg, tx_node_id);
    _nbr_mngr->update_neighbors(this, ncfg, tx_node_id);

#if 0
    if (_node_id == 0 || _node_id == 0) {
        NSLOBJ_DEBUG("\nDump Neighbor List: (tx_node_id = %u)\n", tx_node_id);
        DEBUG_FUNC(_nbr_mngr->dump());
    }
#endif
    return 0;
}

int
mac802_16_MeshSS::procMSHDSCH(struct mgmt_msg* recvmm, int cid, uint16_t tx_node_id, int len)
{
    if (!is_operational())
        return 1;

    /*
     * This case will happen if MSH-DSCH is transmitted before the first
     * transmission of MSH-NCFG.
     */
    _net_entry_mngr->stop_sponsoring_due_to_recv_node_id(tx_node_id);

    MSH_DSCH dsch(reinterpret_cast<u_char*>(recvmm), len - 1);

    if (dsch.getCoordFlag() == 0 && dsch.getGrantFlag() == 0) {
        // Coordinated
        _nbr_mngr->update_neighbors(this, dsch, tx_node_id);
    }
    else {
        // Uncoordinated
        FATAL("Uncoordinated mode is not supported now\n");
        exit(0);
    }

    NSLOBJ_DEBUG("tx_node_id=%d,seqCnt=%d\n",
            tx_node_id, dsch.getSeqCounter());

    Neighbor* nbr = _nbr_mngr->neighbor(tx_node_id);
#if 0
    if ((_node_id == 1 && tx_node_id == 98) || (_node_id == 98 && tx_node_id == 1)) {
        NSLOBJ_INFO("dump tx_node %d:\n", tx_node_id);
        INFO_FUNC(nbr->dump());
    }
#endif

    dsch.proc_availability(*_data_scheduler, nbr, _fr_mngr->cur_frame());
    dsch.proc_request(*_data_scheduler, nbr);
    dsch.proc_grant(*_data_scheduler, nbr, _fr_mngr->cur_frame());
    dsch.proc_confirm(*_data_scheduler, nbr, _fr_mngr->cur_frame());

    return 0;
}

/*
 * Push to data queue or pass to upper routing module
 */
void
mac802_16_MeshSS::put_pkt_to_upper_layer(Packet* pkt)
{
    assert(pkt);

    pkt->pkt_setflow(PF_RECV);

    Event* ep = createEvent();
    ep->DataInfo_ = reinterpret_cast<void*>(pkt);
    put(ep, recvtarget_);
}

int
mac802_16_MeshSS::reschedule_next_ncfg(uint32_t max_txopp_num)
{
    uint32_t cur_tx_opp = _ncfg_scheduler->next_tx_opp();
    uint32_t next_tx_opp = _ncfg_scheduler->update_next_tx_opp();
    NSLOBJ_INFO("tx_opp from %u to %u\n", cur_tx_opp, next_tx_opp);

    /*
     * Deal with cases where this node wins another NCFG transmission
     * opportunities within one frame.
     */

    /* wrapping case */
    if ( next_tx_opp < cur_tx_opp ) {

        NSLOBJ_DEBUG("NCFG Txopp number is wrapped. (next_tx_opp=%u, cur_tx_opp=%u)\n",
            next_tx_opp, cur_tx_opp);

        next_tx_opp += max_txopp_num;
    }

    /*
     * The timer is restarted only in the case that the next tx_opp
     * was scheduled in the same frame of previous one.
     */
    timerSendNCFG->cancel();
    if (_fr_mngr->is_ncfg_tx_opps_within_the_same_frame(cur_tx_opp, next_tx_opp)) {

        _fr_mngr->assure_ncfg_next_tx_opp_slot_distinct();
        if (_fr_mngr->is_valid_ncfg_tx_opp_slot_no())
            timerSendNCFG->start(_fr_mngr->next_tx_tick(), 0);
        else
            assert(0);
    }

    return 1;
}

void
mac802_16_MeshSS::start_pseudo_scheduler()
{
    if (!_pseudo_sched_flag)
        return;

    uint64_t pseudo_sched_period; /* unit: sec */
    SEC_TO_TICK(pseudo_sched_period, 3);

    uint64_t ps_scheduler_start_time;
    SEC_TO_TICK(ps_scheduler_start_time, 300);
    timerPseudoScheduler->start(ps_scheduler_start_time, pseudo_sched_period);
}

int
mac802_16_MeshSS::_check_ctrl_msg_len( u_int32_t msg_type, u_int32_t len) const
{
    int fec      = QPSK_1_2;
    int nsymbols = 0;

     if (msg_type == CTLMSG_TYPE_MSH_NCFG ) {

        nsymbols = 2;

     }
     else if (msg_type == CTLMSG_TYPE_MSH_NENT ) {

        nsymbols = 4;

     }
     else if (msg_type == CTLMSG_TYPE_MSH_DSCH ) {

        nsymbols = 4;

     }
     else if (msg_type == CTLMSG_TYPE_MSH_CSCH ) {

        nsymbols = 4;

     }
     else if (msg_type == CTLMSG_TYPE_MSH_CSCF ) {

        nsymbols = 4;

     }
     else {

        NSLOBJ_FATAL("unknown control message type.\n");

     }

     return WiMaxBurst::symbolsToBytes(nsymbols, fec);
}

int
mac802_16_MeshSS::_self_determine_start_to_function_timepoint(Event_*)
{
    u_int32_t node_num = getNumOfNodes();

    u_int32_t my_nid = get_nid();

    double      min_bs_dist = pow(10,10);
    u_int32_t   min_dist_bs_nid = 0;
    u_int64_t   determined_start_to_function_time = 0x0;

    for (unsigned int i = 1; i <= node_num; ++i) {

        const char* node_name = getNodeName(i);

        NSLOBJ_DEBUG("nodename = %s \n", node_name);


        const char* find_str_p = strstr(node_name,"WIMAX_MESH_BS");

        if (find_str_p) {

            double dist = GetNodeDistance(my_nid, i);

            if (dist < min_bs_dist) {

                min_bs_dist = dist;
                min_dist_bs_nid = i;

            }


        }

    }

    NSLOBJ_INFO ("determine connected BS nid = %d distance = %lf\n",
                min_dist_bs_nid, min_bs_dist );

    /* Process the case in which I am just a BS */
    if (my_nid == min_dist_bs_nid) {

        /* start to function immediately. */
        determined_start_to_function_time = 0;

    }
    else {

        /* computer the proper timepoint to start to attach to the network. */

        double      one_hop_dist = DEFAULT_ONE_HOP_DISTANCE;
        u_int32_t   diff_base    = DEFAULT_FUNCTIONAL_TIME_DIFF;

        u_int32_t estimated_hop_cnt = static_cast<u_int32_t> (min_bs_dist/one_hop_dist);

        if (estimated_hop_cnt<1)
            estimated_hop_cnt = 1;

        u_int32_t estimated_functional_timepoint = (estimated_hop_cnt-1)*diff_base;

        u_int32_t random_factor = random() % diff_base;
        determined_start_to_function_time =
                static_cast<u_int64_t> (estimated_functional_timepoint + random_factor);

    }

    SEC_TO_TICK(_start_to_function_tick, determined_start_to_function_time);

    NSLOBJ_INFO ("chosen start_to_function_tick = %lf\n", (double)(_start_to_function_tick/10000000) );

    return 0;
}

int
mac802_16_MeshSS::_check_sched_validity(Event_*)
{
    _data_scheduler->remove_expired_alloc(_fr_mngr->cur_frame());

    if (!_data_scheduler->refresh_job_list())
        return 1;


    if (_node_id == 5 || _node_id == 5) {

        NSLOBJ_DEBUG("job_list: (cur_frame = %#03x)\n", _fr_mngr->cur_frame());
        DEBUG_FUNC(_data_scheduler->job_list_dump());
    }

    uint64_t firstTick = 0;
    const schedule_alloc_t* job = _data_scheduler->job_list().begin()->second;
    unsigned int symbols = job->slot_start() * _fr_mngr->slot_size();
    MICRO_TO_TICK(firstTick, symbols * Ts());
    timerSendData->start(firstTick, 0);

    return 0;
}

int
mac802_16_MeshSS::_pseudo_scheduler(Event_*)
{
    if (_data_scheduler)
        _data_scheduler->set_forced_request();

    return 0;
}

const char*
mac802_16_MeshSS::_state_str() const
{
    enum { STR_LEN = 32 };
    static char state[STR_LEN];

    switch (_state) {
    case MSH_INIT:              strncpy(state, "MSH_INIT", STR_LEN); break;
    case MSH_NET_ENTRY_SYNC:    strncpy(state, "MSH_NET_ENTRY_SYNC", STR_LEN); break;
    case MSH_NET_ENTRY_CONTEND: strncpy(state, "MSH_NET_ENTRY_CONTEND", STR_LEN); break;
    case MSH_OPERATIONAL:       strncpy(state, "MSH_OPERATIONAL", STR_LEN); break;
    default: assert(0);
    }

    return state;
}

void
mac802_16_MeshSS::_dump_log_to_file()
{
    //NSLOBJ_STAT("\n");

#if 0
    if (_ncfg_scheduler->ht_assigner()->sch_info()) {
        NSLOBJ_STAT("NCFG Local schedule infomations:\n");
        _ncfg_scheduler->ht_assigner()->sch_info()->dump();
    }
    if (_dsch_scheduler->ht_assigner()->sch_info()) {
        NSLOBJ_STAT("DSCH Local schedule infomations:\n");
        _dsch_scheduler->ht_assigner()->sch_info()->dump();
    }
#endif

    static FILE*    fp = NULL;

    bool show_link_stat_flag = false;

    /* The information of Link Establishment Process */
    size_t est_link_cnt = _nbr_mngr->nf_active_link();

    if ( show_link_stat_flag ) {

        NSLOBJ_STAT("num_of_established_links = %u. node_degree = %d\n",
                est_link_cnt, _node_deg_des->nf_1_hop_neighbors());

    }

    bool show_entry_proc_and_dolink_log_flag = true;

    if (show_entry_proc_and_dolink_log_flag) {

    	if (!fp) {
           char filepath[200];

           sprintf(filepath, "%s.stat", GetScriptName());
	   fp = fopen(filepath, "w");
	}


        const Nent_and_link_est_log* nent_log = _net_entry_mngr->nent_log();

        if (_node_id == 1 || nent_log->is_end()) {

            fprintf(fp, "[%03u] network entry process:\t\t\tstart: %11.7f end: %11.7f diff: %11.7f\n",
                    _node_id,
                    nent_log->start_sec(),
                    nent_log->end_sec(),
                    nent_log->period_in_sec());

            if (est_link_cnt == _node_deg_des->nf_1_hop_neighbors()) {

                for (Neighbor_mngr::map_t::const_iterator it = _nbr_mngr->neighbors().begin();
                        it != _nbr_mngr->neighbors().end(); it++) {

                    Neighbor* nbr = it->second;

                    if (nbr->hop_count() == 1)
                        fprintf(fp, "[%03u] doLink protocol establishment with [%03u]:"
                                "\tstart: %11.7f end: %11.7f diff: %11.7f\n",
                                _node_id, nbr->node_id(),
                                nbr->link_est_log()->start_sec(),
                                nbr->link_est_log()->end_sec(),
                                nbr->link_est_log()->period_in_sec());
                }

                const Nent_and_link_est_log* link_est_log = _net_entry_mngr->link_est_log();
                fprintf(fp, "[%03u] Global doLink protocol establishment:\tstart: %11.7f end: %11.7f diff: %11.7f\n\n",
                        _node_id,
                        link_est_log->start_sec(),
                        link_est_log->end_sec(),
                        link_est_log->period_in_sec());
            }
            else {
                fprintf(fp, "[%03u] doLink protocol with all neighbors have not finished yet.\n\n", _node_id);
            }

        }
        else {

            fprintf(fp, "[%03u] network entry process have not finished yet.\n", _node_id);

            if (est_link_cnt == _node_deg_des->nf_1_hop_neighbors()) {

                for (Neighbor_mngr::map_t::const_iterator it = _nbr_mngr->neighbors().begin();
                        it != _nbr_mngr->neighbors().end(); it++) {

                    Neighbor* nbr = it->second;

                    if (nbr->hop_count() == 1)
                        fprintf(fp, "[%03u] doLink protocol establishment with [%03u]:"
                                "\tstart: %11.7f end: %11.7f diff: %11.7f\n",
                                _node_id, nbr->node_id(),
                                nbr->link_est_log()->start_sec(),
                                nbr->link_est_log()->end_sec(),
                                nbr->link_est_log()->period_in_sec());
                }

                const Nent_and_link_est_log* link_est_log = _net_entry_mngr->link_est_log();
                fprintf(fp, "[%03u] Global doLink protocol establishment:\tstart: %11.7f end: %11.7f diff: %11.7f\n\n",
                        _node_id,
                        link_est_log->start_sec(),
                        link_est_log->end_sec(),
                        link_est_log->period_in_sec());

            }
            else {
                fprintf(fp, "[%03u] doLink protocol with all neighbors have not finished yet.\n\n", _node_id);
            }
        }
    }
}


/*
 * Member function definitions of class `mac802_16_MeshSS::Tx_params'.
 */

/*
 * Constructor
 */
mac802_16_MeshSS::Tx_params::Tx_params()
: _power(15) // 38 dBm (See 6.3.2.3.35)
, _antenna(0)
{
}
/*
 * Destructor
 */
mac802_16_MeshSS::Tx_params::~Tx_params()
{
}
