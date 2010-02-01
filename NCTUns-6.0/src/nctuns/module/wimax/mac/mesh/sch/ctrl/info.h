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

#ifndef __NCTUNS_WIMAX_CTRL_SCH_INFO_H__
#define __NCTUNS_WIMAX_CTRL_SCH_INFO_H__


#include <inttypes.h>


namespace Ctrl_sch {
    class Info;
    class Info_nbr;
}
class Ctrl_sch::Info {

private:
    enum {
        UNKNOWN_XMT_OPP  = 0xffffffff,
    };

private:
    uint8_t     _tx_holdoff_exp_base;
    uint8_t     _tx_holdoff_exp;

public:
    explicit Info(uint8_t, uint8_t);
    virtual ~Info();

    void dump() const;

    inline void set_tx_holdoff_exp_base(uint8_t base) { _tx_holdoff_exp_base = base; }
    inline void set_tx_holdoff_exp(uint8_t exp) { _tx_holdoff_exp = exp; }
    inline uint8_t tx_holdoff_exp_base() const { return _tx_holdoff_exp_base; }
    inline uint8_t tx_holdoff_exp() const { return _tx_holdoff_exp; }
    inline uint32_t tx_holdoff_opp() const { return 1 << (_tx_holdoff_exp_base + _tx_holdoff_exp); }
    inline static bool is_valid_tx_opp(uint32_t tx_opp) { return tx_opp != UNKNOWN_XMT_OPP; }
    inline static uint32_t invalid_tx_opp() { return UNKNOWN_XMT_OPP; }
};


class Neighbor;
namespace Ctrl_sch {
    class Entry;
};

class Ctrl_sch::Info_nbr : public Ctrl_sch::Info {

private:
    Neighbor*   _neighbor;
    uint32_t    _next_tx_opp_start;
    uint32_t    _next_tx_opp_end;
    bool        _reported;

public:
    explicit Info_nbr(uint8_t, uint8_t, Neighbor*);
    virtual ~Info_nbr();

    void update(const Ctrl_sch::Entry&, uint32_t, uint32_t);
    void fill_sch_info(Ctrl_sch::Entry&, uint32_t, uint32_t);
    bool is_eligible(uint32_t, uint32_t) const;
    void dump() const;

    inline void set_reported(bool reported) { _reported = reported; }
    inline Neighbor* neighbor() const { return _neighbor; }
    inline uint32_t next_tx_opp_start() const { return _next_tx_opp_start; }
    inline uint32_t next_tx_opp_end() const { return _next_tx_opp_end; }
    inline bool is_reported() const { return _reported; }
    inline bool is_valid_next_tx_opp() const { return is_valid_tx_opp(_next_tx_opp_start); }
};


#endif /* __NCTUNS_WIMAX_CTRL_SCH_INFO_H__ */
