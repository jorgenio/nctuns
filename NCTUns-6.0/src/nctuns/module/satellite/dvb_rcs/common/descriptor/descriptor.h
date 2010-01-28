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

#ifndef __NCTUNS_descriptor_h__
#define __NCTUNS_descriptor_h__

#include <stdint.h>
#include <netinet/in.h>
//#include "contention_control_descriptor.h"

#pragma pack(1)
class Descriptor {

 /*
  * We use 'friend' mechanism to let other classes can use the private
  * memebers of class Descriptor
  */
 friend class Section;
 friend struct Descriptor_circleq;

 /*
  * private members
  */

 protected:	
	uint8_t		_descriptor_tag;
	uint8_t		_descriptor_length;

 /*
  * public functions
  */
 public:
	Descriptor();
	~Descriptor();
	int			get_descriptor_total_len();
	int			descriptor_serialize(u_char*);
	static Descriptor*	descriptor_deserialize(u_char* , int*);
	Descriptor*		descriptor_copy();
	void			show_descriptor();
	inline uint8_t		get_descriptor_tag() { return _descriptor_tag; };
};
#pragma pack()

#endif /* __NCTUNS_descriptor_h__ */

