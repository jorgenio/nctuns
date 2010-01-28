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

#include <assert.h>
#include "tbtp_table_list.h"

void
Tbtp_table_list::free()
{
	list<Tbtp*>::iterator		tbtp_it;


	for(tbtp_it = tbtp_tables.begin();
	    tbtp_it != tbtp_tables.end();
	    tbtp_it++)
	{
		delete (*tbtp_it);
	}

	tbtp_tables.clear();
}

int
Tbtp_table_list::add_tbtp(Tbtp* ptr_tbtp)
{
	list<Tbtp*>::iterator	it;
	Tbtp*		tbtp;
	uint16_t	network_id;
	uint8_t		group_id;
	uint16_t	superframe_count;
	u_char		version_number;


	network_id = ptr_tbtp->get_network_id();
	group_id = ptr_tbtp->get_group_id();
	superframe_count = ptr_tbtp->get_superframe_count();
	version_number = ptr_tbtp->get_version_number();

	for(it = tbtp_tables.begin();
	    it != tbtp_tables.end();
	    it++)
	{
		tbtp = (*it);
		if((network_id==tbtp->get_network_id()) &&
		   (group_id==tbtp->get_group_id()) &&
		   (superframe_count==tbtp->get_superframe_count()) &&
		   (version_number==tbtp->get_version_number()))
		{
			printf("[Warning]Tbtp_table_list::add_tbtp error --- TBTP already exist\n");
			assert(0);
		}
	}

	tbtp_tables.push_back(ptr_tbtp);
	return (0);
}

Tbtp*		
Tbtp_table_list::get_tbtp(u_char ver_num, 
			  uint16_t net_id, 
			  uint8_t group_id, 
			  uint16_t superframe_count)
{
	list<Tbtp*>::iterator	it;
	Tbtp*		tbtp;


	for(it = tbtp_tables.begin();
	    it != tbtp_tables.end();
	    it++)
	{
		tbtp = (*it);
		if((net_id==tbtp->get_network_id()) &&
		   (group_id==tbtp->get_group_id()) &&
		   (superframe_count==tbtp->get_superframe_count()) &&
		   (ver_num==tbtp->get_version_number()))
		{
			return (tbtp);
		}
	}

	return (NULL);
}

