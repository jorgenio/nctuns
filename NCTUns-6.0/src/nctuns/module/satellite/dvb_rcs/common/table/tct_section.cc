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
#include "tct_table.h"
#include "tct_section.h"
#include "section_draft.h"

/*
 * Note:
 * section_length[] is mean total_section_size - 3
 *
 */

void 				tct_table_to_section_show(Tct* tct_table);

void	Tct_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Tct* tct_table)
{
	uint32_t				_offset, _new_offset, _current_section_number;
	uint32_t				_timeslot_info_len;
	const uint32_t				_INIT_OFFSET = TCT_SECTION_DRAFT_SIZE ;
	bool					_end_of_table;
	Tct_timeslot_info_entry*		_timeslot_info_entry;
	Tct_timeslot_info_circleq*		_timeslot_info_q;
	

	// Initialization.
	_current_section_number = 0;
	_end_of_table = false;
	_timeslot_info_q = &(tct_table-> _tct_timeslot_info_circleq);
	_timeslot_info_entry = CIRCLEQ_FIRST(_timeslot_info_q);


	_offset = _INIT_OFFSET;
	while(!_end_of_table) 
	{
		// default _timeslot_info_len 21 = 12 + 7 + 1 + 1
		_timeslot_info_len = _timeslot_info_entry->timeslot_info_len();
		// To check if the remainder space is larger enough for this supertimeslot.
		
		
		_new_offset = _offset + _timeslot_info_len;

		if(_new_offset <= MAX_TCT_SECTION_SIZE_NO_INCLUDING_CRC32)
		{
			// There is still enough space to be filled
			// with this information.
			//
			 _offset = _new_offset;
		}
		else
		{
			// There is not enough space. And we have to
			// close this section and open a new section.
			//
			_section_length[_current_section_number++] = _offset -
				SECTION_DRAFT_SIZE + CRC32_SIZE;

			_offset = _INIT_OFFSET;
			_offset += _timeslot_info_len;
		}

		_timeslot_info_entry = CIRCLEQ_NEXT(_timeslot_info_entry, entries);

		if(_timeslot_info_entry == (Tct_timeslot_info_entry*)_timeslot_info_q)
		{
			_end_of_table = true;
			// Close the last section.
			_section_length[_current_section_number] = _offset -
				SECTION_DRAFT_SIZE + CRC32_SIZE;
		}

	}

	// Set the variable '_last_section_number'.
	_last_section_number = _current_section_number;


}

void	Tct_table_to_section_handler::tct_table_to_section_init(Tct* tct_table)
{
	// Fetch information from the input table.
	_tct_table = tct_table;
	_table_id = TCT_TABLE_ID;
	_network_id = _tct_table-> get_network_id();
	_version_number = _tct_table-> get_version_number();
	_current_next_indicator = _tct_table-> get_current_next_indicator();

	_calculate_last_section_number_and_section_lengths(_tct_table);

	// Initialize data which will be used.
	_current_section_number = 0;
	_ptr_timeslot_info_entry = CIRCLEQ_FIRST(&(
		_tct_table-> _tct_timeslot_info_circleq));
	
	
		assert(_ptr_timeslot_info_entry != (Tct_timeslot_info_entry*) 
			(&(_tct_table-> _tct_timeslot_info_circleq)));
}// End of Tct_section::tct_table_to_section_init()


void*	Tct_table_to_section_handler::tct_table_to_section()
{
	uint32_t				_offset, _current_section_total_size,_current_section_size;
	u_char					_timeslot_loop_count;
	void*					_section;
	Tct_section_draft*			_ptr_draft;


	if(_current_section_number > _last_section_number)
	// No more section is generated.
		return 0;

	// Memory allocation for one section.
	_current_section_size = _section_length[_current_section_number]; 
	_current_section_total_size = _current_section_size + 
		SECTION_DRAFT_SIZE;
	_section = malloc(_current_section_total_size);

	// The draft is in the top of section.
	//  And we copy the draft to the right assition.
	//
	_ptr_draft = (Tct_section_draft*) _section;

	_ptr_draft-> set_section_draft(_table_id,
		0x1, 0x0, _current_section_size);

	_ptr_draft-> set_si_type_a_section_draft(
		_network_id, _version_number, _current_next_indicator,
		_current_section_number, _last_section_number);

	_offset = TCT_SECTION_DRAFT_SIZE;
	// Initial the _timeslot_loop_count as zero. We will increase the counter
	// each time we insert one timeslot loop into the section.
	//
	_timeslot_loop_count = 0;


	
	// Insert information.
	while(_offset != (_current_section_total_size - CRC32_SIZE))
	{

		_timeslot_loop_count++;

		// 1. Copy timeslot_info(12B) to section
		memcpy((char*)_section+_offset, &(_ptr_timeslot_info_entry->timeslot_info), TCT_TIMESLOT_INFO_SIZE);
		
		_offset += TCT_TIMESLOT_INFO_SIZE;

		// 2. put permutation_parameters(7B) to section
		if(_ptr_timeslot_info_entry->get_inner_code_type()==1&&
			_ptr_timeslot_info_entry->get_new_permutation()==1)
		{
			memcpy((char*)_section+_offset, _ptr_timeslot_info_entry->permutation_parameters, 7);
			_offset += 7;
		}
		
		// 3. put preamble_length(1B) to section
		memcpy((char*)_section+_offset, &(_ptr_timeslot_info_entry->preamble_length), 1);
		_offset += 1;
		

		if(_ptr_timeslot_info_entry->preamble_length!=0)
		{
			memset((char*)_section+_offset, 0, 
			       (int)ceil(_ptr_timeslot_info_entry->preamble_length/4.0));

			_offset +=(int)ceil(_ptr_timeslot_info_entry->preamble_length/4.0);
		}
		// Iterate to the next superframe info.
		_ptr_timeslot_info_entry = CIRCLEQ_NEXT(_ptr_timeslot_info_entry, entries);
	}

	_timeslot_loop_count--; //A zero count in 'section'indicates one loop.

	// Set the 'timeslot_loop_count' field.
	_ptr_draft-> set_timeslot_loop_count(_timeslot_loop_count);

	_current_section_number++;

	// Compute and fill the CRC32 value.
	fill_crc(_section);

	return _section;
}// End of sct_section::sct_table_to_section().

/* One Tct_section_to_table_handler instance would correspond to
 * the handling for one Tct table in the section module.
 * The pair (network_id, version_number) identifies
 * distinct Tct table.
 */


bool Tct_section_to_table_handler::_is_complete() {
	for(int i=0; i<=_last_section_number; i++)
	{
		if(!(_received[i]))
			return false;
	}
	return true;
}// End of sct_section_to_table_handler::_is_complete(). 


int	Tct_section_to_table_handler::init(void* section)
{
	Tct_section_draft* 		_ptr_draft;


	// Fetch basis information within table.
	_ptr_draft = (Tct_section_draft*) section;
	
	_network_id = _ptr_draft-> get_network_id();
	_version_number = _ptr_draft-> get_version_number();
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_last_section_number = _ptr_draft-> get_last_section_number();

	for(int i=0; i<=_last_section_number; i++)
	{
		_received[i] = false;
	}
	// Create one table instance.
	_ptr_table = new Tct(_network_id,
		_version_number, _current_next_indicator);

	return 0;
} // End of sct_section_to_table_handler::init()




Tct* 	Tct_section_to_table_handler::to_table(void* tct_section)
{
	Tct_section_draft 		*_ptr_draft;
	Tct_timeslot_info_entry 	*_ptr_timeslot_info_entry;
	Tct_timeslot_info_circleq	*_ptr_timeslot_info_q;
	Tct_timeslot_info		*_ptr_timeslot_info;
	void				*_info_start_addr;
	u_char				_section_number;
	uint16_t			_section_length, _total_info_len;


	_ptr_timeslot_info_q = &(_ptr_table-> _tct_timeslot_info_circleq);
  	
	//Step1.Fetch informatin from the input section.
	_ptr_draft = (Tct_section_draft*) tct_section;

	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_section_number = _ptr_draft-> get_section_number();
	// _section_length = data_payload + CRC32(4B) + Si_header_after_section_length_field(6B)
	_section_length = _ptr_draft-> get_section_length();
	_total_info_len = _section_length - CRC32_SIZE;
	
	//Step2.Add the information into the imcomplete table.

	/* Copy the timeslot information within the section to 
	 * a FCT timeslot information entry.
	 */

	/* If the current_next_indicator in the section is '1',
	 * which means that the table is currently appliable,
	 * we turn on the table's flag.
	 */

	if(_current_next_indicator==1)
		_ptr_table-> set_current_next_indicator(1);
	
	// _ptr = _tct_section + 9B
	_info_start_addr = _ptr_timeslot_info = 
		(Tct_timeslot_info*) (((char*)tct_section) + TCT_SECTION_DRAFT_SIZE);


	// Scan information before the CRC32 field.
	// _total_info_len - 6(Si_partial_header) = actual data_payload len
	while((((char*) _ptr_timeslot_info) - ((char*)_info_start_addr)) < _total_info_len-6)
	{
		Tct_timeslot_info			_tmp_timeslot_info;

		// Fetch one timeslot information.
		_tmp_timeslot_info = (*_ptr_timeslot_info);
		
		// Fetch timeslot information(s), put to table

			// 1. put timeslot_info to table
			// _ptr = _tct_section + 12B
			_ptr_timeslot_info_entry = new Tct_timeslot_info_entry((*_ptr_timeslot_info));
			_ptr_timeslot_info++;

			// 2. put permuatation_parameters 7B to table
			if(_ptr_timeslot_info_entry->get_inner_code_type()==1&&
				_ptr_timeslot_info_entry->get_new_permutation()==1)
			{
				Permutation_parameters *ptr_pp = new Permutation_parameters();
				_ptr_timeslot_info_entry->permutation_parameters = ptr_pp;
				
				uint8_t		p0;
				uint16_t	p1;
				uint16_t	p2;
				uint16_t	p3;
				Permutation_parameters *get_pp;
				get_pp = (Permutation_parameters*)_ptr_timeslot_info;
				
				p0 = get_pp->p0;
				p1 = get_pp->p1;
				p2 = get_pp->p2;
				p3 = get_pp->p3;
				
				_ptr_timeslot_info_entry->set_permutation_parameters(p0, p1, p2, p3);
				_ptr_timeslot_info = (Tct_timeslot_info*)((char*)get_pp + 7);
			}

			// 3. put preamble_length to table
			const uint8_t preamble_len = *((uint8_t*) _ptr_timeslot_info);
			_ptr_timeslot_info_entry->set_preamble_length(preamble_len);
			_ptr_timeslot_info =(Tct_timeslot_info*)((char*)_ptr_timeslot_info + 1 +
								 (uint16_t) ceil(_ptr_timeslot_info_entry->get_preamble_length()/4.0));


		CIRCLEQ_INSERT_HEAD(_ptr_timeslot_info_q, _ptr_timeslot_info_entry, entries);
		int tmp_flc;
		tmp_flc = _ptr_table->get_timeslot_loop_count();
		_ptr_table->set_timeslot_loop_count(tmp_flc+1);
		// To next timeslot. 
		_ptr_timeslot_info = (Tct_timeslot_info*) _ptr_timeslot_info;
	}
  	
	//Step3.Determine if the table is completed.
 	//Step4.If the table is completed, return the pointer to this table.
 	//Else return null.
	_received[_section_number] = true;
	if(_is_complete())
		return _ptr_table;
	else
		return 0;

}
