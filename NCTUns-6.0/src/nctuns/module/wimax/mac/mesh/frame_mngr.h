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

#ifndef __NCTUNS_WIMAX_FRAME_MNGR_H__
#define __NCTUNS_WIMAX_FRAME_MNGR_H__


#include <inttypes.h>


struct PHYInfo;
class mac802_16_MeshSS;

class Frame_mngr {

private:
    enum {
        /*
         * See 8.3.5.3
         */
        NF_SYMBOL_PER_TX_OPP    = 7,

        FRAME_BIT_MASK          = 0x0fff,
        UNKNOWN_TX_OPP_SLOT_NO  = 0x00ff,
    };

private:
    mac802_16_MeshSS*   _mac;

    uint16_t            _cur_frame;
    uint64_t            _frame_start_tick;
    PHYInfo*            _last_signal_info;

    uint8_t             _tx_opp_slot_no;
    uint8_t             _nf_slot;
    uint8_t             _slot_size;

    uint8_t             _nf_tx_opp;
    uint8_t             _nf_dsch_tx_opp;
    uint8_t             _csch_data_fraction;
    uint8_t             _nf_sch_frame_idx;
    uint32_t            _nf_net_ctrl_frame;

    uint32_t            _nent_opp_seq_end;
    uint32_t            _ncfg_opp_seq_end;
    uint32_t            _dsch_opp_seq_end;

public:
    explicit Frame_mngr(mac802_16_MeshSS*);
    virtual ~Frame_mngr();

    void set_nf_sch_frame_idx(uint8_t);
    void set_cur_frame(uint16_t);
    void update_cur_frame();
    void update_last_signal_info(PHYInfo*);
    void fix_prop_delay(uint64_t delta = 0);
    uint64_t prop_delay_tick() const;
    uint64_t find_time_sync(uint8_t slot_no, double rtt);

    uint32_t cur_ncfg_tx_opp(uint8_t) const;
    uint32_t cur_dsch_tx_opp() const;

    uint32_t max_nent_tx_opp() const;
    uint32_t max_ncfg_tx_opp() const;
    uint32_t max_dsch_tx_opp() const;

    bool is_ncfg_tx_opps_within_the_same_frame(uint32_t, uint32_t) const;
    bool is_valid_ncfg_tx_opp_slot_no();
    bool is_valid_dsch_tx_opp_slot_no();
    uint64_t next_tx_tick() const;
    void assure_ncfg_next_tx_opp_slot_distinct() const;
    void assure_dsch_next_tx_opp_slot_distinct() const;
    void dump() const;

    inline void set_nf_slot(uint8_t nf_slot) { _nf_slot = nf_slot; }
    inline void set_slot_size(uint8_t size) { _slot_size = size; }

    inline void set_nf_tx_opp(uint8_t tx_opp) { _nf_tx_opp = tx_opp; }
    inline void set_nf_dsch_tx_opp(uint8_t tx_opp) { _nf_dsch_tx_opp = tx_opp; }
    inline void set_csch_data_fraction(uint8_t fraction) { _csch_data_fraction = fraction; }

    inline uint16_t cur_frame() const { return _cur_frame; }
    inline uint64_t frame_start_tick() const { return _frame_start_tick; }

    inline uint8_t tx_opp_slot_no() const { return _tx_opp_slot_no; }
    inline uint8_t nf_slot() const { return _nf_slot; }
    inline uint8_t slot_size() const { return _slot_size; }

    inline uint8_t nf_tx_opp() const { return _nf_tx_opp; }
    inline uint8_t nf_ncfg_tx_opp() const { return _nf_tx_opp - 1; }
    inline uint8_t nf_dsch_tx_opp() const { return _nf_dsch_tx_opp; }
    inline uint8_t csch_data_fraction() const { return _csch_data_fraction; }
    inline uint8_t nf_sch_frame_idx() const { return _nf_sch_frame_idx; }

    inline uint32_t nent_opp_seq_end() const { return _nent_opp_seq_end; }
    inline uint32_t ncfg_opp_seq_end() const { return _ncfg_opp_seq_end; }
    inline uint32_t dsch_opp_seq_end() const { return _dsch_opp_seq_end; }

    inline bool is_net_ctrl_frame() const { return cur_frame() % (_nf_sch_frame_idx * 4 + 1) == 0; }
    inline bool is_valid_tx_opp_slot_no() const { return _tx_opp_slot_no != UNKNOWN_TX_OPP_SLOT_NO; }

    inline uint8_t nf_symbol_per_tx_opp() const { return NF_SYMBOL_PER_TX_OPP; }
    inline uint8_t nf_ctrl_symbol() const { return NF_SYMBOL_PER_TX_OPP * _nf_tx_opp; }

private:
    void _reset_opp_seq_end();
};


#endif  /* __NCTUNS_WIMAX_FRAME_MNGR_H__ */
