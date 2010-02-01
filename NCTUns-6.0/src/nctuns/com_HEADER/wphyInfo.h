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

#ifndef __NCTUNS_wphyInfo_h__
#define __NCTUNS_wphyInfo_h__

/* Wireless-PHY Information */

struct wphyInfo {
	u_int32_t	nid;			/* node id 				*/ 
	u_int32_t	pid;			/* port id				*/ 
	int		channel_;		/* channel number (1, 2, 3...)		*/
	double		freq_;			/* frequency				*/
	double		bw_;			/* bandwidth				*/
	double		BER;			/* bit error rate			*/
	double		TxPr_;			/* transmitter power (watt)		*/ 
	double		RxPr_;			/* received power of receiver (watt)	*/ 
	double		srcX_;			/* transmitter location x		*/
	double		srcY_;			/* transmitter location y		*/
	double		srcZ_;			/* transmitter location z		*/
	double		Pr_; 			/* power in channel model (dbm)		*/
	double		currAzimuthAngle_;	/* azimuth angle of antenna (degree)	*/

	//double		txGain_;		/* transmitter gain (dbi)		*/
	//double		CSRange_;		/* carrier sense range (meter)		*/

	/* the following two parameters are for future use */
	//double	currElevationAngle_;	/* elevation angle of antenna (degree)	*/
	//double	antennaLength_;		/* the length of antenna (m) 		*/
}; 
                                                        
#endif /* __NCTUNS_wphyInfo_h__ */
