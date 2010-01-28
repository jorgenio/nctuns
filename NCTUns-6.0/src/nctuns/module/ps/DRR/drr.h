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

#ifndef __NCTUNS_drr_h__
#define __NCTUNS_drr_h__

#include <object.h>
#include <packet.h>
#include <timer.h>

typedef struct ePacket_inQue {
	ePacket_ 		*ep;
	struct ePacket_inQue 	*Next;
}ePacket_inQue;

typedef struct drrTbl {
	u_long			s_ip;
	u_long			d_ip;
	int			s_portn;
	int			d_portn;
	int			Protocol;
	int			Counter;
	int			Priority;
	int			pkt_length;
	int			Timer;
	ePacket_inQue		*First;
	ePacket_inQue		*Tail;
	struct drrTbl		*Next;
}drrTbl;

typedef struct drrTblAll {
	int			Counter;
	drrTbl			*First;
	drrTbl			*Tail;
}drrTblAll;

class drr : public NslObject {

 private:

	int			DrrClassTotal;
	int			DrrPacketTotal;
	int			SpecialPacketTotal;
	int			DrrMaxBufSpaceForAllFlows;
	int			DrrMaxQueLenForEachFlow;
	int			CreditAddedPerRound;
	int			DrrTableAllMax;
	drrTblAll		*drrTableAll;
	drrTbl			*drrTableRR;
	drrTbl			*SpecialTbl;
	drrTbl			*SpanningTbl;
	drrTbl			*newdrrTbl;

	int			drrTableRR_var;
	char			*s_mask_string;
	char			*d_mask_string;
	char			*protocol_care;
	char			*s_portn_care;
	char			*d_portn_care;
	u_long			s_mask_long;
	u_long			d_mask_long;
	
 protected:
	int 			intrq(MBinder *);

 public:

	drr(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~drr();
        
	int			init();
	int			recv(ePacket_ *);
	int			send(ePacket_ *);
        int                     Delete_que(Event_ *event);
};


#endif /* __NCTUNS_drr_h__ */

