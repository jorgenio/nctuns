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

#include "int_section.h"
#include "section_draft.h"
#include "../fec/crc.h"
#include "../descriptor/target_ip_slash_descriptor.h"

Int_table_to_section_handler::Int_table_to_section_handler():
	_int_table(NULL), _sec_list_cnt(0), _sec_list_head(NULL),
	_sec_list_tail(NULL)
{}


Int_table_to_section_handler::~Int_table_to_section_handler() {
	struct sec_entry *	sec_ent;
	struct sec_entry *	nxt_sec_ent;
	struct for_entry * 	for_ent;
	struct for_entry *	nxt_for_ent;
	struct des_entry *	des_ent;
	struct des_entry *	nxt_des_ent;
	u_int32_t		i,j,k,m;

	for(i=0, sec_ent=_sec_list_head; i<_sec_list_cnt && sec_ent != NULL; i++) {

		for(j=0, des_ent=sec_ent->platform_des_list_head; j<sec_ent->platform_des_list_cnt && des_ent != NULL; j++) {
			delete des_ent->des;
			nxt_des_ent = des_ent->next;
			delete des_ent;
			des_ent = nxt_des_ent;
		}

		for(k=0, for_ent=sec_ent->for_loop_list_head; k<sec_ent->for_loop_list_cnt && for_ent != NULL; k++) {

			for(m=0, des_ent=for_ent->target_des_list_head; m<for_ent->target_des_list_cnt && des_ent != NULL; m++) {
				delete des_ent->des;
				nxt_des_ent = des_ent->next;
				delete des_ent;
				des_ent = nxt_des_ent;
			}

			for(m=0, des_ent=for_ent->operational_des_list_head; m<for_ent->operational_des_list_cnt && des_ent != NULL; m++) {
				delete des_ent->des;
				nxt_des_ent = des_ent->next;
				delete des_ent;
				des_ent = nxt_des_ent;
			}

			nxt_for_ent = for_ent;
			delete for_ent;
			for_ent = nxt_for_ent;
		}

		nxt_sec_ent = sec_ent->next;
		delete sec_ent;
		sec_ent = nxt_sec_ent;
	}
}

void Int_table_to_section_handler::_calculate_last_section_number_and_section_lengths(Int* table) {
	struct sec_entry *		sec_ent;
	u_int32_t			i,j;
	Descriptor *			platform_des;
	Descriptor *			target_des;
	Descriptor *			operational_des;
	struct for_loop_entry * 	for_loop_ent;
	struct for_entry *		for_ent;

	/* at least one section */
	sec_ent = _new_sec_entry();
	/* The 2-byte field of loop length for platform 
	 * descriptor loop has to be reserved here. */
	sec_ent->sec_rest_space_in_byte -= 2;	
	sec_ent->sec_final_size_in_byte += 2;	 

	if(table->platform_des_loop != NULL && table->platform_des_loop->descriptor_count > 0) {
		for(i=0; i<(u_int32_t)table->platform_des_loop->descriptor_count; i++) {
			platform_des = table->platform_des_loop->get_sequencing_descriptor(i);
			if(sec_ent->sec_rest_space_in_byte >= (u_int32_t)platform_des->get_descriptor_total_len()) {
				_add_platform_des_entry(sec_ent, platform_des);
				sec_ent->sec_rest_space_in_byte -= platform_des->get_descriptor_total_len();
				sec_ent->sec_final_size_in_byte += platform_des->get_descriptor_total_len();
			}
			else { sec_ent = _new_sec_entry();
				sec_ent->sec_rest_space_in_byte -= 2;  // for the field of loop length
				sec_ent->sec_final_size_in_byte += 2;  // for the field of loop length
				if(sec_ent->sec_rest_space_in_byte >= (u_int32_t)platform_des->get_descriptor_total_len()) {
					_add_platform_des_entry(sec_ent, platform_des);
					sec_ent->sec_rest_space_in_byte -= platform_des->get_descriptor_total_len();
					sec_ent->sec_final_size_in_byte += platform_des->get_descriptor_total_len();
				}
				else assert(0);
			}
		}
	}

	if(table->for_loop_cnt > 0) {
		for(i=0, for_loop_ent = table->for_loop_head; 
		    i<table->for_loop_cnt && for_loop_ent != NULL; 
		    i++, for_loop_ent = for_loop_ent->next) {

			if(for_loop_ent->target_des_loop->descriptor_count > 0 && 
			   for_loop_ent->operational_des_loop->descriptor_count > 0) {

				if(sec_ent->sec_rest_space_in_byte - 4 >=
			   	   (u_int32_t)for_loop_ent->target_des_loop->get_all_descriptor_total_length() + 
			    	   (u_int32_t)for_loop_ent->operational_des_loop->get_all_descriptor_total_length() ) {

					for_ent = _new_for_entry(sec_ent);
					sec_ent->sec_rest_space_in_byte -= 4;  // for two fields of loop length
					sec_ent->sec_final_size_in_byte += 4;  // for two fields of loop length
				
					for(j=0; j<(u_int32_t)for_loop_ent->target_des_loop->descriptor_count; j++) {
						target_des = for_loop_ent->target_des_loop->get_sequencing_descriptor(j);
						_add_target_des_entry(for_ent, target_des);
						sec_ent->sec_rest_space_in_byte -= target_des->get_descriptor_total_len();
						sec_ent->sec_final_size_in_byte += target_des->get_descriptor_total_len();
					}
					for(j=0; j<(u_int32_t)for_loop_ent->operational_des_loop->descriptor_count; j++) {
						operational_des = for_loop_ent->operational_des_loop->get_sequencing_descriptor(j);
						_add_operational_des_entry(for_ent, operational_des);
						sec_ent->sec_rest_space_in_byte -= operational_des->get_descriptor_total_len();
						sec_ent->sec_final_size_in_byte += operational_des->get_descriptor_total_len();
					}
				}
				else {
					sec_ent = _new_sec_entry();
					/* The 2-byte field of loop length for platform 
					 * descriptor loop has to be reserved here. */
					sec_ent->sec_rest_space_in_byte -= 2;
					sec_ent->sec_final_size_in_byte += 2;

					if(sec_ent->sec_rest_space_in_byte - 4 >=
			   		   (u_int32_t)for_loop_ent->target_des_loop->get_all_descriptor_total_length() + 
			   		   (u_int32_t)for_loop_ent->operational_des_loop->get_all_descriptor_total_length() ) {
					
						for_ent = _new_for_entry(sec_ent);
						sec_ent->sec_rest_space_in_byte -= 4;  // for two fields of loop length
						sec_ent->sec_final_size_in_byte += 4;  // for two fields of loop length

						for(j=0; j<(u_int32_t)for_loop_ent->target_des_loop->descriptor_count; j++) {
							target_des = for_loop_ent->target_des_loop->get_sequencing_descriptor(j);
							_add_target_des_entry(for_ent, target_des);
							sec_ent->sec_rest_space_in_byte -= target_des->get_descriptor_total_len();
							sec_ent->sec_final_size_in_byte += target_des->get_descriptor_total_len();
							//delete (Target_ip_slash_descriptor*)target_des;
						}
						for(j=0; j<(u_int32_t)for_loop_ent->operational_des_loop->descriptor_count; j++) {
							operational_des = for_loop_ent->operational_des_loop->get_sequencing_descriptor(j);
							_add_operational_des_entry(for_ent, operational_des);
							sec_ent->sec_rest_space_in_byte -= operational_des->get_descriptor_total_len();
							sec_ent->sec_final_size_in_byte += operational_des->get_descriptor_total_len();
						}
					}
					else {
						printf("int_section.cc: MAX_SECTION_BYTE_SIZE is too small !!\n"); 
						assert(0);
					}
				}
			}
		}
		//delete (Target_ip_slash_descriptor*)target_des;
	}

	_last_section_number = (u_char)(_sec_list_cnt-1); // the section number is numbered from 0
}

struct sec_entry * Int_table_to_section_handler::_new_sec_entry() {
	struct sec_entry *	sec_ent;
	
	sec_ent = new struct sec_entry;
	assert(sec_ent);

	sec_ent->sec_num 			= _sec_list_cnt;
	sec_ent->sec_rest_space_in_byte 	= MAX_ALL_DESCRIPTOR_LOOP_BYTE_LENGTH;
	sec_ent->sec_final_size_in_byte 	= SECTION_HEAD_AND_TAIL_BYTE_LENGTH;

	sec_ent->platform_des_list_cnt		= 0;
	sec_ent->platform_des_list_len		= 0;
	sec_ent->platform_des_list_head 	= NULL;
	sec_ent->platform_des_list_tail		= NULL;

	sec_ent->for_loop_list_cnt		= 0;
	sec_ent->for_loop_list_head		= NULL;
	sec_ent->for_loop_list_tail		= NULL;

	sec_ent->next 				= NULL;

	if(_sec_list_cnt == 0) {
		_sec_list_head = sec_ent;
		_sec_list_tail = sec_ent;
	}else if(_sec_list_cnt > 0) {
		_sec_list_tail->next = sec_ent;
		_sec_list_tail = sec_ent;
	}

	_sec_list_cnt++;
	
	return sec_ent;	
}

struct for_entry * Int_table_to_section_handler::_new_for_entry(struct sec_entry * sec_ent) {
	struct for_entry *	for_ent;

	for_ent = new struct for_entry;
	assert(for_ent);

	for_ent->target_des_list_cnt		= 0;
	for_ent->target_des_list_len		= 0;
	for_ent->target_des_list_head 		= NULL;
	for_ent->target_des_list_tail		= NULL;

	for_ent->operational_des_list_cnt	= 0;
	for_ent->operational_des_list_len	= 0;
	for_ent->operational_des_list_head 	= NULL;
	for_ent->operational_des_list_tail	= NULL;

	for_ent->next				= NULL;

	if(sec_ent->for_loop_list_cnt == 0) {
		sec_ent->for_loop_list_head = for_ent;
		sec_ent->for_loop_list_tail = for_ent;
	}else if(sec_ent->for_loop_list_cnt > 0) {
		sec_ent->for_loop_list_tail->next = for_ent;
		sec_ent->for_loop_list_tail = for_ent;
	}
	
	sec_ent->for_loop_list_cnt++;

	return for_ent;
}


void Int_table_to_section_handler::_add_platform_des_entry(struct sec_entry * sec_ent, Descriptor *des) {
	struct des_entry *	des_ent;
	
	des_ent = new struct des_entry;
	assert(des_ent);
	
	des_ent->des_len = des->get_descriptor_total_len();
	des_ent->des = des;
	des_ent->next = NULL;

	sec_ent->platform_des_list_cnt++;
	sec_ent->platform_des_list_len += des_ent->des_len;

	if(sec_ent->platform_des_list_cnt == 1) {
		sec_ent->platform_des_list_head = des_ent;
		sec_ent->platform_des_list_tail = des_ent;
	}else if(sec_ent->platform_des_list_cnt > 1) {
		sec_ent->platform_des_list_tail->next = des_ent;
		sec_ent->platform_des_list_tail = des_ent;
	}
}

void Int_table_to_section_handler::_add_target_des_entry(struct for_entry * for_ent, Descriptor *des) {
	struct des_entry *	des_ent;
	
	des_ent = new struct des_entry;
	assert(des_ent);
	
	des_ent->des_len = des->get_descriptor_total_len();
	des_ent->des = des;
	des_ent->next = NULL;

	for_ent->target_des_list_cnt++;
	for_ent->target_des_list_len += des_ent->des_len;

	if(for_ent->target_des_list_cnt == 1) {
		for_ent->target_des_list_head = des_ent;
		for_ent->target_des_list_tail = des_ent;
	}else if(for_ent->target_des_list_cnt > 1) {
		for_ent->target_des_list_tail->next = des_ent;
		for_ent->target_des_list_tail = des_ent;
	}
}

void Int_table_to_section_handler::_add_operational_des_entry(struct for_entry * for_ent, Descriptor *des) {
	struct des_entry *	des_ent;
	
	des_ent = new struct des_entry;
	assert(des_ent);
	
	des_ent->des_len = des->get_descriptor_total_len();
	des_ent->des = des;
	des_ent->next = NULL;

	for_ent->operational_des_list_cnt++;
	for_ent->operational_des_list_len += des_ent->des_len;

	if(for_ent->operational_des_list_cnt == 1) {
		for_ent->operational_des_list_head = des_ent;
		for_ent->operational_des_list_tail = des_ent;
	}else if(for_ent->operational_des_list_cnt > 1) {
		for_ent->operational_des_list_tail->next = des_ent;
		for_ent->operational_des_list_tail = des_ent;
	}
}

void Int_table_to_section_handler::int_table_to_section_init(Int * table) {

	_int_table 		= table;
	_table_id 		= table->get_table_id();
	_version_number 	= table->get_version_number();
	_current_next_indicator = table-> get_current_next_indicator();
	_platform_id 		= table->get_platform_id();
	_processing_order	= table->get_processing_order();
	_last_section_number	= 0x00;

	_sec_list_cnt		= 0;
	_sec_list_head		= NULL;
	_sec_list_tail		= NULL;

	_calculate_last_section_number_and_section_lengths(table);
}

void * Int_table_to_section_handler::int_table_to_section() {
	u_char *		section;
	struct sec_entry *	sec_ent;
	struct for_entry *	for_ent;
	struct for_entry *	nxt_for_ent;
	struct des_entry *	des_ent;
	struct des_entry *	nxt_des_ent;
	u_int32_t		i,j;
	u_char *		pointer_address;
	Section_draft	*ptr_draft;

	if(_sec_list_cnt > 0) {
		sec_ent = _sec_list_head;
		assert(sec_ent);
		_sec_list_head = _sec_list_head->next;

		section = (u_char *)malloc(sizeof(u_char) * sec_ent->sec_final_size_in_byte);
		memset(section, 0, sec_ent->sec_final_size_in_byte);

		ptr_draft = (Section_draft *)section;
		ptr_draft->set_section_draft(_table_id,
                                     0x1, 0x0,
                                     sec_ent->sec_final_size_in_byte - 3);
		fill_action_type(section, 0x01);
		fill_platform_id_hash(section, _platform_id);
		fill_version_number(section, _version_number);
		fill_current_next_indicator(section, _current_next_indicator);
		fill_section_number(section, (u_char)sec_ent->sec_num);
		fill_last_section_number(section, _last_section_number);
		fill_platform_id(section, _platform_id);
		fill_processing_order(section, _processing_order);

		pointer_address = &section[12];
		
		/* platform_descriptor_loop */
		if(sec_ent->platform_des_list_cnt > 0) {
			fill_descriptor_loop_length(pointer_address, sec_ent->platform_des_list_len);
			pointer_address = pointer_address + 2;
			
			des_ent = sec_ent->platform_des_list_head;
			for(i=0; i<sec_ent->platform_des_list_cnt; i++) {
				des_ent->des->descriptor_serialize(pointer_address);
				pointer_address += des_ent->des_len;

				delete des_ent->des;
				nxt_des_ent = des_ent->next;
				delete des_ent;
				des_ent = des_ent->next;
			}
		}
		else {
			fill_descriptor_loop_length(pointer_address, 0);
			pointer_address = pointer_address + 2;
		}

		for(j=0, for_ent=sec_ent->for_loop_list_head; j<sec_ent->for_loop_list_cnt && for_ent != NULL; j++) {

			/* target_descriptor_loop */
			if(for_ent->target_des_list_cnt > 0) {
				fill_descriptor_loop_length(pointer_address, for_ent->target_des_list_len);
				pointer_address = pointer_address + 2;
			
				des_ent = for_ent->target_des_list_head;
				for(i=0; i<for_ent->target_des_list_cnt; i++) {
					des_ent->des->descriptor_serialize(pointer_address);
					pointer_address += des_ent->des_len;

					delete (Target_ip_slash_descriptor*)des_ent->des;
					nxt_des_ent = des_ent->next;
					delete des_ent;
					des_ent = NULL;
					des_ent = nxt_des_ent;
				}
			}

			/* operational_descriptor_loop */
			if(for_ent->operational_des_list_cnt > 0) {
				fill_descriptor_loop_length(pointer_address, for_ent->operational_des_list_len);
				pointer_address = pointer_address + 2;
			
				des_ent = for_ent->operational_des_list_head;
				for(i=0; i<for_ent->operational_des_list_cnt; i++) {
					des_ent->des->descriptor_serialize(pointer_address);
					pointer_address += des_ent->des_len;

					delete des_ent->des;
					nxt_des_ent = des_ent->next;
					delete des_ent;
					des_ent = nxt_des_ent;
				}
			}

			nxt_for_ent = for_ent->next;
			delete for_ent;
			for_ent = nxt_for_ent;
		}

		_sec_list_head = sec_ent->next;
		delete sec_ent;
		_sec_list_cnt--;

		// Compute and fill the CRC32 value.
		fill_crc(section);


		return (void *)section;
	}
	else
		return NULL;
}

void Int_table_to_section_handler::fill_table_id(u_char * section, u_char id) {

	section[0] = id;
}

void Int_table_to_section_handler::fill_section_syntax_indicator(u_char * section, u_char ind) {
	u_char		tmp_buf;

	tmp_buf = ind << 7;
	section[1] = tmp_buf & 0x80;
}

void Int_table_to_section_handler::fill_section_length(u_char * section, u_int16_t len) {
	u_char *	char_buf;
	u_char		tmp_buf;

	char_buf = (u_char *)&len;
	tmp_buf = char_buf[1]& 0x0f;
	section[1] = section [1] ^ tmp_buf;
	section[2] = char_buf[0];	
}

void Int_table_to_section_handler::fill_action_type(u_char * section, u_char type) {

	section[3] = type;
}

void Int_table_to_section_handler::fill_platform_id_hash(u_char * section, u_int32_t pfid) {
	u_char *	char_buf;

	char_buf = (u_char *)&pfid; 
	section[4] = char_buf[0] ^ char_buf[1] ^ char_buf[2];
}

void Int_table_to_section_handler::fill_version_number(u_char * section, u_char num) {
	u_char		tmp_buf;

	tmp_buf = (num << 1) & 0x3e;
	section[5] = section[5] ^ tmp_buf;
}

void Int_table_to_section_handler::fill_current_next_indicator(u_char * section, u_char ind) {
	u_char		tmp_buf;

	tmp_buf = ind & 0x01;
	section[5] = section[5] ^ tmp_buf;
}

void Int_table_to_section_handler::fill_section_number(u_char * section, u_char num) {
	
	section[6] = num;
}

void Int_table_to_section_handler::fill_last_section_number(u_char * section, u_char num) {

	section[7] = num;
}

void Int_table_to_section_handler::fill_platform_id(u_char * section, u_int32_t pfid) {
	u_char *	char_buf;

	char_buf = (u_char *)&pfid;
	section[8] = char_buf[2];
	section[9] = char_buf[1];
	section[10] = char_buf[0];
}

void Int_table_to_section_handler::fill_processing_order(u_char * section, u_char order) {

	section[11] = order;
}

void Int_table_to_section_handler::fill_descriptor_loop_length(u_char * field_start, u_int16_t len) {
	u_char *	char_buf;
	u_char		tmp_buf;

	char_buf = (u_char *)&len;
	tmp_buf = char_buf[1]& 0x0f;
	field_start[0] = field_start [0] ^ tmp_buf;
	field_start[1] = char_buf[0];	
}

Int_section_to_table_handler::~Int_section_to_table_handler() {

	delete [] _received;
}


bool Int_section_to_table_handler::_is_complete() {
	int		i;

	for (i=0; i<=_last_section_number; i++) {
		if (!(_received[i]))
			return false;
		}
	return true;
}

int Int_section_to_table_handler::init(void * section) {

	_version_number = get_version_number((u_char *)section);
	_current_next_indicator = get_current_next_indicator((u_char *)section);
	_platform_id = get_platform_id((u_char *)section);
	_processing_order = get_processing_order((u_char *)section);
	
	_last_section_number = get_last_section_number((u_char *)section);
	_received = new bool[_last_section_number + 1];
	assert(_received);
	memset(_received, 0, sizeof (bool) * (_last_section_number + 1));

	_int_table = new Int(_version_number, _current_next_indicator, _platform_id, _processing_order);
	assert(_int_table);

	return (0);
}

Int * Int_section_to_table_handler::to_table(void * section) {
	u_char *	pointer_address;
	u_char		section_number;
	u_int16_t	section_len;
	u_int16_t	total_des_loop_len;

	u_int16_t	total_platform_des_loop_len;
	u_int16_t	total_target_des_loop_len;
	u_int16_t	total_operational_des_loop_len;
	
	u_char *	target_des_loop_addr;
	u_char *	operational_des_loop_addr;

	int		return_des_len;
	Descriptor *	platform_des;
	Descriptor *	target_des;
	Descriptor *	operational_des;

	pointer_address = (u_char *)section;
	section_number = get_section_number((u_char *)pointer_address);
	section_len = get_section_length((u_char *)pointer_address);
	total_des_loop_len = section_len - (INT_SECTION_FOREHEAD_BYTE_LENGTH - 3) - CRC_32_FIELD_BYTE_LENGTH;

	pointer_address = pointer_address + 12;

	total_platform_des_loop_len = get_descriptor_loop_length(pointer_address);
	pointer_address += 2;
	total_des_loop_len -= 2;
	while(total_platform_des_loop_len > 0) {
		platform_des = Descriptor::descriptor_deserialize(pointer_address, &return_des_len);

		total_platform_des_loop_len -= return_des_len;
		pointer_address += return_des_len;
		total_des_loop_len -= return_des_len;

		_int_table->add_platform_descriptor(platform_des);
		delete platform_des;
	}

	while(total_des_loop_len > 0) {
		total_target_des_loop_len = get_descriptor_loop_length(pointer_address);
		pointer_address += 2;
		total_des_loop_len -= 2;
		target_des_loop_addr = pointer_address;

		pointer_address += total_target_des_loop_len;
		total_des_loop_len -= total_target_des_loop_len;

		total_operational_des_loop_len = get_descriptor_loop_length(pointer_address);
		pointer_address += 2;
		total_des_loop_len -= 2;
		operational_des_loop_addr = pointer_address;

		pointer_address += total_operational_des_loop_len;
		total_des_loop_len -= total_operational_des_loop_len;

		while(total_target_des_loop_len > 0 && total_operational_des_loop_len > 0) {
			target_des = Descriptor::descriptor_deserialize(target_des_loop_addr, &return_des_len);
			total_target_des_loop_len -= return_des_len;
			target_des_loop_addr += return_des_len;

			operational_des = Descriptor::descriptor_deserialize(operational_des_loop_addr, &return_des_len);
			total_operational_des_loop_len -= return_des_len;
			operational_des_loop_addr += return_des_len;

			_int_table->add_target_and_operational_descriptor(target_des, operational_des);
			delete (Target_ip_slash_descriptor*)target_des;
			delete operational_des;
		}
	}

	_received[section_number] = true;
	if (_is_complete())
		return _int_table;
	else
		return NULL;
}

u_char Int_section_to_table_handler::get_table_id(u_char * section) {
	
	return section[0];
}

u_char Int_section_to_table_handler::get_section_syntax_indicator(u_char * section) {
	u_char		tmp_buf;

	tmp_buf = section[1] >> 7;
	tmp_buf = tmp_buf & 0x01;
	return tmp_buf;
}

u_int16_t Int_section_to_table_handler::get_section_length(u_char * section){
	Section_draft *ptr_draft;

	ptr_draft = (Section_draft *)section;
	return ptr_draft->get_section_length();
}

u_char Int_section_to_table_handler::get_action_type(u_char * section) {
	
	return section[3];
}

u_int32_t Int_section_to_table_handler::get_platform_id_hash(u_char * section) {

	return section[4];
}

u_char Int_section_to_table_handler::get_version_number(u_char * section) {
	u_char		tmp_buf;

	tmp_buf = section[5] >> 1;
	tmp_buf = tmp_buf & 0x1f;

	return tmp_buf;	
}

u_char Int_section_to_table_handler::get_current_next_indicator(u_char * section) {
	u_char		tmp_buf;

	tmp_buf = section[5] & 0x01;

	return tmp_buf;
}

u_char Int_section_to_table_handler::get_section_number(u_char * section) {
	
	return section[6];
}

u_char Int_section_to_table_handler::get_last_section_number(u_char * section) {

	return section[7];
}

u_int32_t Int_section_to_table_handler::get_platform_id(u_char * section) {
	u_int32_t	id;
	u_char *	char_buf;

	char_buf = (u_char *)&id;
	char_buf[3] = 0x00;
	char_buf[2] = section[8];
	char_buf[1] = section[9];
	char_buf[0] = section[10];

	return id;
}

u_char Int_section_to_table_handler::get_processing_order(u_char * section) {

	return section[11];
}

u_int16_t Int_section_to_table_handler::get_descriptor_loop_length(u_char * field_start){
	u_int16_t	len;
	u_char *	char_buf;

	char_buf = (u_char *)&len;
	char_buf[1] = field_start[0] & 0x0f;
	char_buf[0] = field_start[1];

	return len;
}

u_int32_t Int_section_to_table_handler::get_crc(u_char * section) {
	u_int32_t	crc;
	u_char *	char_buf;
	u_char *	target_address;
	u_int16_t	section_len;

	
	char_buf = (u_char *)&section_len;
	char_buf[1] = section[1] & 0x0f;
	char_buf[0] = section[2];

	target_address = section + 3 + section_len - 4;

	char_buf = (u_char *)&crc;
	char_buf[3] = target_address[0];
	char_buf[2] = target_address[1];
	char_buf[1] = target_address[2];
	char_buf[0] = target_address[3];
	
	return crc;
}

