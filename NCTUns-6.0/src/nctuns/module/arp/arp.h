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

#ifndef __NCTUNS_arp_h__
#define __NCTUNS_arp_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <timer.h>

#define ARP_TBL_MAX_ENTRYS	65536 /* cclin: A typical setting for the maximum number of arp entries is 254;
				       * however, the ITS application may operate and be validated using an extra 
				       * large-scale MANET topology.
				       */

typedef struct mapTbl {
	u_long		ip;
	u_char		mac[6];
	u_int64_t	timestamp;
	ePacket_	*pkt;
	struct mapTbl	*next;
} mapTbl;

typedef struct arpTbl {
	mapTbl		*head, *tail;
	int		entrys;
} arpTbl;

class arp : public NslObject {

 private:
	u_char			*mac_;
 	u_long			*ip_;
	arpTbl			*arpTable;
	timerObj		flushTimer;

	char			*ARP_MODE;//know_in_advance or run_arp
	int			flushInterval;	// flush interval(unit:ms)
	u_int64_t		flushInterval_; // flush interval(unit:tick)
	char			*fileName;

 public:
 	arp(u_int32_t type, u_int32_t id, struct plist * pl, const char *name); 
 	~arp();   

	int 			init(); 
	int                     recv(ePacket_ *pkt);
	int                     send(ePacket_ *pkt);
	int 			command(int argc, const char *argv[]); 
	
	inline u_long		getDstIp(ePacket_ *pkt);
	u_char			*findArpTbl(u_long ipDst, int &recordExistButNoMac);
	int			addArpTbl(u_long ip, u_char *mac, ePacket_ *pkt, arpTbl *arpTable);
	int			delArpTbl(mapTbl *lastEntry, mapTbl *delEntry);
        int                     flushArpTbl();
	int			atchMacHdr(ePacket_ *pkt, u_char *macDst, u_short frameType);
	int			updatePktBuf(u_long ipDst, ePacket_ *pkt);
	int			arpRequest(u_long ipDst);
	int			pktIsArp(ePacket_ *pkt);
	int			freeArpUpdate(ePacket_ *pkt);
	int			iAmTpa(ePacket_ *pkt);
	int			freeArpLearning(ePacket_ *pkt);
	u_short			getArpOp(ePacket_ *pkt);
	int			arpReply(ePacket_ *pkt);
	int			resumeSend(ePacket_ *pkt);
	
	int			dumpArpTable();
	int			parseLine(char *line, char *ip, char *mac);
	int			StrToIP(char *str, u_long &ip);
}; 
 

#endif /* __NCTUNS_arp_h__ */
