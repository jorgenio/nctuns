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
#include "target_mac_address_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Target_mac_address_descriptor::Target_mac_address_descriptor(){

	_descriptor_tag = TARGET_MAC_ADDRESS_DESCRIPTOR;
	_descriptor_length = 6;
	_MAC_addr_list_pointer = NULL;
}
/*
 * consructor
 * Usage : Only for creating mac address list 
 */
Target_mac_address_descriptor::Target_mac_address_descriptor(struct ether_addr set_mac_addr_mask , int mac_addr_number , struct ether_addr* set_mac_addr_list){

	_descriptor_tag = TARGET_MAC_ADDRESS_DESCRIPTOR;
	_descriptor_length = 6 + (mac_addr_number*MAC_ADDR_SIZE);
	set_MAC_addr_mask(set_mac_addr_mask);
	_MAC_addr_list_pointer = new struct ether_addr[mac_addr_number];
	
	for(int i = 0 ; i <= (mac_addr_number - 1) ; i++){

		_MAC_addr_list_pointer[i] = set_mac_addr_list[i];
	}
}

/*
 * consructor
 * Usage : For appending other mac address list
 */
Target_mac_address_descriptor::Target_mac_address_descriptor(int original_mac_addr_number, Target_mac_address_descriptor* original_descriptor , int append_mac_addr_number , struct ether_addr* append_mac_addr_list){

	int			total_mac_addr_number = 0;
	struct ether_addr	*tmp = NULL;

	total_mac_addr_number = original_mac_addr_number + append_mac_addr_number;
	_descriptor_tag = TARGET_MAC_ADDRESS_DESCRIPTOR;
	_descriptor_length = 6 + ((total_mac_addr_number)*MAC_ADDR_SIZE);
	set_MAC_addr_mask(original_descriptor->_MAC_addr_mask);
	_MAC_addr_list_pointer = new struct ether_addr[total_mac_addr_number];

	/* copy original mac address list to new descripotr */
	tmp = original_descriptor->_MAC_addr_list_pointer;
 	memcpy(_MAC_addr_list_pointer , tmp , original_mac_addr_number*MAC_ADDR_SIZE);	
	
	/* append new mac address list to new descripotr */
	for(int i = 0 ; i <= (append_mac_addr_number - 1) ; i++){

		_MAC_addr_list_pointer[original_mac_addr_number + i] = append_mac_addr_list[i];
	}
}

/*
 * destructor
 */
Target_mac_address_descriptor::~Target_mac_address_descriptor(){
	
	delete _MAC_addr_list_pointer;
}

void
Target_mac_address_descriptor::set_MAC_addr_mask(struct ether_addr set_mac_addr_mask){

	_MAC_addr_mask = set_mac_addr_mask;
}

struct ether_addr
Target_mac_address_descriptor::get_MAC_addr_mask(){

	return _MAC_addr_mask;
}

/*
 * lookup_MAC_addr_list function
 * Usage : If mac address searched is exist in mac address list,
 *         then return true, otherwise return false.
 */
bool
Target_mac_address_descriptor::lookup_MAC_addr_list(struct ether_addr serched_mac_addr){

	int		mac_addr_number__ = 0;
	int		mask_number__ = 0;
	
	mask_number__ = _get_mask_num();
	mac_addr_number__ = _get_mac_addr_num();
	
	for(int i = 0 ; i <= (mac_addr_number__ - 1) ; i++){

		/* only compare bytes of mask_number__ */
		if(memcmp(_MAC_addr_list_pointer + i , &serched_mac_addr , mask_number__) == 0)
			return EXIST; 
	}
	return NON_EXIST;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Target_mac_address_descriptor
 */
Target_mac_address_descriptor*
Target_mac_address_descriptor::descriptor_copy(){

	if(_MAC_addr_list_pointer != NULL){

		Target_mac_address_descriptor *des_copy = new Target_mac_address_descriptor(_MAC_addr_mask , _get_mac_addr_num() , _MAC_addr_list_pointer);
		return(des_copy);
	}
	else{

		Target_mac_address_descriptor *des_copy = new Target_mac_address_descriptor();	
		return(des_copy);
	}
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Target_mac_address_descriptor::descriptor_serialized(u_char* serialize_start){

	/* serialize */
	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_MAC_addr_mask , MAC_ADDR_SIZE);

	/* serialize [8] ~ [final] by memcpy */
	if(_descriptor_length > 6)
		memcpy(serialize_start + 8 , _MAC_addr_list_pointer , _descriptor_length - MAC_ADDR_SIZE);
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Target_mac_address_descriptor::descriptor_deserialized(u_char* serialize_start){

	int	mac_addr_num = 0;

	mac_addr_num = _get_mask_num();
	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_MAC_addr_mask , serialize_start + 2 , MAC_ADDR_SIZE);

	if(_descriptor_length > 6){
		_MAC_addr_list_pointer = new struct ether_addr[mac_addr_num];
		memcpy(_MAC_addr_list_pointer , serialize_start + 8 , _descriptor_length - MAC_ADDR_SIZE);
	}
}

int
Target_mac_address_descriptor::_get_mask_num(){

	int count = 0;
	
	for(int i = 0 ; i <= (MAC_ADDR_SIZE - 1) ; i++){

		if(_MAC_addr_mask.octet[i] == 0xff)
			count++;
		else
			break;
	}		
	return count;
}

int
Target_mac_address_descriptor::_get_mac_addr_num(){

	return((_descriptor_length - 6)/MAC_ADDR_SIZE);
}
