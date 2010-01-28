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
#include "CCK.h"
#include <sys/time.h>

CCK::CCK() {

	Rs = 11;  

	/* set random seed */
	/*
	struct timeval 	Time;
	gettimeofday(&Time,NULL);
	srandom(Time.tv_usec);
	*/
} 

CCK::~CCK() {

}


/*
 * For Rayleigh fading channels
 */
double CCK::_ProbBitError(double ENR) 
{

	double K,M,m,T;

	//	
	// M = (1/2) * 2^K
	///
	if (Rs == 11.0) {
		
		// 11Mbps mode, there are 8 bits in a symbol
		 
		K = 4; M = 8;
	}else{
		
		// 5.5Mbps mode, there are 4 bits in a symbol
		
		K = 3; M = 4;
	}

	
	T = sqrt(2 * ENR);
	BER = 0;
	double C = 1;
	double sign = 1;
	for ( m = 1; m <= M - 1; m++){
		C *= (M - m);
		C /= m;
		BER += (sign * (C / ( 1 + m + m * 8 * T)));
		sign *= -1;
	}
	
	if ( Rs == 11.0){
		BER *= 8;
		BER /= 15;
	}else{
		BER *= 4;
		BER /= 7;
	}
	
	return(BER);   
}
/*
double CCK::_ProbBitError(double ENR) 
{

	double K,M,m,T;

	//	
	// M = (1/2) * 2^K
	///
	if (Rs == 11.0) {
		
		// 11Mbps mode, there are 8 bits in a symbol
		 
		K = 4; M = 8;
	}else{
		
		// 5.5Mbps mode, there are 4 bits in a symbol
		
		K = 3; M = 4;
	}

	//	M-1
	//	__
	// Pm = \   (-1)^(n+1) * ( M - 1 ) * (1/n+1) * exp(-1 * (m/m+1) * k * ENR)
	// 	/                    m
	// 	--
	// 	n=1
	// 
	
	BER = 0;
	double C = 1;
	double sign = 1;
	for ( m = 1; m <= M - 1; m++){
		C *= (M - m);
		C /= m;
		BER += (sign * C * exp(-1 * m * K * ENR / (m + 1)));
		sign *= -1;
	}

	
	if ( Rs == 11.0){
		BER *= 8;
		BER /= 15;
	}else{
		BER *= 4;
		BER /= 7;
	}
	
	return(BER);   
}
*/
