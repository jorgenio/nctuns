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

/*
 * The following formula capture from 
 * "Satellite Communicatios, 2nd Edition" chap 8.
 *
 * Qu1:	Hrp not sure
 * Qu2:	what the rain_atten diff between one region 
 *	have heavy or light rain
 * An2:	usually rain rate(mm/h) is estimated in average one year
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "attenuation.h"
#define DTR M_PI/180.0

/* 
 * to round off, round up or down
 * ex, 17.8 => 18, 17.1 => 17
 */
int 
attenuation::ro(double d) {
	if ( d < ((ceil(d)+floor(d))/2) ) {
		return (int)floor(d);
	} else {
		return (int)ceil(d);
	}
}


/*
 * parameter required :
 * 1. freq		frequency(GHz)
 * 2. ele_angle		elevation(antenna) angle(degree)
 * 3. taur  		polariztion angle relative to horizatal(degree)
 *			(45 deg for circlar polarization)
 * 4. Hrp		melting layer height(km)
 * 5. Hs		height above mean sea level(km)
 * 6. latitude		latitude of location
 * 7. longtitude	longtitude of location
 */
double 
attenuation::rain_attenuation(double freq, double ele_angle, double taur, double Hrp, double Hs, int latitude , double rainfall_rate)
{
	
/*	Hrp = 4;			// melting layer height(km), assume = 4
	Hs = 0.05;			// height above mean sea level(km)
	latitude = 25;			// latitude of location, assume TW
	longtitude = 120;		// longtitude of location, assume TW
*/

	int	ro_freq;		// rounded frequency
	double	Lg;	  		// horizontal projection of Ls(km)
	double	Ls;			// slant-path length(km)
	double	lo;			// calculated for adjustment factor v
	double	Lr;			// calculated for effective path len
	double	r;			// horizontal reduction factor, v(0.01)
	double	v;			// vertical adjustment factor
	double	Le;			// effective path length(km)
	double	A;			// predicted attenuation(dB)
	double	spec_atten;		// specific attenuation (dB/km)
	double	k, a;			// frequency dependent parameter
	double	kh[100], kv[100], ah[100], av[100];	
	// regression coefficient, define uncompletely

	kh[1]=0.0000259;        kv[1]=0.0000308;       ah[1]=0.9691;   av[1]=0.8592;
	kh[2]=0.0000847;        kv[2]=0.0000998;       ah[2]=1.0664;   av[2]=0.9490;
	kh[3]=0.0001390;        kv[3]=0.0001942;       ah[3]=1.2322;   av[3]=1.0688;
	kh[4]=0.0001071;        kv[4]=0.0002461;       ah[4]=1.6009;   av[4]=1.2476;
	kh[5]=0.0002162;        kv[5]=0.0002428;       ah[5]=1.6969;   av[5]=1.5317;
	kh[6]=0.0007056;        kv[6]=0.0004878;       ah[6]=1.5900;   av[6]=1.5728;
	kh[7]=0.001915;         kv[7]=0.001425;        ah[7]=1.4810;   av[7]=1.4745;
	kh[8]=0.004115;         kv[8]=0.003450;        ah[8]=1.3905;   av[8]=1.3797;
	kh[9]=0.007535;         kv[9]=0.006691;        ah[9]=1.3155;   av[9]=1.2895;
	kh[10]=0.01217;         kv[10]=0.01129;        ah[10]=1.2571;  av[10]=1.2156;
	kh[11]=0.01772;         kv[11]=0.01731;        ah[11]=1.2140;  av[11]=1.1617;
	kh[12]=0.02386;         kv[12]=0.02455;        ah[12]=1.1825;  av[12]=1.1216;
	kh[13]=0.03041;         kv[13]=0.03266;        ah[13]=1.1586;  av[13]=1.0901;
	kh[14]=0.03738;         kv[14]=0.04126;        ah[14]=1.1396;  av[14]=1.0646;
	kh[15]=0.04481;         kv[15]=0.05008;        ah[15]=1.1233;  av[15]=1.0440;
	kh[16]=0.05282;         kv[16]=0.05899;        ah[16]=1.1086;  av[16]=1.0273;
	kh[17]=0.06146;         kv[17]=0.06797;        ah[17]=1.0949;  av[17]=1.0137;
	kh[18]=0.07078;         kv[18]=0.07708;        ah[18]=1.0818;  av[18]=1.0025;
	kh[19]=0.08084;         kv[19]=0.08642;        ah[19]=1.0691;  av[19]=0.9930;
	kh[20]=0.09164;         kv[20]=0.09611;        ah[20]=1.0568;  av[20]=0.9847;
	kh[21]=0.1032;          kv[21]=0.1063;         ah[21]=1.0447;  av[21]=0.9771;
	kh[22]=0.1155;          kv[22]=0.1170;         ah[22]=1.0329;  av[22]=0.9700;
	kh[23]=0.1286;          kv[23]=0.1284;         ah[23]=1.0214;  av[23]=0.9630;
	kh[24]=0.1425;          kv[24]=0.1404;         ah[24]=1.0101;  av[24]=0.9561;
	kh[25]=0.1571;          kv[25]=0.1533;         ah[25]=0.9991;  av[25]=0.9491;
	kh[26]=0.1724;          kv[26]=0.1669;         ah[26]=0.9884;  av[26]=0.9421;
	kh[27]=0.1884;          kv[27]=0.1813;         ah[27]=0.9780;  av[27]=0.9349;
	kh[28]=0.2051;          kv[28]=0.1964;         ah[28]=0.9679;  av[28]=0.9277;
	kh[29]=0.2224;          kv[29]=0.2124;         ah[29]=0.9580;  av[29]=0.9203;
	kh[30]=0.2403;          kv[30]=0.2291;         ah[30]=0.9485;  av[30]=0.9129;

	ro_freq = ro(freq);

        // step2: calc slant-path length, Ls
        if( ele_angle < 5 ) {
                Ls = 2*(Hrp-Hs)/(pow((pow(sin(ele_angle*DTR), 2)
		     + 2*(Hrp-Hs)/8500), 0.5)+sin(ele_angle*DTR));
        }
        else {
                Ls = (Hrp-Hs)/sin(ele_angle*DTR);
        }

        // step3: calc horizontal projection, Lg
        Lg = Ls*cos(ele_angle*DTR);

	/* 
	 * step4: obtain rainfall ratk(rainfall_rate), R(0.01).
	 * according to the location(longtitude, latitude)
	 * to look up ITU-R P.387-4 finging R(0.01).
	 */

        // Step5: calc the specific attenuation, spec_atten
	k = ( kh[ro(freq)]+kv[ro(freq)]+(kh[ro(freq)]-kv[ro(freq)])*pow(cos(ele_angle*DTR), 2)
	      *cos(2*taur*DTR))/2;
	a = ( kh[ro(freq)]*ah[ro(freq)]+kv[ro(freq)]*av[ro(freq)]+(kh[ro(freq)]*ah[ro(freq)]-kv[ro(freq)]
	      *av[ro(freq)])*pow(cos(ele_angle*DTR), 2)*cos(2*taur*DTR) )/(2*k);
	spec_atten = k * (pow(rainfall_rate, a));
	
        // Step6: calc r(0.01), the horizontal reduction factor
        r = 1/(1+0.78*pow(3.95f*4.7175f/17.8f, 0.5f)-0.38*(1-exp(-2*3.95)));

        // Step7: calc v(0.01), the vertical adjustment factor
        lo = atan(3.95/(Lg*r))/(DTR);
	if( lo > ele_angle ){	
  		Lr = (Lg*r) / cos(ele_angle*DTR);
	} else {
		Lr = (Hrp-Hs) / sin(ele_angle*DTR);
	}
	
        // v has a little bit of diff
	if( abs(latitude) < 36 ) {
        v = 1.0f / (1.0f + pow(sinf(ele_angle*DTR), 0.5) 
	            * (31*(1-exp(-(ele_angle/1.0+(36-latitude))))*pow(Lr*spec_atten, 0.5)/(freq*freq)-0.45));
	} else { 
	v = 1.0f / (1.0f + pow(sinf(ele_angle*DTR), 0.5)                                                     	
	            * (31*(1-exp(-ele_angle))*pow(Lr*spec_atten, 0.5)/(freq*freq)-0.45));}
        
	// Step8: calc Le, the effective path length
        Le = Lr * v;
	
        // Step9: calc A(0.01), predicted attenuation exceeded for .01%
        A = spec_atten * Le;
	return A;

}
