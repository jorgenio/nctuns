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

#ifndef __NCTUNS_SgsnBSSGP_h__
#define __NCTUNS_SgsnBSSGP_h__

#include "leakbucket.h"
#include "bssgp.h"
#include <gprs/include/GprsObject.h>

struct CListEntry {
	unsigned int id;
	LeakBucket* bucket;
	CListEntry *next;
};

class CList{
	CListEntry* root;
	NslObject* obj;
	int (NslObject::*memfunc)(Event_ *);
	int DefaultBmax;
	double DefaultRate;
	public:
	
	CList(NslObject* o, int (NslObject::*mf)(Event_ *));
	~CList();
	void setDefaultBmax(int);
	void setDefaultRate(double);
	LeakBucket* Bucket(unsigned int id);
	int remove(unsigned int id);
};

class SgsnBSSGP : public GprsObject {

 protected:

	 CList* MSCList;
	 CList* BVCList;
	 LeakBucket* BVCBucket;
	 int MaxVC;
 public:
	
 	SgsnBSSGP(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);

 	virtual ~SgsnBSSGP();   

	virtual int		recv(ePacket_ *pkt);
	virtual int 		send(ePacket_ *pkt); 
	virtual int 		directSend(ePacket_ *pkt, bool quepacket = true); 
	int 		        leak(Event* pkt); 

}; 
  

#endif	/* __NCTUNS_SgsnBSSGP_h__ */
