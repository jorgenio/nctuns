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

#ifndef __NCTUNS_WIMAX_BURST_H__
#define __NCTUNS_WIMAX_BURST_H__

#include <vector>
#include <list>

#include "structure.h"
#include "connection.h"

class AllocInfo {
public:
	Connection * conn;
	int len;
	std::vector < int >pduInfo;
	u_int position;

	AllocInfo(Connection * c, int l, std::vector < int >p)
	{
		conn = c;
		len = l;
		pduInfo = p;
		position = 0;
	}
	inline int getNext() { return pduInfo[position]; }
	//      ~AllocInfo(){printf("~AllocInfo %p\n", this);};
};

class WiMaxBurst {

protected:
	char _fec;

	WiMaxBurst(char fec)
	{
		_fec = fec;
		payload = NULL;
		length = 0;
	}
public:
	 int length;		// in Bytes
	 char *payload;

	 std::list < AllocInfo * >ConnectionCollection;

	 virtual ~WiMaxBurst()
	 {
		 AllocInfo *a;
		 while (ConnectionCollection.size()) {
			 a = ConnectionCollection.back();
			 delete a;
			 ConnectionCollection.pop_back();
		 }
		 if (payload) {
			 delete [] payload;
		 }
	 }
	 inline int toSymbol(int byte) {
		 return bytesToSymbols(byte, _fec);
	 }
	 inline int toByte(int symbol) {
		 return symbolsToBytes(symbol, _fec);
	 }

	 static int bytesToSymbols(int bytes, int fec) {
		 static int uncodedBlockSize[] =
		 { 12, 24, 36, 48, 72, 96, 108 };
		 if (!bytes)
			 return 0;
		 int r = (bytes + 1) / uncodedBlockSize[fec];
		 if (bytes + 1 - r * uncodedBlockSize[fec] > 0)
			 r++;
		 return r;
	 }
	 static int symbolsToBytes(int symbols, int fec) {
		 static int uncodedBlockSize[] =
		 { 12, 24, 36, 48, 72, 96, 108 };

		 return (symbols * uncodedBlockSize[fec]) - 1;
	 }
};

class downBurst:public WiMaxBurst {

public:
	OFDM_DLMAP_IE ie;
	int duration;		// in symbols

	downBurst(char pDIUC, char fec)
	: WiMaxBurst(fec)
	{
		ie.cid = 0;
		ie.diuc = pDIUC;
		ie.preamble = 0;
		ie.stime = 0;
		length = 0;
		payload = NULL;
		duration = 0;
	}
	void Dump()
	{
		std::list < AllocInfo * >::iterator it;
		printf("Cid=%d, Diuc=%d, Stime=%d, lenth=%d duration=%d\n",
				ie.cid, ie.diuc, ie.stime, length, duration);
		for (it = ConnectionCollection.begin();
				it != ConnectionCollection.end(); it++) {
			printf("\tconn=%p (%d) len=%d\n", (*it)->conn,
					static_cast <
					Connection * >((*it)->conn)->getCID(),
					(*it)->len);
		}
	}
};

class upBurst:public WiMaxBurst {

public:
	struct OFDM_ULMAP_IE ie;

	upBurst(struct OFDM_ULMAP_IE pie, char fec)
	: WiMaxBurst(fec)
	{
		ie = pie;
		length = 0;
		payload = NULL;
	}
	upBurst(u_int16_t cid, char pUIUC, int pStime, int pDuration, char fec)
	:WiMaxBurst(fec)
	{
		ie.cid = cid;
		ie.uiuc = pUIUC;
		ie.stime = pStime;
		ie.duration = pDuration;
		ie.chidx = 0;
		ie.midamble = 0;
		length = 0;
		payload = NULL;
	}

	void Dump()
	{
		printf("Cid=%d, UsageCode=%d, Stime=%d, Duration=%d fec=%d\n",
				ie.cid, ie.uiuc, ie.stime, ie.duration, _fec);
	}
};


#endif				/* __NCTUNS_WIMAX_BURST_H__ */
