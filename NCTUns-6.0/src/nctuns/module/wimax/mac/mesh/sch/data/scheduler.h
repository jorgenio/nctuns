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

#ifndef __NCTUNS_WIMAX_MESH_DATA_SCHEDULER_H__
#define __NCTUNS_WIMAX_MESH_DATA_SCHEDULER_H__


#include <stdint.h>
#include <list>
#include <map>
#include "alloc_base.h"
#include "../../mac802_16_mesh_mode_sys_param.h"


class Bitmap;
class mac802_16_MeshSS;
class Mesh_connection;
class Neighbor;
typedef class ScheduleAllocInfoList sched_alloc_stat_list_t;


class Data_scheduler {

public:
	class Alloc;

	typedef std::list<Alloc*> alloc_list_t;
	typedef std::map<uint8_t, Alloc*> job_list_t;

	typedef enum {
		INIT			= (0),
		SENT_NET_ENTRY		= (1 << 0),
		SENT_AVAILABILITY	= (1 << 1),
		SENT_REQUEST		= (1 << 2),
		SENT_GRANT		= (1 << 3),
		SENT_CONFIRM		= (1 << 4),
		RECV_NET_ENTRY		= (1 << 5),
		RECV_AVAILABILITY	= (1 << 6),
		RECV_REQUEST		= (1 << 7),
		RECV_GRANT		= (1 << 8),
		RECV_CONFIRM		= (1 << 9),
		NBR_BUSY_XMIT		= (1 << 10),
		NBR_BUSY_RECV		= (1 << 11),
		STATUS_ALL			=
			SENT_NET_ENTRY		| RECV_NET_ENTRY	|
			SENT_AVAILABILITY	| RECV_AVAILABILITY	|
			SENT_REQUEST		| RECV_REQUEST		|
			SENT_GRANT		| RECV_GRANT		|
			SENT_CONFIRM		| RECV_CONFIRM		|
			NBR_BUSY_XMIT		| NBR_BUSY_RECV,
		STATUS_SENT			=
			SENT_NET_ENTRY		|
			SENT_AVAILABILITY	|
			SENT_REQUEST		|
			SENT_GRANT		|
			SENT_CONFIRM,
		STATUS_RECV			=
			RECV_NET_ENTRY		|
			RECV_AVAILABILITY	|
			RECV_REQUEST		|
			RECV_GRANT		|
			RECV_CONFIRM,
		STATUS_ALL_WITHOUT_REQUEST	=
			STATUS_ALL & ~SENT_REQUEST & ~RECV_REQUEST,
		STATUS_ALL_WITHOUT_NBR_BUSY	=
			STATUS_ALL & ~NBR_BUSY_XMIT & ~NBR_BUSY_RECV,
	} alloc_status_t;

	enum {
		AVAILABILITY_SIZE		= 40,
		REQUEST_SIZE			= 20,
		SCHEDULE_VALIDITY		= 128,
		NEW_NODE_ID		    	= 0,
	};

private:
	typedef enum alloc_list_idx {
		IDX_SENT_NET_ENTRY = 0,
		IDX_SENT_AVAILABILITY,
		IDX_SENT_REQUEST,
		IDX_SENT_GRANT,
		IDX_SENT_CONFIRM,
		IDX_RECV_NET_ENTRY,
		IDX_RECV_AVAILABILITY,
		IDX_RECV_REQUEST,
		IDX_RECV_GRANT,
		IDX_RECV_CONFIRM,
		IDX_NBR_BUSY_XMIT,
		IDX_NBR_BUSY_RECV,
		IDX_DUMMY_END
	} alloc_list_idx_t;

	enum {
		ALLOC_GRANT_MIN	= 0,
		LINK_ID_ALL	= 0x00ff,
		FR_NEGOTIATION	= 0x100,
	};


private:
	static Bitmap*		_bitmap;
	static size_t		_bitmap_ref_cnt;

	const mac802_16_MeshSS*	_mac;
	alloc_list_t		_alloc_list[IDX_DUMMY_END];
	job_list_t		_job_list;

public:
	explicit Data_scheduler(const mac802_16_MeshSS*);
	virtual ~Data_scheduler();

	void start();

	inline const mac802_16_MeshSS* mac() const { return _mac; }
	inline const job_list_t& job_list() const { return _job_list; }

	alloc_list_t& alloc_list(alloc_status_t);
	void alloc_list_clear(alloc_status_t);

	Alloc* get_nent_alloc(uint8_t);
	bool free_nent_alloc(uint16_t, alloc_status_t);
	bool set_nent_alloc_and_no_consider_busy_status(
			Neighbor*, Alloc&, uint32_t);
	bool set_nent_alloc_and_consider_busy_status(
			Neighbor*, Alloc&, uint32_t);

	bool get_availability(Alloc&, const Mesh_connection*, uint16_t);
	void get_request(Alloc&, const Mesh_connection*, size_t, uint32_t, uint32_t);
	bool get_grant(Alloc&, const Alloc&, const Alloc&, uint16_t, size_t);
	bool check_and_remove_request_info(Alloc& grant, uint16_t, bool is_last_grant);
	bool check_and_sync_confirm_with_avail(Alloc&, uint16_t, bool is_last_confirm);
	bool check_and_sync_confirm_with_grant(Alloc&, uint16_t);
	void remove_expired_alloc(uint16_t);
	size_t refresh_job_list();

	size_t alloc_find(alloc_list_t&, alloc_status_t) const;
	size_t alloc_find(alloc_list_t&, alloc_status_t, uint16_t,
			uint8_t link_id = LINK_ID_ALL) const;
	bool alloc_remove(const Alloc&);
#if 0
	bool alloc_remove_matched_container(
			const Alloc&, alloc_status_t, uint8_t);
#endif
	bool alloc_check_free_recv_grant(const Alloc&, uint16_t, uint16_t);
	bool alloc_check_free_recv_confirm(const Alloc&, uint16_t, uint16_t);
	void alloc_register(Alloc&, alloc_status_t,
			const Neighbor* peer = NULL,
			uint8_t link_id = LINK_ID_ALL);

	const Alloc* pop_head_job();

	void alloc_list_dump(alloc_status_t) const;
	void job_list_dump() const;
	void bitmap_dump() const;

private:
	uint16_t _nf_holdoff_frame() const;

	size_t _alloc_find(alloc_list_t&, alloc_list_idx_t) const;
	size_t _alloc_find(alloc_list_t&, alloc_list_idx_t, uint16_t,
			uint8_t link_id = LINK_ID_ALL) const;
	size_t _alloc_remove(const alloc_list_idx_t,
			const uint16_t, const uint8_t link_id = LINK_ID_ALL);
	bool _alloc_check_free(const Alloc&,
			uint16_t, uint16_t, alloc_status_t);

	void _bitmap_cumulate(alloc_status_t);
	void _bitmap_cumulate(alloc_list_idx_t);

	alloc_list_idx_t _status2idx(alloc_status_t) const;
	bool _set_nent_alloc(bool, Neighbor*, Alloc&, uint32_t);
	size_t _remove_expired_alloc(alloc_status_t,
			uint16_t, uint16_t negotiation_timeout = 0);

	/*
	 * Entries used for statistic.
	 */
private:
	sched_alloc_stat_list_t*	_statistic_list;

	int				_pseudo_schedule_flag;
	int				_pseudo_peer_conn_list_index;
	int				_pseudo_peer_conn_list_size;

public:
	inline void set_forced_request() { _pseudo_schedule_flag = 1; }
	inline void pseudo_schedule_off() { _pseudo_schedule_flag = 0; }
	int update_pseudo_schedule_index(size_t);
	bool is_pseudo_schedule_skip(bool, int);
	void record_request_time(Alloc&, uint32_t);
	void record_confirm_time(Alloc&, uint32_t);
	void dump_statistics() const;

	void auto_upgrade_pregrants(uint16_t peer_nid);
#if 0
	double    get_total_dsch_three_handshake_times() { return total_dsch_three_handshake_times;}
	u_int32_t get_num_of_dsch_handshakes() { return num_of_dsch_handshakes;}
	int is_request_rejected(MinislotAlloc* grant_p);
#endif
};

typedef Data_scheduler::alloc_list_t schedule_alloc_list_t;
typedef Data_scheduler::job_list_t schedule_job_list_t;

typedef class Data_scheduler::Alloc : public Alloc_base {

private:
	const Neighbor*	_peer;
	uint8_t		_link_id;
	alloc_status_t	_status;

private:
	inline void set_peer(const Neighbor* peer) { _peer = peer; }
	inline void set_status(alloc_status_t status) { _status = status; }

public:

	Alloc();
	Alloc(const Alloc&);
	Alloc(uint16_t, uint8_t, uint8_t, uint8_t,
			uint8_t link_id = LINK_ID_ALL);
	virtual ~Alloc();

	inline const Neighbor* peer() const { return _peer; }
	inline alloc_status_t status() const { return _status; }
	inline uint8_t link_id() const { return _link_id; }
	inline void set_link_id(uint8_t id) { _link_id = id; }

	void dump(const char* tag = "") const;

	friend void Data_scheduler::alloc_register(
			Alloc&, alloc_status_t, const Neighbor*, uint8_t);
	/*
	 * FIXME: This implementation is temporarily, it must be done in
	 *        another way.
	 */
private:

	bool			_is_path_control;
	const Mesh_connection*	_connection;

public:

	inline void path_control_set() { _is_path_control = true; }
	inline void path_control_clr() { _is_path_control = false; }
	inline bool is_path_control() const { return _is_path_control; }
	inline void hook_connection(const Mesh_connection* connection) { _connection = connection; }
	inline const Mesh_connection* connection() const { return _connection; }

} schedule_alloc_t;


#endif /* __NCTUNS_WIMAX_MESH_DATA_SCHEDULER_H__ */
