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

// sct_table.h

#ifndef __NCTUNS_sct_table_h__
#define __NCTUNS_sct_table_h__

#include <stdio.h>
#include "table.h"
#include "section_draft.h"
#include <mylist.h>

#define	SCT_SUPERFRAME_INFO_SIZE	sizeof(Sct_superframe_info)
#define	SCT_FRAME_INFO_SIZE		sizeof(Sct_frame_info)

#pragma pack(1)
struct Sct_frame_info {
	uint64_t		frame_id			: 8;
	uint64_t		frame_start_time		: 32;
	uint64_t		frame_centre_frequency_offset	: 24;
	

	/* Get functions */
	uint8_t			get_frame_id();
	uint32_t		get_frame_start_time();
	int32_t			get_frame_centre_frequency_offset();

	/* Set functions */
	int			set_frame_id(uint8_t frame_id);
	int			set_frame_start_time(uint32_t frame_start_time);
	int			set_frame_centre_frequency_offset(int32_t frame_centre_frequency_offset);
};

/* Declare the entry of SCT frame information queue. */
struct Sct_frame_info_entry {
	struct Sct_frame_info				frame_info;
	char						frame_number;
	CIRCLEQ_ENTRY(Sct_frame_info_entry)		entries;


	Sct_frame_info_entry(){} 
	Sct_frame_info_entry(Sct_frame_info info);
	/* Get functions */
	char		get_frame_number() { return frame_number;}
	uint8_t		get_frame_id() { return frame_info.get_frame_id();}
	uint32_t 	get_frame_start_time() { return frame_info.get_frame_start_time();}
	int32_t		get_frame_centre_frequency_offset() { return frame_info.get_frame_centre_frequency_offset();}

	/* Set functions */
	int		set_frame_number(char f_number) { frame_number = f_number; return 0;}
	int		set_frame_id(uint64_t id) { return frame_info.set_frame_id(id);}
	int		set_frame_start_time(uint32_t start_time) { return frame_info.set_frame_start_time(start_time);}
	int		set_frame_centre_frequency_offset(int32_t centre_frequency_offset)
					{ return frame_info.set_frame_centre_frequency_offset(centre_frequency_offset);}
};

/* Declare the SCT frame information queue. */
struct Sct_frame_info_circleq {
  public:
	struct Sct_frame_info_entry *cqh_first;		/* first element */	
	struct Sct_frame_info_entry *cqh_last;		/* last element */

	void free();
	static void copy(Sct_frame_info_circleq* dst, Sct_frame_info_circleq* src);
};
	
struct Sct_superframe_info {

	char		superframe_id;
	char		uplink_polarization;
	char		start_time_base_ext[6];
	uint32_t	superframe_duration;
	uint32_t	superframe_centre_frequency;
	uint16_t	superframe_counter;
	char		frame_loop_count;


	/* Get functions */

	uint8_t		get_superframe_id();

	uint8_t		get_uplink_polarization();

	uint64_t	get_superframe_start_time_base();

	uint64_t	get_superframe_start_time_ext();

	uint32_t	get_superframe_duration();

	uint32_t	get_superframe_centre_frequency();

	uint16_t	get_superframe_counter();

	uint8_t		get_frame_loop_count();
	

	/* Set functions */
	int		set_superframe_id(uint8_t para_superframe_id);

	int		set_uplink_polarization(uint8_t para_uplink_polarization);

	int		set_superframe_start_time_base(uint64_t para_superframe_start_time_base);

	int		set_superframe_start_time_ext(uint64_t para_superframe_start_time_ext);

	int		set_superframe_duration(uint32_t para_superframe_duration);

	int		set_superframe_centre_frequency(uint32_t para_superframe_centre_frequency);

	int		set_superframe_counter(uint16_t para_superframe_counter);

	int		set_frame_loop_count(uint8_t para_frame_loop_count);
};


/* Declare the entry of SCT superframe information queue. */
struct Sct_superframe_info_entry {
	struct Sct_superframe_info			superframe_info;
	CIRCLEQ_ENTRY(Sct_superframe_info_entry)	entries;
	Sct_frame_info_circleq				sct_frame_info_circleq;


	Sct_superframe_info_entry(Sct_superframe_info superframe_info);

	/* super_frame_info_size() would return the length of
	 * this superframe information in bytes.
	 */
	uint32_t	superframe_info_len();

	/* Get functions */
	char 		get_superframe_id() { return 	superframe_info.get_superframe_id();}

	char 		get_uplink_polarization() { return superframe_info.get_uplink_polarization();}

	uint64_t 	get_superframe_start_time_base() { return superframe_info.get_superframe_start_time_base();}

	uint64_t 	get_superframe_start_time_ext() { return superframe_info.get_superframe_start_time_ext();}

	uint32_t 	get_superframe_duration() { return superframe_info.get_superframe_duration();}

	uint32_t 	get_superframe_centre_frequency() { return superframe_info.get_superframe_centre_frequency();}

	uint16_t 	get_superframe_counter() { return superframe_info.get_superframe_counter();}

	char 		get_frame_loop_count() { return superframe_info.get_frame_loop_count();}

	/* Set functions */
	void set_superframe_id(char id) { superframe_info.set_superframe_id(id);}

	void set_uplink_polarization(char uplink_polar) { superframe_info.set_uplink_polarization(uplink_polar);}

	void set_superframe_start_time_base(uint64_t start_time_base) { superframe_info.set_superframe_start_time_base(start_time_base);}

	void set_superframe_start_time_ext(uint64_t start_time_ext) { superframe_info.set_superframe_start_time_ext(start_time_ext);}

	void set_superframe_duration(uint32_t duration) { superframe_info.set_superframe_duration(duration);}

	void set_superframe_centre_frequency(uint32_t centre_frequency) { superframe_info.set_superframe_centre_frequency(centre_frequency);}

	void set_superframe_counter(uint16_t counter) { superframe_info.set_superframe_counter(counter);}

	void set_frame_loop_count(uint8_t loop_count) { superframe_info.set_frame_loop_count(loop_count);}
};

/* Declare the SCT superframe information queue. */
struct Sct_superframe_info_circleq {
  public:
	struct Sct_superframe_info_entry *cqh_first;		/* first element */	
	struct Sct_superframe_info_entry *cqh_last;		/* last element */

	void free();
	static void copy(Sct_superframe_info_circleq* dst, Sct_superframe_info_circleq* src);
};


class Sct : public Table {
  friend class Sct_table_to_section_handler;
  friend class Sct_section_to_table_handler;

  private:
	uint16_t	_network_id;
	u_char		_version_number;
	u_char		_current_next_indicator;
	/* Note: Superframe_loop_count==0 means no loop here.
	 * 		 while in the SCT section, Superframe_loop_count==0 
	 *		 means one loop.
	 */

	// Question, what is this loop_count function, what diff with Sct_section_draft's loop_count
	uint32_t	_superframe_loop_count;
	
	Sct_superframe_info_circleq		_sct_superframe_info_circleq;

  public:
		Sct();

		Sct(uint16_t _network_id, u_char _version_number, 
			u_char _current_next_indicator);

		~Sct();

	Sct*		copy();

	/* Get functions */

	uint16_t	get_network_id() {return _network_id;}

	u_char		get_version_number() {return _version_number;}

	u_char		get_current_next_indicator() {return _current_next_indicator;}

	uint32_t	get_superframe_loop_count() {return _superframe_loop_count;}


	/* Set functions */

	int			set_network_id(uint16_t network_id) {
			_network_id = network_id; return 0;}

	int			set_version_number(u_char version_number) {
			_version_number =version_number; return 0;}

	int			set_current_next_indicator(u_char current_next_indicator) {
			_current_next_indicator= current_next_indicator; return 0;}

	uint32_t	set_superframe_loop_count(int slc) { _superframe_loop_count = slc; return 0;}
	/* Table operation functions */
	
	int			add_superframe_info(Sct_superframe_info superframe_info, Sct_frame_info frame_info);

	/* add_frame_info() add one frame information 'frame_info' to the superframe information
	 * with id 'superframe_id'. 'frame_info' is given frame_number 'frame_number'.
	 */
	int			add_frame_info(u_char superframe_id, uint16_t superframe_counter, Sct_frame_info frame_info, u_char frame_number);

	/* remove_frame_info() remove one frame information of frame_number 'frame_number'
	 * from superframe information of superframe_id 'superframe_id'.
	 */
	int			remove_frame_info(u_char superframe_id, uint16_t superframe_counter, u_char frame_number);

	/* remove_superframe_info() remove one superframe information
	 * of superframe_id 'superframe_id'.
	 */
	int			remove_superframe_info(u_char superframe_id ,uint16_t superframe_counter);

	int			get_superframe_info(u_char superframe_id, uint16_t superframe_counter, Sct_superframe_info* superframe_info_buf);

	bool			get_frame_info(u_char superframe_id, uint16_t superframe_counter, u_char frame_number, Sct_frame_info* frame_info_buf);

}; // End of class Sct.
#pragma pack()
#endif /* __NCTUNS_sct_table_h__ */
