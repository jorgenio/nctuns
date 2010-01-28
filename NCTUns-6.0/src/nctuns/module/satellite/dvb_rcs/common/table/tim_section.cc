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

// tim_section.cc

#include <stdlib.h>
#include <string.h>
#include "tim_table.h"
#include "tim_section.h"
#include "section_draft.h"

// Table to section functions

// This function will set up _section_length[] and _last_section_number.
// Note: _section_length[i] means ith section size, no including 3B section_draft size
// section size = 9B(12B(Psi header size) - 3B(section_drft size)) + data_payload size + 4B(CRC32)
void Tim_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Tim* tim_table) {
	u_int32_t				_offset;
	const u_int32_t				_INIT_OFFSET = sizeof(Tim_section_draft);//14B
	bool					_end_of_table;
	Descriptor_entry			*dct;
	Descriptor_circleq			*dq;
	//Tim					*copy_tbl;
	
	// Initialization.
	_current_section_number = 0;
	_end_of_table = false;
	dq = (tim_table-> _tim_des_circleq);

	_offset = _INIT_OFFSET;	//14B

	// Count descriptor length
	CIRCLEQ_FOREACH(dct, dq, entries) {
	
		if(_offset + (dct->descriptor)->get_descriptor_total_len() <= MAX_TIM_SECTION_SIZE_NO_INCLUDING_CRC32) {
		
			_offset = _offset + (dct->descriptor)->get_descriptor_total_len();
		}
		else {
		
			_section_length[_current_section_number++] = _offset - SECTION_DRAFT_SIZE + CRC32_SIZE;
			_offset = _INIT_OFFSET+ (dct->descriptor)->get_descriptor_total_len();
		}
	}


	_section_length[_current_section_number++] = _offset - SECTION_DRAFT_SIZE + CRC32_SIZE;


	// Set the variable '_last_section_number'.
	_last_section_number = _current_section_number-1;

}


// Get some table base info
// Note: _dct_entry. _ptr_frame_info_entry. _dct_entry1 
// means current where table cut point
// if this pointer == circleq_header means for loop finished
void Tim_table_to_section_handler::tim_table_to_section_init(Tim* tim_table) {


	// Fetch information from the input table.
	_tim_table = tim_table;
	_table_id = TIM_TABLE_ID;
	_current_next_indicator = _tim_table-> get_current_next_indicator();
	_calculate_last_section_number_and_section_lengths(_tim_table);

	// Initialize data which will be used.
	_current_section_number = 0;
	_dct_entry = CIRCLEQ_FIRST((_tim_table->_tim_des_circleq));
}


void* Tim_table_to_section_handler::tim_table_to_section() {
	u_int32_t				_offset, _current_section_total_size,_current_section_size;
	void*					_section;
	Tim_section_draft*			_ptr_draft;
	u_char					mac_addr[6];

	// No more section is generated.
	if(_current_section_number > _last_section_number)
		return 0;


	// Memory allocation for one section.
	_current_section_size = _section_length[_current_section_number]; 
	_current_section_total_size = _current_section_size + 
		SECTION_DRAFT_SIZE;
	_section = malloc(_current_section_total_size);

	// The draft is in the top of section.
	//  And we copy the draft to the right frameition.
	//
	_ptr_draft = (Tim_section_draft*) _section;
	_ptr_draft-> set_section_draft((u_int16_t)_table_id,
		(u_int16_t)_tim_table->get_section_syntax_indicator(),
		(u_int16_t)_tim_table->get_private_indicator(),
		(u_int16_t)_current_section_size);
	_ptr_draft->set_dsm_cc_section_draft(
		_tim_table->get_payload_scrambling_control(),
		_tim_table->get_address_scrambling_control(),
		_tim_table->get_llc_snap_flag(),
		_tim_table->get_current_next_indicator(),
		_current_section_number,
		_last_section_number);
	_tim_table ->get_mac_address(mac_addr);
	_ptr_draft ->set_mac_address(mac_addr);

	_ptr_draft->set_status(_tim_table->get_status());
	_ptr_draft->set_descriptor_loop_count(_tim_table->get_descriptor_loop_count());



	// Current 'actual' total data size in section
	_offset = sizeof(Tim_section_draft);	// 14B

	// Copy information into the section.

	// Copy program_info_descriptor into section

	_ptr_draft->set_descriptor_loop_count(0);
	Descriptor_entry *a = (Descriptor_entry*)((_tim_table->_tim_des_circleq));
	if(_dct_entry != a)
    {
	while((_offset + (_dct_entry->descriptor)->get_descriptor_total_len()) <= (_current_section_total_size - CRC32_SIZE))
	{
		// Check if the program_info_descriptor scan over
		Descriptor_entry *a = (Descriptor_entry*)((_tim_table->_tim_des_circleq));
		if(_dct_entry == a) break;
		// Serialize function will copy _dct_entry->descriptor data to address (_section+_offset)

		(_dct_entry->descriptor)->descriptor_serialize((u_char*)_section+_offset);

		_offset += (_dct_entry->descriptor)->get_descriptor_total_len();

		// Increase descriptor_loop_count
		int descriptor_loop_count = _ptr_draft->get_descriptor_loop_count();
		descriptor_loop_count += 1;
		_ptr_draft->set_descriptor_loop_count(descriptor_loop_count);

		// To next descritpor_entry
		_dct_entry = CIRCLEQ_NEXT(_dct_entry, entries);
		if(_dct_entry == a) break;
	}
    }
	// Compute and fill the CRC32 value.
	fill_crc(_section);

	_current_section_number++;
	return _section;
}

// Section to table functions


bool Tim_section_to_table_handler::_is_complete() {
	for(int i=0; i<=_last_section_number; i++)
	{
		if(!(_received[i]))
			return false;
	}
	return true;
}


// Function	init should be called each time
// when we need to receive sections for a new table.
//
int Tim_section_to_table_handler::init(void* section) {
	Tim_section_draft* 	_ptr_draft;

	_ptr_draft = (Tim_section_draft*) section;
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_last_section_number = _ptr_draft-> get_last_section_number();

	for(int i=0; i<=_last_section_number; i++)
	{
		_received[i] = false;
	}
	// Create one table instance.
	_ptr_table = new Tim(_current_next_indicator);
	return 0;
}


// Function tim_section_to_table_handler do the following:
// 1.Fetch informatin from the input section.
// 2.Add the information into the imcomplete table.
// 3.Determine if the table is completed.
// 4.If the table is completed, return the pointer to this table.
// 	 Else return null.
//
Tim*  Tim_section_to_table_handler::to_table(void* tim_section) {
	Tim_section_draft* 		_ptr_draft;
	void*				_info_start_addr;
	u_char				_section_number;
	u_int16_t			_section_length, _total_info_len;
	u_char				*current;
	u_char				mac_addr[6];
	

  	// Step1.Fetch informatin from the input section.
	_ptr_draft = (Tim_section_draft*) tim_section;
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_section_number = _ptr_draft-> get_section_number();
	_section_length = _ptr_draft-> get_section_length();
	_total_info_len = _section_length - CRC32_SIZE;
	_ptr_draft ->get_mac_address(mac_addr);
	_ptr_table ->set_mac_address(mac_addr);
	_ptr_table->set_section_syntax_indicator(_ptr_draft->get_section_syntax_indicator());
	_ptr_table->set_private_indicator(_ptr_draft->get_private_indicator());
	_ptr_table->set_payload_scrambling_control(_ptr_draft->get_payload_scrambling_control());
	_ptr_table->set_address_scrambling_control(_ptr_draft->get_address_scrambling_control());
	_ptr_table->set_llc_snap_flag(_ptr_draft->get_llc_snap_flag());
	_ptr_table->set_current_next_indicator(_ptr_draft->get_current_next_indicator());
	_ptr_table->set_status(_ptr_draft->get_status());
	_ptr_table->set_descriptor_loop_count(0);


	
	// Step2.Add the information into the imcomplete table.
	// Copy the satellite information within the section to 
	// a frameition information entry.
	//


	_info_start_addr = 
		(((char*)tim_section) + TIM_SECTION_DRAFT_SIZE);

	current = (u_char*)_info_start_addr;


	// Get program_info_descriptor to section
	int descriptor_loop_count = _ptr_draft->get_descriptor_loop_count();
	while((descriptor_loop_count=_ptr_draft->get_descriptor_loop_count()) != 0 )
	{
		Descriptor	*a;
		int		size;
		a = Descriptor::descriptor_deserialize(current, &size);
		_ptr_table->add_descriptor(a);
		descriptor_loop_count -= 1;
		_ptr_draft->set_descriptor_loop_count(descriptor_loop_count);
		current += size;
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
