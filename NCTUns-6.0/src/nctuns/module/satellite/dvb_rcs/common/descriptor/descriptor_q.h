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

#ifndef __NCTUNS_descriptor_q_h__
#define __NCTUNS_descriptor_q_h__

#include <mylist.h>
#include "descriptor.h"

/*
 * define descriptor identification 
 *
 * Network_name_descriptor			descriptor_tag
 * Linkage_descriptor				descriptor_tag		linkage_type
 * Data_broadcast_id_descriptor			descriptor_tag		data_broadcast_id
 * Stream_identifier_descriptor 		descriptor_tag		component_tag
 * Ip_mac_platform_name_descriptor		descriptor_tag		
 * Ip_mac_platform_provider_name_descriptor	descriptor_tag
 * Target_ip_slash_descriptor			descriptor_tag		rcst_ip_address
 * Ip_mac_stream_location_descriptor		descriptor_tag		component_tag
 */

/* 
 * Define the entry of descriptor queue. 
 */
struct Descriptor_entry {

	CIRCLEQ_ENTRY(Descriptor_entry)		entries;
	Descriptor				*descriptor;
	
	/* 
	 * Constructors
	 */
	Descriptor_entry();
	Descriptor_entry(Descriptor* des) { descriptor = des; }

	/* Destructor */
	~Descriptor_entry(){ free(); }

	inline void				free();
};
 
void Descriptor_entry::free() {

        delete descriptor;
}

struct Descriptor_circleq {

	struct Descriptor_entry		*cqh_first;		// first element
	struct Descriptor_entry		*cqh_last;		// last element
	
	int				location_counter;
	int				descriptor_count;
	/*
	 * function
	 */
	Descriptor_circleq();
	~Descriptor_circleq();
	void				free();
	int				add_descriptor(Descriptor* added_descriptor);
	Descriptor*			get_descriptor(int pnum , uint8_t searched_des , ...);		
	int				remove_descriptor(int pnum , uint8_t deleted_des , ...);
	int				get_location_num(int pnum , uint8_t searched_des , ...);
	Descriptor*			get_descriptor_by_location_num(int loc_num);
	int				get_all_descriptor_total_length();
	Descriptor*			get_sequencing_descriptor(int sequence);
	Descriptor_circleq*		copy();

 private:
	Descriptor_entry*		get_des_entry(int pnum , uint8_t searched_des , ...);		
};

#endif /* __NCTUNS_descriptor_q_h__ */
