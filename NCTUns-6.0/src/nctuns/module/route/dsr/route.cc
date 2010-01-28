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

#include "route.h"

bool
Route::operator==( const Route &rhs ) const
{
    if ( this != &rhs ) {
        if ( length != rhs.length ) return false;
        for ( int i = 0 ; i < length ; ++i )
            if ( path[i] != rhs.path[i] ) return false;
    }
    return true;
}

bool
Route::operator!=( const Route &rhs ) const
{
    if ( this != &rhs ) {
        if ( length != rhs.length ) return true;
        for ( int i = 0 ; i < length ; ++i )
            if ( path[i] != rhs.path[i] ) return true;
    }
    return false;
}

void
Route::push_back( const Route &rhs )
{
    assert( length + rhs.length < dsr::MAX_ROUTE_LEN );

    for ( int i = 0U ; i < rhs.length ; ++i )
        path[length++] = rhs.path[i];
}

void
Route::erase( unsigned short begin, unsigned short end )
// the nodes at indices (begin -> end - 1) are removed from the route
{
    if ( end <= begin ) return;
    if ( current_index > begin ) current_index -= end - begin;

    for ( unsigned short i = end, j = 0U ; i < length ; ++i, ++j )
        path[ begin + j ] = path[ i ];

    length -= end - begin;
}

void
Route::reverse( Route &route ) const
{
    if ( length == 0 ) route = Route();

    for ( unsigned short begin = 0U, end = length - 1U; begin < length;
        ++begin, --end )
        route.path[end] = path[begin];

    route.length        = length;
    route.current_index = ( length - 1 ) - current_index;
}

void
Route::reverse()
{
    if ( length == 0 ) return;

    u_long temp;
    for ( unsigned short fp = 0U, bp = length - 1U; fp < bp; ++fp, --bp ) {
        temp = path[ fp ];
        path[ fp ] = path[ bp ];
        path[ bp ] = temp;
    }

    current_index = ( length - 1 ) - current_index;
}

bool
Route::hasMember( u_int32_t node ) const
{
    for ( unsigned short i = 0U; i < length; ++i )
        if ( path[ i ] == node ) return true;

    return false;
}

bool
Route::duplicate() const
{
    for ( unsigned short i = 0 ; i < length - 2 ; ++i ) {
    	for ( unsigned short j = i + 1 ; j < length ; ++j ) {
    	    if ( path[i] == path[j] ) return true;
    	}
    }
    return false;
}

void
Route::compressPath()
// take a route and remove any double backs from it
// e.g. A B C B D --> A B D
{
    unsigned short fp = 0U, bp;
    while( fp < length ) {
        for ( bp = length - 1; bp != fp; --bp ) {
            if ( path[ fp ] == path[ bp ] ) {
                unsigned short from, to;
                for ( from = bp, to = fp; from < length ; ++from, ++to )
                    path[ to ] = path[ from ];
                length = to;
                break;
            } // end of removing double back
        } // end of scaning to check for double back
        ++fp; // advance the forward moving pointer
    }
}
