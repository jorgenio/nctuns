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

#include <event.h>
#include <packet.h>
#include <nctuns_api.h>
#include <string.h>
#include <sys/types.h>

#include "logchktbl.h"

chkEntry *
logChkTbl::
insertRecord(ePacket_ *pkt)
{
	Packet		*pkt_ = (Packet *)pkt->DataInfo_;
	u_int64_t	pktid = pkt_->pkt_getpid();

        for ( int j = 0; j < (int)size; ++tbptr %= size, ++j ) {
		if ( chkTbl[tbptr].chkFlag == 0 ) {
			chkTbl[tbptr].chkFlag = 1;
			chkTbl[tbptr].id = pktid;
			chkTbl[tbptr].rtxcnt = 0;
			chkTbl[tbptr].stime = GetCurrentTime();
			u_int _tbptr = tbptr;
			++tbptr %= size;
			return &chkTbl[_tbptr];
		} 
	}

	chkEntry	*oldChkTbl = chkTbl;
	chkTbl = new chkEntry[size * 2];
	//for ( int i = 0; i < size; ++i )
	memcpy(&chkTbl[0], &oldChkTbl[0], sizeof(chkEntry) * size);

	delete [] oldChkTbl;
	tbptr = size;
	size *= 2;

	chkTbl[tbptr].chkFlag = 1;
	chkTbl[tbptr].id = pktid;
	u_int _tbptr = tbptr;
	++tbptr %= size;
	return &chkTbl[_tbptr];
}

chkEntry *
logChkTbl::
endingRecord(ePacket_ *pkt, u_char etype, u_char dreason, u_int rtxcnt)
{
        Packet		*pkt_ = (Packet *)pkt->DataInfo_;
        u_int64_t       pktid = pkt_->pkt_getpid();

	tbptr += size;
	tbptr -= 50;
	tbptr %= size;

        for ( int j = 0; j < (int)size; ++tbptr %= size, ++j ) {
                if ( chkTbl[tbptr].id == pktid && chkTbl[tbptr].chkFlag == 1 ) {
                        chkTbl[tbptr].chkFlag = 2;
                        chkTbl[tbptr].id = pktid;
                        chkTbl[tbptr].rtxcnt = rtxcnt;
                        chkTbl[tbptr].etime = GetCurrentTime();
			chkTbl[tbptr].etype = etype;
			chkTbl[tbptr].dreason = dreason;
			u_int _tbptr = tbptr;
                        ++tbptr %= size;
			return &chkTbl[_tbptr];
                }
        }

	return 0;
}

void
logChkTbl::
flush()
{
	u_int64_t now = GetCurrentTime();

        for ( int i = 0; i < (int)size; ++i ) {
                if ( chkTbl[i].chkFlag != 0 ) {
			if ( now - chkTbl[i].stime > rcdlife ) {
				chkTbl[i].chkFlag = 0;
			}
                }
        }
}
