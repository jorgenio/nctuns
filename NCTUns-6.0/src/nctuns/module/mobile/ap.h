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

#ifndef __NCTUNS_ap_h__
#define __NCTUNS_ap_h__

#include <timer.h>

/*
 *  Association table's data structure.
 */
struct AssoEntry{
	u_int32_t		nodeID;
	u_char			mac_addr[6];
	u_int64_t		asso_time;
	u_int64_t		update_time;
	u_char			state;
	u_long			ip;
	u_char			*name;
	double			consume_thrput;
	double			old_consume_thrput;
	struct AssoEntry	*pre_entry;
	struct AssoEntry	*next_entry;
};


#define	MANAGE_BUF_LENGTH		20
#define DATA_BUF_LENGTH			1

class NslObject;

class AP : public NslObject {

 private:

	int			*using_chanl;
	struct AssoEntry	*AssoTable;
	int			AssoCount;
	u_char			mac_[6];

	timerObj		GenBeaconTimer;
	timerObj		FlushAssoTableTimer;

	ePacket_		*DataBuf[DATA_BUF_LENGTH + 1];
	int			DataBuf_len;
	int			data_buf_head;
	int			data_buf_tail;
	ePacket_		*ManageBuf[MANAGE_BUF_LENGTH];
	int			ManageBuf_len;
	int			manage_buf_head;
	int			manage_buf_tail;

	int			BEACON_TIMEVAL;		//ms
	//char			*PCF;
	//char			*PowerManage;
	//char			*MobileIP;


 public:

	AP(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~AP();

	int			init();
	int			recv(ePacket_ *pkt);
	int			send(ePacket_ *pkt);
	int			command(int argc, const char *argv[]);	
	
	void			FlushAssoTable();
	void			GenerateBeacon();
	void			ProcessChanlScanReq(ePacket_ *pkt);
	void			ProcessAsso(ePacket_ *pkt);
	void			ProcessReAsso(ePacket_ *pkt);
	void			ProcessDisAsso(ePacket_ *pkt);
	void			PushBufferPkt();	
	void			DumpAssoTable();
	int			CheckAssoAddr(char *dst_addr, Packet *pkt_);
	void			ProcessRecvBroPkt(ePacket_ *pkt);

};


#endif  /* __NCTUNS_ap_h__ */
