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

#include "ht_assigner.h"

#include "info.h"
#include "scheduler.h"
#include "../../mac802_16_meshss.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "../../../verbose.h"


using namespace Ctrl_sch;


Ht_assigner::Ht_assigner(
        Ctrl_scheduler* scheduler,
        ht_assignment_mode_t mode,
        ht_function_mode_t func_mode,
        const Info& sch_info)
: _ht_assignment_mode(mode)
, _ht_func_mode(func_mode)
, _mac(scheduler->mac())
, _scheduler(scheduler)
, _sch_info(new Info(sch_info))
{
    switch (mode) {

    case assigned_by_default_value:
        _ht_func_mode = undefined;
        break;

    case not_specified:
    default:
        assert(0);
    }
}

Ht_assigner::~Ht_assigner()
{
    if (_sch_info)
        delete _sch_info;
}

void
Ht_assigner::dump() const
{
    const char* assign_mode;
    switch (_ht_assignment_mode) {

    case assigned_by_default_value: assign_mode = "assigned_by_default_value"; break;
    case not_specified:
    default:
        assert(0);
    }

    const char* func_mode;
    switch (_ht_func_mode) {
    case undefined:         func_mode = "undefined"; break;
    default: assert(0);
    }

    STAT("[%03u]assign mode: \e[1;33m%s\e[m\n", _mac->get_nid(), assign_mode);
    STAT("[%03u]function mode: %s\n", _mac->get_nid(), func_mode);
    STAT("[%03u]tx_holdoff_exp_base = %u, tx_holdoff_exp = %u\n",
            _mac->get_nid(), _sch_info->tx_holdoff_exp_base(), _sch_info->tx_holdoff_exp());
}
