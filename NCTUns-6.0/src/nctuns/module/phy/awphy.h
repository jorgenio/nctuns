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

#ifndef __NCTUNS_awphy_h__
#define __NCTUNS_awphy_h__

#include <object.h>
#include <event.h>
#include <timer.h>
#include <mylist.h>
#include <phy/modulation/BPSK.h>
#include <phy/modulation/CCK.h>
 
 /*=======================================================================
   Define Advanced WPHY class
  =======================================================================*/
class awphy : public NslObject {

 private :
#ifdef LINUX
	int			*tunfd_;
#endif
	Modulation		*errorModel;

	int			channel_;	// channel number (1, 2, 3...)
	double			CSThresh_; 	// Carrier Sense threshold (dbm)
	double			CPThresh_;	// Capture threshold (dbm)
	double			freq_;		// frequency (MHz)
	double                  bw_;		// bit per second
	double			bw_Mbps;	// megabit per second
	double			TransPower;	// transmitter power (w)
	double			BER;		// bit error rate
	double			Noise;		// Additional Noise
	double			Gain;		// Antenna gain

	/*
	 * Directional Antenna (added by hwchu)
	 *
	 *   For advanced wireless phy,
	 *   (1) antenna gains are looked up in a pre-computed table.
	 *   (2) receivers outside the beamwidth may still receive transmitted
	 *       signal.
	 */
	int			beamwidth;
	double			pointingDirection;
	double			angularSpeed;	// degrees per second

	/* for log file */
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

 public :
   
	awphy(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
	~awphy();
	
	int			init();
	int			recv(ePacket_ *pkt);
	int		 	send(ePacket_ *pkt);
	int			command(int argc, const char *argv[]);

	int			log();
	
	void			TurnOnLinkFailFlag(Event_ *ep);
	void			TurnOffLinkFailFlag(Event_ *ep);       
};  

#endif /* __NCTUNS_awphy_h__ */
