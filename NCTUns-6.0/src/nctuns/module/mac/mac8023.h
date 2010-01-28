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

#ifndef __NCTUNS_mac8023_h__
#define __NCTUNS_mac8023_h__

#include <object.h>
#include <mac/mac.h>
#include <event.h>
#include <timer.h>
#include <sys/types.h>

#define min(x, y)       ((x) < (y) ? (x) : (y))

#define IEEE_8023_SLOT          512     	// 512 bit times
#define IEEE_8023_IFS           9.6   		// 9.6us interframe spacing
#define IEEE_8023_ALIMIT        16              // attempt limit
#define IEEE_8023_BLIMIT        10              // backoff limit
#define IEEE_8023_JAMSIZE       32              // bits
#define IEEE_8023_MAXFRAME      1518            // bytes
#define IEEE_8023_MINFRAME      64              // bytes

// hwchu: 2005/06/13
#define	IEEE_8023_FCS_LEN	4		// bytes

class mac8023 : public mac {

 private:

 	MacState		macState;	  //half duplex used 
	bool			busy;
	int			retxCnt;
	ePacket_		*pktRetx, *pktRx, *pktJam;
	timerObj		txTimer, rxTimer, retxTimer, IFSTimer, jamTimer;
	u_int64_t		IFSStartTick;
	
 public:
 	mac8023(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	~mac8023();   

	int 			init(); 
	int			recv(ePacket_ *pkt);
	void			recvComplete(ePacket_ *pkt);
	void			resume();
	int 			send(ePacket_ *pkt);
	int			_send(ePacket_ *pkt);
	int			__send(ePacket_ *pkt);
	void			collision(ePacket_ *pkt);
	int			log();

	void			txScheduler(ePacket_ *pkt);
	void			txHandler();
	void			rxScheduler(ePacket_ *pkt);
	void			rxHandler();
	void			retxReset();
	void			retxScheduler();
	void			retxHandler();
	void			IFSScheduler(double IFS_);
	void			IFSHandler();
	void			jamScheduler(int bits);
	void			jamHandler();
	
	void			set(MacState setState)
					{ macState = setState; }
	MacState		state() { return macState; }
	bool			state(MacState state)
					{ return (macState == state); }
	void			bufRxPkt(ePacket_ *pkt) { pktRx = pkt; }
	void			bufRetxPkt(ePacket_ *pkt) { pktRetx = pkt; }
}; 
  

#endif	/* __NCTUNS_mac8023_h__ */
