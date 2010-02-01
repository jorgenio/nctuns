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

/*
 *  Universal TUN/TAP device driver.
 *  Copyright (C) 1999-2000 Maxim Krasnyansky <max_mk@yahoo.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  $Id: if_tun.h,v 1.2 2001/06/01 18:39:47 davem Exp $
 */

#ifndef __IF_TUN_H
#define __IF_TUN_H

/* Read queue size */
#define TUN_READQ_SIZE	500

/* TUN device flags */
#define TUN_TUN_DEV 	0x0001	
#define TUN_TAP_DEV	0x0002
#define TUN_TYPE_MASK   0x000f

#define TUN_FASYNC	0x0010
#define TUN_NOCHECKSUM	0x0020
#define TUN_NO_PI	0x0040
#define TUN_ONE_QUEUE	0x0080
#define TUN_PERSIST 	0x0100	
//NCTUNS_start
#define TUN_UP		0x1000
#define TUN_DOWN	0x2000
//NCTUNS_end

/* Ioctl defines */
#define TUNSETNOCSUM  _IOW('T', 200, int) 
#define TUNSETDEBUG   _IOW('T', 201, int) 
#define TUNSETIFF     _IOW('T', 202, int) 
#define TUNSETPERSIST _IOW('T', 203, int) 
#define TUNSETOWNER   _IOW('T', 204, int)
#define TUNSETLINK    _IOW('T', 205, int)
#define TUNSETGROUP   _IOW('T', 206, int)
//NCTUNS_start
#define TUNSETUD      _IOW('T', 207, int)
#define TUNGETUD      _IOW('T', 208, int)
//NCTUNS_end

/* TUNSETIFF ifr flags */
#define IFF_TUN		0x0001
#define IFF_TAP		0x0002
#define IFF_NO_PI	0x1000
#define IFF_ONE_QUEUE	0x2000

struct tun_pi {
	unsigned short flags;
	unsigned short proto;
};
#define TUN_PKT_STRIP	0x0001

#endif /* __IF_TUN_H */
