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

#ifndef __NCTUNS_radiolink_h__
#define __NCTUNS_radiolink_h__

#include <object.h>
#include <module/phy/modulation/GMSK.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include <module/phy/modulation/BPSK.h>
#include <module/phy/modulation/CCK.h>
//#include <module/phy/power/tworayground.h>
#include <module/phy/cm.h>

#define SPEED_OF_LIGHT		300000000.0
#define Lambda			(SPEED_OF_LIGHT/(914.0 * 1e6)) 

class timerObj;

 /*=======================================================================
   Define Wireless-PHY class
  =======================================================================*/
class radiolink: public NslObject {

 private :

	Modulation		*errorModel;

	int			uGPRS;		// index of using gprs for channel model
	int                     beamwidth;	// antenna beamwidth
	double                  pointingDirection; // antenna pointing direction
	double                  angularSpeed;   // antenna angular speed (degrees per second)

	double                  bw_;		// bit per second
	double			bw_Mbps;	// megabit per second
	double			freq_;		// frequency band in MHz 
	double			CPThresh_;	// Capture threshold (db)
	double			CSThresh_; 	// Carrier Sense threshold Iw)
	double			TransPower;	// transmission power (w)
	double			BER;
	double			Noise;		// Additional Noise
	//int			CSOpt;
	double		Gain;		// Antenna Gain
	//double		CSRange;
	u_char			channels[256];
	
	char			*_log, *_inOpt, *_outOpt, *_inoutOpt;
	u_char			_in, _out, _inout;
	double			_logInterval;		// millisecond
	u_int64_t		_logIntervalTick;
	timerObj		logTimer;
	double			inKb, outKb, inoutKb;
	char			*logInFileName;
	char			*logOutFileName;
	char			*logInOutFileName;
	FILE			*logInFile;
	FILE			*logOutFile;
	FILE			*logInOutFile;

	char			*_linkfail;
	char			*linkfailFileName;
	FILE			*linkfailFile;
	u_int32_t		LinkFailFlag;

	FILE			*obstacleFile;
 public :
   
	radiolink(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~radiolink();
	
	int			init();
	int			recv(ePacket_ *pkt);
	int		 	send(ePacket_ *pkt);
	int			command(int argc, const char *argv[]);

	int			log();
	
	int 			set_listen_channel(u_char* );
	void			TurnOnLinkFailFlag(Event_ *ep);
	void			TurnOffLinkFailFlag(Event_ *ep);       
};  


#endif /* __NCTUNS_wphy_h__ */
