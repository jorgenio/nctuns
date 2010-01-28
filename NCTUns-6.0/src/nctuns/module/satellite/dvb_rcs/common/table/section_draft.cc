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

#include "section_draft.h"
#include <string.h>

bool crc_okay(void* section)
{
	u_int16_t	section_total_len;
	u_int32_t crc32;

	section_total_len = section_total_length(section);
	memcpy(&crc32, (u_int32_t *)((char *)section + (section_total_len - 4)), 4);
	if ((crc32 | CRC32(section, section_total_len - 4, IEEE_8023_CRC32_POLY, 1)) == 0xFFFFFFFF)
		return true;
	else
		return false;
}


void fill_crc(void* section)
{
	u_int16_t	section_total_len, offset;
	u_int32_t	crc32;

	section_total_len = section_total_length(section);
	crc32 = ~CRC32(section, section_total_len - CRC32_SIZE, IEEE_8023_CRC32_POLY, 1);

	offset = section_total_len - CRC32_SIZE;
	memcpy((char*)section+offset, &crc32, sizeof(u_int32_t));
}


u_int16_t	section_total_length(void* section)
{
	Section_draft* s_ptr = (Section_draft*) section;
	return s_ptr-> get_section_total_length();
}


u_int8_t	table_id(void* section)
{
	return *((char*) section);
}

u_int8_t	version_number(void* section)
{
	Psi_section_draft	*draft	= (Psi_section_draft*) section;
	return draft ->get_version_number();
}

CUR_NEXT_INDICATOR current_next_indicator(void* section)
{
	Psi_section_draft	*draft	= (Psi_section_draft*) section;
	return (CUR_NEXT_INDICATOR) (draft ->get_current_next_indicator());
}


u_int16_t Section_draft::get_section_length()
{
	return	_section_length;
}


u_int16_t Section_draft::get_section_total_length()
{
	return	_section_length + SECTION_DRAFT_SIZE;
}

int	Section_draft::set_section_draft(u_int16_t	table_id,
	u_int16_t section_syntax_indicator, u_int16_t private_indicator,
		u_int16_t section_length) {
	_table_id = table_id;
	_section_syntax_indicator = section_syntax_indicator;
	_private_indicator = private_indicator;
	_section_length = section_length;
	return 0;
}

void Psi_section_draft::set_psi_section_draft(u_int16_t tid_or_pnum,
						u_char version_number, u_char current_next_indicator,
						u_char section_number, u_char last_section_number)
{
		_program_number = tid_or_pnum;
		_version_number = version_number;
		_current_next_indicator = current_next_indicator;
		_section_number = section_number;
		_last_section_number = last_section_number;
}

int Si_type_a_section_draft::set_si_type_a_section_draft(
		u_int16_t		network_id,
		u_char			version_number,
		u_char			current_next_indicator,
		u_char			section_number,
		u_char			last_section_number		) {
		_network_id = network_id,
		_version_number = version_number;
		_current_next_indicator = current_next_indicator;
		_section_number = section_number;
		_last_section_number = last_section_number;
		return 0;
}
