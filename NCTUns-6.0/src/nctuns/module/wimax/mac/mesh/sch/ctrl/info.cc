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

#include "info.h"

#include <cassert>
#include <cstdio>

#include "entry_interface.h"


using namespace Ctrl_sch;


/*
 * Member function definitions of class `Info'.
 */

/*
 * Constructor
 */
Info::Info(uint8_t tx_holdoff_exp_base, uint8_t tx_holdoff_exp)
: _tx_holdoff_exp_base(tx_holdoff_exp_base)
, _tx_holdoff_exp(tx_holdoff_exp)
{
}

/*
 * Destructor
 */
Info::~Info()
{
}

void
Info::dump() const
{
    printf("\ttx_holdoff_exp = %u, tx_holdoff_opp = %u\n",
            _tx_holdoff_exp, tx_holdoff_opp());
}


/*
 * Member function definitions of class `Info_nbr'.
 */

/*
 * Constructor
 */
Info_nbr::Info_nbr(
		uint8_t tx_holdoff_exp_base, uint8_t tx_holdoff_exp, Neighbor* nbr)
: Info(tx_holdoff_exp_base, tx_holdoff_exp)
, _neighbor(nbr)
, _next_tx_opp_start(invalid_tx_opp())
, _next_tx_opp_end(invalid_tx_opp())
, _reported(false)
{
    assert(nbr);
}

/*
 * Destructor
 */
Info_nbr::~Info_nbr()
{
}

void
Info_nbr::update(
        const Ctrl_sch::Entry& entry, uint32_t cur_tx_opp, uint32_t max_tx_opp)
{
    if (!entry.is_valid_next_tx_mx()) {
        _next_tx_opp_start = invalid_tx_opp();
        _next_tx_opp_end = invalid_tx_opp();
        return;
    }

    set_tx_holdoff_exp(entry.tx_holdoff_exp());
    
    int interval = 1 << entry.tx_holdoff_exp();
    int ref = ((cur_tx_opp - 1) / interval) * interval + 1;
    
    _next_tx_opp_start = ref + entry.next_tx_mx() * interval;
    _next_tx_opp_end = ref + (entry.next_tx_mx() + 1) * interval - 1;
    
    if (_next_tx_opp_start > max_tx_opp) {
        _next_tx_opp_start %= max_tx_opp;
        _next_tx_opp_start += (max_tx_opp % interval);
    }

    if (_next_tx_opp_end > max_tx_opp) {
        _next_tx_opp_end %= max_tx_opp;
        _next_tx_opp_end += (max_tx_opp % interval);
    }
}

void
Info_nbr::fill_sch_info(
        Ctrl_sch::Entry& entry, uint32_t cur_tx_opp, uint32_t max_tx_opp)
{
    if (!is_valid_next_tx_opp())
        return;

    /*
     * The action of resetting NextXmtMx and XmtHoldExp fields
     * are moved out to the caller function. On the one hand,
     * this function can be more concise; on the other hand,
     * the caller has more flexibility to manipulate the 
     * contents of this NCFGInfo structure and those which 
     * will be sent out.
     */
    if (_next_tx_opp_start < cur_tx_opp ||
            _next_tx_opp_start - cur_tx_opp > max_tx_opp / 2)
        return;

    int interval = 1 << tx_holdoff_exp();
    int ref1 = ((cur_tx_opp - 1) / interval) * interval + 1;
    int ref2 = ((_next_tx_opp_start - 1) / interval) * interval + 1;

    /*
     * Wrapping.
     */
    if (ref2 < ref1)
        ref2 += max_tx_opp;

    entry.set_next_tx_mx((ref2 - ref1) / interval);
    entry.set_tx_holdoff_exp(tx_holdoff_exp());
}

bool
Info_nbr::is_eligible(uint32_t tx_opp, uint32_t max_tx_opp) const
{
    /*
     * The unknown schedule case: lack the scheduling information.
     */
    if (!is_valid_next_tx_opp())
        return true;


    /*
     * Process the case in which tx_opp is between
     * _next_tx_opp_start and _next_tx_opp_end.
     */
    if (_next_tx_opp_end < _next_tx_opp_start) {

        // Wrapping
        if ((tx_opp >= _next_tx_opp_start && tx_opp <= max_tx_opp) ||
                tx_opp <= _next_tx_opp_end)
            return true;

    } else if (tx_opp >= _next_tx_opp_start && tx_opp <= _next_tx_opp_end)
        return true;


    /*
     * Process the remaining cases.
     */
    uint32_t earliest_tx_opp = _next_tx_opp_start + tx_holdoff_opp();

    if (earliest_tx_opp > max_tx_opp)
        earliest_tx_opp %= max_tx_opp;

    if (earliest_tx_opp <= tx_opp ||
            (earliest_tx_opp - tx_opp) > (max_tx_opp / 2))
        return true;

    return false;
}

void
Info_nbr::dump() const
{
    Info::dump();
    if (is_valid_next_tx_opp())
        printf("next_tx_opp range: [%u, %u]\n",
                _next_tx_opp_start, _next_tx_opp_end);
    else 
        printf("next_tx_opp range: UNKNOWN\n");
}
