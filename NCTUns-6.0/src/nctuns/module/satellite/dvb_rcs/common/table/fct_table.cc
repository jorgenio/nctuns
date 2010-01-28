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

#include "fct_table.h"
Fct_timeslot_info_entry::Fct_timeslot_info_entry(Fct_timeslot_info info)
{
	timeslot_info = info;
}


void Fct_timeslot_info_circleq::free() {
	Fct_timeslot_info_entry*		_entry;

	while(this-> cqh_first != ((Fct_timeslot_info_entry*) this))
	{	
		_entry = this-> cqh_first;	
		CIRCLEQ_REMOVE(this, _entry, entries);

		delete _entry;
	}
}


void Fct_timeslot_info_circleq::copy(Fct_timeslot_info_circleq* dst,
		Fct_timeslot_info_circleq* src) 
{
	Fct_timeslot_info_entry		*_dst_ass_info_entry, *_src_ass_info_entry;


	CIRCLEQ_INIT(dst);						

	CIRCLEQ_FOREACH(_src_ass_info_entry, src, entries)
	{
		_dst_ass_info_entry = new Fct_timeslot_info_entry(*_src_ass_info_entry);
		CIRCLEQ_INSERT_TAIL(dst, _dst_ass_info_entry, entries);		
	}
}



// Constructor
Fct_frame_info_entry::Fct_frame_info_entry(Fct_frame_info s_info)
{
	frame_info = s_info;
	CIRCLEQ_INIT(&fct_timeslot_info_circleq);
}// End of Fct_superframe_info_entry::Fct_superframe_info_entry()


u_int32_t Fct_frame_info_entry::frame_info_len()
{
	u_int32_t	frame_info_len;	
	

	frame_info_len = FCT_FRAME_INFO_SIZE + (FCT_TIMESLOT_INFO_SIZE * get_timeslot_loop_count());
	return frame_info_len;
}

void Fct_frame_info_circleq::free() {
	Fct_frame_info_entry*		_entry;
	Fct_timeslot_info_circleq*			_ptr_timeslot_info_q;


	while(this-> cqh_first != ((Fct_frame_info_entry*) this))
	{	
		_entry = this-> cqh_first;	

		CIRCLEQ_REMOVE(this, _entry, entries);

		_ptr_timeslot_info_q = &(_entry-> fct_timeslot_info_circleq);
		_ptr_timeslot_info_q-> free();

		delete _entry;
	}														
}

void Fct_frame_info_circleq::copy(Fct_frame_info_circleq* dst,
		Fct_frame_info_circleq* src) 
{
	Fct_frame_info_entry		*_dst_frame_info_entry, *_src_frame_info_entry;
	Fct_timeslot_info_circleq			*_src_timeslot_info_q, *_dst_timeslot_info_q;

	CIRCLEQ_INIT(dst);						
	CIRCLEQ_FOREACH(_src_frame_info_entry, src, entries)
	{
		// Copy the data except dynamic allocated queue.
		_dst_frame_info_entry = new Fct_frame_info_entry(*_src_frame_info_entry);

		// Copy the queue.
		_src_timeslot_info_q = &(_src_frame_info_entry-> fct_timeslot_info_circleq);
		_dst_timeslot_info_q = &(_dst_frame_info_entry-> fct_timeslot_info_circleq);
		Fct_timeslot_info_circleq::copy(_dst_timeslot_info_q, _src_timeslot_info_q);
				
		CIRCLEQ_INSERT_TAIL(dst, _dst_frame_info_entry, entries);		
	}
}


// Constructor
Fct::Fct() {
	_table_id = SCT_TABLE_ID;
	_frame_loop_count = 0;
	CIRCLEQ_INIT(&_fct_frame_info_circleq);
}


Fct::Fct(u_int16_t net_id, u_char version_number, 
		u_char current_next_indicator)
{

	_table_id = FCT_TABLE_ID;
	_network_id = net_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
	_frame_loop_count = 0;
	CIRCLEQ_INIT(&_fct_frame_info_circleq);
}


Fct::~Fct() {
	// Free the dynamically allocated queue.
	_fct_frame_info_circleq.free();
}

Fct* Fct::copy() {
	Fct*		clone;

	// Copy all fields but the dynamically allocated queue.
	clone = new Fct(*this);

	Fct_frame_info_circleq::copy(&(clone-> _fct_frame_info_circleq), 
			&(this-> _fct_frame_info_circleq));
	
	return clone;
}


int	Fct::add_frame_info(Fct_frame_info frame_info, 
	Fct_timeslot_info timeslot_info)
{
	Fct_timeslot_info_entry			*_new_timeslot_info_entry;
	Fct_timeslot_info_circleq		*_ptr_timeslot_info_q;
	Fct_frame_info_entry			*_ptr_frame_info_entry, *_new_frame_info_entry;
	u_char					_arg_frame_id;
	uint16_t				_rep;
	

	// Check if the frame id is already used.	
	_arg_frame_id = frame_info.frame_id;

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		if(_arg_frame_id == _ptr_frame_info_entry-> get_frame_id())
			return (1);
	}

	_new_frame_info_entry = new Fct_frame_info_entry(frame_info);

	_new_frame_info_entry->set_timeslot_loop_count(1);
	_new_frame_info_entry->set_start_timeslot_number(0);
	_rep = timeslot_info.get_repeat_count();
	_new_frame_info_entry->set_total_timeslot_count(1+_rep);
	// Attach one node onto the frame info queue.
	CIRCLEQ_INSERT_TAIL(&_fct_frame_info_circleq,
			_new_frame_info_entry, entries);

	// Create a timeslot info entry.
	_new_timeslot_info_entry = new Fct_timeslot_info_entry(timeslot_info);
	_new_timeslot_info_entry-> timeslot_number = 0;

	_ptr_timeslot_info_q = &(_new_frame_info_entry-> fct_timeslot_info_circleq);

	// Attach one node onto the timeslot info queue within the frame info.
	CIRCLEQ_INSERT_TAIL(_ptr_timeslot_info_q,
			_new_timeslot_info_entry, entries);

	// Increase the frame_loop_count field.
	_frame_loop_count++;



	return 0;
}// End of Sct::add_frame_info()


int	Fct::add_timeslot_info(u_char frame_id, 
			       Fct_timeslot_info timeslot_info, 
			       u_int16_t timeslot_number)
{
	Fct_timeslot_info_entry			*_ptr_timeslot_info_entry, *_new_timeslot_info_entry;
	Fct_timeslot_info_circleq		*_ptr_timeslot_info_q;
	Fct_frame_info_entry			*_ptr_frame_info_entry;
	bool					_frame_id_exist;
	char					_timeslot_loop_count;
	uint16_t				_total_timeslot_count;



	// Check if the frame id exists.
	_frame_id_exist = false;

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		if(frame_id == _ptr_frame_info_entry-> get_frame_id())
		{
			_frame_id_exist = true;
			break;
		}
	}

	if(!_frame_id_exist)	
	{
		return (1);
	}

	// Check if the timeslot number is already used.	
	_ptr_timeslot_info_q = &(_ptr_frame_info_entry-> fct_timeslot_info_circleq);

	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
	{
		int ins_frame_num_st, ins_frame_num_ed;
		int q_frame_num_st, q_frame_num_ed;
		ins_frame_num_st = timeslot_number;
		ins_frame_num_ed = timeslot_number+timeslot_info.get_repeat_count();
		q_frame_num_st = _ptr_timeslot_info_entry->get_timeslot_number();
		q_frame_num_ed = _ptr_timeslot_info_entry->get_timeslot_number()+_ptr_timeslot_info_entry->get_repeat_count();
		if( (ins_frame_num_st<=q_frame_num_st&&q_frame_num_st<=ins_frame_num_ed) ||
			(ins_frame_num_st<=q_frame_num_ed&&q_frame_num_ed<=ins_frame_num_ed))
			{
				printf("\n@@@@@@@@@@@@@@@@@@ insert timeslot frame number error @@@@@@@@@@@@@@@@@@@\n");
				return 1;
			}
	}

	// Create a timeslot info entry.
	_new_timeslot_info_entry = new Fct_timeslot_info_entry(timeslot_info);
	_new_timeslot_info_entry-> timeslot_number = timeslot_number;

	// use insert sort to insert timeslot_info to 'correct' location
	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
	{
		// if _ptr_timeslot_info_q is empty, then just insert
		if(timeslot_number < _ptr_timeslot_info_entry->get_timeslot_number()){
			CIRCLEQ_INSERT_BEFORE(_ptr_timeslot_info_q,_ptr_timeslot_info_entry,
			_new_timeslot_info_entry, entries);
			break;
		}
		else {

			if(CIRCLEQ_NEXT(_ptr_timeslot_info_entry, entries) == (Fct_timeslot_info_entry*)_ptr_timeslot_info_q){
			CIRCLEQ_INSERT_AFTER(_ptr_timeslot_info_q,_ptr_timeslot_info_entry, _new_timeslot_info_entry, entries);
			break;
			}
		}
	}

	// Attach one node onto the timeslot info queue within the frame info.

	// Increase the timeslot_loop_count field.
	_timeslot_loop_count = _ptr_frame_info_entry->get_timeslot_loop_count();

	_timeslot_loop_count++;

	_ptr_frame_info_entry->set_timeslot_loop_count(_timeslot_loop_count);


	// Increase the total_timeslot_count field.
	_total_timeslot_count = _ptr_frame_info_entry->get_total_timeslot_count();
	_total_timeslot_count += (1 + timeslot_info.get_repeat_count());
	_ptr_frame_info_entry->set_total_timeslot_count(_total_timeslot_count);
	return 0;
}// End of Fct::add_timeslot_info()




int	Fct::remove_timeslot_info(u_char frame_id, u_int16_t timeslot_number)
{
	Fct_timeslot_info_entry			*_ptr_timeslot_info_entry;
	Fct_timeslot_info_circleq			*_ptr_timeslot_info_q;
	Fct_frame_info_entry		*_ptr_frame_info_entry;
	bool							_frame_id_exist;
	char							_timeslot_loop_count;



	// Check if the frame id exists.
	_frame_id_exist = false;

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		if(frame_id == _ptr_frame_info_entry-> get_frame_id())
		{
			_frame_id_exist = true;
			break;
		}
	}

	if(!_frame_id_exist)	return 1;


	_ptr_timeslot_info_q = &(_ptr_frame_info_entry-> fct_timeslot_info_circleq);

	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
	{
		if(timeslot_number - _ptr_timeslot_info_entry-> timeslot_number <= _ptr_timeslot_info_entry->get_repeat_count())
		{
			// Dettach entry.
			CIRCLEQ_REMOVE(_ptr_timeslot_info_q, _ptr_timeslot_info_entry, entries);
			delete(_ptr_timeslot_info_entry);

			// Decrease the timeslot_loop_count field.
			_timeslot_loop_count = _ptr_frame_info_entry-> get_timeslot_loop_count();

			_timeslot_loop_count--;

			_ptr_frame_info_entry-> set_timeslot_loop_count(_timeslot_loop_count);
			
			return 0;
		}
	}

	// Frame number doesn't not exist.
	return 1;
}



int	Fct::remove_frame_info(u_char frame_id)
{
	Fct_frame_info_entry		*_ptr_frame_info_entry;
	Fct_timeslot_info_circleq			*_ptr_timeslot_info_q;


	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		if(frame_id == _ptr_frame_info_entry-> get_frame_id())// Supertimeslot info entry is found.
		{
			// Free the timeslot info queue within frame info.
			_ptr_timeslot_info_q = &(_ptr_frame_info_entry-> fct_timeslot_info_circleq);
			_ptr_timeslot_info_q-> free();

			CIRCLEQ_REMOVE(&_fct_frame_info_circleq, _ptr_frame_info_entry, entries);
			delete(_ptr_frame_info_entry);
			_frame_loop_count--;

			return 0;
		}
	}
	// Such frame info doesn't exist.
	return 1;
}


int Fct::get_frame_info(u_char frame_id, Fct_frame_info* frame_info_buf)
{
	Fct_frame_info_entry		*_ptr_frame_info_entry;


	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		if(frame_id == _ptr_frame_info_entry-> get_frame_id())// Supertimeslot info entry is found.
		{
			// Copy the frame info.
			*frame_info_buf = _ptr_frame_info_entry-> frame_info;

			return 0;
		}
	}

	// Such frame info doesn't exist.
	return 1;
}



int Fct::get_timeslot_info(u_char frame_id, u_int16_t timeslot_number, Fct_timeslot_info* timeslot_info_buf)
{
	Fct_timeslot_info_entry			*_ptr_timeslot_info_entry;
	Fct_timeslot_info_circleq			*_ptr_timeslot_info_q;
	Fct_frame_info_entry		*_ptr_frame_info_entry;
	bool							_frame_id_exist;



	// Check if the frame id exists.
	_frame_id_exist = false;

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		if(frame_id == _ptr_frame_info_entry-> get_frame_id())
		{
			_frame_id_exist = true;
			break;
		}
	}

	if(!_frame_id_exist)	return 1;


	_ptr_timeslot_info_q = &(_ptr_frame_info_entry-> fct_timeslot_info_circleq);

	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
	{
		if(_ptr_timeslot_info_entry->get_timeslot_number() <= timeslot_number &&
			timeslot_number <= _ptr_timeslot_info_entry->get_timeslot_number() + 
			_ptr_timeslot_info_entry->get_repeat_count()) // The timeslot entry is found.
		{
			(*timeslot_info_buf) = _ptr_timeslot_info_entry-> timeslot_info;
			return 0;
		}
	}

	// Frame number doesn't not exist.
	return 1;
}

int Fct::table_len() {
	Fct_frame_info_entry		*_ptr_frame_info_entry;
	int				table_len = 0;
	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &_fct_frame_info_circleq, entries)
	{
		table_len += 	(_ptr_frame_info_entry->frame_info_len());
		printf("%d %d %d\n", _ptr_frame_info_entry->frame_info_len(), _ptr_frame_info_entry->get_timeslot_loop_count(), FCT_TIMESLOT_INFO_SIZE);
	}
	return table_len;
}
