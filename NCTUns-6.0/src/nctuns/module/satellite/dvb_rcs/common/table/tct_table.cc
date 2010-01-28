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

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include "tct_table.h"

/*
 * Question
 * 1. get_timeslot_info parameter???
 */

//timeslot_info function

	// Get functions //

	int Tct_timeslot_info::	get_timeslot_id()
		{ return timeslot_id;}
	uint64_t Tct_timeslot_info::	get_symbol_rate()
		{ return symbol_rate;}				
	int Tct_timeslot_info::	get_timeslot_duration()
		{ return timeslot_duration;} 
	int Tct_timeslot_info::	get_burst_start_offset()
		{ return burst_start_offset;}
	int Tct_timeslot_info::	get_inner_code_type()
		{ return inner_code_type;}
        int Tct_timeslot_info::	get_inner_code_ordering()
		{ return inner_code_ordering;}
        int Tct_timeslot_info::	get_outer_coding()
		{ return outer_coding;}
	int Tct_timeslot_info::	get_inner_code_puncturing()
		{ return inner_code_puncturing;}
	int Tct_timeslot_info::	get_modulation()
		{ return modulation;}
	int Tct_timeslot_info::	get_baseband_shaping()
		{ return baseband_shaping;}
	int Tct_timeslot_info::	get_timeslot_payload_type()
		{ return timeslot_payload_type;}
	int Tct_timeslot_info::	get_route_id_flag()
		{ return route_id_flag;}
	int Tct_timeslot_info::	get_acm_flag()
		{ return acm_flag;}
	int Tct_timeslot_info::	get_sac_length()
		{ return sac_length;}
	int Tct_timeslot_info::	get_request_flag()
		{ return request_flag;}
        int Tct_timeslot_info::	get_m_and_c_flag()
		{ return m_and_c_flag;}
        int Tct_timeslot_info::	get_group_id_flag()
		{ return group_id_flag;}
	int Tct_timeslot_info::	get_logon_id_flag()
		{ return logon_id_flag;}
	int Tct_timeslot_info::	get_capacity_requests_number()
		{ return capacity_requests_number;}
	int Tct_timeslot_info::	get_new_permutation()
		{ return new_permutation;}


	// Set functions //

	int Tct_timeslot_info::	set_timeslot_id(int  para_timeslot_id)
		{
			timeslot_id = para_timeslot_id;
			return 0;
		}
	int Tct_timeslot_info::	set_symbol_rate (uint64_t  para_symbol_rate )
		{
			symbol_rate = para_symbol_rate;
			return 0;
		}
	int Tct_timeslot_info::	set_timeslot_duration (int  para_timeslot_duration )
		{
			timeslot_duration = para_timeslot_duration;
			return 0;
		}
	int Tct_timeslot_info::	set_burst_start_offset (int  para_burst_start_offset )
		{
			burst_start_offset =  para_burst_start_offset;
			return 0;
		}
	int Tct_timeslot_info::	set_inner_code_type (int  para_inner_code_type )
		{
			inner_code_type =  para_inner_code_type;
			return 0;
		}
	int Tct_timeslot_info::	set_inner_code_ordering( int  para_inner_code_ordering )
        	{
        		inner_code_ordering = para_inner_code_ordering;
        		return 0;
        	}
        int Tct_timeslot_info::	set_outer_coding( int  para_assignment_id )
        	{
        		outer_coding = para_assignment_id;
        		return 0;
        	}
	int Tct_timeslot_info::	set_inner_code_puncturing(int  para_inner_code_puncturing)
		{
			inner_code_puncturing = para_inner_code_puncturing;
			return 0;
		}
	int Tct_timeslot_info::	set_modulation(int  para_modulation)
		{
			modulation = para_modulation;
			return 0;
		}
	int Tct_timeslot_info::	set_baseband_shaping(int  para_baseband_shaping)
		{
			baseband_shaping = para_baseband_shaping;
			return 0;
		}
	int Tct_timeslot_info::	set_timeslot_payload_type(int  para_timeslot_payload_type)
		{
			timeslot_payload_type = para_timeslot_payload_type;
			return 0;
		}
	int Tct_timeslot_info::	set_route_id_flag (int  para_route_id_flag )
		{
			route_id_flag = para_route_id_flag;
			return 0;
		}
	int Tct_timeslot_info::	set_acm_flag (int  para_acm_flag )
		{
			acm_flag = para_acm_flag;
			return 0;
		}
	int Tct_timeslot_info::	set_sac_length (int  para_sac_length )
		{
			sac_length =  para_sac_length;
			return 0;
		}
	int Tct_timeslot_info::	set_request_flag (int  para_request_flag )
		{
			request_flag =  para_request_flag;
			return 0;
		}
	int Tct_timeslot_info::	set_m_and_c_flag( int  para_m_and_c_flag )
        	{
        		m_and_c_flag = para_m_and_c_flag;
        		return 0;
        	}
        int Tct_timeslot_info::	set_group_id_flag( int  para_assignment_id )
        	{
        		group_id_flag = para_assignment_id;
        		return 0;
        	}
	int Tct_timeslot_info::	set_logon_id_flag(int  para_logon_id_flag)
		{
			logon_id_flag = para_logon_id_flag;
			return 0;
		}
	int Tct_timeslot_info::	set_capacity_requests_number(int  para_capacity_requests_number)
		{
			capacity_requests_number = para_capacity_requests_number;
			return 0;
		}
	int Tct_timeslot_info::	set_new_permutation(int  para_new_permutation)
		{
			new_permutation = para_new_permutation;
			return 0;
		}


//timeslot_info_entry

	// Get functions //

	int Tct_timeslot_info_entry::	get_timeslot_id()
		{ return timeslot_info.get_timeslot_id();}
	uint64_t Tct_timeslot_info_entry::	get_symbol_rate()
		{ return timeslot_info.get_symbol_rate();}
	int Tct_timeslot_info_entry::	get_timeslot_duration()
		{ return timeslot_info.get_timeslot_duration();}
	int Tct_timeslot_info_entry::	get_burst_start_offset()
		{ return timeslot_info.get_burst_start_offset();}
	int Tct_timeslot_info_entry::	get_inner_code_type()
		{ return timeslot_info.get_inner_code_type();}
        int Tct_timeslot_info_entry::	get_inner_code_ordering()
		{ return timeslot_info.get_inner_code_ordering();}
        int Tct_timeslot_info_entry::	get_outer_coding()
		{ return timeslot_info.get_outer_coding();}
        int Tct_timeslot_info_entry::	get_inner_code_puncturing()
		{ return timeslot_info.get_inner_code_puncturing();}
	int Tct_timeslot_info_entry::	get_modulation()
		{ return timeslot_info.get_modulation();}
	int Tct_timeslot_info_entry::	get_baseband_shaping()
		{ return timeslot_info.get_baseband_shaping();}                        		
	int Tct_timeslot_info_entry::	get_timeslot_payload_type()
		{ return timeslot_info.get_timeslot_payload_type();}
	int Tct_timeslot_info_entry::	get_route_id_flag()
		{ return timeslot_info.get_route_id_flag();}
	int Tct_timeslot_info_entry::	get_acm_flag()
		{ return timeslot_info.get_acm_flag();}
	int Tct_timeslot_info_entry::	get_sac_length()
		{ return timeslot_info.get_sac_length();}
	int Tct_timeslot_info_entry::	get_request_flag()
		{ return timeslot_info.get_request_flag();}
        int Tct_timeslot_info_entry::	get_m_and_c_flag()
		{ return timeslot_info.get_m_and_c_flag();}
        int Tct_timeslot_info_entry::	get_group_id_flag()
		{ return timeslot_info.get_group_id_flag();}
        int Tct_timeslot_info_entry::	get_logon_id_flag()
		{ return timeslot_info.get_logon_id_flag();}
	int Tct_timeslot_info_entry::	get_capacity_requests_number()
		{ return timeslot_info.get_capacity_requests_number();}
	int Tct_timeslot_info_entry::	get_new_permutation()
		{ return timeslot_info.get_new_permutation();}
	
	int Tct_timeslot_info_entry::	get_permutation_parameters(uint8_t &p0, uint16_t &p1, uint16_t &p2, uint16_t &p3)
	{
		p0 = permutation_parameters->p0;
		p1 = permutation_parameters->p1;
		p2 = permutation_parameters->p2;
		p3 = permutation_parameters->p3;
		return 0;
	}
	
	int Tct_timeslot_info_entry::get_preamble_length()
	{
		return preamble_length;
	}
	// Set functions //

	int Tct_timeslot_info_entry::	set_timeslot_id(int para_timeslot_id)
		{
			return timeslot_info.set_timeslot_id(para_timeslot_id);
		}
	int Tct_timeslot_info_entry::	set_symbol_rate (uint64_t para_symbol_rate )
		{
			return timeslot_info.set_symbol_rate (para_symbol_rate );
		}
	int Tct_timeslot_info_entry::	set_timeslot_duration (int para_timeslot_duration )
		{
			return timeslot_info.set_timeslot_duration (para_timeslot_duration );
		}
	int Tct_timeslot_info_entry::	set_burst_start_offset (int para_burst_start_offset )
		{
			return timeslot_info.set_burst_start_offset (para_burst_start_offset );
		}
	int Tct_timeslot_info_entry::	set_inner_code_type (int para_inner_code_type )
		{
			return timeslot_info.set_inner_code_type (para_inner_code_type);
		}
	int Tct_timeslot_info_entry::	set_inner_code_ordering (int para_inner_code_ordering )
		{
			return timeslot_info.set_inner_code_ordering (para_inner_code_ordering );
		}
	int Tct_timeslot_info_entry::	set_outer_coding (int para_outer_coding)
		{
			return timeslot_info.set_outer_coding (para_outer_coding);
		}
	int Tct_timeslot_info_entry::	set_inner_code_puncturing (int para_inner_code_puncturing )
		{
			return timeslot_info.set_inner_code_puncturing (para_inner_code_puncturing);
		}
	int Tct_timeslot_info_entry::	set_modulation (int para_modulation )
		{
			return timeslot_info.set_modulation (para_modulation );
		}
	int Tct_timeslot_info_entry::	set_baseband_shaping (int para_baseband_shaping )
		{
			return timeslot_info.set_baseband_shaping (para_baseband_shaping );
		}
	int Tct_timeslot_info_entry::	set_timeslot_payload_type(int para_timeslot_payload_type)
		{
			return timeslot_info.set_timeslot_payload_type(para_timeslot_payload_type);
		}
	int Tct_timeslot_info_entry::	set_route_id_flag (int para_route_id_flag )
		{
			return timeslot_info.set_route_id_flag (para_route_id_flag );
		}
	int Tct_timeslot_info_entry::	set_acm_flag (int para_acm_flag )
		{
			return timeslot_info.set_acm_flag (para_acm_flag );
		}
	int Tct_timeslot_info_entry::	set_sac_length (int para_sac_length )
		{
			return timeslot_info.set_sac_length (para_sac_length );
		}
	int Tct_timeslot_info_entry::	set_request_flag (int para_request_flag )
		{
			return timeslot_info.set_request_flag (para_request_flag);
		}
	int Tct_timeslot_info_entry::	set_m_and_c_flag (int para_m_and_c_flag )
		{
			return timeslot_info.set_m_and_c_flag (para_m_and_c_flag );
		}
	int Tct_timeslot_info_entry::	set_group_id_flag (int para_group_id_flag)
		{
			return timeslot_info.set_group_id_flag (para_group_id_flag);
		}
	int Tct_timeslot_info_entry::	set_logon_id_flag (int para_logon_id_flag )
		{
			return timeslot_info.set_logon_id_flag (para_logon_id_flag);
		}
	int Tct_timeslot_info_entry::	set_capacity_requests_number (int para_capacity_requests_number )
		{
			return timeslot_info.set_capacity_requests_number (para_capacity_requests_number );
		}
	int Tct_timeslot_info_entry::	set_new_permutation (int para_new_permutation )
		{
			return timeslot_info.set_new_permutation (para_new_permutation );
		}
	int Tct_timeslot_info_entry::	set_permutation_parameters(uint8_t p0, uint16_t p1, uint16_t p2, uint16_t p3)
	{
		permutation_parameters->p0 = p0;
		permutation_parameters->p1 = p1;
		permutation_parameters->p2 = p2;
		permutation_parameters->p3 = p3;
		return 0;
	}
	
	int Tct_timeslot_info_entry::	set_preamble_length(uint8_t para_preamble_length)
	{
		preamble_length = para_preamble_length;
		return 0;
	}

// Tct_timeslot_info_entry functions

/* Constructor */
Tct_timeslot_info_entry::Tct_timeslot_info_entry()
{
	assert(permutation_parameters = new struct Permutation_parameters);
}
Tct_timeslot_info_entry::Tct_timeslot_info_entry(Tct_timeslot_info info) {
	assert(permutation_parameters = new struct Permutation_parameters);
	timeslot_info = info;
}


// Destructor //
Tct_timeslot_info_entry::~Tct_timeslot_info_entry() {
	delete permutation_parameters;
}


// timeslot_info_len()
int	Tct_timeslot_info_entry::timeslot_info_len() {
	return 	(sizeof(Tct_timeslot_info)+
		(get_inner_code_type()==1&&get_new_permutation()==1)*7+
		(int)ceil(preamble_length/4.0)+1
		);
}

// Tct_timeslot_info_circleq functions

void Tct_timeslot_info_circleq::free(){
	Tct_timeslot_info_entry*		_entry;

	while(this-> cqh_first != ((Tct_timeslot_info_entry*) this))
	{	
		_entry = this-> cqh_first;	
		CIRCLEQ_REMOVE(this, _entry, entries);

		delete _entry;
	}

}


void Tct_timeslot_info_circleq::copy(Tct_timeslot_info_circleq* dst, Tct_timeslot_info_circleq* src) {
	Tct_timeslot_info_entry			*_dst_ass_info_entry, *_src_ass_info_entry;


	CIRCLEQ_INIT(dst);			

	CIRCLEQ_FOREACH(_src_ass_info_entry, src, entries)
	{
		_dst_ass_info_entry = new Tct_timeslot_info_entry(*_src_ass_info_entry);
		CIRCLEQ_INSERT_TAIL(dst, _dst_ass_info_entry, entries);		
	}
}


// table
// Get functions //

	uint16_t	Tct::get_network_id() {return _network_id;}
	
	u_char		Tct::get_version_number() {return _version_number;}
	
	u_char		Tct::get_current_next_indicator() {return _current_next_indicator;}
	
	uint32_t	Tct::get_timeslot_loop_count() {return _timeslot_loop_count;}


// Set functions //

	int		Tct::set_network_id(uint16_t network_id)
			{ _network_id = network_id; return 0;}

	int		Tct::set_version_number(u_char version_number)
			{ _version_number = version_number; return 0;}

	int		Tct::set_current_next_indicator(u_char current_next_indicator)
			{ _current_next_indicator = current_next_indicator; return 0;}
			
	int		Tct::set_timeslot_loop_count(uint8_t timeslot_loop_count)
			{ _timeslot_loop_count = timeslot_loop_count; return 0;}
// Constructor //
Tct::Tct() {
	_table_id = TCT_TABLE_ID;
	CIRCLEQ_INIT(&_tct_timeslot_info_circleq);
	_timeslot_loop_count = 0;
}

Tct::Tct(uint16_t network_id, u_char version_number,
		u_char current_next_indicator) {
	_table_id = TCT_TABLE_ID;
	CIRCLEQ_INIT(&_tct_timeslot_info_circleq);
	_network_id = network_id;
	_version_number = version_number;
	_current_next_indicator = current_next_indicator;
	_timeslot_loop_count = 0;
}


// Destructor //
Tct::~Tct() {
	_tct_timeslot_info_circleq.free();
}

// Q	
/* Table operation functions */


Tct*	Tct::copy() {
	Tct*		clone;

	// Copy all fields but the dynamically allocated queue.
	clone = new Tct(*this);

	Tct_timeslot_info_circleq::copy(&(clone-> _tct_timeslot_info_circleq), 
			&(this-> _tct_timeslot_info_circleq));
	
	return clone;
}


int	
Tct::add_timeslot_info(Tct_timeslot_info timeslot_info, uint8_t p0, uint16_t p1, 
		       uint16_t p2, uint16_t p3, uint8_t preamble_length)
{
	Tct_timeslot_info_entry         *_ptr_timeslot_info_entry;
	 
	// Check if the timeslot_id exist
	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, &_tct_timeslot_info_circleq, entries) {
	        if(timeslot_info.get_timeslot_id()==_ptr_timeslot_info_entry->get_timeslot_id()) {
			return 0;
		}
	}
	
	assert(_ptr_timeslot_info_entry = new Tct_timeslot_info_entry(timeslot_info));

	if(timeslot_info.inner_code_type==1 && timeslot_info.new_permutation==1)
	{
		assert((_ptr_timeslot_info_entry->permutation_parameters = new Permutation_parameters()));
		_ptr_timeslot_info_entry->set_permutation_parameters(p0, p1, p2, p3);
	}

	_ptr_timeslot_info_entry->set_preamble_length(preamble_length);

	CIRCLEQ_INSERT_TAIL(&_tct_timeslot_info_circleq, _ptr_timeslot_info_entry, entries);

	_timeslot_loop_count++;

	return 1;
}

int	Tct::remove_timeslot_info(u_char timeslot_id) {
	Tct_timeslot_info_entry			*_ptr_timeslot_info_entry;


	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, &_tct_timeslot_info_circleq, entries)
	{
		if(timeslot_id == _ptr_timeslot_info_entry-> get_timeslot_id())// Supertimeslot info entry is found.
		{
			CIRCLEQ_REMOVE(&_tct_timeslot_info_circleq, _ptr_timeslot_info_entry, entries);
			if(_ptr_timeslot_info_entry->get_inner_code_type()==1 && 
			_ptr_timeslot_info_entry->get_new_permutation()==1){
				delete _ptr_timeslot_info_entry->permutation_parameters;
			}

			delete(_ptr_timeslot_info_entry);
			_timeslot_loop_count--;

			return 0;
		}
	}
	// Such timeslot info doesn't exist.
	return 1;
	
}

int	Tct::get_timeslot_info(Tct_timeslot_info* timeslot_info_buf, uint8_t timeslot_id,
			       uint8_t &p0, uint16_t &p1, uint16_t &p2,
			       uint16_t &p3, uint8_t &preamble_length) 
{
	Tct_timeslot_info_entry			*_ptr_timeslot_info_entry;


	CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, &_tct_timeslot_info_circleq, entries)
	{
		// Supertimeslot info entry is found.
		if(timeslot_id == _ptr_timeslot_info_entry-> get_timeslot_id())	{
			// Copy the timeslot info.
			*timeslot_info_buf = _ptr_timeslot_info_entry-> timeslot_info;
			if(_ptr_timeslot_info_entry->get_inner_code_type()==1 && 
			_ptr_timeslot_info_entry->get_new_permutation()==1){
				p0 = _ptr_timeslot_info_entry->permutation_parameters->p0;
				p1 = _ptr_timeslot_info_entry->permutation_parameters->p1;
				p2 = _ptr_timeslot_info_entry->permutation_parameters->p2;
				p3 = _ptr_timeslot_info_entry->permutation_parameters->p3;
			}
			preamble_length = _ptr_timeslot_info_entry->preamble_length;

			return 0;
		}
	}

	// Such frame info doesn't exist.
	return 1;

}

