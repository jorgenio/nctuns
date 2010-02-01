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

#include "timer_mngr.h"

using namespace mobileRelayTimer;

Timer_mngr::Timer_mngr()
{
	for (int i = 0; i < NF_TIMER; i++)
	{
		timer_t[i] = NULL;
		interval_t[i] = 0;
	}

	timer_t[1] = new timerObj();        // wait for DCD
	SEC_TO_TICK(interval_t[1], 50);     // 50 sec

	timer_t[2] = new timerObj();        // wait for broadcast ranging
	SEC_TO_TICK(interval_t[2], 10);     // 10 sec

	timer_t[3] = new timerObj();        // RNG-RSP reception
	MILLI_TO_TICK(interval_t[3], 50);   // 50 ms

	timer_t[4] = new timerObj();        // wait for data grant
	SEC_TO_TICK(interval_t[4], 35);     // 35 sec

	timer_t[6] = new timerObj();        // wait for registration response
	SEC_TO_TICK(interval_t[6], 1);      // 1 sec

	timer_t[7] = new timerObj();        // wait for DSA/DSC/DSD-RSP timeout
	SEC_TO_TICK(interval_t[7], 1);      // 1 sec

	timer_t[8] = new timerObj();        // wait for DSA/DSC-ACK timeout
	MILLI_TO_TICK(interval_t[8], 300);  // 300 ms

	timer_t[9] = new timerObj();        // Registration timeout
	MILLI_TO_TICK(interval_t[9], 300);  // 300 ms

	timer_t[10] = new timerObj();       // wait for transaction end timeout
	SEC_TO_TICK(interval_t[10], 1);     // 1 sec

	timer_t[12] = new timerObj();       // wait for UCD
	SEC_TO_TICK(interval_t[12], 50);    // 50 sec

	timer_t[13] = new timerObj();
	SEC_TO_TICK(interval_t[13], 900);    // 15 min

	timer_t[14] = new timerObj();

	timer_t[15] = new timerObj();
	MILLI_TO_TICK(interval_t[15], 20);

	timer_t[17] = new timerObj();
	SEC_TO_TICK(interval_t[17], 300);    // 5 min

	timer_t[18] = new timerObj();
	MILLI_TO_TICK(interval_t[18], 50);

	timer_t[20] = new timerObj();

	timer_t[21] = new timerObj();       // MS searchs for decodable DL-MAP
	SEC_TO_TICK(interval_t[21], 11);    // 11 sec (16e.)

	timer_t[22] = new timerObj();
	SEC_TO_TICK(interval_t[22], 0.5);

	timer_t[42] = new timerObj();
	SEC_TO_TICK(interval_t[42], 0.5);

	timer_t[58] = new timerObj();		//Time the MR-BS waits for MR_Generic-ACK from RS
        MILLI_TO_TICK(interval_t[58],15*5 );	/*7+4*(maximum hop count number of the MR-system) (frames)
						 *In tranparent mode , maximum hop count number = 2 , frame duration = 5ms
						*/

	timer_t[59] = new timerObj();           //Time the MR-BS waits for DSA-RSp from RS
        MILLI_TO_TICK(interval_t[59],10);       // << T7(1sec)  , Assume it be 10 ms

	timer_t[60] = new timerObj();		/*Wait for MR_RNG-REQ message from the subordinate RS triggering by receiving	 					      *CDMA ranging code , default(6 frames)
						*/
	MILLI_TO_TICK(interval_t[60],6*5);

	timer_t[61] = new timerObj();		//Wait for ACK from RS for DCD/UCD messages
	MILLI_TO_TICK(interval_t[61],300); 	//maximum 300 ms

	timer_t[64] = new timerObj();		//Wait for RS_NBR_MEASREP after sending REG-RSP to RS
	MILLI_TO_TICK(interval_t[64],300);	//300ms
	
	timer_t[66] = new timerObj();		//Wait for RNG-RSP from MR-BS after relaying the received RNG-REQ from MS
	MILLI_TO_TICK(interval_t[66],35);	//default value of T3 - RS processing time

	timer_t[67] = new timerObj();		//Wait for RS_config-CMD after receiving REG-RSP from MR-BS
	MILLI_TO_TICK(interval_t[67],300);	//300ms

	timer_t[68] = new timerObj();		//Wait for ACK after sending RS_Config-CMD to RS
	MILLI_TO_TICK(interval_t[68],100);	//100ms

	timer_t[70] = new timerObj();		//Wait for RNG-REQ message after receining ACK for RS_AccessRSREQ
	MILLI_TO_TICK(interval_t[70],300);

						
}

Timer_mngr::~Timer_mngr()
{
	for (int i = 0; i < NF_TIMER; i++)
	{
		if (timer_t[i])
			delete timer_t[i];
	}
}

void Timer_mngr::set_func_t(unsigned int timer_no, NslObject* obj, int (NslObject::*func)(Event_*))
{
	assert(timer_t[timer_no]);
	timer_t[timer_no]->setCallOutObj(obj, func);
}

void Timer_mngr::set_interval_t(unsigned int timer_no, uint64_t interval)
{
	interval_t[timer_no] = interval;
}

void Timer_mngr::reset_t(unsigned int timer_no)
{
	timer_t[timer_no]->start(interval_t[timer_no], 0);
}

void Timer_mngr::cancel_t(unsigned int timer_no)
{
	timer_t[timer_no]->cancel();
}
