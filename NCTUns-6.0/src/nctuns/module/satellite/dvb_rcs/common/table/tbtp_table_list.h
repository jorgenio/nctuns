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

#ifndef	__NCTUNS_TBTP_TABLE_LIST_H__
#define	__NCTUNS_TBTP_TABLE_LIST_H__
#include <satellite/dvb_rcs/common/table/tbtp_table.h>
#include <list>

using std::list;

class Tbtp_table_list
{
  public:
	std::list<Tbtp*>	tbtp_tables;
	void			free();
	int			add_tbtp(Tbtp* ptr_tbtp);
	Tbtp*			get_tbtp(u_char ver_num, 
					 uint16_t net_id, 
					 uint8_t group_id, 
					 uint16_t superframe_count);		
};

#endif /* __NCTUNS_TBTP_TABLE_LIST_H__ */
