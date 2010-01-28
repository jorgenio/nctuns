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
#include "contention_control_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Contention_control_descriptor::Contention_control_descriptor(){

	_descriptor_tag = CONTENTION_CONTROL_DESCRIPTOR;
	_descriptor_length = 10;
}

/*
 * consructor
 */
//Contention_control_descriptor::Contention_control_descriptor(uint8_t set_sync_achieved_time_threshold , uint8_t set_max_SYNC_tries , uint8_t set_sync_achieved_frequency_threshold , uint16_t set_sync_start_superframe , uint8_t set_sync_frame_number , uint16_t set_sync_repeat_period , uint16_t set_sync_slot_number){
Contention_control_descriptor::
Contention_control_descriptor(	uint8_t Superframe_ID, 
				uint32_t CSC_response_timeout,
				uint8_t CSC_max_losses,
				uint32_t Max_time_before_retry){

	_descriptor_tag = CONTENTION_CONTROL_DESCRIPTOR;
	_descriptor_length = 10;
	_Superframe_ID = Superframe_ID ;
	_CSC_response_timeout = CSC_response_timeout ;
	_CSC_max_losses = CSC_max_losses ;
	_Max_time_before_retry = Max_time_before_retry ;
}


/*
 * destructor
 */
Contention_control_descriptor::~Contention_control_descriptor(){

}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Target_ip_slash_descriptor
 */
Contention_control_descriptor*
Contention_control_descriptor::descriptor_copy(){
	
	Contention_control_descriptor *des_copy = new Contention_control_descriptor(_Superframe_ID, _CSC_response_timeout, _CSC_max_losses, _Max_time_before_retry);
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Contention_control_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	serialize_start[2] = _Superframe_ID;
	memcpy(serialize_start + 3 , &_CSC_response_timeout, 4);
	serialize_start[7] = _CSC_max_losses;
	memcpy(serialize_start + 8 , &_Max_time_before_retry, 4);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Contention_control_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	_Superframe_ID = serialize_start[2];
	memcpy(&_CSC_response_timeout, serialize_start + 3 , 4);
	_CSC_max_losses = serialize_start[7];
	memcpy(&_Max_time_before_retry, serialize_start + 8 , 4);
}


bool
Contention_control_descriptor::is_match(uint8_t si){
	return (_Superframe_ID == si);
}


