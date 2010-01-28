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

#ifndef __NCTUNS_ophy_h__
#define __NCTUNS_ophy_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <misc/log/logHeap.h>
#include <misc/log/logchktbl.h>
#include <module/opt/Lheader.h>

class Packet;

class ophy : public NslObject {

 private:

	double			bw_;		//bit per second
	double			bw_Mbps;	//megabit per second

	double			BER;

	int			PropDelay_micro;
	u_int64_t		PropDelay_tick;
	bool			txState, rxState;
	ePacket_		*rxtmp;
	timerObj		txTimer, rxTimer;

	char			*_linkfail;
	char			*linkfailFileName;
	FILE			*linkfailFile;
	u_int32_t		LinkFailFlag;
	
	bool			is_switch;
	
	//log mechanism for packet trace
	u_int64_t		ssTime, rsTime;
	u_char			ptr_log;
        timerObj                ptrlogTimer;
        ePacket_		*txBuf, *rxBuf;
        int			ConnNodeID_;

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
 	ophy(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~ophy();
 	
	logChkTbl               *logTbl;
	int 			init(); 
	int 			recv(ePacket_ *pkt); 
	int 			send(ePacket_ *pkt); 

	int			BitError(double BER_, int plen);
	u_int64_t		scheduleTime(ePacket_ *pkt);
	
	void			TurnOnLinkFailFlag(Event_ *ep);
	void			TurnOffLinkFailFlag(Event_ *ep); 
	void			TxHandler();
	void			RxHandler();
	
	//for packet trace log
	int			logFlush();
	int			sslog(ePacket_ *, int = 0, u_int64_t = 0);
	int			selog(ePacket_ *, u_char = SuccessTX, u_char = 0, int = 0);
	int			rslog(ePacket_ *);
	int			relog(ePacket_ *, u_char = SuccessRX, u_char = 0);

	//for optional log
	int 			log();
	int                     ptype(Packet* p);
}; 
 

#endif /* __NCTUNS_ophy_h__ */
