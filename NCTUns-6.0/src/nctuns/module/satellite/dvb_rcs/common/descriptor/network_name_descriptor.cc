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

#include <string.h>
#include "network_name_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Network_name_descriptor::Network_name_descriptor(){

	_descriptor_tag = NETWORK_NAME_DESCRIPTOR;
	_descriptor_length = 0;
	_char = NULL;
}

/*
 * consructor
 */
Network_name_descriptor::Network_name_descriptor(char* set_network_name){

	_descriptor_tag = NETWORK_NAME_DESCRIPTOR;
	_descriptor_length = strlen(set_network_name);
	_char = new char[_descriptor_length];
	strcpy(_char , set_network_name);
}

/*
 * destructor
 */
Network_name_descriptor::~Network_name_descriptor(){

	delete _char;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Network_name_descriptor
 */
Network_name_descriptor*
Network_name_descriptor::descriptor_copy(){
	
	if(_char != NULL){
		Network_name_descriptor *des_copy = new Network_name_descriptor(_char);
		return(des_copy);
	}
	else{
		Network_name_descriptor *des_copy = new Network_name_descriptor();
		return(des_copy);
	}
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all conte of descriptor into continuous memory space)
 */
void
Network_name_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;

	if(_descriptor_length > 0)
		memcpy(serialize_start + 2 , _char , _descriptor_length);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Network_name_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];

	if(_descriptor_length > 0){
		_char = new char[_descriptor_length];
		memcpy(_char , serialize_start + 2 , _descriptor_length);
	}
}
