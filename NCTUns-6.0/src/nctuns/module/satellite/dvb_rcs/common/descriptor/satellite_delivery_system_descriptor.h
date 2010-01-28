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

#ifndef __NCTUNS_satellite_delivery_system_descriptor_h__
#define __NCTUNS_satellite_delivery_system_descriptor_h__

#include "descriptor.h"

#define FREQUENCY_SIZE		8
#define ORBITAL_POSITION_SIZE	4
#define SYMBOL_RATE_SIZE	7


class Satellite_delivery_system_descriptor : public Descriptor {
	
 /*
  * We use 'friend' mechanism to let other classes can use the private
  * memebers of class Descriptor
  */
 friend class Section;

 private:
	/*
	 * all fields are private member
	 * at first two field are 
 	 * descriptor_tag
	 * descriptor_length
	 */
	uint32_t		_frequency; 
	uint16_t		_orbital_position; 

	uint8_t			_west_east_flag		:1;
	uint8_t			_polarization		:2;
	uint8_t			_roll_off		:2;	// used for DVB_S2
	uint8_t			_modulation_system	:1;	// 0 indicate DVB_S, 1 indicate DVB_S2
	uint8_t			_modulation_type	:2;

	uint32_t		_symbol_rate		:28;
	uint32_t		_FEC_inner		:4;

 /*
  * public functions
  */
 public:
	Satellite_delivery_system_descriptor(char* , char* , uint8_t , uint8_t , uint8_t , uint8_t , uint8_t , char* , uint32_t); 
	Satellite_delivery_system_descriptor(uint32_t , uint16_t , uint8_t , uint8_t , uint8_t , uint8_t , uint8_t , uint32_t , uint32_t); 
	Satellite_delivery_system_descriptor(); 
	~Satellite_delivery_system_descriptor(); 
	Satellite_delivery_system_descriptor*	descriptor_copy();
	void					descriptor_serialized(u_char*);
	void					descriptor_deserialized(u_char*);

	void					set_frequency(char*);
	void					get_frequency(char*);
	void					set_orbital_position(char*);
	void					get_orbital_position(char*);
	void					set_symbol_rate(char*);
	void					get_symbol_rate(char*);
};

#endif /* __NCTUNS_satellite_delivery_system_descriptor_h__ */

