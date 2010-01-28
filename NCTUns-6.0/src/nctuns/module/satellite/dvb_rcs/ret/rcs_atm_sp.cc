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

#include <stdlib.h>
#include <assert.h>
#include <regcom.h>
#include <object.h>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/fec/crc.h>
#include <satellite/dvb_rcs/common/sch_info.h>

#include "rcs_atm.h"
#include "rcs_atm_sp.h"

MODULE_GENERATOR(Rcs_atm_sp);

/*
 * Constructor
 */
Rcs_atm_sp::Rcs_atm_sp(u_int32_t type, u_int32_t id, struct plist *pl,
		 const char *name)
:Rcs_atm(type, id, pl, name)
{
}


/*
 * module initialize
 */
int Rcs_atm_sp::init()
{
	return (Rcs_atm::init());
}


/*
 * SP node should not send packet to return link, by pass to forward link
 * module
 */
int Rcs_atm_sp::send(ePacket_ * event)
{
	return NslObject::send(event);
}


/*
 * re-assembly atm cell to Dvb_pkt packet and send to upper layer 
 */
int Rcs_atm_sp::recv(ePacket_ * event)
{
	/*
	 * implementation the recv procedure in Rcs_atm class
	 */
	return Rcs_atm::recv(event);
}
