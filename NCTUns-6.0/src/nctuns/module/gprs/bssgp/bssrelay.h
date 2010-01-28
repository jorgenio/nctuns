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

#ifndef __NCTUNS_BSSRELAY_h__
#define __NCTUNS_BSSRELAY_h__

#include <stdio.h>
#include <object.h>
#include <event.h>


class BSSRelay : public NslObject {

 private: 
	MBinder		*MSport,*SGSNport;
	char		*MSPortName,*SGSNPortName;

 public:
 	BSSRelay(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~BSSRelay();   

	int 			init(); 
	int			recv(ePacket_ *pkt);
	
        int                     get(ePacket_ *pkt, MBinder *frm);
        int                     send(ePacket_ *pkt, MBinder *frm);
}; 
 

#endif /* __NCTUNS_BSSRELAY_h__ */
