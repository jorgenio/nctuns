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

#ifndef __NCTUNS_dvbrcs_api_h__
#define __NCTUNS_dvbrcs_api_h__

#include <event.h>

#ifndef MIN
#define MIN(x, y)	((x) < (y) ? (x) : (y))
#endif	/* MIN */

#ifndef MAX
#define MAX(x, y)	((x) > (y) ? (x) : (y))
#endif	/* MAX */

/*
 * node identify magic number
 */
enum dvb_node_type {
	NODE_ID_SAT	= 0xDD01,
	NODE_ID_NCC	= 0xD010,
	NODE_ID_RCST	= 0x010D,
	NODE_ID_SP	= 0x10DD,
	NODE_ID_NONE	= 0x0000
};

class Dvb_pkt;

ePacket_ *dvb_pkt_copy(ePacket_ *src);
int dvb_freePacket(ePacket_ *pkt);
int calculate_bit_error(void *data, int len, double ber);

int is_forward_control(Dvb_pkt *pkt);
#endif	/* __NCTUNS_dvbrcs_api_h__ */
