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

#include "rcst_info.h"


bool
Rcst_id::operator==(const Rcst_id& id) const
{
	if( (group_id==id.group_id) && (logon_id==id.logon_id) )
		return true;
	else
		return false;
}


int	
Rcst_info_list::get_rcst_id(Rcst_id &rcst_id, uint32_t node_id)
{
	list<Rcst_info>::iterator	it;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if (it->node_id==node_id) //found.
		{
			rcst_id = it->rcst_id;
			return 0;
		}
	}

	//No match.
	return -1;
}

int
Rcst_info_list::get_cra_level(uint32_t& cra_level, Rcst_id rcst_id)
{
	list<Rcst_info>::iterator	it;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if((it->rcst_id)==rcst_id)
		{
			cra_level = it->cra_level;
			return 0;
		}
	}

	//No match.
	return -1;
}

int
Rcst_info_list::get_rbdc_max(uint32_t& rbdc_max, Rcst_id rcst_id)
{
	list<Rcst_info>::iterator	it;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if(it->rcst_id==rcst_id)
		{
			rbdc_max = it->rbdc_max;
			return 0;
		}
	}

	//No match.
	return -1;
}

int
Rcst_info_list::get_rbdc_timeout(uint16_t& rbdc_timeout, Rcst_id rcst_id)
{
	list<Rcst_info>::iterator	it;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if(it->rcst_id==rcst_id)
		{
			rbdc_timeout = it->rbdc_timeout;
			return 0;
		}
	}

	//No match.
	return -1;
}

int
Rcst_info_list::get_vbdc_max(uint16_t& vbdc_max, Rcst_id rcst_id)
{
	list<Rcst_info>::iterator	it;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if(it->rcst_id==rcst_id)
		{
			vbdc_max = it->vbdc_max;
			return 0;
		}
	}

	//No match.
	return -1;
}

int
Rcst_info_list::get_superframe_id_for_transmission(uint8_t& sid, Rcst_id rcst_id)
{
	list<Rcst_info>::iterator	it;
	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if(rcst_id==it->rcst_id)
		{
			sid = it->superframe_id_for_transmission;
			return 0;
		}
	}
	
	//No match.
	return -1;
}


int
Rcst_info_list::look_up_rcst_id_with_mac(Rcst_id& rcst_id, u_char *mac)
{
	list<Rcst_info>::iterator	it;
	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if(!memcmp(mac, it->rcst_mac, 6))
		{
			rcst_id = it->rcst_id;
			return 0;
		}
	}
	
	//No match.
	return -1;
}

int
Rcst_info_list::look_up_logon_id_with_mac(uint16_t& logon_id, u_char *mac)
{
	Rcst_id		rcst_id;
	int		result;

	result = look_up_rcst_id_with_mac(rcst_id, mac);
	logon_id = rcst_id.logon_id;
	return result;
}


int
Rcst_info_list::look_up_group_id_with_mac(uint8_t& group_id, u_char *mac)
{
	Rcst_id		rcst_id;
	int		result;

	result = look_up_rcst_id_with_mac(rcst_id, mac);
	group_id = rcst_id.group_id;
	return result;
}

bool
Rcst_info_list::no_rcst(uint16_t superframe_id)
{
	return ( rcst_amount(superframe_id)==0 );
}

uint32_t
Rcst_info_list::rcst_amount(uint16_t superframe_id)
{
	uint32_t	counter;
	list<Rcst_info>::iterator	it;

	counter = 0;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if((it->superframe_id_for_transmission)==superframe_id)
		{
			counter++;
		}
	}

	//No match.
	return (counter);
}

uint32_t
Rcst_info_list::active_rcst_amount(uint16_t superframe_id)
{
	uint32_t	counter;
	list<Rcst_info>::iterator	it;

	counter = 0;

	for(it=info_list.begin(); it!=info_list.end(); it++)
	{
		if((it->superframe_id_for_transmission)==superframe_id &&
		   (it->rcst_state==RCST_STATE_FINE_SYNC))
		{
			counter++;
		}
	}

	//No match.
	return (counter);
}
