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
#include "rcs_mac_rcst.h"
#include "dvb_rcs.h"

MODULE_GENERATOR(Rcs_mac_rcst);

/*
 * Constructor
 */
Rcs_mac_rcst::Rcs_mac_rcst(u_int32_t type, u_int32_t id, struct plist * pl,
		 const char *name)
: Rcs_mac(type, id, pl, name)
{
	/*
	 * Parameters from tcl
	 */
	vBind_mac("mac", _mac);

	/*
	 * register variable
	 */
	REG_VAR("MAC", &_mac);

	/*
	 * REG_VAR queue manager object pointer, this pointer will be used by rcst_ctl to
	 * operate queue manager when RCST logout or other state
	 */
	REG_VAR("QUEUE_QM_PTR", &_rcst_qm);
	REG_VAR("RCS_RCST_MAC_PTR", this);
}


/*
 * Destructor
 */
Rcs_mac_rcst::~Rcs_mac_rcst()
{
	if (_rcst_qm)
		delete _rcst_qm;
}


/*
 * module initialize
 */
int Rcs_mac_rcst::init()
{
	/*
	 * get rcst ctrl module pointer from rcst_ctl.cc to IPC some
	 * timeslot allocation message
	 */
	_rcst_obj = GET_REG_VAR1(get_portls(), "RCST_PTR", Rcst_ctl *); 
	_rcst_config = GET_REG_VAR1(get_portls(), "RCST_CONFIG_PTR", Rcst_queue_config *); 
	_ncc_config = GET_REG_VAR1(get_portls(), "NCC_CONFIG", Ncc_config *); 

	const uint32_t *const timeout_in_superframe = GET_REG_VAR1(get_portls(), 
								   "TIMEOUT_IN_SUPERFRAME", 
								   uint32_t*); 

	assert(_rcst_obj && _rcst_config && _ncc_config && timeout_in_superframe);

	const uint32_t timeout_in_frame = (*timeout_in_superframe) * _ncc_config->num_of_frame_per_superframe;
	/*
	 * create queue manager
	 */
	assert((_rcst_qm = new Rcst_queue_manager(_rcst_config, _rcst_obj, _ncc_config, timeout_in_frame)));

	return (Rcs_mac::init());
}


/*
 * segmentation payload depend on AAL5 spec and padding into ATM cell upper
 * layer must send Dvb_pkt packet struct to me, then send those cell to bottom
 * layer.
 */
int Rcs_mac_rcst::send(ePacket_ * event)
{
	return Rcs_mac::send(event);
}


/*
 * RCST node should not receive packet from return link, by pass to control
 * module
 */
int Rcs_mac_rcst::recv(ePacket_ * event)
{
	return NslObject::recv(event);
}
