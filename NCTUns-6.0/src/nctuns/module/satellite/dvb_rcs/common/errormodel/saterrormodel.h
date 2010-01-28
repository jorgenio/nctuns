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

#include "attenuation.h"

#ifndef	__NCTUNS_modulation_h__
#define __NCTUNS_modulation_h__

/*
 * uplink or downlink
 */
enum link_type {
	UPLINK		= 1,
	DNLINK		= 2
};

/*
 * link budget
 */
struct link_budget {
	double		freq;
	double		es_dia;
	double		es_e;
	double		st_dia;
	double		st_e;
	double		tx_pwr;
	double		rx_bw;
	double		rx_snt;
	double		rain_fade;
	char		*rainfade_option;
};

class SatErrorModel {
 public :
 	double		BER;

	SatErrorModel() {};

	double ber(enum link_type link, struct link_budget &bgt);

	double ber(link_type link, struct link_budget &bgt, struct link_info &info);

	double power_to_ratio(double pwr);

 private :
 	double free_space_path_loss(double dist, double freq);


	double ratio_to_power(double ratio);

	double gain(double freq, double dia, double e);

	double lnkb(double EIRP, double Lp, double gont, double bw);
	double up_carrier_to_noise_ratio(double freq,
					double es_dia,
					double es_e,
					double st_dia,
					double st_e,
					double tx_pwr,
					double rx_bw, 
					double rx_snt);

	double dn_carrier_to_noise_ratio(double freq,
					double es_dia,
					double es_e,
					double st_dia,
					double st_e,
					double tx_pwr,
					double rx_bw,
					double rx_snt);
	
	double Q(double z);

};
 


#endif	/* __NCTUNS_modulation_h__ */
