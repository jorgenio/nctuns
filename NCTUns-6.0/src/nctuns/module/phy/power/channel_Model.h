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

#ifndef	__NCTUNS_CHANNEL_MODEL_h__
#define __NCTUNS_CHANNEL_MODEL_h__

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <sys/types.h>
#include <nctuns_api.h>
#include <cmInfo.h>
#include <assert.h>
#include <sock_skel.h>
#include "../modulation/modulation.h"

#define VELOCITY_OF_LIGHT          3e8
/*==========================================================================
   Porting from cmu extension to ns
   
   Propagation Channel Models :
   	
   	Using position and wireless transmission interface properties,
   	propagation models compute the power with which a given
   	packet will be received.
 ==========================================================================*/

#define FREE_SPACE	0x00
#define TWO_RAY_GROUND	0x01
#define FREE_SPACE_AND_SHADOWING 0x02
class channelModel {

 private :

 	double	last_hr;
 	double	last_ht;
 	double	crossover_dist; // (m)
	double	freq_;		// frequency (Hz)
	double	Lambda;		// wave-length (m)

	/*
	 * Configured via TCL
	 */
	double  max_velocity;       /* Maximum velocity of vehicle/objects in 
				   environment.  Used for computing doppler */

	/* Internal values */
	int N;                  /* Num points in table */
	float fm0;              /* Max doppler freq in table */
	float fm;               /* Max doppler freq in scenario */
	float fs;               /* Sampling rate */
	float dt;               /* Sampling period = 1/fs */
	
	float K;                /* Ricean K factor */

	int initialized;

 public :  

	channelModel(double f);
	~channelModel();

	/* The followings are theoretical models */
	double Pr(struct cmInfo *cminfo);
	// two-ray ground model combines with free-space model
	double	TRG_FS(double, double, double, double, double, double, double);
	// free space with shadowing model
	double	FS_Shadowing(double, double, double, double, double, double, double, double);

	double	Friis(double, double, double, double, double, double); // (mW)
	double	TwoRay(double, double, double, double, double, double); // (mW)
	// rayleigh fading model
	double	RayleighFading(double, double);
	// ricean fading model
	double	RiceanFading(double, int, int);

	/* The followings are empirical models */
	double	LeeMicrocell(double, double, double, double, double, double, double);
	double	Okumura_Hata_VHF_UHF(double, double, double, double, double, double);
	double	Okumura_Hata_large_urban(double, double, double, double, double, double);
	double	Okumura_Hata_medium_urban(double, double, double, double, double, double);
	double	Okumura_Hata_suburban(double, double, double, double, double, double);
	double	Okumura_Hata_open_areas(double, double, double, double, double, double);
	double	COST_231_Hata(double, double, double, double, double, double);
	double	Suburban_1_9GHz_TA(double, double, double, double, double, double);
	double	Suburban_1_9GHz_TB(double, double, double, double, double, double);
	double	Suburban_1_9GHz_TC(double, double, double, double, double, double);
	double	M2M_UMTS_LOS(double, double, double, double, double, double);
	double	M2M_UMTS_NLOS(double, double, double, double, double, double, double);
	double	M2M_UMTS_EXTRA(double, double, double, double, double, double, double);
	double	ECC33(double, double, double, double, double, double);
	double	ECC33_dversion(double, double, double, double, double, double);
	double  AdHoc_LOS(double, double, double, double, double,double);
	double  HarXiaMicrocell_Low_Rise_NLOS(double, double, double, double, double, double, double);
	double  HarXiaMicrocell_High_Rise_Lateral(double, double, double, double, double, double);
	double  HarXiaMicrocell_High_Rise_ST(double, double, double, double, double, double);
	double  HarXiaMicrocell_LOS(double, double, double, double, double, double);
	double  HowardXia_UniformRoofTop(double, double, double, double, double, double,
			double, double, double);
	double  HowardXia_BSAntennaAboveRoofTopLevel(double, double, double, double, double, double,
			double, double, double);
	double  HowardXia_BSAntennaBelowRoofTopLevel(double, double, double, double, double, double,
			double, double, double);
	double  Indoor_2_4G_LOS_80211_a_b(double, double, double, double);
	double  Indoor_5_3G_LOS_80211_a_b(double, double, double, double);
	double  Indoor_2_4G_NLOS_80211_a_b(double, double, double, double);
	double  Indoor_5_3G_NLOS_80211_a_b(double, double, double, double);
	double  Bertoni_Walfisch_Urban(double, double, double, double, double, double, double, double);
	double  Egli_Urban_Rural(double, double, double, double, double, double);
	double  COST_231_Hata_3GPP_TR_25966_V8_Urban(double, double, double, double, double, double);
	double  COST_231_Hata_3GPP_TR_25966_V8_Suburban(double, double, double, double, double, double);

	double	Calculate_Carrier_Sense_Dist(struct cmInfo *, double);
};

void	Dump_CM_List(char*);
void 	Get_CM_Property();
double	Get_Antenna_Gain(int, int, int, double[]);
void	parseAGPFile(char *, double *);

#endif	/* __NCTUNS_CHANNEL_MODEL_h__ */
