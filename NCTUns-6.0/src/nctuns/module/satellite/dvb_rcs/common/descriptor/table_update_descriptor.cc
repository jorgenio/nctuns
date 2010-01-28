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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "table_update_descriptor.h"

/*
 * default consructor
 */
Table_update_descriptor::Table_update_descriptor(){

	_descriptor_tag = TABLE_UPDATE_DESCRIPTOR;
	_descriptor_length = 2;
	_interactive_network_id = 0;
	_table_id_format_pointer = NULL;
}

/*
 * consructor
 */
Table_update_descriptor::Table_update_descriptor(uint16_t set_interactive_network_id , int table_id_format_number , Table_id_format* table_id_format_list){

	_descriptor_tag = TABLE_UPDATE_DESCRIPTOR;
	_descriptor_length = 2 + table_id_format_number*2;
	_interactive_network_id = set_interactive_network_id;
	_table_id_format_pointer = new Table_id_format[table_id_format_number];
	
	for(int i = 0 ; i <= (table_id_format_number - 1) ; i++){

		_table_id_format_pointer[i] = table_id_format_list[i];	

	}
}

/*
 * consructor
 * Usage : For appending other table id list
 */
Table_update_descriptor::Table_update_descriptor(int original_table_id_format_number , Table_update_descriptor* original_descriptor , int append_table_id_format_number , Table_id_format* append_table_id_format_list){

	int			total_table_id_format_number = 0;
	Table_id_format		*tmp = NULL;

	total_table_id_format_number = original_table_id_format_number + append_table_id_format_number;
	_descriptor_tag = TABLE_UPDATE_DESCRIPTOR;
	_descriptor_length = 2 + total_table_id_format_number*2;
	_interactive_network_id = original_descriptor->_interactive_network_id;
	_table_id_format_pointer = new Table_id_format[total_table_id_format_number];

	tmp = original_descriptor->_table_id_format_pointer;
	
	/* copy original table list to new descripotr */
 	memcpy(_table_id_format_pointer , tmp , original_table_id_format_number*2);	
	
	
	/* append new table id list to new descripotr */
	for(int i = 0 ; i <= (append_table_id_format_number - 1) ; i++){

		_table_id_format_pointer[original_table_id_format_number + i] = append_table_id_format_list[i];
	}
}

/*
 * destructor
 */
Table_update_descriptor::~Table_update_descriptor(){

	delete _table_id_format_pointer;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Target_ip_slash_descriptor
 */
Table_update_descriptor*
Table_update_descriptor::descriptor_copy(){
	
	int	table_id_format_number_ = 0;
		
	table_id_format_number_ = (_descriptor_length - 2)/2;
	
	if(_table_id_format_pointer != NULL){

		Table_update_descriptor *des_copy = new Table_update_descriptor(_interactive_network_id , table_id_format_number_ , _table_id_format_pointer);
		return(des_copy);
	}
	else{

		Table_update_descriptor *des_copy = new Table_update_descriptor();
		return(des_copy);
	}
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Table_update_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_interactive_network_id , 2);

	if(_descriptor_tag > 2)
		memcpy(serialize_start + 4 , _table_id_format_pointer , _descriptor_length - 2);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Table_update_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_interactive_network_id , serialize_start + 2 , 2);

	if(_descriptor_length > 2){

		int	table_id_format_number_ = 0;

		table_id_format_number_ = (_descriptor_length - 2)/2;
		 _table_id_format_pointer = new Table_id_format[table_id_format_number_];
		memcpy(_table_id_format_pointer , serialize_start + 4 , _descriptor_length - 2);
	}
}
