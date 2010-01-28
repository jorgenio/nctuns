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
#include "data_broadcast_id_descriptor.h"
#include "common.h"

/*
 * consructor
 */
Data_broadcast_id_descriptor::Data_broadcast_id_descriptor(){

	_descriptor_tag = DATA_BROADCAST_ID_DESCRIPTOR;
	_descriptor_length = 2;					// by default, no id_selector_bytes
	_id_selector_byte_pointer = NULL;
}

/*
 * consructor
 */
Data_broadcast_id_descriptor::Data_broadcast_id_descriptor(uint16_t set_data_broadcast_id , int platform_id_info_count){

	_descriptor_tag = DATA_BROADCAST_ID_DESCRIPTOR;
	_data_broadcast_id = set_data_broadcast_id;
	_platform_id_info_count = platform_id_info_count;

	/* definition for IP_MAC Notification table */
	if(_data_broadcast_id == 0x0B){

		Ip_mac_notification_info* tmp_ = NULL;
			
		tmp_ = new Ip_mac_notification_info();
		tmp_->_platform_id_info_pointer = new Platform_id_info[platform_id_info_count];
		tmp_->_platform_id_data_length = platform_id_info_count*5;
		_id_selector_byte_pointer = (void*)tmp_;
		_descriptor_length = 2 + (1 + tmp_->_platform_id_data_length);
	}
	/* for other types */
	else{

	} 
}

/*
 * destructor
 */
Data_broadcast_id_descriptor::~Data_broadcast_id_descriptor(){

	if(_data_broadcast_id == 0x0B){

		delete[] ((Ip_mac_notification_info*)_id_selector_byte_pointer)->_platform_id_info_pointer;
		delete (Ip_mac_notification_info*)_id_selector_byte_pointer;
	}

}

/*
 * set_Platform_id_info function
 * Usage : useful for platform_id_info definition
 */
void
Data_broadcast_id_descriptor::set_Platform_id_info(int num , uint32_t set_platform_id , uint32_t set_action_type , uint8_t set_int_versioning_flag , uint8_t set_int_version){

	Platform_id_info* tmp = NULL;

	tmp = _get_platform_id_info_pointer();
	tmp[num]._platform_id = set_platform_id;
	tmp[num]._action_type = set_action_type;
	tmp[num]._reserved = 0;
	tmp[num]._INT_versioning_flag = set_int_versioning_flag;
	tmp[num]._INT_version = set_int_version;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Data_broadcast_id_descriptor
 */
Data_broadcast_id_descriptor*
Data_broadcast_id_descriptor::descriptor_copy(){
	
	if(_id_selector_byte_pointer != NULL){

		Platform_id_info* tmp = NULL;

		tmp = _get_platform_id_info_pointer();
		Data_broadcast_id_descriptor *des_copy = new Data_broadcast_id_descriptor(_data_broadcast_id , _platform_id_info_count);
		memcpy(des_copy->_get_platform_id_info_pointer() , tmp , ((Ip_mac_notification_info*)(des_copy->_id_selector_byte_pointer))->_platform_id_data_length);
		return(des_copy);
	}
	else{
		Data_broadcast_id_descriptor *des_copy = new Data_broadcast_id_descriptor();
		return(des_copy);
	}
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Data_broadcast_id_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_data_broadcast_id , 2);

	if(_descriptor_length > 2){

		serialize_start[4] = ((Ip_mac_notification_info*)_id_selector_byte_pointer)->_platform_id_data_length;
		memcpy(serialize_start + 5 , ((Ip_mac_notification_info*)_id_selector_byte_pointer)->_platform_id_info_pointer , ((Ip_mac_notification_info*)_id_selector_byte_pointer)->_platform_id_data_length);
	}
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Data_broadcast_id_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_data_broadcast_id , serialize_start + 2 , 2);

	if(_descriptor_length > 2){

		/* definition for IP_MAC Notification table */
		if(_data_broadcast_id == 0x0B){

			Ip_mac_notification_info* tmp_ = NULL;
			
			tmp_ = new Ip_mac_notification_info();
			tmp_->_platform_id_data_length = serialize_start[4];
			_platform_id_info_count = tmp_->_platform_id_data_length/5;
			tmp_->_platform_id_info_pointer = new Platform_id_info[_platform_id_info_count];
			memcpy(tmp_->_platform_id_info_pointer , serialize_start + 5 , tmp_->_platform_id_data_length);
			_id_selector_byte_pointer = (void*)tmp_;
		}
		/* for other types */
		else{

		} 
	}
}

/*
 * _get_platform_id_info_pointer function
 */
Platform_id_info*
Data_broadcast_id_descriptor::_get_platform_id_info_pointer(){

	return ((Ip_mac_notification_info*)_id_selector_byte_pointer)->_platform_id_info_pointer;
}

/*
 * get_platform_id function
 */
uint32_t
Data_broadcast_id_descriptor::get_platform_id(int num){

	Platform_id_info* tmp = NULL;

	tmp = _get_platform_id_info_pointer();
	return tmp[num]._platform_id;
}

/*
 * get_action_type function
 */
uint32_t
Data_broadcast_id_descriptor::get_action_type(int num){

	Platform_id_info* tmp = NULL;

	tmp = _get_platform_id_info_pointer();
	return tmp[num]._action_type;
}

/*
 * get_INT_versioning_flag function
 */
uint8_t
Data_broadcast_id_descriptor::get_INT_versioning_flag(int num){

	Platform_id_info* tmp = NULL;

	tmp = _get_platform_id_info_pointer();
	return tmp[num]._INT_versioning_flag;
}

/*
 * get_INT_version function
 */
uint8_t
Data_broadcast_id_descriptor::get_INT_version(int num){

	Platform_id_info* tmp = NULL;

	tmp = _get_platform_id_info_pointer();
	return tmp[num]._INT_version;
}

/*
 * is_match function
 */
bool
Data_broadcast_id_descriptor::is_match(uint16_t data_broadcast_id){

	return (_data_broadcast_id == data_broadcast_id);
}

