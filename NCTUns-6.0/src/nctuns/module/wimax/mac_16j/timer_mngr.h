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

#ifndef __NCTUNS_80216J_TIMER_MNGR_H__
#define __NCTUNS_80216J_TIMER_MNGR_H__

#include <stdint.h>
#include <timer.h>
#include <assert.h>

#define NF_TIMER 73

namespace mobileRelayTimer {
	class Timer_mngr {

		private:
			timerObj*   timer_t[NF_TIMER];    // Timer Object of Ti
			uint64_t    interval_t[NF_TIMER]; // Current Value of Ti in tick.

		public:
			Timer_mngr();
			~Timer_mngr();

			void set_func_t(unsigned int, NslObject*, int (NslObject::*)(Event_*));
			void set_interval_t(unsigned int, uint64_t);
			void reset_t(unsigned int);
			void cancel_t(unsigned int);
	};
}


#endif /* __NCTUNS_80216J_TIMER_MNGR_H__ */
