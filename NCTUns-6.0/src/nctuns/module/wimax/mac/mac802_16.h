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

#ifndef __NCTUNS_mac802_16_h__
#define __NCTUNS_mac802_16_h__

#include <stdint.h>
#include <object.h>
#include <packet.h>

#include "common.h"

// for 5-ms frame
/*
#define SYMBOLS_PER_FRAME	130
#define BITS_PER_SYMBOL		200
#define BITS_PER_FRAME		26000
*/

/*
 *	Spec 10.4
 */
enum well_known_cids {
	initRangingCID  = 0x0000,
	paddingCID      = 0xFFFE,
	broadcastCID    = 0xFFFF
};

class Mac_address;
class Timer_mngr;

class mac802_16:public NslObject {

private:
	Mac_address*	_addr;
	double*		_Ts;
	double*		_PS;
	double*		_Pt;
	double*		_CPratio;
	double*		_PSratio;
	int*		_symbolsPerFrame;

	double*		_frame_duration;
	Timer_mngr*	_timer_mngr;

protected:
	enum {
		csUndef,
		csIPv4,			// 1: Packet, IPv4
		csIPv6,			// 2: Packet, IPv6
		csEthernet,		// 3: Packet, 802.3/Ethernet
		csVLAN,			// 4: Packet, 802.1Q VLAN
		csIPv4overEthernet,	// 5: Packet, IPv4 over 802.3/Ethernet
	};

	u_char		_CSType;
	int		_maxqlen;

	inline double Tb() const { return Ts() / (1 + CPratio()); }
	inline double Ts() const { return *_Ts; }
	inline double PS() const { return *_PS; }
	inline double Pt() const { return *_Pt; }
	inline double CPratio() const { return *_CPratio; }
	inline double PSratio() const { return *_PSratio; }
	inline int symbolsPerFrame() const { return *_symbolsPerFrame; }

	int resetTimerT(int);
	int SendNTFYtoPHY(Pkthdr*);
	int symbolsToPSs(int);
	double symbolsToMicro(int);
	double PSsToMicro(int);
	Packet *asPacket(char*, int);

public:
	explicit mac802_16();
	explicit mac802_16(uint32_t, uint32_t, plist*, const char*);
	virtual ~mac802_16();

	virtual int init();

	inline Mac_address* address() { return _addr; }
	inline double frame_duration() const { return *_frame_duration; }
	inline void set_frame_duration(double duration)
	{ *_frame_duration = duration; }
	inline Timer_mngr* timer_mngr() { return _timer_mngr; }
};


#endif				/* __NCTUNS_mac802_16_h__ */
