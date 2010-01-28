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

#include <assert.h>
#include <stdlib.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include "rcst_queue_basic.h"
#include "rcst_queue_ctrl.h"

/*
 * Constructor
 */
Rcst_queue_ctrl::Rcst_queue_ctrl(uint32_t qlen, Ncc_config* ncc_config)
: Rcst_queue_basic(0, Rcst_queue_basic::CTRL, 1, qlen, ncc_config)
{
	/*
	 * nothing to do
	 */
}

/*
 * Destructor
 */
Rcst_queue_ctrl::~Rcst_queue_ctrl()
{
	/*
	 * nothing to do
	 */
}


/*
 * push buffer Dvb_pkt into this queue, this function will handle all data
 * queue mechanism of pushing queue, control queue must re-write it self.
 */
int Rcst_queue_ctrl::push(Dvb_pkt *pkt)
{
	/*
	 * buffer queue has enough space, then attach atm cell linked
	 * list to it
	 */
	if (_queue_len + 1 <= this->get_max_queue_len()) {
		struct pkt_buffer *pkt_buf = new struct pkt_buffer;

		_queue_len++;
		pkt_buf->next = NULL;

		if (_buffer_head.buf_pkt == NULL) {
			_buffer_head.buf_pkt = _buffer_tail.buf_pkt = pkt_buf;
			pkt_buf->pkt = pkt;
		}
		else {
			_buffer_tail.buf_pkt->next = pkt_buf;
			_buffer_tail.buf_pkt = pkt_buf;

			pkt_buf->pkt = pkt;
		}
	}
	/*
	 * falt to send this packet, then free all tmp atm_cell linked
	 * list
	 */
	else {
		delete pkt;
		return (1);
	}
	return (0);
}


/*
 * control queue didn't need grant demand timeslot
 */
int Rcst_queue_ctrl::grant_demand(const uint32_t amount, int extra = 0)
{
	return (0);
}


/*
 * control queue will return the timeslot information of the first packet of
 * the queue
 */
struct slot_info *Rcst_queue_ctrl::get_timeslot()
{
	if (_buffer_head.buf_pkt) {
		return &_buffer_head.buf_pkt->pkt->pkt_getretinfo()->timeslot;
	}
	return NULL;
}
