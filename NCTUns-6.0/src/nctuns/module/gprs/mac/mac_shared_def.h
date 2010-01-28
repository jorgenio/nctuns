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

#ifndef __GPRS_MAC_SHARED_DEFINITION__
#define __GPRS_MAC_SHARED_DEFINITION__

#include <gprs/include/types.h>

/* The definition of MAC_MS states */

#define POWER_ON_NOT_SYNC       3
#define PACKET_IDLE_STATE       0
#define PACKET_TRANSFER_STATE   1
#define MAC_SHARED_STATE        2
#define ABNORMAL                100

/* Types of send queues */
#define PSI1_SQUEUE             -1
#define PBCCH_SQUEUE            -2
#define PPCH_SQUEUE             -3
#define PNCH_SQUEUE             -4
#define PAGCH_SQUEUE            -5
#define PACCH_SQUEUE            -6
#define PRACH_SQUEUE            -7
#define PDTCH_SQUEUE            100

/* The definition of MAC_BTS states */
#define NOT_READY   0
#define READY       1

/* System Parameter */
#define MAX_CHANNEL_REQ_TIMES 7

/* SCHEDULING PARAMETERS */
#define DOWNLINK_ALLOC_TS   3
#define UPLINK_ALLOC_TS     1


#define UPLINK_START_CH(start_ch)               (start_ch)
#define UPLINK_END_CH(end_ch)                   (end_ch)
#define DOWNLINK_START_CH(start_ch)             (start_ch)+125
#define DOWNLINK_END_CH(end_ch)                 (end_ch)+125
#define CORRESPONDING_DOWNLINK_CH(uplink_ch)    (uplink_ch)+125
#define CORRESPONDING_UPLINK_CH(downlink_ch)    (downlink_ch)-125


#define SHOW_TIME do {  \
                        \
    cout << "bn= " << bn << " tn= " << tn << " fn= " << fn << " blkn= " << blkn << endl; \
    cout << "uplink_tn=" << uplink_tn << endl; \
                        \
} while(0)  \

typedef class RecvBurstRecord {

    public:
    uchar   recv_tn;
    uchar   recv_bn;
    double  recv_rssi;
    long    src_nid;

    RecvBurstRecord() { recv_tn=100; recv_bn=0; recv_rssi=0; src_nid=0;}
    int clear()      { recv_tn=100; recv_bn=0; recv_rssi=0; src_nid=0;return 1;}
} RBRec ;

#endif

