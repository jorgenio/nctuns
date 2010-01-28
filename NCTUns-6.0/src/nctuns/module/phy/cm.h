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

#ifndef __NCTUNS_cm_h__
#define __NCTUNS_cm_h__

#include <object.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include <phy/modulation/BPSK.h>
#include <phy/modulation/CCK.h>
#include <phy/power/channel_Model.h>

 /*=======================================================================
   Define Channel Model class
  =======================================================================*/
class cm : public NslObject {
//            |    |		        |    |
//            |    |			|    |
//            |____|			|____|
//                  -----stree width----
//               ----building's distance---
//                            
 private :
	channelModel		*propModel;	// propagation channel model

	int		*uGPRS;
	int		 uGPRS_;		// index of using gprs

	char		*AGPFileName;	// user defined antenna gain pattern file name
	char		*propChannelMode;	// Propagation Channel Mode
	char		*PLModel;	// Theoretical Path Loss Channel Model
	char		*fadingModel;	// fading model
	char		*empiricalModel;// Empirical Channel Model

	int		*beamwidth, beamwidth_;	// beamwidth
	int		AGPOpt;		// user defined antenna gain pattern option (enable/disable)
	int		PropModel;	// index of the propagation model
	int		fadingOpt;	// index of the fading model
	double		gainUser[360];	// user defined antenna gain pattern
	double 		L_; 		// system loss factor
	double		FadingVar;	// fading variable
	double		RiceanK;	// Ricean K factor (db)
	double		averageHB;	// average heights of surrounding buildings (m)
	double		streetWidth;	// street width (m)
	double		averageDist;	// average separation distance between the rows of buildings (m)
	double		antennaHeight;	// antennaHeight (m)

	/* For shadowing model */
	double		PLExp_;		// path-loss exponent
	double		std_db_;	// shadowing standard deviation (dB)
	double		dist0_;		// close-in reference distance (m)

	double		*pointingDirection, *angularSpeed; // antenna direction and speed
	double		pointingDirection_, angularSpeed_; // antenna direction and speed
	double		*freq, freq_;		// frequency (MHz)

	FILE		*obstacleFile;

 public :
   
	cm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~cm();
	
	int		init();
	int		recv(ePacket_ *pkt);
	int	 	send(ePacket_ *pkt);
	int		command(int argc, const char *argv[]);

	/* compute the power loss on channel model.
	 * 		M.S.Hsu  2008.3.4
	 */
	double	 	computePr(struct wphyInfo *wphyinfo);
	double		getAntennaGain(u_int32_t, u_int32_t, int ,double, int, double[]);

};  

#endif /* __NCTUNS_cm_h__ */
