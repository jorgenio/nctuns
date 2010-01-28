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

#include <sys/time.h>
#include "channel_Model.h"
#include "ricean_table.h"
#include "queryCM_struct.h"
#define PROP_CHANNEL_DEBUG 0
#define CSRANGE_DEBUG 0

/*
 * mshsu: 
 *    This file is used for calculating received power by different channel model function. 
 *    It can provide many propagation channel models to simulate the propagation path loss.
 */

/*
 *  Reference:
 *    D.B.Green, M.S.Obaidat, "An Accurate Line of Sight Propagation
 *    Performance Model for Ad-Hoc 802.11 Wireless LAN (WLAN) Devices",
 *    Proceedings of IEEE ICC 2002, New York, April 2002
 *   
 * Propagation model and power calculation
 *
 * Pr = Pt - Ct + Gt - PL + Gr - Cr 
 *
 * Note:
 * 	EIRP: Effective Isotropic Radiated Power (dbm) = Pt - Ct + Gt
 * 	Pr: Received power level at receiver (dbm)
 * 	Pt: Output power of transmitted (dbm)
 * 	PL: Path loss (db)
 * 	Gt: Antenna Gain of transmitter (dbi)
 * 	Gr: Antenna Gain of receiver (dbi)
 * 	Ct: Cable antennuation of transmitter (db)
 * 	Cr: Cable antennuation of receiver (db)
 *
 * 	Ct = Cr = 0 for integrated antenna
 */

int pcount = 0;
timeval startTime; // initial time
double startT;	// initial time (second)

channelModel::channelModel(double f){

	if(f == 0){
		fprintf(stderr, "[%s] Warning: Frequency can't be 0\n", __func__);
		exit(0);
	}
	last_hr = last_ht = 0;
   	crossover_dist = 0;	// meter
	Lambda = VELOCITY_OF_LIGHT/f;
	freq_ = f;	// Hz

	N = 0;
	fm0 = fs = dt = fm = 0.0;
	K = 0.0; 
	max_velocity = 30.0; // 30 m/s = 108 km/hr
	initialized = 1;

	gettimeofday(&startTime, 0);
	startT = (double) startTime.tv_sec + ((double) startTime.tv_usec / 1000000);
}

channelModel::~channelModel(){

}

// Note:
//   PropModel: index of the channel model
//   fv: fading variable
//   L: system loss >= 1
//   Pt: transmitter power
//   Gt: transmitter gain
//   Gr: receiver gain
//   dist: distance between transmitter and receiver
//   hbd: average height of surrounding buildings (rooftops)
//   w: street width
//   d: average separation distance between the rows of buildings
//   Pr: received power of receiver (dbm)
//                            _____     _____
//			     |_____|   |_____|
//                            |   |     |   |
//                            |   |     |   |
//                            |___|     |___|
//				   --w--
//            			-----d-----
//

double
channelModel::Pr(struct cmInfo *cminfo){

	if(!cminfo){
		fprintf(stderr, "[%s] Error: No cminfo data\n", __func__);
		return 0;
	}

	K = pow(10.0, (cminfo->RiceanK) / 10.0);	// Ricean factor K 

	int PropModel = cminfo->PropModel;	// index of the propagation channel model
	int fadingOpt = cminfo->fadingOpt_;	// index of fading model
	int txNid	= cminfo->txNid;
	int rxNid	= cminfo->rxNid;
	double fv	= cminfo->fv;		// fading variable
	double tZ	= cminfo->txAntennaHeight;
	double rZ	= cminfo->rxAntennaHeight;
	double dist	= cminfo->nodeDist;	// node distance (meter)
	double L	= cminfo->L;		// systom loss
	double Ptmw	= cminfo->Pt * 1e3;	// transmitter power (milli-Watt)
	double Pt	= 10 * log10(Ptmw);	// dbm
	double Gt	= cminfo->Gt;		// transmitter gain
	double Gr	= cminfo->Gr;		// receiver gain
	double hbd	= cminfo->hbd;		// average building heights (m)
	double w	= cminfo->w;		// street width (m)
	double d	= cminfo->d;		// average separation distance between the rows of buildings (m)
	double PLExp_	= cminfo->pathlossExp_;	// path-loss exponent
	double std_db_	= cminfo->std_db_;	// shadowing standard deviation (dB)
	double dist0_	= cminfo->dist0_;	// close-in reference distance (m)
	double Pr;				// received power of receiver (dbm)

	if((txNid == rxNid) || (dist == 0)){
		//fprintf(stderr, "[%s] Warning: the distance between two nodes is zero\n", __func__);
		return 0;
	}
	if(L == 0) L = 1;
	if(hbd == 0) hbd = 10.0; // 10 meter
	if(w == 0) w = 30.0; // 30 meters
	if(d == 0) d = 80.0; // 80 meter
	if(dist0_ == 0) dist0_ = 1;
	if(K == 0) K = 10.0;

	switch(PropModel){
		case FREE_SPACE:
			Pr = 10 * log10(Friis(Ptmw, Gt, Gr, Lambda, L, dist)); // dbm
			if(fadingOpt == 1){
				Pr = RayleighFading(Pr, fv);
			}
			else if(fadingOpt == 2){
				Pr = RiceanFading(Pr, txNid, rxNid);
			}
			break;
		case TWO_RAY_GROUND:
			Pr = TRG_FS(Ptmw, Gt, Gr, tZ, rZ, L, dist);
			if(fadingOpt == 1){
				Pr = RayleighFading(Pr, fv);
			}
			else if(fadingOpt == 2){
				Pr = RiceanFading(Pr, txNid, rxNid);
			}
			break;
		case FREE_SPACE_AND_SHADOWING:
			Pr = FS_Shadowing(Ptmw, Gt, Gr, L, dist, PLExp_, std_db_, dist0_);
			if(fadingOpt == 1){
				Pr = RayleighFading(Pr, fv);
			}
			else if(fadingOpt == 2){
				Pr = RiceanFading(Pr, txNid, rxNid);
			}
			break;
		case 3:
			Pr = LeeMicrocell(Pt, Gt, Gr, tZ, rZ, L, dist);
			break;
		case 4:
			Pr = Okumura_Hata_VHF_UHF(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 5:
			Pr = Okumura_Hata_large_urban(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 6:
			Pr = Okumura_Hata_medium_urban(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 7:
			Pr = Okumura_Hata_suburban(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 8:
			Pr = Okumura_Hata_open_areas(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 9:
			Pr = COST_231_Hata(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 10:
			Pr = Suburban_1_9GHz_TA(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 11:
			Pr = Suburban_1_9GHz_TB(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 12:
			Pr = Suburban_1_9GHz_TC(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 13:
			Pr = M2M_UMTS_LOS(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 14:
			Pr = M2M_UMTS_NLOS(Pt, Gt, Gr, tZ, rZ, dist, hbd);
			break;
		case 15:
			Pr = ECC33(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 16:
			Pr = ECC33_dversion(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 17:
			Pr = AdHoc_LOS(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 18:
			Pr = HarXiaMicrocell_Low_Rise_NLOS(Pt, Gt, Gr, tZ, rZ, dist, hbd);
			break;
		case 19:
			Pr = HarXiaMicrocell_High_Rise_Lateral(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 20:
			Pr = HarXiaMicrocell_High_Rise_ST(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 21:
			Pr = HarXiaMicrocell_LOS(Pt, Gt, Gr, tZ, rZ, dist);
			break;
		case 22:
			Pr = HowardXia_UniformRoofTop(Pt, Gt, Gr, tZ, rZ, dist, hbd, w, d);
			break;
		case 23:
			Pr = HowardXia_BSAntennaAboveRoofTopLevel(Pt, Gt, Gr, tZ, rZ, dist, hbd, w, d);
			break;
		case 24:
			Pr = HowardXia_BSAntennaBelowRoofTopLevel(Pt, Gt, Gr, tZ, rZ, dist, hbd, w, d);
			break;
		case 25:
			Pr = Indoor_2_4G_LOS_80211_a_b(Pt, Gt, Gr, dist);
			break;
		case 26:
			Pr = Indoor_5_3G_LOS_80211_a_b(Pt, Gt, Gr, dist);
			break;
		case 27:
			Pr = Indoor_2_4G_NLOS_80211_a_b(Pt, Gt, Gr, dist);
			break;
		case 28:
			Pr = Indoor_5_3G_NLOS_80211_a_b(Pt, Gt, Gr, dist);
			break;
		case 29:
			Pr = Bertoni_Walfisch_Urban(Pt, Gt, Gr, tZ, rZ, dist, hbd, d);
			break;
		case 30:
			Pr = Egli_Urban_Rural(Pt, Gt, Gr, tZ, rZ, dist); 
			break;
		case 31:
			Pr = COST_231_Hata_3GPP_TR_25966_V8_Urban(Pt, Gt, Gr, tZ, rZ, dist); 
			break;
		case 32:
			Pr = COST_231_Hata_3GPP_TR_25966_V8_Suburban(Pt, Gt, Gr, tZ, rZ, dist); 
			break;
		default:
			fprintf(stderr, "[%s] Warning:: No such channel model.\n", __func__);
			Pr = TRG_FS(Ptmw, Gt, Gr, tZ, rZ, L, dist);
			if(fadingOpt == 1){
				Pr = RayleighFading(Pr, fv);
			}
			else if(fadingOpt == 2){
				Pr = RiceanFading(Pr, txNid, rxNid);
			}
	}

#if PROP_CHANNEL_DEBUG
	double Prmw = pow(10, Pr/10);
	//if( (pcount == 0)  && (rxNid == 2)){
	fprintf(stderr, "[channelModel::%s]: model %d, Pr %f, Prmw %f, Ptmw %f, Gt %f, Gr %f, tZ %f, rZ %f, dist %f\n", 
			__func__, PropModel, Pr, Prmw, Ptmw, Gt, Gr, tZ, rZ, dist);
	//}
	pcount++;
	pcount %= 1000;
#endif

	return Pr; //dbm
}

// Reference:
//    NS-2 PropRicean channel model
//
double
channelModel::RiceanFading(double Pr, int txNid, int rxNid)
{
	double Pr_Rice = 0.0, Pr_tot;

	if(initialized) { /* Ricean loss */
		double time_index;
		double envelope_fac, tmp_x1, tmp_x2, x1_interp, x2_interp;
		double diffSec; // time passed from initial time (seconds)
		timeval callTime; // time of calling this function

		N = 16384;
		fs = 1000.0;
		fm0 = 30.0;

		fm = max_velocity / Lambda;
		if(gettimeofday(&callTime, 0) == 0){
			double callT = (double) callTime.tv_sec + ((double) callTime.tv_usec / 1000000);
			diffSec = callT - startT;
		} else {
			diffSec = 0;
		}

		time_index = ((diffSec + (double)(2*txNid*rxNid)) * fs * fm / fm0);
		
		time_index = time_index -
			    double(N)*floor(time_index/double(N));

		{
			/* Do envelope interpolation using Legendre
			   polynomials */
			double X0, X1, X2, X3;
			int ind0, ind1, ind2, ind3;
			
			ind1 = int(floor(time_index));
			ind0 = (ind1-1+N) % N;
			ind2 = (ind1+1) % N;
			ind3 = (ind1+2) % N;
			
			X1 = time_index - ind1;
			X0 = X1+1.0;
			X2 = X1-1.0;
			X3 = X1-2.0;

			x1_interp = ricean_data1[ind0]*X1*X2*X3/(-6.0) +
				ricean_data1[ind1]*X0*X2*X3*(0.5) +
				ricean_data1[ind2]*X0*X1*X3*(-0.5) +
				ricean_data1[ind3]*X0*X1*X2/6.0;
				
			x2_interp = ricean_data2[ind0]*X1*X2*X3/(-6.0) +
				ricean_data2[ind1]*X0*X2*X3*(0.5) +
				ricean_data2[ind2]*X0*X1*X3*(-0.5) +
				ricean_data2[ind3]*X0*X1*X2/6.0;
		}

		/* Find the envelope multiplicative factor */
		tmp_x1 = x1_interp + sqrt(2.0 * K);
		tmp_x2 = x2_interp;

		envelope_fac = (tmp_x1*tmp_x1 + tmp_x2*tmp_x2) / 
			(2.0 * (K+1)); 

		Pr_Rice = 10.0 * log10(envelope_fac);
	}
	
	Pr_tot = Pr + Pr_Rice;

	return Pr_tot;
}

// Reference:
//    NS-2 channel model
//
/* calculate the receiving power by two-ray ground and free-space path loss models */
double 
channelModel::TRG_FS(double Pt, double Gt, double Gr, double ht, double hr, double L, double d)
{
	double		Pr0, Pr;
	
	/* We're going to assume the ground is essentially flat.
	 * Thie empirical two ground ray reflection model doesn't make
	 * any sense if the ground is not a plane.
	 */

	if (ht != hr) {
		//fprintf(stderr, "[TwoRayGround] Warning:: TwoRayGround propagation model assumes flat ground.\n"); 
	}

	if (hr!=last_hr || ht!=last_ht) {
		/* recalc the cross-over distance
		 *
		 *	  16 * PI^2  * L * hr^2 * ht^2
		 * d^2 = --------------------------------
		 *	  	   Lambda^2
		 */
		crossover_dist = sqrt((16 * M_PI * M_PI * L * ht * ht * hr * hr)
					/ (Lambda * Lambda));
 		last_hr = hr; last_ht = ht;
	}
	
	/* If the transmitter is within the cross-over range, use the
	 * Friis equation. Otherwise, use the two-ray 
	 * ground reflection model.
	 */

  	if (d <= crossover_dist) {
		Pr0 = Friis(Pt, Gt, Gr, Lambda,  L, d); // mWatt
 		Pr = 10 * log10(Pr0); 
 		return(Pr); // dbm
	}
	else {
		Pr0 = TwoRay(Pt, Gt, Gr, ht, hr, d); // mWatt
 		Pr = 10 * log10(Pr0);
   		return(Pr); // dbm
	} 
}

/* calculate the receiving power by free space path loss and shadowing */
//
//  Reference:
//    NS-2 channel model
//
//  Note:
//   Pt: transmitter power in mWatt
//   pathLossExp_ : path-loss exponent
//   std_db_: shadowing standard deviation (db)
//   dist0_: close-in reference distance (m)
//
double 
channelModel::FS_Shadowing(double Pt, double Gt, double Gr, double L, double dist, 
		double pathlossExp_, double std_db_, double dist0_)
{
	// calculate receiving power at reference distance
	double Pr0 = Friis(Pt, Gt, Gr, Lambda, L, dist0_); // mWatt

	// calculate average power loss predicted by path loss model
	double avg_db = -10.0 * pathlossExp_ * log10(dist / dist0_);
   
	// get power loss by adding a normal random variable (shadowing)
	// the power loss is relative to that at reference distance dist0_
	double powerLoss_db = avg_db + gaussian_normal(0.0, std_db_);

	// calculate the receiving power at dist
	double Pr = Pr0 * pow(10.0, powerLoss_db / 10.0);
	
	return (10 * log10(Pr)); // dbm
}

/* Friis Free Space Path Loss Model */
//  Reference:
//    [1] NS-2 channel model
//
//    [2] The book "Antennas and Propagation for Wireless Communication Systems" second edition
//    Simon R. Saunders, Alejandro Aragon-Zavala
double
channelModel::Friis(double Pt, double Gt, double Gr, double lambda, double L, double d) {
	/* 
	 * Pt -- transmitted signal power
	 * Gt -- transmitter antenna gain
	 * Gr -- receiver antenna gain
	 * lambda -- wavelength (Meter)
	 * L -- system loss (L >= 1)
	 * d -- distance between transmitter and receiver (Meter)
	 * return -- received signal power 
	 */
	
	/* 
	 * Friis free space equation:
	 * 
	 *	Pt * Gt * Gr * (Lambda^2)
	 *  P = ---------------------------
 	 *	(4 * pi * d)^2 * L
	 */
	double M = lambda / (4 * M_PI * d);
	// The unit of the returned value is depends on the unit of Pt (Watt or milli-Watt) 
  	return ((Pt * Gt * Gr * (M * M)) / L); 
}

/* Two Ray Ground Path Loss Model */
//  Reference:
//    [1] NS-2 channel model
//
//    [2] The book "Antennas and Propagation for Wireless Communication Systems" second edition
//    Simon R. Saunders, Alejandro Aragon-Zavala
//
double
channelModel::TwoRay(double Pt, double Gt, double Gr, double ht, double hr, double d) {

	/*
	 * Two-ray ground reflection model (Plane Earth Loss Model)
	 *
	 *	Pt * Gt * Gr * (ht^2 * hr^2)
	 *  P = ------------------------------
	 *		d^4
	 */
	// The unit of the returned value is depends on the unit of Pt (Watt or milli-Watt) 
	return (Pt * Gt * Gr * (hr * hr * ht * ht) / (d * d * d *d)); 
}

/* Rayleigh Fading */
double
channelModel::RayleighFading(double mean, double fv)
{
	/*
	 * fv = fading variable
	 */
	int r = rand();
	double y = sqrt(fv);
	return mean + y + y * log((double)r/(double)INT_MAX); // dbm
}

/* Lee Microcell Path Loss Model */
// Reference:
//    The book "Antennas and Propagation for Wireless Communication Systems" second edition
//    Simon R. Saunders, Alejandro Aragon-Zavala
//
//    W.C.Y Lee and D.J.Y.Lee, "Microcell prediction in dense urban area,".
//    IEEE Trans. Veh. Technol., vol.47, pp. 246-253, Feb. 1998
double
channelModel::LeeMicrocell(double Pt, double Gt, double Gr, double ht, double hr, double L, double dist){

	/*  
	 * The ling-of-sight contribution is determined via a dual-slop model with
	 * an empirically determined path loss exponent of 4.3 for distances beyond
	 * the breakpoint rb at 900MHz.
	 *
	 * L = L_LOS + Alpha(B)
	 *
	 * Base station antenna heights are assumed to be below 15m.
	 * B is the total length of the straight-line path occupied by buildings.
	 * Alpha(B) reaches a nearly constant value of 18db for distances beyond 150m.
	 *
	 */

	double rb = 4*ht*hr/Lambda; // Fresnel zone distance
	double alphaB = 18; //db
	double PL;
	double Pr;

	if(dist <= rb){
		PL = 20*log10((4*M_PI*dist)/Lambda) + alphaB;
	}
	else{
		PL = (20*log10((4*M_PI*rb)/Lambda) + 43*log10(dist/rb)) + alphaB;
	}
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f, dist %f\n", __func__, PL, ht, hr, dist);
#endif
	return Pr; // dbm

}

//
// Reference:
//
//   Okumura-Hata propagation prediction model for VHF and UHF range, in
//   the "Prediction methods for the terrestrial land mobile
//   service in the VHF and UHF bands"
//   Rec. ITU-R P.529-3
//
double
channelModel::Okumura_Hata_VHF_UHF(double Pt, double Gt, double Gr, double hb, double hm, double dist){
	/*
	 * hb: the height of base station
	 * hm: the height of mobile station
	 * ahm: alpha of hm
	 * E: field strength for 1kW e.r.p
	 * R: distance (km)
	 * freq_: frequency (MHz)
	 *
	 * This model is valid for
	 * 30MHz <= freq_ <= 1500MHz
	 * 30m   <= hb <= 200m
	 * 1m    <= hm <= 10m
	 * 0km	 <= R  <= 100km
	 *
	 */
	double freq = freq_/1e6; //MHz
	double ahm = (1.1*log10(freq)-0.7)*hm - (1.56*log10(freq)-0.8);
	double R = dist/1000; //km
	double A;
	double PL;
	double Pr;
	if (R > 20 && R <= 100){
		double b = 1 + (0.14 + 1.87*1e-4*freq + 1.07 * 1e-3 * hb)*(pow(log10(R/20), 0.8));
		A = pow(log10(R), b);
	}
	else{
		// R <= 20 km
		A = log10(R);
	}
	PL = 69.82 - 6.16*log10(freq) + 13.82*log10(hb) + ahm - (44.9 - 6.55*log10(hb))*A; //db

	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr; //dbm 
}

// Reference:
//    The book "Antennas and Propagation for Wireless Communication Systems" second edition
//    Simon R. Saunders, Alejandro Aragon-Zavala
//
//    Masaharu Hata, "Empirical Formula for Propagation Loss in Land Mobile Radio
//    Services," IEEE Transactions on Vehicular Technology, Vol29, No.3, pp.317-325, August 1980 
//
// Note:
//    150 MHz <= freq <= 1500 MHz
//    30 m    <= hb   <= 200 m
//    1 m     <= hm   <= 10 m
//    1 km    <= R    <= 20 km
double
channelModel::Okumura_Hata_large_urban(double Pt, double Gt, double Gr, double hb, double hm, double dist){
	double freq = freq_/1e6; //MHz
	double A = 69.55 + 26.16*log10(freq) - 13.82*log10(hb);
	double B = 44.9 - 6.55*log10(hb);
	double R = dist / 1000; //km
	double E;
	double PL, Pr;

	if(freq >= 300){
		E = 3.2*pow(log10(11.75*hm), 2) - 4.97;
	}
	else{
		E = 8.29*pow(log10(1.54*hm), 2) - 1.1;
	}
	PL = A + B*log10(R) - E;
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr; //dbm 
}

// medium or small city
double
channelModel::Okumura_Hata_medium_urban(double Pt, double Gt, double Gr, double hb, double hm, double dist){
	double freq = freq_/1e6; //MHz
	double A = 69.55 + 26.16*log10(freq) - 13.82*log10(hb);
	double B = 44.9 - 6.55*log10(hb);
	double R = dist / 1000; //km
	double E = (1.1*log10(freq) - 0.7)*hm - (1.56*log10(freq) - 0.8);
	double PL, Pr;

	PL = A + B*log10(R) - E;
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr; //dbm 
}

double
channelModel::Okumura_Hata_suburban(double Pt, double Gt, double Gr, double hb, double hm, double dist){
	double freq = freq_/1e6; //MHz
	double A = 69.55 + 26.16*log10(freq) - 13.82*log10(hb);
	double B = 44.9 - 6.55*log10(hb);
	double R = dist / 1000; //km
	double C = 2 * pow(log10(freq/28), 2) + 5.4;
	double PL, Pr;

	PL = A + B*log10(R) - C;
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr; //dbm 
}

// open areas
double
channelModel::Okumura_Hata_open_areas(double Pt, double Gt, double Gr, double hb, double hm, double dist){
	double freq = freq_/1e6; //MHz
	double A = 69.55 + 26.16*log10(freq) - 13.82*log10(hb);
	double B = 44.9 - 6.55*log10(hb);
	double R = dist / 1000; //km
	double D = 4.78 * pow(log10(freq), 2) - 18.33 * log10(freq) + 40.94;
	double PL, Pr;

	PL = A + B*log10(R) - D;
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr; //dbm 
}

// Reference:
//
//   V. S. Abhayawardhana, I. J. Wassell, D. Crosby, M. P. Sellars and 
//   M. G. Brown, “Comparison of Empirical Propagation Path Loss Models
//   for Fixed Wireless Access Systems,” In Proc. of the IEEE Vehicular 
//   Technology Conference (VTC’05), Vol. 1, May 30-Jun 1, 2005,
//   pp. 73-77, Stockholm, Sweden.
//
//   This model is designed for:
//
//   500MHz <= frequency <= 2000MHz
//   urban, suburban and rural (flat) environments
//
//   Notes:
//      hb is the height of AP antenna
//      hr is the height of CPE antenna
//      Cm = 0 dB for suburban or open environments. We use this in our simulator.
//      Cm = 3 dB for urban environments
//
double
channelModel::COST_231_Hata(double Pt, double Gt, double Gr, double hb, double hr, double dist){

	double ahm, PL;
	double Cm = 0; // db
	double Pr;
	double freq = freq_ / 1e6; //MHz
	double d = dist / 1e3; //km

	if(freq >= 400){
		// for urban environments
		ahm = 3.2*log10(11.75*hr)*log10(11.75*hr) - 4.97;
	}
	else{
		// for suburban or rural (flat) environments
		ahm = (1.1*log10(freq) - 0.7)*hr - (1.56*log10(freq) - 0.8);
	}
	PL = 46.3 + 33.9*log10(freq) - 13.82*log10(hb) - ahm + (44.9 - 6.55*log10(hb))*log10(d) + Cm; //db
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hr %f, dist %f\n", __func__, PL, hb, hr, dist);
#endif
	return Pr; //dbm
}

//
//   Stanford University Interim (SUI) Model
//
// Reference:
//
//   V. Erceg et. al, "An empirically based path loss model for wireless
//   channels in suburban environments," IEEE JSAC, vol. 17, no. 7, July 1999,
//   pp. 1205-1211.
//
//   V. S. Abhayawardhana, I. J. Wassell, D. Crosby, M. P. Sellars and 
//   M. G. Brown, “Comparison of Empirical Propagation Path Loss Models
//   for Fixed Wireless Access Systems,” In Proc. of the IEEE Vehicular 
//   Technology Conference (VTC’05), Vol. 1, May 30-Jun 1, 2005,
//   pp. 73-77, Stockholm, Sweden.
//
// Median path loss:
//
//   PL = A + 10*r*log10(d/d0) + s; d>=d0
//
// Freqyence correction term:
//
//   PLf = 6*log10(f/2000); f is the frequency in MHz
//
// Receive antenna height correction term:
//
//   PLh = -10.8 * log10(h/2); for category B
// 
// Note:
//
//   Frequency: 1.9GHz
//   Area: Suburban
//   10 <= BS antenna height (m) <= 80
//  0.1 <= base-to-terminal distance (km) <= 8
//    2 <= SS(terminal) antenna height <= 10
//
double 
channelModel::Suburban_1_9GHz_TA(double Pt, double Gt, double Gr, 
		double _txHeight, double _rxHeight, double dist)
{
	/*
	 * We adopt terrain terrain A.
	 */
	double a = 4.6;
	double b = 0.0075;
	double c = 12.6;
	double d0 = 100.0;
	double A = 78.0;
	double r = a - b * _txHeight + c / _txHeight;
	double x = gaussian(-1.5, 1.5);
	double y = gaussian(-2.0, 2.0);
	double z = gaussian(-2.0, 2.0);
	double mean = 9.6;
	double stdDev = 3.0;
	double s = 10.0 * x * log10(dist / d0) + y * mean + y * z * stdDev;
	double PLf = 6 * log10(freq_ * 1e-6 / 2000.0);
	double PLh = -10.8 * log10(_rxHeight / 2.0);
	double PL = A + 10.0 * r * log10(dist / d0) + s + PLf + PLh; // db
	double Pr = Pt + Gt + Gr - PL; // dbm
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f, dist %f\n", __func__, PL, _txHeight, _rxHeight, dist);
#endif
	return Pr;
}

double 
channelModel::Suburban_1_9GHz_TB(double Pt, double Gt, double Gr,
		double _txHeight, double _rxHeight, double dist)
{
	/*
	 * We adopt terrain terrain B.
	 */
	double a = 4.0;
	double b = 0.0065;
	double c = 17.1;
	double d0 = 100.0;
	double A = 78;
	double r = a - b * _txHeight + c / _txHeight;
	double x = gaussian(-1.5, 1.5);
	double y = gaussian(-2.0, 2.0);
	double z = gaussian(-2.0, 2.0);
	double mean = 9.6;
	double stdDev = 3.0;
	double s = 10.0 * x * log10(dist / d0) + y * mean + y * z * stdDev;
	double PLf = 6 * log10(freq_ * 1e-6 / 2000.0);
	double PLh = -10.8 * log10(_rxHeight / 2.0);
	double PL = A + 10.0 * r * log10(dist / d0) + s + PLf + PLh;
	double Pr = Pt - PL;//Pt + Gt + Gr - PL; // dbm
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f, dist %f\n", __func__, PL, _txHeight, _rxHeight, dist);
#endif
	return Pr;
}

double 
channelModel::Suburban_1_9GHz_TC(double Pt, double Gt, double Gr,
		double _txHeight, double _rxHeight, double dist)
{
	/*
	 * We adopt terrain terrain C.
	 */
	double a = 3.6;
	double b = 0.005;
	double c = 20;
	double d0 = 100.0;
	double A = 78;
	double r = a - b * _txHeight + c / _txHeight;
	double x = gaussian(-1.5, 1.5);
	double y = gaussian(-2.0, 2.0);
	double z = gaussian(-2.0, 2.0);
	double mean = 9.6;
	double stdDev = 3.0;
	double s = 10.0 * x * log10(dist / d0) + y * mean + y * z * stdDev;
	double PLf = 6 * log10(freq_ * 1e-6 / 2000.0);
	double PLh = -10.8 * log10(_rxHeight / 2.0);
	double PL = A + 10.0 * r * log10(dist / d0) + s + PLf + PLh;
	double Pr = Pt + Gt + Gr - PL; // dbm
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f, dist %f\n", __func__, PL, _txHeight, _rxHeight, dist);
#endif
	return Pr;
}

/*
 * Reference:
 *      K. Konstantinou,
 *      "A Measurement-Based Model for Mobile-to-Mobile UMTS Links,"
 *      VTC2007-Spring. IEEE 65th.
 *
 * Line of Sight:
 *    PL_los(db) = 4.62 + 20log10(4PI / lambda) - 2.24ht - 4.9hr + 29.6log10(d)
 * Non Line of Sight:
 *    PL_nlos(db) = 20log10(4PI / lambda) - 2hr + 40log10(d) + C
 * Extra Path Loss between LOS and NLOS:
 *    PL_extra(db) = -4.62 + 2.24ht + 2.9hr + 10.4log10(d) + C
 *
 *      where   lambda: wavelength
 *              ht: transmitter height 0.5 ~ 3 (m)
 *              hr: receiver height 0.5 ~ 3 (m)
 *              hb: the average building height
 *              d: distance between the transmitter and the receiver (m)
 */
double
channelModel::M2M_UMTS_LOS(double Pt, double Gt, double Gr, 
		double ht, double hr, double dist)
{
	if (dist == 0.0)
		dist = 10;

    double PL = 4.62 + 20 * log10(4 * M_PI / Lambda) - 2.24 * ht - 4.9 * hr + 29.6 * log10(dist);
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f\n", __func__, PL, ht, hr);
#endif
    double Pr = Pt + Gt + Gr - PL;
    return Pr; //dbm
}

double
channelModel::M2M_UMTS_NLOS(double Pt, double Gt, double Gr, 
		double ht, double hr, double dist, double hb)
{
    double C;
    if (dist == 0.0)
	dist = 10;
    if(hb > 18)
	    C = 0;
    else
	    C = -4;

    double PL = 20 * log10(4 * M_PI / Lambda) - 2 * hr + 40*log10(dist) + C;
    double Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f, hb %f\n", __func__, PL, ht, hr, hb);
#endif
    return Pr; //dbm
}

/* Refernece for the ECC-33 path loss model:
 *
 * Electronic Communication Committee (ECC) within the European
 * Conference of Postal and Telecommunications Administration (CEPT),
 * "The Analysis of the Coexistence of FWA cells in the 3.4 - 3.8 GHz Band,"
 * technical report, ECC Report 33, May 2003.
 *
 * FWA: Fixed Wireless Access
 *
 * PL(db) = Afs + Abm - Gb - Gr
 * where Afs denotes the free space attenuation, 
 *       Abm denotes the basic median path loss,
 *       Gb denotes the BS height gain factor, and
 *       Gr denotes the terminal (receiver) station height gain factor.
 *
 *       Afs = 92.4 + 20log10(d) + 20log10(f)
 *       Abm = 20.41 + 9.83log10(d) + 7.894log10(f) + 9.56(pow(log10(f), 2))
 *       Gb  = log10(hb/200){13.958+5.8(pow(log10(d), 2))}
 *       Gr  = [42.57+13.7log10(f)][log10(hr)-0.585]
 *
 *       where f is the freq. in GHz, d is the distance between the BS and 
 *       the terminal nodes, hb is the height of the BS antenna in meters,
 *       hr is the height of the terminal antenna in meters.
 */
double 
channelModel::ECC33(double Pt, double G1, double G2, double hb, double hr, double dist_) {
	// G1 : antenna gain of transmitter
	// G2 : antenna gain of receiver

    double dist = dist_/1e3; /* translate the unit from meter to kilometer */
    double freq = freq_/1e9; /* translate the unit from Hz to GHz */

    //fprintf(stderr, "PL:ECC-33: dist: %f freq:%f \n", dist, freq);

    double     Afs = 92.4 + 20*log10(dist) + 20*log10(freq);
    double     Abm = 20.41 + 9.83 * log10(dist) + 7.894 * log10(freq) + 9.56 * (pow(log10(freq), 2));

    double     Gb  = log10((hb/200)) * (13.958 + 5.8 * (pow(log10(dist), 2)));

    /* for medium city environment */
    double     Gr  = (42.57 + 13.7*log10(freq))*(log10(hr) - 0.585);

    /* for large city environment */
    //double     Gr  = 0.795*hr - 1.862;

    double PL = Afs + Abm - Gb - Gr;
    double Pr = Pt - PL; // + Gb + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hr %f, hb %f\n", __func__, PL, hb, hr, hb);
#endif
    return Pr;

}

/* In the directional antenna version of the ECC-33 path loss model. The antenna
 * gains of the transmitting node and receving node are given directly by
 * the passed parameters. In such a way, although the scale of the antenna
 * pattern may differ from that provided by the original ECC-33 path loss model,
 * the resultant path loss model is still valuable for describing the interference
 * relationship among neighboring nodes. The scale of radio interference is
 * increased or decreased equally; therefore, the path loss pattern will not be
 * distorted.
 */

double 
channelModel::ECC33_dversion(double Pt, double G1, double G2, 
		double hb, double hr, double dist){
	// G1 : antenna gain of transmitter
	// G2 : antenna gain of receiver

    dist = dist/1e3; /* translate the unit from meter to kilometer */
    double     freq = freq_/1e9; /* translate the unit from Hz to GHz */

    double     Afs = 92.4 + 20*log10(dist) + 20*log10(freq);
    double     Abm = 20.41 + 9.83 * log10(dist) + 7.894 * log10(freq) + 9.56 * (pow(log10(freq), 2));

    double     Gb  = G1;
    double     Gr  = G2;

    double     PL = Afs + Abm - Gb - Gr;
    double     Pr = Pt + G1 + G2 - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hr %f, hb %f\n", __func__, PL, hb, hr, hb);
#endif
    return     Pr;

 }
//
//  Reference:
//    D.B.Green, M.S.Obaidat, "An Accurate Line of Sight Propagation
//    Performance Model for Ad-Hoc 802.11 Wireless LAN (WLAN) Devices",
//    Proceedings of IEEE ICC 2002, New York, April 2002
//
//  Note:
//    ht,hr : antenna heights for Tx and Rx
//
double 
channelModel::AdHoc_LOS(double Pt, double Gt, double Gr, 
		double ht, double hr, double dist) {

	double freq = freq_ / 1e9; //GHz
	double PL = 40.0*log10(dist) + 20.0*log10(freq) - 20.0*log10(ht*hr);
	double Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, ht %f, hr %f, dist %f\n", __func__, PL, ht, hr, dist);
#endif
	return Pr; // dbm
}

/*
 *  Reference:
 *    Dongsoo Har, HowardH.Xia, Henry L. Bertoni, "Path-Loss Prediction
 *    Model for Microcells", IEEE Transactions on Vehicular Technology,
 *    Vol. 48, No. 5, September 1999
 *  
 *  Implement Table 1
 *
 *  This model is valid on the following conditions:
 *    0.9 GHz <= frequency <= 2 GHz
 *    -8 m    <= delta_h   <= 6 m
 *    0.05 km <= Rk        <= 3 km
 *
 *    sgn(x) = 1, if x >= 0
 *    sgn(x) = -1, otherwise
 *
 *  Note:
 *    delta_h = hb - hbd
 *    delta_hm = hbd - hm
 *    hbd: average height of surrounding rooftops
 *    Rk: mobile distance from transmitter in km
 *    delta_h: relative height of transmitter to average building height in meters
 *    delta_hm: height of the last building relative to the mobile in meters
 *    rh: distance of mobile from the last rooftop in meters = 250m
 *    hb: antenna height of base station
 *    hm: antenna height of mobile station
 *    Rbk: break point distance converted to km = 4*hb*hm / 1000*Lambda
 *    sgn(x): sign function
 */
double
channelModel::HarXiaMicrocell_Low_Rise_NLOS(double Pt, double Gt, double Gr,
		double hb, double hm, double dist, double hbd){
	if(hbd == 0) hbd = 8; // default 8 m

	double delta_h = fabs(hb - hbd);
	double delta_hm = fabs(hbd - hm);
	double Rk = dist / 1000; // km
	double freq = freq_ / 1e9; // GHz
	double rh = 250;
	double sgn; // sign function of delta_h => sgn(delta_h)
	double PL, Pr;
	if(delta_h >= 0)
		sgn = 1;
	else
		sgn = -1;
	PL = 139.01 + 42.59*log10(freq) 
		- (14.97 + 4.99*log10(freq)) * sgn * log10(1 + delta_h)
		+ (40.67 - 4.57*sgn*log10(1 + delta_h)) * log10(Rk)
		+ 20*log10(1 + delta_hm / 7.8) + 10*log10(20 / rh);
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, hbd %f, dist %f\n", __func__, PL, hb, hm, hbd, dist);
#endif
	return Pr;
}

double
channelModel::HarXiaMicrocell_High_Rise_Lateral(double Pt, double Gt, double Gr,
		double hb, double hm, double dist){
	double Rk = dist / 1000; // km
	double freq = freq_ / 1e9; // GHz
	double PL, Pr;
	PL = 135.41 + 12.49*log10(freq) - 4.99*log10(hb)
		+ (46.84 - 2.34*log10(hb)) * log10(Rk);
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/* combined Staircase and Transverse */
double
channelModel::HarXiaMicrocell_High_Rise_ST(double Pt, double Gt, double Gr,
		double hb, double hm, double dist){
	double Rk = dist / 1000; // km
	double freq = freq_ / 1e9; // GHz
	double PL, Pr;
	PL = 143.21 + 29.74*log10(freq) - 0.99 * log10(hb)
		+ (47.23 + 3.72 * log10(hb)) * log10(Rk);
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

double
channelModel::HarXiaMicrocell_LOS(double Pt, double Gt, double Gr,
		double hb, double hm, double dist){
	double Rk = dist / 1000; // km
	double freq = freq_ / 1e9; // GHz
	double PL, Pr;
	double Rbk = 4*hb*hm / (1000*Lambda);
	if(Rk <= Rbk){
		PL = 81.14 + 39.40 * log10(freq) - 0.09 * log10(hb)
			+ (15.80 - 5.73 * log10(hb)) * log10(Rk);
	}else{
		PL = 48.38 - 32.10 * log10(Rbk) + 45.70 * log10(freq)
			+ (25.34 - 13.90 * log10(Rbk)) * log10(hb)
			+ (32.10 + 13.90 * log10(hb)) * log10(Rk);
	}
	
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/*  Reference:
 *    H. Xi, "A Simplified Analytical Model for predicting path loss in urban and 
 *    suburban environments," IEEE transactions on vehicular technology,
 *    vol. 46, pp. 1040-1046, 1997.
 *
 *  Note:
 *    dist: distance between two nodes (kilometers)
 *    delta_h = hb - hbd
 *    delta_hm = hbd - hm
 *    hbd: average height of surrounding rooftops
 *    delta_h: relative height of transmitter to average building height in meters
 *    delta_hm: height of the last building relative to the mobile in meters
 *    hb: antenna height of base station
 *    hm: antenna height of mobile station
 *    w: street width
 *    d: average separation distance between the rows of buildings
 *    x: horizontal distance between the mobile station and the diffracting edge.
 *       In general, x is taken as x = w/2 by assuming that the mobile travel 
 *       in the middle lane of a street.
 */       

/* Assume uniform height of rooftops */
double
channelModel::HowardXia_UniformRoofTop(double Pt, double Gt, double Gr,
		double hb, double hm, double dist, double hbd, double w, double d){

	if(w == 0) w = 30;
	if(d == 0) d = 80;
	if(hbd == 0) hbd = 12;

	double PL, Pr;
	double delta_hm = hbd - hm;
	double x = w/2;
	double thita = atan(delta_hm / x);
	double r = sqrt(delta_hm*delta_hm + x*x);
	
	PL = -10 * log10(pow((Lambda / (2*sqrt(2)*M_PI*dist)),2))
		-10 * log10((Lambda / (2*M_PI*M_PI*r)) * pow(((1/thita) - (1/(2*M_PI+thita))),2))
		-10 * log10(pow((d/dist),2)); 
		
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/* Assume base station antenna is above the rooftop level*/
double
channelModel::HowardXia_BSAntennaAboveRoofTopLevel(double Pt, double Gt, double Gr,
		double hb, double hm, double dist, double hbd, double w, double d){
	
	if(w == 0) w = 30;
	if(d == 0) d = 80;
	if(hbd == 0) hbd = 12;

	double PL, Pr;
	double delta_hb = fabs(hb - hbd); // hb > hbd;
	double delta_hm = hbd - hm;
	double x = w/2;
	double thita = atan(delta_hm / x);
	double r = sqrt(delta_hm*delta_hm + x*x);

	PL = -10 * log10(pow(Lambda / (4*M_PI*dist), 2))
		-10 * log10((Lambda / (2*M_PI*M_PI*r)) * pow(((1/thita) - (1/(2*M_PI+thita))), 2))
		-10 * log10(2.35 * 2.35 * pow(((delta_hb/dist) * sqrt(d/Lambda)), 1.8));

	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/* Assume base station antenna is below the rooftop level*/
double
channelModel::HowardXia_BSAntennaBelowRoofTopLevel(double Pt, double Gt, double Gr,
		double hb, double hm, double dist, double hbd, double w, double d){
	
	if(w == 0) w = 30;
	if(d == 0) d = 80;
	if(hbd == 0) hbd = 12;

	double PL, Pr;
	double delta_hm = hbd - hm;
	double delta_hb = hb - hbd; // hb < hbd;
	double x = w/2;
	double thita = atan(delta_hm / x);
	double phi = -atan(delta_hb / d);
	double r = sqrt(delta_hm*delta_hm + x*x);
	
	PL = -10 * log10(pow(Lambda / (2*sqrt(2)*M_PI*dist), 2))
		-10 * log10((Lambda / (2*M_PI*M_PI*r)) * pow(((1/thita) - (1/(2*M_PI+thita))),2))
		-10 * log10(pow(d/(2*M_PI*(dist-d)),2) * (Lambda / sqrt(delta_hb*delta_hb + d*d))
				* pow((1/phi) - (1/(2*M_PI + phi)) ,2));

	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/* Reference:
 *    Lim, J., Shin, Y. & Yook, J. "Experimanetal performance analysis of IEEE802.11a/b
 *    operating at 2.4 and 5.3 GHz," proceedings of 10th Asia-Pacific conference
 *    on communications, 2004, pp. 133-136.
 */
double
channelModel::Indoor_2_4G_LOS_80211_a_b(double Pt, double Gt, double Gr, double dist){
	double PL = 33.4 + 10 * 1.4 * log10(dist) + 4.42;
	double Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, dist %f\n", __func__, PL, dist);
#endif
	return Pr;
}

double
channelModel::Indoor_5_3G_LOS_80211_a_b(double Pt, double Gt, double Gr, double dist){
	double PL = 43.6 + 10 * 1.7 * log10(dist) + 2.74;
	double Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, dist %f\n", __func__, PL, dist);
#endif
	return Pr;
}

double
channelModel::Indoor_2_4G_NLOS_80211_a_b(double Pt, double Gt, double Gr, double dist){
	double PL = 35.7 + 10 * 3.75 * log10(dist) + 5.09;
	double Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, dist %f\n", __func__, PL, dist);
#endif
	return Pr;
}

double
channelModel::Indoor_5_3G_NLOS_80211_a_b(double Pt, double Gt, double Gr, double dist){
	double PL = 42.62 + 10 * 4.85 * log10(dist) + 5.18;
	double Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, dist %f\n", __func__, PL, dist);
#endif
	return Pr;
}

/*  Reference:
 *    B. Yesim HANCE and I. Hakki CAVDAR, "Mobile Radio Propagation Measurements and Tuning the
 *    Path Loss Model in Urban Areas at GSM-900 Band in Istanbul-Turkey", IEEE Vehicular Technology
 *    Conference(VTC2004),Vol., pp.139 – 143, Fall 2004.
 *
 *  Note:
 *    R: distance between base station and the mobile in km
 *    f: frequency in MHz
 *    hbd: average height of surrounding rooftops
 *    hb: antenna height of base station
 *    hm: antenna height of mobile station
 *    d: center-to-center spacing of the row of the buildings in meters
 */
double
channelModel::Bertoni_Walfisch_Urban(double Pt, double Gt, double Gr, 
		double hb, double hm, double dist, double hbd, double d){
	if(hbd == 0) hbd = 10;
	if(d == 0) d = 40;

	double R = dist / 1e3; //km
	double freq = freq_ / 1e6; //MHz
	double A;
	double PL, Pr;

	A = 5*log10(pow((d/2) ,2) + pow(fabs(hbd - hm) ,2)) - 9*log10(d)
		+ 20 * log10(1 + fabs(atan(2*(hbd - hm)/d)));
	PL = 89.5 + A + 38*log10(R) - 18*log10(1 + fabs(hb - hbd)) + 21*log10(freq);
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/*  Reference:
 *    G. Y. Delisle, J. Lefevre, M. Lecours and J. Chouinard, "Propagation loss
 *    prediction: a comparative study with application to the mobile radio channel,"
 *    IEEE Transactions on Vehicular Technology, 26 (4), 295-308, 1985
 */
double
channelModel::Egli_Urban_Rural(double Pt, double Gt, double Gr,
		double hb, double hm, double dist){
	double R = dist / 1e3; //km
	double freq = freq_ / 1e6; //MHz
	double Lm;
	double PL, Pr;
	if(hm < 10){
		Lm = 76.3 - 10 * log10(hm);
	}
	else{
		Lm = 85.9 - 20 * log10(hm);
	}
	PL = 20*log10(freq) + 40*log10(R) - 20*log10(hb) + Lm;
	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/* Reference:
 *    3GPP TR 25.996 V8.0.0 (2008-12)
 *    3rd Generation Partnership Project; 
 *    Technical Specification Group Radio Access Network;
 *    Spatial channel model for Multiple Input Multiple Output (MIMO) simulations
 *    (Release 8)
 *    On page 17.
 *
 *  Note:
 *    dist: distance between base station and the mobile in meters
 *    f: frequency in MHz
 *    hb: antenna height of base station in meters
 *    hm: antenna height of mobile station in meters
 *    C: is a constant factor (C = 0dB for suburban macro and C = 3db for urban macro).
 *
 *  Suggested Parameter values:
 *    hb = 32 m
 *    hm = 1.5 m
 *    f = 1900 MHz
 *    dist > 35 m
 */

double
channelModel::COST_231_Hata_3GPP_TR_25966_V8_Urban(double Pt, double Gt, double Gr,
		double hb, double hm, double dist){

	double freq = freq_ / 1e6; //MHz
	double PL, Pr;
	int C = 3; // urban

	if(dist < 35){
#if PROP_CHANNEL_DEBUG
		fprintf(stderr, "[%s]: Warning: The distance %f is too small to fit this channel model\n", 
				__func__, dist);
#endif
	}
	PL = (44.9 - (6.55 * log10(hb))) * log10(dist / 1e3) + 45.5 
		+ (35.46 - (1.1 * hm)) * log10(freq)
		- 13.82 * log10(hb) + 0.7 * hm + C;

	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

double
channelModel::COST_231_Hata_3GPP_TR_25966_V8_Suburban(double Pt, double Gt, double Gr,
		double hb, double hm, double dist){

	double freq = freq_ / 1e6; //MHz
	double PL, Pr;
	int C = 0; // suburban

	if(dist < 35){
#if PROP_CHANNEL_DEBUG
		fprintf(stderr, "[%s]: Warning: The distance %f is too small to fit this channel model\n", 
				__func__, dist);
#endif
	}
	PL = (44.9 - (6.55 * log10(hb))) * log10(dist / 1e3) + 45.5 
		+ (35.46 - (1.1 * hm)) * log10(freq)
		- 13.82 * log10(hb) + 0.7 * hm + C;

	Pr = Pt + Gt + Gr - PL;
#if PROP_CHANNEL_DEBUG
    fprintf(stderr, "[%s]: PL = %f, hb %f, hm %f, dist %f\n", __func__, PL, hb, hm, dist);
#endif
	return Pr;
}

/*
 * Note:
 *    The following APIs will be used by GUI, and GUI will use the stdout to read data.
 *    Therefore, don't use printf to print out the error message.
 *    We should print out the message to stderr to avoid read error from GUI.
 */

/*
 * This API is used for GUI to calculate carrier sense distance.
 */
double
channelModel::Calculate_Carrier_Sense_Dist(struct cmInfo *cminfo, double CSThresh){
	if(!cminfo){
		fprintf(stderr, "[%s] Error: No cminfo data\n", __func__);
		return 0;
	}

	K = pow(10.0, (cminfo->RiceanK) / 10.0);	// Ricean factor K 

	int PropModel = cminfo->PropModel;	// index of the propagation channel model
	int fadingOpt = cminfo->fadingOpt_;	// index of fading model
	int txNid	= cminfo->txNid;
	int rxNid	= cminfo->rxNid;
	double fv	= cminfo->fv;		// fading variable
	double tZ	= cminfo->txAntennaHeight;
	double rZ	= cminfo->rxAntennaHeight;
	double L	= cminfo->L;		// systom loss
	double Ptmw	= cminfo->Pt * 1e3;	// transmitter power (milli-Watt)
	double Pt	= 10 * log10(Ptmw);	// dbm
	double Gt	= cminfo->Gt;		// transmitter gain
	double Gr	= cminfo->Gr;		// receiver gain
	double hbd	= cminfo->hbd;		// average building heights (m)
	double w	= cminfo->w;		// street width (m)
	double d	= cminfo->d;		// average separation distance between the rows of buildings (m)
	double PLExp_	= cminfo->pathlossExp_;	// path-loss exponent
	double std_db_	= cminfo->std_db_;	// shadowing standard deviation (dB)
	double dist0_	= cminfo->dist0_;	// close-in reference distance (m)
	double Pr = CSThresh + 1;		// received power of receiver (dbm)
	double dist = 1;

	if(L == 0) L = 1;
	if(hbd == 0) hbd = 10.0; // 10 meter
	if(w == 0) w = 30.0; // 30 meters
	if(d == 0) d = 80.0; // 80 meter
	if(dist0_ == 0) dist0_ = 1;
	if(K == 0) K = 10.0;

	while(Pr > CSThresh){
		switch(PropModel){
			case FREE_SPACE:
				Pr = 10 * log10(Friis(Ptmw, Gt, Gr, Lambda, L, dist)); // dbm
				if(fadingOpt == 1){
					Pr = RayleighFading(Pr, fv);
				}
				else if(fadingOpt == 2){
					Pr = RiceanFading(Pr, txNid, rxNid);
				}
				break;
			case TWO_RAY_GROUND:
				Pr = TRG_FS(Ptmw, Gt, Gr, tZ, rZ, L, dist);
				if(fadingOpt == 1){
					Pr = RayleighFading(Pr, fv);
				}
				else if(fadingOpt == 2){
					Pr = RiceanFading(Pr, txNid, rxNid);
				}
				break;
			case FREE_SPACE_AND_SHADOWING:
				Pr = FS_Shadowing(Ptmw, Gt, Gr, L, dist, PLExp_, std_db_, dist0_);
				if(fadingOpt == 1){
					Pr = RayleighFading(Pr, fv);
				}
				else if(fadingOpt == 2){
					Pr = RiceanFading(Pr, txNid, rxNid);
				}
				break;
			case 3:
				Pr = LeeMicrocell(Pt, Gt, Gr, tZ, rZ, L, dist);
				break;
			case 4:
				Pr = Okumura_Hata_VHF_UHF(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 5:
				Pr = Okumura_Hata_large_urban(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 6:
				Pr = Okumura_Hata_medium_urban(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 7:
				Pr = Okumura_Hata_suburban(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 8:
				Pr = Okumura_Hata_open_areas(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 9:
				Pr = COST_231_Hata(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 10:
				Pr = Suburban_1_9GHz_TA(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 11:
				Pr = Suburban_1_9GHz_TB(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 12:
				Pr = Suburban_1_9GHz_TC(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 13:
				Pr = M2M_UMTS_LOS(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 14:
				Pr = M2M_UMTS_NLOS(Pt, Gt, Gr, tZ, rZ, dist, hbd);
				break;
			case 15:
				Pr = ECC33(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 16:
				Pr = ECC33_dversion(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 17:
				Pr = AdHoc_LOS(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 18:
				Pr = HarXiaMicrocell_Low_Rise_NLOS(Pt, Gt, Gr, tZ, rZ, dist, hbd);
				break;
			case 19:
				Pr = HarXiaMicrocell_High_Rise_Lateral(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 20:
				Pr = HarXiaMicrocell_High_Rise_ST(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 21:
				Pr = HarXiaMicrocell_LOS(Pt, Gt, Gr, tZ, rZ, dist);
				break;
			case 22:
				Pr = HowardXia_UniformRoofTop(Pt, Gt, Gr, tZ, rZ, dist, hbd, w, d);
				break;
			case 23:
				Pr = HowardXia_BSAntennaAboveRoofTopLevel(Pt, Gt, Gr, tZ, rZ, dist, hbd, w, d);
				break;
			case 24:
				Pr = HowardXia_BSAntennaBelowRoofTopLevel(Pt, Gt, Gr, tZ, rZ, dist, hbd, w, d);
				break;
			case 25:
				Pr = Indoor_2_4G_LOS_80211_a_b(Pt, Gt, Gr, dist);
				break;
			case 26:
				Pr = Indoor_5_3G_LOS_80211_a_b(Pt, Gt, Gr, dist);
				break;
			case 27:
				Pr = Indoor_2_4G_NLOS_80211_a_b(Pt, Gt, Gr, dist);
				break;
			case 28:
				Pr = Indoor_5_3G_NLOS_80211_a_b(Pt, Gt, Gr, dist);
				break;
			case 29:
				Pr = Bertoni_Walfisch_Urban(Pt, Gt, Gr, tZ, rZ, dist, hbd, d);
				break;
			case 30:
				Pr = Egli_Urban_Rural(Pt, Gt, Gr, tZ, rZ, dist); 
				break;
			case 31:
				Pr = COST_231_Hata_3GPP_TR_25966_V8_Urban(Pt, Gt, Gr, tZ, rZ, dist); 
				break;
			case 32:
				Pr = COST_231_Hata_3GPP_TR_25966_V8_Suburban(Pt, Gt, Gr, tZ, rZ, dist); 
				break;
			default:
				fprintf(stderr, "[%s] Warning:: No such channel model.\n", __func__);
				Pr = TRG_FS(Ptmw, Gt, Gr, tZ, rZ, L, dist);
				if(fadingOpt == 1){
					Pr = RayleighFading(Pr, fv);
				}
				else if(fadingOpt == 2){
					Pr = RiceanFading(Pr, txNid, rxNid);
				}
		}
#if CSRANGE_DEBUG
		fprintf(stderr, "CSThresh %f, Dist %f, Pr %f, \n", CSThresh, dist, Pr);
#endif
		dist++;
	}
	dist = dist - 1;
#if CSRANGE_DEBUG
	fprintf(stderr, "[%s]:: Carrier Sense Range = %f\n", __func__, dist);
#endif
	return dist;
}

/* dump all channel model name into file */
/*
 * This API is used for GUI to dump channel model list 
 */
void
Dump_CM_List(char *fname){

	FILE *dumpCM = fopen(fname, "w+");
	assert(dumpCM);

	fprintf(dumpCM, "[Theoretical Path Loss Model]::PropModel\n");
	fprintf(dumpCM, "0: Free_Space\n");
	fprintf(dumpCM, "1: Two_Ray_Ground\n");
	fprintf(dumpCM, "2: Free_Space_and_Shadowing\n");

	fprintf(dumpCM, "[Theoretical Fading Model]::FadingOpt\n");
	fprintf(dumpCM, "0: None\n");
	fprintf(dumpCM, "1: Rayleigh\n");
	fprintf(dumpCM, "2: Ricean\n");

	fprintf(dumpCM, "[Empirical Model]::PropModel\n");
	fprintf(dumpCM, "3: LEE_Microcell\n");
	fprintf(dumpCM, "4: Okumura_Hata_VHF_UHF\n");
	fprintf(dumpCM, "5: Okumura_Hata_large_urban\n");
	fprintf(dumpCM, "6: Okumura_Hata_medium_urban\n");
	fprintf(dumpCM, "7: Okumura_Hata_suburban\n");
	fprintf(dumpCM, "8: Okumura_Hata_open_areas\n");
	fprintf(dumpCM, "9: COST_231_Hata\n");
	fprintf(dumpCM, "10: Suburban_1_9GHz_TA\n");
	fprintf(dumpCM, "11: Suburban_1_9GHz_TB\n");
	fprintf(dumpCM, "12: Suburban_1_9GHz_TC\n");
	fprintf(dumpCM, "13: M2M_UMTS_LOS\n");
	fprintf(dumpCM, "14: M2M_UMTS_NLOS\n");
	fprintf(dumpCM, "15: ECC33\n");
	fprintf(dumpCM, "16: ECC33_dversion\n");
	fprintf(dumpCM, "17: AdHoc_LOS\n");
	fprintf(dumpCM, "18: HarXiaMicrocell_Low_Rise_NLOS\n");
	fprintf(dumpCM, "19: HarXiaMicrocell_High_Rise_Lateral\n");
	fprintf(dumpCM, "20: HarXiaMicrocell_High_Rise_ST\n");
	fprintf(dumpCM, "21: HarXiaMicrocell_LOS\n");
	fprintf(dumpCM, "22: HowardXia_UniformRoofTop\n");
	fprintf(dumpCM, "23: HowardXia_BSAntennaAboveRoofTopLevel\n");
	fprintf(dumpCM, "24: HowardXia_BSAntennaBelowRoofTopLevel\n");
	fprintf(dumpCM, "25: Indoor_2_4G_LOS_80211_a_b\n");
	fprintf(dumpCM, "26: Indoor_5_3G_LOS_80211_a_b\n");
	fprintf(dumpCM, "27: Indoor_2_4G_NLOS_80211_a_b\n");
	fprintf(dumpCM, "28: Indoor_5_3G_NLOS_80211_a_b\n");
	fprintf(dumpCM, "29: Bertoni_Walfisch_Urban\n");
	fprintf(dumpCM, "30: Egli_Urban_Rural\n");
	fprintf(dumpCM, "31: COST_231_Hata_3GPP_TR_25966_V8_Urban\n");
	fprintf(dumpCM, "32: COST_231_Hata_3GPP_TR_25966_V8_Suburban\n");

	fclose(dumpCM);
}

/*
 * This API is used for GUI to get some properties from channel model.
 *    e.g., gain, carrier sense distance, power threshold, etc.
 *
 * It is called in nctuns.cc
 */
void 
Get_CM_Property(){

	int nb; // n bytes
	char	*msg;
	struct	cmInfo *cminfo;
	struct	CM_request *qcm;
	struct	CM_reply *outcm;
	channelModel *propModel;

	cminfo = (struct cmInfo *)malloc(sizeof(struct cmInfo));
	outcm = (struct CM_reply *)malloc(sizeof(struct CM_reply));
	msg = (char *)malloc(sizeof(struct CM_request));
	assert(cminfo && outcm);

	while(1){
		double	txGainUser[360], rxGainUser[360];	// User defined antenna gain pattern

		// get parameters from stdin
		nb = readn(0, (char *)msg, sizeof(struct CM_request));
		if(nb <= 0){
			free(cminfo);
			free(outcm);
			free(msg);
			//fprintf(stderr, "[%s]: Terminated!!\n", __func__);
			exit(0);
		}

		qcm = (struct CM_request *)msg;
		if(qcm->closeFlag) {
			free(cminfo);
			free(outcm);
			free(msg);
			//fprintf(stderr, "[%s]: Finished!!\n", __func__);
			exit(0);
		}

		propModel = new channelModel((qcm->Freq) * 1e6);

		assert(propModel);

		if((qcm->TxAGPOpt) == 1){
			parseAGPFile((qcm->TxAGPFileName), txGainUser);
		}
		if((qcm->RxAGPOpt) == 1){
			parseAGPFile((qcm->RxAGPFileName), rxGainUser);
		}

		cminfo->PropModel = qcm->PropModel;
		cminfo->txNid = qcm->TxNid;
		cminfo->rxNid = qcm->RxNid;
		cminfo->fadingOpt_ = qcm->FadingOpt;
		cminfo->fv = qcm->FadingVar;
		cminfo->RiceanK = qcm->RiceanK; // db
		cminfo->txAntennaHeight = qcm->TxAntennaHeight;
		cminfo->rxAntennaHeight = qcm->RxAntennaHeight;
		cminfo->nodeDist = qcm->DTR;
		cminfo->L = qcm->SystemLoss;
		cminfo->Pt = pow(10, (qcm->TransPower)/10) * 1e-3; // Watt
		cminfo->Gt = Get_Antenna_Gain(qcm->TxAngle, qcm->TxBeamwidth, qcm->TxAGPOpt, txGainUser);
		cminfo->Gr = Get_Antenna_Gain(qcm->RxAngle, qcm->RxBeamwidth, qcm->RxAGPOpt, rxGainUser);
		cminfo->hbd = qcm->AverageBuildingHeight; // meter
		cminfo->w = qcm->StreetWidth; // meter
		cminfo->d = qcm->AverageBuildingDist; // meter 
		cminfo->pathlossExp_ = qcm->PathLossExponent;
		cminfo->std_db_ = qcm->StandardDeviation;
		cminfo->dist0_ = qcm->CloseInDist;	// meter

		// output the results to stdout
		outcm->TxGain = cminfo->Gt;
		outcm->RxGain = cminfo->Gr;
		outcm->Dist = propModel->Calculate_Carrier_Sense_Dist(cminfo, qcm->CSThresh);
		outcm->CRPT = propModel->Pr(cminfo); // Corresponding Received Power Threshold by DTR

		cminfo->nodeDist = qcm->DIR_;
		outcm->CCSPT = propModel->Pr(cminfo); // Corresponding Carrier Sense Power Threshold by DIR
		writen(1, (char *)outcm, sizeof(struct CM_reply));
		delete propModel;
	}
}

/*
 * Arguments:
 * 	AGPOpt_		antenna gain pattern option, 1 = enable, 0 = disable
 */
double 
Get_Antenna_Gain(int angle_, int beamwidth_, int AGPOpt_, double gainUser_[360]) {

	double gain;	// dimensionless units
	extern double gain60[];
	extern double gain120[];
	if((angle_ >= 360) || (angle_ < 0)) {
		fprintf(stderr, "Error: wrong antenna angle %d\n", angle_);
		return 1.0;
	}
	if(AGPOpt_ == 0){
		// use default antenna gain pattern
		switch (beamwidth_)
		{
			case 60:
				gain = gain60[angle_];
				break;
			case 120:
				gain = gain120[angle_];
				break;
			case 360:
				return 1.0;
			default:
				fprintf(stderr, "Unsupported beamwidth: %d\n", beamwidth_);
				return 1.0;
		}
	} else {
		// use user defined antenna gain pattern
		gain = gainUser_[angle_];
		//fprintf(stderr, "user defined gain %f\n", gain);
	}

	if((gain < -1000) || (gain > 100)){
		/*
		 * To make sure the gain is in a reasonable value 
		 */
		fprintf(stderr, "Warning: antenna gain may be too small or too big, %lf\n", gain);
	}

	return pow(10, gain/10); // db
}

void
parseAGPFile(char *filename, double *gainUser_){
	FILE		*AGPFile;	// Antenna Gain Pattern File
	int i = 0;

	if((filename == NULL) || (strcmp(filename, "null") == 1)){
		fprintf(stderr, "Error: AGPFileName is null\n");
		exit(0);
	}

	AGPFile = fopen(filename, "r");
	if(AGPFile == NULL){
		fprintf(stderr, "Open %s error\n", filename);
	}
	assert(AGPFile);
	while(!feof(AGPFile)){
		int tmps;
		double tmpGain;
		char line[1024];
		line[0] = '\0';
		fgets(line, 1024, AGPFile);
		if((line[0] == '\0') || (line[0] == '#')) {
			continue;
		}
		if(sscanf(line, "%d,%lf\n", &tmps, &tmpGain) != 2) {
			fprintf(stderr, "Warning: AGP file format error.\n");
			continue;
		}
		gainUser_[i] = tmpGain;
		i++;
	}
	if( i != 360){
		fprintf(stderr, "Warning: antenna gain pattern may be incorrect\n");
	}
	fclose(AGPFile);
}
