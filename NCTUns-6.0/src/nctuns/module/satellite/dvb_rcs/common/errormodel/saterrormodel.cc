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
#include <stdlib.h>
#include <math.h>
#include "saterrormodel.h"
#define		PAI	3.14159265
#define		R	36000		//sat to surface dist



/*
 * calc free space path loss
 */
double 
SatErrorModel::free_space_path_loss(double dist, double freq){
	return -1*(92.45 + 20*log10(dist) + 20*log10(freq) );
}


/* 
 * power to ratio function
 */

double 
SatErrorModel::power_to_ratio(double pwr) {
	return 10*log10(pwr);
}


/* 
 * ratio to power function 
 */
double 
SatErrorModel::ratio_to_power(double ratio) {
	return pow(10, ratio/10);
}


/*
 * calc gain
 *
 * Parameter:
 * 1. freq	frequency
 * 2. dia	antenna diameter
 * 3. e		efficiency
 */
double 
SatErrorModel::gain(double freq, double dia, double e) {
	return 10.0*log( 4*PAI*e*PAI*dia*dia*freq*freq/(4*.3*.3))/log(10.0);
}


double 
SatErrorModel::lnkb(double EIRP, double Lp, double gont, double bw) {
	return (EIRP - Lp - power_to_ratio(bw) + 228.6 + gont);
}


/* 
 * Calc uplink carrier to noise ratio
 *
 * Paramter:
 *
 * 1. freq		downlink freqency(GHz)
 * 2. es_dia		earth station antenna diameter(m)
 * 3. es_e		earth station efficiency => 50% ~ 75%, horn antennas can reach 90%
 * 4. st_dia		satellite antenna diameter(m)
 * 5. st_e		satellite antenna efficiency
 * 6. rx_pwr		transponder satureated output power(W)
 * 7. rx_bw		receiver IF bandwidth(MHz)
 * 8. rx_snt		receiveing system noise temperature(K)
 */
double 
SatErrorModel::up_carrier_to_noise_ratio(	double freq,
					double es_dia,
					double es_e,
					double st_dia,
					double st_e,
					double tx_pwr,
					double rx_bw, 
					double rx_snt)
{
	double	Pt;			// Satellite transponder output power, ok
	double	Bo;			// Transponder output backoff
	double	Gt;			// Satellite antenna gain, on axis, ok
	double	Gr;			// Earth station antenna gain, ok
	double	Lp;			// Path loss, ok
	double	Lant;			// Edge of beam loss at freq GHz
	double	La;			// Clear air atmospheric loss
	double	Pr;			// Received power, ok

	double	k;			// fixed Boltzmann's constant
	double	Ts;			// systme noise temperature
	double	Bn;			// noise bandwidth
	double	Pn;			// receiver noise power 
	
	double	CN;			// carrier to noise ratio in receiver in clear air
	
	/* p.114 uplink power budget */
	Pt = power_to_ratio(tx_pwr);
	Bo = -2;
	Gt = gain(freq, es_dia, es_e);
	Gr = gain(freq, 1.128379, 1.0);
	Gr = gain(freq, st_dia, st_e);

	Lp = free_space_path_loss(R, freq);
	Lant = -3;
	La = -0.73;
	Pr = Pt + Bo + Gt + Gr +(Lp + La);

	/* uplink noise power budget in clear air */
	k = -228.6;
	Ts = power_to_ratio(rx_snt);
	Bn = power_to_ratio(rx_bw*1000000);
	Pn = k + Ts + Bn;
	
	CN = Pr - Pn;
	return CN;

}


/* 
 * Calc downlink carrier to noise ratio
 *
 *  required paramter:
 *
 * 1. freq		downlink freqency(GHz)
 * 2. es_dia		earth station antenna diameter(m)
 * 3. es_e		earth station efficiency => 50% ~ 75%, horn antennas can reach 90%
 * 4. st_dia		satellite antenna diameter(m)
 * 5. st_e		satellite antenna efficiency
 * 6. tx_pwr		transponder satureated output power(W)
 * #7. EIRP		efficient isotropically radiated power(dbW)
 * 8. rx_bw		receiver IF bandwidth(MHz)
 * 9. rx_snt		receiveing system noise temperature(K)
 */
double 
SatErrorModel::dn_carrier_to_noise_ratio(	double freq,
					double es_dia,
					double es_e,
					double st_dia,
					double st_e,
					double tx_pwr,
					double rx_bw,
					double rx_snt)
{
	double	Bo;			// Transponder output backoff
	double	Gr;			// Earth station antenna gain, ok
	double	Lp;			// Path loss, ok
	double	Lant;			// Edge of beam loss at freq GHz
	double	La;			// Clear air atmospheric loss
	double	Lm;			// Other losses
	double	gont;

	
	double	CN;			// carrier to noise ratio in receiver in clear air

	double	EIRP;
	
	/* p.114 downlink power budget */
	Gr = gain(freq, es_dia, es_e);
	gont = Gr - power_to_ratio(rx_snt);
	Lp = -1*free_space_path_loss(R, freq);
	Lant = -3;
	La = -0.73;			// f > 13, La=0.73
	Lm = -0.8;
	Bo = -3;

	/* downlink noise power budget in clear air */

	EIRP = power_to_ratio(tx_pwr) + gain(freq, st_dia, st_e);
	CN = lnkb(EIRP, Lp, gont, rx_bw*1000000) + Lant + La + Lm + Bo;

	return CN;
}


/* 
 * Q function.
 * The following formula has a little bit different with in book.
 * adjust by multiply 19/20.
 */
double 
SatErrorModel::Q(double z){
	return (1.0f/(z * powf(2.0f*M_PI, 0.5f))) * (expf(-(z*z/2.0f)))*(19.0/20.0);
}


/* 
 * calc BER
 */
double 
SatErrorModel::ber(enum link_type link, struct link_budget &bgt) {

	double	ideal_cn_ratio;
	double	rx_cn_ratio;
	double	rx_pwr_ratio;

	double freq = bgt.freq;
	double es_dia = bgt.es_dia;
	double es_e = bgt.es_e;
	double st_dia = bgt.st_dia;
	double st_e = bgt.st_e;
	double tx_pwr = bgt.tx_pwr;
	double rx_bw = bgt.rx_bw;
	double rx_snt = bgt.rx_snt;

	switch(link){
		case UPLINK:
			ideal_cn_ratio = up_carrier_to_noise_ratio(freq, es_dia, es_e, st_dia, st_e, tx_pwr, rx_bw, rx_snt);
			break;
		case DNLINK:
			ideal_cn_ratio = dn_carrier_to_noise_ratio(freq, es_dia, es_e, st_dia, st_e, tx_pwr, rx_bw, rx_snt);
			break;
	}
	rx_cn_ratio = ideal_cn_ratio - bgt.rain_fade;
	rx_pwr_ratio = ratio_to_power(rx_cn_ratio);

	BER = Q(pow(rx_pwr_ratio,0.5));

	return BER;

}

double 
SatErrorModel::ber(enum link_type link, struct link_budget &bgt, struct link_info &info) {

	double	ideal_cn_ratio;
	double	rain_atten;
	double	rx_cn_ratio;
	double	rx_pwr_ratio;

	double freq = bgt.freq;
	double es_dia = bgt.es_dia;
	double es_e = bgt.es_e;
	double st_dia = bgt.st_dia;
	double st_e = bgt.st_e;
	double tx_pwr = bgt.tx_pwr;
	double rx_bw = bgt.rx_bw;
	double rx_snt = bgt.rx_snt;

	double freqa = info.freq;
	double ele_angle = info.ele_angle;
	double taur = info.taur;
	double Hrp = info.Hrp;
	double Hs = info.Hs;
	int latitude = info.latitude;
	double rainfall_rate = info.rainfall_rate;

	attenuation fade;
	rain_atten = fade.rain_attenuation(freqa, ele_angle, taur, Hrp, Hs, latitude, rainfall_rate);

	switch(link){
		case UPLINK:
			ideal_cn_ratio = up_carrier_to_noise_ratio(freq, es_dia, es_e, st_dia, st_e, tx_pwr, rx_bw, rx_snt);
			break;
		case DNLINK:
			ideal_cn_ratio = dn_carrier_to_noise_ratio(freq, es_dia, es_e, st_dia, st_e, tx_pwr, rx_bw, rx_snt);
			break;
	}

	rx_cn_ratio = ideal_cn_ratio - rain_atten;
	rx_pwr_ratio = ratio_to_power(rx_cn_ratio);

	BER = Q(pow(rx_pwr_ratio,0.5));

	return BER;

}
