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

// pmt_section.cc

#include <stdlib.h>
#include <string.h>
#include "pmt_table.h"
#include "pmt_section.h"
#include "section_draft.h"
#include "../descriptor/stream_identifier_descriptor.h"
#include "../descriptor/data_broadcast_id_descriptor.h"


// Table to sectino functions

// This function will set up _section_length[] and _last_section_number.
// Note: _section_length[i] means ith section size, no including 3B section_draft size
// section size = 9B(12B(Psi header size) - 3B(section_drft size)) + data_payload size + 4B(CRC32)
void Pmt_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Pmt* pmt_table) {
	u_int32_t				_offset;
	const u_int32_t				_INIT_OFFSET = sizeof(Pmt_section_draft);//12B
	bool					_end_of_table;
	Pmt_frame_info_entry*			frame_info_entry;
	Pmt_frame_info_circleq*			frame_info_q;
	Descriptor_entry			*dct;
	Descriptor_circleq			*dq;
	
	// Initialization.
	_current_section_number = 0;
	_end_of_table = false;
	dq = (pmt_table-> _pmt_des_circleq);
	frame_info_q = &(pmt_table->_pmt_frame_info_circleq);
	frame_info_entry = CIRCLEQ_FIRST(frame_info_q);

	_offset = _INIT_OFFSET;	//12B

	// Count program_info_descriptor length
	CIRCLEQ_FOREACH(dct, dq, entries)
	{
		if(_offset + (dct->descriptor)->get_descriptor_total_len() <= MAX_PMT_SECTION_SIZE_NO_INCLUDING_CRC32) {
			_offset = _offset + (dct->descriptor)->get_descriptor_total_len();
		}
		else
		{
			_section_length[_current_section_number++] = _offset - SECTION_DRAFT_SIZE + CRC32_SIZE;
			_offset = _INIT_OFFSET+ (dct->descriptor)->get_descriptor_total_len();
		}
	}

	// Count frame_info length
	CIRCLEQ_FOREACH(frame_info_entry, frame_info_q, entries)
	{
		if(_offset + PMT_FRAME_INFO_SIZE <= MAX_PMT_SECTION_SIZE_NO_INCLUDING_CRC32)
		{
			_offset = _offset + PMT_FRAME_INFO_SIZE;
			int i=0;
			CIRCLEQ_FOREACH(dct, (frame_info_entry->pmt_frame_info_des_circleq), entries)
			{
				i++;
				if(_offset + (dct->descriptor)->get_descriptor_total_len() <= MAX_PMT_SECTION_SIZE_NO_INCLUDING_CRC32)
				{
					_offset = _offset + (dct->descriptor)->get_descriptor_total_len();
				}
				else
				{
					_section_length[_current_section_number++] = _offset - SECTION_DRAFT_SIZE + CRC32_SIZE;
					// if cut the middle of the descriptor, the next section will include repeat frame data
					_offset = PMT_FRAME_INFO_SIZE + _INIT_OFFSET+ (dct->descriptor)->get_descriptor_total_len();
					
				}
			}
		}
		else
		{
			_section_length[_current_section_number++] = _offset - SECTION_DRAFT_SIZE + CRC32_SIZE;
			_offset = _INIT_OFFSET+ PMT_FRAME_INFO_SIZE;

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
void Pmt_table_to_section_handler::pmt_table_to_section_init(Pmt* pmt_table) {


	// Fetch information from the input table.
	_pmt_table = pmt_table;
	_table_id = PMT_TABLE_ID;
	_program_number = _pmt_table-> get_program_number();
	_version_number = _pmt_table-> get_version_number();
	_current_next_indicator = _pmt_table-> get_current_next_indicator();
	_calculate_last_section_number_and_section_lengths(_pmt_table);

	// Initialize data which will be used.
	_current_section_number = 0;
	_dct_entry = CIRCLEQ_FIRST((_pmt_table-> _pmt_des_circleq));
	_ptr_frame_info_entry = CIRCLEQ_FIRST(&(_pmt_table->_pmt_frame_info_circleq));
	Pmt_frame_info_entry		*end_of_frame_info_cq;
	end_of_frame_info_cq = (Pmt_frame_info_entry*)&(_pmt_table->_pmt_frame_info_circleq);
	if(_ptr_frame_info_entry != end_of_frame_info_cq)
		_dct_entry1 = CIRCLEQ_FIRST(_ptr_frame_info_entry->pmt_frame_info_des_circleq);
		
}

void* Pmt_table_to_section_handler::pmt_table_to_section() {
	u_int32_t				_offset, _current_section_total_size,_current_section_size;
	void*					_section;
	Pmt_section_draft*			_ptr_draft;

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
	_ptr_draft = (Pmt_section_draft*) _section;
	_ptr_draft-> set_section_draft((u_int16_t)_table_id,
		(u_int16_t)0x1,(u_int16_t)0x0,(u_int16_t)_current_section_size);
	_ptr_draft-> set_psi_section_draft(
		_program_number, _version_number, _current_next_indicator,
		_current_section_number, _last_section_number);

	// Question, how to set pcr_pid
	u_int16_t	pcrpid = _pmt_table->get_pcr_pid();
	_ptr_draft->set_pcr_pid(pcrpid);
	_ptr_draft->set_program_info_length(0);

	// Current 'actual' total data size in section
	_offset = sizeof(Pmt_section_draft);	// 12B

	// Copy information into the section.

	// Copy program_info_descriptor into section

	Descriptor_entry *a = (Descriptor_entry*)((_pmt_table->_pmt_des_circleq));
	if(_dct_entry != a)
    {
	while((_offset + (_dct_entry->descriptor)->get_descriptor_total_len()) != (_current_section_total_size - CRC32_SIZE))
	{
		// Check if the program_info_descriptor scan over
		Descriptor_entry *a = (Descriptor_entry*)((_pmt_table->_pmt_des_circleq));
		if(_dct_entry == a) break;
		// Serialize function will copy _dct_entry->descriptor data to address (_section+_offset)
		(_dct_entry->descriptor)->descriptor_serialize((u_char*)_section+_offset);

		_offset += (_dct_entry->descriptor)->get_descriptor_total_len();

		// Increase the program_info_length
		int len = _ptr_draft->get_program_info_length();
		len += (_dct_entry->descriptor)->get_descriptor_total_len();
		_ptr_draft->set_program_info_length(len);
		

		// To next descritpor_entry
		_dct_entry = CIRCLEQ_NEXT(_dct_entry, entries);
		if(_dct_entry == a) break;
	}
    }
	// Copy frame_info data into section
	while( (_offset + PMT_FRAME_INFO_SIZE) <= (_current_section_total_size - CRC32_SIZE))
	{
		// Check if the program_info_descriptor scan over
		Pmt_frame_info_entry *a = (Pmt_frame_info_entry*)(&(_pmt_table->_pmt_frame_info_circleq));
		if( _ptr_frame_info_entry==a ) break;

		// Copy (_ptr_frame_info_entry->frame_info) data to address (_section+_offset)
		memcpy(((char*)_section+_offset), &(_ptr_frame_info_entry->frame_info), PMT_FRAME_INFO_SIZE);
		Pmt_frame_info		*b;
		b = (Pmt_frame_info*)((char*)_section+_offset);

		_offset += PMT_FRAME_INFO_SIZE;

		// Initialize es_info_length
		b->set_es_info_length(0);


	    Descriptor_entry *end_of_des_cq = (Descriptor_entry*)((_ptr_frame_info_entry->pmt_frame_info_des_circleq));
	    if(_dct_entry1 != end_of_des_cq)
	    {	

		while((_offset + (_dct_entry1->descriptor)->get_descriptor_total_len())<= (_current_section_total_size - CRC32_SIZE))
		{
			(_dct_entry1->descriptor)->descriptor_serialize((u_char*)_section+_offset);
			_offset += (_dct_entry1->descriptor)->get_descriptor_total_len();
			int len = b->get_es_info_length();
			len += (_dct_entry1->descriptor)->get_descriptor_total_len();
			b->set_es_info_length(len);
			_dct_entry1 = CIRCLEQ_NEXT(_dct_entry1, entries);
			if( _dct_entry1==end_of_des_cq ) break;

		}
	    }
		// To next frame_info_entry

		// the frame_info_des_circleq is scan over, reset frame_info_entry and dct_entry1
		if( _dct_entry1 == end_of_des_cq){
			
			// Next _ptr_frame_info_entry
			_ptr_frame_info_entry = CIRCLEQ_NEXT(_ptr_frame_info_entry, entries);

			// Next _dct_entry1
			Pmt_frame_info_entry *aa = (Pmt_frame_info_entry*)(&(_pmt_table->_pmt_frame_info_circleq));
			if(_ptr_frame_info_entry != aa)
			_dct_entry1 = CIRCLEQ_FIRST((_ptr_frame_info_entry->pmt_frame_info_des_circleq));

			}
			else break;
		}
		// Reset_dct_entry1 location

	// Compute and fill the CRC32 value.
	fill_crc(_section);

	_current_section_number++;
	return _section;
}


// Section to table functions


bool Pmt_section_to_table_handler::_is_complete() {
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
int Pmt_section_to_table_handler::init(void* section) {
	Pmt_section_draft* 	_ptr_draft;

	_ptr_draft = (Pmt_section_draft*) section;
	_program_number = _ptr_draft-> get_program_number();
	_version_number = _ptr_draft-> get_version_number();
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_last_section_number = _ptr_draft-> get_last_section_number();
	_pcr_pid = _ptr_draft->get_pcr_pid();

	for(int i=0; i<=_last_section_number; i++)
	{
		_received[i] = false;
	}
	// Create one table instance.
	_ptr_table = new Pmt(_program_number,
		_version_number, _current_next_indicator);
	_ptr_table->set_pcr_pid(_pcr_pid);
	return 0;
}


// Function pmt_section_to_table_handler do the following:
// 1.Fetch informatin from the input section.
// 2.Add the information into the imcomplete table.
// 3.Determine if the table is completed.
// 4.If the table is completed, return the pointer to this table.
// 	 Else return null.
//
Pmt*  Pmt_section_to_table_handler::to_table(void* pmt_section) {
	Pmt_section_draft* 		_ptr_draft;
	Pmt_frame_info_circleq*		_ptr_frame_info_q;
	Pmt_frame_info*			_ptr_frame_info;
	void*				_info_start_addr;
	u_char				_section_number;
	u_int16_t			_section_length, _total_info_len;
	u_char				*current;

	_ptr_frame_info_q = &(_ptr_table-> _pmt_frame_info_circleq);
	
	
  	// Step1.Fetch informatin from the input section.
	_ptr_draft = (Pmt_section_draft*) pmt_section;
	_current_next_indicator = _ptr_draft-> get_current_next_indicator();
	_section_number = _ptr_draft-> get_section_number();
	_section_length = _ptr_draft-> get_section_length();
	_total_info_len = _section_length - CRC32_SIZE;


	
	// Step2.Add the information into the imcomplete table.
	// Copy the satellite information within the section to 
	// a frameition information entry.
	//


	// If the current_next_indicator in the section is '1',
	// * which means that the table is currently appliable,
	// * we turn on the table's flag.
	//
	if(_current_next_indicator==1)
		_ptr_table-> set_current_next_indicator(1);
	
	_info_start_addr = _ptr_frame_info = 
		(Pmt_frame_info*) (((char*)pmt_section) + PMT_SECTION_DRAFT_SIZE);

	current = (u_char*)_info_start_addr;

	// Get program_info_descriptor to section
	int	program_info_len;
	while((program_info_len=_ptr_draft->get_program_info_length()) != 0 )
	{
		Descriptor	*a;
		int		size;
		a = Descriptor::descriptor_deserialize(current, &size);
		_ptr_table->add_program_info_descriptor(a);
		program_info_len -= size;
		_ptr_draft->set_program_info_length(program_info_len);
		current += size;
	}


	// Get frame_info to section
	while((char*)current - (char*)_info_start_addr  != _total_info_len-9)
	{
		_ptr_frame_info = (Pmt_frame_info*)current;
		Pmt_frame_info	*a = new Pmt_frame_info();
		memcpy(a, _ptr_frame_info, PMT_FRAME_INFO_SIZE);
		Pmt_frame_info	*b;
		b = (Pmt_frame_info*)current;
		int es_info_len;
		es_info_len = _ptr_frame_info->es_info_length;
		current += PMT_FRAME_INFO_SIZE;
		_ptr_table->add_frame_info(*a);
		
		// Get es_info_descriptor to section
		while((es_info_len=(a->get_es_info_length()))!=0)
		{
			Descriptor	*aa;
			int		size;
			aa = Descriptor::descriptor_deserialize(current, &size);
			_ptr_table->add_es_info_descriptor(a->elementary_pid,aa);
			es_info_len -= size;
			a->set_es_info_length(es_info_len);
			current += size;
			delete (Data_broadcast_id_descriptor*)aa;
		}
		delete (Pmt_frame_info*)a;
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

