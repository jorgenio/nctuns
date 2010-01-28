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
#include "sync_assign_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Sync_assign_descriptor::Sync_assign_descriptor(){

	_descriptor_tag = SYNC_ASSIGN_DESCRIPTOR;
	_descriptor_length = SYNC_ASSIGN_DESCRIPTOR_LENGTH;

}

/*
 * consructor
 */
Sync_assign_descriptor::Sync_assign_descriptor(uint8_t set_sync_achieved_time_threshold , uint8_t set_max_SYNC_tries , uint8_t set_sync_achieved_frequency_threshold , uint16_t set_sync_start_superframe , uint8_t set_sync_frame_number , uint16_t set_sync_repeat_period , uint16_t set_sync_slot_number){

	_descriptor_tag = SYNC_ASSIGN_DESCRIPTOR;
	_descriptor_length = SYNC_ASSIGN_DESCRIPTOR_LENGTH;
	_SYNC_achieved_time_threshold = set_sync_achieved_time_threshold;
	_max_SYNC_tries = set_max_SYNC_tries;
	_SYNC_achieved_frequency_threshold = set_sync_achieved_frequency_threshold;
	_SYNC_start_superframe = set_sync_start_superframe;
	_SYNC_frame_number = set_sync_frame_number;
	_SYNC_repeat_period = set_sync_repeat_period;
	_SYNC_slot_number = set_sync_slot_number;
}

/*
 * destructor
 */
Sync_assign_descriptor::~Sync_assign_descriptor(){



}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Target_ip_slash_descriptor
 */
Sync_assign_descriptor*
Sync_assign_descriptor::descriptor_copy(){
	
	Sync_assign_descriptor *des_copy = new Sync_assign_descriptor(_SYNC_achieved_time_threshold , _max_SYNC_tries , _SYNC_achieved_frequency_threshold , _SYNC_start_superframe , _SYNC_frame_number , _SYNC_repeat_period , _SYNC_slot_number);
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Sync_assign_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	serialize_start[2] = _SYNC_achieved_time_threshold;
	serialize_start[3] = _max_SYNC_tries;
	memcpy(serialize_start + 4 , &_SYNC_achieved_frequency_threshold , 2);
	memcpy(serialize_start + 6 , &_SYNC_start_superframe , 2);
	serialize_start[8] = _SYNC_frame_number;
	memcpy(serialize_start + 9 , &_SYNC_repeat_period , 2);
	memcpy(serialize_start + 11 , &_SYNC_slot_number , 2);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Sync_assign_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	_SYNC_achieved_time_threshold = serialize_start[2];
	_max_SYNC_tries = serialize_start[3];
	memcpy(&_SYNC_achieved_frequency_threshold , serialize_start + 4 , 2);
	memcpy(&_SYNC_start_superframe , serialize_start + 6 , 2);
	_SYNC_frame_number = serialize_start[8];
	memcpy(&_SYNC_repeat_period , serialize_start + 9 , 2);
	memcpy(&_SYNC_slot_number , serialize_start + 11 , 2);
}
