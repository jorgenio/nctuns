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

#ifndef __NCTUNS_tct_section_h__
#define __NCTUNS_tct_section_h__

#include "tct_table.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#define MAX_SECTIONS				1000
#define MAX_TCT_SECTION_SIZE			1024
#define MAX_TCT_SECTION_SIZE_NO_INCLUDING_CRC32	(MAX_TCT_SECTION_SIZE - CRC32_SIZE)
#define	TCT_TIEMSLOT_INFO_SIZE			sizeof(Tct_timeslot_info)



class Tct_table_to_section_handler {
  
  public:
	// These data are stored for section generation.
	Tct*				_tct_table;
	u_char				_table_id;
	u_int16_t			_network_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	Tct_timeslot_info_entry*	_ptr_timeslot_info_entry;

	// _section_length stores the value of 'section_length' filed defined in SI.
	// it's mean total_section_size must -3
	u_int16_t			_section_length[MAX_SECTIONS];
	u_int16_t			_last_section_number;
	u_int16_t			_current_section_number;

	void				_calculate_last_section_number_and_section_lengths(Tct* tct_table);

  public:
	void				tct_table_to_section_init(Tct* tct_table);
	void*				tct_table_to_section();

};

/* One Tct_section_to_table_handler instance would correspond to
 * the handling for one Tct table in the section module.
 * The pair (network_id, version_number) identifies
 * distinct Tct table.
 */
class Tct_section_to_table_handler {

  private:
	u_int16_t			_network_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	bool				_received[256];	// The section numbers of sections that have been received.
	u_char				_last_section_number;
	Tct*				_ptr_table;
	
	bool				_is_complete();

  public:
  	int 		init(void* section);

	Tct*	 	to_table(void* spt_section);

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
};// End of class Tct_section_to_table_handler. 


#endif /* __NCTUNS_tct_section_h__ */
