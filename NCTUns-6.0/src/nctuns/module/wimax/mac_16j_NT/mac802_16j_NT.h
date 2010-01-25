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

#ifndef __NCTUNS_80216J_NT_MAC_80216J_NT_H__
#define __NCTUNS_80216J_NT_MAC_80216J_NT_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ethernet.h>

#include <nctuns_api.h>
#include <packet.h>
#include <object.h>

#include "neighbor_bs.h"
#include "mac_address.h"
#include "timer_mngr.h"
#include "common.h"
#include "library.h"

// Spec 16e. 10.4 Table 345. CIDs
enum well_known_cids {
	initRangingCID = 0x0000, paddingCID = 0xFFFE, broadcastCID = 0xFFFF
};

enum support_HO_t {
	HHO = 0x01, MDHO = 0x02, FBSS_HO = 0x04
};

enum ranging_code {
	INITIAL_RANGING,
	PERIOD_RANGING,
	BW_REQUEST,
	HANDOVER_RANGING,
	RS_INITIAL_RANGING,
	RS_DEDICATED_CODES
};

using namespace MR_MacAddress_NT;
using namespace MR_Timer_NT;
using namespace MR_Common_NT;

namespace MR_Mac80216j_NT {
class mac802_16j_NT: public NslObject {
private:
	Mac_address *_addr;
	double *_Ts;
	double *_PS;
	double *_Pt;
	double *_CPratio;
	double *_PSratio;
	int *_symbolsPerFrame;
	Timer_mngr *_timer_mngr;

private:
	int *_DLsubchannels;
	int *_ULsubchannels;
	int *_TTG;
	int *_RTG;
	int *_RTTI;
	int *_RRTI;

protected:
	enum {
		csUndef, csIPv4, // 1: Packet, IPv4
		csIPv6, // 2: Packet, IPv6
		csEthernet, // 3: Packet, 802.3/Ethernet
		csVLAN, // 4: Packet, 802.1Q VLAN
		csIPv4overEthernet
	// 5: Packet, IPv4 over 802.3/Ethernet
	};

	int _maxqlen;
	uint8_t _CSType;
	uint8_t _HO_support;
	uint8_t _initRang_code_db[64][18]; // each 144 bits
	uint8_t _periodRang_code_db[64][18]; // each 144 bits
	uint8_t _bwRequest_code_db[64][18]; // each 144 bits
	uint8_t _handoverRang_code_db[64][18]; // each 144 bits
	uint8_t _rs_initRang_code_db[64][18];
	uint8_t _rs_dedicated_code_db[64][18];

	int S;
	int N;
	int M;
	int L;
	int O;
	int P;
	int Q;

	inline double Tb() const {
		return Ts() / (1 + CPratio());
	}
	inline double Ts() const {
		return *_Ts;
	}
	inline double PS() const {
		return *_PS;
	}
	inline double Pt() const {
		return *_Pt;
	}
	inline double CPratio() const {
		return *_CPratio;
	}
	inline double PSratio() const {
		return *_PSratio;
	}
	inline int symbolsPerFrame() const {
		return *_symbolsPerFrame;
	}
	inline int DLsubchannels() const {
		return *_DLsubchannels;
	}
	inline int ULsubchannels() const {
		return *_ULsubchannels;
	}
	inline int TTG() const {
		return *_TTG;
	}
	inline int RTG() const {
		return *_RTG;
	}
	inline int RTTI() const {
		return *_RTTI;
	}
	inline int RRTI() const {
		return *_RRTI;
	}
	inline char* ipToStr(uint32_t ip) {
		char* str = new char[16];

		sprintf(str, "%d.%d.%d.%d", (uint8_t) (0x000000FF & ip),
				(uint8_t) ((0x0000FF00 & ip) >> 8),
				(uint8_t) ((0x00FF0000 & ip) >> 16), (uint8_t) ((0xFF000000
						& ip) >> 24));
		return str;
	}

	int getCode(int);
	uint8_t getCodeIndex(uint8_t *, uint8_t *);
	int resetTimerT(int);
	int SendNTFYtoPHY(Pkthdr *);
	int symbolsToPSs(int);
	double symbolsToMicro(int);
	double PSsToMicro(int);
	Packet* asPacket(char *, int);

public:
	explicit mac802_16j_NT();
	explicit mac802_16j_NT(uint32_t, uint32_t, plist *, const char *);
	virtual ~mac802_16j_NT();

	virtual int init();

	inline Mac_address* address() {
		return _addr;
	}
	inline Timer_mngr* timer_mngr() {
		return _timer_mngr;
	}
};
}

#endif                /* __NCTUNS_80216J_NT_MAC_80216J_NT_H__ */
