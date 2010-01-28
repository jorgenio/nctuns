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

#ifndef __GPRS_NODETYPE_DEF
#define __GPRS_NODETYPE_DEF

#include "types.h"
/* nodetype definition:
 * Notice that the definition of these types may not correspond to
 * nodeType_ in NslObject. The value of nodeType_ in NslObject is controlled
 * by the sequence of nodetype registration and may be changed in each release.
 */

    namespace gprs {
        const ulong ms          = 0;
        const ulong bts         = 1;
        const ulong sgsn        = 2;
        const ulong ggsn        = 3;

        /* 0 stands for sender, 1 stands for receiver , 2 stands for single-block allocation */
        const ulong sender      = 0;
        const ulong receiver    = 1;
        const ulong sba         = 2;

        const uchar random      = 100;
        const uchar free        = 100;
        const uchar rrbp_unused = 100;
        const uchar usf_unused  = 100;
        const uchar not_granted = 200;
    }

#endif
