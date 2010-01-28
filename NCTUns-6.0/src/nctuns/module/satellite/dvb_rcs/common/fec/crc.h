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

#ifndef __NCTUNS_crc_h__
#define __NCTUNS_crc_h__

#include <sys/types.h>

#define CRC8(data, len, poly, init)	\
	(u_int8_t)Crc::crc((void *)data, len, 8, poly, init)
#define CRC16(data, len, poly, init)	\
	(u_int16_t)Crc::crc((void *)data, len, 16, poly, init)
#define CRC32(data, len, poly, init)	\
	(u_int32_t)Crc::crc((void *)data, len, 32, poly, init)

#define IEEE_8023_CRC32_POLY      0x04C11DB7      /* 802.3 CRC-32 Polynomial */
class Crc {

private:

public:
	static u_int32_t	crc(void *user_packet, u_int32_t len, u_int32_t crc_len, u_int32_t gen_poly, int init_value);
};

#endif	/* __NCTUNS_crc_h__ */
