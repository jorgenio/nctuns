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

#ifndef __NCTUNS_data_broadcast_id_descriptor_h__
#define __NCTUNS_data_broadcast_id_descriptor_h__

#include "descriptor.h"

class Platform_id_info {

 friend class Data_broadcast_id_descriptor;
 friend class Ip_mac_notification_info;

 private:
	uint32_t	_platform_id		:24;
	uint32_t	_action_type		:8;
	uint8_t		_reserved		:2;
	uint8_t		_INT_versioning_flag	:1;
	uint8_t		_INT_version		:5;
};

class Ip_mac_notification_info {

 friend class Data_broadcast_id_descriptor;

 private:
	uint8_t			_platform_id_data_length;
	Platform_id_info	*_platform_id_info_pointer;
	void*			*_private_data_byte_pointer;
};

class Data_broadcast_id_descriptor : public Descriptor {
	
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
	uint16_t	_data_broadcast_id; 
	void		*_id_selector_byte_pointer;

	/* by programmer defined */
	int		_platform_id_info_count;

	/*
	 * private function
	 */
	Platform_id_info*	_get_platform_id_info_pointer();
	
 /*
  * public function
  */
 public:
	Data_broadcast_id_descriptor(); 
	Data_broadcast_id_descriptor(uint16_t , int); 
	~Data_broadcast_id_descriptor(); 
	Data_broadcast_id_descriptor*		descriptor_copy();
	void					descriptor_serialized(u_char*);
	void					descriptor_deserialized(u_char*);

	void					set_Platform_id_info(int , uint32_t , uint32_t , uint8_t , uint8_t);
	uint32_t				get_platform_id(int);
	uint32_t				get_action_type(int);
	uint8_t					get_INT_versioning_flag(int);
	uint8_t					get_INT_version(int);
	bool					is_match(uint16_t);
	void					show_descriptor();
};

#endif /* __NCTUNS_data_broadcast_id_descriptor_h__ */

