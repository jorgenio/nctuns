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

#include <stdlib.h>
#include "section_draft.h"
#include "tbtp_table.h"
#include "tbtp_section.h"

#define	mem_location (void*)((char*)_section + _offset)


// This function will set up _section_length[] and _last_section_number. //

void Tbtp_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Tbtp* tbtp_table) 
{
	u_int32_t				_offset, _new_offset, _current_section_number;
	u_int32_t				_frame_info_len;
	
	//TBTP_SECTION_DRAFT_SIZE = '12B'
	const u_int32_t				_INIT_OFFSET = TBTP_SECTION_DRAFT_SIZE;
	bool					_end_of_table;
	Tbtp_frame_info_entry*			_frame_info_entry;
	Tbtp_frame_info_circleq*		_frame_info_q;
	

	// Initialization.
	_current_section_number = 0;
	_end_of_table = false;
	_frame_info_q = &(tbtp_table-> _tbtp_frame_info_circleq);
	_frame_info_entry = CIRCLEQ_FIRST(_frame_info_q);


	_offset = _INIT_OFFSET;
	while(!_end_of_table) 
	{
		// the timeslot_info_len is not fixed, because of Channel_ID
		_frame_info_len = _frame_info_entry->frame_info_len();
		// To check if the remainder space is larger enough for this superframe.
		
		
		_new_offset = _offset + _frame_info_len;

		if(_new_offset <= MAX_TBTP_SECTION_SIZE_NO_INCLUDING_CRC32)
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
			_offset += _frame_info_len;
		}

		_frame_info_entry = CIRCLEQ_NEXT(_frame_info_entry, entries);

		if(_frame_info_entry == (Tbtp_frame_info_entry*)_frame_info_q)
		{
			_end_of_table = true;
			// Close the last section.
			_section_length[_current_section_number] = _offset -
				SECTION_DRAFT_SIZE + CRC32_SIZE;
		}

	}

	// Set the variable '_last_section_number'.
	_last_section_number = _current_section_number;

}// End of Sct_table_to_section_handler::_calculate_last_section_number_and_section_lengths() 



void Tbtp_table_to_section_handler::tbtp_table_to_section_init(Tbtp* tbtp_table) 
{
	// Fetch information from the input table.
	_tbtp_table = tbtp_table;

	Tbtp_timeslot_info_entry	*b;
	CIRCLEQ_FOREACH(_ptr_frame_info_entry, &(_tbtp_table->_tbtp_frame_info_circleq), entries)
	{
		CIRCLEQ_FOREACH(b, &(_ptr_frame_info_entry->tbtp_timeslot_info_circleq), entries)
		{}
	}
		



	
	_table_id = TBTP_TABLE_ID;
	_network_id = _tbtp_table-> get_network_id();
	_version_number = _tbtp_table-> get_version_number();
	_current_next_indicator = _tbtp_table-> get_current_next_indicator();

	_calculate_last_section_number_and_section_lengths(_tbtp_table);

	// Initialize data which will be used.
	_current_section_number = 0;
	_ptr_frame_info_entry = CIRCLEQ_FIRST(&(
		_tbtp_table-> _tbtp_frame_info_circleq));
	
	
		assert(_ptr_frame_info_entry != (Tbtp_frame_info_entry*) 
			(&(_tbtp_table-> _tbtp_frame_info_circleq)));
}// End of Tbtp_section::tbtp_table_to_section_init()




void* Tbtp_table_to_section_handler::tbtp_table_to_section()
{
	u_int32_t					_offset, _current_section_total_size,_current_section_size;
	u_char						_frame_loop_count;
	void*						_section;
	Tbtp_section_draft*				_ptr_draft;
	Tbtp_timeslot_info_circleq*			_ptr_timeslot_info_q;
	Tbtp_timeslot_info_entry*			_ptr_timeslot_info_entry;


	if(_current_section_number > _last_section_number)
	// No more section is generated.
		return 0;

	// Memory allocation for one section.
	_current_section_size = _section_length[_current_section_number]; 
	_current_section_total_size = _current_section_size + 
		SECTION_DRAFT_SIZE;
	_section = malloc(_current_section_total_size);

	// The draft is in the top of section.
	// And we copy the draft to the right assition.
	//
	_ptr_draft = (Tbtp_section_draft*) _section;

	_ptr_draft-> set_section_draft(_table_id,
		0x1, 0x0, _current_section_size);

	_ptr_draft-> set_si_type_a_section_draft(
		_network_id, _version_number, _current_next_indicator,
		_current_section_number, _last_section_number);
	
	_ptr_draft-> set_group_id(_tbtp_table->_group_id);
	_ptr_draft-> set_superframe_count(_tbtp_table->_superframe_count);

	// TBTP_SECTION_DRAFT_SIZE = 12B
	_offset = TBTP_SECTION_DRAFT_SIZE;
	// Initial the _frame_loop_count as zero. We will increase the counter
	// each time we insert one frame loop into the section.
	//
	_frame_loop_count = 0;

	
	// Insert information.
	while(_offset != (_current_section_total_size - CRC32_SIZE))
	{

		assert(_ptr_frame_info_entry != (Tbtp_frame_info_entry*) 
			(&(_tbtp_table-> _tbtp_frame_info_circleq)));

		_frame_loop_count++;

		// Copy frame information into the section.
		int _btp_loop_count;
		if((_btp_loop_count=_ptr_frame_info_entry->get_btp_loop_count()) != 0)
			_ptr_frame_info_entry->set_btp_loop_count(_btp_loop_count-1);
		memcpy((char*)_section+_offset, &(_ptr_frame_info_entry->frame_info), TBTP_FRAME_INFO_SIZE);
		_offset += TBTP_FRAME_INFO_SIZE;


		// Copy timeslot information corresponding to the frame above into the section.
		_ptr_timeslot_info_q = &(_ptr_frame_info_entry -> tbtp_timeslot_info_circleq);
		_ptr_timeslot_info_entry = 0;
		CIRCLEQ_FOREACH(_ptr_timeslot_info_entry, _ptr_timeslot_info_q, entries)
		{
			memcpy((char*)_section+_offset, _ptr_timeslot_info_entry,
				TBTP_TIMESLOT_INFO_SIZE);

			if(_ptr_timeslot_info_entry->get_multiple_channels_flag())
			{
				memcpy((char*)_section+_offset+TBTP_TIMESLOT_INFO_SIZE, (char*)_ptr_timeslot_info_entry+sizeof(Tbtp_timeslot_info), 3);
			}
			else
			{
				memcpy((char*)_section+_offset+TBTP_TIMESLOT_INFO_SIZE, (char*)_ptr_timeslot_info_entry+sizeof(Tbtp_timeslot_info)+1, 2);
			}
			

			_offset += (TBTP_TIMESLOT_INFO_SIZE + _ptr_timeslot_info_entry->get_multiple_channels_flag()+2);
		}

		// Iterate to the next frame info.
		_ptr_frame_info_entry = CIRCLEQ_NEXT(_ptr_frame_info_entry, entries);
	}

	_frame_loop_count--; //A zero count indicates one loop.

	// Set the 'frame_loop_count' field.
	_ptr_draft-> set_frame_loop_count(_frame_loop_count);

	// Compute and fill the CRC32 value.
	fill_crc(_section);

	_current_section_number++;
	return _section;
}// End of tbtp_section::tbtp_table_to_section().



bool Tbtp_section_to_table_handler::_is_complete() {
	for(int i=0; i<=_last_section_number; i++)
	{
		if(!(_received[i]))
			return false;
	}
	return true;
}// End of sct_section_to_table_handler::_is_complete(). 



int Tbtp_section_to_table_handler::init(void* section) {
	Tbtp_section_draft* 		_ptr_draft;


	// Fetch basis information within table.
	_ptr_draft = (Tbtp_section_draft*) section;
	
	_network_id = _ptr_draft-> get_network_id();
	_version_number = _ptr_draft-> get_version_number();
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_last_section_number = _ptr_draft-> get_last_section_number();

	for(int i=0; i<=_last_section_number; i++)
	{
		_received[i] = false;
	}

	u_int16_t	superframe_count = _ptr_draft->get_superframe_count();
	u_int8_t		group_id = _ptr_draft->get_group_id();
	// Create one table instance.
	_ptr_table = new Tbtp(_network_id,
		_version_number, _current_next_indicator, superframe_count, group_id);

	return 0;
} // End of sct_section_to_table_handler::init()


// Function sct_section_to_table_handler do the following:
// * 1.Fetch informatin from the input section.
// * 2.Add the information into the imcomplete table.
// * 3.Determine if the table is completed.
// * 4.If the table is completed, return the pointer to this table.
// * 	 Else return null.
//


Tbtp*  Tbtp_section_to_table_handler::to_table(void* tbtp_section) {
	Tbtp_section_draft 		*_ptr_draft;
	Tbtp_frame_info_entry 		*_ptr_frame_info_entry;
	Tbtp_timeslot_info_entry 	*_ptr_timeslot_info_entry;
	Tbtp_frame_info_circleq		*_ptr_frame_info_q;
	Tbtp_timeslot_info_circleq	*_ptr_timeslot_info_q;
	Tbtp_frame_info			*_ptr_frame_info;
	Tbtp_timeslot_info		*_ptr_timeslot_info;
	void				*_info_start_addr;
	u_char				_section_number;
	u_int16_t			_section_length, _total_info_len;


	_ptr_frame_info_q = &(_ptr_table-> _tbtp_frame_info_circleq);
  	
	//Step1.Fetch informatin from the input section.
	_ptr_draft = (Tbtp_section_draft*) tbtp_section;

	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_section_number = _ptr_draft-> get_section_number();
	// _section_length = data_payload + CRC32(4B) + Si_header_after_section_length_field(9B)
	_section_length = _ptr_draft-> get_section_length();
	_total_info_len = _section_length - CRC32_SIZE;
	
	//Step2.Add the information into the imcomplete table.

	// Copy the frame information within the section to 
	// * a TBTP frame information entry.
	 //

	// If the current_next_indicator in the section is '1',
	// * which means that the table is currently appliable,
	// * we turn on the table's flag.
	 //

	if(_current_next_indicator==1)
		_ptr_table-> set_current_next_indicator(1);
	
	_info_start_addr = _ptr_frame_info = 
		(Tbtp_frame_info*) (((char*)tbtp_section) + TBTP_SECTION_DRAFT_SIZE);


	// Scan information before the CRC32 field.
	// _total_info_len - 6(Si_partial_header) = actual data_payload len
	while((((char*) _ptr_frame_info) - ((char*)_info_start_addr)) < _total_info_len-9)
	{
		Tbtp_frame_info			_tmp_frame_info;
		u_char				_btp_loop_count;

		// Fetch one frame information.
		_tmp_frame_info = (*_ptr_frame_info);
		
		_btp_loop_count = _tmp_frame_info.get_btp_loop_count();
		_ptr_frame_info_entry = new Tbtp_frame_info_entry(_tmp_frame_info);
		int tmp_tlc = _ptr_frame_info_entry->get_btp_loop_count();
		_ptr_frame_info_entry->set_btp_loop_count(tmp_tlc+1);

		
		_ptr_timeslot_info_q = &(_ptr_frame_info_entry-> tbtp_timeslot_info_circleq);
		_ptr_timeslot_info = (Tbtp_timeslot_info*) (((char*) _ptr_frame_info) + TBTP_FRAME_INFO_SIZE);

		// Fetch timeslot information(s) for the frame.
		for(int i=0; i<=_btp_loop_count; i++)
		{
			_ptr_timeslot_info_entry = new Tbtp_timeslot_info_entry((*_ptr_timeslot_info));
			_ptr_timeslot_info = (Tbtp_timeslot_info*)(((char*) _ptr_timeslot_info)+sizeof(Tbtp_timeslot_info));
			if(_ptr_timeslot_info_entry->get_multiple_channels_flag()==1)
			{
				_ptr_timeslot_info_entry->set_channel_id((int)*((char*)_ptr_timeslot_info));
				_ptr_timeslot_info = (Tbtp_timeslot_info*) (((char*) _ptr_timeslot_info) + sizeof(u_int8_t));
			}
			_ptr_timeslot_info_entry->set_assignment_count((int)*((u_int16_t*)_ptr_timeslot_info));
			_ptr_timeslot_info = (Tbtp_timeslot_info*) (((char*) _ptr_timeslot_info) + sizeof(u_int16_t));
			
			CIRCLEQ_INSERT_HEAD(_ptr_timeslot_info_q, _ptr_timeslot_info_entry, entries);
			// To next timeslot.
		}


		CIRCLEQ_INSERT_HEAD(_ptr_frame_info_q, _ptr_frame_info_entry, entries);
		int tmp_flc;
		tmp_flc = _ptr_table->get_frame_loop_count();
		_ptr_table->set_frame_loop_count(tmp_flc+1);
		// To next frame. 
		_ptr_frame_info = (Tbtp_frame_info*) _ptr_timeslot_info;
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

