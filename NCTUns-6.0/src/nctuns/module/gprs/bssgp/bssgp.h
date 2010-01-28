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

#ifndef BSSGP_H
#define BSSGP_H

#define D_BVC_BUCKET_SIZE 20000
#define D_BUCKET_LEAK_RATE 100
#define D_BMAX_DEFAULT_MS 20000
#define D_R_DEFAULT_MS 50

/* IEI */
#define IEI_ALIGNMENT_OCTECTS		0x00
#define IEI_BMAX_DEFAULT_MS		0x01
#define IEI_BSS_AREA_INDICATION		0x02
#define IEI_BUCKET_LEAK_RATE		0x03
#define IEI_BVCI			0x04
#define IEI_BVC_BUCKET_SIZE		0x05
#define IEI_BVC_MEASUREMENT		0x06
#define IEI_CAUSE			0x07
#define IEI_PDU_CELLID			0x08
#define IEI_CHANNEL_NEEDED		0x09
#define IEI_DRX_PARAMETERS		0x0a
#define IEI_EMLPP_PRIORITY		0x0b
#define IEI_FLUSH_ACTION		0x0c
#define IEI_IMSI			0x0d
#define IEI_LLC_PDU			0x0e
#define IEI_LLC_FRAMES_DISCARDED	0x0f
#define IEI_LOCATION_AREA		0x10
#define IEI_MOBILE_ID			0x11
#define IEI_MS_BUCKET_SIZE		0x12
#define IEI_MS_RADIO_ACCESS_CAPCABILITY	0x13
#define IEI_OMC_ID			0x14
#define IEI_PDU_IN_ERROR		0x15
#define IEI_PDU_LIFETIME		0x16
#define IEI_PRIORITY			0x17
#define IEI_QOS_PROFILE			0x18
#define IEI_RADIO_CAUSE			0x19
#define IEI_RA_CAP_UPD_CAUSE		0x1a
#define IEI_ROUTING_AREA		0x1b
#define IEI_R_DEFAULT_MS		0x1c
#define IEI_SUSPEND_REFERENCE_NUMBER	0x1d
#define IEI_TAG				0x1e
#define IEI_TLLI			0x1f
#define IEI_TMSI			0x20


/* PDU Type */
#define DL_UNITDATA			0x01
#define UL_UNITDATA			0x02
#define RA_CAPABILITY 			0x03
#define PTM_UNITDATA			0x04

#define BVC_BLOCK			0x20
#define BVC_BLOCK_ACK			0x21
#define BVC_RESET			0x22
#define BVC_RESET_ACK			0x23
#define BVC_UNBLOCK			0x24
#define BVC_UNBLOCK_ACK			0x25
#define FLOW_CONTROL_BVC		0x26
#define FLOW_CONTROL_BVC_ACK		0x27
#define FLOW_CONTROL_MS			0x28
#define FLOW_CONTROL_MS_ACK		0x29


#endif
