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

#ifndef __NCTUNS_section_draft_h__
#define __NCTUNS_section_draft_h__

#define		SECTION_DEBUG

#include <stdio.h>
#include <sys/types.h>
#include <stdint.h>
#include "../si_config.h"
#include "../fec/crc.h"
#pragma pack(1)

/* Macro definition about section size */
#define	SECTION_DRAFT_SIZE				3
#define	CRC32_SIZE						4
#define	CHECKSUM_SIZE					4
#define	SCT_SECTION_DRAFT_SIZE			sizeof(Sct_section_draft)
#define	PAT_SECTION_DRAFT_SIZE			sizeof(Pat_section_draft)
#define	FCT_SECTION_DRAFT_SIZE			sizeof(Fct_section_draft)
#define	TCT_SECTION_DRAFT_SIZE			sizeof(Tct_section_draft)
#define	TBTP_SECTION_DRAFT_SIZE			sizeof(Tbtp_section_draft)
#define	PMT_SECTION_DRAFT_SIZE			sizeof(Pmt_section_draft)
#define	TIM_SECTION_DRAFT_SIZE			sizeof(Tim_section_draft)

u_int16_t	section_total_length(void* section);
u_int8_t	table_id(void* section);
u_int8_t	version_number(void* section);
CUR_NEXT_INDICATOR current_next_indicator(void* section);
bool		crc_okay(void* section);
void		fill_crc(void* section);
const uint8_t MAX_VERSION	= 32;

#pragma pack(1)
class Section_draft {
  protected:
	u_char			_table_id;
	u_int16_t		_section_syntax_indicator : 1;
	u_int16_t		_private_indicator : 1;
	u_int16_t		_reserved_1 : 2;		// The number is used for distincting different variables.
	u_int16_t		_section_length : 12;


  public:
	int				set_section_draft(u_int16_t	table_id,
			u_int16_t section_syntax_indicator, u_int16_t private_indicator,
			u_int16_t section_length);  

	/* Get functions */

	/* The function get_section_length will return the value of the
	 * section_length field in the section.
	 */
	u_int16_t	get_section_length();

	/* The function get_section_length will compute the value of the
	 * total length of the section.
	 */
	u_int16_t	get_section_total_length();
	u_char		get_section_syntax_indicator(){return _section_syntax_indicator;}
	u_char		get_private_indicator(){return _private_indicator;}

}__attribute__((packed)); // End of class Section_draft. 


class Psi_section_draft : public Section_draft {
  protected:
	union{
		u_int16_t		_transport_stream_id;
		u_int16_t		_program_number;
	};
	u_char			_reserved_2 : 2;		// The number is used for distincting different variables.
	u_char			_version_number : 5;
	u_char			_current_next_indicator : 1;
	u_char			_section_number;
	u_char			_last_section_number;

  public:

	/* Get functions */
	u_int16_t		get_transport_stream_id() {return _transport_stream_id;}

	u_int16_t		get_program_number() {return _program_number;}

	u_char			get_version_number() {return _version_number;}

	u_char			get_current_next_indicator() {return _current_next_indicator;}

	u_char			get_section_number() {return _section_number;}

	u_char			get_last_section_number() {return _last_section_number;}


	/* Set functions */
	void			set_transport_stream_id(u_char transport_stream_id) { _transport_stream_id = transport_stream_id; }

	void			set_program_number(u_char program_number) { _program_number = program_number; }

	void			set_version_number(u_char version_number) { _version_number = version_number; }

	void			set_current_next_indicator(u_char current_next_indicator) { _current_next_indicator = current_next_indicator; }

	void			set_section_number(u_char section_number) { _section_number = section_number; }

	void			set_last_section_number(u_char last_section_number) { _last_section_number = last_section_number; }

	void			set_psi_section_draft(u_int16_t tid_or_pnum,
						u_char version_number, u_char current_next_indicator,
						u_char section_number, u_char last_section_number);
};

class Si_type_a_section_draft : public Section_draft { 
	friend class  Spt_section_to_table_handler;
  protected:
	u_int16_t		_network_id;
	u_char			_reserved_2 : 2;		// The number is used for distincting different variables.
	u_char			_version_number : 5;
	u_char			_current_next_indicator : 1;
	u_char			_section_number;
	u_char			_last_section_number;

  public:

	/* Get functions */
	u_int16_t		get_network_id() {return _network_id;}

	u_char			get_version_number() {return _version_number;}

	u_char			get_current_next_indicator() {return _current_next_indicator;}

	u_char			get_section_number() {return _section_number;}

	u_char			get_last_section_number() {return _last_section_number;}

	/* Set functions */
	void			set_network_id(u_char network_id) { _network_id = network_id; }

	void			set_version_number(u_char version_number) { _version_number = version_number; }

	void			set_current_next_indicator(u_char current_next_indicator) { _current_next_indicator = current_next_indicator; }

	void			set_section_number(u_char section_number) { _section_number = section_number; }

	void			set_last_section_number(u_char last_section_number) { _last_section_number = last_section_number; }

  	int				set_si_type_a_section_draft(
		u_int16_t		network_id,
		u_char			version_number,
		u_char			current_next_indicator,
		u_char			section_number,
		u_char			last_section_number		); 


};

class Pat_section_draft : public Psi_section_draft {
};

class Dsm_cc_section_draft : public Section_draft { 
	friend class Dgm_section_to_ip_datagram_handler; 
	friend class Ip_datagram_to_dgm_section_handler; 
  protected:
	u_char			_mac_address_6;
	u_char			_mac_address_5;
	u_char			_reserved_2 : 2;		// The number is used for distincting different variables.
	u_char			_payload_scrambling_control : 2;
	u_char			_address_scrambling_control : 2;
	u_char			_llc_snap_flag : 1;
	u_char			_current_next_indicator : 1;
	u_char			_section_number;
	u_char			_last_section_number;
	u_char			_mac_address_4;
	u_char			_mac_address_3;
	u_char			_mac_address_2;
	u_char			_mac_address_1;
  public:
	int	set_mac_address(const u_char *mac_addr)
	{
		_mac_address_1 = mac_addr[5];
		_mac_address_2 = mac_addr[4];
		_mac_address_3 = mac_addr[3];
		_mac_address_4 = mac_addr[2];
		_mac_address_5 = mac_addr[1];
		_mac_address_6 = mac_addr[0];
		return 0;
	}
	int	set_payload_scrambling_control(u_char psc){
		_payload_scrambling_control  = psc;
		return 0;
	}
	int	set_address_scrambling_control(u_char asc){
		_address_scrambling_control = asc;
		return 0;
	}
	int	set_llc_snap_flag(u_char lsf){
		_llc_snap_flag = lsf;
		return 0;
	}
	int	set_current_next_indicator(u_char cni){
		_current_next_indicator = cni;
		return 0;
	}
	int	set_section_number(u_char sn){
		_section_number = sn;
		return 0;
	}
	int	set_last_section_number(u_char lsn){
		_last_section_number = lsn;
		return 0;
	}
	int	set_dsm_cc_section_draft(u_char psc, u_char asc, u_char lsf, u_char cni, u_char sn, u_char lsn) {

		_payload_scrambling_control  = psc;
		_address_scrambling_control = asc;
		_llc_snap_flag = lsf;
		_current_next_indicator = cni;
		_section_number = sn;
		_last_section_number = lsn;
		return 0;
	}
	/* get function */
	int get_mac_address(u_char *mac_addr)
	{
		mac_addr[5] = _mac_address_1; 
		mac_addr[4] = _mac_address_2; 
		mac_addr[3] = _mac_address_3; 
		mac_addr[2] = _mac_address_4; 
		mac_addr[1] = _mac_address_5; 
		mac_addr[0] = _mac_address_6; 
		return 0;
	}
	u_char	get_payload_scrambling_control(){
		return _payload_scrambling_control;
	}
	u_char	get_address_scrambling_control(){
		return _address_scrambling_control;
	}
	u_char	get_llc_snap_flag(){
		return _llc_snap_flag;
	}
	u_char	get_current_next_indicator(){
		return _current_next_indicator;
	}
	u_char	get_section_number(){
		return _section_number;
	}
	u_char	get_last_section_number(){
		return _last_section_number;
	}

};

class Pmt_section_draft : public Psi_section_draft {
	u_int16_t		_reserved1 : 3;
	u_int16_t		_pcr_pid	: 13;
	u_int16_t		_reserved : 4;
	u_int16_t		_program_info_length : 12;
  public:
  	u_int16_t		get_pcr_pid(){ return _pcr_pid;}
	u_int16_t		get_program_info_length(){ return _program_info_length;}
	
	int			set_pcr_pid(int pcr_pid){ _pcr_pid = pcr_pid;return 1;}
	int			set_program_info_length(int pil){ _program_info_length = pil;return 1;}
};

class Dgm_section_draft : public Dsm_cc_section_draft {
};

class Nit_section_draft : public Si_type_a_section_draft {
	/* Note: In the NIT section, the fields defined here are not continuous. */
};



class Spt_section_draft : public Si_type_a_section_draft {
	u_char					_satellite_loop_count;
  public:
	int			set_satellite_loop_count(u_char satellite_loop_count) {
					_satellite_loop_count = satellite_loop_count;
					return 0;
				}
	u_char 		get_satellite_loop_count() {return _satellite_loop_count;}
};



class Sct_section_draft : public Si_type_a_section_draft {
	friend class Sct_section_to_table_handler;
	friend class Sct_table_to_section_handler;

	u_char					_superframe_loop_count;
  public:
	int			set_superframe_loop_count(u_char superframe_loop_count) {
					_superframe_loop_count = superframe_loop_count;
					return 0;
				}
	u_char 		get_superframe_loop_count() {return _superframe_loop_count;}
};



class Fct_section_draft : public Si_type_a_section_draft {
	u_char					_frame_id_loop_count;
  public:
	int			set_frame_id_loop_count(u_char frame_id_loop_count) {
					_frame_id_loop_count = frame_id_loop_count;
					return 0;
				}
	u_char 		get_frame_id_loop_count() {return _frame_id_loop_count;}
};

class Tct_section_draft : public Si_type_a_section_draft {
	u_char					_timeslot_loop_count;
  public:
	int			set_timeslot_loop_count(u_char timeslot_loop_count) {
					_timeslot_loop_count = timeslot_loop_count;
					return 0;
				}
	u_char 		get_timeslot_loop_count() {return _timeslot_loop_count;}
};

class Tbtp_section_draft : public Si_type_a_section_draft {
	u_char					_group_id;
	u_int16_t				_superframe_count;
	u_char					_frame_loop_count;
  public:
	int		set_group_id(u_int8_t group_id)
			{ _group_id = group_id; return 0;}
	int		set_superframe_count(u_int16_t superframe_count)
			{ _superframe_count = superframe_count; return 0;}
	int		set_frame_loop_count(u_char frmae_loop_count)
			{_frame_loop_count = frmae_loop_count;return 0;}
				
	u_int8_t	get_group_id()
			{ return _group_id;}
	u_int16_t	get_superframe_count()
			{ return _superframe_count;}
	u_char 		get_frame_loop_count()
			{ return _frame_loop_count;}

};

class Tim_section_draft : public Dsm_cc_section_draft {
	u_int8_t				_status;
	u_int8_t				_descriptor_loop_count;
  public:
  	int		set_status(u_int8_t status)
			{ _status = status; return 0;}
	int		set_descriptor_loop_count(u_int8_t dlc)
			{ _descriptor_loop_count = dlc; return 0; }

	u_int8_t	get_status()
			{ return _status; }
	u_int8_t	get_descriptor_loop_count()
			{ return _descriptor_loop_count; }
};
#pragma pack()

#pragma pack()

#endif /* __NCTUNS_section_draft_h__ */
