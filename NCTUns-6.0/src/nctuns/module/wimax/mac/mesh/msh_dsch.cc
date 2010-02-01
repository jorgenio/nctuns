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

#include <stdio.h>
#include <string.h>
#include <nctuns_api.h>

#include "mac802_16_mesh_mode_sys_param.h"
#include "mac802_16_meshss.h"
#include "mesh_connection.h"
#include "msh_dsch.h"
#include "neighbor.h"
#include "net_entry_mngr.h"
#include "sch/data/alloc_base.h"
#include "sch/ctrl/info.h"
#include "sch/ctrl/scheduler.h"
#include "sch/data/scheduler.h"

#define VERBOSE_LEVEL	MSG_INFO
#include "../verbose.h"


MSH_DSCH::MSH_DSCH()
: ManagementMessage(CTLMSG_TYPE_MSH_DSCH)
{
	//Type = 41;
	f = new MSH_DSCH_field;
	bzero(f, sizeof(MSH_DSCH_field));
	Scheduling.f = new DSCH_SchedulingIE_field;
	Scheduling.entry = new DSCH_SchedEntry[256];
	Availability = new DSCH_AvailabilityIE[NF_AVAILABILITY_IE_MAX];
	Request = new DSCH_RequestIE[NF_REQUEST_IE_MAX];
	Grant = new DSCH_GrantIE[NF_GRANT_IE_MAX];
}

MSH_DSCH::MSH_DSCH(u_char* pBuffer, int pLength)
: ManagementMessage(pBuffer)
{
	int pos = 1 + sizeof(MSH_DSCH_field);
	Request = NULL;
	Availability = NULL;
	Grant = NULL;

	f = reinterpret_cast < MSH_DSCH_field * >(pBuffer + 1);

	if (f->CoordFlag == 0) {

		Scheduling.f = reinterpret_cast<struct DSCH_SchedulingIE_field *> (pBuffer + pos);

		pos += sizeof(struct DSCH_SchedulingIE_field);

		Scheduling.entry = reinterpret_cast<struct DSCH_SchedEntry *> (pBuffer + pos);

		pos += Scheduling.f->NSchedEntry * sizeof(struct DSCH_SchedEntry);
	}
	if (f->NRequest) {

		Request = reinterpret_cast <struct DSCH_RequestIE *>(pBuffer + pos);
		pos += f->NRequest * sizeof(struct DSCH_RequestIE);
	}
	if (f->NAvail) {

		Availability = reinterpret_cast <struct DSCH_AvailabilityIE *>(pBuffer + pos);
		pos += f->NAvail * sizeof(struct DSCH_AvailabilityIE);
	}
	if (f->NGrant) {

		Grant = reinterpret_cast <struct DSCH_GrantIE *>(pBuffer + pos);
		pos += f->NGrant * sizeof(struct DSCH_GrantIE);
	}
}

MSH_DSCH::~MSH_DSCH()
{
	if (flag) {

		delete f;
		delete Scheduling.f;
		delete[] Scheduling.entry;
		delete[] Request;
		delete[] Availability;
		delete[] Grant;

	}
}

int
MSH_DSCH::copyTo(u_char* dstbuf) const
{
	int pos = 0;
	memcpy(dstbuf, &Type, 1);
	memcpy(dstbuf + 1, f, sizeof(MSH_DSCH_field));
	//printf(" Type = %d  dstBuf[0]=%d\n", Type, dstbuf[0]&0xff);
	pos += 1 + sizeof(MSH_DSCH_field);

	if (f->CoordFlag == 0) {

		memcpy(dstbuf + pos, Scheduling.f,
				sizeof(struct DSCH_SchedulingIE_field));

		pos += sizeof(struct DSCH_SchedulingIE_field);

		memcpy(dstbuf + pos, Scheduling.entry,
				Scheduling.f->NSchedEntry *
				sizeof(struct DSCH_SchedEntry));

		pos += Scheduling.f->NSchedEntry * sizeof(struct DSCH_SchedEntry);
	}

	if (f->NRequest) {

		memcpy(dstbuf + pos, Request, f->NRequest * sizeof(struct DSCH_RequestIE));
		pos += f->NRequest * sizeof(struct DSCH_RequestIE);

	}

	if (f->NAvail) {
		memcpy(dstbuf + pos, Availability, f->NAvail * sizeof(struct DSCH_AvailabilityIE));
		pos += f->NAvail * sizeof(struct DSCH_AvailabilityIE);
	}

	if (f->NGrant) {

		memcpy(dstbuf + pos, Grant, f->NGrant * sizeof(struct DSCH_GrantIE));
		pos += f->NGrant * sizeof(struct DSCH_GrantIE);

		// printf("baa %d %d %d %d %d %d\n", Grant[0].LinkID, Grant[0].StartNumber, Grant[0].SlotStart, Grant[0].SlotRange, Grant[0].Direction, Grant[0].Persistence);
	}

	/*for(int i=0;i<pos;i++)
	  printf("%02x ", dstbuf[i]);
	  printf("COPYTO\n");
	  */
	return pos;     // Total Length
}

int
MSH_DSCH::getLen() const
{
	int len = sizeof(struct MSH_DSCH_field) + 1;

	if (f->CoordFlag == 0) {
		len += sizeof(struct DSCH_SchedulingIE_field);
		len +=
			Scheduling.f->NSchedEntry *
			sizeof(struct DSCH_SchedEntry);
	}

	if (f->NRequest) {
		len += f->NRequest * sizeof(struct DSCH_RequestIE);
	}

	if (f->NAvail) {
		len += f->NAvail * sizeof(struct DSCH_AvailabilityIE);
	}

	if (f->NGrant) {
		len += f->NGrant * sizeof(struct DSCH_GrantIE);
	}

	return len;
}

int
MSH_DSCH::addSchedParam(u_int32_t pCurrXmtTime, u_int32_t pNextXmtTime,
		u_char pHoldoff, u_int32_t maxXmtOpps)
{
	int interval, ref1, ref2;

	interval = (1 << pHoldoff);
	ref1 = ((pCurrXmtTime - 1) / interval) * interval + 1;
	ref2 = ((pNextXmtTime - 1) / interval) * interval + 1;

	//printf("setSchedParameter: pCurrXmtTime=%d, pNextXmtTime=%d, ref1=%d, ref2=%d\n",
	//      pCurrXmtTime, pNextXmtTime, ref1, ref2);

	if (ref2 < ref1) {
		// Wrapping
		ref2 += maxXmtOpps;
	}

	if ((ref2 - ref1) / interval > 31) {
		Scheduling.f->NextXmtMx = 15;
		Scheduling.f->XmtHoldExp = pHoldoff;
	} else {
		Scheduling.f->NextXmtMx = (ref2 - ref1) / interval;
		Scheduling.f->XmtHoldExp = pHoldoff;
	}

	f->CoordFlag = 0;   // Coordinated distributed scheduling
	Scheduling.f->NSchedEntry = 0;

	return 0;
}

int
MSH_DSCH::add_nbr_sch_entry(Ctrl_sch::Entry& entry)
{
	int i = Scheduling.f->NSchedEntry;

	if (i >= 255) {
		return -1;
	}

	Scheduling.entry[i].msbNbrNodeID   = entry.node_id() >> 8;
	Scheduling.entry[i].lsbNbrNodeID   = entry.node_id() & 0xFF;

	Scheduling.entry[i].NextXmtMx      = entry.next_tx_mx();
	Scheduling.entry[i].XmtHoldExp     = entry.tx_holdoff_exp();

	return Scheduling.f->NSchedEntry++;
}

int
MSH_DSCH::add_request_info(
		Data_scheduler& scheduler,
		uint16_t cur_frame,
		uint32_t cur_tx_time,
		uint32_t& num_of_requested_data_schedules)
{
	const Mesh_link_tx::hash_t& tx_links = scheduler.mac()->net_entry_mngr()->tx_links();
	scheduler.update_pseudo_schedule_index(tx_links.size());

	uint32_t cnt = 0;
	for (Mesh_link_tx::hash_t::const_iterator it = tx_links.begin();
			it != tx_links.end(); it++) {

		if (getNumRequest() >= NF_REQUEST_IE_MAX) {

			VINFO("%11.7f: [%03u] %s: Defer sending remaining request IEs because"
						"the number of packed request IEs has reached"
						"the maximum number for a DSCH."
						"(cur_frame = %#x)\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__, cur_frame);
			break;
		}



		const Mesh_link_tx& tx_link = *(it->second);

		bool link_pseudo_sched_flag = false;
		if (!scheduler.is_pseudo_schedule_skip(
					tx_link.pending_data_len(), ++cnt)) {
			link_pseudo_sched_flag = true;
		}

		Mesh_connection* connection_mac_management;

		assert(connection_mac_management =
				tx_link.connection(
					Mesh_connection::gen_id(tx_link.id(),
					Mesh_connection::TYPE_MAC_MANAGEMENT,
					Mesh_connection::RELIABILITY_NO_RE_XMIT,
					Mesh_connection::CLASS_GENERAL, 0)));

		Mesh_connection* connection_ip;

		assert(connection_ip =
				tx_link.connection(
					Mesh_connection::gen_id(tx_link.id(),
					Mesh_connection::TYPE_IP,
					Mesh_connection::RELIABILITY_NO_RE_XMIT,
					Mesh_connection::CLASS_GENERAL, 0)));

		Mesh_connection* connection_selected;
		if (connection_mac_management->pending_data_len())
			connection_selected = connection_mac_management;

		else if (connection_ip->pending_data_len() ||
				link_pseudo_sched_flag ) {

			connection_selected = connection_ip;

		}
		else
			continue;

		/*
		 * So far, for each connection the scheduler is able to send
		 * a request IE at a time.
		 */

		if (!_add_request_info(scheduler, connection_selected, cur_frame, cur_tx_time))
			continue;
		else {

#if defined __SEC_PHASE_REQ_PREDICTION__ && __SEC_PHASE_REQ_PREDICTION__
			connection_selected->set_in_advance_requesting_flag(false);
#endif

		}

		++num_of_requested_data_schedules;

	}

	return 0;
}

bool
MSH_DSCH::_add_request_info(
		Data_scheduler& scheduler,
		const Mesh_connection* connection,
		uint16_t cur_frame, uint32_t cur_tx_time)
{
	/*
	 * Get the allocation of availability from the scheduler.
	 */

	schedule_alloc_t avail;

	if (!scheduler.get_availability(avail, connection, cur_frame))
		return false;

	/*
	 * The frame range of the availability should be adjusted to
	 * conform to the corresponding persistence.
	 */

	_adjust_frame_range(avail);

	/*
	 * Register the allocation with the scheduler and
	 * set the peer node of the allocation of availability.
	 */

	scheduler.alloc_register(avail,
			Data_scheduler::SENT_AVAILABILITY,
			connection->node_dst());
	/*
	 * Get the allocation of request from the scheduler
	 * via the availability information.
	 */

	schedule_alloc_t request;
	u_int32_t alloc_validity   = 128; /* FIXME: just try */
	u_int32_t alloc_slot_range = 0;

	scheduler.get_request(request, connection,
			avail.slot_range(), alloc_validity, alloc_slot_range);

	/*
	 * Register the allocation with the scheduler and
	 * set the peer node of the allocation of request.
	 * The information of the registered requests is
	 * useless under current implementation.
	 */

	scheduler.alloc_register(request,
			Data_scheduler::SENT_REQUEST,
			connection->node_dst(), connection->id());

	/*
	 * Add availability and request IEs.
	 * XXX: Assumption here:
	 *      The availability with index i implies
	 *      the availability range of the request with index i.
	 */

	assert(_encapsulate_availability(avail));
	assert(_encapsulate_request(request));

	VINFO("%11.7f: [%03u] %s: Add an availability: (cur_frame = %#x)\n",
				(double)GetCurrentTime() / 10000000,
				scheduler.mac()->node_id(),
				__FUNCTION__, cur_frame);

	VINFO_FUNC(avail.dump());

	VINFO("%11.7f: [%03u] %s: Add a request: (cur_frame = %#x, txopp: %u)\n",
				(double)GetCurrentTime() / 10000000,
				scheduler.mac()->node_id(),
				__FUNCTION__,
				cur_frame, cur_tx_time);

	VINFO_FUNC(request.dump());

	scheduler.record_request_time(avail, cur_tx_time);

	VINFO("[%03u] %s: Add a request: (cur_frame: %#x, txopp: %u)\n",
			scheduler.mac()->node_id(), __func__,
			cur_frame, cur_tx_time);

	return true;
}

void
MSH_DSCH::add_grant_info(class Data_scheduler& scheduler, uint16_t cur_frame, uint32_t txopp)
{
	DEBUG("%11.7f: [%03u] try to Add grant: (cur_txopp: %u).\n",
			(double)GetCurrentTime() / 10000000,
			scheduler.mac()->node_id(),
			txopp);

	schedule_alloc_list_t& request_list =
		scheduler.alloc_list(Data_scheduler::RECV_REQUEST);

	/*
	 * We must consider the number of total grant
	 * IEs, which may be specified for different requests,
	 * must be less than 64.
	 */
	if (!request_list.empty()) {

		VINFO("%11.7f: [%03u] %s: Dump allocation list:\n",
					(double)GetCurrentTime() / 10000000,
					scheduler.mac()->node_id(),
					__FUNCTION__);
		Data_scheduler::alloc_status_t status_ignored =
			static_cast<Data_scheduler::alloc_status_t>(
#if 0
					Data_scheduler::SENT_CONFIRM |
					Data_scheduler::RECV_CONFIRM |
#endif
					Data_scheduler::NBR_BUSY_XMIT |
					Data_scheduler::NBR_BUSY_RECV);
		VINFO_FUNC(scheduler.alloc_list_dump(
				static_cast<Data_scheduler::alloc_status_t>(
					Data_scheduler::STATUS_ALL & ~status_ignored)));
	}

	for ( ; !request_list.empty(); ) {

		if (getNumGrant() + getNumConfirm() >= NF_GRANT_IE_MAX) {

			VINFO("%11.7f: [%03u] %s: Defer sending remaining grant IEs"
						"because the number of packed grant IEs "
						"has reached the maximum number for a DSCH."
						"(cur_frame = %#x)\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__, cur_frame);
			break;
		}
		/*
		 * Get and remove a request from the request list.
		 */
		const schedule_alloc_t* request = request_list.front();
		request_list.pop_front();
		assert(request->peer());

		/*
		 * Get the availabilities sent to
		 * the corresponding node and link ID.
		 */
		schedule_alloc_list_t avail_recv_from_peer;
		if (!scheduler.alloc_find(avail_recv_from_peer,
					Data_scheduler::RECV_AVAILABILITY,
					request->peer()->node_id(),
					request->link_id())) {

			scheduler.alloc_list_dump(Data_scheduler::STATUS_ALL);
			assert(0);
#if 0
			/*
			 * Deal with the case where the corresponding
			 * availability IE has been expired.
			 */
			/*
			 * Recycle the request.
			 */
			delete request;
			continue;
#endif
		}

		/*
		 * The corresponding availability should be unique.
		 */
		assert(avail_recv_from_peer.size() == 1);
		schedule_alloc_t& avail = *avail_recv_from_peer.front();

		/*
		 * If the starting frame number of the availability IE is less than
		 * the current frame number, the starting frame number and frame range
		 * of the availability IE should be adjusted.
		 */
		if (avail.is_active(cur_frame)) {
			if (avail.frame_remain(cur_frame)) {
				if (avail.frame_range() !=
						Alloc_base::FRAME_RANGE_PERSISTENCE) {

					avail.frame_range(avail.frame_start() +
							avail.frame_range() -
							cur_frame);
				}
				avail.frame_start(cur_frame);

			} else {

				avail.frame_range(0);
				WARN("%11.7f: [%03u] %s: \e[1;31m[WARN] Set "
						"the availability to zero due to "
						"its expiration.\e[m\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(), __FUNCTION__);
				WARN_FUNC(avail.dump());
			}
		}

		_add_grant_info(&scheduler, &avail, request, cur_frame);

		/*
		 * The corresponding availability is no longer valid.
		 */
		VINFO("[%03u] %s: \e[1;33mRemove the availability IE: "
					"(cur_frame = %#x)\e[m\n",
					scheduler.mac()->node_id(),
					__FUNCTION__, cur_frame);

		VINFO_FUNC(avail.dump("Removed availability IE: "));
		assert(scheduler.alloc_remove(avail));

		/*
		 * Recycle the request.
		 */

		delete request;
	}
}

size_t
MSH_DSCH::_add_grant_info(class Data_scheduler* scheduler,
		const Alloc_base* avail_base,
		const Alloc_base* request_base, uint16_t cur_frame,
		size_t needed_area)
{
	const schedule_alloc_t* avail =
		dynamic_cast<const schedule_alloc_t*>(avail_base);

	const schedule_alloc_t* request =
		dynamic_cast<const schedule_alloc_t*>(request_base);

	assert(avail && request);

	schedule_alloc_t grant;
	if (!scheduler->get_grant(grant, *avail, *request, cur_frame, needed_area)) {

		VINFO("%11.7f: [%03u] %s: \e[1;34mSend a zero grant.\e[m\n",
					(double)GetCurrentTime() / 10000000,
					scheduler->mac()->node_id(), __FUNCTION__);
	}

	/*
	 * The frame range of the grant should be adjusted to
	 * conform to the corresponding persistence.
	 */

	_adjust_frame_range(grant);

	/*
	 * - The confirm message may be dropped due to collision. Thus,
	 *   we need to upgrade the previous grant being 'SENT_GRANT'
	 *   status to 'RECV_CONFIRM' status immediately.
	 * - We only do the upgrade in the 1-st grant of the MSH-DSCH and
	 *   for the same peer node.
	 */
	scheduler->auto_upgrade_pregrants(request->peer()->node_id());

	/*
	 * Register the grant with the scheduler and
	 * set the peer node which sent the request.
	 */

	scheduler->alloc_register(grant,
			Data_scheduler::SENT_GRANT, request->peer());
	/*
	 * Add grant IE.
	 */
	assert(_encapsulate_grant(grant));
	VINFO("%11.7f: [%03u] %s: \e[1;34m"
			"Add a grant: (cur_frame = %#x)\e[m\n",
			(double)GetCurrentTime() / 10000000,
			scheduler->mac()->node_id(), __FUNCTION__, cur_frame);

	VINFO_FUNC(grant.dump());

	//return (grant != 0);
	return (grant.area());
}


int
MSH_DSCH::add_confirm_info(Data_scheduler& scheduler,
		uint16_t cur_frame,
		uint32_t cur_tx_time,
		uint32_t& num_of_granted_data_schedules)
{
	schedule_alloc_list_t& grant_list =
		scheduler.alloc_list(Data_scheduler::RECV_GRANT);

	if (!grant_list.empty()) {

		VINFO("%11.7f: [%03u] %s: Dump allocation list:\n",
					(double)GetCurrentTime() / 10000000,
					scheduler.mac()->node_id(),
					__FUNCTION__);
		Data_scheduler::alloc_status_t status_ignored =
			static_cast<Data_scheduler::alloc_status_t>(
#if 0
					Data_scheduler::SENT_CONFIRM |
					Data_scheduler::RECV_CONFIRM |
#endif
					Data_scheduler::NBR_BUSY_XMIT |
					Data_scheduler::NBR_BUSY_RECV);
		VINFO_FUNC(scheduler.alloc_list_dump(
				static_cast<Data_scheduler::alloc_status_t>(
					Data_scheduler::STATUS_ALL & ~status_ignored)));
	}

	for ( ; !grant_list.empty(); ) {

		if (getNumGrant() + getNumConfirm() >= NF_GRANT_IE_MAX) {
			VINFO("%11.7f: [%03u] %s: Defer confirms to send due "
						"to reaching the max confirm "
						"number in a DSCH.\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__);
			break;
		}
		/*
		 * Get and remove the grant from the list.
		 */
		const schedule_alloc_t* grant = grant_list.front();
		grant_list.pop_front();

		/*
		 * We just copy the grant received to generate the
		 * corresponding confirm IE.
		 */

		schedule_alloc_t confirm(
				grant->frame_start(), grant->frame_range(),
				grant->slot_start(), grant->slot_range());

		/*
		 * Register the confirm IE with the scheduler and set the
		 * peer node which sent the grant IE.
		 */

		scheduler.alloc_register(confirm,
				Data_scheduler::SENT_CONFIRM, grant->peer());

		/*
		 * Add confirm IE.
		 */
		assert(_encapsulate_confirm(confirm));

		VINFO("%11.7f: [%03u] %s: Add a confirm (frn: %#x, txopp: %u):\n",
					(double)GetCurrentTime() / 10000000,
					scheduler.mac()->node_id(),
					__FUNCTION__,
					cur_frame,
					cur_tx_time);

		VINFO_FUNC(confirm.dump());
		/*
		 * Recycle the allocation.
		 */
		delete grant;

		++num_of_granted_data_schedules;

		scheduler.record_confirm_time(confirm, cur_tx_time);
		/*
		 * Fixed me.
		 */
		//_pop_dynamic_ht_info(grant, ie_list_p);

	}
	return 0;
}

int
MSH_DSCH::set_tx_holdoff_exp(u_int32_t htime_exp_val)
{
	Scheduling.f->XmtHoldExp = htime_exp_val;
	return 1;
}

void
MSH_DSCH::get_nbr_sch_entry(Ctrl_sch::Entry& nbr_sch_entry, int idx) const
{
	assert(idx < Scheduling.f->NSchedEntry);

	uint16_t node_id =
		(Scheduling.entry[idx].msbNbrNodeID << 8) +
		(Scheduling.entry[idx].lsbNbrNodeID);

	nbr_sch_entry.set_node_id(node_id);
	nbr_sch_entry.set_next_tx_mx(Scheduling.entry[idx].NextXmtMx);
	nbr_sch_entry.set_tx_holdoff_exp(Scheduling.entry[idx].XmtHoldExp);
}

uint8_t
MSH_DSCH::getNumGrant()
{
	int num = 0;
	int i;

	for (i = 0; i < f->NGrant; i++) {
		if (Grant[i].Direction == DIRECTION_TO_REQUEST) {
			num++;
		}
	}

	return num;
}

uint8_t
MSH_DSCH::getNumConfirm()
{
	return f->NGrant - getNumGrant();
}

void
MSH_DSCH::proc_availability(class Data_scheduler& scheduler,
		class Neighbor* peer, uint16_t cur_frame)
{
	for (int i = 0; i < getNumAvail(); i++) {
		/*
		 * Get availability IE.
		 */
		schedule_alloc_t avail;
		assert(get_availability_info(avail, i, cur_frame));

		/*
		 * Register the availability with the scheduler and set
		 * the peer node and the link of the availability to
		 * the link of corresponding request.
		 * XXX: Assumption here:
		 *      The availability with index i implies
		 *      the availability range of the request with index i.
		 */
		// Set an expiration time.
		//avail.frame_range() = NEGOTIATION_TIMEOUT;
		schedule_alloc_t request;
		assert(get_request_info(request, i));

		/*
		 * FIXME: If we want to support multi-link-access in each node, we must
		 * consider 'link' information in 'alloc_register' function.
		 */
		scheduler.alloc_register(avail,
				Data_scheduler::RECV_AVAILABILITY,
				peer, request.link_id());

		VINFO("%11.7f: [%03u] %s: \e[0;32mGet an availability: "
					"(cur_frame = %#x, "
					"rx_link ID of node %03u: %u)\e[m\n",
					(double)GetCurrentTime() / 10000000,
					scheduler.mac()->node_id(),
					__FUNCTION__, cur_frame,
					peer->node_id(),
					peer->rx_link()?peer->rx_link()->id():255);
		VINFO_FUNC(avail.dump());
	}
}

void
MSH_DSCH::proc_request(class Data_scheduler& scheduler, class Neighbor* peer)
{
	for (int i = 0; i < getNumRequest(); i++) {
		/*
		 * Get request IE.
		 */
		schedule_alloc_t request;
		assert(get_request_info(request, i));
		/*
		 * Skip if the request is not specified for me.
		 */
		if (!peer->tx_link() || request.link_id() != peer->tx_link()->id())
			continue;
		/*
		 * Register the request with the scheduler.
		 */
		scheduler.alloc_register(request,
				Data_scheduler::RECV_REQUEST, peer);

		VINFO("%lf: [%03u] %s: \e[0;33mGet a request: "
					"(rx_link ID of node %03u: %u)\e[m\n",
					GetCurrentTime()/10000000.0,
					scheduler.mac()->node_id(),
					__FUNCTION__,
					peer->node_id(),
					peer->rx_link()?peer->rx_link()->id():Mesh_link::UNKNOWN_ID);
		VINFO_FUNC(request.dump());
	}
}

void
MSH_DSCH::proc_grant(class Data_scheduler& scheduler, class Neighbor* peer, uint16_t cur_frame)
{
	for (int i = 0; i < getNumGrant(); i++) {
		/*
		 * Get grant IE.
		 */
		schedule_alloc_t grant;
		assert(get_grant_info(grant, i, cur_frame));
		/*
		 * Check if the grant is specified for me.
		 */
		if (peer->tx_link() && grant.link_id() == peer->tx_link()->id()) {

			VINFO("%11.7f: [%03u] %s: \e[0;34mGet a grant: "
						"(cur_frame = %#x, "
						"rx_link ID of node %03u: %u)\e[m\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__, cur_frame,
						peer->node_id(),
						peer->rx_link()?peer->rx_link()->id():Mesh_link::UNKNOWN_ID);
			VINFO_FUNC(grant.dump());

			assert(scheduler.check_and_remove_request_info(
						grant, peer->node_id(), IS_LAST_GRANT));

			/*
			 * I am the node who sent the request.
			 * Register the grant with the scheduler.
			 * The grant will be processed
			 * at the confirm sending time.
			 */
			scheduler.alloc_register(grant,
					Data_scheduler::RECV_GRANT, peer);

		} else {
			VINFO("%11.7f: [%03u] %s: \e[0;36mGet a grant: "
						"(cur_frame = %#x, "
						"rx_link ID of node %03u: %u)\e[m\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__, cur_frame,
						peer->node_id(),
						peer->rx_link()?peer->rx_link()->id():Mesh_link::UNKNOWN_ID);
			VINFO_FUNC(grant.dump());

			if (!scheduler.alloc_check_free_recv_grant(grant,
						peer->node_id(), cur_frame)) {
				ERROR("%11.7f: [%03u] %s: \e[1;31mFree alloc checking failed:"
							" peer node ID: %u\e[m\n",
							(double)GetCurrentTime() / 10000000,
							scheduler.mac()->node_id(),
							__FUNCTION__, peer->node_id());
				grant.dump();
				Data_scheduler::alloc_status_t status_ignored =
					static_cast<Data_scheduler::alloc_status_t>(
							Data_scheduler::SENT_NET_ENTRY |
							Data_scheduler::RECV_NET_ENTRY |
							Data_scheduler::SENT_AVAILABILITY |
							Data_scheduler::RECV_AVAILABILITY |
							Data_scheduler::RECV_CONFIRM |
							Data_scheduler::NBR_BUSY_XMIT |
							Data_scheduler::NBR_BUSY_RECV);
				scheduler.alloc_list_dump(
						static_cast<Data_scheduler::alloc_status_t>(
							Data_scheduler::STATUS_ALL_WITHOUT_REQUEST & ~status_ignored));
				scheduler.bitmap_dump();
				/*
				 * We can't guarantee the consistency of
				 * schedule allocations among the node and
				 * its neighbors in the network entry stage,
				 * so we let it go here.
				 */
				//assert(0);
			}

			/*
			 * I am one of the neighbor of the granter.
			 * Assume the transmission takes places as granted.
			 * Register the grant as busy with the scheduler.
			 */
			scheduler.alloc_register(grant,
					Data_scheduler::NBR_BUSY_RECV, peer);
		}
	}
}

void
MSH_DSCH::proc_confirm(class Data_scheduler& scheduler,
		class Neighbor* peer, uint16_t cur_frame)
{
	if (getNumConfirm()) {
		VINFO("%11.7f: [%03u] %s: Dump allocation list:\n",
					(double)GetCurrentTime() / 10000000,
					scheduler.mac()->node_id(),
					__FUNCTION__);
		Data_scheduler::alloc_status_t status_ignored =
			static_cast<Data_scheduler::alloc_status_t>(
#if 0
					Data_scheduler::SENT_CONFIRM |
					Data_scheduler::RECV_CONFIRM |
#endif
					Data_scheduler::NBR_BUSY_XMIT |
					Data_scheduler::NBR_BUSY_RECV);
		VINFO_FUNC(scheduler.alloc_list_dump(
				static_cast<Data_scheduler::alloc_status_t>(
					Data_scheduler::STATUS_ALL & ~status_ignored)));
	}

	for (int i = 0; i < getNumConfirm(); i++) {
		/*
		 * Get confirm IE.
		 */
		schedule_alloc_t confirm;
		assert(get_confirm_info(confirm, i, cur_frame));
		/*
		 * Check if the confirm is specified for me.
		 */
		if (peer->tx_link() && confirm.link_id() == peer->tx_link()->id()) {
			/*
			 * I am the node who sent the grant.
			 * Register the confirm with the scheduler.
			 */
			assert(scheduler.check_and_sync_confirm_with_grant(
						confirm, peer->node_id()));

			scheduler.alloc_register(confirm,
					Data_scheduler::RECV_CONFIRM, peer);

			VINFO("%11.7f: [%03u] %s: \e[0;35mGet a confirm: "
						"(cur_frame = %#x, "
						"rx_link ID of node %03u: %u)\e[m\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__, cur_frame,
						peer->node_id(),
						peer->rx_link()?peer->rx_link()->id():Mesh_link::UNKNOWN_ID);
			VINFO_FUNC(confirm.dump());
		} else {
			/*
			 * I am one of the neighbor of the requester.
			 * Assume the transmission takes places as granted.
			 * Register the confirm as busy with the scheduler.
			 */
			assert(scheduler.check_and_sync_confirm_with_avail(confirm,
							peer->node_id(), IS_NOT_LAST_CONFIRM));
			VINFO("%11.7f: [%03u] %s: \e[1;36mGet a confirm: "
						"(cur_frame = %#x, "
						"rx_link ID of node %03u: %u)\e[m\n",
						(double)GetCurrentTime() / 10000000,
						scheduler.mac()->node_id(),
						__FUNCTION__, cur_frame,
						peer->node_id(),
						peer->rx_link()?peer->rx_link()->id():Mesh_link::UNKNOWN_ID);
			VINFO_FUNC(confirm.dump());

			if (!scheduler.alloc_check_free_recv_confirm(confirm,
						peer->node_id(), cur_frame)) {
				ERROR("%11.7f: [%03u] %s: \e[1;31mFree alloc checking failed:"
							" peer node ID: %u\e[m\n",
							(double)GetCurrentTime() / 10000000,
							scheduler.mac()->node_id(),
							__FUNCTION__, peer->node_id());
				confirm.dump();
				Data_scheduler::alloc_status_t	status_ignored =
					static_cast<Data_scheduler::alloc_status_t>(
							Data_scheduler::SENT_NET_ENTRY |
							Data_scheduler::RECV_NET_ENTRY |
							Data_scheduler::SENT_AVAILABILITY |
							Data_scheduler::RECV_AVAILABILITY |
							Data_scheduler::NBR_BUSY_XMIT |
							Data_scheduler::NBR_BUSY_RECV);
				scheduler.alloc_list_dump(
						static_cast<Data_scheduler::alloc_status_t>(
							Data_scheduler::STATUS_ALL_WITHOUT_REQUEST & ~status_ignored));
				scheduler.bitmap_dump();
				assert(0);
			}
			scheduler.alloc_register(confirm,
					Data_scheduler::NBR_BUSY_XMIT, peer);
		}
	}
}

bool
MSH_DSCH::get_availability_info(
		Alloc_base& avail, int idx, uint16_t cur_frame)
{
	if (idx >= getNumAvail())
		return false;

	assert(dynamic_cast<schedule_alloc_t*>(&avail));
	dynamic_cast<schedule_alloc_t&>(avail) = schedule_alloc_t(
			_frame_sync(Availability[idx].StartNumber, cur_frame),
			_persistence2frame(Availability[idx].Persistence),
			Availability[idx].SlotStart,
			Availability[idx].SlotRange);
	return true;
}

bool
MSH_DSCH::get_request_info(Alloc_base& request, int idx)
{
	if (idx >= getNumRequest())
		return false;

	assert(dynamic_cast<schedule_alloc_t*>(&request));
	dynamic_cast<schedule_alloc_t&>(request) = schedule_alloc_t(
			0,
			_persistence2frame(Request[idx].DemandPersist),
			0,
			Request[idx].DemandLevel,
			Request[idx].LinkID);
	return true;
}

bool
MSH_DSCH::get_grant_info(
		Alloc_base& grant, int idx, uint16_t cur_frame)
{
	if (idx >= getNumGrant())
		return false;

	int i;
	for (i = 0; i < f->NGrant; i++) {
		if (Grant[i].Direction == DIRECTION_TO_REQUEST && idx-- == 0)
			break;
	}

	assert(dynamic_cast<schedule_alloc_t*>(&grant));
	dynamic_cast<schedule_alloc_t&>(grant) = schedule_alloc_t(
			_frame_sync(Grant[i].StartNumber, cur_frame),
			_persistence2frame(Grant[i].Persistence),
			Grant[i].SlotStart,
			Grant[i].SlotRange,
			Grant[i].LinkID);
	return true;
}

bool
MSH_DSCH::get_confirm_info(
		Alloc_base& confirm, int idx, uint16_t cur_frame)
{
	if (idx >= getNumConfirm())
		return false;

	int i;
	for (i = 0; i < f->NGrant; i++) {
		if (Grant[i].Direction == DIRECTION_FROM_REQUEST && idx-- == 0)
			break;
	}

	assert(dynamic_cast<schedule_alloc_t*>(&confirm));
	dynamic_cast<schedule_alloc_t&>(confirm) = schedule_alloc_t(
			_frame_sync(Grant[i].StartNumber, cur_frame),
			_persistence2frame(Grant[i].Persistence),
			Grant[i].SlotStart,
			Grant[i].SlotRange,
			Grant[i].LinkID);
	return true;
}


void
MSH_DSCH::_adjust_frame_range(Alloc_base& alloc)
{
	uint8_t frame_range = alloc.frame_range();

	switch(frame_range) {
	case 0:
	case 1u:
	case 2u:
	case 4u:
	case 8u:
	case 32u:
	case 128u:
	case 0xffu:
		return;
	}

	if (frame_range < 4u)
		alloc.frame_range(2u);
	else if (frame_range < 8u)
		alloc.frame_range(4u);
	else if (frame_range < 32u)
		alloc.frame_range(8u);
	else if (frame_range < 128u)
		alloc.frame_range(32u);
	else
		alloc.frame_range(128u);
}

bool
MSH_DSCH::_encapsulate_availability(const Alloc_base& avail)
{
	int i = f->NAvail;

	if (i >= NF_AVAILABILITY_IE_MAX)
		return false;

	f->NAvail++;

	Availability[i].StartNumber = avail.frame_start();
	Availability[i].SlotStart = avail.slot_start();
	Availability[i].SlotRange = avail.slot_range();
	Availability[i].Persistence = _frame2persistence(avail.frame_range());
	//  Availability[i].Direction =
	//  Availability[i].Channel =

	return true;
}


bool
MSH_DSCH::_encapsulate_request(const Alloc_base& request)
{
	int i = f->NRequest;

	if (i >= NF_REQUEST_IE_MAX)
		return false;

	f->NRequest++;

	assert(dynamic_cast<const schedule_alloc_t*>(&request));
	Request[i].LinkID =
		dynamic_cast<const schedule_alloc_t*>(&request)->link_id();
	Request[i].DemandLevel = request.slot_range();
	Request[i].DemandPersist = _frame2persistence(request.frame_range());

	return true;
}

bool
MSH_DSCH::_encapsulate_grant(const Alloc_base& grant)
{
	int i = f->NGrant;

	if (i >= NF_GRANT_IE_MAX)
		return false;

	f->NGrant++;

	assert(dynamic_cast<const schedule_alloc_t*>(&grant));
	Grant[i].LinkID =
		dynamic_cast<const schedule_alloc_t*>(&grant)->link_id();
	Grant[i].StartNumber = grant.frame_start();
	Grant[i].SlotStart = grant.slot_start();
	Grant[i].SlotRange = grant.slot_range();
	Grant[i].Direction = DIRECTION_TO_REQUEST;
	Grant[i].Persistence = _frame2persistence(grant.frame_range());
	//  Grant[i].Channel =

	return true;
}

bool
MSH_DSCH::_encapsulate_confirm(const Alloc_base& confirm)
{
	int i = f->NGrant;

	if (i >= NF_GRANT_IE_MAX || !_encapsulate_grant(confirm))
		return false;

	Grant[i].Direction = DIRECTION_FROM_REQUEST;

	return true;
}

uint16_t
MSH_DSCH::_frame_sync(uint8_t unsync_frame, uint16_t cur_frame)
{
	uint16_t sync_frame =
		(cur_frame & ~FRAME_BIT_MASK) | unsync_frame;

	/*
	 * The synchronized frame should be equal to or larger
	 * than the current frame.
	 * Otherwise the frame was wrapped before transmission,
	 * it has to be shifted forward FRAME_START_MAX frames.
	 */
	if (((sync_frame - cur_frame) & Alloc_base::FRAME_BIT_MASK)
			>= Alloc_base::FRAME_START_MAX) {
		sync_frame += Alloc_base::FRAME_START_MAX;
		sync_frame &= Alloc_base::FRAME_BIT_MASK;
	}
	return sync_frame;
}

uint8_t
MSH_DSCH::_persistence2frame(uint8_t persistence)
{
	switch (persistence) {
	case 0u: return 0u;
	case 1u: return 1u;
	case 2u: return 2u;
	case 3u: return 4u;
	case 4u: return 8u;
	case 5u: return 32u;
	case 6u: return 128u;
	case 7u: return 0xffu;
	default: assert(0);
	}
}

uint8_t
MSH_DSCH::_frame2persistence(uint8_t frame)
{
	switch (frame) {
	case 0u: return 0u;
	case 1u: return 1u;
	case 2u: return 2u;
	case 4u: return 3u;
	case 8u: return 4u;
	case 32u: return 5u;
	case 128u: return 6u;
	case 0xffu: return 7u;
	default: FATAL("frame = %#x\n", frame);
	}
}
