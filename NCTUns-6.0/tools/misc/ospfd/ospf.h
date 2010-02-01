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

#ifndef __OSPF_H
#define __OSPF_H

typedef unsigned long  ulong;
typedef unsigned short ushort;
typedef unsigned char  byte;

#include <stdio.h>
#define MAX_NUM_OF_ENTRIES 100
#define MAX_NUM_OF_ATTACHED_INTERFACES 20
#define OSPF_PORT 4990 /* our specifying */

/* miscellaneous constant definition */
#define DEFAULT_HELLO_INTERVAL 	1
#define DEFAULT_LSA_INTERVAL 	8
#define DEFAULT_START_TO_FUNCTION_TIME 1*DEFAULT_HELLO_INTERVAL
#define INFINITE	10000
#define REAL_NETWORK 	1
#define NCTUNS		2
#define ADD          	1
#define DEL          	2
#define CHANGE		3

#define UP		1
#define DOWN 		0

/* OSPF packet type */
#define HELLO			1
#define DATABASE_DESCRIPTION 	2
#define LINK_STATE_REQUEST	3
#define LINK_STATE_UPDATE	4
#define LINK_STATE_ACK		5
#define MY_HELLO		6
#define MY_LSA			7

FILE	*debugfile;

extern int system_rt_entry_manipulate( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env );

class  OspfPktHeader
{

    private:
    byte		version;
    byte		type;
    unsigned short	packet_length;
    unsigned long	router_id;
    unsigned long       area_id;
    unsigned short	checksum;
    unsigned short	au_type;
    unsigned long 	authentication0;
    unsigned long 	authentication1;
    
    public:
    OspfPktHeader();
    /* manipulation methods */
    int set_version(byte version);
    int set_packet_length(ushort pkt_len);
    int set_type(byte type);
    int set_router_id(ulong rid );
    int set_area_id(ulong area_id );
    int set_checksum(ushort chksum);
    int set_autype(ushort au_type);
    int set_authentication0(ulong au0);
    int set_authentication1(ulong au1);
    
    byte   get_version();
    ushort get_packet_length();
    byte   get_type();
    ulong  get_router_id();
    ulong  get_area_id();
    ushort get_checksum();
    ushort get_autype();
    ulong  get_authentication0();
    ulong  get_authentication1();
    
    char*  header_serialize();
    int    header_deserialize(char* buf);
    
    
};

typedef class RoutingTableEntry
{
    public:
    
    ulong  src_ip;
    ushort dest_type;
    ulong  dest_ip;
    ulong  addr_mask;
    byte   optional_capabilities;
    ulong  area;
    byte   path_type;
    ulong  cost;
    ulong  type2_cost;
    ulong  link_state_origin;
    ulong  next_hop;
    ulong  advertising_router;    
    RoutingTableEntry* prev;
    RoutingTableEntry* next;
    RoutingTableEntry();
    	
}RoutingTableEntry;

class RoutingTable
{
    public:
    RoutingTableEntry  	*head, *cur ;
    ulong 	       	index;
    bool		entry_changed;
    
    RoutingTable()
    {    	
    	head  = NULL;
    	cur   = NULL;
    	index = 0 ;
    	entry_changed = false;
    }
    
    ~RoutingTable()
    {
	if (!cur)
		return;

    	RoutingTableEntry* tmp = cur ;
    	RoutingTableEntry* tmp2 = cur->prev ;

    	for (unsigned int i=0 ; i<index ; ++i )
    	{
    	   if ( tmp ) delete tmp;
    	   tmp = tmp2;
	   if ( tmp ) tmp2 = tmp->prev;
    	}
    }
    
    /* manipulation method */
    void add_entry( ulong src_ip, ulong dest_ip, ulong next_hop , ulong cost ) 
    {
    	if ( !head )
    	{
    	   head = new RoutingTableEntry;
    	   cur  = head;
    	   cur->next = NULL;
    	   cur->prev = NULL;
    	}
    	else
    	{
    	   cur->next = new RoutingTableEntry;
    	   cur->next->prev = cur;
    	   cur = cur->next;
    	   cur->next = NULL;
    	}  
    	
    	++index;
    	cur->src_ip = src_ip;
    	cur->dest_ip = dest_ip;
    	cur->next_hop = next_hop;
    	cur->cost = cost;
    	   
    }
    
    int del_entry( ulong src_ip, ulong dest_ip )
    {
        RoutingTableEntry* rt_handle = NULL ;	
        rt_handle = lookup_entry( src_ip, dest_ip );
        if ( !rt_handle )
        {
            //printf("no entry with this dest_ip found\n");
            return -1;
        }
        else
        {
	    if( rt_handle->next && rt_handle->prev ) {
	    	/* normal case */
	    	rt_handle->next->prev = rt_handle->prev;
            	rt_handle->prev->next = rt_handle->next;

		if( cur == rt_handle ) {
			cur = rt_handle->next;
		}

	    } else if( !rt_handle->next && rt_handle->prev ) {
	    	/* rt_handle is the tail */
	    	rt_handle->prev->next = NULL;

		if( cur == rt_handle ) {
			cur = rt_handle->prev;
		}

	    } else if( rt_handle->next && !rt_handle->prev ) {
		/* rt_handle is the head */
	    	rt_handle->next->prev = NULL;
		head = rt_handle->next;

		if( cur == rt_handle ) {
			cur = rt_handle->next;
		}
		
	    } else {
	    	/* rt_handle is the only one entry */
	    	head = NULL;
		cur = NULL;
	    }

	    delete rt_handle;

	    index--;
/*
            rt_handle->next->prev = rt_handle->prev;
            rt_handle->prev->next = rt_handle->next;
            if ( rt_handle ) delete rt_handle;                        
*/
        }            
	return 0;
    }
    
    /* set the cost of entries with this currently-inavailuable
     * nexthop to INFINITE
     */
    int set_entry_invalid( ulong nexthop )
    {        
        RoutingTableEntry* tmp = head;
    	for ( ulong i=0 ; i<index ; ++i , tmp=tmp->next )
    	{
    	    if ( tmp->next_hop == nexthop )
    	       tmp->cost = INFINITE;
    	    
    	}   
    	return 1;         
    }
    
    int update_entry( ulong src_ip, ulong dest_ip , ulong next_hop, ulong cost )
    {
        RoutingTableEntry* rt_handle = NULL ;	
        rt_handle = lookup_entry( src_ip, dest_ip );
        struct in_addr src_ipv4, dest_ipv4 , next_hop_ipv4;
        src_ipv4.s_addr = src_ip;
        dest_ipv4.s_addr = dest_ip;
        next_hop_ipv4.s_addr = next_hop;
        
        if ( !rt_handle )
        {
            //printf("A new entry is found!\n");
            add_entry( src_ip, dest_ip, next_hop, cost );
            system_rt_entry_manipulate( CHANGE , src_ipv4 , dest_ipv4 , next_hop_ipv4 , NCTUNS );	      
            entry_changed = true;
            return -1;
        }
        else
        {
            if ( rt_handle->next_hop == next_hop ) {
               rt_handle->cost = cost;
	    }
            else /* if a different next hop is found, compare the two costs and choose a better one. */
            {
            	if ( rt_handle->cost < cost ) {
            	    ; /* remain the same route */
		}
            	else
            	{
            	    rt_handle->cost = cost;
            	    rt_handle->next_hop = next_hop;
            	    system_rt_entry_manipulate( CHANGE , src_ipv4 , dest_ipv4 , next_hop_ipv4 , NCTUNS );	      
            	    entry_changed = true;
            	} 
            }            
        }
	return 0;
    }
    
    RoutingTableEntry* lookup_entry( ulong src_ip, ulong dest_ip )
    {
    	RoutingTableEntry* tmp = head;
    	for ( ulong i=0 ; i<index ; ++i , tmp=tmp->next )
    	{
    	    if ( (tmp->src_ip == src_ip) && (tmp->dest_ip == dest_ip) )
    	        return tmp;
    	}
    	
    	return NULL;
    }

    int *dump(ulong my_ospf_id)
    {
    	RoutingTableEntry* tmp = head;
	struct in_addr tmp_addr;

	tmp_addr.s_addr = my_ospf_id;
	printf("===================================\n");
	printf("====== routingtable of %s =======\n\n", inet_ntoa(tmp_addr));

    	for(ulong i=0; i<index; i++, tmp=tmp->next) {
		tmp_addr.s_addr = tmp->src_ip;
		printf("from %s to ", inet_ntoa(tmp_addr));
		tmp_addr.s_addr = tmp->dest_ip;
		printf("%s ", inet_ntoa(tmp_addr));
		tmp_addr.s_addr = tmp->next_hop;
		printf("via %s, cost = %lu\n", inet_ntoa(tmp_addr), tmp->cost);
	}

        return 0;
    }
};

typedef struct HelloPacketPayload
{
     ulong 	netmask;
     ushort 	hello_interval;
     byte	options;
     byte	rtr_pri;
     ulong 	router_dead_interval;
     ulong 	designated_router;
     ulong 	backup_designated_router;
     
     /* neighber list would be appened in main function
      * since it is a variable-length fields.
      */
}hello_pkt_payload_t;

typedef struct my_hello_pkt
{
    ulong   netmask;
    ushort  hello_interval;
    byte    priority;
    byte    options;
    ulong   ospf_id;
    ulong   num_of_interfaces;    
    /* ulong *interfaces;*/
    
} my_hello_pkt_t;

struct DatabaseDescriptionPacket
{
    ushort interface_mtu;
    ushort options;
    ushort setting_bits;
    ulong  dd_sequence_number;
    /* LSA header */
};

typedef struct LinkStateAdvHeader
{ 
    /* fixed size = 17 */
    ushort  type;
    ushort  cost;
    ulong ipv4_src_ospf_id;
    ulong seq_no;
    ulong num_of_interfaces;
    ulong num_of_neighbors;
    /*
    n 4-byte ipv4_src interfaces 
    */
    
    /* 
    n 4-byte ipv4 neighbors's address 
    */
    
} LSA_header_t;


typedef struct If_Addr
{
    byte		state;
    ulong		ipv4_addr;
} If_Addr;

typedef struct Neighbor
{
    byte	state;
    ulong 	inactive_timer; /* if the value of this counter exceeds RouterDeadInterval seconds, the neighbor could be considered as DOWN. */
    ulong   	DD_sequence_number;		/* not used */
    ulong 	neighbor_ospf_id;
    byte 	neighbor_pri;			/* not used */
    ulong 	neighbor_attached_interface_ipv4;
    byte	neighbor_options;		/* not used */
    ulong 	neighbor_num_of_interfaces;
    If_Addr*	neighbor_interface_list;
    struct Neighbor* next;
} neighbor_t; 

class NeighborList
{
    
    neighbor_t*  head;
    neighbor_t*  cur;
    ulong 	 num_of_neighbors;
    public:
    NeighborList()
    {
    	head = NULL;
    	cur = NULL;    
    	num_of_neighbors = 0; 	
    }   
    ~NeighborList()
    {
    	neighbor_t* ptr , *ptr2 ;
    	if ( head )
    	{
            for ( ptr=head ; ptr ; )
            {
                if ( !ptr )
                    break;
            	ptr2 = ptr->next;
            	if ( ptr )
            	{
		/*
            	    if ( ptr->neighbor_interfaces )
            	    //if ( ptr->neighbor_num_of_interfaces && ptr->neighbor_interfaces )
            	       delete ptr->neighbor_interfaces;
		       */
		    if ( ptr->neighbor_interface_list ) {
		    	delete ptr->neighbor_interface_list;
		    }
            	    delete ptr;
            	}
            	ptr = ptr2;
            }    	   
    	}
    }
    
    int add( neighbor_t* neighbor );
    int del( ulong ipv4 );
    neighbor_t* search( ulong ipv4 );    
    neighbor_t*  get_head() { return head; };
    ulong  get_num_of_neighbors() {return num_of_neighbors;};
    int    set_num_of_neighbors(ulong i) {num_of_neighbors=i; return 1;};
    int dump();

void mydump();
    
};

typedef struct Node
{
    byte 		state;
    ulong 		inactive_timer;
    ulong 		LSA_sequence_num;
    ulong 		ospf_id;
    ulong      	 	num_of_interfaces;
    If_Addr*		interface_list;
    NeighborList*      	neighbors;
    ulong 		cost;
    ulong 		nexthop; /* nexthop from here */
    struct Node*  	next;
} node_t;

class NodeList
{
    
    node_t* head;
    node_t* cur;
    ulong   num_of_nodes;

    public:
    NodeList()
    {
    	head = NULL;
    	cur = NULL;   
    	num_of_nodes = 0 ; 	
    }   
    ~NodeList()
    {
    	node_t* ptr , *ptr2 ;
    	if ( head )
    	{
            for ( ptr=head ; ptr ; )
            {
            	if ( !ptr )
            	   break;
            	   
            	ptr2 = ptr->next;
            	
		/*
            	if ( ptr->interfaces )
            	    delete ptr->interfaces;
		    */
		if( ptr->interface_list )
		    delete ptr->interface_list;
            	if ( ptr->neighbors )
            	    delete ptr->neighbors;
            	    
            	delete ptr;
            	ptr = ptr2;
            }    	   
    	}
    }
    
    int add( node_t* node );
    int del( ulong ipv4 );
    node_t* search( ulong ipv4 );
    node_t* get_head() { return head; };
    ulong get_num_of_nodes() { return num_of_nodes; };
    int dump(int mode);
    
};

typedef struct AdjacencyDescriptor
{
    ulong cost;
    ulong nexthop;    
} adjacency_descriptor;

/* used for shortest path tree building */
ulong*  perm ;
ushort* distance ;
ulong*  precede ;
ulong** weight;
ulong*  node_id_mapping;
ulong   cur_table_size = 0;

#define INCLUDED     1
#define NOT_INCLUDED 0
#define INFINITY     10000

#endif
