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

#include <stdarg.h>
#include <assert.h>
#include "descriptor_q.h"

/* for NIT */
#include "network_name_descriptor.h"
#include "linkage_descriptor.h"
#include "satellite_delivery_system_descriptor.h"
/* for PMT */
#include "data_broadcast_id_descriptor.h"
#include "stream_identifier_descriptor.h"
#include "adaptation_field_data_descriptor.h"
#include "service_move_descriptor.h"
/* for INT */
#include "ip_mac_platform_name_descriptor.h"
#include "ip_mac_platform_provider_name_descriptor.h"
#include "target_mac_address_descriptor.h"
#include "target_ip_slash_descriptor.h"
#include "ip_mac_stream_location_descriptor.h"
/* for TIM */
#include "correction_message_descriptor.h"
#include "logon_initialize_descriptor.h"
#include "contention_control_descriptor.h"
#include "sync_assign_descriptor.h"
#include "table_update_descriptor.h"
#include "rcs_content_descriptor.h"
#include "common.h"

/*
 * constructor
 */
Descriptor_circleq::Descriptor_circleq() {
	
	descriptor_count = 0;
	CIRCLEQ_INIT(this);
}

/*
 * destructor
 */
Descriptor_circleq::~Descriptor_circleq() {
	free();
}

/*
 * free function 
 */
void
Descriptor_circleq::free() {

	Descriptor_entry *des_entry = NULL;	

	while (cqh_first != (Descriptor_entry*)this) {

		des_entry = cqh_first;
		CIRCLEQ_REMOVE(this , des_entry , entries);

		switch ((des_entry->descriptor)->_descriptor_tag)
		{

			case (NETWORK_NAME_DESCRIPTOR):{ 
				delete ((Network_name_descriptor*) des_entry->descriptor);
				break;
			}
			case (LINKAGE_DESCRIPTOR):{
				delete ((Linkage_descriptor*) des_entry->descriptor);
				break;
			}
			case (DATA_BROADCAST_ID_DESCRIPTOR):{
				delete ((Data_broadcast_id_descriptor*) des_entry->descriptor);
				break;
			} case (STREAM_IDENTIFIER_DESCRIPTOR):{
				delete ((Stream_identifier_descriptor*) des_entry->descriptor);
				break;
			}
			case(IP_MAC_PLATFORM_NAME_DESCRIPTOR):{ 
				delete ((Ip_mac_platform_name_descriptor*) des_entry->descriptor);
				break;
			}
			case(IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR):{ 
				delete ((Ip_mac_platform_provider_name_descriptor*) des_entry->descriptor);
				break;
			}
			case(TARGET_IP_SLASH_DESCRIPTOR):{ 
				delete ((Target_ip_slash_descriptor*) des_entry->descriptor);
				break;
			}
			case(IP_MAC_STREAM_LOCATION_DESCRIPTOR):{ 
				delete ((Ip_mac_stream_location_descriptor*) des_entry->descriptor);
				break;
			}
			case(LOGON_INITIALIZE_DESCRIPTOR):{
				delete ((Logon_initialize_descriptor*) des_entry->descriptor);
				break;
			}
			case(CONTENTION_CONTROL_DESCRIPTOR):{
				delete ((Contention_control_descriptor*) des_entry->descriptor);
				break;
			}
			default:
				assert(0);
		}	

		des_entry->descriptor = NULL;
		delete (des_entry);
	}
}

/*
 * add_descriptor function 
 */
int
Descriptor_circleq::add_descriptor(Descriptor *added_des) {

	Descriptor_entry *new_descriptor_entry = NULL;

	/* clone original descriptor and insert it to table */
	new_descriptor_entry = new Descriptor_entry(added_des->descriptor_copy());
	
	if (!new_descriptor_entry)
			return -1;

	/* insert this descriptor_entry into head of descriptor circleq */
	else {
		CIRCLEQ_INSERT_HEAD(this , new_descriptor_entry , entries);
		descriptor_count++;
		return 0;
	}
}

/*
 * get_descriptor function 
 * assume only one descriptor to be searched
 */
Descriptor*
Descriptor_circleq::get_descriptor(int pnum , uint8_t searched_des , ...) {

	Descriptor_entry *des_entry = NULL;
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	
	va_start(parg , searched_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			des_entry = get_des_entry(pnum , searched_des);
			break;
		}
		/* 2 parameters */
		case (2):{

			p1 = va_arg(parg , u_long);
			des_entry = get_des_entry(pnum , searched_des , p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			des_entry = get_des_entry(pnum , searched_des , p1 , p2);
			break;
		}		
		/* wrong input */
		default:{

			va_end(parg);	
			return NULL;
		}
	}
	va_end(parg);	
		
	if (!des_entry)
		return NULL;

	/* return clone descriptor */
	else
		return ((des_entry->descriptor)->descriptor_copy());
}

/*
 * remove_descriptor function 
 */
int
Descriptor_circleq::remove_descriptor(int pnum , uint8_t deleted_des , ...) {

	Descriptor_entry	*rm_des_entry = NULL;
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	
	va_start(parg , deleted_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			rm_des_entry = get_des_entry(pnum , deleted_des);
			break;
		}
		/* 2 parameters */
		case (2):{
			p1 = va_arg(parg , u_long);
			rm_des_entry = get_des_entry(pnum , deleted_des , p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			rm_des_entry = get_des_entry(pnum , deleted_des , p1 , p2);
			break;
		}	
		/* wrong input */
		default:{

			va_end(parg);	
			return -1;
		}
	}
	va_end(parg);	

	if (rm_des_entry){

		CIRCLEQ_REMOVE(this , rm_des_entry , entries);
		delete(rm_des_entry);
		descriptor_count--;
		return 0;
	}
	else
		return -1;

}
/*
 * get_des_entry function 
 * pnum is parameter number of input
 * default possible pnum is 1, 2, 3
 */
Descriptor_entry*
Descriptor_circleq::get_des_entry(int pnum , uint8_t searched_des , ...) {

	Descriptor_entry *tmp = NULL;
	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	location_counter = 0;
	
	va_start(parg , searched_des);

	if (pnum == 2)
		p1 = va_arg(parg , u_long);
	
	if (pnum == 3) {

		p1 = va_arg(parg , u_long);
		p2 = va_arg(parg , u_long);
	}
	va_end(parg);	

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* descriptor type matched */
		if ((tmp->descriptor)->_descriptor_tag == searched_des) {

			/* descriptor matched */
			switch ((tmp->descriptor)->_descriptor_tag){

				case (NETWORK_NAME_DESCRIPTOR):{ 

					return tmp;
					break;
				}
				case (LINKAGE_DESCRIPTOR):{

					if (((Linkage_descriptor*)(tmp->descriptor))->is_match((uint8_t)p1))
						return tmp;
					
					break;
				}
				case (DATA_BROADCAST_ID_DESCRIPTOR):{

					if (((Data_broadcast_id_descriptor*)(tmp->descriptor))->is_match((uint16_t)p1))
						return tmp;
					
					break;
				} case (STREAM_IDENTIFIER_DESCRIPTOR):{

					if (((Stream_identifier_descriptor*)(tmp->descriptor))->is_match((uint8_t)p1)){
						return tmp;
					}
					
					break;
				}
				case(IP_MAC_PLATFORM_NAME_DESCRIPTOR):{ 

					return tmp;
					break;
				}
				case(IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR):{ 

					return tmp;
					break;
				}
				case(TARGET_IP_SLASH_DESCRIPTOR):{ 

					if (((Target_ip_slash_descriptor*)(tmp->descriptor))->is_match(p1))
						return tmp;
					
					break;
				}
				case(IP_MAC_STREAM_LOCATION_DESCRIPTOR):{ 

					if (((Ip_mac_stream_location_descriptor*)(tmp->descriptor))->is_match(p1))
						return tmp;

					break;
				}
				case(LOGON_INITIALIZE_DESCRIPTOR):{
						return tmp;

					break;
				}
				case(CONTENTION_CONTROL_DESCRIPTOR):{
					if(((Contention_control_descriptor*)(tmp->descriptor))->is_match(p1))
						return tmp;
					break;
				}
				default:
					return NULL;
			}	
		}
		location_counter++;
	}
	return NULL;
}

/*
 * get_location_num function 
 * first element in circleq is numbered as 0
 */
int
Descriptor_circleq::get_location_num(int pnum , uint8_t searched_des , ...) {

	va_list parg;
	u_long p1 = 0;
	u_long p2 = 0;
	
	va_start(parg , searched_des);

	switch (pnum) {

		/* 1 parameter */
		case (1):{

			get_des_entry(pnum , searched_des);
			break;
		}
		/* 2 parameters */
		case (2):{

			p1 = va_arg(parg , u_long);
			get_des_entry(pnum , searched_des , p1);
			break;
		}
		/* 3 parameters */
		case (3):{

			p1 = va_arg(parg , u_long);
			p2 = va_arg(parg , u_long);
			get_des_entry(pnum , searched_des , p1 , p2);
			break;
		}	
		/* wrong input */
		default:{

			va_end(parg);	
			return -1;
		}
	}
	va_end(parg);	
	return location_counter;
}

/*
 * get_descriptor_by_location_num function 
 */
Descriptor*
Descriptor_circleq::get_descriptor_by_location_num(int loc_num){

	Descriptor_entry *tmp = NULL;
	int count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries){
	
		if (count == loc_num)
			return (tmp->descriptor)->descriptor_copy();

		count++;
	}
	return NULL;
}

/*
 * get_all_descriptor_total_length function 
 */
int
Descriptor_circleq::get_all_descriptor_total_length(){
	
	int			descriptors_total_length = 0;
	int 			count = 0;
	Descriptor_entry 	*tmp;

	CIRCLEQ_FOREACH(tmp , this , entries){
	
		if (count == descriptor_count)
			break;

		descriptors_total_length += (tmp->descriptor)->get_descriptor_total_len();
		count++;
	}
	return descriptors_total_length;
}

/*
 * copy function 
 */
Descriptor_circleq*
Descriptor_circleq::copy() {

	Descriptor *tmp;

	Descriptor_circleq *circleq_cpy = new Descriptor_circleq();	

	for (int i = descriptor_count - 1 ; i >= 0 ; i--) {

		tmp = get_descriptor_by_location_num(i);
		circleq_cpy->add_descriptor(tmp);

		// Delete the descriptor copy.
		switch ((tmp)->_descriptor_tag)
		{

			case (NETWORK_NAME_DESCRIPTOR):{ 
				delete ((Network_name_descriptor*) tmp);
				break;
			}
			case (LINKAGE_DESCRIPTOR):{
				delete ((Linkage_descriptor*) tmp);
				break;
			}
			case (DATA_BROADCAST_ID_DESCRIPTOR):{
				delete ((Data_broadcast_id_descriptor*) tmp);
				break;
			} case (STREAM_IDENTIFIER_DESCRIPTOR):{
				delete ((Stream_identifier_descriptor*) tmp);
				break;
			}
			case(IP_MAC_PLATFORM_NAME_DESCRIPTOR):{ 
				delete ((Ip_mac_platform_name_descriptor*) tmp);
				break;
			}
			case(IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR):{ 
				delete ((Ip_mac_platform_provider_name_descriptor*) tmp);
				break;
			}
			case(TARGET_IP_SLASH_DESCRIPTOR):{ 
				delete ((Target_ip_slash_descriptor*) tmp);
				break;
			}
			case(IP_MAC_STREAM_LOCATION_DESCRIPTOR):{ 
				delete ((Ip_mac_stream_location_descriptor*) tmp);
				break;
			}
			case(LOGON_INITIALIZE_DESCRIPTOR):{
				delete ((Logon_initialize_descriptor*) tmp);
				break;
			}
			case(CONTENTION_CONTROL_DESCRIPTOR):{
				delete ((Contention_control_descriptor*) tmp);
				break;
			}
			default:
				assert(0);
		}	
	}
	return circleq_cpy;
}

/*
 * get_sequencing_descriptor function
 */
Descriptor*
Descriptor_circleq::get_sequencing_descriptor(int sequence) {

	Descriptor_entry *tmp;
	int seq_count = 0;

	CIRCLEQ_FOREACH(tmp , this , entries) {

		/* sct sequence matched */
		if (seq_count == sequence) {

			return (tmp->descriptor)->descriptor_copy();
		}
		seq_count++;
	}
	return NULL;
}
