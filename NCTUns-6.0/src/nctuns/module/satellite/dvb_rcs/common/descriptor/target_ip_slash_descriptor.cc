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
#include "target_ip_slash_descriptor.h"

/*
 * default consructor
 */
Target_ip_slash_descriptor::Target_ip_slash_descriptor(){

	_descriptor_tag = TARGET_IP_SLASH_DESCRIPTOR;
	_descriptor_length = 0;
	_IPv4_addr_list_pointer = NULL;
}
/*
 * consructor
 * Usage : Only for creating IPv4 address list 
 */
Target_ip_slash_descriptor::Target_ip_slash_descriptor(int ipv4_addr_number , IPv4_format* set_ipv4_addr_list){

	_descriptor_tag = TARGET_IP_SLASH_DESCRIPTOR;
	_descriptor_length = ipv4_addr_number*(IPV4_ADDR_SIZE + 1);
	_IPv4_addr_list_pointer = new IPv4_format[ipv4_addr_number];
	
	for(int i = 0 ; i <= (ipv4_addr_number - 1) ; i++){

		_IPv4_addr_list_pointer[i].IPv4_addr = set_ipv4_addr_list[i].IPv4_addr;
		_IPv4_addr_list_pointer[i].IPv4_slash_mask = set_ipv4_addr_list[i].IPv4_slash_mask;
	}
}

/*
 * consructor
 * Usage : For appending other list address list
 */
Target_ip_slash_descriptor::Target_ip_slash_descriptor(int original_ipv4_addr_number, Target_ip_slash_descriptor* original_descriptor , int append_ipv4_addr_number , IPv4_format* append_ipv4_addr_list){

	int			total_ipv4_addr_number = 0;
	IPv4_format		*tmp = NULL;

	total_ipv4_addr_number = original_ipv4_addr_number + append_ipv4_addr_number;
	_descriptor_tag = TARGET_IP_SLASH_DESCRIPTOR;
	_descriptor_length = (total_ipv4_addr_number)*(IPV4_ADDR_SIZE + 1);
	_IPv4_addr_list_pointer = new IPv4_format[total_ipv4_addr_number];

	/* copy original ipv4 address list to new descripotr */
	tmp = original_descriptor->_IPv4_addr_list_pointer;
 	memcpy(_IPv4_addr_list_pointer , tmp , original_ipv4_addr_number*(IPV4_ADDR_SIZE + 1));	

	/* append new ipv4 address list to new descripotr */
	for(int i = 0 ; i <= (append_ipv4_addr_number - 1) ; i++){

		_IPv4_addr_list_pointer[original_ipv4_addr_number + i].IPv4_addr = append_ipv4_addr_list[i].IPv4_addr;
		_IPv4_addr_list_pointer[original_ipv4_addr_number + i].IPv4_slash_mask = append_ipv4_addr_list[i].IPv4_slash_mask;
	}
}

/*
 * destructor
 */
Target_ip_slash_descriptor::~Target_ip_slash_descriptor(){

        delete[] _IPv4_addr_list_pointer;
}

/*
 * lookup_IPv4_addr_list function
 * Usage : If ipv4 address searched is exist in ipv4 address list,
 *         then return true, otherwise return false.
 */
bool
Target_ip_slash_descriptor::lookup_IPv4_addr_list(u_long serched_ipv4_addr){

        int             ipv4_addr_number__ = 0;

        ipv4_addr_number__ = _get_ipv4_addr_num(); 

        for(int i = 0 ; i <= (ipv4_addr_number__ - 1) ; i++){

                u_long		*tmp;
		uint8_t		mask_number__ = 0;
	
		mask_number__ = (_IPv4_addr_list_pointer[i].IPv4_slash_mask)/8;
                tmp = &(_IPv4_addr_list_pointer[i].IPv4_addr);

                if(memcmp(tmp , &serched_ipv4_addr , mask_number__) == 0)
                        return IPV4_EXIST;
        }
        return IPV4_NON_EXIST;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Target_ip_slash_descriptor
 */
Target_ip_slash_descriptor*
Target_ip_slash_descriptor::descriptor_copy(){

	if(_IPv4_addr_list_pointer != NULL){

	        Target_ip_slash_descriptor* des_copy = new Target_ip_slash_descriptor(_get_ipv4_addr_num() , _IPv4_addr_list_pointer);
		return(des_copy);
	}
	else{

		Target_ip_slash_descriptor* des_copy = new Target_ip_slash_descriptor();	
		return(des_copy);
	}
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Target_ip_slash_descriptor::descriptor_serialized(u_char* serialize_start){

	/* serialize */
	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	/* serialize [2] ~ [final] by memcpy */
	if(_descriptor_length > 0)
		memcpy(serialize_start + 2 , _IPv4_addr_list_pointer , _descriptor_length);
}

int
Target_ip_slash_descriptor::_get_ipv4_addr_num(){

	return(_descriptor_length/5);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Target_ip_slash_descriptor::descriptor_deserialized(u_char* serialize_start){

        int             ipv4_addr_number_ = 0;

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];

	/*
	 * _get_ipv4_addr_num() will use '_descriptor_length' member variable
	 * so that you should set up this variable first, then you can use this
	 * function
	 */
        ipv4_addr_number_ = _get_ipv4_addr_num(); 

	if(_descriptor_length > 0){

		_IPv4_addr_list_pointer = new IPv4_format[ipv4_addr_number_];
		memcpy(_IPv4_addr_list_pointer , serialize_start + 2 , _descriptor_length);
	}
}

/*
 * is_match function
 */
bool
Target_ip_slash_descriptor::is_match(u_long rcst_ipaddr){
	
	return (lookup_IPv4_addr_list(rcst_ipaddr));
}

