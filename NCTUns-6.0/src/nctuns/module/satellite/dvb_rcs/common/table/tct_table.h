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

#ifndef __NCTUNS_tct_table_h__
#define __NCTUNS_tct_table_h__

#define	TCT_DEBUG
#define	TCT_TIMESLOT_INFO_SIZE			sizeof(Tct_timeslot_info)

#include <stdint.h>
#include "table.h"
#include "section_draft.h"
#include <mylist.h>
#include <stdio.h>

#pragma pack(1)
struct Permutation_parameters {
	uint8_t		p0;
	uint16_t	p1;
	uint16_t	p2;
	uint16_t	p3;
};

enum Timeslot_type
{
	TIMESLOT_TYPE_TRF_WITH_ONE_ATM_CELL		= 0x01,
	TIMESLOT_TYPE_TRF_WITH_TWO_ATM_CELL		= 0x02,
	TIMESLOT_TYPE_TRF_WITH_FOUR_ATM_CELL		= 0x04,
	TIMESLOT_TYPE_TRF_WITH_MPEG2_TS_PACKET	= 0x05,
	TIMESLOT_TYPE_CSC				= 0x06,
	TIMESLOT_TYPE_ACQ				= 0x07,
	TIMESLOT_TYPE_SYNC				= 0x08
};



struct Tct_timeslot_info {
	uint8_t		timeslot_id;
	uint64_t	symbol_rate			: 24;
	uint64_t	timeslot_duration		: 24;
	uint64_t	burst_start_offset		: 16;
	uint8_t		inner_code_type			: 1;
	uint8_t		inner_code_ordering		: 1;
	uint8_t		outer_coding			: 2;
	uint8_t		inner_code_puncturing		: 4;
	uint8_t		modulation			: 5;
	uint8_t		baseband_shaping		: 3;
	uint8_t		timeslot_payload_type;
	uint8_t		route_id_flag			: 1;
	uint8_t		acm_flag			: 1;
	uint8_t		sac_length			: 6;
	uint8_t		request_flag			: 1;
	uint8_t		m_and_c_flag			: 1;
	uint8_t		group_id_flag			: 1;
	uint8_t		logon_id_flag			: 1;
	uint8_t		capacity_requests_number	: 3;
	uint8_t		new_permutation 		: 1;
	



	// Get functions //
	int		get_timeslot_id();

	uint64_t	get_symbol_rate();

	int		get_timeslot_duration();

	int		get_burst_start_offset();

	int		get_inner_code_type();

	int		get_inner_code_ordering();

	int		get_outer_coding();

	int		get_inner_code_puncturing();

	int		get_modulation();

	int		get_baseband_shaping();

	int		get_timeslot_payload_type();

	int		get_route_id_flag();

	int		get_acm_flag();

	int		get_sac_length();

	int		get_request_flag();

	int		get_m_and_c_flag();

	int		get_group_id_flag();

	int		get_logon_id_flag();

	int		get_capacity_requests_number();

	int		get_new_permutation();


	// Set functions //
	int				set_timeslot_id(int timeslot_id);

	int				set_symbol_rate(uint64_t symbol_rate);

	int				set_timeslot_duration(int timeslot_duration);

	int				set_burst_start_offset(int burst_start_offset);

	int				set_inner_code_type(int inner_code_type);

	int				set_inner_code_ordering(int inner_code_ordering);

	int				set_outer_coding(int outer_coding);

	int				set_inner_code_puncturing(int inner_code_puncturing);

	int				set_modulation(int modulation);

	int				set_baseband_shaping(int baseband_shaping);

	int				set_timeslot_payload_type(int timeslot_payload_type);

	int				set_route_id_flag(int route_id_flag);

	int				set_acm_flag(int acm_flag);

	int				set_sac_length(int sac_length);

	int				set_request_flag(int request_flag);

	int				set_m_and_c_flag(int m_and_c_flag);

	int				set_group_id_flag(int group_id_flag);

	int				set_logon_id_flag(int logon_id_flag);

	int				set_capacity_requests_number(int capacity_requests_number);

	int				set_new_permutation(int new_permutation);
};

// Declare the entry of TCT timeslot information queue. //
struct Tct_timeslot_info_entry {
	struct Tct_timeslot_info		timeslot_info;
	struct Permutation_parameters		*permutation_parameters;
	uint8_t					preamble_length;
	CIRCLEQ_ENTRY(Tct_timeslot_info_entry)	entries;


	// Constructors //
	Tct_timeslot_info_entry();
	Tct_timeslot_info_entry(Tct_timeslot_info info);


	// Destructor //
	~Tct_timeslot_info_entry();

	// total_length() return the total size //
	int		timeslot_info_len();

	// Get functions //
	int		get_timeslot_id();

	uint64_t	get_symbol_rate();

	int		get_timeslot_duration();

	int		get_burst_start_offset();

	int		get_inner_code_type();

	int		get_inner_code_ordering();

	int		get_outer_coding();

	int		get_inner_code_puncturing();

	int		get_modulation();

	int		get_baseband_shaping();

	int		get_timeslot_payload_type();

	int		get_route_id_flag();

	int		get_acm_flag();

	int		get_sac_length();

	int		get_request_flag();

	int		get_m_and_c_flag();

	int		get_group_id_flag();

	int		get_logon_id_flag();

	int		get_capacity_requests_number();

	int		get_new_permutation();

	int		get_permutation_parameters(uint8_t &p0, uint16_t &p1, uint16_t &p2, uint16_t &p3);
	
	int		get_preamble_length();
  
	// Set functions //
	int		set_timeslot_id(int timeslot_id);

	int		set_symbol_rate(uint64_t symbol_rate);

	int		set_timeslot_duration(int timeslot_duration);

	int		set_burst_start_offset(int burst_start_offset);

	int		set_inner_code_type(int inner_code_type);

	int		set_inner_code_ordering(int inner_code_ordering);

	int		set_outer_coding(int outer_coding);

	int		set_inner_code_puncturing(int inner_code_puncturing);

	int		set_modulation(int modulation);

	int		set_baseband_shaping(int baseband_shaping);

	int		set_timeslot_payload_type(int timeslot_payload_type);

	int		set_route_id_flag(int route_id_flag);

	int		set_acm_flag(int acm_flag);

	int		set_sac_length(int sac_length);

	int		set_request_flag(int request_flag);

	int		set_m_and_c_flag(int m_and_c_flag);

	int		set_group_id_flag(int group_id_flag);

	int		set_logon_id_flag(int logon_id_flag);

	int		set_capacity_requests_number(int capacity_requests_number);

	int		set_new_permutation(int new_permutation);
	
	int		set_permutation_parameters(uint8_t p0, uint16_t p1, uint16_t p2, uint16_t p3);
	
	int		set_preamble_length(uint8_t preamble_length);

};

// Declare the TCT timeslot information queue. //
struct Tct_timeslot_info_circleq {
  public:
	struct Tct_timeslot_info_entry *cqh_first;		// first element //	
	struct Tct_timeslot_info_entry *cqh_last;		// last element //

	void free();
	static void copy(Tct_timeslot_info_circleq* dst, Tct_timeslot_info_circleq* src);
};
	



class Tct : public Table {
  friend class Tct_table_to_section_handler;
  friend class Tct_section_to_table_handler;

  private:
	uint16_t						_network_id;
	u_char							_version_number;
	u_char							_current_next_indicator;
	// Note: timeslot_loop_count==0 means no loop here.
	// * 		 while in the TCT section, timeslot_loop_count==0 
	// *		 means one loop.
	 //
	uint8_t						_timeslot_loop_count;
	
	Tct_timeslot_info_circleq				_tct_timeslot_info_circleq;


  public:
	// Constructors //
	Tct();

	Tct(uint16_t _network_id, u_char _version_number, 
		u_char _current_next_indicator);


	// Destructor //
	~Tct();


	// Get functions //

	uint16_t	get_network_id();

	u_char		get_version_number(); 

	u_char		get_current_next_indicator(); 

	uint32_t	get_timeslot_loop_count(); 


	// Set functions //

	int			set_network_id(uint16_t network_id); 

	int			set_version_number(u_char version_number); 

	int			set_current_next_indicator(u_char current_next_indicator);

	int			set_timeslot_loop_count(uint8_t timeslot_loop_count); 


	// Table operation functions //

	Tct*		copy();
	
	int			add_timeslot_info(Tct_timeslot_info timeslot_info, uint8_t p0,
						  uint16_t p1, uint16_t p2, uint16_t p3, 
						  uint8_t preamble_length);

	int			remove_timeslot_info(u_char timeslot_id);

	int			get_timeslot_info(Tct_timeslot_info* timeslot_info, uint8_t timeslot_id, 
						  uint8_t &p0, uint16_t &p1, uint16_t &p2, uint16_t &p3, 
						  uint8_t &preamble_length);

}; // End of class Tct.

#pragma pack()

#endif	// __NCTUNS_tct_table_h__ //
