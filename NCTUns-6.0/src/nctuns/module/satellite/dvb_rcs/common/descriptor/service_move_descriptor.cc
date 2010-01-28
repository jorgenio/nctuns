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
#include "service_move_descriptor.h"
#include "common.h"

/*
 * consructor
 */
Service_move_descriptor::Service_move_descriptor(){

	_descriptor_tag = SERVICE_MOVE_DESCRIPTOR;
	_descriptor_length = ADAPTATION_FIELD_DATA_DESCRIPTOR_LENGTH;
	_new_original_network_id = 0;
	_new_transport_stream_id = 0;
	_new_service_id = 0;
}

/*
 * consructor
 */
Service_move_descriptor::Service_move_descriptor(uint16_t set_new_original_network_id , uint16_t set_new_transport_stream_id , uint16_t set_new_service_id){

	_descriptor_tag = SERVICE_MOVE_DESCRIPTOR;
	_descriptor_length = ADAPTATION_FIELD_DATA_DESCRIPTOR_LENGTH;
	_new_original_network_id = set_new_original_network_id;
	_new_transport_stream_id = set_new_transport_stream_id;
	_new_service_id = set_new_service_id;
}

/*
 * destructor
 */
Service_move_descriptor::~Service_move_descriptor(){

}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Service_move_descriptor
 */
Service_move_descriptor*
Service_move_descriptor::descriptor_copy(){
	
	Service_move_descriptor *des_copy = new Service_move_descriptor(_new_original_network_id , _new_transport_stream_id , _new_service_id);
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Service_move_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_new_original_network_id , 2);
	memcpy(serialize_start + 4 , &_new_transport_stream_id , 2);
	memcpy(serialize_start + 6 , &_new_service_id , 2);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Service_move_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_new_original_network_id , serialize_start + 2 , 2);
	memcpy(&_new_transport_stream_id , serialize_start + 4 , 2);
	memcpy(&_new_service_id , serialize_start + 6 , 2);
}
