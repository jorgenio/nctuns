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
#include "ip_mac_stream_location_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Ip_mac_stream_location_descriptor::Ip_mac_stream_location_descriptor(){

	_descriptor_tag = IP_MAC_STREAM_LOCATION_DESCRIPTOR;
	_descriptor_length = IP_MAC_STREAM_LOCATION_DESCRIPTOR_LENGTH;
}

/*
 * consructor
 */
Ip_mac_stream_location_descriptor::Ip_mac_stream_location_descriptor(uint16_t set_network_id , uint16_t set_original_network_id , uint16_t set_transport_stream_id , uint16_t set_service_id , uint8_t set_component_tag){

	_descriptor_tag = IP_MAC_STREAM_LOCATION_DESCRIPTOR;
	_descriptor_length = IP_MAC_STREAM_LOCATION_DESCRIPTOR_LENGTH;
	_network_id = set_network_id;
	_original_network_id = set_original_network_id;
	_transport_stream_id = set_transport_stream_id;
	_service_id = set_service_id;
	_component_tag = set_component_tag;
}
/*
 * destructor
 */
Ip_mac_stream_location_descriptor::~Ip_mac_stream_location_descriptor(){

}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Ip_mac_stream_location_descriptor
 */
Ip_mac_stream_location_descriptor*
Ip_mac_stream_location_descriptor::descriptor_copy(){
	
	Ip_mac_stream_location_descriptor *des_copy = new Ip_mac_stream_location_descriptor(_network_id , _original_network_id , _transport_stream_id , _service_id , _component_tag);
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Ip_mac_stream_location_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_network_id , 2);
	memcpy(serialize_start + 4 , &_original_network_id , 2);
	memcpy(serialize_start + 6 , &_transport_stream_id , 2);
	memcpy(serialize_start + 8 , &_service_id , 2);
	serialize_start[10] = _component_tag;
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Ip_mac_stream_location_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_network_id , serialize_start + 2 , 2);
	memcpy(&_original_network_id , serialize_start + 4 , 2);
	memcpy(&_transport_stream_id , serialize_start + 6 , 2);
	memcpy(&_service_id , serialize_start + 8 , 2);
	_component_tag = serialize_start[10];
}

/*
 * is_match function
 */
bool
Ip_mac_stream_location_descriptor::is_match(uint8_t component_tag){

	return (_component_tag == component_tag);
}

