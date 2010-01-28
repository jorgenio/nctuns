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

#ifndef __NCTUNS_contentin_control_descriptor_h__
#define __NCTUNS_contentin_control_descriptor_h__

#include "descriptor.h"

class Contention_control_descriptor : public Descriptor {
	
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
	uint8_t					_Superframe_ID;
	uint32_t				_CSC_response_timeout;
	uint8_t					_CSC_max_losses;
	uint32_t				_Max_time_before_retry;

 /*
  * public functions
  */
 public:
	Contention_control_descriptor();
	Contention_control_descriptor(uint8_t , uint32_t, uint8_t, uint32_t); 
	~Contention_control_descriptor();
	Contention_control_descriptor*		descriptor_copy();
	void					descriptor_serialized(u_char*);
	void					descriptor_deserialized(u_char*);
	bool					is_match(uint8_t);
	void					show_descriptor();
	
	void		set_superframe_id(uint8_t s_id) { _Superframe_ID = s_id; }
	void		set_csc_response_timeout(uint32_t csc_res_timeout) { _CSC_response_timeout = csc_res_timeout; }
	void		set_csc_max_losses(uint8_t csc_max_losses) { _CSC_max_losses = csc_max_losses; }
	void		set_max_time_before_retry(uint32_t max_time_before_retry) { _Max_time_before_retry = max_time_before_retry;}
	uint8_t		get_superframe_id() { return _Superframe_ID; }
	uint32_t	get_csc_response_timeout() { return _CSC_response_timeout; }
	uint8_t		get_csc_max_losses() { return _CSC_max_losses; }
	uint32_t	get_max_time_before_retry() { return _Max_time_before_retry; }
};

#endif /* __NCTUNS_contentin_control_descriptor_h__ */

