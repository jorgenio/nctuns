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

#ifndef __HT_ASSIGNER_H__
#define __HT_ASSIGNER_H__

#include <vector>


namespace Ctrl_sch {

    class Info;
    class Ht_assigner;
}
class Ctrl_scheduler;
class mac802_16_MeshSS;

class Ctrl_sch::Ht_assigner {

public:
    enum {
        MAX_HT_EXP = 0x07,
    };

    typedef enum {

        not_specified               = 0,
        assigned_by_default_value   = 1,

    } ht_assignment_mode_t;

    typedef enum {

        undefined           = 0,

    } ht_function_mode_t;

protected:
    ht_assignment_mode_t    _ht_assignment_mode;
    ht_function_mode_t      _ht_func_mode;

    const mac802_16_MeshSS* _mac;
    Ctrl_scheduler*         _scheduler;
    Ctrl_sch::Info*         _sch_info;

public:
    explicit Ht_assigner(
            Ctrl_scheduler*,
            ht_assignment_mode_t,
            ht_function_mode_t,
            const Ctrl_sch::Info&);
    virtual ~Ht_assigner();

    virtual void dump() const;

    virtual void update_ncfg_sched_info() { ; }
    virtual void update_dsch_sched_info() { ; }
    inline const Ctrl_sch::Info* sch_info() const { return _sch_info; }
};


#endif /* __HT_ASSIGNER_H__ */
