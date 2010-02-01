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
#include <stdio.h>
#include "mesh_link.h"
#include "mesh_connection.h"
#include "neighbor.h"


/*
 * Member function definitions of class `Mesh_link'.
 */

/*
 * Constructor
 */
Mesh_link::Mesh_link(uint8_t id, uint16_t dst, uint16_t src)
: _id(id)
, _nid_dst(dst)
, _nid_src(src)
{
}


/*
 * Destructor
 */
Mesh_link::~Mesh_link()
{
}


/*
 * Member function definitions of class `Mesh_link_tx'.
 */

/*
 * Constructor
 */
Mesh_link_tx::Mesh_link_tx(uint8_t id, const Neighbor* dst, uint16_t src)
: Mesh_link(id, dst?dst->node_id():0, src)
, _node_dst(dst)
{
	assert(dst);
}


/*
 * Destructor
 */
Mesh_link_tx::~Mesh_link_tx()
{
	/*
	 * Recycle data connections.
	 */
	for (connection_t::iterator it = _connections.begin();
			it != _connections.end(); it++)
		delete it->second;
}


/*
 * Get a connection in the link via the specified ID.
 * - cid: The ID for the connection.
 * - return value: The pointer of the specified connection. NULL if
 *                 the specified connection does not exist.
 */
Mesh_connection*
Mesh_link_tx::connection(uint16_t cid) const
{
	connection_t::const_iterator it = _connections.find(cid);

	if (it == _connections.end())
		return NULL;

	return it->second;
}


/*
 * Create a connection with specified ID upon the link.
 * - cid: The ID for the connection.
 * - return value: The pointer of the created connection. NULL if
 *                 the specified connection ID was used.
 */
Mesh_connection*
Mesh_link_tx::create_connection(uint16_t cid)
{
	if (_connections.find(cid) == _connections.end()) {
		_connections[cid] = new Mesh_connection(cid, this);
		return _connections[cid];
	}
	return NULL;
}


/*
 * Encapsulate all data in the every connections of the link
 * into the specified buffer.
 * - buf: The buffer for encapsulate the data.
 * - maxlen: The maximum length allowed to encapsulate.
 * - return value: The length of actually encapsulated data.
 */
size_t
Mesh_link_tx::encapsulate(uint8_t* buf, size_t maxlen)
{
	unsigned int	sendlen = 0;

	for (connection_t::iterator it = _connections.begin();
			it != _connections.end(); it++) {

		Mesh_connection* connection = it->second;
		sendlen += connection->EncapsulateAll(
				reinterpret_cast<char*>(&buf[sendlen]),
				maxlen - sendlen);
	}
	return sendlen;
}


/*
 * Compute the length of data waiting to be sent in the connections of the link.
 * - return value: The length of the summation of all pending data.
 */
size_t
Mesh_link_tx::pending_data_len() const
{
	size_t	len = 0;

	for (connection_t::const_iterator it = _connections.begin();
			it != _connections.end(); it++)
		len += it->second->pending_data_len();

	return len;
}


/*
 * Compute the number of packets waiting to be sent in the connections of the link.
 * - return value: The number of packets of all pending packets.
 */
size_t
Mesh_link_tx::nf_pending_packet() const
{
	size_t	nf_pkt = 0;

	for (connection_t::const_iterator it = _connections.begin();
			it != _connections.end(); it++)
		nf_pkt += it->second->nf_pending_packet();

	return nf_pkt;
}


/*
 * TODO
 */
void
Mesh_link_tx::dump() const
{
	printf("Mesh_link_tx::\e[1;33m%s:\e[m "
			"ID: %#06x, dst: %03u, src: %03u, %u connections\n",
			__FUNCTION__, id(),
			node_id_dst(), node_id_src(), _connections.size());

	for (connection_t::const_iterator it = _connections.begin();
			it != _connections.end(); it++)
		it->second->dump();
}


/*
 * Member function definitions of class `Mesh_link_rx'.
 */

/*
 * Constructor
 */
Mesh_link_rx::Mesh_link_rx(uint8_t id, uint16_t dst, uint16_t src)
: Mesh_link(id, dst, src)
{
}


/*
 * Destructor
 */
Mesh_link_rx::~Mesh_link_rx()
{
	/*
	 * Recycle connection receiver.
	 */
	for (connection_receiver_t::iterator it = _connections.begin();
			it != _connections.end(); it++)
		delete it->second;
}


/*
 * Get a connection in the link via the specified ID.
 * - cid: The ID for the connection.
 * - return value: The pointer of the specified connection. NULL if
 *                 the specified connection does not exist.
 */
ConnectionReceiver*
Mesh_link_rx::connection(uint16_t cid)
{
	connection_receiver_t::const_iterator it = _connections.find(cid);

	if (it == _connections.end())
		return NULL;

	return it->second;
}


/*
 * Create a connection with specified ID upon the link.
 * - cid: The ID for the connection.
 * - return value: The pointer of the created connection. NULL if
 *                 the specified connection ID was used.
 */
ConnectionReceiver*
Mesh_link_rx::create_connection(uint16_t cid)
{
	if (_connections.find(cid) == _connections.end()) {
		_connections[cid] =
			new ConnectionReceiver(cid, _nid_src, _nid_dst);
		return _connections[cid];
	}
	return NULL;
}
