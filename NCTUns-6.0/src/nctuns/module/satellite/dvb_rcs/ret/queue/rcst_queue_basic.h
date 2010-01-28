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

#ifndef __NCTUNS_rcst_queue_basic_h__
#define __NCTUNS_rcst_queue_basic_h__

#include <stdint.h>
#include <satellite/dvb_rcs/common/ncc_config.h>
#define TWO_KBIT_PER_SEC	2000

class Dvb_pkt;
struct atm_cell;
struct slot_info;

/*
 * Rcst Queue Basic, this class will be inherited by all queue type instance
 */
class Rcst_queue_basic {
friend class Rcst_queue_manager;
public:

	enum rcst_queue_type {
		RT	= 0x08, /* CRA Base */
		VR_RT	= 0x04,
		VR_JT	= 0x02,
		JT	= 0x01,	/* Volumn Base */
		CTRL	= 0x10	/* Control Queue */
	};

	enum rcst_queue_demand_type {
		QD_RATE		= 0x01,
		QD_QUEUE_LEN	= 0x02
	};

private:
	/*
	 * private member
	 */

	/*
	 * the latest timeslot for this queue id, this struct just only for
	 * buffer queue
	 */
	struct slot_info	_timeslot;

	/* for linked list next pointer */
	Rcst_queue_basic	*_next_queue;

protected:
	Ncc_config		*_ncc_config;
	uint32_t		_atm_cell_per_slot;
	/*
	 * struct for pkt buffer
	 */
	struct pkt_buffer {
		Dvb_pkt *pkt;
		struct pkt_buffer *next;
	};

	struct atm_buffer {
		Frame_ident	frame_in_which_buffer_created;
		struct atm_cell *atm_list;
		struct atm_buffer *next;
	};
		
	/*
	 * struct for demand linked list
	 */
	struct pkt_demand {
		Frame_ident	frame; // The (superframe_id, frame_id) pair to which this demand applies.
		uint32_t	require_slot; // Total amount of required timeslot in the specified frame.
		uint64_t	require_ticks; // When the last time require demand in the specified frame.
		struct pkt_demand *next;
	};

	uint32_t		_queue_id;	/* store Aggregate Flow ID */
	rcst_queue_type		_queue_type;	/* queue type */
	uint16_t		_queue_priority;/* queue priority */
	uint32_t		_max_queue_len;	/* max queue length in ATM cell*/
	uint32_t		_queue_len;	/* current queue length in ATM cell*/
	uint32_t		_next_demand_cell;/* next demand length in ATM cell*/
	uint32_t		_next_notice_ctrl_len; /* length of granted demand in ATM cell in next frame*/

	/*
	 * head of Linked list for buffer
	 */
	union buffer_ptr_union {
		struct atm_buffer *buf_atm;
		struct pkt_buffer *buf_pkt;
	} _buffer_head;
	union buffer_ptr_union _buffer_tail;

	/*
	 * head of Linked list for demand
	 */
	struct pkt_demand	*_demand_head;
	struct pkt_demand	*_demand_tail;

private:
	/*
	 * private function
	 */
	uint32_t	_compute_cell_cnt(struct atm_cell *atm_head);
	void		_free_atm_buf();
	void		_free_atm_cell_list(struct atm_cell *head);
	void		_free_pkt_buf(struct pkt_buffer *head);
	void		_free_demand_list(struct pkt_demand *head);
	void		_concatenate_atm_list(struct atm_cell *head, struct atm_cell *p_cell);
	struct atm_buffer* _detach_buffer(struct atm_buffer* buf);

protected:
	/*
	 * protected function
	 */
	uint32_t	_next_demand_slot();
	void	_append_pkt_demand(struct pkt_demand *demand);

	struct pkt_demand* _detach_demand_head();

	struct atm_buffer* _detach_buffer_head();

	void	_drop_atm_cell(uint16_t superframe_count, uint8_t frame_number, uint32_t amount);
	void	_drop_atm_cell(Frame_ident frame, uint32_t amount);
	void	_drop_tail(uint32_t amount);
	void	_drop_head(uint32_t amount);
	void	_set_qlen(uint32_t len);
public:
	/*
	 * public function
	 */
	Rcst_queue_basic(uint32_t queue_id, rcst_queue_type type, uint16_t priority, uint32_t qlen, Ncc_config* config);
	virtual ~Rcst_queue_basic();

	void	_print_buffer_list();
	/*
	 * return this queue ID
	 */
	inline uint32_t	get_queue_id() {
		return _queue_id;
	};

	/*
	 * return this queue type
	 */
	inline rcst_queue_type get_type() {
		return _queue_type;
	};

	/*
	 * return this queue priority
	 */
	inline uint16_t get_priority() {
		return _queue_priority;
	};

	/*
	 * return maximum queue length of this queue
	 */
	inline uint32_t get_max_queue_len() {
		return _max_queue_len;
	};

	/*
	 * return current queue length
	 */
	inline uint32_t get_current_queue_len() {
		return _queue_len;
	};

	/*
	 * return next demand queue length
	 */
	inline uint32_t get_next_demand_queue_len() {
		return _next_demand_cell;
	};

	/*
	 * return next demand queue length
	 */
	inline uint32_t get_next_notice_ctrl_grant_len() {
		uint32_t ret = _next_notice_ctrl_len;

		_next_notice_ctrl_len = 0;
		return ret;
	};

	/*
	 * return the first timeslot of demand require
	 */
	inline uint32_t get_first_require_timeslot() {
		return _demand_head ? _demand_head->require_slot : 0;
	};

	inline uint32_t get_first_require_timeslot(Frame_ident& frame) {
		if (_demand_head)
		{
			frame = _demand_head->frame;
		}
		return _demand_head ? _demand_head->require_slot : 0;
	};

	/*
	 * Linked list set/get function
	 */
	inline void set_next_queue(Rcst_queue_basic *queue) {
		_next_queue = queue;
	};

	inline Rcst_queue_basic *get_next_queue() {
		return _next_queue;
	};

	/*
	 * push packet into buffer queue or shift first packet of buffer
	 * outgoing buffer
	 */
	virtual int	push(Dvb_pkt *pkt);
	virtual void	*shift();
	virtual void	clean_demand();
	virtual void	clean_queue();

	virtual uint32_t	compute_demand(rcst_queue_demand_type type, uint32_t amount, 
					       Frame_ident frame, double frame_period, uint32_t msl);

	virtual int	grant_demand(const uint32_t amount,const uint16_t superframe_count, 
				     const uint8_t frame_number);

	pkt_demand*	_search_demand(Frame_ident target_frame);

	uint32_t	demand_list_statistic();

	virtual struct	slot_info *get_timeslot();
}; 

#endif	/* __NCTUNS_rcst_queue_basic_h__ */
