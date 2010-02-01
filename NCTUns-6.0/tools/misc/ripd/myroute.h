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

/* miscellaneous value */

//#define RIP_PORT							520
#define RIP_PORT							5200
#define MAX_NUM_OF_ENTRIES						255
#define MAX_NUM_OF_ATTACHED_INTERFACES  				8
#define MAX_DATA_SIZE							1300
#define MAX_BUFFER_SIZE							6000
#define INFINITY							16
#define NOT_KNOWN_YET							-1
#define NOT_VALID							-1
#define JUST_START_UP							0	
#define AFTER_START_UP							1
#define MAX_HOST_NAME_SIZE						256
#define MAX_SUBNET_SIZE                 				5
#define UP		  		    				1
#define DOWN								0

#define SIMPLE_SPLIT_HORIZON						1
#define SPLIT_HORIZON_WITH_POISONED_REVERSE 				2

#define REAL_NETWORK 	1
#define NCTUNS		2
#define ADD          	1
#define DEL          	2
#define CHANGE		3

#define NUM_OF_INTERVALS_A_CHECK	2	/* specify how many intervals a time to take alivness check 
                                                 * 6 is defined in spec. however, we select 2 to be easy to observe.
                                                 */
#include "unp.h"
/* myroute.h contains data structures that RIP needed 

 According to rfc2453,
 The RIP packet format is:

       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      |  command (1)  |  version (1)  |       must be zero (2)        |
      +---------------+---------------+-------------------------------+
      |                                                               |
      ~                         RIP Entry (20)                        ~
      |                                                               |
      +---------------+---------------+---------------+---------------+
      
      There may be between 1 and 25 (inclusive) RIP entries.  A RIP-1 entry
      has the following format:

       0                   1                   2                   3
       0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
      +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
      | address family identifier (2) |      must be zero (2)         |
      +-------------------------------+-------------------------------+
      |                        IPv4 address (4)                       |
      +---------------------------------------------------------------+
      |                        must be zero (4)                       |
      +---------------------------------------------------------------+
      |                        must be zero (4)                       |
      +---------------------------------------------------------------+
      |                           metric (4)                          |
      +---------------------------------------------------------------+
      

    The same header format is used for RIP-1 and RIP-2 messages (see
    section 3.4).  The format for the 20-octet route entry (RTE) for
    RIP-2 is:

    0                   1                   2                   3 3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   | Address Family Identifier (2) |        Route Tag (2)          |
   +-------------------------------+-------------------------------+
   |                         IP Address (4)                        |
   +---------------------------------------------------------------+
   |                         Subnet Mask (4)                       |
   +---------------------------------------------------------------+
   |                         Next Hop (4)                          |
   +---------------------------------------------------------------+
   |                         Metric (4)                            |
   +---------------------------------------------------------------+

*/

/* not used */
struct Ripv1Entry
{
    unsigned short	addr_family;
    unsigned short 	must_be_zero1;
    struct in_addr	ipv4;
    unsigned long 	must_be_zero2;
    unsigned long 	must_be_zero3;
    long		metric;
};

struct Ripv2Entry
{
    unsigned short	addr_family;
    unsigned short 	route_tag;
    struct in_addr	ipv4;
    struct in_addr 	subnet_mask;
    struct in_addr 	next_hop;
    unsigned long	metric;
};

struct RipHeader
{
    unsigned char 	command;
    unsigned char 	version;
    unsigned short	routing_domain;
    
    //struct Ripv2Entry * rip_entry_head; /* not practical in network programming */
    /* According to the spec, the maximum
     * number of entry list is 25 */
};

struct RoutingTableEntry
{
    struct in_addr  dest_ipv4;
    struct in_addr  next_hop_ipv4;
    struct in_addr  subnet_mask;
    unsigned long   metric;
    unsigned short  aliveness_metric;
    unsigned char   status;
    int             changed_flag;
};

/* routing table */
struct RoutingTableEntry routing_table[MAX_NUM_OF_ENTRIES];
int max_current_used_index = 0; 

struct RoutingTableEntry* search_routing_entry( struct in_addr ipv4);
int    update_entry( struct RoutingTableEntry new_entry );
int    add_entry( struct RoutingTableEntry new_entry );                        
int    delete_entry( struct in_addr ipv4 );
int    get_nexthop_metric( struct in_addr nexthop_ipv4);
int    dump_entry();

