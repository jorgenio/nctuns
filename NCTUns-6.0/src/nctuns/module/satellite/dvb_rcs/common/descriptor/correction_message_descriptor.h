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

#ifndef __NCTUNS_correction_message_descriptor_h__
#define __NCTUNS_correction_message_descriptor_h__

#include "descriptor.h"

class Power_control {

 friend class Correction_message_descriptor;
 friend class Section;

 private:
	u_char			_Power_control_flag	:1;

	union {
		u_char		_Power_correction	:7;
		u_char		_ExN0			:7;
	};
 private:
	inline Power_control();
	inline Power_control(u_char , u_char);
};

Power_control::Power_control(){

	_Power_control_flag = 0;
	_ExN0 = 0;
}

Power_control::Power_control(u_char set_Power_control_flag , u_char set_value){

	_Power_control_flag = set_Power_control_flag;
	
	if(_Power_control_flag == 1)
		_Power_correction = set_value;

	else
		_ExN0 = set_value;
}

class Correction_message_descriptor : public Descriptor {
	
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
	u_char			_Time_correction_flag		:1;
	u_char			_Power_correction_flag		:1;
	u_char			_Frequency_correction_flag	:1;
	u_char			_Slot_Type			:2;
	u_char			_Burst_time_scaling		:3;

	u_char			*_Burst_time_scaling_pointer;
	Power_control		*_Power_control_pointer;
	uint16_t		*_Frequency_correction_pointer;		
	
 /*
  * public functions
  */
 public:
	Correction_message_descriptor(); 
	Correction_message_descriptor(u_char, u_char , u_char , u_char , u_char , u_char , u_char); 
	~Correction_message_descriptor(); 
	Correction_message_descriptor*		descriptor_copy();
	void					descriptor_serialized(u_char*);
	void					descriptor_deserialized(u_char*);
};

#endif /* __NCTUNS_correction_message_descriptor_h__ */

