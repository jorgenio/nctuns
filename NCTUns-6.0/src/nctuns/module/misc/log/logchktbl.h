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

#ifndef _NCTUNS_LOGCHKTBL_H_
#define _NCTUNS_LOGCHKTBL_H_

#include <sys/types.h>
#include <event.h>


#define DFCHKTBLSIZE    1000
#define PRODELAY        10

typedef struct chkEntry {
        u_char          chkFlag;
        u_int64_t       id;
        u_int           rtxcnt;
        u_int64_t       stime;
        u_int64_t       etime;
        u_char          etype;
	u_char		dreason;
} chkEntry;

class logChkTbl {
public:
        u_int           tbptr;
        u_int           size;
        chkEntry        *chkTbl;
        u_int64_t       rcdlife;

        logChkTbl(u_int64_t rcdlife_) :
            tbptr(0), size(DFCHKTBLSIZE), rcdlife(rcdlife_) {
                chkTbl = new chkEntry[size];

		memset(chkTbl, 0, sizeof(chkEntry) * size);
        }

        ~logChkTbl() { delete [] chkTbl; }

        chkEntry        *insertRecord(ePacket_ *);
        chkEntry        *endingRecord(ePacket_ *, u_char, u_char, u_int);
        void             flush();
};

#endif /* !_NCTUNS_LOGCHKTBL_H_ */

