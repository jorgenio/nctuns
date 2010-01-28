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
#include <object.h>
#include <math.h>
#include "delay.h"

uint64_t
dvb_rcs_prop_delay(class NslObject* obj_a,
	    class NslObject* obj_b)
{
	double		loc_x1;
	double		loc_y1;
	double		loc_z1;
	double		loc_x2;
	double		loc_y2;
	double		loc_z2;
	double		df;
	uint64_t	prop_delay;

	assert(obj_a && obj_b);
	/*
	 * Get locations. 
	 */
	assert(GetNodeLoc(obj_a->get_nid(), loc_x1, loc_y1, loc_z1) > 0);
	assert(GetNodeLoc(obj_b->get_nid(), loc_x2, loc_y2, loc_z2) > 0);
	/*
	 * Calculate the propagation delay.
	 */
	df = sqrt((loc_x1 - loc_x2) * (loc_x1 - loc_x2) +
		  (loc_y1 - loc_y2) * (loc_y1 - loc_y2) +
		  (loc_z1 - loc_z2) * (loc_z1 - loc_z2));

	df = (df / SPEED_OF_LIGHT) * 1000000;	/* us */
	/*
	 * Unit transfer: us to ticks
	 */
	MICRO_TO_TICK(prop_delay, df);

	return prop_delay;
}

uint64_t
dvb_rcs_prop_delay(uint32_t node_id_1,
	    uint32_t node_id_2)
{
	double		loc_x1;
	double		loc_y1;
	double		loc_z1;
	double		loc_x2;
	double		loc_y2;
	double		loc_z2;
	double		df;
	uint64_t	prop_delay;

	/*
	 * Get locations. 
	 */
	if(GetNodeLoc(node_id_1, loc_x1, loc_y1, loc_z1) <= 0)
	{
		printf("[Error] %s --- No node %u in %s.sce\n",
			__FUNCTION__, node_id_1, GetScriptName());
		assert(0);
	}

	if(GetNodeLoc(node_id_2, loc_x2, loc_y2, loc_z2) <= 0)
	{
		printf("[Error] %s --- No node %u in %s.sce\n",
			__FUNCTION__, node_id_2, GetScriptName());
		assert(0);
	}

	/*
	 * Calculate the propagation delay.
	 */
	df = sqrt((loc_x1 - loc_x2) * (loc_x1 - loc_x2) +
		  (loc_y1 - loc_y2) * (loc_y1 - loc_y2) +
		  (loc_z1 - loc_z2) * (loc_z1 - loc_z2));

	df = (df / SPEED_OF_LIGHT) * 1000000;	/* us */
	/*
	 * Unit transfer: us to ticks
	 */
	MICRO_TO_TICK(prop_delay, df);

	return prop_delay;
}
