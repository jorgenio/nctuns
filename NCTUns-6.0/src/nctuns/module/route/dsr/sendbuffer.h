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

/* [Page 20], DSR Draft, 21 November, 2001
 *
 * 4.2. Send Buffer
 *
 * The Send Buffer of a node implementing DSR is a queue of packets that
 * cannot be sent by that node because it does not yet have a source
 * route to each such packet's destination.  Each packet in the Send
 * Buffer is logically associated with the time that it was placed into
 * the Buffer, and SHOULD be removed from the Send Buffer and silently
 * discarded after a period of SendBufferTimeout after initially being
 * placed in the Buffer.  If necessary, a FIFO strategy SHOULD be used
 * to evict packets before they timeout to prevent the buffer from
 * overflowing.
 *
 * Subject to the rate limiting defined in Section 6.2, a Route
 * Discovery SHOULD be initiated as often as possible for the
 * destination address of any packets residing in the Send Buffer.
 */

#ifndef __NCTUNS_DSR_SENDBUFFER_H
#define __NCTUNS_DSR_SENDBUFFER_H

#include <list>
#include "constants.h"

class SendBufferEntry {

friend class SendBuffer;

private:
    ePacket_		                *pkt;
    u_int64_t		                timestamp;
    u_long		                dst;

public:
    SendBufferEntry( ePacket_ *pkt ): pkt( pkt ), timestamp( GetCurrentTime()) {
        Packet *p;
        GET_PKT( p, pkt );
        IP_DST( dst, p->pkt_sget());
    }
};

class SendBuffer {

private:
    list<SendBufferEntry>	        buffer;
    u_int64_t			        timeout;

    // delete timeout packet from send buffer
    void refresh() {

        u_int64_t now( GetCurrentTime());

        while ( buffer.size() && ( ( now - buffer.begin()->timestamp ) > timeout )) {
            ePacket_ *pkt( buffer.begin()->pkt );
            freePacket( pkt );

#ifdef DSR_SEND_BUFFER_MESSAGE
            static u_int32_t i( 0 );
            char tmp[20]; ipv4addr_to_str( buffer.begin()->dst, tmp );
            cout << "i = " << ++i;
            cout << "--- SendBuffer Timeout " << tmp << endl << endl;
#endif

            buffer.pop_front();
        }
    }

public:

    SendBuffer() {
        SEC_TO_TICK( timeout, (u_int64_t)dsr::SendBufferTimeout );
    }

    bool empty() { 
        refresh();
        return buffer.empty();
    }
    	
    void push_back( ePacket_ *&pkt ) {

        refresh();

#ifdef DSR_SEND_BUFFER_MESSAGE
        Packet *p;
        GET_PKT( p, pkt );

        u_long dst;
        IP_DST( dst, p->pkt_sget());

        char tmp[20]; ipv4addr_to_str( dst, tmp );
        cout << "Destination: " << tmp << endl;
#endif

        SendBufferEntry entry( pkt );
        pkt = NULL;
        buffer.push_back( entry );

        if ( buffer.size() > (unsigned int)dsr::SEND_BUFFER_SIZE ) {
            // send buffer is full, drop the first packet
            ePacket_ *pkt = buffer.begin()->pkt;
            freePacket( pkt );

#ifdef DSR_SEND_BUFFER_MESSAGE
            ipv4addr_to_str( buffer.begin()->dst, tmp );
            cout << "--- Drop " << tmp << endl << endl;
#endif

            buffer.pop_front();
            assert ( buffer.size() == (unsigned int)dsr::SEND_BUFFER_SIZE );
            
        }
    }

    /* bool exist( u_long dst ) {

        refresh();

        list<SendBufferEntry>::iterator end( buffer.end());
        for ( list<SendBufferEntry>::iterator it = buffer.begin() ; it != end ; ++it ) {
            if ( it->dst == dst ) {
                return true;
            }
        }

        return false;
    } */

    ePacket_* find( u_long dst ) {

        refresh();

        list<SendBufferEntry>::iterator end( buffer.end());
        for ( list<SendBufferEntry>::iterator it = buffer.begin() ; it != end ; ++it ) {
            if ( it->dst == dst ) {
                ePacket_ *pkt = it->pkt;
                buffer.erase( it );
                return pkt;
            }
        }

        return NULL;
    }
};

#endif
