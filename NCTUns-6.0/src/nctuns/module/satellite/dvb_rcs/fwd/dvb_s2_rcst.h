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

#ifndef __NCTUNS_dvb_s2_rcst_h__
#define __NCTUNS_dvb_s2_rcst_h__


#include "dvb_s2.h"
#include "../common/errormodel/saterrormodel.h"
#include <misc/log/logmacro.h>
#include <misc/log/logHeap.h>


class Dvb_s2_rcst : public Dvb_s2 {

private:
	/*
	 * Enumberations.
	 */
	/*
	 * DVB-S.2 RCST states
	 */
	enum _dvb_s2_states {
		DVB_S2_IDLE	= 0x00,	/* Idle state */
		DVB_S2_RECV	= 0x01,	/* Receive state */
	};

public:
	/*
	 * Constructor and Destructor.
	 */
 	Dvb_s2_rcst(uint32_t type, uint32_t id, struct plist* pl, const char *name);
 	~Dvb_s2_rcst();   

	/*
	 * Public functions.
	 */
	int init();
	int recv(ePacket_*);
	int send(ePacket_*);
	void	TurnOnLinkFailFlag(Event_ *ep);
	void	TurnOffLinkFailFlag(Event_ *ep); 

private:
	/*
	 * Private variables.
	 */
	enum _dvb_s2_states	_state;
	class Stream_buffer**	_incomplete_up_buf;
	class Dvb_pkt*		_recv_buffer;
	uint8_t			_ldpc_iteration_threshold;
	/*
	 * Just for bit rate accounting.
	 */
	double			_byte_recv;
	uint64_t		_prev_tick;

	/*
	 * error calculate model
	 */
	double			_bit_err_rate;
	struct link_budget	_budget;
	struct link_info	_info;

	/*
	 * Timers.
	 */
	class timerObj*		_rx_timer;

	/*
	 * Private functions.
	 */
	int _rx_handler(Event_*);

	void _demultiplexer(void*);
	void _recv_user_packet(uint8_t, void*);

	int _fec_decoding(void*);
	int _bch_decoder(void*);
	int _ldpc_decoder(void*);


	/*
	 * for log
	 */
	u_char			_ptrlog;


}; 
  

#endif	/* __NCTUNS_dvb_s2_rcst_h__ */
