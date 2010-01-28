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

#include "dsragent.h"
#include <mbinder.h>

//#define MY_DEBUG

MODULE_GENERATOR( DSRAgent );

DSRAgent::DSRAgent( u_int32_t type, u_int32_t id, struct plist* pl, const char *name )
: NslObject( type, id, pl, name ), rreq_seq( 0U ),
  qmax_( 64U ), qcur_( 0U ), dsr_head( NULL ), dsr_tail( NULL )
{
    // disable flow control
    s_flowctl = DISABLED;
    r_flowctl = DISABLED;
}

int
DSRAgent::init()
{
    u_long *tmp = GET_REG_VAR( get_port(), "IP", u_long* );
    myIP = *tmp;

    // request_table.init( myIP );
    promis = GET_REG_VAR( get_port(), "promiscuous", char* );

    // hhchen: I should bind variable here... (not yet done)
    
    // if ( myIP == dsr::IP_five ) *promis = 1;
    // *promis = 1;

    assert( sizeof( DSR_Info ) < 50 );
    assert( sizeof( RREQ ) == 8 );
    assert( sizeof( RREP ) == 3 );
    assert( sizeof( RERR ) == 16 );
    assert( sizeof( ROUTE ) == 4 );

    return 1;
}

int
DSRAgent::push()
{
    if ( qcur_ > 0 ) {
        //ePacket_ *pkt = dsr_head;
        dsr_head = dsr_head->next_ep;
        --qcur_;

        /* int (NslObject::*upcall)() = (int (NslObject::*)())&DSRAgent::push;
        assert( s_queue->enqueue( pkt, sendtarget_, upcall ) > 0 ); */
        // handlePktWithoutSR( pkt, true );
    }

    return 1;
}

int
DSRAgent::recv( ePacket_ *pkt )
{
    Packet *p;
    GET_PKT( p, pkt );

    /* hhchen: Here, I should handle unicast from my neighbor,
       since it's come from my neighbor,
       it may not have DSR Header on it.
       for now, it always has.
    */

    // decapsulate DSR Info and DSR Header from Packet
    DSR_Info *dsr_info = (DSR_Info*)p->pkt_getinfo( "dsr" );
    if ( ! dsr_info ) {		// it's not a DSR packet
        freePacket( pkt );
        return 1;
    }

    // setup appropriate field in dsr_info for future use
    u_char *ptr( (u_char*)p->pkt_get());
    assert( ptr );
    
    if ( dsr_info->upper_layer_data ) {
    	dsr_info->ip_hdr = (ip*)p->pkt_sget();
    } else {
    	dsr_info->ip_hdr = (ip*)ptr;
        ptr += sizeof( ip );
    }
    Hdr_SR *hdr_sr = (Hdr_SR*)ptr;
    ptr += sizeof( Hdr_SR );
    // dsr_info->option = ptr;

/* hwchu: moved to below
    --dsr_info->ip_hdr->ip_ttl;
    if ( dsr_info->upper_layer_data ) {
        struct ip *ip_hdr = dsr_info->ip_hdr;

        if ( ip_hdr->ip_ttl == 0 ) {
            p->pkt_seek( dsrHeaderSize( dsr_info ));	    	
            put( pkt, recvtarget_ );
            return 1;
        }
    }
*/

    // get destination ip address
    u_long dst( getDst( hdr_sr ));

#ifdef DSR_DEBUG
    Debugger();

    u_long src( getSrc( dsr_info ));

    char src_ip[20], dst_ip[20];
    ipv4addr_to_str( src, src_ip );
    ipv4addr_to_str( dst, dst_ip );

    cout << " now is in recv() "
         << " src ip : " << src_ip << " dst ip : " << dst_ip << endl << endl;
#endif

    if ( ( *promis == 1 ) && ( p->rt_gateway() != dsr::IP_broadcast ) && ( p->rt_gateway() != myIP )) 
    {
        // Debugger( "promiscuous mode..." );
        autoRouteShortening( pkt, dsr_info, hdr_sr );
     
        freePacket( pkt );
        return 1;   
    }

    /* filter destination address */
    if ( dst == myIP || dst == dsr::IP_broadcast ) {
        return handlePacketReceipt( pkt, dsr_info, hdr_sr );
    } else {
/* hwchu: moved to below
    --dsr_info->ip_hdr->ip_ttl;
    if ( dsr_info->upper_layer_data ) {
        struct ip *ip_hdr = dsr_info->ip_hdr;

        if ( ip_hdr->ip_ttl == 0 ) {
            p->pkt_seek( dsrHeaderSize( dsr_info ));	    	
            put( pkt, recvtarget_ );
            return 1;
        }
    }
*/
	/* hwchu:
	 *   There is no chance for this packet to enter the kernel, 
	 *   so we decrement its TTL here.
	 */
    	dsr_info->ip_hdr->ip_ttl--;
    	if ( dsr_info->upper_layer_data && dsr_info->ip_hdr->ip_ttl <= 0) {
		p->pkt_seek( dsrHeaderSize( dsr_info ));	    	
		return put(pkt, recvtarget_);
	}

        if ( dsr_info->route_error ) {
            handleRouteError( pkt, dsr_info, hdr_sr );
        }

        if ( dsr_info->route_request )
            return handleRouteRequest( pkt, dsr_info, hdr_sr );
        else
            return handleForwarding( pkt, dsr_info, hdr_sr );
    }
}

int
DSRAgent::send( ePacket_ *pkt )
{
    assert( pkt );

#ifdef DSR_DEBUG
    Packet *p;
    GET_PKT( p, pkt );

    Debugger();

    // get destination ip address
    u_long src, dst;
    IP_SRC( src, p->pkt_sget());
    IP_DST( dst, p->pkt_sget());

    char ip[20], src_ip[20], dst_ip[20];
    ipv4addr_to_str( src, src_ip );
    ipv4addr_to_str( dst, dst_ip );

    cout << " now is in send() "
         << " src ip : " << src_ip << " dst ip : " << dst_ip << endl << endl;
#endif

    if( sendtarget_->get_curqlen() > 0 ) {
        // insert to dsr queue
        if ( qcur_ < qmax_ ) {
            if ( ! qcur_ ) {
                dsr_head = pkt;
                dsr_tail = pkt;
                pkt->next_ep = 0;
            } else {
                dsr_tail->next_ep = pkt;
                dsr_tail = pkt;
            }
            cout << "send(): enqueue to dsr queue...\n";
            ++qcur_;
        } else {
            cout << "send(): queue full drop...\n";
            // dsr queue is full, drop packet
            freePacket( pkt );
            return -1;
        }
        return 1;
    } else {
        return handlePacketWithoutSR( pkt, false );
    }
}

int
//DSRAgent::Debugger( char *ptr = NULL )
DSRAgent::Debugger( const char *ptr )
{
    //char ip[20];

    //cout << "DEBUG: DSRAgent, ";
    //NslObject::Debugger();

/*
    ipv4addr_to_str( myIP, ip );
    cout << "DEBUG: DSRAgent, My ip is: " << ip << endl;
    if ( ptr ) cout << ptr << endl;
*/

    return 1;
}

int
DSRAgent::handlePacketWithoutSR( ePacket_ *pkt, bool retry )
{
    Packet *p;
    GET_PKT( p, pkt );

    // get destination ip address
    u_long dst;
    IP_DST( dst, p->pkt_sget());

    // if ( dst == myIP ) return handlePacketReceipt();

    // Route route;
    Route const* route( route_cache.findRoute( dst ));
    if ( route ) {
#ifdef DSR_DEBUG
        cout << "handlePacketWithRoute()\n";
#endif
        return handlePacketWithRoute( pkt, dst, *route, retry );
    } else {
#ifdef DSR_DEBUG
        cout << "getRouteForPacket()\n";
#endif
#ifdef MY_DEBUG
cout << get_name() << " (1)\n";
#endif
        return getRouteForPacket( pkt, dst, retry );
    }
}

void
DSRAgent::debugger()
{	
	vector<u_int64_t> v;
	int size = rreq_timeout.size();

	cout << "size = " << size << " [ ";

	for(int i=0; i<size; i++) {
		cout << rreq_timeout.top() << ' ';
		v.push_back(rreq_timeout.top());
		rreq_timeout.pop();
	}

	for(int i=0; i<size; i++) {
		rreq_timeout.push(v[i]);
	}

	cout << "] size = " << rreq_timeout.size() << endl;

	return;
}

int
DSRAgent::getRouteForPacket( ePacket_ *pkt, u_long dst, bool retry )
{
    // Stick packet in the send buffer
    if ( pkt ) send_buffer.push_back( pkt );
    
    // check whether we can send another route request (RREQ)
    // since we MUST abide exponential backoff algorithm
    if ( GetCurrentTime() < request_table.nextAllowedTime( dst )) {
        return 1;
    }

    // send out Route Request (RREQ)
    Packet *newPacket = new Packet();
    newPacket->rt_setgw( dsr::IP_broadcast );
    newPacket->pkt_setflow( PF_SEND );

    // initialize DSR_Info
    DSR_Info *dsr_info = new DSR_Info( myIP );
    dsr_info->route_request = true;
    newPacket->pkt_addinfo( "dsr", (const char*)dsr_info, sizeof( DSR_Info ));

    // initialize IP header
    // char *tmp( newPacket->pkt_malloc( sizeof( Hdr_SR )));
    char *tmp( newPacket->pkt_malloc( dsrHeaderSize( dsr_info )));
    assert( tmp );
    delete dsr_info;

    ip ip_hdr;
    ip_hdr.ip_src = myIP;
    ip_hdr.ip_dst = dst;
    ip_hdr.ip_ttl = dsr::MAX_ROUTE_LEN;

    memcpy( tmp, (char*)&ip_hdr, sizeof( ip ));
    tmp += sizeof( ip );

    // initialize DSR header
    Hdr_SR hdr_sr;

    hdr_sr.destination  = dst;
    hdr_sr.sequence_num = ++rreq_seq;

    memcpy( tmp, (char*)&hdr_sr, sizeof( Hdr_SR ));
 
#if 0   
    u_int64_t next = request_table.notifyRequestInitiate( dst, dsr::MAX_ROUTE_LEN );
    float time;
    TICK_TO_SEC( time, next );
    cout << "We can next generate RREQ at " << time << endl;
    
    rreq_timeout.push( next );
#endif

#ifdef MY_DEBUG
int m = dsr::MAX_ROUTE_LEN;
int p = request_table.notifyRequestInitiate( dst, dsr::MAX_ROUTE_LEN );

cout << "(" << get_name() << ")" << " MAX_ROUTE_LEN = " << m << endl;
cout << "push " << p << endl;

cout << "before push..." << endl;
debugger();
rreq_timeout.push( p );
#else
    rreq_timeout.push( request_table.notifyRequestInitiate( dst, dsr::MAX_ROUTE_LEN ));
    //debugger();
#endif

#ifdef MY_DEBUG
cout << "after push..." << endl;
debugger();
#endif
    
    if ( ! send_buffer.empty()) {
        BASE_OBJTYPE( type );
        type = POINTER_TO_MEMBER( DSRAgent, checkSendBuffer );
        IF_timer.setCallOutObj( this, type );

#ifdef MY_DEBUG
cout << "rreq_timeout.top() = " << rreq_timeout.top() << endl;
cout << "GetCurrentTime() = " << GetCurrentTime() << endl;
#endif

/* comment by hwchu
        assert( rreq_timeout.top() - GetCurrentTime() != 0 );
        IF_timer.start( rreq_timeout.top() - GetCurrentTime(), 0 );
*/
	if( rreq_timeout.top() == GetCurrentTime() ) {
		/* Timer event should be triggered first */
		IF_timer.cancel();
		checkSendBuffer();
	} else {
        	IF_timer.start( rreq_timeout.top() - GetCurrentTime(), 0 );
	}

    } else {
    	cout << "send buffer is empty, ";
    	if ( rreq_timeout.empty()) {
    	    cout << "and rreq_timeout is empty...\n";
    	} else {
    	    cout << "but rreq_timeout is not...\n";
    	}
    }
    
    ePacket_ *newPkt = createEvent();
    //CREATE_EVENT( newPkt );

    // attach (packet)newPacket to (event)newPkt
    //SET_EVENT_CALLOUTOBJ( newPkt, NULL, NULL, (void*)newPacket);
    ATTACH_PKT( newPacket, newPkt );

    // put( pkt, sendtarget_, 0 );
    // int (NslObject::*upcall)() = (int (NslObject::*)())&DSRAgent::push;
    put( newPkt, sendtarget_ );
    
    return 1;
}

int
DSRAgent::handlePacketReceipt( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    Packet *p;
    GET_PKT( p, pkt );

    if ( dsr_info->route_reply ) {

#ifdef DSR_DEBUG
        cout << "acceptRouteReply();\n";
#endif
        acceptRouteReply( pkt, dsr_info, hdr_sr );

        // we now know how to send packet to the target node from route reply
        // find send_buffer if there are packets destined to this target node
        u_long dst( hdr_sr->rrep_route[hdr_sr->rrep_route.getLength()-1] );
        
        // notify request table
        request_table.notifyRouteReply( dst );
        
        ePacket_ *tmp_pkt( send_buffer.find( dst ));
        while ( tmp_pkt ) {
            Packet *tmp_p;
            GET_PKT( tmp_p, tmp_pkt );

            if ( tmp_p->pkt_getinfo( "dsr" )) {
                handlePacketWithRoute( tmp_pkt, dst, hdr_sr->rrep_route, true );
            } else {
                handlePacketWithRoute( tmp_pkt, dst, hdr_sr->rrep_route, false );
            }
            tmp_pkt = send_buffer.find( dst );
        }
    }

    if ( dsr_info->route_request ) {

        u_long src( getSrc( dsr_info ));

        char ip[20];
        ipv4addr_to_str( src, ip );

        if ( hdr_sr->sequence_num <= request_table.get( src )) {

#ifdef DSR_RREQ_MESSAGE
            Debugger();
            cout << " ignore RREQ from " << ip << ", and RREQ Number is " << hdr_sr->sequence_num << "  \n";
#endif
            freePacket( pkt );
            return 1;
        } else {

#ifdef DSR_DEBUG
            cout << "returnSrcRouteToRequestor();\n";
#endif
            /* hhchen: I don't know whether we should insert sequence_num
               into request table or not, since it's not mentioned in the
               draft.  But, if we do not insert sequence_num into table,
               we MAY reply two or more times to the sender.
            */ 
            request_table.insert( hdr_sr->sequence_num, src );
            returnSrcRouteToRequestor( pkt, dsr_info, hdr_sr );

            return 1;
        }
    } else {

        // route error report should be handled here.
        if ( dsr_info->route_error ) {
            handleRouteError( pkt, dsr_info, hdr_sr );
        }

        if ( dsr_info->upper_layer_data ) {
            // p->pkt_seek( sizeof( Hdr_SR ));
            p->pkt_seek( dsrHeaderSize( dsr_info ));
            put( pkt, recvtarget_ );
            return 1;
        } else {
            /* pure DSR control packet, can't deliver to upper layer,
             * otherwise, simulator will crash.
             */
            freePacket( pkt );
            return 1;
        }
    }
}

int
DSRAgent::handleRouteRequest( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    Packet *p;
    GET_PKT( p, pkt );

#ifdef DSR_RREQ_MESSAGE
    char ip[20]; ipv4addr_to_str( myIP, ip );

    cout << " I am " << ip << "  now handleRouteRequest(); \n";
    u_long src( getSrc( dsr_info ));
#endif

    // check whether the route request had been handled before
    if ( ignoreRouteRequest( dsr_info, hdr_sr )) {

#ifdef DSR_RREQ_MESSAGE
        ( src, ip );
        cout << " ignore RREQ from " << ip << ", and RREQ Number is " << hdr_sr->sequence_num << "\n\n";
#endif

        freePacket( pkt );
        return 1;
    }

    // insert sequence number into request table to avoid loop
    request_table.insert( hdr_sr->sequence_num, getSrc( dsr_info ));

    hdr_sr->rrep_route.push_back( myIP );

    // check whether we can reply from route cache
    if ( replyFromRouteCache( pkt, dsr_info, hdr_sr )) return 1;

#ifdef DSR_RREQ_MESSAGE
    cout << " requesttable's rreq_seq number is  " << request_table.get( getSrc( dsr_info )) << endl;
    hdr_sr->rrep_route.dump();
#endif

    // hhchen: a BroadcastJitter is required.
    p->pkt_setflow( PF_SEND );
    if ( dsr_info->ip_hdr->ip_ttl ) put( pkt, sendtarget_ );
    else freePacket( pkt );

    return 1;
}

int
DSRAgent::handleForwarding( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    Packet *p;
    GET_PKT( p, pkt );

    if ( hdr_sr->route.current() != myIP ) {
        char tmp[20]; 
        ipv4addr_to_str( myIP, tmp );
        cout << "My ip is " << tmp << endl;
        ipv4addr_to_str( hdr_sr->route.current(), tmp);
    	cout << "the other is " << tmp << endl;
    }
    // assert( hdr_sr->route.current() == myIP );
    p->rt_setgw( hdr_sr->route.next());

    // set error handler
    BASE_OBJTYPE( type );
    type = POINTER_TO_MEMBER( DSRAgent, errorHandler );
    p->pkt_setHandler( this, type );

    p->pkt_setflow( PF_SEND );
    return put( pkt, sendtarget_ );
}

void
DSRAgent::returnSrcRouteToRequestor( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    Packet *p;
    GET_PKT( p, pkt );

    u_long src( getSrc( dsr_info ));

    Packet *newPacket = new Packet();

    // initialize DSR_Info
    DSR_Info *new_dsr_info = new DSR_Info( myIP );
    new_dsr_info->route_reply = true;
    newPacket->pkt_addinfo( "dsr", (const char*)new_dsr_info, sizeof( DSR_Info ));
    
    // initialize IP header
    // char *tmp = newPacket->pkt_malloc( sizeof( Hdr_SR ));
    char *tmp = newPacket->pkt_malloc( dsrHeaderSize( new_dsr_info ));
    assert( tmp );
    delete new_dsr_info;
    
    ip ip_hdr;
    ip_hdr.ip_src = myIP;
    ip_hdr.ip_dst = src;
    ip_hdr.ip_ttl = 255;
    
    memcpy( tmp, (char*)&ip_hdr, sizeof( ip ));
    tmp += sizeof( ip );

    // initialize DSR header
    Hdr_SR new_hdr_sr;
    new_hdr_sr.destination   = src;
    new_hdr_sr.sequence_num  = hdr_sr->sequence_num;
    new_hdr_sr.route         = hdr_sr->rrep_route;
    /* hhchen: here , I assume MAC layer use bi-directional link such as
               IEEE 802.11, otherwise, we shold initiate another
               route request, instead of just reverse the route.
    */
    new_hdr_sr.route.reverse();
    new_hdr_sr.rrep_route    = hdr_sr->rrep_route;
    new_hdr_sr.route.push_back( src );
    new_hdr_sr.rrep_route.push_back( myIP );

    new_hdr_sr.route.resetIterator();

    // store the path in the route cache
    route_cache.addRoute( new_hdr_sr.route );

    newPacket->pkt_setflow( PF_SEND );
    newPacket->rt_setgw( new_hdr_sr.route.next());
    memcpy( tmp, (char*)&new_hdr_sr, sizeof( Hdr_SR ));

#ifdef DSR_RREP_MESSAGE
    cout << "ReturnSrcRouteToRequestor(), and the route is \n";
    new_hdr_sr.route.dump();
#endif

    ePacket_ *newPkt;
    CREATE_EVENT( newPkt );

    // attach (packet)newPacket to (event)newPkt
    SET_EVENT_CALLOUTOBJ( newPkt, NULL, NULL, (void*)newPacket);

    put( newPkt, sendtarget_ );
    return;
}

void
DSRAgent::acceptRouteReply( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    Packet *p;
    GET_PKT( p, pkt );

#ifdef DSR_ACCEPT_RREP_MESSAGE
    Debugger( "In acceptRouteReply();" );
    hdr_sr->rrep_route.dump();
#endif

    route_cache.addRoute( hdr_sr->rrep_route );
    return;
}

int
DSRAgent::handlePacketWithRoute( ePacket_ *pkt, u_long dst, Route const &route, bool retry )
{
    Packet *p;
    GET_PKT( p, pkt );

    if ( ! retry ) {
        // initialize DSR_Info
        DSR_Info *dsr_info = new DSR_Info( myIP );
        dsr_info->upper_layer_data = true;
        p->pkt_addinfo( "dsr", (const char*)dsr_info, sizeof( DSR_Info ));
        delete dsr_info;

        // initialize DSR header
        // char *tmp( p->pkt_malloc( sizeof( Hdr_SR )));
        char *tmp( p->pkt_malloc( sizeof( Hdr_SR )));
        assert( tmp );

        Hdr_SR hdr_sr;
        hdr_sr.route         = route;
        hdr_sr.destination   = dst;

#ifdef DSR_SOURCE_ROUTE_MESSAGE
        Debugger();
        hdr_sr.route.dump();
#endif

        p->rt_setgw( hdr_sr.route.next());

        memcpy( tmp, (char*)&hdr_sr, sizeof( Hdr_SR ));
    } else {
        char *ptr( p->pkt_get());
        assert( ptr );
        Hdr_SR *hdr_sr = (Hdr_SR*)ptr;

        hdr_sr->route = route;
        p->rt_setgw( hdr_sr->route.next());
    }

    // set error handler
    BASE_OBJTYPE( type );
    type = POINTER_TO_MEMBER( DSRAgent, errorHandler );
    p->pkt_setHandler( this, type );

    // if ( retry )
        p->pkt_setflow( PF_SEND );
        put( pkt, sendtarget_ );
        return 1;
    /* else {
        int (NslObject::*upcall)() = (int (NslObject::*)())&DSRAgent::push;
        assert( s_queue->enqueue( pkt, sendtarget_, upcall ) > 0 );
        return 1;
    } */
}

bool
DSRAgent::ignoreRouteRequest( DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    u_long src( getSrc( dsr_info ));

    if ( myIP == src                                       ||
         hdr_sr->rrep_route.hasMember( myIP )              ||
         request_table.search( hdr_sr->sequence_num, src ) ||
         /* hhchen: the following line is because in my implementation,
            route length can't exceed dsr::MAX_ROUTE_LEN 
            this was discussed in draft v5, but in draft v6, 
            this constraint was took away. */
         hdr_sr->rrep_route.full()                 )
        return true;

    return false;
}

int
DSRAgent::errorHandler( ePacket_ *pkt )
{
    /// 
    Debugger( "---------------->>>>errorHandler is called...\n" );

    Packet *p;
    GET_PKT( p, pkt );

    // since this packet was forwarded by me, it MUST have DSR_Info on it
    DSR_Info *dsr_info = (DSR_Info*)p->pkt_getinfo( "dsr" );
    assert( dsr_info );

    char *tmp( p->pkt_get());
    assert( tmp );

    // setup appropriate field in DSR_Info for future use
    if ( dsr_info->upper_layer_data ) {
        dsr_info->ip_hdr = (ip*)p->pkt_sget();
    } else {
        dsr_info->ip_hdr = (ip*)tmp;
        tmp += sizeof( ip );
    }
    Hdr_SR *hdr_sr( (Hdr_SR*)tmp );
    ip *ip_hdr( dsr_info->ip_hdr );
    
    /// hhchen: old implementation
    /* tmp += sizeof( Hdr_SR );
     * dsr_info->option = (u_char*)tmp;
     */

    // If I was the originator of this packet...
    if ( myIP == ip_hdr->ip_src ) {
        
        // flush cache entry
        route_cache.erase( hdr_sr->route[0] );

        /* hhchen: I try to find another route to destination,
           but should I, [Page 64] says that "When an intermediate node
           forwarding a packet detects...."
        */

        // if I still have route to destination
        /// 
        assert( getDst( hdr_sr ) == ip_hdr->ip_dst );
        Route const* route( route_cache.findRoute( ip_hdr->ip_dst ));
        if ( route ) {
            // hhchen: these code section have not yet been tested...
            hdr_sr->route = *route;

            p->rt_setgw( hdr_sr->route.next());
            put( pkt, sendtarget_ );
        } else {
            //getRouteForPacket( pkt, getDst( hdr_sr ), true );
#ifdef MY_DEBUG
cout << get_name() << " (2)\n";
#endif
            getRouteForPacket( pkt, ip_hdr->ip_dst, true );
        }
        return 1;
    }

    /* I was not the originator of this packet, 
     * send out Route Error (RERR) to the originator.
     */
    Packet *newPacket = new Packet();
    newPacket->pkt_setflow( PF_SEND );

    // initialize Route Error option
    u_char *ptr = new u_char[sizeof(RERR)];
    RERR *opt( (RERR*)ptr );

    // [Page 32-Page 34], DSR Draft, 21 November, 2001
    opt->option_type  = dsr::ROUTE_ERROR;
    opt->opt_data_len = 10 + 4; // 10, plus the size of any type-specific information
    opt->error_type   = dsr::NODE_UNREACHABLE;
    opt->reservd      = 0;
    opt->salvage      = 1;  // hhchen: need to be corrected;
    opt->source       = myIP;
    ///opt->destination  = getSrc( dsr_info );
    opt->destination  = ip_hdr->ip_src;
    // intended next hop which can't be reached by me
    opt->info         = hdr_sr->route.current();

    // initialize DSR_Info
    DSR_Info *new_dsr_info = new DSR_Info( myIP );
    ////
    // new_dsr_info->length = sizeof(RERR);
    new_dsr_info->option = ptr;
    new_dsr_info->route_error = true;
    newPacket->pkt_addinfo( "dsr", (const char*)new_dsr_info, sizeof( DSR_Info ));
    delete new_dsr_info;

    // initialize IP header
    tmp = newPacket->pkt_malloc( sizeof( ip ) + sizeof( Hdr_SR ));
    assert( tmp );

    ip new_ip_hdr;
    new_ip_hdr.ip_src = myIP;
    new_ip_hdr.ip_dst = ip_hdr->ip_src;
    new_ip_hdr.ip_ttl = dsr::MAX_ROUTE_LEN;

    memcpy( tmp, (char*)&ip_hdr, sizeof( ip ));
    tmp += sizeof( ip );
   
    // initialize DSR header
    Hdr_SR new_hdr_sr;
    new_hdr_sr.payload_length = sizeof(RERR);
    ////
    // new_hdr_sr.option = ptr;
    //// hhchen: source route option do not have destination
    new_hdr_sr.destination = ip_hdr->ip_src;

    Debugger();
    assert( hdr_sr->route.previous() == myIP );

    for ( int i = hdr_sr->route.getIterator() - 3 ; i >= 0 ; --i )
        new_hdr_sr.route.push_back( hdr_sr->route[i] );
    new_hdr_sr.route.push_back( ip_hdr->ip_src );

    newPacket->pkt_setflow( PF_SEND );
    newPacket->rt_setgw( new_hdr_sr.route.next());
    new_hdr_sr.route.dump();

    memcpy( tmp, (char*)&new_hdr_sr, sizeof( Hdr_SR ));

    ePacket_ *newPkt = createEvent();

    // attach (packet)newPacket to (event)newPkt
    ATTACH_PKT( newPacket, newPkt );

    // after originating the route error, I SHOULD try packet salvage
    if ( route_cache.findRoute( ip_hdr->ip_dst, hdr_sr->route )) {
        p->rt_setgw( hdr_sr->route.next());
        put( pkt, sendtarget_ );
    } else {
        freePacket( pkt );
    }
    return 1;
}

void
DSRAgent::handleRouteError( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    int offset( 0 );
    u_char const *ptr( dsr_info->option );
    vector<RERR*> rerr;

    // parse option
    while( offset < hdr_sr->payload_length ) {
        if ( ptr[offset] == dsr::ROUTE_ERROR ) {
            rerr.push_back( (RERR*)(ptr+offset));
        } 

        if ( ptr[offset] == dsr::PAD_1 ) {
            offset += 1;
        } else {
            // the other 2 bytes are option_type and opt_date_len
            offset += ptr[offset+1] + 2;
        }
    }
    assert ( offset == hdr_sr->payload_length );

    vector<RERR*>::const_iterator end( rerr.end());
    for ( vector<RERR*>::const_iterator it = rerr.begin(); it != end; ++it ) {
        if ( (*it)->error_type == dsr::NODE_UNREACHABLE ) {
            assert( (*it)->opt_data_len == 14 );
            //assert( (*it)->reservd == 0 );
            //assert( (*it)->salvage == 0 );    //hhchen: needed to be corrected;

            if ( (*it)->source == myIP ) {
                route_cache.erase( (*it)->info );
            } else if ( (*it)->info == myIP ) {
                route_cache.erase( (*it)->source );
            } else {
                route_cache.erase( (*it)->source, (*it)->info );
            }
        } else {
            // error type was not defined by DSR Draft, this MUST not happen
            cerr << "Error type not defined...\n";
            assert( false );
        }
    }
    return;
}

void
DSRAgent::checkSendBuffer()
{
    u_long node( request_table.whichNodeAllowed());
    assert( GetCurrentTime() == rreq_timeout.top());

#ifdef MY_DEBUG
cout << "(" << get_name() << ")" << endl;
cout << "before pop..." << endl;
debugger();
#endif

    rreq_timeout.pop();

#ifdef MY_DEBUG
cout << "after pop..." << endl;
debugger();
#endif
    
    // if ( node != dsr::IP_invalid_addr ) {
    if ( request_table.whichNodeAllowed() != dsr::IP_invalid_addr ) {
#ifdef MY_DEBUG
cout << get_name() << " (3)\n";
#endif
    	getRouteForPacket( NULL, node, true );
    } else {
        if ( ! rreq_timeout.empty()) {	
            if ( ! send_buffer.empty()) {    
    	        BASE_OBJTYPE( type );
                type = POINTER_TO_MEMBER( DSRAgent, checkSendBuffer );
                IF_timer.setCallOutObj( this, type );
                assert( rreq_timeout.top() - GetCurrentTime() != 0 );
                IF_timer.start( rreq_timeout.top() - GetCurrentTime(), 0 );
            }
        } else {
            assert( send_buffer.empty());
        }
    }

    return;
}  

bool
DSRAgent::replyFromRouteCache( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    Route const *route( route_cache.findRoute( getDst( hdr_sr )));
    
    if ( route && ( hdr_sr->rrep_route.getLength() + (*route).getLength() <= dsr::MAX_ROUTE_LEN )) {
    	
        Route tmp_route( hdr_sr->rrep_route );
        tmp_route.push_back( *route );
        
        u_long src( getSrc ( dsr_info ));
        if ( tmp_route.hasMember( src ) || tmp_route.duplicate()) {
            Debugger( "Duplicate found...\n" );
            return false;
        }
    
        // we can now send cached route reply
        assert( ! dsr_info->upper_layer_data );
        
        Packet *p;
        GET_PKT( p, pkt );

        // change DSR_Info to repond route reply
        dsr_info->source        = myIP;
        dsr_info->route_request = false;
        dsr_info->route_reply   = true;

        // initialize DSR header
        hdr_sr->destination = src;
        hdr_sr->route = hdr_sr->rrep_route;
        hdr_sr->route.pop_back();
        hdr_sr->route.reverse();
        hdr_sr->route.push_back( src );
        hdr_sr->rrep_route.push_back( *route );
        
        /* char *tmp = newPacket->pkt_malloc( sizeof( Hdr_SR ));
    assert( tmp );

        Hdr_SR new_hdr_sr;
        hdr_hdr_sr.destination   = src;
        new_hdr_sr.sequence_num  = hdr_sr->sequence_num;
    new_hdr_sr.route         = hdr_sr->rrep_route;
    new_hdr_sr.route.reverse();
    new_hdr_sr.rrep_route    = hdr_sr->rrep_route;
    new_hdr_sr.route.push_back( src );
    new_hdr_sr.rrep_route.push_back( myIP );
    */
    
        hdr_sr->route.resetIterator();

        // store the path in the route cache
        route_cache.addRoute( hdr_sr->route );

        p->pkt_setflow( PF_SEND );
        p->rt_setgw( hdr_sr->route.next());
        // memcpy( tmp, (char*)&new_hdr_sr, sizeof( Hdr_SR ));

#ifdef DSR_RREP_MESSAGE
    cout << "ReturnSrcRouteToRequestor(), and the route is \n";
    hdr_sr->route.dump();
#endif

        put( pkt, sendtarget_ );
        return true;
    }    

    return false;
}

void
DSRAgent::autoRouteShortening( ePacket_ *pkt, DSR_Info *dsr_info, Hdr_SR *hdr_sr )
{
    if ( getDst( hdr_sr ) == myIP || hdr_sr->route.latter( myIP )) {
        Route tmp_route( hdr_sr->route );
        for ( int i = hdr_sr->route.getIterator() ; i < hdr_sr->route.getLength() ; ++i )
            if ( tmp_route[i] == myIP ) { tmp_route.erase( hdr_sr->route.getIterator() - 1, i ); break; }
        
        /* Packet *p;
        GET_PKT( p, pkt );
        cout << "------->\n\n";
        char *tmp = new char[20]; ipv4addr_to_str( p->rt_gateway(), tmp );
        cout << tmp << endl;
        tmp_route.dump(); */
        
        u_long src( getSrc( dsr_info ));
        
        Packet *newPacket = new Packet();

        // initialize DSR_Info
        DSR_Info *new_dsr_info = new DSR_Info( getDst( hdr_sr ));
        new_dsr_info->route_reply = true;
        newPacket->pkt_addinfo( "dsr", (const char*)new_dsr_info, sizeof( DSR_Info ));
        delete new_dsr_info;
        
        // initialize DSR header
        char *tmp = newPacket->pkt_malloc( sizeof( Hdr_SR ));
        assert( tmp );

        Hdr_SR new_hdr_sr;
        new_hdr_sr.destination   = src;
        // new_hdr_sr.sequence_num  = hdr_sr->sequence_num;
        for ( int i = hdr_sr->route.getIterator()-2 ; i >= 0 ; --i )
            new_hdr_sr.route.push_back( hdr_sr->route[i] );
        new_hdr_sr.route.push_back( getSrc( dsr_info ));
        
        new_hdr_sr.rrep_route    = tmp_route;

        new_hdr_sr.route.resetIterator();

        // store the path in the route cache
        route_cache.addRoute( new_hdr_sr.route );

        newPacket->pkt_setflow( PF_SEND );
        newPacket->rt_setgw( new_hdr_sr.route.next());
        memcpy( tmp, (char*)&new_hdr_sr, sizeof( Hdr_SR ));

#ifdef DSR_PRO_RREP_MESSAGE
        Debugger( "ReturnPromiscuousSrcRouteToRequestor(), and the route is \n" );
        new_hdr_sr.route.dump();
        
        cout << " and the replied route is \n";
        new_hdr_sr.rrep_route.dump();
        
        //ipv4addr_to_str( newPacket->rt_gateway(), tmp );
        //cout << tmp << endl;
        
        //Hdr_SR *abc( (Hdr_SR*)tmp );
        //abc->route.dump();
#endif

        ePacket_ *newPkt;
        CREATE_EVENT( newPkt );

        // attach (packet)newPacket to (event)newPkt
        SET_EVENT_CALLOUTOBJ( newPkt, NULL, NULL, (void*)newPacket);

        put( newPkt, sendtarget_ );
        return;  
    } 
}
