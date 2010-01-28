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
#include "correction_message_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Correction_message_descriptor::Correction_message_descriptor(){

	_descriptor_tag = CORRECTION_MESSAGE_DESCRIPTOR;
	_descriptor_length = 1;
	_Burst_time_scaling_pointer = NULL;	
	_Power_control_pointer = NULL;
	_Frequency_correction_pointer = NULL;
}

/*
 * consructor
 */
Correction_message_descriptor::Correction_message_descriptor(u_char set_Time_correction_flag , u_char set_Power_correction_flag , u_char set_Frequency_correction_flag , u_char set_Slot_Type , u_char set_Burst_time_scaling , u_char set_Power_control_flag , u_char set_value){

	int	length_count = 0;

	_descriptor_tag = CORRECTION_MESSAGE_DESCRIPTOR;
	_Time_correction_flag = set_Time_correction_flag;
	_Power_correction_flag = set_Power_correction_flag;
	_Frequency_correction_flag = set_Frequency_correction_flag;
	_Slot_Type = set_Slot_Type;
	_Burst_time_scaling = set_Burst_time_scaling;

	if(_Time_correction_flag == 1){	
		_Burst_time_scaling_pointer = new u_char;
		length_count++;
	}

	if(_Power_correction_flag == 1){
		_Power_control_pointer = new Power_control(set_Power_control_flag , set_value); 	
		length_count++;
	}

	if(_Frequency_correction_flag == 1){
		_Frequency_correction_pointer = new uint16_t;
		length_count = length_count + 2;
	}

	_descriptor_length = length_count + 1;
}

/*
 * destructor
 */
Correction_message_descriptor::~Correction_message_descriptor(){

	if(_Time_correction_flag == 1)	
		delete _Burst_time_scaling_pointer;

	if(_Power_correction_flag == 1)
		delete _Power_control_pointer;

	if(_Frequency_correction_flag == 1)
		delete _Frequency_correction_pointer;
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Correction_message_descriptor
 */
Correction_message_descriptor*
Correction_message_descriptor::descriptor_copy(){

	u_char			set_Time_correction_flag = 0;
	u_char			set_Power_correction_flag = 0;
	u_char			set_Frequency_correction_flag = 0;
	u_char			set_Slot_Type = 0;
	u_char			set_Burst_time_scaling = 0;
	u_char 			set_Power_control_flag = 0;
	u_char			set_value = 0;

	set_Time_correction_flag = _Time_correction_flag;
	set_Power_correction_flag = _Power_correction_flag;
	set_Frequency_correction_flag = _Frequency_correction_flag;
	set_Slot_Type = _Slot_Type;
	set_Burst_time_scaling = _Burst_time_scaling;
	set_Power_control_flag = _Power_control_pointer->_Power_control_flag;
	
	if(set_Power_control_flag == 1)
		set_value = _Power_control_pointer->_Power_correction;

	else
		set_value = _Power_control_pointer->_ExN0;

	Correction_message_descriptor* des_copy = new Correction_message_descriptor(set_Time_correction_flag , set_Power_correction_flag , set_Frequency_correction_flag , set_Slot_Type , set_Burst_time_scaling , set_Power_control_flag , set_value);	
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Correction_message_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	serialize_start[2] &= 0x00;
	serialize_start[2] |= _Time_correction_flag << 7;
	serialize_start[2] |= _Power_correction_flag << 6;
	serialize_start[2] |= _Frequency_correction_flag << 5;
	serialize_start[2] |= _Slot_Type << 3;
	serialize_start[2] |= _Burst_time_scaling; 

	if(_Time_correction_flag == 1){	

		memcpy(serialize_start + 3 , _Burst_time_scaling_pointer , 1);
		
		if(_Power_correction_flag == 1){

			serialize_start[4] &= 0x00;
			serialize_start[4] |= _Power_control_pointer->_Power_control_flag << 7;
			/* if _Power_control_flag = 1 ==> _Power_correction or _Power_control_flag = 0 ==> _ExN0 */
			serialize_start[4] |= _Power_control_pointer->_Power_correction; 

			if(_Frequency_correction_flag == 1)
				memcpy(serialize_start + 5 , _Frequency_correction_pointer , 2);
			
			else{
			}
		}
		else{

			if(_Frequency_correction_flag == 1)
				memcpy(serialize_start + 4 , _Frequency_correction_pointer , 2);
			
			else{
			}
		}
	}

	else{

		if(_Power_correction_flag == 1){

			serialize_start[3] &= 0x00;
			serialize_start[3] |= _Power_control_pointer->_Power_control_flag << 7;
			/* if _Power_control_flag = 1 ==> _Power_correction or _Power_control_flag = 0 ==> _ExN0 */
			serialize_start[3] |= _Power_control_pointer->_Power_correction; 

			if(_Frequency_correction_flag == 1)
				memcpy(serialize_start + 4 , _Frequency_correction_pointer , 2);
			
			else{
			}
		}
		else{

			if(_Frequency_correction_flag == 1)
				memcpy(serialize_start + 3 , _Frequency_correction_pointer , 2);
			
			else{
			}
		}
	}	
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Correction_message_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	_Time_correction_flag = (serialize_start[2] & 0x80)>> 7;
	_Power_correction_flag = (serialize_start[2] & 0x40)>> 6;
	_Frequency_correction_flag = (serialize_start[2] & 0x20)>> 5;
	_Slot_Type = (serialize_start[2] & 0x18)>> 3;
	_Burst_time_scaling = serialize_start[2] & 0x07;

	if(_Time_correction_flag == 1){	

		_Burst_time_scaling_pointer = new u_char();
		memcpy(_Burst_time_scaling_pointer , serialize_start + 3 , 1);

		if(_Power_correction_flag == 1){

			_Power_control_pointer = new Power_control();	
			_Power_control_pointer->_Power_control_flag = (serialize_start[4] & 0x80) >> 7;
			/* if _Power_control_flag = 1 ==> _Power_correction or _Power_control_flag = 0 ==> _ExN0 */
			_Power_control_pointer->_Power_correction = serialize_start[4]& 0x7F;

			if(_Frequency_correction_flag == 1){

				_Frequency_correction_pointer = new uint16_t();
				memcpy(_Frequency_correction_pointer , serialize_start + 5 , 2);
			}
			else{
			}
		}
		else{

			if(_Frequency_correction_flag == 1){

				_Frequency_correction_pointer = new uint16_t();
				memcpy(_Frequency_correction_pointer , serialize_start + 5 , 2);
			}
			else{
			}
		}
	}
	else{

		if(_Power_correction_flag == 1){

			_Power_control_pointer = new Power_control();	
			_Power_control_pointer->_Power_control_flag = (serialize_start[3] & 0x80) >> 7;
			/* if _Power_control_flag = 1 ==> _Power_correction or _Power_control_flag = 0 ==> _ExN0 */
			_Power_control_pointer->_Power_correction = serialize_start[3]& 0x7F;

			if(_Frequency_correction_flag == 1){

				_Frequency_correction_pointer = new uint16_t();
				memcpy(_Frequency_correction_pointer , serialize_start + 4 , 2);
			}
			else{
			}
		}
		else{

			if(_Frequency_correction_flag == 1){

				_Frequency_correction_pointer = new uint16_t();
				memcpy(_Frequency_correction_pointer , serialize_start + 4 , 2);
			}
			else{
			}
		}
	}	
}
