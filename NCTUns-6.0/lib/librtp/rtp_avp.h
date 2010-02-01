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
 * Request for Comments: 3551
 * RTP Prole for Audio and Video Conferences with Minimal Control
 * RTP/AVP
 */


/********************************************************************/


#ifndef _RTP_AVP_H
#define _RTP_AVP_H


/********************************************************************/


#define RTP_MAX_PT 128

int RTP_TIMERATE[RTP_MAX_PT] = {
	125, 0, 0, 125, 125, 125, 63, 125, 125, 125, 	/* 0-9 */
 	23, 23, 125, 125, 11, 125, 91, 45, 125, 0,      /* 10-19 */
 	0, 0, 0, 0, 0, 11, 11, 0, 11, 0,                /* 20-29 */
 	0, 11, 11, 11, 11, 0, 0, 0, 0, 0,               /* 30-39 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 40-49 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 50-59 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 60-69 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 70-79 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 80-89 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 90-99 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 100-109 */
 	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,                   /* 110-119 */
 	0, 0, 0, 0, 0, 0, 0, 0				/* 120-127 */
 };


/********************************************************************/


#endif /* _RTP_AVP_H */
 
