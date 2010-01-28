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
#include <satellite/dvb_rcs/common/sch_info.h>
#include <satellite/dvb_rcs/ret/queue/rcst_queue_manager.h>
#include <satellite/dvb_rcs/rcst/rcst_queue_config.h>
#include <mylist.h>

#include "rcs_atm.h"
#include "rcs_mac.h"
#include "rcs_mac_spncc.h"
#include "dvb_rcs.h"
MODULE_GENERATOR(Rcs_mac_spncc);

/*
 * Constructor
 */
Rcs_mac_spncc::Rcs_mac_spncc(u_int32_t type, u_int32_t id, struct plist * pl,
		 const char *name)
: Rcs_mac(type, id, pl, name)
{
}

/*
 * module initialize
 */
int Rcs_mac_spncc::init()
{
	_ncc_config = GET_REG_VAR1(get_portls(), "NCC_CONFIG", Ncc_config *);
	assert(_ncc_config);

	return (Rcs_mac::init());
}


/*
 * SP node should not send packet to return link, by pass to forward link
 * module
 */
int Rcs_mac_spncc::send(ePacket_ * event)
{
	return NslObject::send(event);
}


/*
 * recv the packet from the Rcst and check whether collision at the same timeslot
 */
int Rcs_mac_spncc::recv(ePacket_ * event)
{
	return Rcs_mac::recv(event);
}
