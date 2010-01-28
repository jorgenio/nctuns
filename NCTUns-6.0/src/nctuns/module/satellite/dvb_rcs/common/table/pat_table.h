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

#ifndef __NCTUNS_pat_table_h__
#define	__NCTUNS_pat_table_h__

#define		PAT_DEBUG
#define		PAT_MAX_NUM_OF_ASSOCIATION	100
#define		network_pid			net_pro_pid
#define		program_map_pid			net_pro_pid
#include <stdio.h>
#include <sys/types.h>
#include "table.h"
#include "section_draft.h"
#include "mylist.h"
#pragma pack(1)
struct Pat_ass_info {
	u_int16_t			program_number;
	u_int16_t			reserved	: 3;
	u_int16_t			net_pro_pid	: 13;
}__attribute__((packed));

// Declare the entry of PAT-pid-association-information queue. //
struct Pat_ass_info_entry {
	struct Pat_ass_info		ass_info;
	CIRCLEQ_ENTRY(Pat_ass_info_entry)	entries;
	
	Pat_ass_info_entry(){}
	Pat_ass_info_entry(u_int16_t program_number, u_int16_t pid) {
		ass_info.program_number =  program_number;
		ass_info.program_map_pid = pid;
	}

	// Get functions//
	u_int16_t		get_program_number()
		{return ass_info.program_number;}

	u_int16_t		get_network_pid()
		{return ass_info.network_pid;}

	u_int16_t		get_program_map_pid()
		{return ass_info.program_map_pid;}
		
	// Set functions//
	void			set_program_number(u_int16_t pnum)
		{ass_info.program_number = pnum;}

	void			set_network_pid(u_int16_t npid)
		{ass_info.network_pid = npid;}

	void			set_program_map_pid(u_int16_t pmpid)
		{ass_info.program_map_pid = pmpid;}
};	// End of struct Pat_ass_info_entry. 


// Declare the PAT-pid-association-information queue. //
struct Pat_ass_info_circleq {

  public:
	struct Pat_ass_info_entry *cqh_first;		// first element //	
	struct Pat_ass_info_entry *cqh_last;		// last element //
  public:
	void free();
	static void copy(Pat_ass_info_circleq* dst,Pat_ass_info_circleq* src);
};

class Pat : public Table {
  friend class	Pat_table_to_section_handler;
  friend class	Pat_section_to_table_handler;

  private:
  	u_int16_t			_transport_stream_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	Pat_ass_info_circleq		_pat_ass_info_circleq;

  public:
						Pat();
						Pat(u_int16_t ts_id, u_char version_number, 
							u_char current_next_indicator);
						~Pat();
	Pat*				copy();

	// Get functions //
	u_int16_t	get_transport_stream_id() {return _transport_stream_id;}
	u_char		get_version_number() {return _version_number;}
	u_char		get_current_next_indicator() {return _current_next_indicator;}

	// Set functions //
	int		set_transport_stream_id(u_int16_t transport_stream_id) {
		_transport_stream_id = transport_stream_id; return 0;}

	int		set_version_number(u_char version_number) {
		_version_number =version_number; return 0;}

	int		set_current_next_indicator(u_char current_next_indicator) {
		_current_next_indicator= current_next_indicator; return 0;}


	// Table operation functions //
	int			get_pat_ass_info(u_int16_t program_number , Pat_ass_info *ass_info_buf);
	int			add_pat_ass_info(u_int16_t program_number, const u_int16_t pid);
	int			remove_pat_ass_info(u_int16_t program_number);

	/*
	 * add by bryan 06/6/3
	 */
	int16_t		get_PMT_pid(u_int16_t pro_num);
};
#pragma pack()

#endif /* __NCTUNS_pat_table_h__ */
