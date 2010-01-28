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

// tim_table.h

#ifndef __NCTUNS_tim_table_h__
#define __NCTUNS_tim_table_h__

#include <stdio.h>
#include "table.h"
#include <stdarg.h>
#include <memory.h>
#include "section_draft.h"
#include "../descriptor/descriptor_q.h"
#include "../descriptor/logon_initialize_descriptor.h"
#include "../descriptor/contention_control_descriptor.h"
#include <mylist.h>

#define	TIM_SUPERFRAME_INFO_SIZE	sizeof(Tim_superframe_info)
#define	TIM_FRAME_INFO_SIZE			sizeof(Tim_frame_info)
#define RCST_status			_status
#define Network_status			_status

#pragma pack(1)

class Tim : public Table {
  friend class Tim_table_to_section_handler;
  friend class Tim_section_to_table_handler;

  private:
  	u_char			_section_syntax_indicator;
	u_char			_private_indicator;
	u_char			_mac_address[6];
	u_char			_payload_scrambling_control;
	u_char			_address_scrambling_control;
	u_char			_llc_snap_flag;
	u_char			_current_next_indicator;
	u_char			_last_section_number;
	u_int8_t		_status;
	u_int8_t		_descriptor_loop_count;

	/* Note: Superframe_loop_count==0 means no loop here.
	 * 		 while in the TIM section, Superframe_loop_count==0 
	 *		 means one loop.
	 */

	Descriptor_circleq      *_tim_des_circleq;

  public:
	Tim();

	Tim(u_char _current_next_indicator);

	~Tim();

	Tim*		copy();

	/* Set functions */
	int		set_mac_address(const u_char* mac_addr)
	{
		memcpy(_mac_address, mac_addr, 6);
		return 0;
	}
	int		set_section_syntax_indicator(u_char ssi) {_section_syntax_indicator = ssi; return 0;}

	int		set_private_indicator(u_char pi) {_private_indicator = pi; return 0;}

	int		set_payload_scrambling_control(u_char psc){
			_payload_scrambling_control  = psc;
			return 0;
	}
	int		set_address_scrambling_control(u_char asc){
			_address_scrambling_control = asc;
			return 0;
	}
	int		set_llc_snap_flag(u_char lsf){
			_llc_snap_flag = lsf;
			return 0;
	}
	int		set_current_next_indicator(u_char current_next_indicator) {
			_current_next_indicator= current_next_indicator; return 0;}

	int		set_status(u_char status){ _status = status; return 0;}

	u_int32_t	set_descriptor_loop_count(int dlc) { _descriptor_loop_count= dlc; return 0;}

	/* Get functions */
	int		get_mac_address(u_char *mac_addr_buf)
	{
		memcpy(mac_addr_buf, _mac_address, 6);
		return 0;
	}

	u_char		get_section_syntax_indicator() {return _section_syntax_indicator;} 
	u_char		get_private_indicator() {return _private_indicator;}
	u_char		get_payload_scrambling_control(){return _payload_scrambling_control;}
	u_char		get_address_scrambling_control(){return _address_scrambling_control;}
	u_char		get_llc_snap_flag(){return _llc_snap_flag;}
	u_char		get_current_next_indicator() {return _current_next_indicator;}
	u_char		get_status(){return _status;}
	u_int32_t	get_descriptor_loop_count() {return _descriptor_loop_count;}



	/* Table operation functions */

        // descriptor operations
        int                     add_descriptor(Descriptor *dct);

        Descriptor*             get_descriptor(int pnum , uint8_t searched_des , ...);

        int                     remove_descriptor(int pnum , uint8_t deleted_des , ...);

}; // End of class Tim.
#pragma pack()
#endif /* __NCTUNS_tim_table_h__ */
