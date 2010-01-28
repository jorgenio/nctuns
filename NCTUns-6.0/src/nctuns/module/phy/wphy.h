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

#ifndef __NCTUNS_wphy_h__
#define __NCTUNS_wphy_h__

#include <object.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include <phy/modulation/BPSK.h>
#include <phy/power/channel_Model.h>
//#include <phy/power/tworayground.h>

#define SPEED_OF_LIGHT		300000000.0
#define Lambda			(SPEED_OF_LIGHT/(914.0 * 1e6)) 

 /*=======================================================================
   Define Wireless-PHY class
  =======================================================================*/
class wphy : public NslObject {

 private :
#ifdef LINUX
	int			*tunfd_;
#endif

	channelModel		*propModel;  
	//TwoRayGround		*propModel;  
	
	double                  bw_;		// bit per second
	double			bw_Mbps;	// megabit per second
	double			freq_; 		// frequency (MHz)
	int			channel_;	// frequency channel number
	double			CPThresh_;	// Capture threshold (db)
	double			CSThresh_; 	// Carrier Sense threshold Iw)
	double			RXThresh_; 	// Receive threshold (w)
	double			TransPower;	// transmission power (w)
	double 			L_; 		// system loss factor
	double			RXRange;
	double			CSRange;
	double			BER;
	double			Gain;		// antenna gain (dbi)
	
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

	/*
	 * Directional Antenna (added by hwchu)
	 *
	 *   For simple wireless phy,
	 *   (1) we do not consider the influence of antenna directivity on
	 *       txGain and rxGain. Gains are always 1.
	 *   (2) receivers outside the beamwidth cannot receive ANY transmitted
	 *       signal.
	 */
	int			beamwidth;
	double			pointingDirection;
	double			angularSpeed;	// degrees per second

 public :
   
	wphy(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~wphy();
	
	int			init();
	int			recv(ePacket_ *pkt);
	int		 	send(ePacket_ *pkt);
	int			command(int argc, const char *argv[]);

	int			log();

	int			BitError(double BER_, int plen);	
	void			TurnOnLinkFailFlag(Event_ *ep);
	void			TurnOffLinkFailFlag(Event_ *ep);       
};  


#endif /* __NCTUNS_wphy_h__ */
