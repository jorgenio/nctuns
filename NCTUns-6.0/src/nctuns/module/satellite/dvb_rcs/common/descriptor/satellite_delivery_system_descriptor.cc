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
#include "satellite_delivery_system_descriptor.h"
#include "common.h"

/*
 * default consructor
 */
Satellite_delivery_system_descriptor::Satellite_delivery_system_descriptor(){

	_descriptor_tag = SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR;
	_descriptor_length = SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_LENGTH;
	_modulation_system = DVB_S2;
}

/*
 * consructor
 */
Satellite_delivery_system_descriptor::Satellite_delivery_system_descriptor(char* set_frequency__ , char* set_orbital_position__ , uint8_t set_west_east_flag , uint8_t set_polarization , uint8_t set_roll_off , uint8_t set_modulation_system , uint8_t set_modulation_type , char* set_symbol_rate__ , uint32_t set_fec_inner){

	_descriptor_tag = SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR;
	_descriptor_length = SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_LENGTH;
	_modulation_system = DVB_S2;

	if(set_frequency__ != NULL && (strlen(set_frequency__) == 8))
		set_frequency(set_frequency__);

	else
		printf("warning ==> frequency setting is wrong (is NULL or string length not equal to 8)\n");

	if(set_orbital_position__ != NULL && (strlen(set_orbital_position__) == 4))
		set_orbital_position(set_orbital_position__);
	else
                printf("warning ==> orbital_position setting is wrong (is NULL or string length not equal to 4)\n");

	_west_east_flag = set_west_east_flag;
	_polarization = set_polarization;
	_roll_off = set_roll_off;
	_modulation_type = set_modulation_type;

	if(set_symbol_rate__ != NULL && (strlen(set_symbol_rate__) == 7))
		set_symbol_rate(set_symbol_rate__);

	else
                printf("warning ==> symbol_rate setting is wrong (is NULL or string length not equal to 7)\n");

	_FEC_inner = set_fec_inner;
}

/*
 * consructor for copying use
 */
Satellite_delivery_system_descriptor::Satellite_delivery_system_descriptor(uint32_t set_frequency__ , uint16_t set_orbital_position__ , uint8_t set_west_east_flag , uint8_t set_polarization , uint8_t set_roll_off , uint8_t set_modulation_system , uint8_t set_modulation_type , uint32_t set_symbol_rate__ , uint32_t set_fec_inner){

	_descriptor_tag = SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR;
	_descriptor_length = SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_LENGTH;
	_frequency = set_frequency__;
	_orbital_position = set_orbital_position__;
	_west_east_flag = set_west_east_flag;
	_polarization = set_polarization;
	_roll_off = set_roll_off;
	_modulation_system = DVB_S2;
	_modulation_type = set_modulation_type;
	_symbol_rate = set_symbol_rate__;
	_FEC_inner = set_fec_inner;
}

/*
 * destructor
 */
Satellite_delivery_system_descriptor::~Satellite_delivery_system_descriptor(){

}

/*
 * set_frequency function 
 * The frequency is a 32-bit field giving the 4-bit BCD values specifying 8 characters 
 * of the frequency value. For the satellite_delivery_system_descriptor the frequency  
 * is coded in GHz, where the decimal point occurs after the third character
 * (e.g. 011.75725)
 * By this function, we can set frequency value. 
 */
void
Satellite_delivery_system_descriptor::set_frequency(char* set_string){

	_frequency = string_to_bcd(set_string , FREQUENCY_SIZE);
}

/*
 * get_frequency function 
 * By this function, we can get frequency value. 
 */
void
Satellite_delivery_system_descriptor::get_frequency(char* return_str){

	bcd_to_string(return_str , (uint32_t)_frequency , FREQUENCY_SIZE);
}

/*
 * set_orbital_positioni function 
 * the same with set_frequency function
 * (e.g. 019.2')
 * By this function, we can set orbital_position value. 
 */
void
Satellite_delivery_system_descriptor::set_orbital_position(char* set_string){

	_orbital_position = string_to_bcd(set_string , ORBITAL_POSITION_SIZE);
}

/*
 * get_orbital_position function 
 * the same with get_frequency function
 * By this function, we can get orbital_position value. 
 */
void
Satellite_delivery_system_descriptor::get_orbital_position(char* return_str){
	
	bcd_to_string(return_str , (uint32_t)_orbital_position , ORBITAL_POSITION_SIZE);
}

/*
 * set_symbol_rate function 
 * the same with set_frequency
 * (e.g. 027.4500)
 * By this function, we can set symbol_rate value. 
 */
void
Satellite_delivery_system_descriptor::set_symbol_rate(char* set_string){

	_symbol_rate = string_to_bcd(set_string , SYMBOL_RATE_SIZE);
}

/*
 * get_symbol_rate function 
 * By this function, we can get symbol_rate value.  
 */
void
Satellite_delivery_system_descriptor::get_symbol_rate(char* return_str){

	bcd_to_string(return_str , (uint32_t)(_symbol_rate), SYMBOL_RATE_SIZE);
}

/*
 * descriptor_copy function
 * Usage : Carbon copy the original Satellite_delivery_system_descriptor
 */
Satellite_delivery_system_descriptor*
Satellite_delivery_system_descriptor::descriptor_copy(){
	
	Satellite_delivery_system_descriptor *des_copy = new Satellite_delivery_system_descriptor(_frequency , _orbital_position , _west_east_flag , _polarization , _roll_off , _modulation_system , _modulation_type , _symbol_rate , _FEC_inner);
	
	return(des_copy);
}

/*
 * descriptor_serialized function
 * Usage : Searialize (copy all content of descriptor into continuous memory space)
 */
void
Satellite_delivery_system_descriptor::descriptor_serialized(u_char* serialize_start){

	serialize_start[0] = _descriptor_tag;
	serialize_start[1] = _descriptor_length;
	memcpy(serialize_start + 2 , &_frequency , 4);
	memcpy(serialize_start + 6 , &_orbital_position , 2);
	serialize_start[8] &= 0x00;
	serialize_start[8] |= _west_east_flag << 7;
	serialize_start[8] |= _polarization << 5;
	serialize_start[8] |= _roll_off << 3;
	serialize_start[8] |= _modulation_system << 2;
	serialize_start[8] |= _modulation_type;
	
	serialize_start[9] = (_symbol_rate & 0xFF00000) >> 20;
	serialize_start[10] = (_symbol_rate & 0x00FF000) >> 12;
	serialize_start[11] = (_symbol_rate & 0x0000FF0) >> 4;

	serialize_start[12] &= 0x00;
	serialize_start[12] |= _symbol_rate << 4;
	serialize_start[12] |= _FEC_inner;
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 */
void
Satellite_delivery_system_descriptor::descriptor_deserialized(u_char* serialize_start){

	_descriptor_tag = serialize_start[0];
	_descriptor_length = serialize_start[1];
	memcpy(&_frequency , serialize_start + 2 , 4);
	memcpy(&_orbital_position , serialize_start + 6 , 2);
	_west_east_flag = serialize_start[8] >> 7;
	_polarization = serialize_start[8] >> 5;
	_roll_off = serialize_start[8] >> 3;
	_modulation_system = serialize_start[8] >> 2;
	_modulation_type = serialize_start[8];
	_symbol_rate &= 0x00;
	_symbol_rate |= serialize_start[9] << 20;
	_symbol_rate |= serialize_start[10] << 12;
	_symbol_rate |= serialize_start[11] << 4;
	_symbol_rate |= (serialize_start[12] & 0xF0) >> 4;
	_FEC_inner |= (serialize_start[12] & 0x0F); 
}
