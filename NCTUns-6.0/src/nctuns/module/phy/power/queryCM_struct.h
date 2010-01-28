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
 * Note: 
 *   If you modify this file, please make sure 
 *   that you also modify the queryCM_struch.h under src/include/ in GUI code
 */

#ifndef __NCTUNS_QUERY_CM_h__
#define __NCTUNS_QUERY_CM_h__

struct CM_request {
	double	Freq;
	int	TxNid;
	int	RxNid;
	int	TxAngle;
	int	TxBeamwidth;
	int	RxAngle;
	int	RxBeamwidth;
	double	CSThresh;
	int	PropModel;
	int	FadingOpt;
	double	FadingVar;
	double	RiceanK;
	double	TxAntennaHeight;
	double	RxAntennaHeight;
	double	SystemLoss;
	double	TransPower;
	double	AverageBuildingHeight;
	double	StreetWidth;
	double	AverageBuildingDist;
	double	PathLossExponent;
	double	StandardDeviation;
	double	CloseInDist;
	double	DTR;	// Desired Transmission Range of a neighboring node (meter), ex: 250 meter
	double	DIR_;	// Desired Interference Range (meter), ex: 250 meter
	int	TxAGPOpt;	// user defined antenna gain pattern option (enable/disable)
	char	TxAGPFileName[1024];
	int	RxAGPOpt;	// user defined antenna gain pattern option (enable/disable)
	char	RxAGPFileName[1024];
	int	closeFlag;
};

struct CM_reply {
	double TxGain;
	double RxGain;
	double Dist;
	double CRPT;
	double CCSPT;
};

#endif	/* __NCTUNS_QUERY_CM_h__ */
