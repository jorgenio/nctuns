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

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "BPSK.h"
#include <sys/time.h>

BPSK::BPSK() {

	Rs = 1;  

	/* set random seed */
	/*
	struct timeval 	Time;
	gettimeofday(&Time,NULL);
	srandom(Time.tv_usec);
	*/
} 

BPSK::BPSK(double S) {

	Rs = S;  
	/* set random seed */
	/*
	struct timeval	Time;
	gettimeofday(&Time,NULL);
	srandom(Time.tv_usec);
	*/
}

BPSK::~BPSK() {

}



/*
 * For Rayleigh fading channels
 */

double BPSK::_ProbBitError(double ENR){

	// For simple wphy module
	if (Rs == 0)return BER;
	//
	//  Bit error rate of Differential binary PSK
	//  Pe = 1 / (2 ( 1 + ENR ) )
	///
	
	BER = 1 / (2 * ( 1 + ENR));
	if (BER >= 1.0) return 1.0;
	else return(BER);   
}

/*
 * For Optimum Reciever
 */
/*
double BPSK::_ProbBitError(double ENR){

	// For simple wphy module
	if (Rs == 0)return BER;
	//
	//  Bit error rate of Differential binary PSK
	// Pe = e^(-ENR) / 2
	//
	
	BER = exp(-ENR) / 2;
	if (BER >= 1.0) return 1.0;
	else return(BER);   
}
*/
