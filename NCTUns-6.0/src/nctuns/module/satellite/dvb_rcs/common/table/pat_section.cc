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

#include "pat_section.h"
#include "section_draft.h"
#include "pat_table.h"
#include <stdlib.h>

#define	mem_location (void*)((char*)_section + _offset)

// This function will set up _section_length[] and _last_section_number. //
void Pat_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Pat* pat_table) {
	u_int32_t				_offset, _new_offset ;
	const u_int32_t				_INIT_OFFSET = sizeof(Pat_section_draft);//8B
	bool					_end_of_table;
	Pat_ass_info_entry*			_ass_info_entry;
	Pat_ass_info_circleq*			_ass_info_q;
	
	// Initialization.
	_current_section_number = 0;
	_end_of_table = false;
	_ass_info_q = &(pat_table-> _pat_ass_info_circleq);
	_ass_info_entry = CIRCLEQ_FIRST(_ass_info_q);
	_offset = _INIT_OFFSET;
	while(!_end_of_table) 
	{
		_new_offset = _offset + PAT_ASS_INFO_SIZE;
		if(_new_offset <= MAX_PAT_SECTION_SIZE_NO_INCLUDING_CRC32)
		{
			// There is still enough space to be filled
			// * with this information.
			 //
			 _offset = _new_offset;
		}
			else
			{
				// There is not enough space. And we have to
				// * close this section and open a new section.
				 //
	 			_section_length[_current_section_number++] = _offset -
					SECTION_DRAFT_SIZE + CRC32_SIZE;

				_offset = _INIT_OFFSET;
				_offset += PAT_ASS_INFO_SIZE;
			}

		_ass_info_entry = CIRCLEQ_NEXT(_ass_info_entry, entries);
		if(_ass_info_entry == (Pat_ass_info_entry*)_ass_info_q)
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

void Pat_table_to_section_handler::pat_table_to_section_init(Pat* pat_table) {


	// Fetch information from the input table.
	_pat_table = pat_table;
	_table_id = PAT_TABLE_ID;
	_transport_stream_id = _pat_table-> get_transport_stream_id();
	_version_number = _pat_table-> get_version_number();
	_current_next_indicator = _pat_table-> get_current_next_indicator();
	_calculate_last_section_number_and_section_lengths(_pat_table);

	// Initialize data which will be used.
	_current_section_number = 0;
	_ptr_info_entry = CIRCLEQ_FIRST(&(
		_pat_table-> _pat_ass_info_circleq));

	 assert(_ptr_info_entry != (Pat_ass_info_entry*)
	 	(&(_pat_table-> _pat_ass_info_circleq)));
}

void* Pat_table_to_section_handler::pat_table_to_section() {
	u_int32_t				_offset, _current_section_total_size,_current_section_size;
	u_char					_loop_count;
	void*					_section;
	Pat_section_draft*			_ptr_draft;

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
	_ptr_draft = (Pat_section_draft*) _section;
	_ptr_draft-> set_section_draft(_table_id,
		0x1, 0x0, _current_section_size);
	_ptr_draft-> set_psi_section_draft(
		_transport_stream_id, _version_number, _current_next_indicator,
		_current_section_number, _last_section_number);

	_offset = sizeof(Pat_section_draft);
	// Initial the _loop_count as zero. We will increase the counter
	// * each time we insert one loop into the section.
	 //
	 _loop_count = 0;

	// Copy association information into the section.
	while(_offset < (_current_section_total_size - CRC32_SIZE))
	{
		assert(_ptr_info_entry != (Pat_ass_info_entry*) 
			(&(_pat_table-> _pat_ass_info_circleq)));


		_loop_count++;
		memcpy((char*)_section+_offset, &(_ptr_info_entry->ass_info), PAT_ASS_INFO_SIZE);

		_offset += PAT_ASS_INFO_SIZE;
		_ptr_info_entry = CIRCLEQ_NEXT(_ptr_info_entry, entries);
	}

	_current_section_number++;

	// Compute and fill the CRC32 value.
	fill_crc(_section);

	return _section;
}




bool Pat_section_to_table_handler::_is_complete() {
	for(int i=0; i<=_last_section_number; i++)
	{
		if(!(_received[i]))
			return false;
	}
	return true;
}


// Function	init should be called each time
// * when we need to receive sections for a new table.
//
int Pat_section_to_table_handler::init(void* section) {
	Pat_section_draft* 	_ptr_draft;

	_ptr_draft = (Pat_section_draft*) section;
	_transport_stream_id = _ptr_draft-> get_transport_stream_id();
	_version_number = _ptr_draft-> get_version_number();
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_last_section_number = _ptr_draft-> get_last_section_number();

	for(int i=0; i<=_last_section_number; i++)
	{
		_received[i] = false;
	}
	// Create one table instance.
	_ptr_table = new Pat(_transport_stream_id,
		_version_number, _current_next_indicator);

	return 0;
}



// Function pat_section_to_table_handler do the following:
// 1.Fetch informatin from the input section.
 // 2.Add the information into the imcomplete table.
 // 3.Determine if the table is completed.
 // 4.If the table is completed, return the pointer to this table.
 // 	 Else return null.
 //

Pat*  Pat_section_to_table_handler::to_table(void* pat_section) {
	Pat_section_draft* 		_ptr_draft;
	Pat_ass_info_entry* 		_ptr_ass_info_entry;
	Pat_ass_info_circleq*		_ptr_ass_info_q;
	Pat_ass_info*			_ptr_ass_info;
	void*				_info_start_addr;
	u_char				_section_number;
	u_int16_t			_section_length, _total_info_len;

	_ptr_ass_info_q = &(_ptr_table-> _pat_ass_info_circleq);
  	//Step1.Fetch informatin from the input section.
	_ptr_draft = (Pat_section_draft*) pat_section;
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_section_number = _ptr_draft-> get_section_number();
	_section_length = _ptr_draft-> get_section_length();
	_total_info_len = _section_length - CRC32_SIZE;
	//Step2.Add the information into the imcomplete table.
	// Copy the satellite information within the section to 
	// * a assition information entry.
	 //

	// If the current_next_indicator in the section is '1',
	// * which means that the table is currently appliable,
	// * we turn on the table's flag.
	 //
	if(_current_next_indicator==1)
		_ptr_table-> set_current_next_indicator(1);
	
	_info_start_addr = _ptr_ass_info = 
		(Pat_ass_info*) (((char*)pat_section) + PAT_SECTION_DRAFT_SIZE);

	// Scan association information before the CRC32 field.
	while((((char*) _ptr_ass_info) - (char*)_info_start_addr) < _total_info_len-5)
	{
		u_int16_t		_program_number = _ptr_ass_info-> program_number; 
		u_int16_t		_program_map_pid = _ptr_ass_info-> program_map_pid; 

		_ptr_ass_info_entry = new Pat_ass_info_entry(_program_number, _program_map_pid);
		CIRCLEQ_INSERT_HEAD(_ptr_ass_info_q, _ptr_ass_info_entry, entries);
		_ptr_ass_info++;
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

