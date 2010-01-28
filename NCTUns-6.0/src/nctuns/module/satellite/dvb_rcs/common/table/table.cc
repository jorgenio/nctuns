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

#include "table.h"
#include "pat_table.h"
#include "pmt_table.h"
#include "fct_table.h"
#include "sct_table.h"
#include "tct_table.h"
#include "int_table.h"
#include "nit_table.h"
#include "tbtp_table.h"
#include "../si_config.h"

/************************************************************
 * newer_than() determines if version ver1 is newer than
 * ver2. Here, we say ver1 is newer than ver2 iff
 * (ver1-ver2)mod 32 is smaller than 4.
 ***********************************************************/
bool 
newer_than(u_char ver1, u_char ver2)
{
	int	diff;

	diff = ((int)ver1) - ver2;
	if(diff < 0)
	{
		diff += 32;
	}

	if((diff>0) && (diff<4))
	{
		return (true);
	}
	else
	{
		return (false);
	}
}
