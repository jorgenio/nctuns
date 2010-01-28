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
#include <string.h>
#include "ip_mac_platform_name_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Ip_mac_platform_name_descriptor::Ip_mac_platform_name_descriptor(){

	_descriptor_tag = IP_MAC_PLATFORM_NAME_DESCRIPTOR;
	_descriptor_length = 3;
	_text_char = NULL;
}

/*
 * consructor
 */
Ip_mac_platform_name_descriptor::Ip_mac_platform_name_descriptor(char* set_ISO_639_language_code , char* set_ip_mac_platform_name){

	_descriptor_tag = IP_MAC_PLATFORM_NAME_DESCRIPTOR;
       	_descriptor_length = (strlen(set_ip_mac_platform_name) + 3);
       	 _text_char = new char[_descriptor_length];
	strcpy(_ISO_639_language_code , set_ISO_639_language_code);
        strcpy(_text_char , set_ip_mac_platform_name);
}

/*
 * destructor
 */
Ip_mac_platform_name_descriptor::~Ip_mac_platform_name_descriptor(){

	delete _text_char;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Ip_mac_platform_name_descriptor
 */
Ip_mac_platform_name_descriptor*
Ip_mac_platform_name_descriptor::descriptor_copy(){
	
	if(_text_char != NULL){
		Ip_mac_platform_name_descriptor *des_copy = new Ip_mac_platform_name_descriptor(_ISO_639_language_code , _text_char);
		return(des_copy);
	}
	else{
		Ip_mac_platform_name_descriptor *des_copy = new Ip_mac_platform_name_descriptor();
		return(des_copy);
	}

}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Ip_mac_platform_name_descriptor::descriptor_serialized(u_char* serialize_start){
			
	/* serialize */
	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , _ISO_639_language_code , 3);

	if(_descriptor_length > 3)
		memcpy(serialize_start + 5 , _text_char , _descriptor_length - 3);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Ip_mac_platform_name_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(_ISO_639_language_code , serialize_start + 2 , 3);

	if(_descriptor_length > 3){
		_text_char = new char[_descriptor_length - 3];
		memcpy(_text_char , serialize_start + 5 , _descriptor_length - 3);
	}
}
