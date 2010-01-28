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

#ifndef __NCTUNS_DSR_ROUTECACHE_H
#define __NCTUNS_DSR_ROUTECACHE_H

#include <vector>

#include "constants.h"
#include "route.h"

class RouteCacheEntry {

friend class RouteCache;

private:
    u_long              ip_addr;
    u_long              size;
    Route               route;
    u_int64_t           timestamp;

public:
    RouteCacheEntry(): ip_addr( dsr::IP_invalid_addr ), size( 0U ) {}
};

class RouteCache {

private:
    RouteCacheEntry               *cache;
    u_long                        index, size;

    int find( u_long node ) const {
        for ( int i = 1 ; i < dsr::MAX_ROUTE_CACHE_ENTRY ; ++i )
            if ( cache[i].ip_addr == node ) return i;
        return 0;
    }

    /*
    // findMin() prefer latest if two or more are found
    u_int8_t findMin( u_int8_t index ) const {

        return 0;
        u_int8_t  entry( 0U );
        u_int64_t time( cache[index].timestamp[0] );
        for ( u_int8_t i = 1U; i < cache[index].size; ++i )
           if ( ( cache[index].route[i].size() < cache[index].route[entry].size()) ||
                ( cache[index].route[i].size() == cache[index].route[entry].size() &&
                  time < cache[index].timestamp[i] )) {
               entry = i;
               time = cache[index].timestamp[i];
           }
       return entry;
    }

    // findMax() prefer oldest if two or more are found
    u_int8_t findMax( u_int8_t index ) const {

        return 0;
        u_int8_t  entry( 0U );
        u_int64_t time( cache[index].timestamp[0] );
        for ( u_int8_t i = 1U; i < cache[index].size; ++i )
           if ( ( cache[index].route[i].size() > cache[index].route[entry].size()) ||
                ( cache[index].route[i].size() == cache[index].route[entry].size() &&
                  time > cache[index].timestamp[i] )) {
               entry = i;
               time = cache[index].timestamp[i];
           }
       return entry;
    } */

public:
    RouteCache(): index( 0U ), size( 1U ) {
        cache = new RouteCacheEntry[dsr::MAX_ROUTE_CACHE_ENTRY];
        cache[0].ip_addr = dsr::IP_invalid_addr;
    }

    void addRoute( Route& );
    bool findRoute( u_long, Route& ) const;
    Route const* findRoute( u_long ) const;
    void erase( u_long );
    void erase( u_long, u_long );
};

inline void
RouteCache::addRoute( Route &route )
{
    // u_long index = find( dst );
    u_long dst( route[route.getLength()-1] );
    u_long index( find( dst ));

    /* if ( dst != route[route.getLength()-1] ) {
        char dst1[20], dst2[20];
        ipv4addr_to_str( dst, dst1 );
        ipv4addr_to_str( route[route.getLength()-1], dst2 );
        cout << dst1 << " != " << dst2 << endl;
        route.dump();
        assert( dst == route[route.getLength()-1] );
    } */
    
    // already has path to destinaion, replace with "route"
    if ( index ) {
        /* u_int8_t entry( cache[index].size );
        // if the cache entry for dst is already full
        if ( entry + 1 >= dsr::CACHE_ENTRY_PER_NODE )
            entry = findMax( index );
        else
            ++cache[index].size;
        */
        
        if ( route.size() <= cache[index].route.size()) {
            route.resetIterator();
            cache[index].route = route;
            cache[index].timestamp = GetCurrentTime();
        }
    } else {
        if ( size < (unsigned int)dsr::MAX_ROUTE_CACHE_ENTRY ) {
            cache[size].ip_addr   = dst;
            cache[size].size      = 1;
            cache[size].route     = route;
            cache[size].timestamp = GetCurrentTime();

            ++size;
        } else {
            cout << "routecche.h is need to be fixed~~~\n";
            // cache[this->index] = entry;
            this->index = ( this->index + 1 ) % dsr::MAX_ROUTE_CACHE_ENTRY;
        }
    }
    return;
}

inline bool
RouteCache::findRoute( u_long dst, Route &route ) const
{
    int index( find( dst ));
    
    if ( index ) {
        // int entry( findMin( index ));
        // route = cache[index].route[entry];

        route = cache[index].route;
        return true;
    }
    else return false;
}

inline Route const*
RouteCache::findRoute( u_long dst ) const
{

    int index( find( dst ));
    
    if ( index ) {
        return &cache[index].route;
    } else {
        return NULL;
    }
}
    
inline void
RouteCache::erase( u_long from, u_long to )
{
    char tmp[20], tmp1[20];
    ipv4addr_to_str( from, tmp );
    ipv4addr_to_str( to, tmp1 );
    cout << "----> from===" << tmp << "===to===" << tmp1 << "===\n";
    
    for ( int i = 1 ; i < dsr::MAX_ROUTE_CACHE_ENTRY ; ++i ) {
        if ( cache[i].ip_addr != dsr::IP_invalid_addr ) {
            for ( int j = 0 ; j < cache[i].route.getLength() - 1 ; ++j ) {
                if (( cache[i].route[j] == from && cache[i].route[j+1] == to ) ||
                   ( cache[i].route[j+1] == from && cache[i].route[j] == to )) {
                    cache[i].ip_addr = dsr::IP_invalid_addr;
                    break;
                }
            }
        }
    }
    return;
}

void
RouteCache::erase( u_long to )
{
    char tmp[20];
    ipv4addr_to_str( to, tmp );
    cout << "----> from=== myself ===to===" << tmp << "===\n";
    
    for ( int i = 1 ; i < dsr::MAX_ROUTE_CACHE_ENTRY ; ++i ) {
        if ( cache[i].ip_addr != dsr::IP_invalid_addr ) {
            if ( cache[i].route[0] == to ) {
                cache[i].ip_addr = dsr::IP_invalid_addr;
                break;
            }
        }
    }
    return;
}

#endif // __NCTUNS_DSR_ROUTECACHE_H
