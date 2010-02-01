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

#ifndef __NCTUNS_WIMAX_CTRL_SCH_INFO_MNGR_H__
#define __NCTUNS_WIMAX_CTRL_SCH_INFO_MNGR_H__


#include <inttypes.h>


namespace Ctrl_sch {
    class Entry;
    class Entry_interface;
}

class Ctrl_sch::Entry_interface {

public:
    Entry_interface();
    virtual ~Entry_interface();

    virtual void get_sch_param(Ctrl_sch::Entry&) = 0;
    virtual uint8_t get_nf_nbr_sch_entry() const = 0;
    virtual void get_nbr_sch_entry(Ctrl_sch::Entry&, int) const = 0;
};


class Ctrl_sch::Entry {

private:
    enum {
        UNKNOWN_NODE_ID         = 0,
        UNKNOWN_HOPS_TO_NBR     = 0xff,
        UNKNOWN_EST_PROP_DELAY  = 0xff,
        UNKNOWN_XMT_MX          = 0x1f,
    };

private:
    uint16_t    _node_id;
    uint8_t     _hops_to_nbr;
    uint8_t     _est_prop_delay;
    uint8_t     _next_tx_mx        :5;
    uint8_t     _tx_holdoff_exp    :3;

public:
    Entry();
    virtual ~Entry();

    inline void set_node_id(uint16_t id) { _node_id = id; }
    inline void set_hops_to_nbr(uint8_t hops) { _hops_to_nbr = hops; }
    inline void set_est_prop_delay(uint8_t delay) { _est_prop_delay = delay; }
    inline void set_next_tx_mx(uint8_t mx) { _next_tx_mx = mx; }
    inline void set_tx_holdoff_exp(uint8_t exp) { _tx_holdoff_exp = exp; }
    inline uint8_t node_id() const { return _node_id; }
    inline uint8_t hops_to_nbr() const { return _hops_to_nbr; }
    inline uint8_t est_prop_delay() const { return _est_prop_delay; }
    inline uint8_t next_tx_mx() const { return _next_tx_mx; }
    inline uint8_t tx_holdoff_exp() const { return _tx_holdoff_exp; }
    inline bool is_valid_hops_to_nbr() const { return _hops_to_nbr != UNKNOWN_HOPS_TO_NBR; }
    inline bool is_valid_est_prop_delay() const { return _est_prop_delay != UNKNOWN_EST_PROP_DELAY; }
    inline bool is_valid_next_tx_mx() const { return _next_tx_mx != UNKNOWN_XMT_MX; }
};


#endif /* __NCTUNS_WIMAX_CTRL_SCH_INFO_MNGR_H__ */
