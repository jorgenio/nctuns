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

#ifndef __NCTUNS_rcs_mac_h__
#define __NCTUNS_rcs_mac_h__

#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/sch_info.h>
#include <satellite/dvb_rcs/ret/rcs_atm.h>
#include <satellite/dvb_rcs/rcst/rcst_ctl.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/errormodel/saterrormodel.h>
#include <misc/log/logmacro.h>
#include <misc/log/logHeap.h>
#include <mylist.h>
#include <timer.h>

#define MAX_CELL_IN_ATM_BURST	0x04
#define QPSK_BIT_PER_SYMBOL	2
#define	RCS_MAC_SEND_BURST_LOG
#define	RCS_MAC_RECV_BURST_LOG

#define NCTUNS_PKT_COPY(dst, src)		\
{						\
	dst = createEvent();			\
	dst->timeStamp_ = src->timeStamp_;	\
	dst->perio_ = src->perio_;		\
	dst->priority_ = src->priority_;	\
	dst->calloutObj_ = src->calloutObj_;	\
	dst->func_ = src->func_;		\
	dst->DataInfo_ = NULL;			\
	dst->flag = src->flag;			\
}

/*
 * ATM burst struct
 */
struct atm_burst {
	char payload[ATM_CELL_LEN * MAX_CELL_IN_ATM_BURST];
	int cell_cnt;
};

class Rcst_queue_config;
class Rcst_queue_manager;
class NslObject;
struct dvbrcs_node_id;

class Rcs_mac : public NslObject {

protected:

	/*
	 * data structure and enumeration definition
	 * enum => num_atm_cell_burst, pkt_timer_state
	 * struct => pkt_timer_buf, atm_burst
	 */
	/*
	 * packet traffic flow direction enumeration
	 */
	enum num_atm_cell_burst {
		ONE	= 0x0001,	/* one cell in each burst */
		TWO	= 0x0002,	/* two cell in each burst */
		FOUR	= 0x0004	/* four cell in each burst */
	};

	/*
	 * packet timer state
	 */
	enum pkt_timer_state {
		IDLE	= 0x0000,	/* idle state, this state may not be used */
		SEND	= 0x0001,	/* send state */
		RECV	= 0x0002,	/* recv state */
		COLL	= 0x0004	/* coll state */
	};

	/*
	 * Packet Timer struct
	 */
	struct pkt_timer_buf {
		CIRCLEQ_ENTRY(pkt_timer_buf) list;

		pkt_timer_state	state;
		timerObj	timer;
		ePacket_	*pkt;
	};

	/*
	 * RCST control module and queue config object pointer
	 */
	Rcst_ctl		*_rcst_obj;
	Rcst_queue_config	*_rcst_config;
	Ncc_config		*_ncc_config;

	/*
	 * RCST queue manager object pointer
	 */
	Rcst_queue_manager	*_rcst_qm;

	/*
	 * Parameters from tcl
	 */
	u_char			_mac[9];	/* mac address */

	/*
	 * for tx timer buffer, at the same time just can send one
	 * packet.
	 */
	Dvb_pkt			*_tx_pkt_buf;

	timerObj		_tx_timer;
	timerObj		_wait_timer;

	/*
	 * for rx timer buffer linked list
	 */
	CIRCLEQ_HEAD(circleq_head, pkt_timer_buf) _rx_ptbuf_head;

	/*
	 * get ncc_ctl module pointer
	 */
	dvbrcs_node_id		_node_id_cfg;
	NslObject       	*ncc_module;


protected:
	int			_recv_burst(ePacket_ * event);

	/*
	 * encapsulation data burst and schedule waiting timer
	 */
	struct atm_burst	*_encapsulation_atm_burst(struct atm_cell *atm_pkt, int count);

	struct pkt_timer_buf	*_check_state(Dvb_pkt *pkt);

	/*
	 * free circleq function and compute how many cells in the linked list
	 */
	int			_compute_cell_cnt(struct atm_cell *atm_head);
	void			_free_atm_cell_buf(struct atm_cell *head);
	void			_free_circleq_buf(struct circleq_head *head);
	int			_parse_nodeid_cfg(char *path, uint32_t &ncc_nodeid);

public:
	/*
	 * Constructor and Destructor.
	 */
 	Rcs_mac(u_int32_t type, u_int32_t id, struct plist* pl, const char *name);
 	virtual ~Rcs_mac();   

	/*
	 * Public functions.
	 */
	uint64_t	calculate_tx_time(u_int32_t bitlen, double symbol_rate);
	int		schedule_wait_timer();
	virtual int	init();
	virtual int	send(ePacket_ *pkt);
	virtual int	recv(ePacket_ *pkt);
	int	clear_buffer();

	/*
	 * Timer handler
	 */
	int	txHandler();
	int	rxHandler();
	int	send_burst();

	/*
	 * For log
	 */
	u_char	_ptrlog;
	u_int64_t _tx_delay(int len, int bw);

}; 
  

#endif	/* __NCTUNS_rcs_mac_h__ */
