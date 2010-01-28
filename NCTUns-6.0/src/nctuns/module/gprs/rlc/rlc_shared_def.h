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

#ifndef __RLC_SHARED_DEF__
#define __RLC_SHARED_DEF__

#define RLCMAC_DATA_BLOCK_SIZE  57 /* 114 * 4 = 456 --> 456/8=57 */

#define	GPRS_SNS                128
#define GPRS_WS                 64

#define	ACKED                   1
#define PENDING_ACK             2
#define NACK                    3

#define INVALID                 0
#define RECEIVED                1
#define RECEIVED_AND_RECYCLED   2

#define UNASSIGNED              255

#define CS1_DATASIZE            20
#define CS2_DATASIZE            30 /* data 247 bit */
#define CS3_DATASIZE            36
#define CS4_DATASIZE            50

/* data block */
#define CS1_BLKSIZE             23 /* header requires 3 bytes */
#define CS2_BLKSIZE             33 /* header requires 3 bytes */
#define CS3_BLKSIZE             39 /* header requires 3 bytes */
#define CS4_BLKSIZE             53 /* header requires 3 bytes */

/* control block: always encoded by CS-1 */
#define DOWNLINK_CTLBLK_SIZE    23 /* header requires 3 bytes */
#define UPLINK_CTLBLK_SIZE      21 /* header requires 1 bytes */

#define IDLE_STATE              0
#define TRANSFER_STATE          1

#define DATA_BLK                1
#define CTL_BLK                 2


/* RLC option: cmd definition */
#define USER_DATA                   0
#define UPLINK_TBF_ESTABLISH        1
#define DOWNLINK_TBF_ESTABLISH      2
#define UPLINK_TBF_RELEASE          3
#define DOWNLINK_TBF_RELEASE        4
#define UPLINK_TBF_ESTABLISHED      5
#define DOWNLINK_TBF_ESTABLISHED    6
#define SEND_PAGING_REQUEST         7
#define PAGING_RESPONSE             8
#define PUSH_A_RLCDATA_BLK          9
#define RRBP_REQUEST                10
#define RRBP_INDICATION             11
#define LLGMM_DETACH_SENT_COMPLETE  12
#define GMM_UPDATE                  13
#define GMM_ROAMING_REQUEST         14
#define ROAMING_COMPLETED           15

#endif
