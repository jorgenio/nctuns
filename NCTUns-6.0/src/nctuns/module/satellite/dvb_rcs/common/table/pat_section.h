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

#ifndef __NCTUNS_pat_section_h__
#define __NCTUNS_pat_section_h__


#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include "pat_table.h"

#define	PAT_ASS_INFO_SIZE			sizeof(Pat_ass_info)
#define MAX_SECTIONS				1000
#define MAX_PAT_SECTION_SIZE		1024
#define MAX_PAT_SECTION_SIZE_NO_INCLUDING_CRC32	(MAX_PAT_SECTION_SIZE - CRC32_SIZE)


class Pat_table_to_section_handler {
  private:
	// These data are stored for section generation.
	Pat*				_pat_table;
	u_char				_table_id;
	u_int16_t			_transport_stream_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	// _section_length stores the value of 'section_length' defined in PSI.
	u_int16_t			_section_length[MAX_SECTIONS];
	u_char				_last_section_number;
	int				_current_section_number;
	Pat_ass_info_entry*		_ptr_info_entry;


  public:
	void				_calculate_last_section_number_and_section_lengths(Pat* pat_table);

  public:
	void				pat_table_to_section_init(Pat* pat_table);
	void*				pat_table_to_section();
};

/* One Pat_section_to_table_handler instance would correspond to
 * the handling for one PAT table in the section module.
 * The pair (transport_stream_id, version_number) identifies
 * distinct PAT table.
 */
class Pat_section_to_table_handler {

  private:
	u_int16_t			_transport_stream_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	bool				_received[256];
	u_char				_last_section_number;
	Pat*				_ptr_table;
	
	bool				_is_complete();

  public:
  	int 		init(void* section);

	Pat*	 	to_table(void* pat_section);

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


#endif /* __NCTUNS_pat_section_h__ */
