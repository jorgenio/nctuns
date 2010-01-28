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

#ifndef __NCTUNS_DSR_DSRAGENT_H
#define __NCTUNS_DSR_DSRAGENT_H

//#define DSR_DEBUG
#define DSR_SOURCE_ROUTE_MESSAGE
#define DSR_ACCEPT_RREP_MESSAGE

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <object.h>

#include <list>
#include <regcom.h>
#include <scheduler.h>
#include <timer.h>
#include <nodetype.h>
#include <stdlib.h>
#include <vector>
#include <stack>

// for NCTU Network Simulator
#include <ethernet.h>
#include <event.h>
#include <ip.h>
#include <nctuns_api.h>
#include <packet.h>

// for Dynamic Source Routing
#include <iostream>
#include <queue>
#include <string>

#include "dsr_info.h"
#include "hdr_sr.h"
#include "option.h"
#include "requesttable.h"
#include "route.h"
#include "routecache.h"
#include "sendbuffer.h"

//extern RegTable         RegTable_;
//extern typeTable        *typeTable_;
//extern scheduler        *scheduler_;

class DSRAgent : public NslObject {

private:

    timerObj				IF_timer;

    u_long              		myIP;
    unsigned short      		rreq_seq;
    char				*promis;

    RequestTable        		request_table;
    RouteCache			        route_cache;
    SendBuffer				send_buffer;
    
    priority_queue<u_int64_t, vector<u_int64_t>, greater<u_int64_t> >	rreq_timeout;
    
    unsigned short      		qmax_;
    unsigned short      		qcur_;

    ePacket_				*dsr_head;
    ePacket_				*dsr_tail;

    void debugger();
    
    int push();
    u_long getSrc( DSR_Info *dsr_info ) { return dsr_info->source; }
    u_long getDst( Hdr_SR *hdr_sr ) { return hdr_sr->destination; }

    void checkSendBuffer();
    u_long dsrHeaderSize( DSR_Info* );

    // send
    int  handlePacketWithoutSR( ePacket_*, bool ); 
    int  handlePacketWithRoute( ePacket_*, u_long, Route const&, bool );
    int  getRouteForPacket( ePacket_*, u_long, bool );
     
    void acceptRouteReply( ePacket_*, DSR_Info*, Hdr_SR* );
    int handlePacketReceipt( ePacket_*, DSR_Info*, Hdr_SR* );
    void handleRouteError( ePacket_*, DSR_Info*, Hdr_SR* );
    int handleRouteRequest( ePacket_*, DSR_Info*, Hdr_SR* );
    int handleForwarding( ePacket_*, DSR_Info*, Hdr_SR* );
    void returnSrcRouteToRequestor( ePacket_*, DSR_Info*, Hdr_SR* );
    void autoRouteShortening( ePacket_*, DSR_Info*, Hdr_SR* );

    bool replyFromRouteCache( ePacket_*, DSR_Info*, Hdr_SR* );
    bool ignoreRouteRequest( DSR_Info*, Hdr_SR* );
    int errorHandler( ePacket_* );

public:
    DSRAgent( u_int32_t type, u_int32_t id, struct plist* pl, const char *name );

    int init();
    int recv( ePacket_* );
    int send( ePacket_* );
    int Debugger( const char *ptr = 0 );

};

inline u_long
DSRAgent::dsrHeaderSize( DSR_Info *dsr_info )
{
    u_long i( sizeof( Hdr_SR ));
    
    // if this is a pure dsr packet, we must add size of ip header
    if ( ! dsr_info->upper_layer_data ) i += sizeof( ip );

    // if ( dsr_info->route_request )
    // if ( dsr_info->route_reply ) 
    if ( dsr_info->route_error ) i+= sizeof( RERR );
 
    return i;   
}
#endif // __NCTUNS_DSR_DSRAGENT_H
