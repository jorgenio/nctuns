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

#include "scheduler.h"

#include <cstdio>
#include <nctuns_api.h>

#include "bitmap.h"
#include "../ctrl/ht_assigner.h"
#include "../ctrl/info.h"
#include "../ctrl/scheduler.h"
#include "../mesh_schedule_info.h"
#include "../../frame_mngr.h"
#include "../../mac802_16_meshss.h"
#include "../../mesh_connection.h"
#include "../../msh_ncfg.h"
#include "../../neighbor.h"
#include "../../net_entry_mngr.h"


#define VERBOSE_LEVEL	MSG_INFO
#include "../../../verbose.h"


Bitmap*
Data_scheduler::_bitmap = NULL;

size_t
Data_scheduler::_bitmap_ref_cnt = 0;

/*
 * Member function definitions of class `Data_scheduler'.
 */

/*
 * Constructor
 */
Data_scheduler::Data_scheduler(const mac802_16_MeshSS* mac)
: _mac(mac)
, _statistic_list(NULL)
, _pseudo_schedule_flag(0)
, _pseudo_peer_conn_list_index(0)
, _pseudo_peer_conn_list_size(0)
{
}

/*
 * Destructor
 */
Data_scheduler::~Data_scheduler()
{
	/*
	 * Recycle the all allocation list.
	 */
	for (int i = 0; i < IDX_DUMMY_END; i++) {
		for ( ; !_alloc_list[i].empty(); _alloc_list[i].pop_front())
			delete _alloc_list[i].front();
	}
	/*
	 * Recycle the static bitmap.
	 */
	if (!--_bitmap_ref_cnt)
		delete _bitmap;

	/*
	 * Recycle the statistic list.
	 */
	delete _statistic_list;
	_statistic_list = NULL;
}

void
Data_scheduler::start()
{
	DEBUG("nf_holdoff_frame = %u\n", _nf_holdoff_frame());
	/*
	 * Allocate the static bitmap if it has not done.
	 */
	if (_bitmap_ref_cnt++ == 0)
		_bitmap = new Bitmap(_mac->fr_mngr()->nf_slot(), Alloc_base::FRAME_START_MAX);

	/*
	 * Allocate the statistic list.
	 */
	assert(_statistic_list == NULL);
	_statistic_list = new sched_alloc_stat_list_t(
			_mac->node_id(),
			_mac->fr_mngr()->max_ncfg_tx_opp(),
			_mac->fr_mngr()->max_dsch_tx_opp());
}

schedule_alloc_list_t&
Data_scheduler::alloc_list(alloc_status_t status)
{
	return _alloc_list[_status2idx(status)];
}

/*
 * Get free allocation of 'Network entry process' usage for sponsor node.
 * This allocation must be 'persistnet' one.
 * Only sponsor node will call this function.
 */
Data_scheduler::Alloc*
Data_scheduler::get_nent_alloc(uint8_t range)
{
	Alloc nent_alloc;

	/*
	 * Init bitmap table, and set bounary searched.
	 */
	_bitmap->init(_mac->fr_mngr()->cur_frame());
	_bitmap_cumulate(STATUS_ALL_WITHOUT_REQUEST);
	_bitmap->set_boundary();

	/*
	 * If the 'NENT allocation' is not found, then return NULL.
	 */
	if (!(_bitmap->get_free_alloc(nent_alloc,
					Bitmap::MINISLOT,
					Alloc_base::FRAME_RANGE_PERSISTENCE,
					range)))
		return NULL;

	/*
	 * - Add the 'NENT allocation' to 'NetEntry alloction_list', and the
	 *   list will set the allocation's status to 'NetEntry' automatically.
	 * - Since the peer node has not become functional,
	 *   the peer node of this allocation has not to be specified here.
	 */
	alloc_register(nent_alloc, SENT_NET_ENTRY);
	Alloc* ret_nent_alloc = new Alloc(nent_alloc);
	return ret_nent_alloc;
}

/*
 * Free the 'NENT' allocation found.
 */
bool
Data_scheduler::free_nent_alloc(uint16_t node_id, alloc_status_t status)
{
	bool free_ok_flag = false;

	/*
	 * Get 'NetEntry' allocation list.
	 */
	alloc_list_t& nent_alloc_list = alloc_list(status);

	/*
	 * Find corresponding allocation which is matched by 'NodeID'.
	 */
	for (alloc_list_t::iterator nent_it = nent_alloc_list.begin();
			nent_it != nent_alloc_list.end(); nent_it++) {

		const Alloc& alloc = **nent_it;
		if ((alloc.peer()?alloc.peer()->node_id():NEW_NODE_ID)
				== node_id) {
			delete *nent_it;
			nent_alloc_list.erase(nent_it);

			free_ok_flag = true;
			break;
		}
	}
	return free_ok_flag;
}

/*
 * For other operational nodes nearby sponsor node will call this function.
 * Hence, we must not consider the allocations of busy status because the 'busy status'
 * allocation and the 'nent status' allocation can co-exist.
 */
bool
Data_scheduler::set_nent_alloc_and_no_consider_busy_status(Neighbor* peer,
		Alloc& sch_alloc, uint32_t currFrameNum)
{
	/*
	 * '0' means the busy status will be not considered.
	 */
	return _set_nent_alloc(0, peer, sch_alloc, currFrameNum);
}

/*
 * Only for new node will call this function.
 * Hence, we must consider the allocations of busy status.
 */
bool
Data_scheduler::set_nent_alloc_and_consider_busy_status(Neighbor* peer,
		Alloc& sch_alloc, uint32_t currFrameNum)
{
	/*
	 * '1' means the busy status will be considered.
	 */
	return _set_nent_alloc(1, peer, sch_alloc, currFrameNum);
}

bool
Data_scheduler::get_availability(Alloc& avail,
		const Mesh_connection* connection, uint16_t cur_frame)
{
	alloc_list_t alloc_list;

	/*
	 * If there is availability of the peer node waiting to be sent,
	 * reject the request.
	 */
	alloc_list.clear();
	alloc_find(alloc_list, SENT_AVAILABILITY, connection->node_id_dst());
	if (alloc_list.size())
		return false;
	/*
	 * If there is grant for the peer node waiting to be processed,
	 * reject the request.
	 */
	alloc_list.clear();
	alloc_find(alloc_list, RECV_GRANT, connection->node_id_dst());
	if (alloc_list.size())
		return false;
	/*
	 * Get the allocation list of with transmission status
	 * to determine if there are allocated schedule for the peer node.
	 */
	alloc_list.clear();
	alloc_find(alloc_list, SENT_CONFIRM, connection->node_id_dst());

#if SCHEDULING_EARLY_THREE_WAY_HANDSHAKE

	if (alloc_list.size() == 2)
		return false;

	if (alloc_list.size() > 2) {
		FATAL("So far, it is not allowed that more than two schedules"
					"to the same node coexist.\n");
	}

	if (alloc_list.size() == 1) {

		Alloc* alloc = alloc_list.front();

		/*
		 * cclin try
		 */
		if (!alloc->is_active(cur_frame) ||
				alloc->frame_remain(cur_frame)
				> _nf_holdoff_frame() * 10u) {

			DEBUG("%s: Peer node ID = %u, cur_frame = %d, "
						"schedHoldoff = %d, "
						"alloc dump:\n",
						__FUNCTION__,
						connection->node_id_dst(),
						cur_frame, _nf_holdoff_frame());
			DEBUG_FUNC(alloc->dump());
			return false;
		}
	}

#else /* ! SCHEDULING_EARLY_THREE_WAY_HANDSHAKE */

	/*
	 * Measure the performances of the most stupid design.
	 * Reject the request if there is schedule given to the link.
	 */
	if (alloc_list.size() > 0)
		return false;

#endif /* SCHEDULING_EARLY_THREE_WAY_HANDSHAKE */

	DEBUG("[%03u] %s: \e[1;33mDump allocation list: "
				"(peer node ID: %u, cur_frame = %#x)\e[m\n",
				_mac->node_id(), __FUNCTION__,
				connection->node_id_dst(), cur_frame);
	DEBUG_FUNC(alloc_list_dump(STATUS_ALL));

	/*
	 * Determine the range of frame and slot according to the connection.
	 */
	uint16_t frame_range = SCHEDULE_VALIDITY;
	uint8_t slot_range = AVAILABILITY_SIZE;

	/*
	 * The availibity size is bounded in the value of 0 ~ 127.
	 */
	assert(slot_range <= Alloc_base::SLOT_RANGE_MAX);

	/*
	 * The schedule hold off here to reduce the validity exhausting during
	 * procedure of three-way-handshake.
	 */
	_bitmap->init(cur_frame);
	_bitmap->set_boundary(cur_frame + _nf_holdoff_frame());
	_bitmap_cumulate(STATUS_ALL_WITHOUT_REQUEST);

	if (!_bitmap->get_free_alloc(avail,
				Bitmap::MINISLOT, frame_range, slot_range)) {
		INFO("%11.7f: [%03u] %s: Can't find availabilities "
				"for peer node %u\n",
				(double)GetCurrentTime() / 10000000,
				_mac->node_id(), __FUNCTION__,
				connection->node_id_dst());
		return false;
	}

#if 0
	printf("%s: dump_bitmap:\n", __FUNCTION__);
	_bitmap->dump();
#endif

	return true;
}

void
Data_scheduler::get_request(Alloc& alloc_request,
		const Mesh_connection* connection, size_t avail_range,
		uint32_t alloc_validity, uint32_t alloc_slot_range)
{
	/*
	 * Determine the range of frame and slot according to
	 * the connection profiles.
	 */
	if (!alloc_validity)
		alloc_request.frame_range(SCHEDULE_VALIDITY);
	else
		alloc_request.frame_range(alloc_validity);
	/*
	 * Set slot range.
	 */
	uint8_t slot_range;

	if (!alloc_slot_range) {

		slot_range = REQUEST_SIZE;
		if (connection->pending_data_len() < 10000)
			slot_range /= 10;
		VINFO("[%03u]%s: \e[1;31mConnection data length: %d, "
				"Request size: %u\e[m\n",
				_mac->node_id(), __FUNCTION__,
				connection->pending_data_len(),
				slot_range);
	}
	else
		slot_range = alloc_slot_range;

	if (slot_range > avail_range)
		slot_range = avail_range;

	alloc_request.slot_range(slot_range);
}

bool
Data_scheduler::get_grant(Alloc& grant,
		const Alloc& avail, const Alloc& request,
		uint16_t cur_frame, size_t needed_area)
{
	/*
	 * XXX: Multi-grant scheme can be implemented here.
	 * Find the available grant range in the peer node's availability area.
	 * Set the search range in the availability area.
	 */
	_bitmap->init(cur_frame);
	_bitmap->set_boundary(
			avail.frame_start(), avail.slot_start(),
			avail.frame_range(), avail.slot_range());

	_bitmap_cumulate(static_cast<alloc_status_t>(
				STATUS_ALL_WITHOUT_REQUEST &
				~RECV_AVAILABILITY));
	/*
	 * Cumulate the received availabilities excluding what was
	 * sent by the requester.
	 */
	alloc_list_t& recv_avail_list = alloc_list(RECV_AVAILABILITY);
	for (alloc_list_t::iterator it = recv_avail_list.begin();
			it != recv_avail_list.end(); it++) {
		const Alloc& availability = **it;
		assert(availability.peer());
		if (availability.peer()->node_id() == request.peer()->node_id())
			continue;
		_bitmap->cumulate(availability);
	}

	if (!_bitmap->get_free_alloc(grant, Bitmap::MINISLOT,
				request.frame_range(), request.slot_range(), needed_area) ||
			grant <= ALLOC_GRANT_MIN) {

		
		/*
		 * A request must correspond to a grant regardless
		 * the amount of the granted allocation.
		 * Hence, we set the start of the grant to the start of
		 * the availability to tell the peer to which availability
		 * the grant corresponds and the range of the grant is set
		 * to zero that says nothing granted.
		 */
		grant.frame_start(avail.frame_start());
		grant.slot_start(avail.slot_start());
		grant.frame_range(0);
		return false;
	}

	return true;
}

bool
Data_scheduler::check_and_remove_request_info(
		Alloc& grant, uint16_t peer_node_id, bool is_last_grant)
{
	/*
	 * Get the availability with corresponding node.
	 */
	bool is_availability_found = false;
	alloc_list_t& avail_sent = alloc_list(SENT_AVAILABILITY);
	for (alloc_list_t::iterator it = avail_sent.begin();
			it != avail_sent.end(); it++) {
		const Alloc& avail = **it;
		assert(avail.peer());
		if (avail.peer()->node_id() != peer_node_id)
			continue;

		/*
		 * Synchronize the frame of the confirm with the availability.
		 */
		grant.sync_frame_with_avail(avail);
		/*
		 * Check whether the grant is completely enclosed by
		 * the availability.
		 */
		if (grant.is_in_between(avail)) {

			_statistic_list->record_granted_time(
					*it, GetCurrentTime());

			DEBUG("%11.7f: [%03u] %s: Record a grant:\n",
						(double)GetCurrentTime() / 10000000,
						_mac->node_id(), __FUNCTION__);
			DEBUG_FUNC(grant.dump("Grant:"));
			DEBUG_FUNC(avail.dump("Availability:"));

			/*
			 * Recycle the availability.
			 */
			VINFO("[%03u] %s: \e[1;33mRemove the matched avail:\e[m\n",
						_mac->node_id(), __FUNCTION__);
			VINFO_FUNC(avail.dump("Removed availability:"));
			delete *it;
			avail_sent.erase(it--);
			is_availability_found = true;
			break;
		}
	}

	/*
	 * We do not use any information from these requests under
	 * current implementation. Hence, we just recycle them here.
	 */

	size_t nf_request_remove = 0;
	nf_request_remove = _alloc_remove(IDX_SENT_REQUEST, peer_node_id);
	
	if (!(is_availability_found && nf_request_remove)) {
		WARN("[%03u] %s: \e[1;33mDump allocation list:\e[m\n",
					_mac->node_id(), __FUNCTION__);
		alloc_list_dump(STATUS_ALL);
	}
	if (!is_availability_found)
		WARN("[%03u] %s: \e[1;33mThe corresponding availability ",
					_mac->node_id(), __FUNCTION__);
	if (!nf_request_remove)
		WARN("[%03u] %s: \e[1;33mThe corresponding requests ",
					_mac->node_id(), __FUNCTION__);

	if (!(is_availability_found && nf_request_remove)) {
		WARN( "is not found, dump grant:\e[m\n");
		WARN_FUNC(grant.dump());
	}

	return is_availability_found && nf_request_remove;
}

bool
Data_scheduler::check_and_sync_confirm_with_avail(
		Alloc& confirm, uint16_t peer_node_id, bool is_last_confirm)
{
	/*
	 * Get the availabilities received from the corresponding node.
	 */
	bool is_availability_found = false;
	alloc_list_t& avail_recv = alloc_list(RECV_AVAILABILITY);
	for (alloc_list_t::iterator it = avail_recv.begin();
			it != avail_recv.end(); it++) {
		const Alloc& avail = **it;
		assert(avail.peer());
		if (avail.peer()->node_id() != peer_node_id ||
				avail.link_id() != confirm.link_id())
			continue;
		/*
		 * Synchronize the frame of the confirm with the availability.
		 */
		confirm.sync_frame_with_avail(avail);

		/*
		 * Check if the confirm is completely matched with the avail sent.
		 * Recycle the avail if the corresponding confirm was received.
		 */
		if (confirm.is_in_between(avail)) {
			/*
			 * The matched availability should be unique.
			 */
			if (is_availability_found)
				WARN("[%03u] %s: \e[1;33m The matched availability "
							"should be unique. "
							"(peer node ID: %u)\e[m\n",
							_mac->node_id(), __FUNCTION__,
							peer_node_id);

			VINFO("[%03u] %s: \e[1;33mRemove the matched avail: "
						"(peer node ID: %u)\e[m\n",
						_mac->node_id(), __FUNCTION__,
						peer_node_id);
			VINFO_FUNC(avail.dump("Removed availability:"));
			delete &avail;
			avail_recv.erase(it--);
			is_availability_found = true;
		} else {
			DEBUG("[%03u] %s: \e[1;33mThe confirm does not match the avail, "
						"dump avail and confirm: "
						"peer node ID: %u\e[m\n",
						_mac->node_id(), __FUNCTION__,
						peer_node_id);
			DEBUG_FUNC(avail.dump("Availability:"));
			DEBUG_FUNC(confirm.dump("Confirm:"));
			DEBUG_FUNC(alloc_list_dump(STATUS_ALL));
			//assert(0);
		}
	}
	if (!is_availability_found)
		WARN("[%03u] %s: \e[1;33m Assume the corresponding "
					"availability was sent before "
					"this node functional, since we "
					"do not find matched one. "
					"(peer node ID: %u)\e[m\n",
					_mac->node_id(), __FUNCTION__,
					peer_node_id);

	return true;
}

bool
Data_scheduler::check_and_sync_confirm_with_grant(
		Alloc& confirm, uint16_t peer_node_id)
{
	/*
	 * Get the grant sent to the corresponding node.
	 */
	bool is_grant_found = false;
	alloc_list_t& grant_sent = alloc_list(SENT_GRANT);
	for (alloc_list_t::iterator it = grant_sent.begin();
			it != grant_sent.end(); it++) {
		const Alloc& grant = **it;
		assert(grant.peer());

		/*
		 * We must check that the found grant match actually (ie. last 8-bit)
		 * to the confirm because there may be collision conditions.
		 */
		if (grant.peer()->node_id() != peer_node_id ||
				   !confirm.is_match_with_grant(grant))
			continue;

		/*
		 * Synchronize the frame of the confirm with the grant.
		 */
		assert(confirm.sync_frame_with_grant(grant));
		/*
		 * Check if the confirm is completely matched with the grant sent.
		 * Recycle the grant if the corresponding confirm was received.
		 */
		if (confirm == grant) {
			VINFO("[%03u] %s: \e[1;33mRemove the matched grant: "
						"(peer node ID: %u)\e[m\n",
						_mac->node_id(), __FUNCTION__,
						peer_node_id);
			VINFO_FUNC(grant.dump("Removed grant: "));
			delete &grant;
			grant_sent.erase(it--);
			is_grant_found = true;
			break;
		} else {
			ERROR("[%03u] \e[1;31mThe confirm does not match "
						"the grant, dump grant and "
						"confirm:\e[m\n",
						_mac->node_id());
			grant.dump();
			confirm.dump();
			alloc_list_dump(STATUS_ALL);
			assert(0);
		}
	}
	assert(is_grant_found);

	return true;
}


void
Data_scheduler::remove_expired_alloc(uint16_t cur_frame)
{
	_remove_expired_alloc(SENT_CONFIRM, cur_frame);
	_remove_expired_alloc(RECV_CONFIRM, cur_frame);
	_remove_expired_alloc(NBR_BUSY_XMIT, cur_frame);
	_remove_expired_alloc(NBR_BUSY_RECV, cur_frame);

#if 1
	_remove_expired_alloc(RECV_AVAILABILITY, cur_frame, FR_NEGOTIATION);
#endif
	_remove_expired_alloc(SENT_AVAILABILITY, cur_frame, FR_NEGOTIATION);
	_remove_expired_alloc(SENT_GRANT, cur_frame, FR_NEGOTIATION);
}

size_t
Data_scheduler::refresh_job_list()
{
	uint16_t cur_frame = _mac->fr_mngr()->cur_frame();
	/*
	 * Get the allocation for transmission.
	 */
	alloc_list_t& confirm_list = _alloc_list[_status2idx(SENT_CONFIRM)];
	for (alloc_list_t::iterator it = confirm_list.begin();
			it != confirm_list.end(); it++) {
		if (!(*it)->is_active(cur_frame))
			continue;

		assert(_job_list.find((*it)->slot_start()) == _job_list.end());
		_job_list[(*it)->slot_start()] = *it;
	}

	/*
	 * Get the allocation for sponsoring the new node.
	 * We never sponsor more than one nodes.
	 */
	alloc_list_t& sent_nent_list = _alloc_list[_status2idx(SENT_NET_ENTRY)];
	if (sent_nent_list.size()) {
		assert(sent_nent_list.size() < 2);
		Alloc* nent = sent_nent_list.front();
		if (nent->is_active(cur_frame)) {

			assert(_job_list.find(nent->slot_start())
					== _job_list.end());
			_job_list[nent->slot_start()] = nent;
		}
	}

	/*
	 * Get the allocations for transmit data to the sponsor.
	 */
	const Neighbor* sponsor = _mac->net_entry_mngr()->sponsor();
	uint16_t sponsor_node_id = sponsor?sponsor->node_id():0;
	alloc_list_t& recv_nent_list = _alloc_list[_status2idx(RECV_NET_ENTRY)];
	for (alloc_list_t::iterator it = recv_nent_list.begin();
			it != recv_nent_list.end(); it++) {

		const Alloc* nent = *it;
		DEBUG("%s: Dump nent:\n", __FUNCTION__);
		DEBUG_FUNC(nent->dump());

		assert(nent->peer());
		if (!nent->is_active(cur_frame) ||
				nent->peer()->node_id() != sponsor_node_id)
			continue;

		assert(_job_list.find(nent->slot_start()) == _job_list.end());
		_job_list[nent->slot_start()] = *it;
	}

	return _job_list.size();
}

size_t
Data_scheduler::alloc_find(
		alloc_list_t& alloc_list, alloc_status_t status) const
{
	size_t      append_cnt = 0;

	if (status & SENT_NET_ENTRY)
		append_cnt += _alloc_find(alloc_list, IDX_SENT_NET_ENTRY);

	if (status & SENT_AVAILABILITY)
		append_cnt += _alloc_find(alloc_list, IDX_SENT_AVAILABILITY);

	if (status & SENT_REQUEST)
		append_cnt += _alloc_find(alloc_list, IDX_SENT_REQUEST);

	if (status & SENT_GRANT)
		append_cnt += _alloc_find(alloc_list, IDX_SENT_GRANT);

	if (status & SENT_CONFIRM)
		append_cnt += _alloc_find(alloc_list, IDX_SENT_CONFIRM);

	if (status & RECV_NET_ENTRY)
		append_cnt += _alloc_find(alloc_list, IDX_RECV_NET_ENTRY);

	if (status & RECV_AVAILABILITY)
		append_cnt += _alloc_find(alloc_list, IDX_RECV_AVAILABILITY);

	if (status & RECV_REQUEST)
		append_cnt += _alloc_find(alloc_list, IDX_RECV_REQUEST);

	if (status & RECV_GRANT)
		append_cnt += _alloc_find(alloc_list, IDX_RECV_GRANT);

	if (status & RECV_CONFIRM)
		append_cnt += _alloc_find(alloc_list, IDX_RECV_CONFIRM);

	if (status & NBR_BUSY_XMIT)
		append_cnt += _alloc_find(alloc_list, IDX_NBR_BUSY_XMIT);

	if (status & NBR_BUSY_RECV)
		append_cnt += _alloc_find(alloc_list, IDX_NBR_BUSY_RECV);

	return append_cnt;
}

size_t
Data_scheduler::alloc_find(alloc_list_t& alloc_list,
		alloc_status_t status, uint16_t node_id, uint8_t link_id) const
{
	size_t      append_cnt = 0;

	if (status & SENT_NET_ENTRY)
		append_cnt += _alloc_find(alloc_list,
				IDX_SENT_NET_ENTRY, node_id, link_id);

	if (status & SENT_AVAILABILITY)
		append_cnt += _alloc_find(alloc_list,
				IDX_SENT_AVAILABILITY, node_id, link_id);

	if (status & SENT_REQUEST)
		append_cnt += _alloc_find(alloc_list,
				IDX_SENT_REQUEST, node_id, link_id);

	if (status & SENT_GRANT)
		append_cnt += _alloc_find(alloc_list,
				IDX_SENT_GRANT, node_id, link_id);

	if (status & SENT_CONFIRM)
		append_cnt += _alloc_find(alloc_list,
				IDX_SENT_CONFIRM, node_id, link_id);

	if (status & RECV_NET_ENTRY)
		append_cnt += _alloc_find(alloc_list,
				IDX_RECV_NET_ENTRY, node_id, link_id);

	if (status & RECV_AVAILABILITY)
		append_cnt += _alloc_find(alloc_list,
				IDX_RECV_AVAILABILITY, node_id, link_id);

	if (status & RECV_REQUEST)
		append_cnt += _alloc_find(alloc_list,
				IDX_RECV_REQUEST, node_id, link_id);

	if (status & RECV_GRANT)
		append_cnt += _alloc_find(alloc_list,
				IDX_RECV_GRANT, node_id, link_id);

	if (status & RECV_CONFIRM)
		append_cnt += _alloc_find(alloc_list,
				IDX_RECV_CONFIRM, node_id, link_id);

	return append_cnt;
}

bool
Data_scheduler::alloc_remove(const Alloc& alloc_to_rm)
{
	assert(alloc_to_rm.peer());
	assert(alloc_to_rm.link_id() != LINK_ID_ALL);

	bool is_alloc_found = false;
	alloc_list_t& alloc_list =
		_alloc_list[_status2idx(alloc_to_rm.status())];
	for (alloc_list_t::iterator it = alloc_list.begin();
			it != alloc_list.end(); it++) {
		const Alloc* alloc = *it;
		assert(alloc->peer());
		if (alloc->peer()->node_id() != alloc_to_rm.peer()->node_id()
				|| alloc->link_id() != alloc_to_rm.link_id())
			continue;
		/*
		 * Recycle the allocation.
		 */
		delete alloc;
		alloc_list.erase(it--);
		/*
		 * The specified allocation should be unique.
		 */
		assert(!is_alloc_found);
		is_alloc_found = true;
	}
	return is_alloc_found;
}

bool
Data_scheduler::alloc_check_free_recv_grant(
		const Alloc& grant, uint16_t peer_node_id, uint16_t cur_frame)
{
	alloc_status_t	status_ignored =
		static_cast<alloc_status_t>(
				SENT_NET_ENTRY |
				RECV_NET_ENTRY |
				SENT_AVAILABILITY |
				RECV_AVAILABILITY |
				RECV_CONFIRM |
				NBR_BUSY_XMIT |
				NBR_BUSY_RECV);

	return _alloc_check_free(
			grant, peer_node_id, cur_frame, status_ignored);
}

bool
Data_scheduler::alloc_check_free_recv_confirm(
		const Alloc& confirm, uint16_t peer_node_id, uint16_t cur_frame)
{
	alloc_status_t	status_ignored =
		static_cast<alloc_status_t>(
				SENT_NET_ENTRY |
				RECV_NET_ENTRY |
				SENT_AVAILABILITY |
				RECV_AVAILABILITY |
				NBR_BUSY_XMIT |
				NBR_BUSY_RECV);

	return _alloc_check_free(
			confirm, peer_node_id, cur_frame, status_ignored);
}

/*
 * Following function is used to register a new allocation with the scheduler.
 * The status of the allocation can only be changed in this function.
 */

void
Data_scheduler::alloc_register(Alloc& alloc, alloc_status_t status,
		const Neighbor* peer, uint8_t link_id)
{
	assert(alloc.status() == INIT);

	if (link_id == LINK_ID_ALL) {

		Mesh_link* link;
		if (status & STATUS_SENT)
			link = peer?peer->rx_link():NULL;
		else if (status & STATUS_RECV)
			link = peer?peer->tx_link():NULL;
		else
			link = NULL;

		link_id = link?link->id():LINK_ID_ALL;
	}

	alloc.set_status(status);
	alloc.set_peer(peer);
	alloc.set_link_id(link_id);

	_alloc_list[_status2idx(status)].push_back(new Alloc(alloc));
#if 0
	/*
	 * Allocate a new allocation with status INIT.
	 * If the status is not INIT, we just put the allocation into the list.
	 * XXX: The caller should remove the allocation from the original list
	 *      by itself if it should be done.
	 */
	if (status_org == INIT)
		_alloc_list[_status2idx(status)].push_back(new class Alloc(alloc));
	else
		_alloc_list[_status2idx(status)].push_back(&alloc);
#endif
}

const Data_scheduler::Alloc*
Data_scheduler::pop_head_job()
{
	if (_job_list.empty())
		return NULL;

	const Alloc* job = _job_list.begin()->second;
	_job_list.erase(_job_list.begin());
	return job;
}

void
Data_scheduler::alloc_list_dump(alloc_status_t status) const
{
	alloc_list_t list_to_dump;

	if (!alloc_find(list_to_dump, status))
		return;

	STAT("[%03u] \e[1;33m%s: (size = %u)\e[m\n",
			_mac->node_id(), __FUNCTION__, list_to_dump.size());

	for (alloc_list_t::const_iterator it = list_to_dump.begin();
			it != list_to_dump.end(); it++)
		(*it)->dump();
}

void
Data_scheduler::job_list_dump() const
{
	if (_job_list.empty())
		return;

	STAT("[%03u] \e[1;33m%s: (size = %u)\e[m\n",
			_mac->node_id(), __FUNCTION__, _job_list.size());

	for (job_list_t::const_iterator it = _job_list.begin();
			it != _job_list.end(); it++)
		it->second->dump();
}

void
Data_scheduler::bitmap_dump() const
{
	_bitmap->dump();
}


int
Data_scheduler::update_pseudo_schedule_index(size_t nf_tx_link)
{
	if (_pseudo_schedule_flag) {
		_pseudo_peer_conn_list_size = nf_tx_link;
		_pseudo_peer_conn_list_index += 1;
		if (_pseudo_peer_conn_list_index > _pseudo_peer_conn_list_size)
			_pseudo_peer_conn_list_index = 1;
	}

	return 1;
}

bool
Data_scheduler::is_pseudo_schedule_skip(bool is_pending_data, int cnt)
{
	if (_pseudo_schedule_flag) {
		/*
		 * C.C. Lin: Do here: pseudo-scheduler
		 * we should generate many schedule requests
		 * with a moderate rate.
		 */
		if (!is_pending_data && cnt == _pseudo_peer_conn_list_index)
			return false;
	}

	return !is_pending_data;
}

void
Data_scheduler::record_request_time(
		class Alloc& avail, uint32_t tx_opp)
{
	_statistic_list->record_request_time(&avail, GetCurrentTime(), tx_opp);
}

void
Data_scheduler::record_confirm_time(
		class Alloc& avail, uint32_t tx_opp)
{
	_statistic_list->record_confirm_time(&avail, GetCurrentTime(), tx_opp);
}

void
Data_scheduler::dump_statistics() const
{
	ASSERT(_statistic_list, "[%u] %s: statistic_list is null.\n",
			_mac->node_id(), __FUNCTION__);

	_statistic_list->dump();
}

void
Data_scheduler::auto_upgrade_pregrants(uint16_t peer_nid)
{
	schedule_alloc_list_t& grant_list =
			alloc_list(SENT_GRANT);

	/*
	 * Remove the 'SENT_GRANT' alloc, and add the 'RECV_CONFIRM' alloc
	 * by the original one. Thus, we upgrade the alloc immediately.
	 */
	for (alloc_list_t::iterator grant_it = grant_list.begin();
			grant_it != grant_list.end(); grant_it++) {

		const Alloc& grant = **grant_it;

		if (grant.peer()->node_id() != peer_nid)
			continue;

		schedule_alloc_t confirm(grant.frame_start(), grant.frame_range(),
					grant.slot_start(), grant.slot_range());

		alloc_register(confirm, RECV_CONFIRM, grant.peer());

		INFO("%11.7f: [%03u] %s: \e[1;36mThe SENT_GRANT alloc upgrade "
				"immediately to RECV_CONFIRM one.\e[m\n",
				(double)GetCurrentTime() / 10000000,
				_mac->node_id(), __FUNCTION__);

		grant.dump("Original grant");
		confirm.dump("Upgrading confirm");

		delete *grant_it;
		grant_list.erase(grant_it--);

	}
}

uint16_t
Data_scheduler::_nf_holdoff_frame() const
{
	return 1 +
		_mac->dsch_scheduler()->ht_assigner()->sch_info()->tx_holdoff_opp() /
		_mac->fr_mngr()->nf_tx_opp();
}

size_t
Data_scheduler::_alloc_find(
		alloc_list_t& alloc_list, alloc_list_idx_t idx) const
{
	for (alloc_list_t::const_iterator it = _alloc_list[idx].begin();
			it != _alloc_list[idx].end(); it++)
		alloc_list.push_back(*it);
	return _alloc_list[idx].size();
}

size_t
Data_scheduler::_alloc_find(alloc_list_t& alloc_list,
		alloc_list_idx_t idx, uint16_t node_id, uint8_t link_id) const
{
	size_t      append_cnt = 0;

	if (link_id == LINK_ID_ALL) {
		for (alloc_list_t::const_iterator it = _alloc_list[idx].begin();
				it != _alloc_list[idx].end(); it++) {
			const Alloc& alloc = **it;
			if ((alloc.peer()?alloc.peer()->node_id():NEW_NODE_ID)
					== node_id) {
				alloc_list.push_back(*it);
				append_cnt++;
			}
		}
	} else {
		for (alloc_list_t::const_iterator it = _alloc_list[idx].begin();
				it != _alloc_list[idx].end(); it++) {
			const Alloc& alloc = **it;
			if ((alloc.peer()?alloc.peer()->node_id():NEW_NODE_ID)
					== node_id &&
					alloc.link_id() == link_id) {
				alloc_list.push_back(*it);
				append_cnt++;
			}
		}
	}
	return append_cnt;
}

size_t
Data_scheduler::_alloc_remove(const alloc_list_idx_t idx,
		const uint16_t node_id, const uint8_t link_id)
{
	size_t nf_removed_alloc = 0;

	alloc_list_t& alloc_list = _alloc_list[idx];
	if (link_id == LINK_ID_ALL) {
		for (alloc_list_t::iterator it = alloc_list.begin();
				it != alloc_list.end(); it++) {
			const Alloc& alloc = **it;
			if ((alloc.peer()?alloc.peer()->node_id():NEW_NODE_ID)
					== node_id) {

				VINFO("%11.7f: [%03u] %s: ",
						(double)GetCurrentTime() / 10000000,
						_mac->node_id(), __FUNCTION__);
				VINFO_FUNC((alloc.dump("Removed allocation:")));
				/*
				 * Recycle the allocation.
				 */
				delete *it;
				alloc_list.erase(it--);
				nf_removed_alloc++;
			}
		}
	} else {
		for (alloc_list_t::iterator it = alloc_list.begin();
				it != alloc_list.end(); it++) {
			const Alloc& alloc = **it;
			if ((alloc.peer()?alloc.peer()->node_id():NEW_NODE_ID)
					== node_id &&
					alloc.link_id() == link_id) {

				VINFO("%11.7f: [%03u] %s: ",
						(double)GetCurrentTime() / 10000000,
						_mac->node_id(), __FUNCTION__);
				VINFO_FUNC(alloc.dump("Removed allocation:"));
				/*
				 * Recycle the allocation.
				 */
				delete *it;
				alloc_list.erase(it--);
				nf_removed_alloc++;
			}
		}
	}
	return nf_removed_alloc;
}

bool
Data_scheduler::_alloc_check_free(const Alloc& alloc,
		uint16_t peer_node_id, uint16_t cur_frame,
		alloc_status_t status_ignored)
{
	if (alloc == 0)
		return true;

#if 0
	printf("[%03u] %s: Bitmap initial, frame_base = %#x\n",
			_mac->node_id(), __FUNCTION__, alloc.frame_start());
	printf("[%03u] %s: alloc:\n", _mac->node_id(), __FUNCTION__);
	alloc.dump();
#endif
	/*
	 * If the allocation to be checked had been active, use the frame
	 * start time of the current frame as the base frame for the bitmap.
	 */
	Alloc_base dup_alloc = alloc;
	if (alloc.is_active(cur_frame)) {
		if (!alloc.frame_remain(cur_frame))
			return true;

		dup_alloc.frame_start(cur_frame);
		dup_alloc.frame_range(alloc.frame_remain(cur_frame));
	}
	_bitmap->init(dup_alloc.frame_start());
#if 0
	printf("[%03u] %s: dump dup_alloc after calling init\n", _mac->node_id(),
			__FUNCTION__);
	dup_alloc.dump();
#endif
	_bitmap->set_boundary(
			dup_alloc.frame_start(), dup_alloc.slot_start(),
			dup_alloc.frame_range(), dup_alloc.slot_range());
	_bitmap_cumulate(static_cast<alloc_status_t>(
				STATUS_ALL_WITHOUT_REQUEST & ~status_ignored));
#if 0
	printf("[%03u] %s: \e[1;36malloc_list_dump:\e[m\n",
			_mac->node_id(), __FUNCTION__);
	alloc_list_dump(static_cast<alloc_status_t>(
				STATUS_ALL_WITHOUT_REQUEST & ~status_ignored));
#endif
	/*
	 * The availability sent to the peer node can't be ignored.
	 */
	alloc_list_t& sent_avail_list = alloc_list(SENT_AVAILABILITY);
	for (alloc_list_t::iterator it = sent_avail_list.begin();
			it != sent_avail_list.end(); it++) {
		const Alloc& sent_avail = **it;
		assert(sent_avail.peer());
		if (sent_avail.peer()->node_id() != peer_node_id) {
			_bitmap->cumulate(**it);
		}
	}
	Alloc_base dummy;
	if (_bitmap->get_free_alloc(dummy, Bitmap::MINISLOT,
				dup_alloc.frame_range(),
				dup_alloc.slot_range())) {
		if (dummy.area() == dup_alloc.area())
			return true;
		else {
			ERROR("%11.7f: [%03u] %s: Dummy does not match alloc, "
						"dump dummy and alloc: "
						"(cur_frame = %#x)\n",
						(double)GetCurrentTime() / 10000000,
						_mac->node_id(), __FUNCTION__, cur_frame);
			ERROR_FUNC(dummy.dump("dummy:"));
			ERROR_FUNC(dup_alloc.dump("dup_alloc:"));
		}
	}
	return false;
}

void
Data_scheduler::_bitmap_cumulate(alloc_status_t status)
{
	if (status & INIT)
		assert(0);

	if (status & SENT_NET_ENTRY)
		_bitmap_cumulate(IDX_SENT_NET_ENTRY);

	if (status & SENT_AVAILABILITY)
		_bitmap_cumulate(IDX_SENT_AVAILABILITY);

	if (status & SENT_REQUEST)
		assert(0);

	if (status & SENT_GRANT)
		_bitmap_cumulate(IDX_SENT_GRANT);

	if (status & SENT_CONFIRM)
		_bitmap_cumulate(IDX_SENT_CONFIRM);

	if (status & RECV_NET_ENTRY)
		_bitmap_cumulate(IDX_RECV_NET_ENTRY);

	if (status & RECV_AVAILABILITY)
		_bitmap_cumulate(IDX_RECV_AVAILABILITY);

	if (status & RECV_REQUEST)
		assert(0);

	if (status & RECV_GRANT)
		_bitmap_cumulate(IDX_RECV_GRANT);

	if (status & RECV_CONFIRM)
		_bitmap_cumulate(IDX_RECV_CONFIRM);

	if (status & NBR_BUSY_XMIT)
		_bitmap_cumulate(IDX_NBR_BUSY_XMIT);

	if (status & NBR_BUSY_RECV)
		_bitmap_cumulate(IDX_NBR_BUSY_RECV);
}

void
Data_scheduler::_bitmap_cumulate(alloc_list_idx_t idx)
{
	assert(idx != IDX_SENT_REQUEST &&
			idx != IDX_RECV_REQUEST &&
			idx != IDX_DUMMY_END);

	for (alloc_list_t::iterator it = _alloc_list[idx].begin();
			it != _alloc_list[idx].end(); it++)
		_bitmap->cumulate(**it);
}

Data_scheduler::alloc_list_idx_t
Data_scheduler::_status2idx(alloc_status_t status) const
{
	switch (status) {
		case SENT_NET_ENTRY:	return IDX_SENT_NET_ENTRY;
		case SENT_AVAILABILITY:	return IDX_SENT_AVAILABILITY;
		case SENT_REQUEST:	return IDX_SENT_REQUEST;
		case SENT_GRANT:	return IDX_SENT_GRANT;
		case SENT_CONFIRM:	return IDX_SENT_CONFIRM;
		case RECV_NET_ENTRY:	return IDX_RECV_NET_ENTRY;
		case RECV_AVAILABILITY:	return IDX_RECV_AVAILABILITY;
		case RECV_REQUEST:	return IDX_RECV_REQUEST;
		case RECV_GRANT:	return IDX_RECV_GRANT;
		case RECV_CONFIRM:	return IDX_RECV_CONFIRM;
		case NBR_BUSY_XMIT:	return IDX_NBR_BUSY_XMIT;
		case NBR_BUSY_RECV:	return IDX_NBR_BUSY_RECV;
		default: assert(0);
	}
}

/*
 * Judge the 'NENT allocation' specified wheather used or not.
 * If used, then return false; otherwise, it must be inserted to
 * 'NetEntry alloc_list', then return true.
 */
bool
Data_scheduler::_set_nent_alloc(bool busy_consider, class Neighbor* peer,
		Alloc& sch_alloc, uint32_t currFrameNum)
{
	/*
	 * Init bitmap table, and set bounary searched.
	 * If busy_consider is '1', then we must consider busy status,
	 * when we generate bitmap; otherwise, no consideration of busy status
	 * is needed.
	 */
	_bitmap->init(sch_alloc.frame_start());

	if (busy_consider)
		_bitmap_cumulate(STATUS_ALL_WITHOUT_REQUEST);
	else
		_bitmap_cumulate(static_cast<alloc_status_t>(
					STATUS_ALL_WITHOUT_REQUEST &
					STATUS_ALL_WITHOUT_NBR_BUSY));

	_bitmap->set_boundary(sch_alloc.frame_start(), sch_alloc.slot_start(),
			sch_alloc.frame_range(), sch_alloc.slot_range());

	/*
	 * If the 'NENT allocation' is not found, then return NULL.
	 */
	Alloc nent_alloc;
	bool is_free = true;
	if (!(_bitmap->get_free_alloc(nent_alloc, Bitmap::MINISLOT,
					Alloc_base::FRAME_RANGE_PERSISTENCE,
					sch_alloc.slot_range()))) {
		VINFO("%s: \e[1;36mDump allocaton list:\e[m\n", __FUNCTION__);
		VINFO_FUNC(alloc_list_dump(STATUS_ALL));
		is_free = false;
	}
	/*
	 * FIXME? We register the allocation without any further checking.
	 * Add the 'NENT allocation' to 'NetEntry alloction_list', and
	 * the list will set the peer and status of the allocation.
	 */
	alloc_register(sch_alloc, RECV_NET_ENTRY, peer);
	VINFO("%11.7f: [%03u] %s: \e[0;31mSet nent alloc received:\e[m\n",
				(double)GetCurrentTime() / 10000000,
				_mac->node_id(), __FUNCTION__);
	VINFO_FUNC(sch_alloc.dump());

	return is_free;
}

size_t
Data_scheduler::_remove_expired_alloc(alloc_status_t status,
		uint16_t cur_frame, uint16_t negotiation_timeout)
{
	size_t nf_removed_alloc = 0;
	alloc_list_t& alloc_to_check = alloc_list(status);
	uint16_t frame_expired =
		(cur_frame - negotiation_timeout) & Alloc_base::FRAME_BIT_MASK;

	for (alloc_list_t::iterator it = alloc_to_check.begin();
			it != alloc_to_check.end(); it++) {

		const Alloc& alloc = **it;
		if (!alloc.is_active(frame_expired) ||
				alloc.frame_remain(frame_expired))
			continue;

		if (negotiation_timeout) {
			WARN("%11.7f: [%03u] %s: [WARN] \e[1;31mRemove the allocation "
					"due to negotiation time out.\e[m (frame_expired = %#x)\n",
					(double)GetCurrentTime() / 10000000,
					_mac->node_id(), __FUNCTION__, frame_expired);
			WARN_FUNC(alloc.dump());
		} else {
			VINFO("%11.7f: [%03u] %s: Remove the expired allocation: "
					"(frame_expired = %#x)\n",
					(double)GetCurrentTime() / 10000000,
					_mac->node_id(), __FUNCTION__, frame_expired);
			VINFO_FUNC(alloc.dump());
		}

		if (status == SENT_AVAILABILITY) {
			WARN("%11.7f: [%03u] %s: [WARN] \e[1;31mRemove the request "
					"corresponding to the expired availability.\e[m "
					"(cur_frame = %#x)\n",
					(double)GetCurrentTime() / 10000000,
					_mac->node_id(), __FUNCTION__,
					cur_frame);
			assert(_alloc_remove(IDX_SENT_REQUEST,
						alloc.peer()->node_id()) == 1);
		}

		/*
		 * Recycle the allocation due to its expiration.
		 */
		delete *it;
		alloc_to_check.erase(it--);
		nf_removed_alloc++;
	}
	return nf_removed_alloc;
}


/*
 * Member function definitions of class `Data_scheduler::Alloc'.
 */

/*
 * Constructor
 */
Data_scheduler::Alloc::Alloc()
: Alloc_base()
, _peer(NULL)
, _link_id(LINK_ID_ALL)
, _status(INIT)
{
}

Data_scheduler::Alloc::Alloc(const class Alloc& alloc)
: Alloc_base(alloc)
, _peer(alloc._peer)
, _link_id(alloc._link_id)
, _status(alloc._status)
, _is_path_control(alloc._is_path_control)
, _connection(alloc._connection)
{
}

Data_scheduler::Alloc::Alloc(uint16_t frame_start, uint8_t frame_range,
		uint8_t slot_start, uint8_t slot_range, uint8_t link_id)
: Alloc_base(frame_start, frame_range, slot_start, slot_range)
, _peer(NULL)
, _link_id(link_id)
, _status(INIT)
, _is_path_control(false)
, _connection(NULL)
{
}

/*
 * Destructor
 */
Data_scheduler::Alloc::~Alloc()
{
}

void
Data_scheduler::Alloc::dump(const char* tag) const
{
	Alloc_base::dump(tag);
	printf("\tPeer Node ID: %04u, link ID: %03u, status: ",
			_peer?_peer->node_id():0, _link_id);
	switch (_status) {
	case INIT:		printf("\e[1;37mINIT\e[m\n");			break;
	case SENT_NET_ENTRY:	printf("\e[1;31mSENT_NET_ENTRY\e[m\n");		break;
	case SENT_AVAILABILITY:	printf("\e[1;32mSENT_AVAILABILITY\e[m\n");	break;
	case SENT_REQUEST:	printf("\e[1;33mSENT_REQUEST\e[m\n");		break;
	case SENT_GRANT:	printf("\e[1;34mSENT_GRANT\e[m\n");		break;
	case SENT_CONFIRM:	printf("\e[1;35mSENT_CONFIRM\e[m\n");		break;
	case RECV_NET_ENTRY:	printf("\e[0;31mRECV_NET_ENTRY\e[m\n");		break;
	case RECV_AVAILABILITY:	printf("\e[0;32mRECV_AVAILABILITY\e[m\n");	break;
	case RECV_REQUEST:	printf("\e[0;33mRECV_REQUEST\e[m\n");		break;
	case RECV_GRANT:	printf("\e[0;34mRECV_GRANT\e[m\n");		break;
	case RECV_CONFIRM:	printf("\e[0;35mRECV_CONFIRM\e[m\n");		break;
	case NBR_BUSY_XMIT:	printf("\e[1;36mNBR_BUSY_XMIT\e[m\n");		break;
	case NBR_BUSY_RECV:	printf("\e[0;36mNBR_BUSY_RECV\e[m\n");		break;
	default:		assert(0);
	}
	//printf("\tis_path_control = %u\n", _is_path_control);
	fflush(stdout);
}
