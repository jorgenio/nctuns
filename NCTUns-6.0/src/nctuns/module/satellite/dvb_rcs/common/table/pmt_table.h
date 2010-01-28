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

#ifndef __NCTUNS_pmt_table_h__
#define __NCTUNS_pmt_table_h__

#include <stdio.h>
#include <stdarg.h>
#include "section_draft.h"
#include "table.h"
#include "../descriptor/descriptor_q.h"
#include "../../../../../mylist.h"

#define	PMT_DEBUG
#define	PMT_SUPERFRAME_INFO_SIZE		sizeof(Pmt_superframe_info)
#define	PMT_FRAME_INFO_SIZE			sizeof(Pmt_frame_info)
#pragma pack(1)
struct Pmt_frame_info {
	u_int8_t		stream_type;
	u_int16_t		reserved1			: 3;
	u_int16_t		elementary_pid			: 13;
	u_int16_t		reserved2			: 4;
	u_int16_t		es_info_length			: 12;
	

	// Get functions //
	u_int8_t		get_stream_type(){ return stream_type;}
	u_int16_t		get_elementary_pid(){ return elementary_pid;}
	u_int16_t		get_es_info_length(){ return es_info_length;}


	// Set functions //	
	int       	set_stream_type(int p_st){ stream_type = p_st; return 0;}
	int		set_elementary_pid(int p_ep){ elementary_pid = p_ep; return 0;}
	int		set_es_info_length(int p_eil){ es_info_length = p_eil; return 0;}
}__attribute__((packed));

// Declare the entry of PMT frame information queue. //
struct Pmt_frame_info_entry {
	struct Pmt_frame_info					frame_info;
	
	// use for count frame number
	CIRCLEQ_ENTRY(Pmt_frame_info_entry)			entries;
	Descriptor_circleq					*pmt_frame_info_des_circleq;


	Pmt_frame_info_entry();
	~Pmt_frame_info_entry(){ delete pmt_frame_info_des_circleq;} 
	Pmt_frame_info_entry(Pmt_frame_info info);

	int			frame_info_len();
	// Get functions //
	u_int8_t		get_stream_tyep(){ return frame_info.get_stream_type();}
	u_int16_t		get_elementary_pid(){ return frame_info.get_elementary_pid();}
	u_int16_t		get_es_info_length(){ return frame_info.get_es_info_length();}


	// Set functions //
	int       	set_stream_type(int p_st){ return frame_info.set_stream_type(p_st);}
	int		set_elementary_pid(int p_ep){ return frame_info.set_elementary_pid(p_ep);}
	int		set_es_info_length(int p_eil){ return frame_info.set_es_info_length(p_eil);}
};

// Declare the PMT frame information queue. //
struct Pmt_frame_info_circleq {
  public:
	struct Pmt_frame_info_entry *cqh_first;		// first element //	
	struct Pmt_frame_info_entry *cqh_last;		// last element //

	void free();
	static void copy(Pmt_frame_info_circleq* dst, Pmt_frame_info_circleq* src);
};


class Pmt : public Table {
  friend class Pmt_table_to_section_handler;
  friend class Pmt_section_to_table_handler;

  private:
	u_int16_t				_program_number;
	u_char					_version_number;
	u_char					_current_next_indicator;
	// Note: Superframe_loop_count==0 means no loop here.
	// * 		 while in the PMT section, Superframe_loop_count==0 
	// *		 means one loop.
	//

	u_int16_t				_pcr_pid;

	u_int32_t				_frame_loop_count;

	Descriptor_circleq			*_pmt_des_circleq;

	Pmt_frame_info_circleq			_pmt_frame_info_circleq;

  public:
	Pmt();

	Pmt(u_int16_t _ts_id, u_char _version_number, 
		u_char _current_next_indicator);

	~Pmt();


	// Get functions //

	u_int16_t	get_program_number() {return _program_number;}

	u_char		get_version_number() {return _version_number;}

	u_char		get_current_next_indicator() {return _current_next_indicator;}

	u_int32_t	get_frame_loop_count() {return _frame_loop_count;}

	u_int16_t	get_pcr_pid() {return _pcr_pid;}


	// Set functions //

	u_int16_t	set_program_number(u_int16_t pcr_pid){_pcr_pid = pcr_pid;return 0;};

	int		set_version_number(u_char version_number) 
				{_version_number =version_number; return 0;}

	int		set_current_next_indicator(u_char current_next_indicator) 
				{_current_next_indicator= current_next_indicator; return 0;}

	u_int32_t	set_frame_loop_count(int slc) { _frame_loop_count = slc; return 0;}

	u_int16_t	set_pcr_pid(u_int16_t pcr_pid){_pcr_pid = pcr_pid;return 0;};




	// Table operation functions //
	
	// program_info_descriptor operations
	int			add_program_info_descriptor(Descriptor *dct);

	Descriptor*		get_program_info_descriptor(int pnum , uint8_t searched_des , ...);

	int			remove_program_info_descriptor(int pnum , uint8_t deleted_des , ...);
	
	
	// frame_info operations
	int			add_frame_info(Pmt_frame_info frame_info);

	int			get_frame_info(int elementary_pid, Pmt_frame_info* frame_info_buf);

	int			remove_frame_info(int elementary_pid);


	// es_info_descriptor operations
	int			add_es_info_descriptor(int elementary_pid, Descriptor *dct);

	Descriptor*		get_es_info_descriptor(int elementary_pid, int pnum , uint8_t searched_des , ...);

	int			remove_es_info_descriptor(int elementary_pid, int pnum , uint8_t deleted_des , ...);

	int			get_es_info_des_pid(int pnum, u_int8_t tag, ...);


	Pmt*		copy();

}; // End of class Pmt.
#pragma pack()

#endif	/* __NCTUNS_pmt_table */
