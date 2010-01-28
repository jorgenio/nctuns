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

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <nctuns_api.h>
#include <packet.h>
#include "dvb_pkt.h"


/*
 * Constructor
 */
Dvb_pkt::Dvb_pkt()
: _flag(FLOW_SEND)
, _pbuf(NULL)
{
}

/*
 * Constructor for convert from Packet struct
 */
Dvb_pkt::Dvb_pkt(Packet* pkt)
: _pbuf(NULL)
{
	convert_from_nctuns_pkt(pkt);
}

/*
 * Destructor
 */
Dvb_pkt::~Dvb_pkt()
{
	_free_pbuf();
}


/*
 * Set/Get flow direction flag.
 */
void
Dvb_pkt::pkt_setflag(flow_direct flag)
{
	_flag = flag;
}

enum flow_direct
Dvb_pkt::pkt_getflag()
{
	return (flow_direct)_flag;
};

/*
 * Set/Get data payload type.
 */
bool
Dvb_pkt::pkt_settype(packet_type type)
{
	if (!_pbuf)
		return false;

	_pbuf->type = type;

	return true;
}

enum packet_type
Dvb_pkt::pkt_gettype()
{
	return _pbuf ? _pbuf->type : PKT_NONE;
};

/*
 * Get data & data length
 */
void*
Dvb_pkt::pkt_getdata()
{
	return _pbuf ? _pbuf->data : NULL;
};
int
Dvb_pkt::pkt_getlen()
{
	return _pbuf ? _pbuf->data_len : 0;
};

/*
 * Get information structure.
 */
struct return_link_info*
Dvb_pkt::pkt_getretinfo()
{
	return &_info.ret_info;
};
struct forward_link_info*
Dvb_pkt::pkt_getfwdinfo()
{
	return &_info.fwd_info;
};

/*
 * free _pbuf data memory
 */
void Dvb_pkt::_free_pbuf()
{
	if (!_pbuf) return;

	/*
	 * if _pbuf just only be referenced by me, then free it 
	 */
	if (_pbuf->refcnt == 1) {
		if (_pbuf->data)
			free(_pbuf->data);
		free(_pbuf);
	}
	/*
	 * else, more then one to be referenced it, then decrease the count 
	 */
	else
		_pbuf->refcnt--;

	_pbuf = NULL;
}

/*
 * copy pkt, and new packet will share the same pbuf with original packet
 */
Dvb_pkt *Dvb_pkt::pkt_copy()
{
	Dvb_pkt *pkt_ = new Dvb_pkt;

	pkt_->_flag = _flag;
	pkt_->_pbuf = _pbuf;
	if (_pbuf) _pbuf->refcnt++;

	pkt_->_info = _info;

	return pkt_;
}

/*
 * attach *padload to _pbuf->data. If _pbuf is not NULL, it will free
 * it first.
 */
int Dvb_pkt::pkt_attach(void *payload, const int len)
{
	struct pbuf *ptr;

	if ((ptr = (struct pbuf *)malloc(sizeof (struct pbuf))) == NULL)
		return (-1);
	ptr->refcnt = 1;
	ptr->data_len = len;
	ptr->data = payload;
	if (_pbuf)
		ptr->type = _pbuf->type;
	else
		ptr->type = PKT_RAWDATA;

	/*
	 * free _pbuf, then re-pointer share data 
	 */
	_free_pbuf();
	_pbuf = ptr;

	return (0);
}

/*
 * memory allocation a space to attach to _data, _pbuf must be NULL,
 * or return -1
 */
void *Dvb_pkt::pkt_sattach(const int len)
{
	/* _pbuf is not empty, not allow to allocation new memory to _pbuf */
	if (_pbuf || (_pbuf = (struct pbuf *)malloc(sizeof (struct pbuf))) == NULL)
		return NULL;

	if ((_pbuf->data = malloc(len)) == NULL) {
		_free_pbuf();
		return NULL;
	}
	_pbuf->refcnt = 1;
	_pbuf->data_len = len;
	_pbuf->type = PKT_RAWDATA;

	return _pbuf->data;
}

/*
 * detach _data and return the pointer, but not release pbuf->data
 */
void *Dvb_pkt::pkt_detach()
{
	void *ptr = NULL;

	/*
	 * if _pbuf is NULL, then return NULL
	 */
	if (!_pbuf) return NULL;

	/*
	 * if _pbuf just be referenced once, then free _pbuf and return data
	 * pointer
	 */
	if (_pbuf->refcnt == 1) {
		ptr = _pbuf->data;
		free(_pbuf);
	}
	/*
	 * if _pbuf be referenced more then one packets, then copy
	 * this memory again and return it
	 */
	else {
		if (_pbuf->data > 0) {
			ptr = malloc(_pbuf->data_len);
			memcpy(ptr, _pbuf->data, _pbuf->data_len);
		}
		_pbuf->refcnt--;
	}

	_pbuf = NULL;

	return ptr;
}

/*
 * convert NCTUns Packet to Dvb_pkt class, if payload buffer is not
 * empty, it will detach it first.
 */
int Dvb_pkt::convert_from_nctuns_pkt(Packet *pkt)
{
	void *pkt_payload, *dvb_payload;

	/*
	 * get payload pointer from packet struct 
	 */
	assert(pkt && (pkt_payload = pkt->pkt_sget()));

	/*
	 * allocation new memory for payload, because pkt->release will 
	 * free pt_sdata 
	 */
	assert(dvb_payload = pkt_sattach(pkt->pkt_getlen()));

	/*
	 * copy payload from Packet 
	 */
	memcpy(dvb_payload, pkt_payload, pkt->pkt_getlen());

	/*
	 * setting initialize flag 
	 */
	if (pkt->pkt_getflags() & PF_SEND)
		_flag = FLOW_SEND;
	else
		_flag = FLOW_RECV;

	return (0);
}

/*
 * convert Dvb_pkt to NCTUns Packet class
 */
Packet *Dvb_pkt::convert_to_nctuns_pkt()
{
	Packet *pkt;
	void *buf;

	if (!_pbuf || _pbuf->data_len <= 0)
		return NULL;

 	pkt = new Packet();
	buf = pkt->pkt_sattach(MAXPACKETSIZE);

	/*
	 * _pbuf->data_len must less than or equal MAXPACKETSIZE 
	 */
	assert(MAXPACKETSIZE >= _pbuf->data_len);

	/*
	 * copy memory from _pbuf to pkt->sget, and prepend payload length 
	 */
	assert(_pbuf->data);

	memcpy(buf, _pbuf->data, _pbuf->data_len);
	pkt->pkt_sprepend((char *)buf, _pbuf->data_len);

	if (_flag == FLOW_SEND)
		pkt->pkt_setflow(PF_SEND);
	else
		pkt->pkt_setflow(PF_RECV);

	return pkt;
}
