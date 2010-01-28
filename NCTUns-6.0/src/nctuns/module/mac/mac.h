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

#ifndef __NCTUNS_mac_h__
#define __NCTUNS_mac_h__

#include <object.h>
#include <event.h>
#include <timer.h>

#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>
#include <ethernet.h>


#define DELAYRATIO		10

enum MacState {
        MAC_IDLE        = 0x0000,
        MAC_POLLING     = 0x0001,
        MAC_RECV        = 0x0010,
	MAC_IFS		= 0x0011,
	MAC_JAM		= 0x0101,
        MAC_SEND        = 0x0100,
        MAC_ACK         = 0x0800,
        MAC_COLL        = 0x1000
};

class mac : public NslObject {

 protected:
	u_char		 	mac_[9];
	double			*bw_;
	char			pktFilterOpt[5];  // on/off
	bool			txState, rxState;
	timerObj		txTimer, rxTimer;
	ePacket_		*txBuf, *rxBuf;
	u_int64_t		ssTime, rsTime;

	int			*ConnNodeID_;

	u_char			ptr_log;
        timerObj                ptrlogTimer;

	char			*promiscuous_bind;
	char			promiscuous;
	char			*mode_;
	char			*_log;

	double			logInterval;
	u_int64_t		logIntervalTick;
	timerObj		_logTimer;

	char			*LogUniIn;
	char			*UniInLogFileName;
	FILE			*UniInLogFILE;
	u_int64_t		NumUniIn;

	char			*LogUniOut;
	char			*UniOutLogFileName;
	FILE			*UniOutLogFILE;
	u_int64_t		NumUniOut;

	char			*LogUniInOut;
	char			*UniInOutLogFileName;
	FILE			*UniInOutLogFILE;
	u_int64_t		NumUniInOut;

	char			*LogBroIn;
	char			*BroInLogFileName;
	FILE			*BroInLogFILE;
	u_int64_t		NumBroIn;

	char			*LogBroOut;
	char			*BroOutLogFileName;
	FILE			*BroOutLogFILE;
	u_int64_t		NumBroOut;

	char			*LogBroInOut;
	char			*BroInOutLogFileName;
	FILE			*BroInOutLogFILE;
	u_int64_t		NumBroInOut;

	char			*LogColl;
	char			*CollLogFileName;
	FILE			*CollLogFILE;
	u_int64_t		NumColl;

	char			*LogDrop;
	char			*DropLogFileName;
	FILE			*DropLogFILE;
	u_int64_t		NumDrop;

	char			*LogInThrput;
	char			*InThrputLogFileName;
	FILE			*InThrputLogFILE;
	double			InThrput;

	char			*LogOutThrput;
	char			*OutThrputLogFileName;
	FILE			*OutThrputLogFILE;
	double			OutThrput;

	char			*LogInOutThrput;
	char			*InOutThrputLogFileName;
	FILE			*InOutThrputLogFILE;
	double			InOutThrput;

 public:
        logChkTbl               *logTbl;
	
 	mac(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	virtual ~mac();   

	virtual int 		init(); 
	virtual int		recv(ePacket_ *pkt);
	virtual int 		send(ePacket_ *pkt); 
	virtual int		Debugger(); 

	u_int64_t		scheduleTime(ePacket_ *pkt, double bw,
						MacState macState = MAC_IDLE);
	u_int64_t		scheduleTime(int bits, double bw);
	u_int64_t		scheduleTime(double microSec);
	void			macTxHandler();
	void			macRxHandler();
	bool			pktFilter(ePacket_ *pkt);

	bool			CRCerror(ePacket_ *pkt);
	int			logFlush();
	int			sslog(ePacket_ *, int = 0, u_int64_t = 0);
	int			selog(ePacket_ *, u_char = SuccessTX, u_char = 0, int = 0);
	int			rslog(ePacket_ *);
	int			relog(ePacket_ *, u_char = SuccessRX, u_char = 0);
}; 
  

#endif	/* __NCTUNS_mac_h__ */
