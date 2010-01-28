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

#ifndef __NCTUNS_DSR_CONSTANTS_H
#define __NCTUNS_DSR_CONSTANTS_H

#include <arpa/inet.h>
#include <string>

namespace dsr {
  
    // option type
    const unsigned char PAD_1           = 0x00;
    const unsigned char PAD_N           = 0x01;
    const unsigned char ROUTE_REQUEST   = 0x02;
    const unsigned char ROUTE_REPLY     = 0x03;
    const unsigned char ROUTE_ERROR     = 0x04;
    const unsigned char ACK_REQUEST     = 0x05;
    const unsigned char ACK             = 0x06;
    const unsigned char SOURCE_ROUTE    = 0x07;

    const unsigned char NODE_UNREACHABLE = 0x01;

    const char           MAX_ROUTE_LEN = 15;         // nodes
    const int           MAX_SALVAGE_COUNT = 15;     // salvages

    // Route Cache
    const int           ROUTE_CACHE_TIMEOUT = 300;  // seconds

    // Send Buffer
    const int		SendBufferTimeout = 30;		// seconds

    // Route Request Table
    const int           REQUEST_TABLE_SIZE = 64;    // nodes
    const int           REQUEST_TABLE_IDS = 16;     // identifiers
    const int           MAX_REQUEST_REXMT = 16;     // retransmissions
    const int           MAX_REQUEST_PERIOD = 10;    // seconds
    const int           REQUEST_PERIOD = 500;       // milliseconds
    const int           NONPROP_REQUEST_TIMEOUT = 30;   // milliseconds
 
    const int 		RequestTableSize = 64;		// nodes
    const int 		RequestTableIdx = 16;		// identifiers
    const int 		MaxRequestRexmit = 16;		// retransmissions
    const int 		MaxRequestPeriod = 10;		// seconds
    const int 		RequestPeriod = 500;		// milliseconds
    const int		NonpropRequestTimeout = 30;	// milliseconds
    
    // Retransmission Buffer
    const int           DSR_RXMT_BUFFER_SIZE = 50;  // packets

    // Retransmission Timer
    const int           DSR_MAXRXTSHIFT = 2;

    // for use in my dsr agent
    const int           CACHE_ENTRY_PER_NODE = 1;
    const int		MAX_ROUTE_CACHE_ENTRY = 20;
    const int		SEND_BUFFER_SIZE = 10;
    const u_int32_t     IP_broadcast = ::inet_addr( "1.0.1.255" );
    const u_int32_t     IP_invalid_addr = ::inet_addr( "0.0.0.0" );
    const u_int32_t     IP_four = ::inet_addr( "1.0.1.4" );
    const u_int32_t     IP_five = ::inet_addr( "1.0.1.5" );
    
#ifdef __NCTUNS_DSR_DSRAGENT_H
    const char		*DSR = "dsr";
#endif
}

#endif // __NCTUNS_DSR_CONSTANTS_H
