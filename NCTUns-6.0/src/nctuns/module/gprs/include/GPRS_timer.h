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

/* This file defines a variety of timers used in RLC/MAC layer. 
 * It conforms to chap. 13 in spec. 3GPP TS 44.060.
 *
 * CreateDate: 	 14/07/2003
 * ModifiedDate: 14/07/2003
 * Author: 	 Chih-che Lin
 * Email:  	 jclin@csie.nctu.edu.tw
 */

#include <timer.h>

class GprsTimer {
    protected: 
    ulong duration;
    void* callout_function;
    public:
    short set_duration(ulong value) {duration=value;return1;};
    short set_callout_function( void* procedure_p);    
}; 
 
class T3158 : public GprsTimer {
    public:
    T3158( ulong duration, void* procedure_p);
};

T3158::T3158(ulong duration , void* procedure_p) {
     set_duration(duration);
     set_calout_function(procedure_p);
     timerObject t3158;
     uint64_t mytick; 
     t3158.init();     
     t3158.setCallOutFunc();
     MILLI_TO_TICK(mytick,duration);
     t3158.start(mytick,0);
}

class T3162 : public GprsTimer {
    public:
    T3162( ulong duration, void* procedure_p);
};


class T3164 : public GprsTimer {
    public:
    T3164( ulong duration, void* procedure_p);
};

class T3166 : public GprsTimer {
    public:
    T3166( ulong duration, void* procedure_p);
};

class T3168 : public GprsTimer {
    public:
    T3168( ulong duration, void* procedure_p);
};
