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
#include "stream_identifier_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Stream_identifier_descriptor::Stream_identifier_descriptor(){

	_descriptor_tag = STREAM_IDENTIFIER_DESCRIPTOR;
	_descriptor_length = STREAM_IDENTIFIER_DESCRIPTOR_LENGTH;
	_component_tag = 0;
}

/*
 * consructor
 */
Stream_identifier_descriptor::Stream_identifier_descriptor(uint8_t set_component_tag){

	_descriptor_tag = STREAM_IDENTIFIER_DESCRIPTOR;
	_descriptor_length = STREAM_IDENTIFIER_DESCRIPTOR_LENGTH;
	_component_tag = set_component_tag;
}

/*
 * destructor
 */
Stream_identifier_descriptor::~Stream_identifier_descriptor(){

}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Stream_identifier_descriptor
 */
Stream_identifier_descriptor*
Stream_identifier_descriptor::descriptor_copy(){
	
	Stream_identifier_descriptor *des_copy = new Stream_identifier_descriptor(_component_tag);
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Stream_identifier_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	serialize_start[2] = _component_tag;
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Stream_identifier_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	_component_tag = serialize_start[2];
}

/*
 * is_match function
 */
bool
Stream_identifier_descriptor::is_match(uint8_t component_tag){
	
	return (_component_tag == component_tag);
}

