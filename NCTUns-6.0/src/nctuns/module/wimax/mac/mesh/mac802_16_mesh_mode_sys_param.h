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

#ifndef __80216_MESH_SYSTEM_PARAM


/*
 * Several parameters that have significant impact
 * on 802.16 network performances are collected
 * together for easy adjustment and reducing
 * bugs caused by inconsistent settings.
 */


/*
 * Number of nodes.
 */
#define TOTAL_MESH_SS_NODES             100

#define DEFAULT_TXOPPS_NUM              8

#define DEFAULT_HOLDOFF_EXP_BASE        4
#define DEFAULT_TX_HOLDOFF_EXP          1

#define DEFAULT_ONE_HOP_DISTANCE        500.0
#define DEFAULT_FUNCTIONAL_TIME_DIFF    10

#define MAX_LINK_ID                     255

#define __RECORD_TXOPP_USE__            1


/*
 * Data scheduling efficiency enhancing mechanisms.
 */
#define SCHEDULING_EARLY_THREE_WAY_HANDSHAKE    0


#endif /* __80216_MESH_SYSTEM_PARAM */
