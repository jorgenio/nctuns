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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ethernet.h>

#include <nctuns_api.h>
#include <packet.h>

#include "mac802_16.h"
#include "mac_address.h"
#include "timer_mngr.h"


mac802_16::mac802_16()
: _addr(new Mac_address())
, _timer_mngr(new Timer_mngr())
, _maxqlen(0)
{
}

mac802_16::mac802_16(uint32_t type, uint32_t id, plist* pl, const char* name)
: NslObject(type, id, pl, name)
, _addr(new Mac_address())
, _timer_mngr(new Timer_mngr())
, _maxqlen(0)
{
	vBind_mac("macaddr", _addr->buf());
	vBind("max_qlen", &_maxqlen);
}

mac802_16::~mac802_16()
{
	if (_timer_mngr)
		delete _timer_mngr;

	if (_addr)
		delete _addr;
}

int
mac802_16::init(void)
{
	assert(_maxqlen > 0);

	_Ts = GET_REG_VAR(get_port(), "Ts", double *);
	_PS = GET_REG_VAR(get_port(), "PS", double *);
	_CPratio = GET_REG_VAR(get_port(), "CPratio", double *);
	_PSratio = GET_REG_VAR(get_port(), "PSratio", double *);
	_Pt = GET_REG_VAR(get_port(), "transPower", double *);
	_symbolsPerFrame = GET_REG_VAR(get_port(), "symbolsPerFrame", int *);
	_frame_duration = GET_REG_VAR(get_port(), "frameDuration", double *);

	assert(_Ts && _PS && _Pt && _CPratio && _PSratio
	       && _symbolsPerFrame && _frame_duration);

	return NslObject::init();
}

int mac802_16::resetTimerT(int tno)
{
	_timer_mngr->reset_t(tno);

	return 0;
}

int mac802_16::SendNTFYtoPHY(Pkthdr * cmd)
{

	Event *ep = createEvent();

	cmd->flag = PF_SEND;
	ep->DataInfo_ = reinterpret_cast < void *>(cmd);
	put(ep, sendtarget_);

	return 0;
}

int mac802_16::symbolsToPSs(int nsymbols)
{

	return static_cast < int >(nsymbols * (1 + CPratio()) / PSratio());

}

double mac802_16::symbolsToMicro(int nsymbols)
{

	return nsymbols * Ts();

}

double mac802_16::PSsToMicro(int nPSs)
{

	return nPSs * PS();

}

Packet *mac802_16::asPacket(char *ptr, int pLen)
{

	Packet *Pkt = new Packet();
	char *buf;

	if (_CSType == csIPv4) {

		buf = Pkt->pkt_sattach(pLen);
		memcpy(buf, ptr, pLen);
		Pkt->pkt_sprepend(buf, pLen);

	} 
	else {

		buf = Pkt->pkt_sattach(pLen - sizeof(struct ether_header));
		memcpy(buf, ptr + sizeof(struct ether_header),
		       pLen - sizeof(struct ether_header));
		Pkt->pkt_sprepend(buf, pLen - sizeof(struct ether_header));
		buf = Pkt->pkt_malloc(sizeof(struct ether_header));
		memcpy(buf, ptr, sizeof(struct ether_header));

	}

	return Pkt;
}
