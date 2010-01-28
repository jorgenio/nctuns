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

#ifndef __NCTUNS_ip_mac_stream_location_descriptor_h__
#define __NCTUNS_ip_mac_stream_location_descriptor_h__

#include "descriptor.h"

class Ip_mac_stream_location_descriptor : public Descriptor {

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
	uint16_t					_network_id;
	uint16_t					_original_network_id;
	uint16_t					_transport_stream_id;
	uint16_t					_service_id;
	uint8_t						_component_tag;	

 /*
  * public functions
  */
 public:	
	Ip_mac_stream_location_descriptor();
	Ip_mac_stream_location_descriptor(uint16_t , uint16_t , uint16_t , uint16_t , uint8_t); 
	~Ip_mac_stream_location_descriptor(); 
	Ip_mac_stream_location_descriptor*		descriptor_copy();
	uint16_t					get_transport_stream_id() { return _transport_stream_id; }
	uint16_t					get_service_id() { return _service_id; }
	uint8_t						get_component_tag() { return _component_tag; }
	void						descriptor_serialized(u_char*);
	void						descriptor_deserialized(u_char*);
	bool						is_match(uint8_t);
	void						show_descriptor();
};

#endif /* __NCTUNS_ip_mac_stream_location_descriptor_h__ */

