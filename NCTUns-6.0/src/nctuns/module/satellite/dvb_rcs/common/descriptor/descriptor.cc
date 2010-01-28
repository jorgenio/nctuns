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

#include "descriptor.h"
#include "contention_control_descriptor.h"
#include "sync_assign_descriptor.h"
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
#include "table_update_descriptor.h"
#include "rcs_content_descriptor.h"
#include "common.h"
#include <assert.h>

/*
 * consructor
 */
Descriptor::Descriptor(){

	_descriptor_tag = RESERVED;
	_descriptor_length = RESERVED_LENGTH;
}

/*
 * destructor
 */
Descriptor::~Descriptor(){

}

/*
 * get_descriptor_total_len function
 * Usage : By this function, we can get total length of any descriptor needed 
 *         in which, total length is equal to 'descriptor length' + 2
 */
int
Descriptor::get_descriptor_total_len(){

	return _descriptor_length + 2;	
}

/*
 * descriptor_copy function
 */
Descriptor*
Descriptor::descriptor_copy(){

	switch(_descriptor_tag){

		case(NETWORK_NAME_DESCRIPTOR): 
			return (((Network_name_descriptor*)this)->descriptor_copy());
			break;

		case(LINKAGE_DESCRIPTOR): 
			return (((Linkage_descriptor*)this)->descriptor_copy());
			break;

		case(SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR): 
			return (((Satellite_delivery_system_descriptor*)this)->descriptor_copy());
			break;

		case(DATA_BROADCAST_ID_DESCRIPTOR): 
			return (((Data_broadcast_id_descriptor*)this)->descriptor_copy());
			break;

		case(STREAM_IDENTIFIER_DESCRIPTOR): 
			return (((Stream_identifier_descriptor*)this)->descriptor_copy());
			break;

		case(ADAPTATION_FIELD_DATA_DESCRIPTOR): 
			return (((Adaptation_field_data_descriptor*)this)->descriptor_copy());
			break;

		case(SERVICE_MOVE_DESCRIPTOR): 
			return (((Service_move_descriptor*)this)->descriptor_copy());
			break;

		case(IP_MAC_PLATFORM_NAME_DESCRIPTOR): 
			return (((Ip_mac_platform_name_descriptor*)this)->descriptor_copy());
			break;

		case(IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR): 
			return (((Ip_mac_platform_provider_name_descriptor*)this)->descriptor_copy());
			break;

		case(TARGET_MAC_ADDRESS_DESCRIPTOR): 
			return (((Target_mac_address_descriptor*)this)->descriptor_copy());
			break;

		case(TARGET_IP_SLASH_DESCRIPTOR): 
			return (((Target_ip_slash_descriptor*)this)->descriptor_copy());
			break;

		case(IP_MAC_STREAM_LOCATION_DESCRIPTOR): 
			return (((Ip_mac_stream_location_descriptor*)this)->descriptor_copy());
			break;

		case(CORRECTION_MESSAGE_DESCRIPTOR): 
			return (((Correction_message_descriptor*)this)->descriptor_copy());
			break;

		case(CONTENTION_CONTROL_DESCRIPTOR): 
			return (((Contention_control_descriptor*)this)->descriptor_copy());
			break;

		case(LOGON_INITIALIZE_DESCRIPTOR):
			return (((Logon_initialize_descriptor*)this)->descriptor_copy());
			break;

		case(SYNC_ASSIGN_DESCRIPTOR): 
			return (((Sync_assign_descriptor*)this)->descriptor_copy());
			break;

		case(RCS_CONTENT_DESCRIPTOR): 
			return (((Rcs_content_descriptor*)this)->descriptor_copy());
			break;

		case(TABLE_UPDATE_DESCRIPTOR): 
			return (((Table_update_descriptor*)this)->descriptor_copy());
			break;

		/* no this descriptor type */
		default:
			assert(0);
	}
	return NULL;
}
/*
 * descriptor_serialize function
 * Usage : By this function, we can serialize (copy descriptor specified into continous memory space)
 *         any descriptor which is judged by descriptor tag value. If no descriptor type defined here,
 *	   then return -1
 */
int
Descriptor::descriptor_serialize(u_char* serialize_start){

	switch(_descriptor_tag){

		case(NETWORK_NAME_DESCRIPTOR): 
			((Network_name_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(LINKAGE_DESCRIPTOR): 
			((Linkage_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR): 
			((Satellite_delivery_system_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(DATA_BROADCAST_ID_DESCRIPTOR): 
			((Data_broadcast_id_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(STREAM_IDENTIFIER_DESCRIPTOR): 
			((Stream_identifier_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(ADAPTATION_FIELD_DATA_DESCRIPTOR): 
			((Adaptation_field_data_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(SERVICE_MOVE_DESCRIPTOR): 
			((Service_move_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(IP_MAC_PLATFORM_NAME_DESCRIPTOR): 
			((Ip_mac_platform_name_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR): 
			((Ip_mac_platform_provider_name_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(TARGET_MAC_ADDRESS_DESCRIPTOR): 
			((Target_mac_address_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(TARGET_IP_SLASH_DESCRIPTOR): 
			((Target_ip_slash_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(IP_MAC_STREAM_LOCATION_DESCRIPTOR): 
			((Ip_mac_stream_location_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(CORRECTION_MESSAGE_DESCRIPTOR): 
			((Correction_message_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(CONTENTION_CONTROL_DESCRIPTOR): 
			((Contention_control_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(LOGON_INITIALIZE_DESCRIPTOR): ((Logon_initialize_descriptor*)this)->descriptor_serialized(serialize_start); break; case(SYNC_ASSIGN_DESCRIPTOR): ((Sync_assign_descriptor*)this)->descriptor_serialized(serialize_start); break;

		case(RCS_CONTENT_DESCRIPTOR): 
			((Rcs_content_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		case(TABLE_UPDATE_DESCRIPTOR): 
			((Table_update_descriptor*)this)->descriptor_serialized(serialize_start);
			break;

		/* no this descriptor type */
		default:
			return -1;
	}
	return 0;
}

/*
 * descriptor_deserialized function
 * Usage : de-Searialize (copy continuous memory space into all content of descriptor)
 *         judged by descriptor tag value which is given in continuous memory space and specify
 *         the total descriptor length
 */
Descriptor*
Descriptor::descriptor_deserialize(u_char* serialize_start , int* return_des_total_len){

	uint8_t		des_type = 0;

	des_type = serialize_start[0];
	/* return total descriptor length which is descriptor_length + 2 */
	*return_des_total_len = serialize_start[1] + 2;
	
	switch(des_type){


		case(NETWORK_NAME_DESCRIPTOR):{ 

			Network_name_descriptor *net_deserial = new Network_name_descriptor();
			net_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)net_deserial;
		}

		case(LINKAGE_DESCRIPTOR):{ 

			Linkage_descriptor *link_deserial = new Linkage_descriptor();
			link_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)link_deserial;
		}

		case(SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR):{ 

			Satellite_delivery_system_descriptor *sat_deserial = new Satellite_delivery_system_descriptor();
			sat_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)sat_deserial;
		}
		case(DATA_BROADCAST_ID_DESCRIPTOR):{ 

			Data_broadcast_id_descriptor *db_id_deserial = new Data_broadcast_id_descriptor();
			db_id_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)db_id_deserial;
		}
		case(STREAM_IDENTIFIER_DESCRIPTOR):{ 

			Stream_identifier_descriptor *str_deserial = new Stream_identifier_descriptor();
			str_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)str_deserial;
		}
		case(ADAPTATION_FIELD_DATA_DESCRIPTOR):{ 

			Adaptation_field_data_descriptor *ad_deserial = new Adaptation_field_data_descriptor();
			ad_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)ad_deserial;
		}
		case(SERVICE_MOVE_DESCRIPTOR):{ 

			Service_move_descriptor *sm_deserial = new Service_move_descriptor();
			sm_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)sm_deserial;
		}
		case(IP_MAC_PLATFORM_NAME_DESCRIPTOR):{ 

			Ip_mac_platform_name_descriptor *pltname_deserial = new Ip_mac_platform_name_descriptor();
			pltname_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)pltname_deserial;
		}
		case(IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR):{ 

			Ip_mac_platform_provider_name_descriptor *plt_prvname_deserial = new Ip_mac_platform_provider_name_descriptor();
			plt_prvname_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)plt_prvname_deserial;
		}
		case(TARGET_MAC_ADDRESS_DESCRIPTOR):{ 

			Target_mac_address_descriptor *macaddr_deserial = new Target_mac_address_descriptor();
			macaddr_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)macaddr_deserial;
		}
		case(TARGET_IP_SLASH_DESCRIPTOR):{ 

			Target_ip_slash_descriptor *ipaddr_deserial = new Target_ip_slash_descriptor();
			ipaddr_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)ipaddr_deserial;
		}
		case(IP_MAC_STREAM_LOCATION_DESCRIPTOR):{ 

			Ip_mac_stream_location_descriptor *str_location_deserial = new Ip_mac_stream_location_descriptor();
			str_location_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)str_location_deserial;
		}
		case(CORRECTION_MESSAGE_DESCRIPTOR):{ 

			Correction_message_descriptor *cormes_deserial = new Correction_message_descriptor();
			cormes_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)cormes_deserial;
		}

		case(CONTENTION_CONTROL_DESCRIPTOR): {
			Contention_control_descriptor *cormes_deserial = new Contention_control_descriptor();
			cormes_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)cormes_deserial;
		}

		case(LOGON_INITIALIZE_DESCRIPTOR):{ 

			Logon_initialize_descriptor *logon_deserial = new Logon_initialize_descriptor();
			logon_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)logon_deserial;

		}
		case(SYNC_ASSIGN_DESCRIPTOR):{ 

			Sync_assign_descriptor *sync_deserial = new Sync_assign_descriptor();
			sync_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)sync_deserial;
		}
		case(RCS_CONTENT_DESCRIPTOR):{ 

			Rcs_content_descriptor *rcs_deserial = new Rcs_content_descriptor();
			rcs_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)rcs_deserial;
		}
		case(TABLE_UPDATE_DESCRIPTOR):{ 

			Table_update_descriptor *table_deserial = new Table_update_descriptor();
			table_deserial->descriptor_deserialized(serialize_start);
			return (Descriptor*)table_deserial;
		}
		/* no this descriptor type */
		default:
			return NULL;
	}
}

