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

#ifndef	__NCTUNS_modulation_h__
#define __NCTUNS_modulation_h__


/*==========================================================================
   Porting from cmu extension to ns
   Modulation Schemes
   
   	Using the receive power and information about the modulation
   	scheme, amount of forward error correction. etc., this class
   	computes whether or not a packet was received correctly or
   	with few enough errors that they can be tolerated.
 ===========================================================================*/                                                                                                                                                       


class Modulation {
 public :
	double			BER;		// Bit Error Rate
   	double                  Rs;     	//data rate per second
	double			Noise;		// Additional Noise
   	
	Modulation() {};
	virtual ~Modulation() {}; 
	
	// calculate Energy ber bit / noise power density
	double			PowerToENR(double);
	// success reception ?
	int			BitError(double);  
	int			BitError(double,int);  
 	// Probability of 1 bit error
 	double		ProbBitError(double);
 	double		ProbBitError(double, int);     
 	virtual double		_ProbBitError(double) = 0;
 	// Probability of n bit errors

	// Every Bandwidth can map to a modulation,and our ModForBandwidth() can do it;
	static Modulation*	ModForBandwidth(double bw);
};
 


#endif	/* __NCTUNS_modulation_h__ */
