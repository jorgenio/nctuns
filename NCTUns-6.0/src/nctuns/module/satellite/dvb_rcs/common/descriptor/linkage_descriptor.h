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

#ifndef __NCTUNS_linkage_descriptor_h__
#define __NCTUNS_linkage_descriptor_h__

#include <string.h>
#include <stdio.h>
#include "descriptor.h"

/*
 * Linkage_platform_name class
 */
class Linkage_platform_name {

 friend class Linkage_descriptor;
 friend class Linkage_for_ip_mac_notification_table;
 friend class Linkage_platform_id_data;

 private:
	char			_ISO_369_language_code[4];
	uint8_t			_platform_name_length;
	/* In order to let code more clear, so we specify the static array to 
	   indicate platform name, but not by dynamically allocated */
	char			_text_char[30];

 public:
	inline Linkage_platform_name();
	inline void 		set_platform_name_parameter(const char* , const char*);
	inline char*		get_platform_name_parameter();
	inline char*		get_iso_369_parameter();
};

Linkage_platform_name::Linkage_platform_name(){
		
	_platform_name_length = 30;
}

void
Linkage_platform_name::set_platform_name_parameter(const char* set_ISO_369_language_code , const char* set_platform_name){

	if(set_ISO_369_language_code != NULL)
	{ 
		strncpy(_ISO_369_language_code , set_ISO_369_language_code , 3);
		_ISO_369_language_code[3] = '\0';
	}
	else
		printf("warning ==> _ISO_369_language_code setting is wrong (may be NULL)\n");

	/* the 30th is reserved for '\0' */
	if(set_platform_name != NULL && (strlen(set_platform_name) <= 29)) {
		strncpy(_text_char , set_platform_name , 29);
		_text_char[30] = '\0';
	}
	else
		printf("warning ==> _text_char setting is wrong (is NULL or length must be less or equal than 29)\n");
}

char*
Linkage_platform_name::get_platform_name_parameter(){

	return _text_char;
}

char*
Linkage_platform_name::get_iso_369_parameter(){

	return _ISO_369_language_code;
}

/*
 * Linkage_platform_id_data class
 */
class Linkage_platform_id_data {

 friend class Linkage_descriptor;
 friend class Linkage_for_ip_mac_notification_table;
 //friend class Section;

 private:

	uint32_t			_platform_id 			:24;
	uint32_t			_platform_name_loop_length	:8;
	Linkage_platform_name		*_platform_name_pointer;
	
 public:
	inline Linkage_platform_id_data();
	inline void 		set_platform_id(uint32_t);
	inline uint32_t		get_platform_id(){ return _platform_id;}
};

Linkage_platform_id_data::Linkage_platform_id_data(){

	_platform_name_pointer = NULL;
}

void
Linkage_platform_id_data::set_platform_id(uint32_t set_platform_id){

	_platform_id = set_platform_id;
}

/*
 * Linkage_for_ip_mac_notification_table class
 */
class Linkage_for_ip_mac_notification_table {

 friend class Linkage_descriptor;
 //friend class Section;

 private:
	uint8_t					_platform_id_data_length;
	Linkage_platform_id_data		*_platform_id_data_pointer;	// point to object of Linkage_platform_id_loop		
	u_char					*_private_data_byte_pointer;
 public:
	inline Linkage_for_ip_mac_notification_table();
};

Linkage_for_ip_mac_notification_table::Linkage_for_ip_mac_notification_table(){

	_platform_id_data_length = 0;
	_platform_id_data_pointer = NULL;
	_private_data_byte_pointer = NULL;
}

/*
 * Linkage_descriptor class
 */
class Linkage_descriptor : public Descriptor {
	
 /*
  * We use 'friend' mechanism to let other classes can use the private
  * memebers of class Descriptor
  */
 friend class Section;
 friend class Sp_ctl;

 private:
	/*
	 * all fields are private member
	 * at first two field are 
 	 * descriptor_tag
	 * descriptor_length
	 */
	uint16_t					_transport_stream_id; 
	uint16_t					_original_network_id; 
	uint16_t					_service_id;
	uint8_t						_linkage_type;
	void						*_linkage_type_pointer;		// this pointer will point to the object decided by linkage_type
	
	/* by programmer defined */
	int						_platform_id_loop_count;

	/*
   	 * private function
	 */
	Linkage_for_ip_mac_notification_table*		_get_linkage_for_int_pointer();
	Linkage_platform_id_data*			_get_Linkage_platform_id_data_pointer();
	u_char*						_get_private_data_byte_pointer();
	Linkage_platform_name*				_get_Linkage_platform_name_pointer(int);
	uint8_t						_get_platform_id_data_len();
	void						_set_platform_id_data_len(uint8_t);
	uint8_t						_get_platform_name_len(int);
	void						_set_platform_name_len(int , uint8_t);

 /*
  * public functions
  */
 public:
	Linkage_descriptor(); 
	Linkage_descriptor(uint16_t , uint16_t , uint16_t , uint8_t , int); 
	~Linkage_descriptor(); 
	uint16_t					get_service_id() { return _service_id; }
	uint16_t					get_transport_stream_id() { return _transport_stream_id; }
	void						set_platform_id(int , uint32_t);
	uint32_t					get_platform_id(int);
	void						creat_platform_name_loop(int , int);
	void						set_platform_name_variable(int , int , const char* , const char*);
	char*						get_platform_name(int , int);
	char*						get_iso_369(int , int);
	void						set_private_data_byte(u_char*);
	u_char*						get_private_data_byte();
	Linkage_descriptor*				descriptor_copy();
	void						descriptor_serialized(u_char*);
	void						descriptor_deserialized(u_char*);
	bool						is_match(uint8_t);
	void					show_descriptor();
};

#endif /* __NCTUNS_linkage_descriptor_h__ */

