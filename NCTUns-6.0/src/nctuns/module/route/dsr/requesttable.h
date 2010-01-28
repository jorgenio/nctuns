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

/* [Page 21], DSR Draft, 21 November, 2001
 *
 * 4.3. Route Request Table
 *
 * The Route Request Table of a node implementing DSR records
 * information about Route Requests that have been recently originated
 * or forwarded by this node.  The table is indexed by IP address.
 *
 * The Route Request Table on a node records the following information
 * about nodes to which this node has initiated a Route Request:
 *
 *  -  The Time-to-Live (TTL) field used in the IP header of the Route
 *     Request for the last Route Discovery initiated by this node for
 *     that target node.  This value allows the node to implement a
 *     variety of algorithms for controlling the spread of its Route
 *     Request on each Route Discovery initiated for a target.  As
 *     examples, two possible algorithms for this use of the TTL field
 *     are described in Section 3.3.4.
 *
 *  -  The time that this node last originated a Route Request for that
 *     target node.
 *
 *  -  The number of consecutive Route Discoveries initiated for this
 *     target since receiving a valid Route Reply giving a route to that
 *     target node.
 *
 *  -  The remaining amount of time before which this node MAY next
 *     attempt at a Route Discovery for that target node.  When the
 *     node initiates a new Route Discovery for this target node, this
 *     field in the Route Request Table entry for that target node is
 *     initialized to the timeout for that Route Discovery, after which
 *     the node MAY initiate a new Discovery for that target.  Until
 *     a valid Route Reply is received for this target node address,
 *     a node MUST implement a back-off algorithm in determining this
 *     timeout value for each successive Route Discovery initiated
 *     for this target using the same Time-to-Live (TTL) value in the
 *     IP header of the Route Request packet.  The timeout between
 *     such consecutive Route Discovery initiations SHOULD increase by
 *     doubling the timeout value on each new initiation.
 *
 * In addition, the Route Request Table on a node also records the
 * following information about initiator nodes from which this node has
 * received a Route Request:
 *
 *  -  A FIFO cache of size RequestTableIds entries containing the
 *     Identification value and target address from the most recent
 *     Route Requests received by this node from that initiator node.
 *
 * Nodes SHOULD use an LRU policy to manage the entries in their Route
 * Request Table.
 *
 * The number of Identification values to retain in each Route
 * Request Table entry, RequestTableIds, MUST NOT be unlimited, since,
 * in the worst case, when a node crashes and reboots, the first
 * RequestTableIds Route Discoveries it initiates after rebooting
 * could appear to be duplicates to the other nodes in the network.
 * In addition, a node SHOULD base its initial Identification value,
 * used for Route Discoveries after rebooting, on a battery backed-up
 * clock or other persistent memory device, in order to help avoid
 * any possible such delay in successfully discovering new routes
 * after rebooting; if no such source of initial Identification
 * value is available, a node after rebooting SHOULD base its initial
 * Identification value on a random number.
 */

/*
 * In my implementation, for simplicity, request_number will be add 1
 * after each route request, so Request_Table simply keep the biggest
 * Route Reqeust Number.
 */

#ifndef __NCTUNS_DSR_REQUESTTABLE_H
#define __NCTUNS_DSR_REQUESTTABLE_H

#include <list>
#include "constants.h"
#include "sendbuffer.h"

struct RequestTableEntry {
    u_long                              ip_addr;

    // initiated route request
    u_char		                ip_ttl;
    u_int64_t		                timestamp;
    u_int8_t		                retry_count;
    u_int64_t		                remain_time;

    // received route request
    u_int16_t                           request_number;
    // hhchen: a circular queue is a much suitable data structure...

    RequestTableEntry(): ip_addr( dsr::IP_invalid_addr ), ip_ttl( 0 ),
        timestamp( 0 ), retry_count( 0 ), remain_time( 0 ),
        request_number( 0 ) {}
};

class RequestTable {

private:

    list<RequestTableEntry>             table;
    list<RequestTableEntry>::iterator   it, end;
    u_int64_t                           maxPeriod, period;

    SendBuffer                          *send_buffer;
    u_long                              myIP;

    void find( u_long node ) {
        end = table.end();

        for ( it = table.begin() ; it != end ; ++it ) {
            if ( it->ip_addr == node ) return;
        }
        return;
    }

public:

    RequestTable() {
        SEC_TO_TICK( maxPeriod, dsr::MaxRequestPeriod );
        MILLI_TO_TICK( period, dsr::RequestPeriod );
    }
    
    void init( u_long ip ) {
    	myIP = ip;
    }

    u_int64_t nextAllowedTime( u_long );
    u_int64_t notifyRequestInitiate( u_long, u_char );
    void notifyRouteReply( u_long );
    u_long whichNodeAllowed();

    void insert( u_int16_t, u_int32_t );
    u_int16_t get( u_long );
    bool search( u_int16_t, u_int32_t );
    // RequestTableEntry& getRequestTableEntry( u_long );
};

inline bool
RequestTable::search( u_int16_t seq, u_int32_t node )
{
    find( node );

    // return ( it != table.end()) ? it->request_number : 0U;
    
    if ( it != table.end()) {
    	if ( seq <= it->request_number )
    	    return true;
    	else
    	    return false;
    } else {
    	return false;
    }
}
    	    
inline void
RequestTable::notifyRouteReply( u_long dst )
{
    find( dst );
    
    if ( it != table.end()) {
    	it->ip_ttl      = 0;
    	it->timestamp   = 0;
    	it->retry_count = 0;
    	it->remain_time = 0;
    }
}

inline u_long
RequestTable::whichNodeAllowed()
{
    end = table.end();
    u_int64_t now( GetCurrentTime());
	
    for ( it = table.begin() ; it != end ; ++it ) {
    	if ( it->ip_ttl && ( now >= it->timestamp + it->remain_time )) {
    	    return it->ip_addr;
    	}
    }
    return dsr::IP_invalid_addr;
}

inline u_int64_t 
RequestTable::nextAllowedTime( u_long dst )
{
    find( dst );
    
    if ( it != table.end()) {
    	return ( it->timestamp + it->remain_time );
    } else {
    	return 0;
    }
}

inline u_int64_t
RequestTable::notifyRequestInitiate( u_long dst, u_char ttl )
{
    find( dst );
    
    if ( it != table.end()) {
    	it->ip_ttl      = ttl;
    	it->timestamp   = GetCurrentTime();
    	++it->retry_count;
    	
    	if ( it->retry_count == 1 ) {			// it's the first time to initiate rreq
    	    it->remain_time = period;
    	} else {
    	    it->remain_time *= 2;
    	}
    	
    	if ( it->remain_time > maxPeriod ) it->remain_time = maxPeriod;
    	
    	return ( it->timestamp + it->remain_time );
    } else {
    	RequestTableEntry entry;
    	
    	entry.ip_addr     = dst;
    	entry.ip_ttl      = ttl;
        entry.timestamp   = GetCurrentTime();
        entry.retry_count = 1;    	
        entry.remain_time = period;
        
        table.push_back( entry );
        
        return ( entry.timestamp + entry.remain_time );
    }
}

inline void
RequestTable::insert( u_int16_t req_num, u_int32_t node )
{
    // list<RequestTableEntry>::iterator it( find( node ));
    find( node );

    if ( it != table.end()) {    //
        it->request_number = req_num;
    } else {
        RequestTableEntry entry;

        entry.ip_addr = node;
        entry.request_number = req_num;

        table.push_back( entry );
    }

    return;
}

inline u_int16_t
RequestTable::get( u_long node )
{
    // list<RequestTableEntry>::iterator it( find( node ));
    find( node );

    return ( it != table.end()) ? it->request_number : 0U;
}

#endif // __NCTUNS_DSR_REQUESTTABLE_H
