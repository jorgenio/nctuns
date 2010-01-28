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

#ifndef __NCTUNS_int_section_h__
#define __NCTUNS_int_section_h__

#include "int_table.h"

/*
  IP/MAC Notification section:
 	
  table_id			: 8;
  section_syntax_indicator	: 1;
  reserved_for_future_use	: 1;
  reserved			: 2;
  section_length		:12;  // max value = 4093
  action_type			: 8;
  platform_id_hash		: 8;
  reserved			: 2;
  version_number		: 5;
  current_next_indicator	: 1;
  section_number		: 8;
  last_section_number		: 8;
  platform_id			:24;
  processing_order		: 8;
  platform_descriptor_loop
  for_loop{
	target_descriptor_loop
	operational_descriptor_loop
  }
  CRC_32			:32;	

*/

#define INT_SECTION_FOREHEAD_BYTE_LENGTH	12
#define CRC_32_FIELD_BYTE_LENGTH		4
#define SECTION_HEAD_AND_TAIL_BYTE_LENGTH	INT_SECTION_FOREHEAD_BYTE_LENGTH + CRC_32_FIELD_BYTE_LENGTH
#define MAX_SECTION_BYTE_SIZE			4096
#define MAX_ALL_DESCRIPTOR_LOOP_BYTE_LENGTH	MAX_SECTION_BYTE_SIZE - SECTION_HEAD_AND_TAIL_BYTE_LENGTH 


struct des_entry {
	u_int32_t			des_len;
	Descriptor *			des;
	struct des_entry *		next;
};

struct for_entry {

	u_int32_t			target_des_list_cnt;
	u_int16_t			target_des_list_len;
	struct des_entry *		target_des_list_head;
	struct des_entry *		target_des_list_tail;
	
	u_int32_t			operational_des_list_cnt;
	u_int16_t			operational_des_list_len;
	struct des_entry *		operational_des_list_head;
	struct des_entry *		operational_des_list_tail;

	struct for_entry *		next;
};


struct sec_entry {
	u_int8_t			sec_num;		// numbered from 0
	u_int32_t			sec_rest_space_in_byte;
	u_int32_t			sec_final_size_in_byte;

	u_int32_t			platform_des_list_cnt;
	u_int16_t			platform_des_list_len;
	struct des_entry *		platform_des_list_head;
	struct des_entry *		platform_des_list_tail;
	
	u_int32_t			for_loop_list_cnt;
	struct for_entry *		for_loop_list_head;
	struct for_entry *		for_loop_list_tail;
	
	struct sec_entry *		next;
};


class Int_table_to_section_handler {
  private:
	Int *			_int_table;
	u_char			_table_id;
	u_char			_version_number;
	u_char			_current_next_indicator;
	u_int32_t		_platform_id;
	u_char			_processing_order;	
	u_char			_last_section_number;

	u_int8_t		_sec_list_cnt;
	struct sec_entry *	_sec_list_head;
	struct sec_entry *	_sec_list_tail;

	struct sec_entry *	_new_sec_entry();
	struct for_entry *	_new_for_entry(struct sec_entry * sec_ent);
	void			_add_platform_des_entry(struct sec_entry * sec_ent, Descriptor *des);	
	void			_add_target_des_entry(struct for_entry * for_ent, Descriptor *des);
	void			_add_operational_des_entry(struct for_entry * for_ent, Descriptor *des);
	void			_calculate_last_section_number_and_section_lengths(Int* table);

  public:	
	Int_table_to_section_handler();
	~Int_table_to_section_handler();

	void			int_table_to_section_init(Int * table);	
	void *			int_table_to_section();

	void 			fill_table_id(u_char * section, u_char id);
	void			fill_section_syntax_indicator(u_char * section, u_char ind);
	void			fill_section_length(u_char * section, u_int16_t len);
	void			fill_action_type(u_char * section, u_char type);
	void			fill_platform_id_hash(u_char * section, u_int32_t pfid);
	void			fill_version_number(u_char * section, u_char num);
	void			fill_current_next_indicator(u_char * section, u_char ind);
	void			fill_section_number(u_char * section, u_char num);
	void			fill_last_section_number(u_char * section, u_char num);
	void			fill_platform_id(u_char * section, u_int32_t pfid);
	void			fill_processing_order(u_char * section, u_char order);
	void			fill_descriptor_loop_length(u_char * field_start, u_int16_t len);
};

class Int_section_to_table_handler {
  private:

    u_char			_version_number;
	u_char			_current_next_indicator;
	u_int32_t		_platform_id;
	u_char			_processing_order;
	
	u_char			_last_section_number;
	bool *			_received;
	Int *			_int_table;

	bool			_is_complete();
  public:
	Int_section_to_table_handler() {}
	~Int_section_to_table_handler();

	int			init(void * section);
	Int *			to_table(void * section);

	u_char 			get_table_id(u_char * section);
	u_char			get_section_syntax_indicator(u_char * section);
	u_int16_t		get_section_length(u_char * section);
	u_char			get_action_type(u_char * section);
	u_int32_t		get_platform_id_hash(u_char * section);
	u_char			get_version_number(u_char * section);
	u_char			get_current_next_indicator(u_char * section);
	u_char			get_section_number(u_char * section);
	u_char			get_last_section_number(u_char * section);
	u_int32_t		get_platform_id(u_char * section);
	u_char			get_processing_order(u_char * section);
	u_int16_t		get_descriptor_loop_length(u_char * field_start);
	u_int32_t		get_crc(u_char * section);

	void	set_current_next_indicator(u_char current_next_indicator) 
	{
		_current_next_indicator = current_next_indicator;
	}

	CUR_NEXT_INDICATOR	get_current_next_indicator() 
	{
					return (CUR_NEXT_INDICATOR) _current_next_indicator;
	}

	void	set_version_number(u_char version_number)
	{
		_version_number = version_number;
	}

	u_char get_version_number()
	{
		return _version_number;
	}
};

#endif /* __NCTUNS_int_section_h__ */
