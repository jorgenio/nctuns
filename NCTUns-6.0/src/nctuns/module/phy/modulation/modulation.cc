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

#include <math.h>
#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include "modulation.h"
#include "BPSK.h"
#include "CCK.h"

#ifdef LINUX
#define MAXFLOAT 3.40282347e+38F
#endif

double Modulation::ProbBitError(double enr)
{
	
	BER = _ProbBitError(pow(10,enr/10));
	if (isnan(BER) || isinf(BER)){
		BER = MAXFLOAT;
	}

	/* ensures that the range of BER is between 0 and 1 */
	if (BER > 1.0){
		BER = 1.0;
	}
	else if (BER < 0.0){
		BER = 0.0;
	}
	else ;

	return BER;
}

double Modulation::ProbBitError(double enr ,int n)
{
	ProbBitError(enr);
	return n * BER;
}

int Modulation::BitError(double Pr) {

	double		Pe;		// Probability of error	
	double		x;
	double 		enr;
	int		nbit = 0;	// number of bit errors tolerated


	enr = PowerToENR(Pr);
	if (nbit == 0) {
		Pe = ProbBitError(enr);
	}
	else {
		Pe = ProbBitError(enr);
	}

	if (Pe == 0.0)
		return(0);	// no bit error
	else if (Pe >= 1.0)
		return (1);
	else {
		// scale the error probability
		Pe *= 1e9;

		x = random() % (1000000000);
  		if (x < Pe)
			return(1);	// bit error
		else 
			return(0);
	}
}

int Modulation::BitError(double Pr,int n) {
	double enr,Pe,x;

	enr = PowerToENR(Pr);
	Pe = ProbBitError(enr,n);

	if (Pe == 0.0)
		return(0);	// no bit error
	else if (Pe >= 1.0)
		return (1);
	else {
		// scale the error probability
		Pe *= 1e9;

		x = random() % (1000000000);
  		if (x < Pe)
			return(1);	// bit error
		else 
			return(0);
	}
}


inline double Modulation::PowerToENR(double Pr)
{
	// Assume temperature T = 300K
	return Pr - 10 * log10(Rs * 1000000) + 203.2 - Noise;
}

#define VERY_SMALL 1e-10
Modulation* Modulation::ModForBandwidth(double bw)
{
	Modulation* mod;

//	if ( bw == 11 || bw == 5.5)
	if( fabs(bw-11)<VERY_SMALL || fabs(bw-5.5)<VERY_SMALL)
	{
		mod = new CCK;
	}else{
		mod = new BPSK;
	}

	mod->Rs = bw;
	return mod;
}
