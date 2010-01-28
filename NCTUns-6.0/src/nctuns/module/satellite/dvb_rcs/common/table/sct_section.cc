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

#include "sct_section.h"
#include "../fec/crc.h"
#define	mem_location (void*)((char*)_section + _offset)



void Sct_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Sct* sct_table) 
{
	u_int32_t				_offset, _new_offset, _current_section_number;
	u_int32_t				_superframe_info_len;
	const u_int32_t				_INIT_OFFSET = SCT_SECTION_DRAFT_SIZE;
	bool					_end_of_table;
	Sct_superframe_info_entry*		_superframe_info_entry;
	Sct_superframe_info_circleq*		_superframe_info_q;
	

	// Initialization.
	_current_section_number = 0;
	_end_of_table = false;
	_superframe_info_q = &(sct_table-> _sct_superframe_info_circleq);
	_superframe_info_entry = CIRCLEQ_FIRST(_superframe_info_q);


	_offset = _INIT_OFFSET;
	while(!_end_of_table) 
	{
		_superframe_info_len = _superframe_info_entry->superframe_info_len();
		// To check if the remainder space is larger enough for this supersuperframe.
		
		
		_new_offset = _offset + _superframe_info_len;

		if(_new_offset <= MAX_SCT_SECTION_SIZE_NO_INCLUDING_CRC32)
		{
			/* There is still enough space to be filled
			 * with this information.
			 */
			 _offset = _new_offset;
		}
		else
		{
			/* There is not enough space. And we have to
			 * close this section and open a new section.
			 */
			_section_length[_current_section_number++] = _offset -
				SECTION_DRAFT_SIZE + CRC32_SIZE;

			_offset = _INIT_OFFSET;
			_offset += _superframe_info_len;
		}

		_superframe_info_entry = CIRCLEQ_NEXT(_superframe_info_entry, entries);

		if(_superframe_info_entry == (Sct_superframe_info_entry*)_superframe_info_q)
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



void Sct_table_to_section_handler::sct_table_to_section_init(Sct* sct_table) 
{
	// Fetch information from the input table.
	_sct_table = sct_table;
	_table_id = SCT_TABLE_ID;
	_network_id = _sct_table-> get_network_id();
	_version_number = _sct_table-> get_version_number();
	_current_next_indicator = _sct_table-> get_current_next_indicator();

	_calculate_last_section_number_and_section_lengths(_sct_table);

	// Initialize data which will be used.
	_current_section_number = 0;
	_ptr_superframe_info_entry = CIRCLEQ_FIRST(&(
		_sct_table-> _sct_superframe_info_circleq));
	
		assert(_ptr_superframe_info_entry != (Sct_superframe_info_entry*) 
			(&(_sct_table-> _sct_superframe_info_circleq)));
}// End of Sct_section::sct_table_to_section_init()



void* Sct_table_to_section_handler::sct_table_to_section()
{
	u_int32_t					_offset, _current_section_total_size,_current_section_size;
	u_char						_superframe_loop_count;
	void*						_section;
	Sct_section_draft*			_ptr_draft;
	Sct_frame_info_circleq*		_ptr_frame_info_q;
	Sct_frame_info_entry*		_ptr_frame_info_entry;


	if(_current_section_number > _last_section_number)
	// No more section is generated.
		return 0;

	// Memory allocation for one section.
	_current_section_size = _section_length[_current_section_number]; 
	_current_section_total_size = _current_section_size + 
		SECTION_DRAFT_SIZE;
	_section = malloc(_current_section_total_size);

	/* The draft is in the top of section.
	 * And we copy the draft to the right assition.
	 */
	_ptr_draft = (Sct_section_draft*) _section;

	_ptr_draft-> set_section_draft(_table_id,
		0x1, 0x0, _current_section_size);

	_ptr_draft-> set_si_type_a_section_draft(
		_network_id, _version_number, _current_next_indicator,
		_current_section_number, _last_section_number);

	_offset = SCT_SECTION_DRAFT_SIZE;
	/* Initial the _superframe_loop_count as zero. We will increase the counter
	 * each time we insert one superframe loop into the section.
	 */
	_superframe_loop_count = 0;

	
	// Insert information.
	while(_offset != (_current_section_total_size - CRC32_SIZE))
	{
		_superframe_loop_count++;

		// Copy superframe information into the section.
		int	_frame_loop_count;
		
		_frame_loop_count = _ptr_superframe_info_entry->get_frame_loop_count();
		if(_frame_loop_count != 0)
		{
			_ptr_superframe_info_entry->set_frame_loop_count(_frame_loop_count-1);
		}
		memcpy((char*)_section+_offset, &(_ptr_superframe_info_entry->superframe_info), SCT_SUPERFRAME_INFO_SIZE);
		
		_offset += SCT_SUPERFRAME_INFO_SIZE;
		
		// Check timeslot ordering
		// timeslot_number order must be "0. 1. 2" , timeslot_number order:" 0. 2. 3" isn't allowed
		_ptr_frame_info_q = &(_ptr_superframe_info_entry -> sct_frame_info_circleq);
		int max_frame_number=0;
		CIRCLEQ_FOREACH(_ptr_frame_info_entry, _ptr_frame_info_q, entries)
		{
			max_frame_number = ( (max_frame_number > _ptr_frame_info_entry->get_frame_number()) ? 
			max_frame_number: _ptr_frame_info_entry->get_frame_number() );
		}
		if(max_frame_number != _ptr_superframe_info_entry->get_frame_loop_count()) {
			printf("\n*************** error *************\n");
			printf("timeslot_number order must be \"0. 1. 2\" ,ex, timeslot_number order:\" 0. 2. 3\" isn't allowed\n");
		}

		// Copy frame information corresponding to the superframe above into the section.
		_ptr_frame_info_q = &(_ptr_superframe_info_entry -> sct_frame_info_circleq);
CIRCLEQ_FOREACH(_ptr_frame_info_entry, _ptr_frame_info_q, entries)
		{
			memcpy((char*)_section+_offset, &(_ptr_frame_info_entry->frame_info), SCT_FRAME_INFO_SIZE);
			_offset += SCT_FRAME_INFO_SIZE;
		}

		// Iterate to the next superframe info.
		_ptr_superframe_info_entry = CIRCLEQ_NEXT(_ptr_superframe_info_entry, entries);
	}

	_superframe_loop_count--; //A zero count indicates one loop.

	// Set the 'superframe_loop_count' field.
	_ptr_draft-> set_superframe_loop_count(_superframe_loop_count);

	// Compute and fill the CRC32 value.
	fill_crc(_section);

	_current_section_number++;
	return _section;
}// End of sct_section::sct_table_to_section().

	

bool Sct_section_to_table_handler::_is_complete() {
	for(int i=0; i<=_last_section_number; i++)
	{
		if(!(_received[i]))
			return false;
	}
	return true;
}// End of sct_section_to_table_handler::_is_complete(). 



/* Function	init should be called each time
 * when we need to receive sections for a new table.
 */


int Sct_section_to_table_handler::init(void* section) {
	Sct_section_draft* 		_ptr_draft;


	// Fetch basis information within table.
	_ptr_draft = (Sct_section_draft*) section;
	
	_network_id = _ptr_draft-> get_network_id();
	_version_number = _ptr_draft-> get_version_number();
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_last_section_number = _ptr_draft-> get_last_section_number();

	for(int i=0; i<=_last_section_number; i++)
	{
		_received[i] = false;
	}
	// Create one table instance.
	_ptr_table = new Sct(_network_id,
		_version_number, _current_next_indicator);

	return 0;
} // End of sct_section_to_table_handler::init()


/* Function sct_section_to_table_handler do the following:
 * 1.Fetch informatin from the input section.
 * 2.Add the information into the imcomplete table.
 * 3.Determine if the table is completed.
 * 4.If the table is completed, return the pointer to this table.
 * 	 Else return null.
 */


Sct*  Sct_section_to_table_handler::to_table(void* sct_section) {
	Sct_section_draft 		*_ptr_draft;
	Sct_superframe_info_entry 		*_ptr_superframe_info_entry;
	Sct_frame_info_entry 	*_ptr_frame_info_entry;
	Sct_superframe_info_circleq		*_ptr_superframe_info_q;
	Sct_frame_info_circleq	*_ptr_frame_info_q;
	Sct_superframe_info			*_ptr_superframe_info;
	Sct_frame_info		*_ptr_frame_info;
	void				*_info_start_addr;
	u_char				_section_number;
	u_int16_t			_section_length, _total_info_len;


	_ptr_superframe_info_q = &(_ptr_table-> _sct_superframe_info_circleq);
  	
	//Step1.Fetch informatin from the input section.
	_ptr_draft = (Sct_section_draft*) sct_section;

	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_section_number = _ptr_draft-> get_section_number();
	_section_length = _ptr_draft-> get_section_length();
	_total_info_len = _section_length - CRC32_SIZE;
	
	//Step2.Add the information into the imcomplete table.

	/* Copy the superframe information within the section to 
	 * a SCT superframe information entry.
	 */

	/* If the current_next_indicator in the section is '1',
	 * which means that the table is currently appliable,
	 * we turn on the table's flag.
	 */

	if(_current_next_indicator==1)
		_ptr_table-> set_current_next_indicator(1);
	
	_info_start_addr = _ptr_superframe_info = 
		(Sct_superframe_info*) (((char*)sct_section) + SCT_SECTION_DRAFT_SIZE);

	
	
	// Scan information before the CRC32 field.
	while((((char*) _ptr_superframe_info) - ((char*)_info_start_addr)) < _total_info_len-6)
	{
		Sct_superframe_info			_tmp_superframe_info;
		u_char					_frame_loop_count;

		// Fetch one superframe information.
		_tmp_superframe_info = (*_ptr_superframe_info);
		
		_frame_loop_count = _tmp_superframe_info.get_frame_loop_count();
		_ptr_superframe_info_entry = new Sct_superframe_info_entry(_tmp_superframe_info);
		int tmp_tlc = _ptr_superframe_info_entry->get_frame_loop_count();
		_ptr_superframe_info_entry->set_frame_loop_count(tmp_tlc+1);

		
		_ptr_frame_info_q = &(_ptr_superframe_info_entry-> sct_frame_info_circleq);
		_ptr_frame_info = (Sct_frame_info*) (((char*) _ptr_superframe_info) + SCT_SUPERFRAME_INFO_SIZE); 

		// Fetch frame information(s) for the superframe.
		for(int i=0; i<=_frame_loop_count; i++)
		{
			_ptr_frame_info_entry = new Sct_frame_info_entry((*_ptr_frame_info));
			_ptr_frame_info_entry->set_frame_number(_frame_loop_count-i);
			CIRCLEQ_INSERT_HEAD(_ptr_frame_info_q, _ptr_frame_info_entry, entries);
			// To next frame.
			_ptr_frame_info++;
		}


		CIRCLEQ_INSERT_HEAD(_ptr_superframe_info_q, _ptr_superframe_info_entry, entries);
		int tmp_flc;
		tmp_flc = _ptr_table->get_superframe_loop_count();
		_ptr_table->set_superframe_loop_count(tmp_flc+1);
		// To next superframe. 
		_ptr_superframe_info = (Sct_superframe_info*) _ptr_frame_info;
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

