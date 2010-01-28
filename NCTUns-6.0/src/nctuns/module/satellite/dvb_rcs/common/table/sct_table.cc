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

// sct_table.cc

#include "sct_table.h"

uint8_t	Sct_frame_info::get_frame_id()
{
	return	frame_id;
}


uint32_t	Sct_frame_info::get_frame_start_time()
{
	return	frame_start_time;
}


int32_t	Sct_frame_info::get_frame_centre_frequency_offset()
{
	int d, e;
        d = frame_centre_frequency_offset;
        e = d>>23;
        if(e==1)
        d = d ^ 0xff000000;
        return d;
}


int			Sct_frame_info::set_frame_id(uint8_t para_frame_id)
{
	frame_id = para_frame_id;
	return 0;
}


int			Sct_frame_info::set_frame_start_time(uint32_t para_frame_start_time)
{
	frame_start_time = para_frame_start_time;
	return 0;
}


int			Sct_frame_info::set_frame_centre_frequency_offset(int32_t para_frame_centre_frequency_offset)
{
	frame_centre_frequency_offset = para_frame_centre_frequency_offset;
	return 0;
}


uint8_t	Sct_superframe_info::get_superframe_id()
{
	return superframe_id;
}


uint8_t	Sct_superframe_info::get_uplink_polarization()
{
	return uplink_polarization;
}


uint64_t	Sct_superframe_info::get_superframe_start_time_base()
{
	uint64_t	tmp;
	uint64_t*	ptr;
	

	ptr = (uint64_t*) start_time_base_ext;

	tmp = (*ptr)<<(64-33);

	tmp = tmp>>(64-33);

	return tmp;
}


uint64_t	Sct_superframe_info::get_superframe_start_time_ext()
{
	uint64_t	tmp;
	uint64_t*	ptr;
	

	ptr = (uint64_t*) start_time_base_ext;

	tmp = (*ptr)<<(64-33-15);

	tmp = tmp>>(64-15);

	return tmp;
}


uint32_t	Sct_superframe_info::get_superframe_duration()
{
	return superframe_duration;
}


uint32_t	Sct_superframe_info::get_superframe_centre_frequency()
{
	return superframe_centre_frequency;
}


uint16_t	Sct_superframe_info::get_superframe_counter()
{
	return superframe_counter;
}


uint8_t	Sct_superframe_info::get_frame_loop_count()
{
	return frame_loop_count;
}


int			Sct_superframe_info::set_superframe_id(uint8_t para_superframe_id)
{
	superframe_id = para_superframe_id;
	return 0;
}


int			Sct_superframe_info::set_uplink_polarization(uint8_t para_uplink_polarization)
{
	uplink_polarization = para_uplink_polarization;
	return 0;
}


int			Sct_superframe_info::set_superframe_start_time_base(uint64_t para_superframe_start_time_base)
{
	uint64_t	tmp, mask;
	uint64_t*	ptr;


	tmp = para_superframe_start_time_base<<(64-33);

	tmp = tmp>>(64-33);

	ptr = (uint64_t*) start_time_base_ext;

	mask = ~0;
	mask >>= 33;
	mask <<= 33;

	(*ptr) &= mask;
	
	(*ptr) |= tmp;
	
	return 0;
}


int			Sct_superframe_info::set_superframe_start_time_ext(uint64_t para_superframe_start_time_ext)
{
	uint64_t	tmp, mask;
	uint64_t*	ptr;


	tmp = para_superframe_start_time_ext<<(64-15);

	tmp = tmp>>(64-15-33);

	ptr = (uint64_t*) start_time_base_ext;


	mask = 0xFFFF0001;
	mask <<= 32;
	mask |= 0xFFFFFFFF;

	(*ptr) &= mask;	
	(*ptr) |= tmp;
	
	return 0;
}


int			Sct_superframe_info::set_superframe_duration(uint32_t para_superframe_duration)
{
	superframe_duration = para_superframe_duration;
	return 0;
}


int			Sct_superframe_info::set_superframe_centre_frequency(uint32_t para_superframe_centre_frequency)
{
	superframe_centre_frequency = para_superframe_centre_frequency;
	return 0;
}


int			Sct_superframe_info::set_superframe_counter(uint16_t para_superframe_counter)
{
	superframe_counter = para_superframe_counter;
	return 0;
}


int			Sct_superframe_info::set_frame_loop_count(uint8_t para_frame_loop_count)
{
	frame_loop_count = para_frame_loop_count;
	return 0;
}





Sct_frame_info_entry::Sct_frame_info_entry(Sct_frame_info info)
{
	frame_info = info;
}




void Sct_frame_info_circleq::free() {
	Sct_frame_info_entry*		_entry;

	while(this-> cqh_first != ((Sct_frame_info_entry*) this))
	{	
		_entry = this-> cqh_first;	
		CIRCLEQ_REMOVE(this, _entry, entries);
		delete _entry;
	}														
}


void Sct_frame_info_circleq::copy(Sct_frame_info_circleq* dst,
		Sct_frame_info_circleq* src) 
{
	Sct_frame_info_entry		*_dst_ass_info_entry, *_src_ass_info_entry;


	CIRCLEQ_INIT(dst);						

	CIRCLEQ_FOREACH(_src_ass_info_entry, src, entries)
	{
		_dst_ass_info_entry = new Sct_frame_info_entry(*_src_ass_info_entry);
		CIRCLEQ_INSERT_HEAD(dst, _dst_ass_info_entry, entries);		
	}
}



Sct_superframe_info_entry::Sct_superframe_info_entry(Sct_superframe_info s_info)
{
	superframe_info = s_info;
	CIRCLEQ_INIT(&sct_frame_info_circleq);
}// End of Sct_superframe_info_entry::Sct_superframe_info_entry()


uint32_t Sct_superframe_info_entry::superframe_info_len()
{
	uint32_t	superframe_info_len;	
	

	superframe_info_len = SCT_SUPERFRAME_INFO_SIZE + (SCT_FRAME_INFO_SIZE * get_frame_loop_count());
	return superframe_info_len;
}



void Sct_superframe_info_circleq::free() {
	Sct_superframe_info_entry*		_entry;
	Sct_frame_info_circleq*			_ptr_frame_info_q;


	while(this-> cqh_first != ((Sct_superframe_info_entry*) this))
	{	
		_entry = this-> cqh_first;	

		CIRCLEQ_REMOVE(this, _entry, entries);

		_ptr_frame_info_q = &(_entry-> sct_frame_info_circleq);
		_ptr_frame_info_q-> free();

		delete _entry;
	}														
}

void Sct_superframe_info_circleq::copy(Sct_superframe_info_circleq* dst,
		Sct_superframe_info_circleq* src) 
{
	Sct_superframe_info_entry		*_dst_superframe_info_entry, *_src_superframe_info_entry;
	Sct_frame_info_circleq			*_src_frame_info_q, *_dst_frame_info_q;

	CIRCLEQ_INIT(dst);						
	CIRCLEQ_FOREACH(_src_superframe_info_entry, src, entries)
	{
		// Copy the data except dynamic allocated queue.
		_dst_superframe_info_entry = new Sct_superframe_info_entry(*_src_superframe_info_entry);

		// Copy the queue.
		_src_frame_info_q = &(_src_superframe_info_entry-> sct_frame_info_circleq);
		_dst_frame_info_q = &(_dst_superframe_info_entry-> sct_frame_info_circleq);
		Sct_frame_info_circleq::copy(_dst_frame_info_q, _src_frame_info_q);
				
		CIRCLEQ_INSERT_HEAD(dst, _dst_superframe_info_entry, entries);		
	}
}



Sct::Sct() {
	_table_id = SCT_TABLE_ID;
	CIRCLEQ_INIT(&_sct_superframe_info_circleq);
	_superframe_loop_count = 0;
}


Sct::Sct(uint16_t net_id, u_char version_number, 
		u_char current_next_indicator)
{
	_table_id = SCT_TABLE_ID;
	_network_id = net_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
	CIRCLEQ_INIT(&_sct_superframe_info_circleq);
	_superframe_loop_count = 0;
}


Sct::~Sct() {
	// Free the dynamically allocated queue.
	_sct_superframe_info_circleq.free();
}

Sct* Sct::copy() {
	Sct*		clone;

	// Copy all fields but the dynamically allocated queue.
	clone = new Sct(*this);

	Sct_superframe_info_circleq::copy(&(clone-> _sct_superframe_info_circleq), 
			&(this-> _sct_superframe_info_circleq));
	
	return clone;
}


int	Sct::add_superframe_info(Sct_superframe_info superframe_info, 
				 Sct_frame_info frame_info)
{
	Sct_frame_info_entry			*_new_frame_info_entry;
	Sct_frame_info_circleq			*_ptr_frame_info_q;
	Sct_superframe_info_entry		*_ptr_superframe_info_entry;
	Sct_superframe_info_entry		*_new_superframe_info_entry;
	u_char					_arg_superframe_id;
	

	
	// Check if the superframe id is already used.	
	_arg_superframe_id = superframe_info.superframe_id;

	CIRCLEQ_FOREACH(_ptr_superframe_info_entry, &_sct_superframe_info_circleq, entries)
	{
		if(_arg_superframe_id == _ptr_superframe_info_entry-> get_superframe_id() &&
		superframe_info.superframe_counter == _ptr_superframe_info_entry-> get_superframe_counter())
			return 1;
	}

	_new_superframe_info_entry = new Sct_superframe_info_entry(superframe_info);

	_new_superframe_info_entry-> set_frame_loop_count(1);

	// Attach one node onto the superframe info queue.
	CIRCLEQ_INSERT_HEAD(&_sct_superframe_info_circleq,
			_new_superframe_info_entry, entries);

	// Create a frame info entry.
	_new_frame_info_entry = new Sct_frame_info_entry(frame_info);
	_new_frame_info_entry-> frame_number = 0;

	_ptr_frame_info_q = &(_new_superframe_info_entry-> sct_frame_info_circleq);

	// Attach one node onto the frame info queue within the superframe info.
	CIRCLEQ_INSERT_HEAD(_ptr_frame_info_q,
			_new_frame_info_entry, entries);
	
	_superframe_loop_count++;

	return 0;
}// End of Sct::add_superframe_info()


int	Sct::add_frame_info(u_char superframe_id, uint16_t superframe_counter,
	Sct_frame_info frame_info, u_char frame_number)
{
	Sct_frame_info_entry			*_ptr_frame_info_entry, *_new_frame_info_entry;
	Sct_frame_info_circleq			*_ptr_frame_info_q;
	Sct_superframe_info_entry		*_ptr_superframe_info_entry;
	bool							_superframe_id_exist;
	char							_frame_loop_count;



	// Check if the superframe id exists.
	_superframe_id_exist = false;

	CIRCLEQ_FOREACH(_ptr_superframe_info_entry, &_sct_superframe_info_circleq, entries)
	{
		if(superframe_id == _ptr_superframe_info_entry-> get_superframe_id() &&
		superframe_counter == _ptr_superframe_info_entry->get_superframe_counter())
		{
			_superframe_id_exist = true;
			break;
		}
	}

	if(!_superframe_id_exist)	return 1;


	// Check if the frame number is already used.	
	_ptr_frame_info_q = &(_ptr_superframe_info_entry-> sct_frame_info_circleq);

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, _ptr_frame_info_q, entries)
	{
		if(frame_number == _ptr_frame_info_entry-> frame_number)
			return 1;
	}


	// Create a frame info entry.
	_new_frame_info_entry = new Sct_frame_info_entry(frame_info);
	_new_frame_info_entry-> frame_number = frame_number;


	// Attach one node onto the frame info queue within the superframe info.
	CIRCLEQ_INSERT_HEAD(_ptr_frame_info_q,
			_new_frame_info_entry, entries);

	// Increase the frame_loop_count field.
	_frame_loop_count = _ptr_superframe_info_entry->get_frame_loop_count();

	_frame_loop_count++;

	_ptr_superframe_info_entry->set_frame_loop_count(_frame_loop_count);


	return 0;
}// End of Sct::add_frame_info()




int	Sct::remove_frame_info(u_char superframe_id, uint16_t superframe_counter,  u_char frame_number)
{
	Sct_frame_info_entry			*_ptr_frame_info_entry;
	Sct_frame_info_circleq			*_ptr_frame_info_q;
	Sct_superframe_info_entry		*_ptr_superframe_info_entry;
	bool							_superframe_id_exist;
	char							_frame_loop_count;



	// Check if the superframe id exists.
	_superframe_id_exist = false;

	CIRCLEQ_FOREACH(_ptr_superframe_info_entry, &_sct_superframe_info_circleq, entries)
	{
		if(superframe_id == _ptr_superframe_info_entry-> get_superframe_id() &&
		superframe_counter == _ptr_superframe_info_entry->get_superframe_counter())
		{
			_superframe_id_exist = true;
			break;
		}
	}

	if(!_superframe_id_exist)	return 1;


	_ptr_frame_info_q = &(_ptr_superframe_info_entry-> sct_frame_info_circleq);

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, _ptr_frame_info_q, entries)
	{
		if(frame_number == _ptr_frame_info_entry-> frame_number)
		{
			// Dettach entry.
			CIRCLEQ_REMOVE(_ptr_frame_info_q, _ptr_frame_info_entry, entries);
			delete (_ptr_frame_info_entry);

			// Decrease the frame_loop_count field.
			_frame_loop_count = _ptr_superframe_info_entry-> get_frame_loop_count();

			_frame_loop_count--;

			_ptr_superframe_info_entry-> set_frame_loop_count(_frame_loop_count);
			
			return 0;
		}
	}

	// Frame number doesn't not exist.
	return 1;
}



int	Sct::remove_superframe_info(u_char superframe_id, uint16_t superframe_counter)
{
	Sct_superframe_info_entry		*_ptr_superframe_info_entry;
	Sct_frame_info_circleq			*_ptr_frame_info_q;


	CIRCLEQ_FOREACH(_ptr_superframe_info_entry, &_sct_superframe_info_circleq, entries)
	{
		if(superframe_id == _ptr_superframe_info_entry-> get_superframe_id() &&
		superframe_counter == _ptr_superframe_info_entry->get_superframe_counter())
		{
			// Free the frame info queue within superframe info.
			_ptr_frame_info_q = &(_ptr_superframe_info_entry-> sct_frame_info_circleq);
			_ptr_frame_info_q-> free();

			CIRCLEQ_REMOVE(&_sct_superframe_info_circleq, _ptr_superframe_info_entry, entries);
			delete(_ptr_superframe_info_entry);

			return 0;
		}
	}

	// Such superframe info doesn't exist.
	return 1;
}


int Sct::get_superframe_info(u_char superframe_id, uint16_t superframe_counter, Sct_superframe_info* superframe_info_buf)
{
	Sct_superframe_info_entry		*_ptr_superframe_info_entry;


	CIRCLEQ_FOREACH(_ptr_superframe_info_entry, &_sct_superframe_info_circleq, entries)
	{
		if(superframe_id == _ptr_superframe_info_entry-> get_superframe_id()&&
		superframe_counter == _ptr_superframe_info_entry->get_superframe_counter())
		{
			// Copy the superframe info.
			*superframe_info_buf = _ptr_superframe_info_entry-> superframe_info;

			return 0;
		}
	}

	// Such superframe info doesn't exist.
	return 1;
}



bool Sct::get_frame_info(u_char superframe_id, uint16_t superframe_counter, u_char frame_number, Sct_frame_info* frame_info_buf)
{
	Sct_frame_info_entry			*_ptr_frame_info_entry;
	Sct_frame_info_circleq			*_ptr_frame_info_q;
	Sct_superframe_info_entry		*_ptr_superframe_info_entry;
	bool							_superframe_id_exist;



	// Check if the superframe id exists.
	_superframe_id_exist = false;

	CIRCLEQ_FOREACH(_ptr_superframe_info_entry, &_sct_superframe_info_circleq, entries)
	{
		if(superframe_id == _ptr_superframe_info_entry-> get_superframe_id()&&
		superframe_counter == _ptr_superframe_info_entry->get_superframe_counter())
		{
			_superframe_id_exist = true;
			break;
		}
	}

	if(!_superframe_id_exist) return 1;

	_ptr_frame_info_q = &(_ptr_superframe_info_entry-> sct_frame_info_circleq);

	CIRCLEQ_FOREACH(_ptr_frame_info_entry, _ptr_frame_info_q, entries)
	{
		if(frame_number == _ptr_frame_info_entry-> frame_number) // The frame entry is found.
		{
			(*frame_info_buf) = _ptr_frame_info_entry-> frame_info;
			return 0;
		}
	}

	// Frame number doesn't not exist.
	return 1;
}

