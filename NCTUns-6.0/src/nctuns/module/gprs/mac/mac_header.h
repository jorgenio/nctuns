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

#ifndef __MAC_HEADER__
#define __MAC_HEADER__

#include <gprs/include/types.h>

typedef class DownlinkMacHeader {
    friend class GprsMsMac;
    friend class GprsBtsMac;

    private:
        uchar payload_type;
        uchar rrbp;
        uchar sp;
        uchar usf;
    public:
        DownlinkMacHeader() : payload_type(0), rrbp(0), sp(0), usf(0)
	{};
        uchar   encode();
        int     decode(uchar);

} DMH;

typedef class UplinkMacHeader {
    friend class GprsMsMac;
    friend class GprsBtsMac;
    private:
        uchar datablk_or_ctlblk; /* 0 stands for datablk, 1 stands for ctlblk */
        uchar payload_type;
        uchar countdown_value; /* this value is set as 15 until the number of remaining data
    			        * blocks to be sent is less than 15 */
        uchar si;
        uchar rbit;
    public:
        UplinkMacHeader() : datablk_or_ctlblk(0), payload_type(0), countdown_value(0), si(0), rbit(0)
	{};
        uchar   encode();
        int     decode(uchar);

} UMH;

#endif
