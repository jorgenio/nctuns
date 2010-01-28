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

#ifndef __NCTUNS_NS_h__
#define __NCTUNS_NS_h__

#include <object.h>
#include <event.h>
#include <timer.h>

#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>

#include <ethernet.h>
#include <gprs/include/GprsObject.h>


#define DELAYRATIO		10

enum NSState {
        NS_IDLE        = 0x0000,
        NS_POLLING     = 0x0001,
        NS_RECV        = 0x0010,
	NS_IFS		= 0x0011,
	NS_JAM		= 0x0101,
        NS_SEND        = 0x0100,
        NS_ACK         = 0x0800,
        NS_COLL        = 0x1000
};

class NS : public GprsObject {

 protected:
	u_char		 	NS_[9];
	double			*bw_;
	bool			txState, rxState;
	timerObj		txTimer, rxTimer;
	ePacket_		*txBuf,*rxBuf;
	u_int64_t		ssTime, rsTime;
	int			MaxVC;

	int			*ConnNodeID_;

	char			*mode_;
	int			ptr_log;
	timerObj		ptrlogTimer;

 public:
	logChkTbl		*logTbl;
	
 	NS(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	virtual ~NS();   

	virtual int 		init(); 
	virtual int		recv(ePacket_ *pkt);
	virtual int 		send(ePacket_ *pkt); 

	u_int64_t		scheduleTime(ePacket_ *pkt, double bw,
						NSState NSState = NS_IDLE);
	u_int64_t		scheduleTime(int bits, double bw);
	u_int64_t		scheduleTime(double microSec);
	void			NSTxHandler();
	void			NSRxHandler();
	int			sslog(ePacket_* );
	int			selog(ePacket_* );
	int			rslog(ePacket_* );
	int			relog(ePacket_* );
	int			logFlush();

}; 
  

#endif	/* __NCTUNS_NS_h__ */
