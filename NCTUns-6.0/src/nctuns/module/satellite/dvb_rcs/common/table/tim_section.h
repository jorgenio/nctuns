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

#ifndef __NCTUNS_tim_section_h__
#define	__NCTUNS_tim_section_h__

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "section_draft.h"
#include "tim_table.h"

#define MAX_SECTIONS				1000
#define MAX_TIM_SECTION_SIZE			1024
#define MAX_TIM_SECTION_SIZE_NO_INCLUDING_CRC32	(MAX_TIM_SECTION_SIZE - CRC32_SIZE)



// Note: Since total bytes for storage frame information for one superframe
// * is no larger than 256, the superframe information would not be segmented into
// * multiple sections.
 //
class Tim_table_to_section_handler {
  private:
	// These data are stored for section generation.
	Tim*				_tim_table;
	u_char				_table_id;
	u_char				_current_next_indicator;

	// use to record where the table cut point
	Descriptor_entry		*_dct_entry;


	// _section_length stores the value of 'section_length' filed defined in SI.
	u_int16_t			_section_length[MAX_SECTIONS];
	u_int16_t			_last_section_number;
	u_int16_t			_current_section_number;


  public:
	void				_calculate_last_section_number_and_section_lengths(Tim* Tim_table);
	void				tim_table_to_section_init(Tim* Tim_table);
	void*				tim_table_to_section();

};

// One Tim_section_to_table_handler instance would correspond to
// * the handling for one Tim table in the section module.
// * The pair (network_id, version_number) identifies
// * distinct Tim table.
 //
class Tim_section_to_table_handler {

  private:
	u_char				_current_next_indicator;
	bool				_received[256];	// The section numbers of sections that have been received.
	u_char				_last_section_number;
	Tim*				_ptr_table;
	
	bool				_is_complete();

  public:
  	int 		init(void* section);

	void		set_current_next_indicator(u_char current_next_indicator) {
					_current_next_indicator = current_next_indicator;}

	u_char		get_current_next_indicator() {
					return _current_next_indicator;}
	Tim*	 	to_table(void* tim_section);
};// End of class Tim_section_to_table_handler. 

#endif /* __NCTUNS_TIM_SECTION__ */
