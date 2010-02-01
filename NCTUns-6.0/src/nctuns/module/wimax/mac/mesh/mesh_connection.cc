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

#include "mesh_connection.h"

#include <cassert>
#include <cstdio>
#include "mesh_link.h"


/*
 * Member function definitions of class `Mesh_connection'.
 */

/*
 * Constructor
 */
Mesh_connection::Mesh_connection(uint16_t id, Mesh_link_tx* link)
: DataConnection(id, true)
, _link(link)
, _status(INIT)
{
    _id.scalar = id;

    if (link) {

        assert(link->id() == _id.structure.link_id);
        SetDstNodeID(link->node_id_dst());
        SetSrcNodeID(link->node_id_src());

    }

}


/*
 * Destructor
 */
Mesh_connection::~Mesh_connection()
{
}


/*
 * Get the pointer of peer node via the link to which the connection belongs.
 * - return value: The pointer of the peer node.
 */
const class Neighbor*
Mesh_connection::node_dst() const
{
	return _link?_link->node_dst():0;
}


/*
 * Get the peer node ID via the link to which the connection belongs.
 * - return value: The peer node ID.
 */
uint16_t
Mesh_connection::node_id_dst() const
{
	return _link?_link->node_id_dst():0;
}


/*
 * Get the node ID of the node itself via
 * the link to which the connection belongs.
 * - return value: The node ID.
 */
uint16_t
Mesh_connection::node_id_src() const
{
	return _link?_link->node_id_src():0;
}


/*
 * Compute the length of data waiting to be sent in the connection.
 * - return value: The length all pending data.
 */
size_t
Mesh_connection::pending_data_len() const
{
	return GetInformation();
}


/*
 * Dump the information of the connection.
 */
void
Mesh_connection::dump(const char* tag) const
{
	printf("Mesh_connection::%s: %s\n"
			"\tID: %#06x, ", __FUNCTION__, tag, id());

	if (link_id() == ALL_NET_BROADCAST)
		printf("Logic Network ID: %#04x\n", logic_network_id());
	else {
		printf("link ID: %#04x\n", link_id());
		printf("\ttype: %#x, reliability: %#x, class: %#x, drop: %#x\n",
			type(), reliability(),
			priority_class(), drop_procedence());
	}
	printf("\tPending data length: %u, status = ", pending_data_len());
	switch (_status) {
		case INIT:			printf("INIT");			break;
		case PATH_CONTROL_REQUEST:	printf("PATH_CONTROL_REQUEST");	break;
		case PATH_CONTROL_OPEN:		printf("PATH_CONTROL_OPEN");	break;
		default: assert(0);
	}
	printf("\n");
}


/*
 * Generate connection ID from the given information.
 * - link_id: The link ID.
 * - type: The type of the connection.
 * - reliability: The retransmission scheme of the connection.
 * - priority_class: The priority or class of the connection.
 * - drop_procedence: Messages with larger Drop Precedence shall have higher
 *                    dropping likelihood during congestion.
 * - return value: The generated connection ID.
 */
uint16_t
Mesh_connection::gen_id(uint8_t link_id, uint8_t type, uint8_t reliability,
		uint8_t priority_class, uint8_t drop_procedence)
{
	cid_t cid;

	cid.structure.link_id			= link_id;
	cid.structure.u.f.type			= type;
	cid.structure.u.f.reliability		= reliability;
	cid.structure.u.f.priority_class	= priority_class;
	cid.structure.u.f.drop_procedence	= drop_procedence;

	return cid.scalar;
}
