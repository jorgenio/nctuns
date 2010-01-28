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

#ifndef __NCTUNS_tbtp_table_h__
#define __NCTUNS_tbtp_table_h__

#include <list>
#include <stdio.h>
#include "table.h"
#include <mylist.h>
#include "section_draft.h"
#include <sys/types.h>

using namespace std;
#define		TBTP_FRAME_INFO_SIZE		sizeof(Tbtp_frame_info)
#define		TBTP_TIMESLOT_INFO_SIZE		sizeof(Tbtp_timeslot_info)

#pragma pack(1)

// 4B
struct Tbtp_timeslot_info {
	u_int16_t	logon_id;
	u_int16_t	multiple_channels_flag		: 1;
	u_int16_t	assignment_type			: 2;
	u_int16_t	vbdc_queue_empty_flag		: 1;
	u_int16_t	start_slot			: 12;

	// Get functions //
	u_int16_t	get_logon_id()
			{ return logon_id;}
	u_int8_t	get_multiple_channels_flag()
			{ return multiple_channels_flag;}
	u_int16_t	get_assignment_type()
			{ return assignment_type;}
	u_int16_t	get_vbdc_queue_empty_flag()
			{ return vbdc_queue_empty_flag;}
	u_int16_t	get_start_slot()
			{ return start_slot;}
	
	// Set functions //
	int		set_logon_id(int para_logon_id)
			{
				logon_id = para_logon_id;
				return 0;
			}
	int		set_multiple_channels_flag (int para_multiple_channels_flag )
			{
				multiple_channels_flag = para_multiple_channels_flag;
				return 0;
			}
	int		set_assignment_type (int para_assignment_type )
			{
				assignment_type = para_assignment_type;
				return 0;
			}
	int		set_vbdc_queue_empty_flag (int para_vbdc_queue_empty_flag )
			{
				vbdc_queue_empty_flag =  para_vbdc_queue_empty_flag;
				return 0;
			}
	int		set_start_slot (int para_start_slot )
			{
				start_slot =  para_start_slot;
				return 0;
			}
	
};

// Declare the entry of TBTP frame information queue. //
struct Tbtp_timeslot_info_entry {
	struct Tbtp_timeslot_info			timeslot_info;
	u_int8_t					channel_id;
	u_int16_t					assignment_count;
	CIRCLEQ_ENTRY(Tbtp_timeslot_info_entry)		entries;

	// Constructor //
	Tbtp_timeslot_info_entry(){} 
	Tbtp_timeslot_info_entry(Tbtp_timeslot_info timeslot_info);

	int		timeslot_info_len();

	// Get functions //
	u_int16_t	get_logon_id()
			{ return timeslot_info.get_logon_id();}
	u_int16_t	get_multiple_channels_flag()
			{ return timeslot_info.get_multiple_channels_flag();}
	u_int16_t	get_assignment_type()
			{ return timeslot_info.get_assignment_type();}
	u_int16_t	get_vbdc_queue_empty_flag()
			{ return timeslot_info.get_vbdc_queue_empty_flag();}
	u_int16_t	get_start_slot()
			{ return timeslot_info.get_start_slot();}
        u_int8_t	get_channel_id()
			{ return channel_id;}
        u_int16_t	get_assignment_count()
			{ return assignment_count;}

	// Set functions //
	int		set_logon_id(int para_logon_id)
			{
				return timeslot_info.set_logon_id(para_logon_id);
			}
	int		set_multiple_channels_flag (int para_multiple_channels_flag )
			{
				return timeslot_info.set_multiple_channels_flag (para_multiple_channels_flag );
			}
	int		set_assignment_type (int para_assignment_type )
			{
				return timeslot_info.set_assignment_type (para_assignment_type );
			}
	int		set_vbdc_queue_empty_flag (int para_vbdc_queue_empty_flag )
			{
				return timeslot_info.set_vbdc_queue_empty_flag (para_vbdc_queue_empty_flag );
			}
	int		set_start_slot (int para_start_slot )
			{
				return timeslot_info.set_start_slot (para_start_slot);
			}
	int		set_channel_id (int para_channel_id )
			{
				channel_id = para_channel_id;
				return 0;
			}
	int		set_assignment_count (int para_assignment_count)
			{
				assignment_count = para_assignment_count;
				return 0;
			}
};

// Declare the TBTP frame information queue. //
struct Tbtp_timeslot_info_circleq {
  public:
	struct Tbtp_timeslot_info_entry *cqh_first;		// first element //
	struct Tbtp_timeslot_info_entry *cqh_last;		// last element //

	void free();
	static void copy(Tbtp_timeslot_info_circleq* dst, Tbtp_timeslot_info_circleq* src);
};

struct Tbtp_timeslot_info_entry_list{
	Tbtp_timeslot_info_entry_list()	{data=0; next=0;}
	Tbtp_timeslot_info_entry	*data;
	Tbtp_timeslot_info_entry_list	*next;
};


struct Tbtp_frame_info {

	u_int8_t		frame_number;
        u_int16_t		btp_loop_count;
                                                                                            
        // Get functions //
        u_int8_t		get_frame_number()
       				{ return frame_number;}
        u_int16_t		get_btp_loop_count()
				{ return btp_loop_count;}
        
	// Set functions //
        int			set_frame_number(int para_frame_number)
       				{
					frame_number = para_frame_number;
					return 0;
				}
        int			set_btp_loop_count(int para_btp_loop_count)
				{ 
					btp_loop_count = para_btp_loop_count;
					return 0;
				}

};


// Declare the entry of TBTP frame information queue. //
struct Tbtp_frame_info_entry {
	struct Tbtp_frame_info				frame_info;
	CIRCLEQ_ENTRY(Tbtp_frame_info_entry)		entries;
	Tbtp_timeslot_info_circleq			tbtp_timeslot_info_circleq;


	// Constructor
	Tbtp_frame_info_entry(Tbtp_frame_info frame_info);

	// super_frame_info_size() would return the length of
	// * this superframe information in bytes.
	
	u_int32_t	frame_info_len();


        // Get functions //
        u_int8_t		get_frame_number()
       				{ return frame_info.get_frame_number();}
        u_int16_t		get_btp_loop_count()
				{ return frame_info.get_btp_loop_count();}
        
	// Set functions //
        int			set_frame_number(int para_frame_number)
       				{ return frame_info.set_frame_number(para_frame_number);}
        int			set_btp_loop_count(int para_btp_loop_count)
				{ return frame_info.set_btp_loop_count(para_btp_loop_count);}

};

// Declare the TBTP superframe information queue. //
struct Tbtp_frame_info_circleq {
  public:
	struct Tbtp_frame_info_entry *cqh_first;		// first element //	
	struct Tbtp_frame_info_entry *cqh_last;		// last element //

	void free();
	static void copy(Tbtp_frame_info_circleq* dst, Tbtp_frame_info_circleq* src);
};


class Tbtp : public Table {
  friend class Tbtp_table_to_section_handler;
  friend class Tbtp_section_to_table_handler;

  private:
	u_int16_t						_network_id;
	u_char							_version_number;
	u_char							_current_next_indicator;
	// Note: Superframe_loop_count==0 means no loop here.
	// 		 while in the TBTP section, Superframe_loop_count==0 
	//		 means one loop.

	u_int8_t						_group_id;
	u_int16_t						_superframe_count;
	u_int8_t						_frame_loop_count;
	
	Tbtp_frame_info_circleq					_tbtp_frame_info_circleq;

  public:
	Tbtp();


	Tbtp(u_int16_t _network_id, u_char _version_number, 
		u_char _current_next_indicator, u_int16_t superframe_count, u_int8_t group_id);

	~Tbtp();

	Tbtp*		copy();

	// Get functions //

	u_int16_t	get_network_id() {return _network_id;}
	
	u_char		get_version_number() {return _version_number;}
	
	u_char		get_current_next_indicator() {return _current_next_indicator;}
	
	u_int8_t	get_group_id() {return _group_id;}
	
	u_int16_t	get_superframe_count() {return _superframe_count;}
	
	u_int8_t	get_frame_loop_count() {return _frame_loop_count;}

	uint16_t	get_frame_number();

	// Set functions //

	int		set_network_id(u_int16_t network_id)
			{ _network_id = network_id; return 0;}

	int		set_version_number(u_char version_number)
			{ _version_number = version_number; return 0;}

	int		set_current_next_indicator(u_char current_next_indicator)
			{ _current_next_indicator = current_next_indicator; return 0;}

	int		set_group_id(u_int8_t group_id)
			{ _group_id = group_id; return 0;}
			
	int		set_superframe_count(u_int16_t superframe_count)
			{ _superframe_count = superframe_count; return 0;}
			
	int		set_frame_loop_count(u_int8_t frame_loop_count)
			{ _frame_loop_count = frame_loop_count; return 0;}
			
	// Table operation functions //


	int		add_frame_info(Tbtp_frame_info frame_info, Tbtp_timeslot_info_entry timeslot_info_entry);

	int		add_timeslot_info(u_char frame_number, Tbtp_timeslot_info_entry timeslot_info_entry);


	int		remove_frame_info(u_char frame_number);

	int		remove_timeslot_info(u_char frame_number, u_int16_t logon_id);

	
	int		get_frame_info(u_char frame_number, Tbtp_frame_info* frame_info_buf);

	int		get_timeslot_info(	
					u_char		frame_number,
					u_int16_t 	logon_id,
					u_int8_t 	&channel_id,
					Tbtp_timeslot_info_entry_list	*ptr_timeslot_info_entry,
					char		*no_of_slot_entry);
	
	int		get_timeslot_info_list(u_char frame_number,
					       uint16_t logon_id, 
					       uint8_t &channel_id,
					       list<Tbtp_timeslot_info_entry> &info_list);
}; // End of class Tbtp.
#pragma pack()

#endif	/* __NCTUNS_tbtp_table_h__ */
