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

#ifndef __NCTUNS_moblnode_h__
#define __NCTUNS_moblnode_h__

#include <timer.h>

/* define the mobile node's active action */

#define NO_ACTION		0x00
#define ACT_CHANNEL_SCAN	0x01
#define PAS_CHANNEL_SCAN	0x02
#define ASSO			0x03
#define REASSO			0x04
#define DISASSO			0x05
#define DATA_FLOW		0x06

/* define the mobile node's active action during Ap-changing Procedure */
#define WAIT_ASSO		0x00
#define WAIT_SCAN		0x01

/* define the mobile node's status */

#define NO_STATUS		0x00
#define	AD_HOC			0x01
#define INFRASTRUCTURE		0x02

/* define the mobile node's power status */

#define POWER_UP		0x01
#define POWER_DOWN		0x02

/* define the channel scanning mode */

#define NOT_SET_SCAN		0x00
#define	ACTIVE_SCAN		0x01
#define PASSIVE_SCAN		0x02
#define STOP_SCAN		0x03

#define CHANNEL_NUM		13
#define MANAGE_BUF_LENGTH	20

struct ap_list{
	u_char			ap_id;
	u_char			ap_mac_addr[6];
	int			channel;
	
	double			RSSI;
	
	u_int64_t		beacon_timestamp;	//tick
	u_int16_t		beacon_timeval;		//ms

	struct ap_list		*pre_ap;	/* pre AP using same channel */
	struct ap_list		*next_ap;	/* next AP using same channel */
};


class NslObject;

class MoblNode : public NslObject {

 private:

	u_char			action;
	u_char			comm_status;
	u_char			power_status; 
 	u_char			scan_mode;
	u_char			original_scan; 
	u_char			checkAPconn_flag;
 	int			*using_chanl;
	u_char			mac_[6];
	int			ap_index;
	struct ap_list		*ap_table[CHANNEL_NUM+1];
 	struct ap_list		*active_ap;


	timerObj		ActiveScanTimer;
	timerObj		PassiveScanTimer;
	timerObj		RetryAssoTimer;
	timerObj		ReAssoTimer;
	timerObj		CheckApConnTimer;

	ePacket_		*DataBuf;
	int			DataBuf_len;
	ePacket_		*ManageBuf[MANAGE_BUF_LENGTH];
	int			ManageBuf_len;
	int			manage_buf_head;
	int			manage_buf_tail;

	char			*Operation;
	char			*Scan_Mode;

 public:

	MoblNode(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~MoblNode();

	int		init();
	int		recv(ePacket_ *pkt);	
	int		send(ePacket_ *pkt);
	
	void		ActiveChanlScan();
	void		PassiveChanlScan();
	void		FindAccessPoint(ePacket_ *pkt);
	void		SelectActiveAP();
	void		Association();
	void		RetryAssociation();
	void		ReAssociation();
	void		DisAssociation();
	void		ProcessBeacon(ePacket_ *pkt);
	void		ProcessBeaconDuringPassiveScan(ePacket_ *pkt);
	void		PushBufferPkt();
	void		CheckApConnectivity();
	void		ProcessRecvBroPkt(ePacket_ *pkt);

};

#endif  /* __NCTUNS_moblnode_h__ */
