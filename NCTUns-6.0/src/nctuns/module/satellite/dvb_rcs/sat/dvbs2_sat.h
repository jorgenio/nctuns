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

#ifndef __NCTUNS_dvbs2_sat_h__
#define __NCTUNS_dvbs2_sat_h__

#include <stdint.h>
#include <object.h>

#define SPEED_OF_LIGHT          300000000.0 // for propagation delay

class NslObject;

class Dvbs2_sat : public NslObject {

private:
	/*
	 * location
	 */
	double		_loc_x;
	double		_loc_y;
	double		_loc_z;

	/*
	 * for log
	 */
	u_char		_ptrlog;
	double		_bw;

	NslObject	*linkObj;			// SatLink object address

	/*
	 * bir error rate
	 */
	double		_tx_fwd_Power;
	double		_tx_fwd_Freq;
	double		_tx_ret_Power;
	double		_tx_ret_Freq;
	double		_rx_fwd_SatAnteLength;
	double		_rx_fwd_SatAnteEfficient;
	double		_tx_fwd_SatAnteLength;
	double		_tx_fwd_SatAnteEfficient;
	double		_rx_ret_SatAnteLength;
	double		_rx_ret_SatAnteEfficient;
	double		_tx_ret_SatAnteLength;
	double		_tx_ret_SatAnteEfficient;

	double		_bit_err_rate;
        float           _diff_of_Freq;

	/*
	 * for turn on/off link fail flag
	 */
	uint32_t	LinkFailFlag;
	char            *_linkfail;
	char            *linkfailFileName;
	FILE            *linkfailFile;
	int		*tunfd_;
        FILE            *freqFile;

public:
 	Dvbs2_sat(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~Dvbs2_sat();   

	int 		init(); 
	int 		recv(ePacket_ *event); 
	int 		send(ePacket_ *event); 

	u_int64_t	_prop_delay_time(NslObject *obj);
	void		TurnOnLinkFailFlag(Event_ *ep);
	void		TurnOffLinkFailFlag(Event_ *ep); 
}; 

#endif	/* __NCTUNS_dvbs2_sat_h__ */
