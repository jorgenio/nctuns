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

#ifndef __NCTUNS_DSR_ROUTE_H
#define __NCTUNS_DSR_ROUTE_H

#include <iostream>

#include <sys/types.h>
#include <nctuns_api.h>

#include <cassert>

#include "constants.h"

using namespace std;

class Route {

private:
    u_long 		path[dsr::MAX_ROUTE_LEN];
    unsigned char       length;
    unsigned char       current_index;

public:
    // static void copyIntoPath( Route&, Route&, unsigned int, unsigned int );

    void dump() {
        cout << "current = " << static_cast<u_long>( current_index  ) << endl;
             // << "length  = " << static_cast<u_long>( length ) << endl;
        
        char ip[20];
        for ( int i = 0 ; i < length ; ++i ) {
            ipv4addr_to_str( path[i], ip );
            cout << ip << endl;
        }
        cout << endl;
    }

    // constructor
    Route(): length( 0U ), current_index( 0U ) {}
    Route( const Route &route ): length( route.length ), current_index( route.current_index ) {
        //for ( int i = 0 ; i < dsr::MAX_ROUTE_LEN ; ++i )
        for( int i = 0 ; i < length ; ++i )
            path[i] = route.path[i];
    }
     
    u_long next() { assert( current_index < length ); return path[current_index++]; }
    bool full() const { return length >= dsr::MAX_ROUTE_LEN; }

    // length and iterator related function
    void resetIterator() { current_index = 0U; }
    void reset() { length = 0U; current_index = 0U; }
    void setIterator( unsigned char i ) { assert( i < length ); current_index = i; }
    void setLength( unsigned char i ) { assert( i < dsr::MAX_ROUTE_LEN ); length = i; }
    unsigned char getIterator() const { return current_index; }
    unsigned char getLength() const { return length; }

    u_long previous() const { 
        return ( ( current_index > 1 ) ? path[current_index-2]: dsr::IP_invalid_addr );
    }        
    u_long current() const { 
        return ( ( current_index > 0 ) ? path[current_index-1]: dsr::IP_invalid_addr );
    }

    long size() const { return length * sizeof( u_long ) + 2; }

    u_long& operator[]( u_long );
    Route& operator=( const Route& );
    bool operator==( const Route& ) const;
    bool operator!=( const Route& ) const;
   
    void push_back( u_int32_t );
    void pop_back();
    void push_back( const Route& );
    void erase( unsigned short, unsigned short );
    void reverse( Route& ) const;
    void reverse();
    bool hasMember( u_int32_t ) const;
    bool duplicate() const;
    bool latter( u_int32_t ) const;
    void compressPath();
};

inline u_long&
Route::operator[]( u_long index )
{
    return path[index];
}

inline Route&
Route::operator=( const Route &rhs )
{
    if ( this != &rhs ) {
        for ( int i = 0 ; i < rhs.length ; ++i )
            path[i] = rhs.path[i];
        
        length        = rhs.length;
        current_index = rhs.current_index;
    }

    return *this;
}

inline void
Route::push_back( u_int32_t node )
{
    assert( length < dsr::MAX_ROUTE_LEN );

    path[length++] = node;
}

inline void
Route::pop_back()
{
    --length;
}

inline bool
Route::latter( u_int32_t node ) const
{
    for ( int i = current_index ; i < length ; ++i )
        if ( path[i] == node ) return true;
    
    return false;
}

#endif // __NCTUNS_DSR_ROUTE_H
