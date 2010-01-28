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

#include "mac_header.h"


uchar DownlinkMacHeader::encode() {
    uchar res = ( (usf<<5) | sp << 4 | rrbp<<2 | payload_type );
    return res;
}

int DownlinkMacHeader::decode(uchar octet ) {
    usf 	    = (octet >> 5)  & 0x07;
    sp		    = (octet >> 4)  & 0x01;
    rrbp	    = (octet >> 2)  & 0x03;
    payload_type    = (octet) 	    & 0x03;
    return 1;
}

uchar UplinkMacHeader::encode() {
    uchar res;
    if ( !datablk_or_ctlblk ) { /* data header */
        res = ( (rbit<<7) | (si<<6) | (countdown_value<<2) | (payload_type) );
    }
    else { /* control header */
        res = 0x00;
        res = ( rbit<<7 | (payload_type) );
    }
    return res;
}

int UplinkMacHeader::decode(uchar octet) {

    payload_type = octet & 0x03;

    if ( payload_type == 0x00 ) { /* data header */
        datablk_or_ctlblk = 0;
        rbit = (octet>>7) & 0x01;
    }
    else { /* control header */
        datablk_or_ctlblk = 1;
        rbit    = (octet>>7) & 0x01;
        si      = (octet>>6) & 0x01;
        countdown_value = (octet>>2) & 0x0f;
    }
    return 1;
}
