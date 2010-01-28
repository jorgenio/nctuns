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
#include "rcs_content_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Rcs_content_descriptor::Rcs_content_descriptor(){

	_descriptor_tag = RCS_CONTENT_DESCRIPTOR;
	_descriptor_length = 0;
	_Table_id_pointer = NULL;
}

/*
 * consructor
 */

Rcs_content_descriptor::Rcs_content_descriptor(int table_id_number , uint8_t* set_table_id_list){

	_descriptor_tag = RCS_CONTENT_DESCRIPTOR;
	_descriptor_length = table_id_number;
	_Table_id_pointer = new uint8_t[table_id_number];
	
	for(int i = 0 ; i <= (table_id_number - 1) ; i++){

		_Table_id_pointer[i] = set_table_id_list[i];
	}
}

/*
 * consructor
 * Usage : For appending other table id list
 */
Rcs_content_descriptor::Rcs_content_descriptor(int original_table_id_number, Rcs_content_descriptor* original_descriptor , int append_table_id_number , uint8_t* append_table_id_list){

	int			total_table_id_number = 0;
	uint8_t			*tmp = NULL;

	total_table_id_number = original_table_id_number + append_table_id_number;
	_descriptor_tag = RCS_CONTENT_DESCRIPTOR;
	_descriptor_length = (total_table_id_number);
	_Table_id_pointer = new uint8_t[total_table_id_number];

	tmp = original_descriptor->_Table_id_pointer;
	
	/* copy original table list to new descripotr */
 	memcpy(_Table_id_pointer , tmp , original_table_id_number);	
	
	for(int i = 0 ; i <= (append_table_id_number - 1) ; i++){

		_Table_id_pointer[original_table_id_number + i] = append_table_id_list[i];
	}
}

/*
 * destructor
 */
Rcs_content_descriptor::~Rcs_content_descriptor(){

	delete _Table_id_pointer;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Target_ip_slash_descriptor
 */
Rcs_content_descriptor*
Rcs_content_descriptor::descriptor_copy(){
	
	int		table_id_number_ = 0;

	table_id_number_ = _descriptor_length;

	if(_Table_id_pointer != NULL){

		Rcs_content_descriptor *des_copy = new Rcs_content_descriptor(table_id_number_ , _Table_id_pointer);
		return(des_copy);
	}
	else{

		Rcs_content_descriptor *des_copy = new Rcs_content_descriptor();
		return(des_copy);
	}
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Rcs_content_descriptor::descriptor_serialized(u_char* serialize_start){

	/* serialize */
	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;

	if(_descriptor_length > 0)
		memcpy(serialize_start + 2 , _Table_id_pointer , _descriptor_length);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Rcs_content_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	
	if(_descriptor_length > 0){

		_Table_id_pointer = new uint8_t[_descriptor_length];
		memcpy(_Table_id_pointer , serialize_start + 2 , _descriptor_length);
	}
}
