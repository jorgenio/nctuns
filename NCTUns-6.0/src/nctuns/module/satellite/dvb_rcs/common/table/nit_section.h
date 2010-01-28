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

#ifndef __NCTUNS_nit_section_h__
#define __NCTUNS_nit_section_h__

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "section_draft.h"
#include "nit_table.h"

#define NIT_SECTION_DRAFT_SIZE		sizeof(Nit_section_draft)
#define MAX_NIT_SECTION_SIZE		1024
#define MAX_NIT_SECTION_SIZE_NO_INCLUDING_CRC32	(MAX_NIT_SECTION_SIZE - CRC32_SIZE)
#define DESCRIPTORS_LENGTH_FIELD_LEN	2
#define DESCRIPTORS_LENGTH_FIELD_MASK	0x0FFF

#define INSERT_SEC_BUF(head, slen, sec_num, des1, cnt1, dlen1, ts, te, des2, cnt2, dlen2)	\
do {												\
	struct nit_sec_buf *sbuf = new struct nit_sec_buf;					\
	CIRCLEQ_INSERT_TAIL(head, sbuf, list);							\
	sbuf->len = slen;									\
	sbuf->section_number = sec_num;								\
	sbuf->des_start_num_1st = des1;								\
	sbuf->des_cnt_1st = cnt1;								\
	sbuf->des_len_1st = (dlen1) & DESCRIPTORS_LENGTH_FIELD_MASK;				\
	sbuf->des_start_num_2nd = des2;								\
	sbuf->des_cnt_2nd = cnt2;								\
	sbuf->des_len_2nd = (dlen2) & DESCRIPTORS_LENGTH_FIELD_MASK;				\
	sbuf->ts_start = ts;									\
	sbuf->ts_end = te;									\
} while (0);

/*
 * Note: Since total bytes for storage frame information for one superframe
 * is no larger than 256, the superframe information would not be segmented into
 * multiple sections.
 */
class Nit_table_to_section_handler {
  private:

	struct nit_sec_buf {
		CIRCLEQ_ENTRY(nit_sec_buf) list;
		int len;
		int section_number;
		uint8_t des_start_num_1st;
		uint8_t des_cnt_1st;
		u_int16_t des_len_1st;
		uint8_t des_start_num_2nd;
		uint8_t des_cnt_2nd;
		u_int16_t des_len_2nd;
		Nit_transport_stream_info_entry *ts_start;
		Nit_transport_stream_info_entry *ts_end;
	};

	// These data are stored for section generation.
	Nit*				_nit_table;
	u_char				_table_id;
	u_int16_t			_network_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	struct Nit_superframe_info_entry	*_ptr_superframe_info_entry;

	// _section_length stores the value of 'section_length' filed defined in SI.
	CIRCLEQ_HEAD(nit_sec_head, nit_sec_buf)	_sec_buf_head;
	u_int16_t			_last_section_number;
	u_int16_t			_current_section_number;

	void				_calculate_last_section_number_and_section_length();

  public:
	void				nit_table_to_section_init(Nit* Nit_table);
	void*				nit_table_to_section();

};

/* One Nit_section_to_table_handler instance would correspond to
 * the handling for one Nit table in the section module.
 * The pair (network_id, version_number) identifies
 * distinct Nit table.
 */
class Nit_section_to_table_handler {

  private:
	u_int16_t			_network_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	bool				*_received;	// The section numbers of sections that have been received.
	u_char				_last_section_number;
	Nit*				_nit_table;
	
	bool				_is_complete();

  public:

	Nit_section_to_table_handler() : _received(NULL), _nit_table(NULL) {};
	
	~Nit_section_to_table_handler() {
		if (_received)
			delete _received;
	};
  	int 		init(void* section);
	Nit*	 	to_table(void* nit_section);

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

};// End of class Nit_section_to_table_handler. 

#endif /* __NCTUNS_nit_section_h__ */
