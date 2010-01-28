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

#ifndef __NCTUNS_target_mac_address_descriptor_h__
#define __NCTUNS_target_mac_address_descriptor_h__

#include <ethernet.h>
#include "descriptor.h"

#define	MAC_ADDR_SIZE	6

enum {
	EXIST		= 1,
	NON_EXIST	= 0
};

class Target_mac_address_descriptor : public Descriptor {
	
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
	struct ether_addr	_MAC_addr_mask;
	struct ether_addr	*_MAC_addr_list_pointer;
	int			_get_mask_num();
	int			_get_mac_addr_num();
 /*
  * public functions
  */
 public:
	Target_mac_address_descriptor();
	Target_mac_address_descriptor(struct ether_addr , int , struct ether_addr*);
	Target_mac_address_descriptor(int , Target_mac_address_descriptor* , int , struct ether_addr*);
	~Target_mac_address_descriptor();
	void					set_MAC_addr_mask(struct ether_addr);
	struct ether_addr			get_MAC_addr_mask();
	bool					lookup_MAC_addr_list(struct ether_addr);
	Target_mac_address_descriptor*		descriptor_copy();
	void					descriptor_serialized(u_char*);
	void					descriptor_deserialized(u_char*);
};

#endif /* __NCTUNS_target_mac_address_descriptor_h__ */

