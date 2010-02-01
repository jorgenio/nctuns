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

#ifndef __NCTUNS_WIMAX_MESH_LINK_H__
#define __NCTUNS_WIMAX_MESH_LINK_H__


#include <ext/hash_map>
#include <stdint.h>


class ConnectionReceiver;
class Mesh_connection;
class Neighbor;

class Mesh_link {

public:
	enum {
		UNKNOWN_ID	= 0xff,
	};

protected:
	uint8_t		_id;
	uint16_t	_nid_dst;
	uint16_t	_nid_src;

public:
	Mesh_link(uint8_t, uint16_t, uint16_t);
	virtual ~Mesh_link();

	inline uint8_t id() const { return _id; }
	inline uint8_t node_id_dst() const { return _nid_dst; }
	inline uint8_t node_id_src() const { return _nid_src; }
};


class Mesh_link_tx : public Mesh_link {

private:
	typedef __gnu_cxx::hash_map<uint16_t, Mesh_connection*>
		connection_t;

public:
	typedef __gnu_cxx::hash_map<uint8_t, Mesh_link_tx*> hash_t;

private:
	const Neighbor*	_node_dst;
	connection_t		_connections;

public:
	Mesh_link_tx(uint8_t, const Neighbor*, uint16_t);
	~Mesh_link_tx();

	inline const Neighbor* node_dst() const { return _node_dst; }
#if 0
	inline connection_t::const_iterator connection_begin() const
	{ return _connections.begin(); }
	inline connection_t::const_iterator connection_end() const
	{ return _connections.end(); }
#endif

	Mesh_connection* connection(uint16_t) const;
	Mesh_connection* create_connection(uint16_t);
	size_t encapsulate(uint8_t*, size_t);
	size_t pending_data_len() const;
	size_t nf_pending_packet() const;
	void dump() const;
};


class Mesh_link_rx : public Mesh_link {

private:
	typedef __gnu_cxx::hash_map<uint16_t, ConnectionReceiver*>
		connection_receiver_t;

public:
	typedef __gnu_cxx::hash_map<uint8_t, Mesh_link_rx*> hash_t;

private:
	connection_receiver_t	_connections;

public:
	Mesh_link_rx(uint8_t, uint16_t, uint16_t);
	~Mesh_link_rx();

	ConnectionReceiver* connection(uint16_t);
	ConnectionReceiver* create_connection(uint16_t);
};

#endif /* __NCTUNS_WIMAX_MESH_LINK_H__ */
