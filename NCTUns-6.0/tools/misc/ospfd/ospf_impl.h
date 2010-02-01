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

#ifndef __OSPF_IMPL
#define __OSPF_IMPL

#include "ospf.h"

/* manipulation methods 
    int set_version(byte version);
    int set_type(byte type);
    int set_router_id(unsigned long rid );
    int set_area_id(ulong areaid );
    int set_checksum(ushort chksum);
    int set_autype(ushort au_type);
    int set_authentication0(ulong au0);
    int set_authentication1(ulong au1);
    
    byte   get_version();
    byte   get_type();
    ulong  get_router_id();
    ulong  get_area_id();
    ushort get_checksum();
    ushort get_autype();
    ulong  get_authentication0();
    ulong  get_authentication1(); */


int OspfPktHeader::set_version(byte version)
{
    this->version = version;
    return 1;
}

int OspfPktHeader::set_packet_length(ushort pkt_len)
{
    this->packet_length = pkt_len;
    return 1;
}

int OspfPktHeader::set_type(byte type)
{
    this->type = type;
    return 1;
}

int OspfPktHeader::set_router_id(ulong rid)
{
    this->router_id = rid;
    return 1;
}

int OspfPktHeader::set_area_id(ulong area_id)
{
    this->area_id = area_id;
    return 1;
}

int OspfPktHeader::set_checksum(ushort chksum)
{
    this->checksum = chksum;
    return 1;
}

int OspfPktHeader::set_autype(ushort au_type)
{
    this->au_type = au_type;
    return 1;
}

int OspfPktHeader::set_authentication0 (ulong au0)
{
    this->authentication0 = au0;
    return 1;
}

int OspfPktHeader::set_authentication1 (ulong au1)
{
    this->authentication1 = au1;
    return 1;
}

byte   OspfPktHeader::get_version()
{
    return this->version;
}

ushort   OspfPktHeader::get_packet_length()
{
    return this->packet_length;
}

byte   OspfPktHeader::get_type()
{
    return this->type;	
}
ulong  OspfPktHeader::get_router_id()
{
    return this->router_id;
}
ulong  OspfPktHeader::get_area_id()
{
    return this->area_id;
}
ushort OspfPktHeader::get_checksum()
{
    return this->checksum;	
}	
ushort OspfPktHeader::get_autype()
{
    return this->au_type;
}	
ulong  OspfPktHeader::get_authentication0()
{
    return this->authentication0;
}
ulong  OspfPktHeader::get_authentication1()
{
    return this->authentication1;
}

char*  OspfPktHeader::header_serialize()
{
    char *buf ;    
    buf = new char[24]; 
        
    memcpy ( &buf[0] , &this->version , 1 );
    memcpy ( &buf[1] , &this->type , 1 );
    memcpy ( &buf[2] , &this->packet_length , 2 );
    memcpy ( &buf[4] , &this->router_id , 4 );
    memcpy ( &buf[8] , &this->area_id   , 4 );
    memcpy ( &buf[12] , &this->checksum , 2 );
    memcpy ( &buf[14] , &this->au_type , 2 );
    memcpy ( &buf[16] , &this->authentication0 , 4 );
    memcpy ( &buf[20] , &this->authentication1 , 4 );
    
    return buf;
}

int OspfPktHeader::header_deserialize(char* buf)
{
    char *ptr;
    
    if ( !buf )
       return -1;
    else
        ptr = buf;    
    
    this->version = ptr[0];
    this->type    = ptr[1];
    this->packet_length   = *(ushort*) (&ptr[2]);
    this->router_id       = *(ulong*)  (&ptr[4]);
    this->area_id         = *(ulong*)  (&ptr[8]);
    this->checksum        = *(ushort*) (&ptr[12]);
    this->au_type         = *(ushort*) (&ptr[14]);
    this->authentication0 = *(ulong*)  (&ptr[16]);
    this->authentication1 = *(ulong*)  (&ptr[20]);    
    
    return 1;
    
}

OspfPktHeader::OspfPktHeader()
{
    this->version = 2 ;
    this->type = 1 ;
    this->packet_length = 0;
    this->router_id = 0;
    this->area_id = 0;
    this->checksum = 0;
    this->au_type = 0;
    this->authentication0 = 0 ;
    this->authentication1 = 0 ;
}




/* routing entry class */
RoutingTableEntry::RoutingTableEntry()
{
    src_ip = 0;
    dest_type = 0 ;
    dest_ip = 0 ;
    addr_mask = 0 ;
    optional_capabilities = 0 ;
    area = 0 ;
    path_type = 0 ;
    cost = 10000 ;
    type2_cost = 0 ;
    link_state_origin = 0 ;
    next_hop = 0 ;
    advertising_router = 0 ;
    next=NULL; 
    prev=NULL; 
}


int NeighborList::add( neighbor_t* neighbor )
{
    neighbor->state = UP; 
    if (!head)
    {
    	if ( !neighbor )    	
    	    return -1;
    	head = neighbor;
    	head->next = NULL;
    	cur = head;    	    	
    }
    else
    {
    	if ( !neighbor )    	
    	    return -1;
    	cur->next = neighbor;
    	cur = cur->next;
    	cur->next = NULL;    	
    }
    
//fprintf(debugfile, "add(): %d -> %d %d\n", cur, cur->neighbor_interfaces, cur->neighbor_num_of_interfaces);

    ++num_of_neighbors;
    return 1;
}
int NeighborList::del( ulong ipv4 ) /* delete from list if interface's ip addr matches */
{
   neighbor_t *ptr , *prev ;   
   
   for ( ptr=head , prev=ptr ; ptr ; prev = ptr , ptr=ptr->next )
   {
   	for ( unsigned int i=0 ; i<ptr->neighbor_num_of_interfaces; ++i )
   	//for ( int i=0 ; i<ptr->cur_neighbor_num_of_interfaces; ++i )
   	{
   	    //if ( ipv4 == ptr->neighbor_interfaces[i] )
	    if ( ipv4 == ptr->neighbor_interface_list[i].ipv4_addr )
   	    {
   	    	prev->next = ptr->next;
		/*
   	    	if ( ptr->neighbor_interfaces )
   	    	   delete ptr->neighbor_interfaces;
		   */
		if ( ptr->neighbor_interface_list )
		   delete ptr->neighbor_interface_list;
   	    	if ( ptr )
   	    	   delete ptr;
   	    	--num_of_neighbors;
   	    	return 1;
   	    }
   	}
   }
   
   return -1;
}
neighbor_t* NeighborList::search( ulong ipv4 )
{
    neighbor_t *ptr ;   
    for ( ptr=head ; ptr ; ptr=ptr->next )
    {
    	if ( ptr->neighbor_ospf_id == ipv4 )
    	     return ptr;
    	     
	//if( !ptr->neighbor_interfaces ) {
	if ( !ptr->neighbor_interface_list ) {
		continue;
	}

	//fprintf(debugfile, "search(): %d -> %d %d\n", ptr, ptr->neighbor_interfaces, ptr->neighbor_num_of_interfaces);
	mydump();

   	for ( unsigned int i=0 ; i<ptr->neighbor_num_of_interfaces ; ++i )
   	//for ( int i=0 ; i<ptr->cur_neighbor_num_of_interfaces ; ++i )
   	{
   	     //if ( ipv4 == ptr->neighbor_interfaces[i] )
	     if ( ipv4 == ptr->neighbor_interface_list[i].ipv4_addr )
   	         return ptr;
   	}
    }
   
    return NULL;
}

int NeighborList::dump( )
{
/*
    neighbor_t* ptr;
    extern FILE *logfile;
    int cnt;
    struct  in_addr addr;
    char buf[16];    
    fprintf( logfile , "*********** neighbor info. ***************\n");
    
    for ( cnt=0 , ptr = head ; ptr ; ptr=ptr->next , ++cnt )       
    {
       addr.s_addr = ptr->neighbor_ospf_id;              
       fprintf( logfile,"nb [%d] id = %s\n" , cnt , inet_ntop( AF_INET , &addr , buf , sizeof(buf) ) );
       fprintf( logfile,"state = %d\n" , ptr->state);
       fprintf( logfile,"inactive_timer = %lu\n" , ptr->inactive_timer );
       fprintf( logfile,"priority = %d\n" , ptr->neighbor_pri);
       addr.s_addr = ptr->neighbor_attached_interface_ipv4;
       fprintf( logfile,"directly connected interface ipv4 = %s\n" , inet_ntop( AF_INET , &addr , buf , sizeof(buf) ) );
       fprintf( logfile,"options = %d\n" , ptr->neighbor_options );
       fprintf( logfile,"num_of_interfaces = %d\n" , ptr->neighbor_num_of_interfaces );
       //fprintf( logfile,"num_of_interfaces = %d\n" , ptr->cur_neighbor_num_of_interfaces );
       
       for ( int i=0 ; i<ptr->neighbor_num_of_interfaces; ++i )
       //for ( int i=0 ; i<ptr->cur_neighbor_num_of_interfaces; ++i )
       {
       	    addr.s_addr = ptr->neighbor_interfaces[i];
            fprintf( logfile , "attached interface ipv4 = %s\n" , inet_ntop( AF_INET , &addr , buf , sizeof(buf) ) );
       }
       
       fprintf( logfile , "\n\n");
       
    }
    */
    
    return 1;
}

void NeighborList::mydump()
{
/*
	neighbor_t* tmp = head;

	fprintf(debugfile, "\n\n==== mydump(%d) ====\n", getpid());
	for( ; tmp; tmp=tmp->next) {
		fprintf(debugfile, "%d %d %d\n", tmp, tmp->neighbor_interfaces, tmp->neighbor_num_of_interfaces);
	}
	fprintf(debugfile, "==== end of mydump() ====\n\n");
	*/
}

int NodeList::dump( int mode )
{
/*
    node_t* ptr;
    int cnt;
    struct  in_addr addr;
    char buf[16];
    extern  FILE* logfile;
    
    fprintf( logfile , "******** node info. **  the number of nodes = %d *********\n" , num_of_nodes );
    for ( cnt=0 , ptr = head ; ptr ; ptr=ptr->next , ++cnt )          
    {      
       addr.s_addr = ptr->ospf_id;
       fprintf( logfile , "node [%d] id = %s\n" , cnt , inet_ntop( AF_INET , &addr , buf , sizeof(buf) ) );
       fprintf( logfile , "state = %d\n" , ptr->state);       
       fprintf( logfile , "LSA seq = %d\n" , ptr->LSA_sequence_num );       
       fprintf( logfile , "Cost =%d\n", ptr->cost);
       fprintf( logfile , "num_of_interfaces = %d\n" , ptr->num_of_interfaces );       
       
       for ( int i=0 ; i<ptr->num_of_interfaces; ++i )
       {
       	    addr.s_addr = ptr->interfaces[i];
            fprintf( logfile , "attached interface ipv4 = %s\n" , inet_ntop( AF_INET , &addr , buf , sizeof(buf) ) );
       }
       
       addr.s_addr = ptr->nexthop;
       fprintf( logfile , "directly-connected or nexthop interface ipv4 = %s\n" , inet_ntop( AF_INET , &addr , buf , sizeof(buf) ) );
       
       if ( mode )
       {
           fprintf( logfile ,"*** node's neighbors ***\n");
           ptr->neighbors->dump();
           fprintf( logfile ,"************************\n");
       }
       
    }
    
    */
    return 1;
}

int NodeList::add( node_t* node )
{
    node->state = UP; 
    node_t *node_p = search(node->ospf_id);

    if( node_p ) {
	printf("================================================= Wrong!!!\n");
	return -1;
    }

    if (!head)
    {
    	if ( !node )    	
    	    return -1;
    	head = node;
    	head->next = NULL;
    	cur = head;    	    	
    }
    else
    {
    	if ( !node )    	
    	    return -1;
    	cur->next = node;
    	cur = cur->next;
    	cur->next = NULL;    	
    }

    ++num_of_nodes;
    return 1;
}

int NodeList::del( ulong ipv4 )
{
    node_t *ptr , *prev ;   
    for ( ptr=head , prev=ptr ; ptr ; prev = ptr , ptr=ptr->next )
    {
   	for ( unsigned int i=0 ; i<ptr->num_of_interfaces; ++i )
   	{
   	    //if ( ipv4 == ptr->interfaces[i] )
	    if ( ipv4 == ptr->interface_list[i].ipv4_addr )
   	    {
   	    	prev->next = ptr->next;
		/*
   	    	if ( ptr->interfaces )
   	    	   delete ptr->interfaces;
		   */
		if ( ptr->interface_list )
		   delete ptr->interface_list;
   	    	if ( ptr )
   	    	   delete ptr;
   	    	   
   	    	--num_of_nodes;
   	    	return 1;
   	    }
   	}
    }
    
    return -1;
}

node_t* NodeList::search( ulong ipv4 )
{
    node_t* ptr;
    //struct in_addr addr;
    //addr.s_addr = ipv4;
    //printf("search ipv4 = %s \n", inet_ntop(AF_INET , &addr , buf , 16) );
 
    for ( ptr=head ; ptr ; ptr=ptr->next )
    {
    	if ( ipv4 == ptr->ospf_id )
             return ptr;
   	for ( unsigned int i=0 ; i<ptr->num_of_interfaces ; ++i )
   	{
   	     //addr.s_addr = ptr->interfaces[i];
             //printf("     interface[%d]=%s \n" , i , inet_ntop( AF_INET , &addr , buf , 16) );   	     
   	     //if ( ipv4 == ptr->interfaces[i] )
	     if ( ipv4 == ptr->interface_list[i].ipv4_addr )
   	     {
   	         return ptr;
   	     }
   	}
    }
   
    return NULL;
}

#endif
