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
#include "linkage_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Linkage_descriptor::Linkage_descriptor(){

	_descriptor_tag = LINKAGE_DESCRIPTOR;
	_descriptor_length = 7;
	_linkage_type_pointer = NULL;
}

/*
 * consructor
 * private_data_byte non-implement yet
 */
Linkage_descriptor::Linkage_descriptor(uint16_t set_transport_stream_id , uint16_t set_original_network_id , uint16_t set_service_id , uint8_t set_linkage_type , int set_platform_id_loop_count){

	_platform_id_loop_count = set_platform_id_loop_count;	
	_descriptor_tag = LINKAGE_DESCRIPTOR;
	_transport_stream_id = set_transport_stream_id;
	_original_network_id = set_original_network_id;
	_service_id = set_service_id;
	_linkage_type = set_linkage_type;
	
	/* for INT */
	if(set_linkage_type == 0x0B){

	
		/* link all pointers needed */

		/* creat a Linkage_for_ip_mac_notification_table */
		_linkage_type_pointer = (void*)new Linkage_for_ip_mac_notification_table();

		/* creat list of linkage_plat_id_data dependenting on _platform_id_loop_count value */
		((Linkage_for_ip_mac_notification_table*)(_linkage_type_pointer))->_platform_id_data_pointer = 
			new Linkage_platform_id_data[set_platform_id_loop_count];

		/* 7 is default length of descriptor, 1 is length of plat id data length field, 4 is length of plat id field and plat name loop length field */
		_descriptor_length = 7 + 1 + 4*set_platform_id_loop_count;
		/* also update plat id data length value */
		_set_platform_id_data_len(set_platform_id_loop_count*4);
	}
	/* for other uses */
	else{

	}
}

/*
 * destructor
 * private_data_byte non-implement yet
 */
Linkage_descriptor::~Linkage_descriptor(){

	if(_linkage_type == 0x0B){

		Linkage_platform_id_data* tmp = NULL;

		tmp = _get_Linkage_platform_id_data_pointer();	
		
		/* delete platform name lists of each platform id loop */
		for(int i = 0 ; i <= (_platform_id_loop_count - 1) ; i++){

			delete[] tmp[i]._platform_name_pointer;
		}
	/* 34 = 3 bytes of _ISO_369_language_code + 1 byte of _platform_name_length + 30 bytes of _text_char */

		//delete _get_private_data_byte_pointer();
		delete[] _get_Linkage_platform_id_data_pointer();
		delete _get_linkage_for_int_pointer();
	}
}

/*
 * set_platform_id function
 */
void
Linkage_descriptor::set_platform_id(int which_plat_id , uint32_t set_plat_id){

	Linkage_platform_id_data* tmp = NULL;

	tmp = _get_Linkage_platform_id_data_pointer();	
	tmp[which_plat_id]._platform_id = set_plat_id;
}

/*
 * get_platform_id function
 */
uint32_t 
Linkage_descriptor::get_platform_id(int which_plat_id){

	Linkage_platform_id_data* tmp = NULL;

	tmp = _get_Linkage_platform_id_data_pointer();	
	return tmp[which_plat_id]._platform_id;
}

/*
 * creat_platform_name_loop function
 */
void
Linkage_descriptor::creat_platform_name_loop(int which_plat_id , int plat_name_count){

	Linkage_platform_id_data* tmp = NULL;
	Linkage_for_ip_mac_notification_table *link_int = NULL;

	tmp = _get_Linkage_platform_id_data_pointer();	
	link_int = _get_linkage_for_int_pointer(); 

	Linkage_platform_name *plat_name_list = new Linkage_platform_name[plat_name_count];
	tmp[which_plat_id]._platform_name_pointer = plat_name_list;
	/* 34 = 3 bytes of _ISO_369_language_code + 1 byte of _platform_name_length + 30 bytes of _text_char */
	tmp[which_plat_id]._platform_name_loop_length = 34*plat_name_count;
	/* increase _descriptor_length when adding some platform name list */
	_descriptor_length += tmp[which_plat_id]._platform_name_loop_length;
	/* also update plat id data length value */
	link_int->_platform_id_data_length += tmp[which_plat_id]._platform_name_loop_length;	
}

/*
 * set_platform_name_variable function
 */
void
Linkage_descriptor::set_platform_name_variable(int which_plat_id , int which_plat_name , const char* set_iso_369_language_code , const char* set_plat_name){

	Linkage_platform_name *set_tmp = NULL;

	set_tmp = _get_Linkage_platform_name_pointer(which_plat_id);	
	set_tmp[which_plat_name].set_platform_name_parameter(set_iso_369_language_code , set_plat_name);
}

/*
 * get_platform_name function
 */
char*
Linkage_descriptor::get_platform_name(int which_plat_id , int which_plat_name){

	Linkage_platform_name *set_tmp = NULL;

	set_tmp = _get_Linkage_platform_name_pointer(which_plat_id);	
	return set_tmp[which_plat_name].get_platform_name_parameter();
}

/*
 * get_iso_369 function
 */
char*
Linkage_descriptor::get_iso_369(int which_plat_id , int which_plat_name){

	Linkage_platform_name *set_tmp = NULL;

	set_tmp = _get_Linkage_platform_name_pointer(which_plat_id);	
	return set_tmp[which_plat_name].get_iso_369_parameter();
}

/*
 * set_private_data_byte function
 * non-implement yet
 */
void 
Linkage_descriptor::set_private_data_byte(u_char* set_private_byte_date){


}

/*
 * get_private_data_byte function
 * non-implement yet
 */
u_char* 
Linkage_descriptor::get_private_data_byte(){

	u_char	*tmp = NULL;

	return tmp;
}

/*
 * descriptor_copy function
 * private_data_byte non-implement yet
 */
Linkage_descriptor*
Linkage_descriptor::descriptor_copy(){
	
	if(_linkage_type_pointer != NULL){

		/* for int use */
		if(_linkage_type == 0x0B){
			/* By constructor function, we can construct the framework of copying descriptor and c.c some content of descriptor */
			Linkage_descriptor *des_copy = new Linkage_descriptor(_transport_stream_id , _original_network_id , _service_id , _linkage_type , _platform_id_loop_count);
			
			/* copy _descriptor_length */
			des_copy->_descriptor_length = _descriptor_length;
			/* copy platform id data length */
			/* 4*_platform_id_loop_count is initial length */
			des_copy->_set_platform_id_data_len(_get_platform_id_data_len() - 4*_platform_id_loop_count);
			
			/* copy left content of descriptor */
			for(int i = 0 ; i < _platform_id_loop_count ; i++){

				int	plat_name_len = 0;
	
				plat_name_len = _get_platform_name_len(i);
				/* copy plat id and plat name length field */
				des_copy->set_platform_id(i , get_platform_id(i));

				if(plat_name_len > 0){
	
					int				plat_name_count_ = 0;
					Linkage_platform_id_data 	*tmp_ = NULL;

					plat_name_count_ = plat_name_len/34;
					des_copy->_set_platform_name_len(i , plat_name_len);
					/* creat platform name list for each platform id */
					Linkage_platform_name *plat_name_list_ = new Linkage_platform_name[plat_name_count_];
					tmp_ = (Linkage_platform_id_data*)(((Linkage_for_ip_mac_notification_table*)(des_copy->_linkage_type_pointer))->_platform_id_data_pointer);
					tmp_[i]._platform_name_pointer = plat_name_list_;

					/* copy content of plat name loop */
					for(int j = 0 ; j < plat_name_count_ ; j++){

						des_copy->set_platform_name_variable(i , j , get_iso_369(i , j) , get_platform_name(i , j));	
					}
				}
				/* plat_name_len is 0 */
				else
					des_copy->_set_platform_name_len(i , 0);

			}
			return(des_copy);
		}
		/* other types */
		else{
	
		}
	}	
	else {
		Linkage_descriptor *des_copy = new Linkage_descriptor();
		return(des_copy);
	}

	return NULL;
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 * private_data_byte non-implement yet
 */
void
Linkage_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_transport_stream_id , 2);
	memcpy(serialize_start + 4 , &_original_network_id , 2);
	memcpy(serialize_start + 6 , &_service_id , 2);
	serialize_start[8] = _linkage_type;
	
	/* for int */
	if(_linkage_type == 0x0B){

		int	platform_id_data_len = 0;
		int 	bytes_counter = 10;

		platform_id_data_len = _get_platform_id_data_len();
		serialize_start[9] = platform_id_data_len;

		for(int i = 0 ; i < _platform_id_loop_count ; i++){

			uint32_t 	plat_id_ = 0;
			uint8_t 	plat_name_len_ = 0;

			plat_id_ = get_platform_id(i);
			plat_name_len_ = _get_platform_name_len(i);
			memcpy(serialize_start + bytes_counter , &plat_id_ , 3);
			bytes_counter += 3;
			memcpy(serialize_start + bytes_counter , &plat_name_len_ , 1);
			bytes_counter += 1;

			if(plat_name_len_ > 0)
				memcpy(serialize_start + bytes_counter , _get_Linkage_platform_name_pointer(i) , plat_name_len_);

			bytes_counter += plat_name_len_;
		}
	}	
	/* for other types */
	else{

	} 
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 * private_data_byte non-implement yet
 */
void
Linkage_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_transport_stream_id , serialize_start + 2 , 2);
	memcpy(&_original_network_id , serialize_start + 4 , 2);
	memcpy(&_service_id , serialize_start + 6 , 2);
	_linkage_type = serialize_start[8];
	
	/* for int */
	if(_linkage_type == 0x0B){

		int	platform_id_data_len_ = 0;
		int	length = 0;
		int	bytes_count = 0;
		int	tmp_count = 0;
		int	plat_id_loop_count = 0;

		/* creat a Linkage_for_ip_mac_notification_table */
		Linkage_for_ip_mac_notification_table *linkage_for_int = new Linkage_for_ip_mac_notification_table();
		_linkage_type_pointer = (void*)linkage_for_int;	

		platform_id_data_len_ = serialize_start[9];
		_set_platform_id_data_len(platform_id_data_len_);

		length = platform_id_data_len_;
		/* starting plat_name_loop_len */
		bytes_count = 13;

		/* compute _platform_id_loop_count */
		for(int i = 0 ; i < platform_id_data_len_ ; i++){

			int	plat_name_loop_len = 0;
			/* compute _platform_id_loop_count still not finish */
			if(length > 0){
		
				plat_name_loop_len = serialize_start[bytes_count];
				length = length - (4 + plat_name_loop_len);	
				bytes_count = bytes_count + (4 + plat_name_loop_len);
				plat_id_loop_count++;
			}	
			/* compute finish */
			else
				break;
		}
		_platform_id_loop_count = plat_id_loop_count;

		/* creat list of linkage_plat_id_data dependenting on _platform_id_loop_count value */
		Linkage_platform_id_data *linkage_plat_id_data_list = new Linkage_platform_id_data[_platform_id_loop_count];
		((Linkage_for_ip_mac_notification_table*)(_linkage_type_pointer))->_platform_id_data_pointer = linkage_plat_id_data_list;

		tmp_count = 10;
		/* creat platform_id list */
		for(int i = 0 ; i < _platform_id_loop_count ; i++){

			uint32_t	plat_id = 0;
			uint8_t		plat_name_len = 0;
				
			memcpy(&plat_id , serialize_start + tmp_count , 3);
			set_platform_id(i , plat_id);	
			tmp_count += 3;
			memcpy(&plat_name_len , serialize_start + tmp_count , 1);
			_set_platform_name_len(i , plat_name_len);
			tmp_count += 1;
			
			if(plat_name_len > 0){
				
				int	plat_name_count = 0;
				Linkage_platform_id_data* tmp = NULL;

				tmp = _get_Linkage_platform_id_data_pointer();	
				/* 34 = 3 bytes of _ISO_369_language_code + 1 byte of _platform_name_length + 30 bytes of _text_char */
				plat_name_count = plat_name_len/34;
				Linkage_platform_name *plat_name_list = new Linkage_platform_name[plat_name_count];
				tmp[i]._platform_name_pointer = plat_name_list;
				memcpy(_get_Linkage_platform_name_pointer(i) , serialize_start + tmp_count , plat_name_len);
				tmp_count += plat_name_len;	
			}
		}
	}
	/* for other types */
	else{

	} 
}

/*
 * _get_platform_id_data_len function
 */
uint8_t
Linkage_descriptor::_get_platform_id_data_len(){

	Linkage_for_ip_mac_notification_table *link_int = NULL;

	link_int = _get_linkage_for_int_pointer(); 
	return link_int->_platform_id_data_length;
}

/*
 * _set_platform_id_data_len function
 * Usage : This function will 'append' length of platform_id_data. 
 */
void
Linkage_descriptor::_set_platform_id_data_len(uint8_t add_plat_id_data_len){

	Linkage_for_ip_mac_notification_table *link_int = NULL;

	link_int = _get_linkage_for_int_pointer(); 
	link_int->_platform_id_data_length += add_plat_id_data_len;
}

/*
 * _get_platform_name_len function
 */
uint8_t
Linkage_descriptor::_get_platform_name_len(int which_plat_id){

	Linkage_platform_id_data *plat_id = NULL;

	plat_id = _get_Linkage_platform_id_data_pointer();	
	return (plat_id[which_plat_id])._platform_name_loop_length;
}

/*
 * _set_platform_name_len function
 */
void
Linkage_descriptor::_set_platform_name_len(int which_plat_id , uint8_t set_plat_name_loop_len){

	Linkage_platform_id_data *plat_id = NULL;

	plat_id = _get_Linkage_platform_id_data_pointer();	
	(plat_id[which_plat_id])._platform_name_loop_length = set_plat_name_loop_len;
}

/*
 * get_linkage_for_int_pointer function
 */
Linkage_for_ip_mac_notification_table*
Linkage_descriptor::_get_linkage_for_int_pointer(){

	return (Linkage_for_ip_mac_notification_table*)_linkage_type_pointer;
}
 
/*
 * get_Linkage_platform_id_data_pointer function
 */
Linkage_platform_id_data*
Linkage_descriptor::_get_Linkage_platform_id_data_pointer(){

	return (((Linkage_for_ip_mac_notification_table*)_linkage_type_pointer)->_platform_id_data_pointer);
}

/*
 * get_private_data_byte_pointer function
 */
u_char*
Linkage_descriptor::_get_private_data_byte_pointer(){

	return (((Linkage_for_ip_mac_notification_table*)_linkage_type_pointer)->_private_data_byte_pointer);
}

/*
 * get_Linkage_platform_name_pointer function
 */
Linkage_platform_name*
Linkage_descriptor::_get_Linkage_platform_name_pointer(int which_plat_id){

	return ((((Linkage_for_ip_mac_notification_table*)_linkage_type_pointer)->_platform_id_data_pointer[which_plat_id])._platform_name_pointer);

}

/*
 * is_match function
 */
bool
Linkage_descriptor::is_match(uint8_t linkage_type){
	
	return (_linkage_type == linkage_type);
}

