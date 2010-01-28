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

#ifndef __NCTUNS_common_h__
#define __NCTUNS_common_h__

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <sys/types.h>

#define IPV4_ADDR_STR_LEN	16

inline void print_binary(uint32_t , int);
inline int string_to_bcd(char* , int);
inline void bcd_to_string(char* , uint32_t , int);
inline void string_to_macaddr(char* , u_char*);
inline void macaddr_to_string(u_char* , char*);
inline void string_to_ipv4addr(char* , u_long*);
inline void ipv4addr_to_string(u_long* , char*);


/*
 * Slot type
 */
enum slot_type {

	TRF						= 0x0000,
	CSC						= 0x0001,
	ACQ						= 0x0002,
	SYNC						= 0x0003
};
	
/*
 * Data_broadcast_id_value
 */
enum data_broadcast_id_value {

	RESERVED_FOR_FUTURE_USE				= 0x0000,
	DATA_PIPE					= 0x0001,
	ASYNCHRONOUS_DATA_STREAM			= 0x0002,
	SYNCHRONOUS_DATA_STREAM				= 0x0003,
	SYNCHRONISED_DATA_STREAM			= 0x0004,
	MULTI_PROTOCOL_ENCAPSULATION			= 0x0005,
	DATA_CAROUSEL					= 0x0006,
	OBJECT_CAROUSEL					= 0x0007,
	DVB_ATM_STREAMS					= 0x0008,
};

/*
 * The ratio of Inner FEC scheme
 */
enum fec_inner_coding_ratio {

	NOT_DEFINED					= 0x0000,
	CC_1_2						= 0x0001,
	CC_2_3						= 0x0002,
	CC_3_4						= 0x0003,
	CC_5_6						= 0x0004,
	CC_7_8						= 0x0005,
	CC_8_9						= 0x0006,
	CC_3_5						= 0x0007,
	CC_4_5						= 0x0008,
	CC_9_10						= 0x0009,
	NO_CC						= 0x000F

};

/*
 * The modulation type for satellite
 */
enum modulation_type_id {

	AUTO						= 0x0000,
	QPSK						= 0x0001,
	EIGHT_PSK					= 0x0002,
	SIXTEEN_QAM					= 0x0003
};

/*
 * The identifier of modulation system
 */
enum modulation_system_id {

	DVB_S						= 0x0000,
	DVB_S2						= 0x0001
};

/*
 * The value of roll_off factor
 */
enum roll_off_value {

	ZERO_THIRTY_FIVE				= 0x0000,
	ZERO_TWENTY_FIVE				= 0x0001,
	ZERO_THIRTY					= 0x0002,
	ROLL_OFF_RESERVED				= 0x0003
};

/*
 * The value of polarization
 */
enum polarization_value {

	LINEAR_HORIZONTAL				= 0x0000,
	LINEAR_VERTICAL					= 0x0001,
	CIRCULAR_LEFT					= 0x0002,
	CIRCULAR_RIGHT					= 0x0003
};

/*
 * The length of each descriptor 
 */
enum descriptor_length {

	RESERVED_LENGTH					= 0x0000,
	SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR_LENGTH	= 0x000B,
	STREAM_IDENTIFIER_DESCRIPTOR_LENGTH		= 0x0001,
	ADAPTATION_FIELD_DATA_DESCRIPTOR_LENGTH		= 0x0001,
	SERVICE_MOVE_DESCRIPTOR_LENGTH			= 0x0006,
	IP_MAC_STREAM_LOCATION_DESCRIPTOR_LENGTH	= 0x0009,
	SYNC_ASSIGN_DESCRIPTOR_LENGTH			= 0x000B
};

/*
 * The tag_value of descriptors enumeration
 */
enum tag_value {
	
	RESERVED				= 0x0000,
	TARGET_SMARTCARD_DESCRIPTOR		= 0x0006,
	TARGET_MAC_ADDRESS_DESCRIPTOR		= 0x0007,
	TARGET_SERIAL_NUMBER_DESCRIPTOR		= 0x0008,
	TARGET_IP_ADDRESS_DESCRIPTOR		= 0x0009,
	TARGET_IPV6_ADDRESS_DESCRIPTOR		= 0x000A,
	IP_MAC_PLATFORM_NAME_DESCRIPTOR		= 0x000C,
	IP_MAC_PLATFORM_PROVIDER_NAME_DESCRIPTOR= 0x000D,
	TARGET_MAC_ADDRESS_RANGE_DESCRIPTOR	= 0x000E,
	TARGET_IP_SLASH_DESCRIPTOR		= 0x000F,
	TARGET_IP_SOURCE_SLASH_DESCRIPTOR	= 0x0010,
	TARGET_IPV6_SLASH_DESCRIPTOR		= 0x0011,
	TARGET_IPV6_SOURCE_SLASH_DESCRIPTOR	= 0x0012,
	IP_MAC_STREAM_LOCATION_DESCRIPTOR	= 0x0013,
	ISP_ACCESS_MODE_DESCRIPTOR		= 0x0014,

	NETWORK_NAME_DESCRIPTOR			= 0x0040,
	SERVICE_LIST_DESCRIPTOR			= 0x0041,
	STUFFING_DESCRIPTOR			= 0x0042,
	SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR	= 0x0043,
	CABLE_DESLVERY_SYSTEM_DESCRIPTOR	= 0x0044,
	VBI_DATA_DESCRIPTOR			= 0x0045,
	VBI_TELETEXT_DESCRIPTOR			= 0x0046,
	BOUQUET_NAME_DESCRIPTOR			= 0x0047,
	SERVICE_DESCRIPTOR			= 0x0048,
	COUNTRY_AVAILABILITY_DESCRIPTOR		= 0x0049,	
	LINKAGE_DESCRIPTOR			= 0x004A,
	NVOD_REFERENCE_DESCRIPTOR		= 0x004B,
	TIME_SHIFTED_SERVICE_DESCRIPTOR		= 0x004C,
	SHORT_EVENT_DESCRIPTOR			= 0x004D,
	EXTENDED_EVENT_DESCRIPTOR		= 0x004E,
        TIME_SHIFTED_EVENT_DESCRIPTOR		= 0x004F,
	COMPONENT_DESCRIPTOR			= 0x0050,
 	MOSAIC_DESCRIPTOR			= 0x0051,
	STREAM_IDENTIFIER_DESCRIPTOR		= 0x0052,
	CA_IDENTIFIER_DESCRIPTOR		= 0x0053,
	CONTENT_DESCRIPTOR			= 0x0054,
	PARENTAL_RATING_DESCRIPTOR		= 0x0055,
	TELETEXT_DESCRIPTOR			= 0x0056,
	TELEPHONE_DESCRIPTOR			= 0x0057,
	LOCAL_TIME_OFFSET_DESCRIPTOR		= 0x0058,
	SUBTITLING_DESCRIPTOR			= 0x0059,
	TERRESRIAL_DELIVERY_SYSTEM_DESCRIPTOR	= 0x005A,
	MULTILINGUAL_NETWORK_NAME_DESCRIPTOR	= 0x005B,
	MULTILINGUAL_BOUQUET_NAME_DESCRIPTOR	= 0x005C,
	MULTILINGUAL_SERVICE_NAME_DESCRIPTOR	= 0x005D,
	MULTILINGUAL_COMPONENT_DESCRIPTOR	= 0x005E,	
	PRIVATE_DATA_SPECIFIER_DESCRIPTOR	= 0x005F,
	SERVICE_MOVE_DESCRIPTOR			= 0x0060,
	SHORT_SMOOTHING_BUFFER_DESCRIPTOR	= 0x0061,
	FREQUENCY_LIST_DESCRIPTOR		= 0x0062,
	PARTIAL_TRANSPORT_STREAM_DESCRIPTOR	= 0x0063,
	DATA_BROADCAST_DESCRIPTOR		= 0x0064,
	SCRAMBLING_DESCRIPTOR			= 0x0065,
	DATA_BROADCAST_ID_DESCRIPTOR		= 0x0066,
	TRANSPORT_STREAM_DESCRIPTOR		= 0x0067,
	DANG_DESCRIPTOR				= 0x0068,
	PDC_DESCRIPTOR				= 0x0069,
	AC_3_DESCRIPTOR				= 0x006A,
	ANCILLARY_DATA_DESCRIPTOR		= 0x006B,
	CELL_LIST_DESCRIPTOR			= 0x006C,
	CELL_FREQUENCY_LINK_DESCRIPTOR		= 0x006D,
	ANNOUNCEMENT_SUPPORT_DESCRIPTOR		= 0x006E,
	APPLICATION_SIGNALLING_DESCRIPTOR	= 0x006F,
	ADAPTATION_FIELD_DATA_DESCRIPTOR	= 0x0070,
	SERVICE_IDENTIFIER_DESCRIPTOR		= 0x0071,
	SERVICE_AVAILABILITY_DESCRIPTOR		= 0x0072,
	DEFAULT_AUTHORITY_DESCRIPTOR		= 0x0073,
	RELATED_CONTENT_DESCRIPTOR		= 0x0074,
	TVA_ID_DESCRIPTOR			= 0x0075,
	CONTENT_IDENTIFIER_DESCRIPTOR		= 0x0076,
	TIME_SLICE_FEC_IDENTIFIER_DESCRIPTOR	= 0x0077,
	ECM_REPETITION_RATE_DESCRIPTOR		= 0x0078,
	S2_SATELLITE_DELIVERY_SYSTEM_DESCRIPTOR	= 0x0079,
	ENHANCED_AC_3_DESCRIPTOR		= 0x007A,
	DTS_DESCRIPTOR				= 0x007B,
	AAC_DESCRIPTOR				= 0x007C,
	RESERVED_FOR_FUTURE_USE_1		= 0x007D,
	RESERVED_FOR_FUTURE_USE_2		= 0x007E,
	EXTENSION_DESCRIPTOR			= 0x007F,

	NETWORK_LAYER_INFO_DESCRIPTOR		= 0x00A0,
	CORRECTION_MESSAGE_DESCRIPTOR		= 0x00A1,
	LOGON_INITIALIZE_DESCRIPTOR		= 0x00A2,
	ACQ_ASSIGN_DESCRIPTOR			= 0x00A3,
	SYNC_ASSIGN_DESCRIPTOR			= 0x00A4,
	ENCRYPTED_LOGON_ID_DESCRIPTOR 		= 0x00A5,
	ECHO_VALUE_DESCRIPTOR			= 0x00A6,
	RCS_CONTENT_DESCRIPTOR			= 0x00A7,
	SATELLITE_FORWARD_LINK_DESCRIPTOR	= 0x00A8,
	SATELLITE_RETURN_LINK_DESCRIPTOR	= 0x00A9,
	TABLE_UPDATE_DESCRIPTOR			= 0x00AA,
	CONTENTION_CONTROL_DESCRIPTOR		= 0x00AB,
	CORRECTION_CONTROL_DESCRIPTOR		= 0x00AC,
	FORWARD_INTERACTION_PATH_DESCRIPTOR	= 0x00AD,
	RETURN_INTERACTION_PATH_DESCRIPTOR	= 0x00AE,
	CONNECTION_CONTROL_DESCRIPTOR		= 0x00AF,

	FORBIDDEN				= 0x00FF
};

class IPv4_format {

 public:
    	u_long                  IPv4_addr;
        uint8_t                 IPv4_slash_mask;
};

class Table_id_format {

 public:
	uint8_t			table_id;
	uint8_t			new_version;
};

/*
 * string_to_bcd function
 * Usage : transfer string into bcd coding 
 */
int string_to_bcd(char* set_str , int str_size){

	uint8_t		set_value = 0; 
	int		tmp_bcd_value = 0;

	for(int i = 0 ; i <= (str_size - 1) ; i++){

		set_value = (uint8_t)(set_str[i] - 0x30);	
		tmp_bcd_value = (set_value & 0x0F) | (tmp_bcd_value << 4);
	}
	return tmp_bcd_value;
}

/*
 * bcd_to_string function
 * Usage : transfer bcd coding into string
 */
void bcd_to_string(char* return_str , uint32_t bcd_value , int str_size){

	//char		*return_characters;
	uint8_t		tmp_value;
	uint32_t	tmp_return_characters;

	tmp_return_characters = bcd_value;
	//return_characters = new char[str_size];

	//print_binary(tmp_orbital_position , 8*sizeof(uint32_t));
	for(int i = (str_size - 1) ; i >= 0 ; i--){
	
		tmp_value = (uint8_t)(tmp_return_characters & 0x0000000F);	
		return_str[i] = tmp_value + 0x30;	
		tmp_return_characters = tmp_return_characters >> 4;
	}
}

/*
 * print_binary function
 * Usage : print out bit by bit 
 */
void print_binary(uint32_t print_buffer, int print_size){

	printf("This binary is : ");

	for(int i = (print_size - 1) ; i >= 0 ; i--){

		int bit = (( print_buffer >> i ) & 1 );
		printf("%d" , bit);
	}
	printf("\n");
}

/*
 * string_to_macaddr function
 * transfer the format of "x:x:x:x:x:x" into u_char arrary size of 6
 */
void string_to_macaddr(char *str, u_char *mac) {

        u_int            tmp[20];
	u_char		 *k = NULL;

	k = (u_char*)mac;
        sscanf(str , "%x:%x:%x:%x:%x:%x" , tmp , (tmp + 1) , (tmp + 2) , (tmp + 3) , (tmp + 4) , (tmp + 5));

        k[0] = tmp[0]; k[1] = tmp[1];
        k[2] = tmp[2]; k[3] = tmp[3];
        k[4] = tmp[4]; k[5] = tmp[5];
        return;
}

/*
 * macaddr_to_string function
 * transfer u_char arrary size of 6 into the format of "x:x:x:x:x:x"
 */
void macaddr_to_string(u_char *mac, char *str) {

        sprintf(str , "%x:%x:%x:%x:%x:%x" , mac[0] , mac[1] , mac[2] , mac[3] , mac[4] , mac[5]);
        return;
}

/*
 * string_to_ipv4addr function
 * transfer the format of "d.d.d.d" into unsigned long
 */
void string_to_ipv4addr(char *str , u_long *ipv4addr) {

        u_int            tmp[20];
	u_char		 *k = NULL;
	
        k = (u_char*)ipv4addr;
        sscanf(str , "%d.%d.%d.%d" , tmp , (tmp + 1) , (tmp + 2) , (tmp + 3));

	k[0] = tmp[0]; k[1] = tmp[1];
	k[2] = tmp[2]; k[3] = tmp[3];
        return;
}

/*
 * ipv4addr_to_string function
 * transfer unsigned long into the format of "d.d.d.d"
 */
void ipv4addr_to_string(u_long *ipv4addr, char *str) {

        u_char          *p = NULL;

        p = (u_char*)ipv4addr;
        sprintf(str , "%d.%d.%d.%d" , p[0] , p[1] , p[2] , p[3]);
        return;
}

#endif /* __NCTUNS_common_h__ */
