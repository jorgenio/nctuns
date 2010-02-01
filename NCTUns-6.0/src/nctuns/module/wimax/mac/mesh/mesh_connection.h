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

#ifndef __NCTUNS_WIMAX_MESH_CONNECTION_H__
#define __NCTUNS_WIMAX_MESH_CONNECTION_H__


#include <stdint.h>
#include <vector>
#include "../connection.h"


class Mesh_link_tx;

class Mesh_connection : public DataConnection {

private:

	typedef struct mesh_cid {

		uint8_t link_id;

		union {

			uint8_t	logic_network_id;

			struct {
				uint8_t	type		:2;
				uint8_t	reliability	:1;
				uint8_t	priority_class	:3;
				uint8_t	drop_procedence	:2;
			} f;

		} u;
	} mesh_mode_cid_t;

	typedef union {

		uint16_t	scalar;
		mesh_mode_cid_t	structure;

	} cid_t;

public:

	enum {
		ALL_NET_BROADCAST         = 0x00ff,
		TYPE_MAC_MANAGEMENT       = 0x0000,
		TYPE_IP                   = 0x0001,
		RELIABILITY_NO_RE_XMIT    = 0x0000,
		RELIABILITY_4_RE_XMIT     = 0x0001,
		CLASS_GENERAL             = 0x0000,
	};

	typedef enum {

		INIT                  = (0),
		PATH_CONTROL_REQUEST  = (1 << 0),
		PATH_CONTROL_OPEN     = (2 << 0),

	} status_t;

private:

	cid_t                                   _id;
	const Mesh_link_tx*                     _link;
	status_t                                _status;

public:

	Mesh_connection(uint16_t, Mesh_link_tx* link = NULL);
	virtual ~Mesh_connection();

	const class Neighbor* node_dst() const;
	uint16_t node_id_dst() const;
	uint16_t node_id_src() const;

	size_t pending_data_len() const;

	void dump(const char* tag = "") const;

	static uint16_t gen_id(uint8_t, uint8_t, uint8_t, uint8_t, uint8_t);

	inline uint16_t id() const { return _id.scalar; }
	inline uint8_t link_id() const { return _id.structure.link_id; }

	inline uint8_t logic_network_id() const
	{ return _id.structure.u.logic_network_id; }

	inline uint8_t type() const
	{ return _id.structure.u.f.type; }

	inline uint8_t reliability() const
	{ return _id.structure.u.f.reliability; }

	inline uint8_t priority_class() const
	{ return _id.structure.u.f.priority_class; }

	inline uint8_t drop_procedence() const
	{ return _id.structure.u.f.drop_procedence; }

	inline status_t status() const { return _status; }
	inline void status(status_t status) { _status = status; }
};


#endif /* __NCTUNS_WIMAX_MESH_CONNECTION_H__ */
