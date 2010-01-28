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

#include "tbtp_table.h"
#include <assert.h>


// Constructor
Tbtp_timeslot_info_entry::Tbtp_timeslot_info_entry(Tbtp_timeslot_info info)
{
	timeslot_info = info;
}

int	Tbtp_timeslot_info_entry::timeslot_info_len()
{
	return (this->get_multiple_channels_flag() + TBTP_TIMESLOT_INFO_SIZE + 2);
}

void Tbtp_timeslot_info_circleq::free() {

	Tbtp_timeslot_info_entry* entry;

	while(cqh_first != ((Tbtp_timeslot_info_entry*) this))
	{	
		entry = cqh_first;	
		CIRCLEQ_REMOVE(this, entry, entries);

		delete entry;
	}

}


void Tbtp_timeslot_info_circleq::copy(Tbtp_timeslot_info_circleq* dst,
		Tbtp_timeslot_info_circleq* src) 
{
	Tbtp_timeslot_info_entry		*_dst_ass_info_entry, *_src_ass_info_entry;


	CIRCLEQ_INIT(dst);						

	CIRCLEQ_FOREACH(_src_ass_info_entry, src, entries)
	{
		_dst_ass_info_entry = new Tbtp_timeslot_info_entry(*_src_ass_info_entry);
		CIRCLEQ_INSERT_HEAD(dst, _dst_ass_info_entry, entries);
	}
}


// Constructor
Tbtp_frame_info_entry::Tbtp_frame_info_entry(Tbtp_frame_info s_info)
{
	frame_info = s_info;
	CIRCLEQ_INIT(&tbtp_timeslot_info_circleq);
}

u_int32_t	Tbtp_frame_info_entry::frame_info_len()
{
	Tbtp_timeslot_info_entry	*pttie;
	u_int32_t			frame_info_len;	

	// search every timeslot_info_entry

	frame_info_len = TBTP_FRAME_INFO_SIZE;

	CIRCLEQ_FOREACH(pttie, &tbtp_timeslot_info_circleq, entries)
	{
		frame_info_len += pttie->timeslot_info_len();
	}

	return frame_info_len;
}



void Tbtp_frame_info_circleq::free() {
	Tbtp_frame_info_entry*		_entry;
	Tbtp_timeslot_info_circleq*			_ptr_timeslot_info_q;


	while(this-> cqh_first != ((Tbtp_frame_info_entry*) this))
	{	
		_entry = this-> cqh_first;	

		CIRCLEQ_REMOVE(this, _entry, entries);

		_ptr_timeslot_info_q = &(_entry->tbtp_timeslot_info_circleq);
		_ptr_timeslot_info_q->free();

		delete _entry;
	}														
}

void Tbtp_frame_info_circleq::copy(Tbtp_frame_info_circleq* dst,
		Tbtp_frame_info_circleq* src) 
{
	Tbtp_frame_info_entry		*_dst_frame_info_entry, *_src_frame_info_entry;
	Tbtp_timeslot_info_circleq	*_src_timeslot_info_q, *_dst_timeslot_info_q;


	CIRCLEQ_INIT(dst);						
	CIRCLEQ_FOREACH(_src_frame_info_entry, src, entries)
	{
		// Copy the data except dynamic allocated queue.
		_dst_frame_info_entry = new Tbtp_frame_info_entry(_src_frame_info_entry->frame_info);

		// Copy the queue.
		_src_timeslot_info_q = &(_src_frame_info_entry-> tbtp_timeslot_info_circleq);
		_dst_timeslot_info_q = &(_dst_frame_info_entry-> tbtp_timeslot_info_circleq);
		Tbtp_timeslot_info_circleq::copy(_dst_timeslot_info_q, _src_timeslot_info_q);
				
		CIRCLEQ_INSERT_HEAD(dst, _dst_frame_info_entry, entries);		
	}
}


Tbtp::Tbtp() {
	_table_id = TBTP_TABLE_ID;
	CIRCLEQ_INIT(&_tbtp_frame_info_circleq);
	_frame_loop_count = 0;
}

Tbtp::Tbtp(u_int16_t net_id, u_char version_number, 
		u_char current_next_indicator, u_int16_t superframe_count, u_int8_t group_id)
{
	_table_id = TBTP_TABLE_ID;
	_network_id = net_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
	CIRCLEQ_INIT(&_tbtp_frame_info_circleq);
	_frame_loop_count = 0;
	_superframe_count = superframe_count;
	_group_id = group_id;
}

Tbtp::~Tbtp() {	
	// Free the dynamically allocated queue.
	_tbtp_frame_info_circleq.free();
}

Tbtp*	Tbtp::copy() {
	Tbtp*		clone;

	// Copy all fields but the dynamically allocated queue.
	clone = new Tbtp(*this);

	Tbtp_frame_info_circleq::copy(&(clone-> _tbtp_frame_info_circleq), 
			&(this-> _tbtp_frame_info_circleq));
	
	return clone;
}

int	Tbtp::add_frame_info(Tbtp_frame_info frame_info, 
	Tbtp_timeslot_info_entry timeslot_info_entry)
{
	Tbtp_timeslot_info_circleq		*_ptr_timeslot_info_q;
	Tbtp_frame_info_entry			*ptr_frame_info_entry, *_new_frame_info_entry;
	u_char					_arg_frame_number;
	

	
	// Check if the frame id is already used.	
	_arg_frame_number = frame_info.frame_number;

	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if(_arg_frame_number == ptr_frame_info_entry-> get_frame_number())
			return 1;
	}

	_new_frame_info_entry = new Tbtp_frame_info_entry(frame_info);

	// Attach one node onto the frame info queue.
	CIRCLEQ_INSERT_HEAD(&_tbtp_frame_info_circleq,
			_new_frame_info_entry, entries);

	// Increase the _frame_loop_count field
	_frame_loop_count++;


	_ptr_timeslot_info_q = &(_new_frame_info_entry-> tbtp_timeslot_info_circleq);


	Tbtp_timeslot_info_entry *a = new Tbtp_timeslot_info_entry(timeslot_info_entry);

	// Attach one node onto the timeslot info queue within the frame info.
	CIRCLEQ_INSERT_HEAD(_ptr_timeslot_info_q,
			a, entries);

	// Increase the btp_loop_count field
	_new_frame_info_entry-> set_btp_loop_count(1);
	

	return 0;
}// End of Tbtp::add_frame_info()


int	Tbtp::add_timeslot_info(u_char frame_number, 
		Tbtp_timeslot_info_entry timeslot_info_entry)
{
	Tbtp_timeslot_info_circleq			*_ptr_timeslot_info_q;
	Tbtp_frame_info_entry		*ptr_frame_info_entry;
	bool							_frame_number_exist;
	char							_btp_loop_count;



	// Check if the frame number exists.
	_frame_number_exist = false;

	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if(frame_number == ptr_frame_info_entry-> get_frame_number())
		{
			_frame_number_exist = true;
			break;
		}
	}

	if(!_frame_number_exist){
		printf("\nTbtp: no such frame_number, can't add this timeslot info\n");
		return 1;
	}


	_ptr_timeslot_info_q = &(ptr_frame_info_entry-> tbtp_timeslot_info_circleq);

	Tbtp_timeslot_info_entry *new_timeslot_info_entyr = new Tbtp_timeslot_info_entry(timeslot_info_entry);

	// Attach one node onto the timeslot info queue within the frame info.
	CIRCLEQ_INSERT_HEAD(_ptr_timeslot_info_q,
			new_timeslot_info_entyr, entries);

	// Increase the timeslot_loop_count field.
	_btp_loop_count = ptr_frame_info_entry->get_btp_loop_count();
	_btp_loop_count++;
	ptr_frame_info_entry->set_btp_loop_count(_btp_loop_count);

	return 0;
}// End of Tbtp::add_timeslot_info()

int	Tbtp::remove_frame_info(u_char frame_number)
{
	Tbtp_frame_info_entry				*ptr_frame_info_entry;
	Tbtp_timeslot_info_circleq			*_ptr_timeslot_info_q;

	// Remove frame_info_entry
	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if(frame_number == ptr_frame_info_entry-> get_frame_number())// Supertimeslot info entry is found.
		{
			// Free the timeslot info queue within frame info.
			_ptr_timeslot_info_q = &(ptr_frame_info_entry-> tbtp_timeslot_info_circleq);
			_ptr_timeslot_info_q-> free();

			CIRCLEQ_REMOVE(&_tbtp_frame_info_circleq, ptr_frame_info_entry, entries);

			delete(ptr_frame_info_entry);
			return 0;
		}
	}

	// Such frame info doesn't exist.
	return 1;
}

int	Tbtp::remove_timeslot_info(u_char frame_number, u_int16_t logon_id)
{
	Tbtp_timeslot_info_entry			*_ptr_timeslot_info_entry;
	Tbtp_timeslot_info_circleq			*_ptr_timeslot_info_q;
	Tbtp_frame_info_entry				*ptr_frame_info_entry;
	bool						_frame_number_exist;
	char						_btp_loop_count;


	// Check if the frame id exists.
	_frame_number_exist = false;

	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if(frame_number == ptr_frame_info_entry-> get_frame_number())
		{
			_frame_number_exist = true;
			break;
		}
	}

	if(!_frame_number_exist)	return 1;

	// Get current frame_number frame_info's tbtp_timeslot_info_circleq
	_ptr_timeslot_info_q = &(ptr_frame_info_entry-> tbtp_timeslot_info_circleq);

	// Remove timeslot_info_entry
	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
	{
		if(logon_id == _ptr_timeslot_info_entry->get_logon_id())
		{
			// Detach entry.
			CIRCLEQ_REMOVE(_ptr_timeslot_info_q, _ptr_timeslot_info_entry, entries);
			delete (_ptr_timeslot_info_entry);

			// Decrease the timeslot_loop_count field.
			_btp_loop_count = ptr_frame_info_entry-> get_btp_loop_count();
			_btp_loop_count--;
			ptr_frame_info_entry-> set_btp_loop_count(_btp_loop_count);
			
			return 0;
		}
	}

	// Frame number doesn't not exist.
	return 1;
}

int	Tbtp::get_frame_info(u_char frame_number, Tbtp_frame_info* frame_info_buf)
{
	Tbtp_frame_info_entry				*ptr_frame_info_entry;


	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if(frame_number == ptr_frame_info_entry-> get_frame_number())// Supertimeslot info entry is found.
		{
			// Copy the frame info.
			*frame_info_buf = ptr_frame_info_entry-> frame_info;

			return 0;
		}
	}

	// Such frame info doesn't exist.
	return 1;
}

bool operator<(Tbtp_timeslot_info_entry& e1,
	       Tbtp_timeslot_info_entry& e2)
{
	return (e1.get_start_slot() < e2.get_start_slot());
}

//Note: The output list is ordered by start_slot.
int
Tbtp::get_timeslot_info_list(u_char frame_number,
			     uint16_t logon_id, 
			     uint8_t &channel_id,
			     list<Tbtp_timeslot_info_entry> &info_list)
{
	info_list.clear();

	Tbtp_frame_info_entry	*ptr_frame_info_entry;

	bool found = false;
	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if (frame_number == ptr_frame_info_entry-> get_frame_number())
		{
			// Get current frame_number frame_info's tbtp_timeslot_info_circleq
			const Tbtp_timeslot_info_circleq* const ptr_timeslot_info_q = 
				&(ptr_frame_info_entry-> tbtp_timeslot_info_circleq);

			// Get timeslot_info_entry
			Tbtp_timeslot_info_entry	*ptr_timeslot_info_entry;

			CIRCLEQ_FOREACH(ptr_timeslot_info_entry, ptr_timeslot_info_q, entries)
			{

				if (ptr_timeslot_info_entry == NULL ) assert(0);

				if (logon_id == ptr_timeslot_info_entry->get_logon_id() ) // The timeslot entry is found.
				{
					info_list.push_back(*ptr_timeslot_info_entry);
					info_list.sort();
					found = true;
				}
			}

		}
	}

	return found ? 0 : -1;
}

int
Tbtp::get_timeslot_info(	
		u_char		frame_number,
		u_int16_t 	logon_id,
		u_int8_t 	&channel_id,
		Tbtp_timeslot_info_entry_list	*tbtp_timeslot_info_entry_list,
		char		*no_of_slot_entry)
{

	Tbtp_timeslot_info_entry			*_ptr_timeslot_info_entry;
	Tbtp_timeslot_info_circleq			*_ptr_timeslot_info_q;
	Tbtp_frame_info_entry				*ptr_frame_info_entry;
	Tbtp_timeslot_info_entry_list			*_tbtp_timeslot_info_entry_buf;
	bool						_frame_number_exist;
	char						_request_or_data_slot;


	// Check if the frame id exists.
	_frame_number_exist = false;

	_request_or_data_slot = 0;

	CIRCLEQ_FOREACH(ptr_frame_info_entry, &_tbtp_frame_info_circleq, entries)
	{
		if(frame_number == ptr_frame_info_entry-> get_frame_number())
		{
			_frame_number_exist = true;
			break;
		}
	}

	// Frame number doesn't not exist.
	if(_frame_number_exist == false) return 1;

	// Get current frame_number frame_info's tbtp_timeslot_info_circleq
	_ptr_timeslot_info_q = &(ptr_frame_info_entry-> tbtp_timeslot_info_circleq);
	// Get timeslot_info_entry
	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
	{

		if(_ptr_timeslot_info_entry == NULL ) assert(0);

		if(logon_id == _ptr_timeslot_info_entry->get_logon_id() ) // The timeslot entry is found.
		{
			Tbtp_timeslot_info_entry       *data;
			data = new Tbtp_timeslot_info_entry(*_ptr_timeslot_info_entry);
			_tbtp_timeslot_info_entry_buf = new Tbtp_timeslot_info_entry_list();
			_request_or_data_slot++;
			channel_id = _ptr_timeslot_info_entry->get_channel_id();
		
			_tbtp_timeslot_info_entry_buf->data = data;
			_tbtp_timeslot_info_entry_buf->next = tbtp_timeslot_info_entry_list-> next;
			tbtp_timeslot_info_entry_list->next = _tbtp_timeslot_info_entry_buf;
		}
	}

	if(_request_or_data_slot != 0)
	{
		*no_of_slot_entry = _request_or_data_slot;
		return 0;
	}
	// Frame number doesn't not exist.
	return 1;
}




uint16_t	
Tbtp::get_frame_number()
{
	assert (_tbtp_frame_info_circleq.cqh_first);
	return (_tbtp_frame_info_circleq.cqh_first->frame_info.frame_number);
}
