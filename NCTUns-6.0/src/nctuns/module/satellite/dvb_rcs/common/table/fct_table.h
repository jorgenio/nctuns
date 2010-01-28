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

// fct_table.h

#ifndef __NCTUNS_fct_table_h__
#define __NCTUNS_fct_table_h__

#include <stdio.h>
#include <mylist.h>
#include "table.h"
#include "section_draft.h"

#define		FCT_FRAME_INFO_SIZE		sizeof(Fct_frame_info)
#define		FCT_TIMESLOT_INFO_SIZE		sizeof(Fct_timeslot_info)

#pragma pack(1)

struct Fct_timeslot_info {
	u_int64_t	timeslot_frequency_offset	: 24;
	u_int64_t	timeslot_time_offset		: 32;
	u_int64_t	timeslot_id			: 8;
	u_int64_t	repeat_count			: 8;

	// Get functions //
	int32_t		get_timeslot_frequency_offset()
			{
				int d, e;
				d = timeslot_frequency_offset;
				e = d>>23;
				if(e==1)
				d = d ^ 0xff000000;
				return d;}
	u_int32_t	get_timeslot_time_offset()
			{ return timeslot_time_offset;}
	u_int8_t	get_timeslot_id()
			{ return timeslot_id;}
	u_int8_t	get_repeat_count()
			{ return repeat_count;}

	// Set functions //
	int		set_timeslot_frequency_offset(int para_timeslot_frequency_offset)
			{
				timeslot_frequency_offset = para_timeslot_frequency_offset;
				return 0;
			}
	int		set_timeslot_time_offset(int para_timeslot_time_offset)
			{
				timeslot_time_offset = para_timeslot_time_offset;
				return 0;
			}
	int		set_timeslot_id(int para_timeslot_id)
			{
				timeslot_id = para_timeslot_id;
				return 0;
			}
	int		set_repeat_count(int para_repeat_count)
			{
				repeat_count = para_repeat_count;
				return 0;
			}
};

// Declare the entry of SCT frame information queue. //
struct Fct_timeslot_info_entry {
	struct Fct_timeslot_info			timeslot_info;
	u_int16_t					timeslot_number;
	CIRCLEQ_ENTRY(Fct_timeslot_info_entry)		entries;

	// Constructor //
	Fct_timeslot_info_entry(){} 
	Fct_timeslot_info_entry(Fct_timeslot_info timeslot_info);

	u_int32_t	timeslot_info_len();

	// Get functions //
	u_int16_t	get_timeslot_number() { return timeslot_number;}
	int		get_timeslot_frequency_offset(){ return timeslot_info.get_timeslot_frequency_offset();}
	u_int32_t	get_timeslot_time_offset(){ return timeslot_info.timeslot_time_offset;}
	u_int8_t	get_timeslot_id(){ return timeslot_info.timeslot_id;}
	u_int8_t	get_repeat_count(){ return timeslot_info.repeat_count;}

	// Set functions //
	void		set_timeslot_number(u_int16_t f_number)
			{ timeslot_number = f_number;}
			
	int		set_timeslot_frequency_offset(int para_timeslot_frequency_offset)
			{ return timeslot_info.set_timeslot_frequency_offset(para_timeslot_frequency_offset);}
	int		set_timeslot_time_offset(int para_timeslot_time_offset)
			{ return timeslot_info.set_timeslot_time_offset(para_timeslot_time_offset);}
	int		set_timeslot_id(int para_timeslot_id)
			{ return timeslot_info.set_timeslot_id(para_timeslot_id);}
	int		set_repeat_count(int para_repeat_count)
			{ return timeslot_info.set_repeat_count(para_repeat_count);}

	void		print_int_to_binary_format(int offset);
};

// Declare the SCT frame information queue. //
struct Fct_timeslot_info_circleq {
  public:
	struct Fct_timeslot_info_entry *cqh_first;		// first element //
	struct Fct_timeslot_info_entry *cqh_last;		// last element //

	void free();
	static void copy(Fct_timeslot_info_circleq* dst, Fct_timeslot_info_circleq* src);
};


struct Fct_frame_info {

	u_int8_t		frame_id;
        u_int32_t		frame_duration;
        u_int16_t		total_timeslot_count;
        u_int16_t		start_timeslot_number;
        u_int8_t		timeslot_loop_count;
                                                                                            
        // Get functions //
        u_int8_t		get_frame_id()
       				{ return frame_id;}
        int			get_frame_duration()
				{ return frame_duration;}
        u_int16_t		get_total_timeslot_count()
				{ return total_timeslot_count;}
        u_int16_t		get_start_timeslot_number()
				{ return start_timeslot_number;	}
        u_int8_t		get_timeslot_loop_count()
				{ return timeslot_loop_count;}
                                                                                            
        // Set functions //
        int			set_frame_id(u_int8_t para_frame_id)
				{
					frame_id = para_frame_id;
					return 0;
				}
        int			set_frame_duration(u_int32_t para_frame_duration)
				{
					frame_duration = para_frame_duration;
					return 0;
				}
        int			set_total_timeslot_count(u_int32_t para_total_timeslot_count)
				{
					total_timeslot_count = para_total_timeslot_count;
					return 0;
				}
        int			set_start_timeslot_number(u_int32_t para_start_timeslot_number)
				{
					start_timeslot_number = para_start_timeslot_number;
					return 0;
				}
        int			set_timeslot_loop_count(u_int32_t para_timeslot_loop_count)
				{
					timeslot_loop_count = para_timeslot_loop_count;
					return 0;
				}

};


// Declare the entry of SCT superframe information queue. //
struct Fct_frame_info_entry {
	struct Fct_frame_info				frame_info;
	CIRCLEQ_ENTRY(Fct_frame_info_entry)		entries;
	Fct_timeslot_info_circleq			fct_timeslot_info_circleq;


	// Constructor
	Fct_frame_info_entry(Fct_frame_info frame_info);

	// super_frame_info_size() would return the length of
	// * this superframe information in bytes.
	
	u_int32_t	frame_info_len();

        // Get functions //
        u_int8_t		get_frame_id()
				{ return frame_info.get_frame_id(); }
        u_int32_t		get_frame_duration()
				{ return frame_info.get_frame_duration();}
        u_int16_t		get_total_timeslot_count()
				{ return frame_info.get_total_timeslot_count();}
        u_int16_t		get_start_timeslot_number()
				{ return frame_info.get_start_timeslot_number();}
        u_int8_t		get_timeslot_loop_count()
				{ return frame_info.get_timeslot_loop_count();}
                                                                                            
        // Set functions //
        int			set_frame_id(u_int8_t frame_id)
				{ return frame_info.set_frame_id(frame_id);}
        int			set_frame_duration(u_int32_t frame_duration)
				{ return frame_info.set_frame_duration( frame_duration);}
        int			set_total_timeslot_count(u_int32_t total_timeslot_count)
				{ return frame_info.set_total_timeslot_count(total_timeslot_count);}
        int			set_start_timeslot_number(u_int32_t start_timeslot_number)
				{ return frame_info.set_start_timeslot_number(start_timeslot_number);}
        int			set_timeslot_loop_count(u_int32_t timeslot_loop_count)
				{ return frame_info.set_timeslot_loop_count(timeslot_loop_count);}

};

// Declare the SCT superframe information queue. //
struct Fct_frame_info_circleq {
  public:
	struct Fct_frame_info_entry *cqh_first;		// first element //	
	struct Fct_frame_info_entry *cqh_last;		// last element //

	void free();
	static void copy(Fct_frame_info_circleq* dst, Fct_frame_info_circleq* src);
};


class Fct : public Table {
  friend class Fct_table_to_section_handler;
  friend class Fct_section_to_table_handler;

  private:
	u_int16_t			_network_id;
	u_char				_version_number;
	u_char				_current_next_indicator;
	// Note: Superframe_loop_count==0 means no loop here.
	// 		 while in the SCT section, Superframe_loop_count==0 
	//		 means one loop.
	 //
	u_int32_t			_frame_loop_count;
	
	
	Fct_frame_info_circleq		_fct_frame_info_circleq;

  public:
				Fct();

				Fct(u_int16_t _network_id, u_char _version_number, 
					u_char _current_next_indicator);

				~Fct();


	// Get functions //

	u_int16_t	get_network_id() {return _network_id;}

	u_char		get_version_number() {return _version_number;}

	u_char		get_current_next_indicator() {return _current_next_indicator;}

	u_int32_t	get_frame_loop_count() {return _frame_loop_count;}


	// Set functions //

	int		set_network_id(u_int16_t network_id)
			{ _network_id = network_id; return 0;}

	int		set_version_number(u_char version_number)
			{ _version_number =version_number; return 0;}

	int		set_current_next_indicator(u_char current_next_indicator)
			{ _current_next_indicator= current_next_indicator; return 0;}

	u_int32_t	set_frame_loop_count(int flc) { _frame_loop_count = flc;return 0;}
	// Table operation functions //
	Fct*		copy();

	int		add_frame_info(Fct_frame_info frame_info, Fct_timeslot_info timeslot_info);

	int		add_timeslot_info(u_char frame_id, Fct_timeslot_info timeslot_info, u_int16_t timeslot_number);


	int		remove_frame_info(u_char frame_id);

	int		remove_timeslot_info(u_char frame_id, u_int16_t timeslot_number);


	int		get_frame_info(u_char frame_id, Fct_frame_info* frame_info_buf);

	int		get_timeslot_info(u_char frame_id, u_int16_t timeslot_number, Fct_timeslot_info* timeslot_info_buf);


	int 		table_len();

}; // End of class Sct.

#pragma pack()

#endif /*__NCTUNS_fct_table_h__ */
