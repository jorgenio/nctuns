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

#ifndef __NCTUNS_dvb_s2_feeder_h__
#define __NCTUNS_dvb_s2_feeder_h__


#include "dvb_s2.h"
#include <satellite/dvb_rcs/common/errormodel/saterrormodel.h>
#include <misc/log/logmacro.h>
#include <misc/log/logHeap.h>
#include <satellite/dvb_rcs/fwd/mpeg2_ts_sp.h>

class Dvb_s2_feeder : public Dvb_s2 {

public:
	/*
	 * Constructor and Destructor.
	 */
 	Dvb_s2_feeder(uint32_t type, uint32_t id, struct plist* pl, const char *name);
 	~Dvb_s2_feeder();   

	/*
	 * Public functions.
	 */
	int init();
	int recv(ePacket_*);
	int send(ePacket_*);
	void	TurnOnLinkFailFlag(Event_ *ep);
	void	TurnOffLinkFailFlag(Event_ *ep); 
	uint32_t remain_len();

	/*
	 * The NCC can change the physical layer signal of the feeder via this.
	 */
	class Dvb_s2_pls* dvb_s2_pls()
	{	return _dvb_s2_pls_from_ncc;	}

	inline void set_mpeg2_ts_sp(Mpeg2_ts_sp* ptr)
	{
		_mpeg2_ts_sp = ptr;
	}

private:
	/*
	 * Private variables.
	 */
	class Dvb_s2_pls*	_dvb_s2_pls_from_ncc;
	class Stream_buffer**	_input_buffer;
	class Link*		_link_obj;
	int			_buffer_size;
	uint8_t			_next_input_stream;
	Mpeg2_ts_sp*		_mpeg2_ts_sp;
	/*
	 * Timers.
	 */
	class timerObj*		_tx_timer;

	/*
	 * Private functions.
	 */
	void _init_link();

	int _tx_handler(Event_*);

	uint64_t _prop_delay(class NslObject*);

	int _mode_adaptation(uint8_t, void*, unsigned int);
	int _input_stream_synchroniser();
	int _null_packet_deletion();
	void* _merger_slicer(unsigned int&);

	void _stream_adaptation(void*, unsigned int);
	void _padder(void*, unsigned int, unsigned int);

	void _fec_encoding(void*);
	void _bch_encoder(void*);
	void _ldpc_encoder(void*);

	/*
	 * error calculate model
	 */
	double			_bit_err_rate;
	struct link_budget	_budget;
	struct link_info	_info;

	/*
	 * for log
	 */
	u_char			_ptrlog;

	FILE                    *freqFile;
}; 
  

#endif	/* __NCTUNS_dvb_s2_feeder_h__ */
