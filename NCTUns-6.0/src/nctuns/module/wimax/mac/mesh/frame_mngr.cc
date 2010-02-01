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

#include "frame_mngr.h"

#include <cstdio>
#include <nctuns_api.h>
#include <math.h>
#include "mac802_16_meshss.h"
#include "sch/ctrl/info.h"
#include "sch/ctrl/scheduler.h"
#include "../structure.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"


/*
 * Member function definitions of class `Frame_mngr'.
 */

/*
 * Constructor
 */
Frame_mngr::Frame_mngr(mac802_16_MeshSS* mac)
: _mac(mac)

, _cur_frame(0)
, _frame_start_tick(0)
, _last_signal_info(NULL)

, _tx_opp_slot_no(UNKNOWN_TX_OPP_SLOT_NO)
, _nf_slot(0)
, _slot_size(0)

, _nf_tx_opp(0)
, _nf_dsch_tx_opp(0)
, _csch_data_fraction(0)
, _nf_sch_frame_idx(0)

, _nent_opp_seq_end(0)
, _ncfg_opp_seq_end(0)
, _dsch_opp_seq_end(0)
{
}


/*
 * Destructor
 */
Frame_mngr::~Frame_mngr()
{
}

void
Frame_mngr::set_nf_sch_frame_idx(uint8_t idx)
{
    _nf_sch_frame_idx = idx;
    _nf_net_ctrl_frame = (FRAME_BIT_MASK + 1) / (idx * 4 + 1) + 1;
}

void
Frame_mngr::set_cur_frame(uint16_t frame)
{
    assert(_nf_tx_opp && _nf_dsch_tx_opp);

    _cur_frame = frame & FRAME_BIT_MASK;

    _nent_opp_seq_end = _cur_frame / (_nf_sch_frame_idx * 4 + 1) + 1;
    _ncfg_opp_seq_end = _nent_opp_seq_end * nf_ncfg_tx_opp();
    _dsch_opp_seq_end = (_cur_frame + 1 - _nent_opp_seq_end) * _nf_dsch_tx_opp;
    /*
     * The DSCH opportunity counting is refined to make it possible
     * coworking with Centralized Scheduling. Original:
     * _dsch_opp_seq_end = (_cur_frame + 1) * _nf_dsch_tx_opp - _nent_opp_seq_end - _ncfg_opp_seq_end;
     */

    DEBUG("[%03u]%s: _nent_opp_seq_end = %u, _ncfg_opp_seq_end = %u, dsch_opp_seq_end = %u\n",
            _mac->node_id(), __func__,
            _nent_opp_seq_end, _ncfg_opp_seq_end, _dsch_opp_seq_end);
}

void
Frame_mngr::update_cur_frame()
{
    assert(_nf_tx_opp && _nf_dsch_tx_opp);

    _cur_frame = (_cur_frame + 1) & FRAME_BIT_MASK;
    _frame_start_tick = GetCurrentTime();

    if (cur_frame() == 0)
        _reset_opp_seq_end();

    if (is_net_ctrl_frame()) {

        _nent_opp_seq_end += 1;
        _ncfg_opp_seq_end += nf_ncfg_tx_opp();

    } else
        _dsch_opp_seq_end += _nf_dsch_tx_opp;
}

void
Frame_mngr::update_last_signal_info(PHYInfo* info)
{
    _last_signal_info = info;
}

/*
 * Fix the propagation delay between two SSs.
 */
void
Frame_mngr::fix_prop_delay(uint64_t delta)
{
    if (_last_signal_info->frameStartTime < _frame_start_tick - delta) {

        uint64_t diff = (_frame_start_tick - _last_signal_info->frameStartTime);
        VINFO("Frame_mngr::%s: diff = %llu, restore it.\n", __func__, diff);
        _last_signal_info->frameStartTime = _frame_start_tick;
    }
}

uint64_t
Frame_mngr::prop_delay_tick() const
{
    return _last_signal_info->frameStartTime - _frame_start_tick;
}

uint64_t
Frame_mngr::find_time_sync(uint8_t slot_no, double rtt)
{
    double delay = rtt / 2.0 / 1000.0;

    DEBUG("[%03u]%s: slot_no = %u, rtt = %lf, delay = %lf\n",
            _mac->node_id(), __func__, slot_no, rtt, delay);

    uint64_t first_tick;
    MILLI_TO_TICK(first_tick,
            _mac->frame_duration() - (slot_no + 1) * NF_SYMBOL_PER_TX_OPP * _mac->Ts() / 1000.0 - delay);

    /*
     * See OFDM_Mesh::recv()
     */
    first_tick += 60;

    return first_tick;
}

uint32_t
Frame_mngr::cur_ncfg_tx_opp(uint8_t slot_no) const
{
    return _ncfg_opp_seq_end - (_nf_tx_opp - 1 - slot_no);
}

uint32_t
Frame_mngr::cur_dsch_tx_opp() const
{
    double micro;
    uint64_t msg_recv_time_in_tick = _last_signal_info->frameStartTime;
    TICK_TO_MICRO(micro, msg_recv_time_in_tick - _frame_start_tick);

    return _dsch_opp_seq_end - (_nf_dsch_tx_opp - 1 - (int)round(micro / _mac->Ts() / NF_SYMBOL_PER_TX_OPP));
}

uint32_t
Frame_mngr::max_nent_tx_opp() const
{
    return _nf_net_ctrl_frame;
}

uint32_t
Frame_mngr::max_ncfg_tx_opp() const
{
    if (!_nf_tx_opp)
        FATAL("request for obtaining maximum NCFG TxOpp at the too early stage.\n");

    return _nf_net_ctrl_frame * nf_ncfg_tx_opp();
}

uint32_t
Frame_mngr::max_dsch_tx_opp() const
{
    if (!_nf_dsch_tx_opp)
        FATAL("request for obtaining maximum DSCH TxOpp at the too early stage.\n");

    return (4096 - _nf_net_ctrl_frame) * _nf_dsch_tx_opp;
}

bool
Frame_mngr::is_ncfg_tx_opps_within_the_same_frame(
        uint32_t tx_opp_0, uint32_t tx_opp_1) const
{
    return tx_opp_0 / nf_ncfg_tx_opp() == tx_opp_1 / nf_ncfg_tx_opp();
}

bool
Frame_mngr::is_valid_ncfg_tx_opp_slot_no()
{
    uint32_t next_tx_opp = _mac->ncfg_scheduler()->next_tx_opp();

    _tx_opp_slot_no = _nf_tx_opp - 1 - (_ncfg_opp_seq_end - next_tx_opp);

    /*
     * The _tx_opp_slot_no is valid only if the opportunity is in the current
     * network control subframe.
     */
    if (!(1 <= _tx_opp_slot_no && _tx_opp_slot_no <= _nf_tx_opp - 1))
        _tx_opp_slot_no = UNKNOWN_TX_OPP_SLOT_NO;

    return _tx_opp_slot_no != UNKNOWN_TX_OPP_SLOT_NO;
}

bool
Frame_mngr::is_valid_dsch_tx_opp_slot_no()
{
    uint32_t next_tx_opp = _mac->dsch_scheduler()->next_tx_opp();

    _tx_opp_slot_no = _nf_tx_opp - 1 - (_dsch_opp_seq_end - next_tx_opp);

    if (!(_tx_opp_slot_no <= _nf_tx_opp - 1))
        _tx_opp_slot_no = UNKNOWN_TX_OPP_SLOT_NO;

    return _tx_opp_slot_no != UNKNOWN_TX_OPP_SLOT_NO;
}

/*
 * XXX: Ugly implementation. The checking function above should be called
 *      before using following function.
 */
uint64_t
Frame_mngr::next_tx_tick() const
{
    assert(_tx_opp_slot_no != UNKNOWN_TX_OPP_SLOT_NO);

    uint64_t tx_tick;
    MICRO_TO_TICK(tx_tick, _tx_opp_slot_no * NF_SYMBOL_PER_TX_OPP * _mac->Ts());

    tx_tick += _frame_start_tick - GetCurrentTime();

    return tx_tick;
}

/*
 * Debugging functions.
 */
void
Frame_mngr::assure_ncfg_next_tx_opp_slot_distinct() const
{
    uint32_t next_tx_opp = _mac->ncfg_scheduler()->next_tx_opp();

    uint8_t tx_opp_slot_no = _nf_tx_opp - 1 - (_ncfg_opp_seq_end - next_tx_opp);

    if (!(1 <= tx_opp_slot_no && tx_opp_slot_no <= _nf_tx_opp - 1))
        tx_opp_slot_no = UNKNOWN_TX_OPP_SLOT_NO;

    if (tx_opp_slot_no == _tx_opp_slot_no) {

        ERROR("Next won tx opp is the same as the previous one.\n");
        ERROR("ncfg_opp_seq_end = %u, next_tx_opp = %u, tx_opp_slot_no: prev = %u, next = %u\n",
                _ncfg_opp_seq_end, next_tx_opp, _tx_opp_slot_no, tx_opp_slot_no);
        assert(0);
    }
}

void
Frame_mngr::assure_dsch_next_tx_opp_slot_distinct() const
{
    uint32_t next_tx_opp = _mac->dsch_scheduler()->next_tx_opp();

    uint8_t tx_opp_slot_no = _nf_tx_opp - 1 - (_dsch_opp_seq_end - next_tx_opp);

    if (!(tx_opp_slot_no <= _nf_tx_opp - 1))
        tx_opp_slot_no = UNKNOWN_TX_OPP_SLOT_NO;

    if (tx_opp_slot_no == _tx_opp_slot_no) {

        ERROR("Next won tx opp is the same as the previous one.\n");
        ERROR("dsch_opp_seq_end = %u, next_tx_opp = %u, tx_opp_slot_no: prev = %u, next = %u\n",
                _dsch_opp_seq_end, next_tx_opp, _tx_opp_slot_no, tx_opp_slot_no);
        assert(0);
    }
}

void
Frame_mngr::dump() const
{
    printf("\tcur_frame = %u\n", _cur_frame);
    printf("\tframe_start_tick = %llu\n", _frame_start_tick);

    printf("\ttx_opp_slot_no = %u\n", _tx_opp_slot_no);
    printf("\tnf_slot = %u\n", _nf_slot);
    printf("\tslot_size = %u\n", _slot_size);

    printf("\tnf_tx_opp = %u\n", _nf_tx_opp);
    printf("\tnf_dsch_tx_opp = %u\n", _nf_dsch_tx_opp);
    printf("\tcsch_data_fraction = %u\n", _csch_data_fraction);
    printf("\tnf_sch_frame_idx = %u\n", _nf_sch_frame_idx);
    printf("\tnf_net_ctrl_frame = %u\n", _nf_net_ctrl_frame);

    printf("\tnent_opp_seq_end = %u\n", _nent_opp_seq_end);
    printf("\tncfg_opp_seq_end = %u\n", _ncfg_opp_seq_end);
    printf("\tdsch_opp_seq_end = %u\n", _dsch_opp_seq_end);
}

void
Frame_mngr::_reset_opp_seq_end()
{
    _nent_opp_seq_end = 0;
    _ncfg_opp_seq_end = 0;
    _dsch_opp_seq_end = 0;
}
