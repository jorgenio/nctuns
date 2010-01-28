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

#ifndef	__NCTUNS_rcst_info_h__
#define	__NCTUNS_rcst_info_h__

#include <satellite/dvb_rcs/rcst/rcst_ctl.h>
#include <stdint.h>
#include <list>

class Rcst_id
{
  public:
	uint8_t		group_id;
	uint16_t	logon_id;

	Rcst_id(){}

	Rcst_id(uint8_t g_id, uint16_t l_id) :
		group_id(g_id), logon_id(l_id){}
	
	bool operator==(const Rcst_id& id) const;
};


class Rcst_info
{
  public:
	uint32_t		node_id;
	Rcst_id			rcst_id;
	Rcst_state		rcst_state;
	u_char			rcst_mac[6];
	uint8_t			superframe_id_for_transmission;
	uint32_t		cra_level; // Unit--> bits/s.
	uint16_t		vbdc_max; // Unit--> timeslots/frame.
	uint32_t		vbdc_max_rate;// Unit--> bits/s
	uint32_t		rbdc_max; // Unit--> bits/s.
	uint16_t		rbdc_timeout; // Unit--> superframes.
	uint16_t		pid;
	list <u_long>		hosts_ipaddr;
	int			host_num;
};

class Rcst_info_list
{
  public:
	list <Rcst_info>	info_list;

	/* Searching functions below return 0 if success,
	 * otherwise, return -1.
	 */
	int	get_rcst_id(Rcst_id & rcst_id, uint32_t node_id);
	int	get_cra_level(uint32_t& cra_level, Rcst_id rcst_id);
	int	get_rbdc_max(uint32_t& rbdc_max, Rcst_id rcst_id);
	int	get_rbdc_timeout(uint16_t& rbdc_timeout, Rcst_id rcst_id);
	int	get_vbdc_max(uint16_t& vbdc_max, Rcst_id rcst_id);
	int	get_superframe_id_for_transmission(uint8_t& sid, Rcst_id rcst_id);
	int	look_up_rcst_id_with_mac(Rcst_id& rcst_id, u_char *mac);
	int	look_up_logon_id_with_mac(uint16_t& logon_id, u_char *mac);
	int	look_up_group_id_with_mac(uint8_t& group_id, u_char *mac);

	/************************************************************
	 * no_rcst() determine if there exists rcst in 'superframe_id'
	 ***********************************************************/
	bool	no_rcst(uint16_t superframe_id);

	/************************************************************
	 * rcst_amount() computes how many rcsts are in 'superframe_id'
	 ***********************************************************/
	uint32_t	rcst_amount(uint16_t superframe_id);
	
	/************************************************************
	 * active_rcst_amount() computes how many active rcsts 
	 * are in 'superframe_id'
	 ***********************************************************/
	uint32_t	active_rcst_amount(uint16_t superframe_id);
};

#endif /* __NCTUNS_rcst_info_h__ */
