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

#ifndef __NCTUNS_cmInfo_h__
#define __NCTUNS_cmInfo_h__

/* channel model information */
struct cmInfo {
//                            _____     _____
//                           |_____|   |_____|
//                            |   |     |   |
//                            |   |     |   |
//                            |___|     |___|
//                                 --w--
//                              -----d-----
//
	int	PropModel;	// index of the propagation channel model
	int	txNid, rxNid;	// tx and rx node id
	int	fadingOpt_;	// index of fading model
	double	fv;		// Rayleigh fading variable
	double	RiceanK;	// Ricean K factor (db) 
	double	txAntennaHeight;// transmitter antenna height (m)
	double	rxAntennaHeight;// receiver antenna height (m)
	double	nodeDist;	// node distance (m)
	double	L;		// system loss  >= 1
	double	Pt;		// transmitter power (dbm)
	double 	Gt;		// transmitter gain (dbi)
	double	Gr;		// receiver gain (dbi)
	double	hbd;		// average height of surrounding buildings (rooftops) (meter)
	double	w;		// street width (m)
	double	d;		// average separation distance between the rows of buildings (m)
	double	pathlossExp_;	// path-loss exponent
	double	std_db_;        // shadowing standard deviation (dB)
	double  dist0_;         // close-in reference distance (m)
}; 
                                                        
#endif /* __NCTUNS_cmInfo_h__ */
