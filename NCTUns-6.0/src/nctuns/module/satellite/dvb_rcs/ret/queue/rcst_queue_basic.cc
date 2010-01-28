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
#include <satellite/dvb_rcs/common/atm_cell.h>
#include "rcst_queue_basic.h"


/*
 * Constructor
 */
Rcst_queue_basic::Rcst_queue_basic(uint32_t queue_id, rcst_queue_type type, uint16_t priority, uint32_t qlen, Ncc_config* config)
: _next_queue(NULL), _queue_len(0), _next_demand_cell(0), _demand_head(NULL), _demand_tail(NULL)
{
	_queue_id = queue_id;
	_queue_type = type;
	_queue_priority = priority;
	_max_queue_len = qlen;
	_ncc_config = config;
	_atm_cell_per_slot = config->num_of_atm_per_slot;

	/*
	 * initialize all veriable
	 */
	_buffer_head.buf_pkt = NULL;

	memset(&_timeslot, 0, sizeof(_timeslot));

}


/*
 * Destructor
 */
Rcst_queue_basic::~Rcst_queue_basic()
{
	/*
	 * data queue
	 */
	if (_queue_type != CTRL)
	{
		_free_atm_buf();
	}
	/*
	 * control queue
	 */
	else
		_free_pkt_buf(_buffer_head.buf_pkt);

	_free_demand_list(_demand_head);
}


uint32_t	
Rcst_queue_basic::_next_demand_slot()
{
	return (_next_demand_cell / _atm_cell_per_slot);
}

/*
 * push buffer Dvb_pkt into this queue, this function will handle all data
 * queue mechanism of pushing queue, control queue must re-write it self.
 */
int Rcst_queue_basic::push(Dvb_pkt *pkt)
{
	struct atm_cell *atm_pkt;

	/*
	 * for all data, it will be atm cell linked list.
	 * So that we must count how many cell in this linked list.
	 */
	atm_pkt = (struct atm_cell *)pkt->pkt_detach();
	delete pkt;

	const uint32_t num = _compute_cell_cnt(atm_pkt);

	/*
	 * buffer queue has enough space, then attach atm cell linked
	 * list to it
	 */
	if (_queue_len + num <= _max_queue_len) {
		_set_qlen(_queue_len + num);

		_next_demand_cell += num;

		atm_buffer* atm_buf = new atm_buffer();

		atm_buf->atm_list = atm_pkt;

		atm_buf->frame_in_which_buffer_created = ((GetCurrentTime()>0) ? 
							  _ncc_config->current_frame() :
							  Frame_ident(0,0));
		atm_buf->next = NULL;

		if (_buffer_head.buf_atm == NULL) //buffer list is empty.
		{
			_buffer_head.buf_atm = _buffer_tail.buf_atm = atm_buf;
		}
		else
		{
			if (_buffer_tail.buf_atm->frame_in_which_buffer_created==_ncc_config->current_frame())
			{
				// Merge with the last buffer.
				_concatenate_atm_list(_buffer_tail.buf_atm->atm_list, atm_pkt);
				delete (atm_buf);
			}
			else
			{
				// Attach a new buffer at the tail.
				_buffer_tail.buf_atm->next = atm_buf;
				_buffer_tail.buf_atm = atm_buf;
			}
		}
		return (0);
	}
	/*
	 * falt to send this packet, then free all tmp atm_cell linked
	 * list
	 */
	else {
		_free_atm_cell_list(atm_pkt);
		return (1);
	}
}


/*
 * shift first element of this queue, this function will handle all data queue
 * mechanism of shiftig queue, control queue must re-write it self.
 *
 * if this queue is data queue, return struct atm_cell pointer
 * else return class Dvb_pkt pointer
 */
void *Rcst_queue_basic::shift()
{
	/*
	 * queue is empty
	 */
	if (_queue_len == 0) {
		assert(_buffer_head.buf_pkt == NULL);
		return (NULL);
	}

	/*
	 * data queue, return struct atm_cell pointer
	 */
	
	_set_qlen(_queue_len-1);
	if (_queue_type != CTRL) 
	{
		struct atm_cell *atm_pkt;

		// Draw one ATM cell out of the head.
		atm_pkt = _buffer_head.buf_atm->atm_list;
		// Let head points to the next ATM cell.
		_buffer_head.buf_atm->atm_list = atm_pkt->next;
		atm_pkt->next = NULL;
		if (_buffer_head.buf_atm->atm_list == NULL)
		{
			//It's a empty buffer now. We have to remove it.
			delete (_detach_buffer_head());
		}
		return (void *)atm_pkt;
	}
	/*
	 * control queue, return class Dvb_pkt pointer
	 */
	else 
	{
		pkt_buffer *pkt_buf;
		Dvb_pkt *pkt;

		pkt_buf = _buffer_head.buf_pkt;
		_buffer_head.buf_pkt = _buffer_head.buf_pkt->next;

		/*
		 * return Dvb_pkt pointer and free pkt buffer
		 */
		pkt = pkt_buf->pkt;
		delete pkt_buf;

		return (void *)pkt;
	}
	return (NULL);
}


/*
 * compute demand timeslot to NCC, this function should be implemented at
 * inherited class
 */
uint32_t Rcst_queue_basic::compute_demand(rcst_queue_demand_type type, uint32_t amount, 
					  Frame_ident frame, double frame_period, uint32_t sml)
{
	return (0);
}


/*
 * clean all demand linked list of this queue
 */
void Rcst_queue_basic::clean_demand()
{
	_free_demand_list(_demand_head);
}


/*
 * clean all data of this queue
 */
void Rcst_queue_basic::clean_queue()
{
	/*
	 * data queue
	 */
	if (_queue_type != CTRL)
	{
		_free_atm_buf();
	}
	/*
	 * control queue
	 */
	else
		_free_pkt_buf(_buffer_head.buf_pkt);

	_buffer_head.buf_pkt = _buffer_tail.buf_pkt = NULL;
	_queue_len = 0;
}


/*
 * grant a number of acceptable timeslot to this queue
 */
int Rcst_queue_basic::grant_demand(const uint32_t amount, const uint16_t superframe_count, 
				   const uint8_t frame_number)
{
	return (0);
}


/*
 * get the latest timeslot information
 */
struct slot_info *Rcst_queue_basic::get_timeslot()
{
	return &_timeslot;
}


/************************************************************
  if the pkt_demand to append is submitted in the same frame
  as previous pkt_demand, the two demand are merged into one.
************************************************************/
void Rcst_queue_basic::_append_pkt_demand(struct pkt_demand *demand)
{
	assert(demand);

	demand->next = NULL;

	if (!_demand_head) {
		_demand_head = _demand_tail = demand;
	}
	else {
		assert(_demand_tail);
		// Append directly.
		_demand_tail->next = demand;
		_demand_tail = demand;
	}
}

/*
 * The instance pointed by _demand_head will be removed from linked list,
 * Return:
 * 	NULL	-->	no instance exist in linked list.
 * 	Non-NULL-->	Success.
 */
Rcst_queue_basic::pkt_demand* 
Rcst_queue_basic::_detach_demand_head()
{
	if(_demand_head)
	{
		pkt_demand	*p_demand = _demand_head;

		if(_demand_tail==_demand_head)	//only one instance.
		{
			_demand_tail = _demand_head = NULL;
		}
		else
		{
			_demand_head = _demand_head->next;
		}

		return (p_demand);
	}
	else // No instance.
	{
		return (NULL);
	}
}

/*
 * The instance pointed by 'buf' will be removed from linked list,
 * Return:
 * 	NULL	-->	no such instance exist in linked list.
 * 	Non-NULL-->	Success.
 */
Rcst_queue_basic::atm_buffer* 
Rcst_queue_basic::_detach_buffer(struct atm_buffer* buf)
{
	if (_buffer_head.buf_atm)
	{
		if (_buffer_head.buf_atm == buf)
		{
			return (_detach_buffer_head());
		}
		else
		{
			atm_buffer *pre_buf = _buffer_head.buf_atm;
			
			// Let pre_buf point to the buffer preceding to 'buf'.
			while (pre_buf->next && pre_buf->next!=buf)
			       pre_buf = pre_buf->next;

			if (pre_buf->next == buf)
			{
				if (buf == _buffer_tail.buf_atm)
				{
					_buffer_tail.buf_atm = pre_buf;
				}
				pre_buf->next = pre_buf->next->next;
				return (buf);
			}
			else // Not found.
			{
				return (NULL);
			}
		}
	}
	else // No instance in list.
	{
		return (NULL);
	}
}

/*
 * The instance pointed by _buffer_head will be removed from linked list,
 * Return:
 * 	NULL	-->	no instance exist in linked list.
 * 	Non-NULL-->	Success.
 */
Rcst_queue_basic::atm_buffer* 
Rcst_queue_basic::_detach_buffer_head()
{
	if (_buffer_head.buf_atm)
	{
		atm_buffer *p_buf = _buffer_head.buf_atm;
		
		if (_buffer_tail.buf_atm == p_buf) //only one instance.
		{
			_buffer_head.buf_atm = _buffer_tail.buf_atm = NULL;
		}
		else
		{
			_buffer_head.buf_atm = _buffer_head.buf_atm->next;
		}

		return (p_buf);
	}
	else // No instance.
	{
		return (NULL);
	}
}
/*
 * compute number of atm cell linked list
 */
uint32_t
Rcst_queue_basic::_compute_cell_cnt(struct atm_cell *atm_head)
{
	struct atm_cell *atm_pkt;
	uint32_t i = 0;

	atm_pkt = atm_head;
	while (atm_pkt) {
		atm_pkt = atm_pkt->next;
		++i;
	}
	return (i);
}


/*
 * free all entries in atm cell linked list
 */
void Rcst_queue_basic::_free_atm_cell_list(struct atm_cell *head)
{
	struct atm_cell *atm_pkt;

	while (head) {
		atm_pkt = head;
		head = head->next;
		free(atm_pkt);
	}
}

/*
 * free all entries in atm_cell buffer linked list
 */
void Rcst_queue_basic::_free_atm_buf()
{
	while (atm_buffer* atm_buf = _detach_buffer_head())
	{
		_free_atm_cell_list(atm_buf->atm_list);
		delete (atm_buf);
	}
}

/*
 * free all entries in dvb_pkt buffer linked list
 */
void Rcst_queue_basic::_free_pkt_buf(struct pkt_buffer *head)
{
	struct pkt_buffer *buf_pkt;

	while (head) {
		buf_pkt = head;
		head = head->next;

		/*
		 * Must remember delete Dvb_pkt object
		 */
		delete buf_pkt->pkt;
		free(buf_pkt);
	}
}


/*
 * free all entries in atm_cell buffer linked list
 */
void Rcst_queue_basic::_free_demand_list(struct pkt_demand *head)
{
	struct pkt_demand *demand;

	while (head) {
		demand = head;
		head = head->next;
		free(demand);
	}
}

Rcst_queue_basic::pkt_demand*	
Rcst_queue_basic::_search_demand(Frame_ident target_frame)
{
	pkt_demand	*p_demand = _demand_head;

	while (p_demand) 
	{
		if (p_demand->frame==target_frame)
		{
			return p_demand;
		}
		p_demand = p_demand->next;
	}

	// Not found.
	return (NULL);
}

/************************************************************
 * Return the statistic of demands in the demand list.
 ***********************************************************/
uint32_t	
Rcst_queue_basic::demand_list_statistic()
{
	uint32_t	total = 0;

	for (struct pkt_demand* p_demand = _demand_head;
	     p_demand; p_demand = p_demand->next)
	{
		total += p_demand->require_slot;
	}

	return (total);
}

void	
Rcst_queue_basic::_drop_atm_cell(Frame_ident frame, uint32_t amount)
{
	// Search every ATM buffers.
	for (atm_buffer* atm_buf = _buffer_head.buf_atm;
	     atm_buf; atm_buf = atm_buf->next)
	{
		if (atm_buf->frame_in_which_buffer_created == frame)
		{
			const uint32_t atm_cnt = _compute_cell_cnt(atm_buf->atm_list);

			if (atm_cnt==amount) //delete all atm cell within this frame.
			{
				_free_atm_cell_list(atm_buf->atm_list);
				delete (_detach_buffer(atm_buf));
			}
			else if (atm_cnt > amount)
			{
				struct atm_cell* p_cell = atm_buf->atm_list;

				// Let p_cell point to the last ATM cell to be kept.
				const uint32_t to_keep = atm_cnt - amount;

				for (uint32_t i=0; i<to_keep; i++)
					p_cell = p_cell->next;
				
				_free_atm_cell_list(p_cell->next);
				p_cell->next = NULL;
			}
			else
			{
				assert(0);
			}
		}
	}
	
	_set_qlen(_queue_len-amount);
}

/************************************************************
 * Drop ATM cells of given amount in given frame.
 ***********************************************************/
void
Rcst_queue_basic::_drop_atm_cell(uint16_t superframe_count, uint8_t frame_number, uint32_t amount)
{
	_drop_atm_cell(Frame_ident(superframe_count,frame_number), 
		       amount);
}

/************************************************************
 * Drop ATM cells of given amount at head of list.
 ***********************************************************/
void
Rcst_queue_basic::_drop_head(uint32_t amount)
{
	uint32_t	remain = amount;
	for (atm_buffer* atm_buf = _buffer_head.buf_atm; atm_buf && (remain>0); 
	atm_buf = atm_buf->next)
	{
		const uint32_t atm_cnt = _compute_cell_cnt(atm_buf->atm_list);

		if (atm_cnt<=remain ) //delete all atm cell within this frame.
		{
			_free_atm_cell_list(atm_buf->atm_list);
			delete (_detach_buffer(atm_buf));
			remain -= atm_cnt;
		}
		else 
		{
			struct atm_cell* p_cell = atm_buf->atm_list;

			for (uint32_t i=0; i<remain; i++)
			{
				atm_cell *tmp = p_cell;
				assert(p_cell = p_cell->next);
				delete (tmp);
			}

			remain = 0;
			atm_buf->atm_list = p_cell;
		}
	}
	if (remain != 0)
		assert (0);

	_set_qlen(_queue_len-amount);
}

/************************************************************
 * Drop ATM cells of given amount at tail of list.
 ***********************************************************/
void
Rcst_queue_basic::_drop_tail(uint32_t amount)
{
}

void	
Rcst_queue_basic::_concatenate_atm_list(struct atm_cell *head, struct atm_cell *p_cell)
{
	assert (head && p_cell);

	atm_cell* last;

	for (last = head; last->next; last = last->next) ;

	last->next = p_cell;
}
void	
Rcst_queue_basic::_print_buffer_list()
{
	printf("***************** buffer list of queue (%u)************************\n", _queue_id);
	for (struct atm_buffer* p_buf = _buffer_head.buf_atm;
	     p_buf;
	     p_buf = p_buf->next)
	{
		printf("%d ATM cells @ (%hu,%hhu)\n", 
		       _compute_cell_cnt(p_buf->atm_list),
		       p_buf->frame_in_which_buffer_created.superframe_count,
		       p_buf->frame_in_which_buffer_created.frame_number);
	}
	printf("******************************************************\n");
}

void
Rcst_queue_basic::_set_qlen(uint32_t len)
{
	_queue_len = len;
}
