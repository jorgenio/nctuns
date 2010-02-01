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

/*
 * route.c:
 * a router software that uses distant vector and split horizon 
 * with poisoned reverse algorithm over NCTUns.
 *
 * NOTICE: This simple version daemon limits the maximum number of nodes 
 *         in the topology. The maximum size of routing table is specified 
 *         by MAX_NUM_OF_ENTRIES, which is defined in myroute.h. Someone
 *         needing to simulate a large topology in which there are more than 
 *         255 nodes should enlarge the value of this macro definition.
 *      
 *         Any other pre-defined size for each table are contained in myroute.h.
 *         Please refer to them , if needed.
 *  
 */

#include <stdio.h>
#include <signal.h>
#include "myroute.h"
#include "unproute.h"
#include "unp.h"
#include <nctuns_syscall.h>

//#define MY_DEBUG
#define false	0
#define true	1

/* for attached interfaces */
int    num_of_attached_interfaces;
struct in_addr attached_interfaces[MAX_NUM_OF_ATTACHED_INTERFACES];
int    attached_if_states[MAX_NUM_OF_ATTACHED_INTERFACES];	
   
int sockfd , mode=30;      /* 30 specifies periodic mode with interval 30 secs */
/* Timer-related variables */
int interval = 1 ;         /* the smallest granularity is usually one sec */    	
/* periodic advertising interval */
int advertising_interval=30;
int tick_cnt = 0 ; 
int my_tick_cnt = 0 ; 
/* two counters used in trigger update mode */
int regular_event_cnt=30;  /* used for specifying an interval to send regular advertisement */
int trigger_event_cnt=-1;  /* used for delaying to send triggered event */

/* broadcasting function */
int broadcast(int sockfd , char* msg , int msglen , struct in_addr dest_ip , const char* netmask );
long randomize_trigger_event_cnt(int interval);

/**** funtions for routing table manipulation ****/
int NCTUns_rt_entry_make( struct in_addr src , struct in_addr dst , struct in_addr *ssdd_fmt );
int system_rt_entry_manipulate( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env );
int system_rt_entry_manipulate_for_local_if_initialize( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env );
void make_ssdd(char *ssdd, const char *ip1, const char *ip2);
int is_tun_up(unsigned long int s_addr); 
void get_attached_if_states();
void dump_attached_if_states();
void check_attached_if_states();
void show_my_ip();

/* err_quit( char* str , unsigned int exit_value ) */
int err_quit( const char* str , unsigned int exit_value )
{    
    fprintf( stderr , "%s\n", str );
    exit( exit_value );
    return 1;
}
/* The reture value indicates if the IP is my interfaces. */
int is_my_interfaces(struct in_addr ipv4)
{
    int i;
    for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    {
       if ( ipv4.s_addr == attached_interfaces[i].s_addr )
           return i;
       else 
           ;  
    }
    return -1;
}

int get_connected_my_interfaces(struct in_addr ipv4 , unsigned long netmask )
{
    int i;
    struct in_addr ipv4_netid;
    ipv4_netid.s_addr = ( ipv4.s_addr & netmask );
    for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    {
       struct in_addr local_if_netid;
       local_if_netid.s_addr = ( attached_interfaces[i].s_addr & netmask );
       if ( ipv4_netid.s_addr == local_if_netid.s_addr )
           return i;
       else 
           ;  
    }
    return -1;
}	

struct RoutingTableEntry* search_routing_entry( struct in_addr ipv4)
{ 
   int i;	
   for ( i=0 ; i<=max_current_used_index ; i++ )  
   {
   	if ( !memcmp( &ipv4 , &routing_table[i].dest_ipv4 , sizeof(ipv4) ) ) /* search hits */
   	   return &routing_table[i];
   }   
   
   return NULL; /* the specific entry would not be found */
}

int    get_nexthop_metric( struct in_addr nexthop_ipv4)
{
      struct RoutingTableEntry* rt_entry_handle=NULL;
      rt_entry_handle = search_routing_entry( nexthop_ipv4 );
      if ( !rt_entry_handle )
          return INFINITY;
      else
          return rt_entry_handle->metric;
}

int    add_entry( struct RoutingTableEntry new_entry )
{ 
    /* the first entry (routing_table[0]) is reserved for default route */ 	 
    routing_table[++max_current_used_index].dest_ipv4.s_addr   = new_entry.dest_ipv4.s_addr;
    routing_table[max_current_used_index].next_hop_ipv4.s_addr = new_entry.next_hop_ipv4.s_addr;
    routing_table[max_current_used_index].metric               = new_entry.metric;
    ++routing_table[max_current_used_index].aliveness_metric;
    routing_table[max_current_used_index].changed_flag = 1 ;
    trigger_event_cnt = randomize_trigger_event_cnt(5);
    
    if ( new_entry.metric == INFINITY )
       routing_table[max_current_used_index].status = DOWN ;
    else
       routing_table[max_current_used_index].status = UP ;
    return 1;
}

/* INPUT PROCESSING */
int update_entry( struct RoutingTableEntry new_entry ) {

    struct RoutingTableEntry* rt_entry_handle;    
    int  index = -1;
    unsigned long netmask;
    netmask = htonl( 0xffffff00 );
    
    if ( !(rt_entry_handle=search_routing_entry( new_entry.dest_ipv4)) ) {

    	/* it is a new entry */
    	/* The first entry (routing_table[0]) is reserved for the default route */

    	routing_table[++max_current_used_index].dest_ipv4.s_addr   = new_entry.dest_ipv4.s_addr;
    	routing_table[max_current_used_index].next_hop_ipv4.s_addr = new_entry.next_hop_ipv4.s_addr;
    	++routing_table[max_current_used_index].aliveness_metric;
    	
    	/* add the cost as the obtained value by 1 
    	 * because the route information is obtained through a neighber router 
    	 */
	if ( new_entry.metric >= INFINITY ) {

            routing_table[max_current_used_index].metric = INFINITY ;
	    routing_table[max_current_used_index].status = DOWN ;

	}   
	else {

	    routing_table[max_current_used_index].metric = new_entry.metric + 1 ; 
	    routing_table[max_current_used_index].status = UP ;

	}

        routing_table[max_current_used_index].changed_flag = 1 ; 
        trigger_event_cnt = randomize_trigger_event_cnt(5);

        if ( (index=get_connected_my_interfaces(new_entry.dest_ipv4,netmask)) <0 ) {

            for (index=0 ; index<num_of_attached_interfaces ; ++index ) {

        	    system_rt_entry_manipulate( CHANGE , attached_interfaces[index] , 
        	           routing_table[max_current_used_index].dest_ipv4 , routing_table[max_current_used_index].next_hop_ipv4 , NCTUNS );        
    	    }
    	}
    }
    else /* the route information has been established */ {

    	/* if the next hop for the route is not changed, just update */
    	if ( !memcmp(&rt_entry_handle->next_hop_ipv4 , 
    	             &new_entry.next_hop_ipv4 , sizeof(rt_entry_handle->next_hop_ipv4) )  ) {

    	    ++rt_entry_handle->aliveness_metric;
    	    
    	   if ( new_entry.metric != rt_entry_handle->metric ) {

    	       rt_entry_handle->changed_flag = 1 ; 
    	       trigger_event_cnt = randomize_trigger_event_cnt(5);
    	   }
    	   
    	   if ( new_entry.metric >= INFINITY ) {

		if (rt_entry_handle->metric > 1 ) {

		   /* nodes within my one-hop neighborhood should be 
		    * updated by their hello messages. By doing this,
		    * the network convergency can be sped up.
		    */

    	            rt_entry_handle->metric = INFINITY; /* set to the newest value */
    	        
		    if ( rt_entry_handle->status == UP )
    	                rt_entry_handle->changed_flag = 1 ; 
    	            
                    rt_entry_handle->status = DOWN ;

		}
           
	   }
           else {

           	int changed_flag = 0;
           	if ( rt_entry_handle->status == DOWN ) {

           	    changed_flag = 1;
           	    rt_entry_handle->changed_flag = 1 ; 
           	    
           	}

           	rt_entry_handle->status = UP ;  
           	rt_entry_handle->metric = new_entry.metric + 1; /* set to the newest value */
           	
           	if ( changed_flag )
           	{
           	    if ( (index=get_connected_my_interfaces(new_entry.dest_ipv4,netmask)) <0 )
                    {
                         for (index=0 ; index<num_of_attached_interfaces ; ++index )
                            system_rt_entry_manipulate( CHANGE , attached_interfaces[index] , 
        	                    routing_table[max_current_used_index].dest_ipv4 , routing_table[max_current_used_index].next_hop_ipv4 , NCTUNS );        
    	            }
           	}           	
            }
    	}
    	
    	/* if receive a info. of a route having different next hop, 
    	 * add the cost from that route and compute the minimums of the 
    	 * candidates
    	 */
    	else {

    	    /* Need to update the routes: There are two scenarios for this.
    	     *
    	     * (1) the candidate metric is less than the original one explicitly.
    	     * (2) if the candidate metric is the same as the original one, and 
    	     *     the freshness of the original route shows it is not fresh,
    	     *     on the basis of the recommandation in RFC, we could alter the route
    	     *     to the candidate one.
    	     */

    	    int candidate_metric;
    	    unsigned long netmask_ul = htonl( 0xffffff00 );
    	    candidate_metric = new_entry.metric + 1 ;    	    
     
    	    if (rt_entry_handle->metric > 1 ) {

	        /* cclin: one-hop neighboring nodes can be updated via hello messages.
		 * This filtering can prevent the network from routing oscillation.
		 * Although in the current imple., one hop has been the smallest 
		 * link cost, such a checking can eliminate possible erroneous
		 * imple. in the future.
		 */
	    
		    if ( candidate_metric < rt_entry_handle->metric )
		    {
			    //printf("debug: we get a new route whose cost is less than the origin one\n");
			    rt_entry_handle->status = UP;
			    rt_entry_handle->dest_ipv4.s_addr = new_entry.dest_ipv4.s_addr;
			    rt_entry_handle->next_hop_ipv4.s_addr = new_entry.next_hop_ipv4.s_addr;
			    rt_entry_handle->metric = candidate_metric;
			    rt_entry_handle->aliveness_metric = 1 ;    	    	
			    rt_entry_handle->changed_flag = 1 ; 
			    trigger_event_cnt = randomize_trigger_event_cnt(5);
			    if ( (index=get_connected_my_interfaces(rt_entry_handle->dest_ipv4,netmask_ul)) <0 )
			    {
				    for ( index=0 ; index<num_of_attached_interfaces ; ++index )   	    	
				    {
					    system_rt_entry_manipulate( CHANGE , attached_interfaces[index] , rt_entry_handle->dest_ipv4 , 
							    rt_entry_handle->next_hop_ipv4 , NCTUNS );        
				    }
			    }
		    }
		    else if ( candidate_metric == rt_entry_handle->metric )
		    {
			    //rt_entry_handle->status = UP;
			    ; /* ignore the processing of the events of this type. 
			       * however, if the aliveness metric fo the original route is over the high water
			       * mark, we could consider replace it with a new one because it seems that the 
			       * new one is currently alive and the original one is going to be expired.
			       */
		    }    	    
		    else /* else expensive routes */
			    ; /* it is not neccesary to do anything. */    	     
	    }
    	}    	
    }    
    return 1;
}                         
                        
int delete_entry( struct in_addr ipv4 )
{
    struct RoutingTableEntry * rt_entry_handle;
    unsigned long netmask ;
    int index=-1;
    netmask = htonl( 0xffffff00 );
    rt_entry_handle = search_routing_entry( ipv4 );
    if (!rt_entry_handle)
        return -1;
    
    if ( rt_entry_handle->status == UP )
       return 2; /* now it is up!*/
    rt_entry_handle->metric = INFINITY ;
    rt_entry_handle->status = DOWN ;
    rt_entry_handle->changed_flag = 1 ; 
    trigger_event_cnt = randomize_trigger_event_cnt(5);
    if ( (index=get_connected_my_interfaces(ipv4,netmask)) <0)
    {
        for ( index=0 ; index<num_of_attached_interfaces ; ++index )   	    	
        {
    	    system_rt_entry_manipulate( DEL , attached_interfaces[index] , routing_table[max_current_used_index].dest_ipv4 , 
                 routing_table[max_current_used_index].next_hop_ipv4 , NCTUNS );        
        }
    }
    return 1;
}

int dump_entry()
{
    int i;
    char  dest_ip_buf[16] , next_hop_ip_buf[16] ;
    
    printf("\n\n****************************************************\n");
    printf("************* routing table of ");
    show_my_ip();
    printf(" *************\n\n");

    printf("Destination     NextHop         Metric  Status  Freshness       changed\n");

    for ( i=0 ; i<=max_current_used_index; ++i )
    {
    	inet_ntop(AF_INET , &routing_table[i].dest_ipv4 , dest_ip_buf , sizeof(dest_ip_buf) );
    	inet_ntop(AF_INET , &routing_table[i].next_hop_ipv4 , next_hop_ip_buf , sizeof(next_hop_ip_buf) );    	
    	
    	printf("%s\t\t%s\t\t%lu\t", dest_ip_buf , next_hop_ip_buf , routing_table[i].metric);

    	if ( !routing_table[i].status )
    	   printf("DOWN\t");
    	else
    	   printf("UP\t");
    	
    	printf("%d\t\t%d\n", routing_table[i].aliveness_metric, routing_table[i].changed_flag);
    }    

    printf("\n");

    return 1;
}

int dump_one_entry(struct RoutingTableEntry rt_entry)
{
    char  dest_ip_buf[16] , next_hop_ip_buf[16] ;
    
    printf("\nDestination               NextHop         Metric       Status     Freshness\n");
    inet_ntop(AF_INET , &rt_entry.dest_ipv4 , dest_ip_buf , sizeof(dest_ip_buf) );
    inet_ntop(AF_INET , &rt_entry.next_hop_ipv4 , next_hop_ip_buf , sizeof(next_hop_ip_buf) );    	
    	
    printf("%16s   %16s     %5lu", dest_ip_buf , next_hop_ip_buf , rt_entry.metric);
    if ( !rt_entry.status )
       printf("           DOWN    ");
    else
       printf("           UP      ");
    
    printf("%5d\n", rt_entry.aliveness_metric);    
    
    return 1;
}

int dump_ripv2_entry(struct Ripv2Entry entry)
{
    char  dest_ip_buf[16] , next_hop_ip_buf[16] ;
    
    inet_ntop(AF_INET , &entry.ipv4 , dest_ip_buf , sizeof(dest_ip_buf) );
    inet_ntop(AF_INET , &entry.next_hop , next_hop_ip_buf , sizeof(next_hop_ip_buf) );
    printf("%s\t\t%s\t\t%lu\t%d\t%d\n", dest_ip_buf , next_hop_ip_buf , 
         entry.metric , entry.addr_family , entry.route_tag);
    return 1;
}

void clean_routing_table()
{
     exit(1);
     return ;
}

/* FOR OUTPUT PROCESSING */
/* get_the_table_entry() will pack all entries in the routing table or changed_entries depending on
 * the operation specified by arg2 , with split horizon or split horizon with poisoned reverse 
 * methodology. and consider the limit of 25 entries in a UDP packet.
 */

#define WHOLE_TABLE   1
#define CHANGED_ENTRY 2 
char* get_the_table_entry( int mode , int operation , int* index , char* msgbuf ,int* len , struct in_addr neighbor_addr )
{
    struct RipHeader*  header;
    struct Ripv2Entry* entry; 
    int the_same_subnet_flag;
    
    if ( *index > max_current_used_index )
    {
    	  printf("Warning: the index exceeds the range of the routing table\n");
          return NULL; 
    }
    
    bzero( msgbuf , sizeof(msgbuf) );
    header = (struct RipHeader*)msgbuf;
    header->command = 2 ;
    header->version = 2 ;
    
    entry = (struct Ripv2Entry*)(msgbuf+4);        
    
    for ( *len=4 ; (*index)<=max_current_used_index ; ++(*index) )
    {       	
    	the_same_subnet_flag=0;
    	
    	if ( *len >= 504 )    	
    	{
    	    printf("packing route information. The length of packet is more than 504!\n");
    	    break;
    	}	
    	
    	if ( (!routing_table[*index].changed_flag) && (operation == CHANGED_ENTRY) )
    	    continue;    
    	/* examine if the next hop is from the same subnet of the requestor*/    	
    	{
    	     char *subnetmask="255.255.255.0"; /* default class C subnet */
    	     struct in_addr nexthop_netid , requestor_netid ;
             unsigned long class_c_netmask_ul;        	   	
   	     class_c_netmask_ul = inet_addr( subnetmask );
   	     
   	     nexthop_netid.s_addr   = (routing_table[*index].next_hop_ipv4.s_addr & class_c_netmask_ul);
   	     requestor_netid.s_addr = (neighbor_addr.s_addr & class_c_netmask_ul);
   	     
   	     if ( nexthop_netid.s_addr == requestor_netid.s_addr )
   	         the_same_subnet_flag = 1 ;
   	}
    	
    	if (routing_table[*index].status == DOWN )
    	    entry->metric = INFINITY;
    	    
    	else if ( the_same_subnet_flag )
    	{   
    	    /* Don't send the route to the nexthop neighbor */
    	    if ( mode == SIMPLE_SPLIT_HORIZON )
    	        continue;
    	    else if ( mode == SPLIT_HORIZON_WITH_POISONED_REVERSE )
    	        entry->metric = INFINITY;
    	    else
    	         ; /* for the future or the hybrid methodology. */
    	}
    	else
    	    entry->metric = routing_table[*index].metric;
    	     	
    	entry->addr_family = 2 ;
    	entry->route_tag   = 0 ;
    	entry->ipv4        = routing_table[*index].dest_ipv4;
    	entry->subnet_mask = routing_table[*index].subnet_mask;
    	entry->next_hop    = routing_table[*index].next_hop_ipv4;
    	entry++;
    	(*len)+=20;
    }    
    
    #ifdef __DEBUG1
    printf("packet maximum index = %d len=%d dump the msg:\n", *index-1 ,*len);
    {
    	struct Ripv2Entry* entry;
    	int size;
    	entry = (struct Ripv2Entry*)(msgbuf+4);
    	for ( size=4 ; size<*len  ; entry++ , size+=20)
            dump_ripv2_entry( *entry );
    }
    #endif
    
    return msgbuf;
}

int broadcast_advertises(int operation)
{
    char msgbuf[512] ;
    struct RipHeader  *header;
    struct Ripv2Entry *entry;
    
    header = (struct RipHeader*)msgbuf;    
    header->version = 2;
    header->command = 2;
    
    entry = (struct Ripv2Entry*)(msgbuf+4);
    
    {               	 
          const char* netmask="255.255.255.0";
          int   len , res , index , i;
                  
          /* send the response back by unicasting */
          for ( i=0 ; i<num_of_attached_interfaces ; ++i )
          {
               index =1; /* the first entry(element0) is the default route, it should not be sent */
               while(1) 
               {
                    bzero( msgbuf , sizeof(msgbuf) ); 
                    
                    if ( operation == WHOLE_TABLE )
                        get_the_table_entry( SPLIT_HORIZON_WITH_POISONED_REVERSE , WHOLE_TABLE , &index , 
                                      msgbuf , &len , attached_interfaces[i] );
                    else 
                        get_the_table_entry( SPLIT_HORIZON_WITH_POISONED_REVERSE , CHANGED_ENTRY , &index , 
                                      msgbuf , &len , attached_interfaces[i] );
                           
                    res = broadcast( sockfd , msgbuf , len , attached_interfaces[i] , netmask );
                    if ( res<0 ) 
                   {
                        printf("Error: Sending periodic advertising failed\n");
                   }
                   if (index > max_current_used_index )
                           break;
                }/* end of advertising the whole table with split_horizon variant algorithm.*/
          } 
    }     
    return 1;
}

int checking_aliveness()
{
    int i; /* the first entry is reserved for default route though it's not useful under NCTUns */
    for ( i=1 ; i<=max_current_used_index ; ++i ) 
    {    	
    	if ( !routing_table[i].aliveness_metric ) /* if the freshness of the entry is zero, it would be dead in this recently six intervals */
    	{
    	    int j=-1;
    	    if ( (j=is_my_interfaces( routing_table[i].next_hop_ipv4 )) < 0 ) /* the freshness of local interfaces is always zero */
    	    {
    	        routing_table[i].status = DOWN; /* the entry is expired, wait for 60 secs to delete it */
    	        routing_table[i].metric = INFINITY ;
    	    }
    	}
    	#ifdef __DEBUG
    	printf("tick_cnt==%d rt_table[%d].aliveness==%d \n", tick_cnt , i , routing_table[i].aliveness_metric);	
    	#endif 
    	routing_table[i].aliveness_metric = 0 ;    	
    }
    return 1;
}

int delete_expired_entry()
{
    int i; /* the first entry is reserved for default route though it's not useful under NCTUns */
    for ( i=1 ; i<=max_current_used_index ; ++i ) 
    {
    	if ( routing_table[i].status == DOWN )
    	   delete_entry( routing_table[i].dest_ipv4 ); /* after 120 secs of the checking for entries, 
    	                                                * rip daemon should clean the expired entries based on RFC requirment.
    	                                                */
    }	
    return 1;
}

void timer_handler ( int signo )
{    
    /* timer handler operates in either two modes: periodic_ad mode or trigger_update mode depending on the 
     * value of global variable "mode". In periodic ad. mode, the handler sends regular advertisement periodically.
     * the interval is default to 30 sec and would be specified via command argument. In trigger_update mode ,
     * the handler would prefer to send advertisement via regular action. Therefore, the regular advertisement would 
     * be performed first. If the expiration of trigger_event_cnt occurred and regular advertisement has not occurred ,
     * a trigger update could be sent at this moment.
     */    
    check_attached_if_states();

/*
    switch( ++my_tick_cnt ) {
    	case 38:
	case 60:
	case 70:
	case 90:
		dump_entry();
		break;
	default:
		//printf("haha %d\n", my_tick_cnt);
		break;
    }
*/

    if ( (++tick_cnt % advertising_interval) == 0  )
    {
    	 #ifdef MY_DEBUG
	 show_my_ip();
    	 printf(" advertise the whole table on tick_cnt=%d ad_interval=%d\n", tick_cnt , advertising_interval );
    	 #endif
    	 
         broadcast_advertises(WHOLE_TABLE);    
    }
    else if ( (!mode) && (--trigger_event_cnt == 0 ) ) /* if in trigger_update mode , send changed entries */
    {
    	#ifdef MY_DEBUG
	show_my_ip();
     	printf(" advertise the changed table on tick_cnt=%d ad_interval=%d\n", tick_cnt , advertising_interval );
     	#endif
     	
        broadcast_advertises( CHANGED_ENTRY ); 
        trigger_event_cnt = -1 ; 
    }      
    else ;
    
    if ( (tick_cnt % 24 ) == 0 ) /* wait for 120 secs to delete invalid routing entries , now set to 24 secs */
         delete_expired_entry();
    if ( tick_cnt >= ( advertising_interval*NUM_OF_INTERVALS_A_CHECK) ) /* every six interval , we check the aliveness of an entry*/
    {
    	 checking_aliveness();
    	 //dump_entry();
    	 tick_cnt = 0; 
    }
         
    signal( SIGALRM , timer_handler );
    alarm( interval );    
    #ifdef __DEBUG
    printf("handler invoked reg_event_cnt=%d tick_cnt=%d  trig_event_cnt=%d\n", regular_event_cnt , tick_cnt , trigger_event_cnt );
    #endif
    
}

/**** process_msg() deals with messages received from socket ****/

int process_msg( char* msg , int msglen , struct sockaddr_in* dest , int sockfd )
{
    struct RipHeader  *header;
    struct Ripv2Entry *entry;
    
    header = (struct RipHeader*)msg;
    
    /* identify which command of this RIP packet */
    if ( header->version == 1 )
        ;
    else if ( header->version == 2 )
    {
         if ( header->command == 1 ) /* get a request */
         {	
              entry = (struct Ripv2Entry*)&msg[4];
              if ( entry->addr_family == 0 && entry->metric == 16 )
              {               	 
              	 char  buffer[16] , msgbuf[512];
              	 int   len , res , index;
              	 /* this is an initialization request, the daemon
                  * should send the response containing all the routing 
                  * information that it knows. That is, all of the 
                  * routing table
                  */
                 
                  /* I should know which interface of mine connects to this neighber, and add an 
                   * routing entry with ssdd format into the system routing table 
                   */
		/* not used ? 
                  {
                       unsigned long netmask_ul = htonl(0xffffff00);
                       struct in_addr dest_ipv4 = dest->sin_addr;
                       int i;
                       if ( (i=get_connected_my_interfaces(dest_ipv4,netmask_ul)) <0 )
                       {
                           for ( i=0 ; i<num_of_attached_interfaces ; ++i )
                           {                       	    
                       	        system_rt_entry_manipulate( CHANGE , attached_interfaces[i] , dest_ipv4 , attached_interfaces[index] , NCTUNS );
                           }
                       }
                  }
		  */
                  index =1; /* the first entry(element0) is the default route, it should not be sent */
                  
                  /* send the response back by unicasting */
                  while (1)
                  {
                       bzero( msgbuf , sizeof(msgbuf) );
                       get_the_table_entry( SPLIT_HORIZON_WITH_POISONED_REVERSE , WHOLE_TABLE , &index , 
                             msgbuf , &len , dest->sin_addr );                       
                           
                       res = sendto( sockfd , msgbuf , len , 0 , (struct sockaddr*)dest , sizeof(struct sockaddr_in) );
                       if ( res<0 ) 
                       {
                           printf("Error: Sending responses to %s failed\n",
                           inet_ntop( AF_INET , &dest->sin_addr , buffer , sizeof(buffer) ) );
                       }
                       
                       
                       #ifdef MY_DEBUG
                       //printf("Debug: index = %d len=%d dump the msg:\n", index-1 , len );
                       {
    	                    struct Ripv2Entry* entry;
    			    int size;

    			    entry = (struct Ripv2Entry*)(msgbuf+4);

			    printf("[Send the response from ");
			    show_my_ip();
			    printf("]\n");

    			    printf("Destination     NextHop         Metric  Family  Tag\n");
    			    for ( size=4 ; size<len  ; entry++ , size+=20)
            			dump_ripv2_entry( *entry );

			    printf("\n");
        		    fflush(stdout);
    		       }
        	       #endif
                        
                       //printf("send responses with index %d\n", index);
                       
                       if (index > max_current_used_index )
                           break;
                  } 
              	
              }
         }
         
         else if ( header->command == 2 ) /* get a response */
         {
         	 int maxlen , len;
         	 struct Ripv2Entry* entry;
         	 struct RoutingTableEntry rt_entry;
         	 
         	 entry = (struct Ripv2Entry*) (msg+4);
         	 
         	 #ifdef MY_DEBUG
         	 {
         	     char buf[16];
         	     bzero( buf, sizeof(buf) );
		     printf("\n****************************************************\n");
		     show_my_ip();
         	     printf(" get a response from %s containing %d entries\n\n", 
         	         inet_ntop(AF_INET , &dest->sin_addr , buf , sizeof(buf) ) , (msglen-4)/20 );
		 }
         	 #endif
         	 
		 #ifdef MY_DEBUG
    		 printf("Destination     NextHop         Metric  Family  Tag\n");
		 #endif

         	 for ( len=0 , maxlen = msglen - 4 ; len<maxlen ; entry++ , len+=20 )
         	 {
         	 	 /* retrieve information from entries in RIP packet, and fill in fields of 
         	 	  * a new routing entry. The only reason we fill in the routing table entry
         	 	  * first is that the fields of a RIP packet would be evolved and the interface
         	 	  * of a function call has better not altered frequently.
         	 	  */
         	 	 bzero( &rt_entry , sizeof(rt_entry) );
         	 	 rt_entry.dest_ipv4     = entry->ipv4;
         	 	 rt_entry.next_hop_ipv4 = dest->sin_addr; /* the next hop should be changed to neighbor to make sense*/
         	 	 rt_entry.subnet_mask   = entry->subnet_mask;
         	 	 rt_entry.metric        = entry->metric;
         	 	 rt_entry.aliveness_metric =0;
         	 	 rt_entry.status = UP ; /* initially for dumping table */    
		 	 #ifdef MY_DEBUG
			 dump_ripv2_entry(*entry);
			 #endif
                         update_entry(rt_entry);                  
                 }                          
             
                 #ifdef MY_DEBUG
                 dump_entry();
                 fflush(stdout);
                 #endif             
         }
         
         else 
             printf("We get a RIP pkt with command = %d\n", header->command );
    } 
    else 
        printf("unidentified version or not RIP packet\n");

    return 0;
}

/**** main function ****/ 
int main(int argc, char** argv)
{
   /* for UDP and socket */
   struct sockaddr_in server_addr;
   /* misellaneous varibles */
   int    i;   
   
   /* RIP packet */
   struct RipHeader   rip_pkt_header;   
   struct Ripv2Entry  ripv2_entry;
   
   /* check the validity of arguments . If they are valid,
    * parsing them to obtain interfaces attached to the 
    * router. 
    *
    * usage: ./router mode interval num_interfaces interface1_ip [interface2_ip ... etc.]
    */   
   
   if ( argc < 5 )
      err_quit("usage: ./router mode interval num_interfaces interface1_ip [interface2_ip..]\n \
                Two modes could be specified: periodic_ad or trigger_update mode" , 2 );
   
   /* interprete the arguments */
   if ( !strcmp( "trigger_update" , argv[1]) )
   {   	
   	printf("Running on Trigger Update mode\n");
   	mode = 0 ;   	
   }
   else if ( !strcmp("periodic_ad" , argv[1]) )
   {
   	printf("Running on periodic mode\n");        
   	mode =1 ;   	
   }
   else   
   	err_quit("Two modes could be specified: periodic_ad or trigger_update mode" , 2 );   	
   
   if ( (advertising_interval=atoi(argv[2])) <= 0 )
   	err_quit("advertising interval should more than zero sec" , 2 );
   
   else
        printf("specify advertising interval=%d sec\n" , advertising_interval);
   
   if ( ( atoi(argv[3]) ) >=1 )
      num_of_attached_interfaces = atoi(argv[3]);
   else   
      err_quit("a router should have at least 2 interfaces" , 2 );
      
   for ( i=0 ; i<num_of_attached_interfaces ; i++ )
   {
      if ( !argv[i+4] )
      	  err_quit("the number of interfaces doesn't match the number of the specified addresses" , 2 );
      
      fprintf( stdout , "detecting interface %s attached\n", argv[i+4] );
      inet_pton( AF_INET , argv[i+4] , &attached_interfaces[i] );   
   }

   get_attached_if_states();
   //dump_attached_if_states();
   
   /* add system routing entries of local interfaces to inject packets into the correct tun interface. */
   {
       int i , j;       
       for ( i=0 ; i<num_of_attached_interfaces; ++i )
       {       	   
       	   for ( j=0  ; j<num_of_attached_interfaces; ++j)
       	   {
       	        system_rt_entry_manipulate_for_local_if_initialize( CHANGE , attached_interfaces[i] , attached_interfaces[j] , attached_interfaces[j] , NCTUNS );
       	   }
       }       
   }

   /* setup signal handler */
   
   signal( SIGHUP  , clean_routing_table);
   signal( SIGINT  , clean_routing_table);
   signal( SIGTERM , clean_routing_table);
   signal( SIGSEGV , clean_routing_table);
   signal( SIGBUS  , clean_routing_table);

   /* initialize the distance vector table */
   memset(&routing_table,0,sizeof(struct RoutingTableEntry)*MAX_NUM_OF_ENTRIES);
   {
   	struct RoutingTableEntry my_interface;   	   	
   	int i;
   	unsigned long class_c_netmask_ul;        	
   	
   	class_c_netmask_ul = htonl( 0xffffff00 );
   	
   	for (i=0; i<num_of_attached_interfaces; ++i)
   	{      	    
   	    my_interface.dest_ipv4.s_addr     = (attached_interfaces[i].s_addr & class_c_netmask_ul);   	    
   	    my_interface.next_hop_ipv4.s_addr = attached_interfaces[i].s_addr;
   	    my_interface.subnet_mask.s_addr   = class_c_netmask_ul; /* default value */
   	    my_interface.metric               = 0 ;   	    
   	    add_entry(my_interface);   	    
   	    #ifdef __DEBUG
   	    fprintf( stdout , "adding entries whose if_ip addr=%s ",       inet_ntop ( AF_INET , &my_interface.dest_ipv4 , buf , sizeof(buf) ));   	    
   	    fprintf( stdout , "nexthop_subnet=%s ",  inet_ntop ( AF_INET , &my_interface.next_hop_ipv4 , buf , sizeof(buf) ));   	
   	    fprintf( stdout , "netmask=%s\n",         inet_ntop ( AF_INET , &my_interface.subnet_mask , buf , sizeof(buf) ));   	   	    
   	    #endif
   	}   	
   	#ifdef MY_DEBUG
   	dump_entry();
   	#endif
   }   
 
   /* create a socket that will listen RIP_PORT=520 */
   if ( (sockfd = socket(AF_INET , SOCK_DGRAM , 0 )) <0 )
   {
   	fprintf(stderr,"Cannot get socket fd\n");
        exit(2);
   }
   
   /* set SO_BROADCAST option for the socket */
   {
       int value;
       value = 1;
       setsockopt( sockfd , SOL_SOCKET , SO_BROADCAST , &value , sizeof(value) );
   }
   
   bzero(&server_addr , sizeof(server_addr) );   
   server_addr.sin_family      = AF_INET ;
   server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
   server_addr.sin_port        = htons(RIP_PORT);
   
   if ( bind(sockfd, (struct sockaddr*)&server_addr , sizeof(server_addr) ) <0 )
   {
       fprintf(stderr,"Cannot bind the RIP port\n");
       exit(2);
   }   
   
   /* **** Initializing phase *****
    *
    * (1)Send Hello message to neighbors:
    *    A hello message is configured as COMMAND == 1 , 
    *    address family = 0 , Metrics == 16.
    */
    
    rip_pkt_header.command = 1 ;
    rip_pkt_header.version = 2 ;
    rip_pkt_header.routing_domain = 0 ;
    ripv2_entry.addr_family = 0 ;
    ripv2_entry.route_tag   = 0 ; /* just initialize */
    ripv2_entry.ipv4.s_addr = htonl(0); /* just initialize */
    ripv2_entry.subnet_mask.s_addr = htonl(0); /* just initialize */
    ripv2_entry.metric      = 16 ;    
    
    {
    	char  sendbuf[512]; /* The size of a RIP message doesn't exceed 512 bytes */    	
    	const char* class_c_netmask = "255.255.255.0";
    	int i , msgsize;
    	
    	memset ( sendbuf , 0 , sizeof(sendbuf) );
    	memcpy ( sendbuf , &rip_pkt_header , 4 ); /* copy header without the pointer to the 20-byte rip block*/    	
  	memcpy( sendbuf+4 , &ripv2_entry , 20 );     	
    	msgsize = 24;    	
 
    	
    	for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    	{
           int  res;
           res=broadcast(sockfd, sendbuf , msgsize , attached_interfaces[i] , class_c_netmask );           
           if ( res < 0 )
               printf("broadcasting initialization requests failed\n");
	}
    }   /* end of broadcasting initialization requests to neighbors */
   
    /* if this daemon is running on periodic advertising mode.
     * set the timer and service routing for advertising periodically. 
     */
    if ( mode > 0 ) /* periodic mode */
    {	
        signal( SIGALRM , timer_handler ); 	
        alarm( interval );
    }
    else /* trigger update mode */
    {    	
    	regular_event_cnt = advertising_interval;
    	signal( SIGALRM , timer_handler ); 	
        alarm( interval ); /* set interval = 1 , and keep two counters for triggered event and 
                            * periodic advertising (regular events) */
    }
   
    /* **** normal phase ****
    * (2)Obtain the initialization responses from neighbors.
    *    Then receive requests and identify whether it is for 
    *    all of the table or part of of the table. 
    */
    
    {
    	int res , len , msglen;
    	char msgbuf[512];
    	struct sockaddr_in neighbor_addr ;
    	fd_set read_set;
    	
    	FD_ZERO( &read_set );    	
    	
    	printf("RIPD: RIP daemon is running now ......\n");
    	while(1)
    	{
    	    int from_local_if = 0 ;
    	    len = sizeof(neighbor_addr);
    	    FD_SET( sockfd , &read_set ); 
    	    res = select( sockfd+1 , &read_set , NULL , NULL , NULL );
    	    if ( res == -1 )
    	        continue;
    	    
    	    if ( FD_ISSET(sockfd , &read_set) )
    	    {
    	       
    	       bzero( msgbuf , sizeof(msgbuf) );
    	       msglen = recvfrom( sockfd , msgbuf , sizeof(msgbuf) , 0 , 
    	                          (struct sockaddr*)&neighbor_addr , (socklen_t *)&len );
    	     
    	       /* filter the broadcast packet which is sent from my interfaces */
    	       {
    	       	   int i;
    	       	   for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    	       	   {
    	               if ( !memcmp( &attached_interfaces[i] , &neighbor_addr.sin_addr , sizeof(neighbor_addr.sin_addr) ) )
    	               {
    	               	   from_local_if = 1 ;
    	                   break;
    	               }
    	           }
    	       }
    	       if ( from_local_if == 1 ) /* ignore the information sent by myself */
    	           continue;  
    	       
    	       #ifdef __DEBUG
    	       {
    	       	    char addr_buf[16];
    	       	    printf("Get an event from  %s \n", 
                      inet_ntop(AF_INET , &neighbor_addr.sin_addr , addr_buf , sizeof(addr_buf) )  );
    	       }    	       
    	       #endif
    	       
    	       res = process_msg( msgbuf , msglen , &neighbor_addr , sockfd );
    	       if ( res <0 )
    	           printf("RIPD: Some exceptions occurs\n");    	       
    	    }
    	     
    	}/* end of waiting while loop */		
    }/* end of receiving responses from neighbors */
    
   return 1;	
} 
 
int broadcast(int sockfd , char* msg , int msglen , struct in_addr dest_ip , const char* netmask )
{
    struct sockaddr_in broadcast_addr;
    unsigned long subnet_broadcast_stuff_ul;
    
    if ( !msg || !netmask )
       return -2;    
    
    if ( msg <=0 )
       return -3; 
    
    subnet_broadcast_stuff_ul = inet_addr(netmask);
    subnet_broadcast_stuff_ul = (~subnet_broadcast_stuff_ul ) ;
    
    broadcast_addr.sin_family = AF_INET ;
    broadcast_addr.sin_port   = htons(RIP_PORT);
    {
    	int  res;
    	char addr_buf[16];
        
        broadcast_addr.sin_addr.s_addr = (dest_ip.s_addr | subnet_broadcast_stuff_ul);
        
        /*printf("Try Broadcasting message on %s ....\n", 
                      inet_ntop(AF_INET , &broadcast_addr.sin_addr , addr_buf , sizeof(addr_buf) )  );*/
        res = sendto( sockfd , msg , msglen , 0 , (struct sockaddr*)&broadcast_addr , sizeof(broadcast_addr) );
        
        if ( res<0 )
        {
            printf("Broadcasting message on %s failed\n", 
                   inet_ntop(AF_INET , &broadcast_addr.sin_addr , addr_buf , sizeof(addr_buf) )  );
                      
            perror("reason");               
            return -1;
        }
    }
    
    return 1;
}

int system_rt_entry_manipulate( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env )
{
    char command_buf[100] , addr_buf[16];
    int  route_corresponding_if_founded = 0 ; 
    struct in_addr ssdd_fmt , local_corresponding_if , dest_ipv4_netid ;
	char ssdd[32], buf1[32], buf2[32];
    
    {
    	int i;
    	struct in_addr netmask;
    	unsigned long nexthop_netid_ul , local_if_netid_ul;
    	netmask.s_addr = htonl(0xffffff00);
    	nexthop_netid_ul  = ( nexthop_ipv4.s_addr & netmask.s_addr );
  	    dest_ipv4_netid.s_addr   = ( dest_ipv4.s_addr & netmask.s_addr );
    	for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    	{
    	    local_if_netid_ul = ( attached_interfaces[i].s_addr & netmask.s_addr) ;
    	    if ( nexthop_netid_ul == local_if_netid_ul )
    	    {
    	        local_corresponding_if.s_addr = attached_interfaces[i].s_addr;
    	        route_corresponding_if_founded = 1;
    	    }
    	}
    }
    
    if ( !route_corresponding_if_founded )
    {
    	printf("error: the nexthop would not be directly connected to me!\n");
    }
    /* note: the command of ssdd and nexthop is wrong , need to be corrected */
    bzero( command_buf , sizeof(command_buf) );
    strcpy(command_buf , "route ");
    
    if ( command == ADD )
    {             
    	strcat(command_buf , "-n add -net ");    	     
    	     
    	if ( exec_env == REAL_NETWORK)
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4 , addr_buf , sizeof(addr_buf) ) );
    	else
    	{
    	    NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	    strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );
    	}
	strcpy(buf1, addr_buf);
    	     
	strcat(command_buf , " netmask ");
    	strcat(command_buf , "0xffffff00 "); /* default netmask used in NCTUns */
	strcat(command_buf , "gw ");
        inet_ntop( AF_INET , &nexthop_ipv4 , addr_buf , sizeof(addr_buf) );
        strcpy(buf2, addr_buf);
        make_ssdd(ssdd, buf1, buf2);
        //strcat(command_buf, buf2);
        strcat(command_buf, ssdd);
    }	
    else if ( command == DEL )    
    {
	strcat(command_buf, "del -net ");
    	if ( exec_env == REAL_NETWORK )
    	{
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4_netid , addr_buf , sizeof(addr_buf) ) );
        }
        else 
        {	 
        	 unsigned long netmask_ul = htonl( 0xffffff00 ); 
    	     NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	     ssdd_fmt.s_addr = (ssdd_fmt.s_addr & netmask_ul);    	     
    	     strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );    	     
    	}
	strcat(command_buf , " netmask ");
    	strcat(command_buf , "0xffffff00 "); /* default netmask used in NCTUns */    	     
    	
    }    
    else if ( command == CHANGE )
    { 
         /* THE better approach is deleteing first and then add .*/
	strcat(command_buf, "del -net ");
    	 if ( exec_env == REAL_NETWORK )
    	{
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4_netid , addr_buf , sizeof(addr_buf) ) );
        }
        else 
        {	  
        	 unsigned long netmask_ul = htonl( 0xffffff00 ); 
    	     NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	     ssdd_fmt.s_addr = (ssdd_fmt.s_addr & netmask_ul);    	     
    	     strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );    	     
    	}
	strcat(command_buf , " netmask ");
    	 strcat(command_buf , "0xffffff00 >/dev/null 2>&1"); /* default netmask used in NCTUns */    	     
    	 
    	 {
      	     int res;

#ifdef MY_DEBUG
	     printf("**1** pid: %d %s\n", getpid(), command_buf);
#endif

    	     if ( (res=system(command_buf)) <0 )
    	         perror("error:");    	
         }    
         bzero(command_buf , sizeof(command_buf));
         strcpy(command_buf , "route ");
    	 strcat(command_buf , "-n add -net ");
    	 if ( exec_env == REAL_NETWORK)
    	     strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4_netid , addr_buf , sizeof(addr_buf) ) );
    	 else
    	 {
    	     NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	     strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );
    	 }
	strcpy(buf1, addr_buf);

	strcat(command_buf , " netmask ");
    	 strcat(command_buf , "0xffffff00 "); /* default netmask used in NCTUns */
	strcat(command_buf , "gw ");
	inet_ntop( AF_INET , &nexthop_ipv4 , addr_buf , sizeof(addr_buf) );
	strcpy(buf2, addr_buf);
	make_ssdd(ssdd, buf1, buf2);
	//strcat(command_buf, buf2);
	strcat(command_buf, ssdd);
    }    
    else ;    
    {
    	int res;

#ifdef MY_DEBUG
	printf("**2** pid: %d %s\n", getpid(), command_buf);
#endif

    	if ( (res=system(command_buf)) <0 )
    	   perror("error:");    	
    }
    return 1;
}

int system_rt_entry_manipulate_for_local_if_initialize( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env )
{
    char command_buf[100] , addr_buf[16] , *if_name;
    int  route_corresponding_if_founded = 0 ; 
    struct in_addr ssdd_fmt , local_corresponding_if , dest_ipv4_netid;    
    struct ifi_info* head = NULL , *ptr;
    
    head = get_ifi_info( AF_INET , 1 );
    {
    	int i;
    	struct in_addr netmask;
    	unsigned long nexthop_netid_ul , local_if_netid_ul;
    	netmask.s_addr = htonl(0xffffff00);
    	nexthop_netid_ul  = ( nexthop_ipv4.s_addr & netmask.s_addr) ;        
    	dest_ipv4_netid.s_addr     = ( dest_ipv4.s_addr & netmask.s_addr );
    	for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    	{
    	    local_if_netid_ul = ( attached_interfaces[i].s_addr & netmask.s_addr) ;
    	    if ( nexthop_netid_ul == local_if_netid_ul ) /* Nexthop should be interfaces connected directly */
    	    {
    	    	struct sockaddr_in *sa;
    	        local_corresponding_if.s_addr = attached_interfaces[i].s_addr;
    	        route_corresponding_if_founded = 1;
    	        for ( ptr=head  ; ptr!=NULL ; ptr=ptr->ifi_next )   
                {
                     sa = (struct sockaddr_in*)ptr->ifi_addr;
                     if ( sa->sin_addr.s_addr == local_corresponding_if.s_addr )
                     {
                         if_name = ptr->ifi_name;
                         break;
                     }
                }       
    	    }
    	}
    }/* end of block for looking up local interfaces */
    
    if ( !route_corresponding_if_founded )
    {
    	printf("error: the nexthop would not be directly connected to me!\n");
    }
    /* note: the command of ssdd and nexthop is wrong , need to be corrected */
    bzero( command_buf , sizeof(command_buf) );
    strcpy(command_buf , "route ");
    
    if ( command == ADD )
    {             
    	strcat(command_buf , "-n add -net ");    	     
    	     
    	if ( exec_env == REAL_NETWORK)
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4_netid , addr_buf , sizeof(addr_buf) ) );
    	else
    	{
    	    NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	    strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );
    	}
    	     
	strcat(command_buf , " netmask ");
    	strcat(command_buf , "0xffffff00 "); /* default netmask used in NCTUns */
	strcat(command_buf , " dev ");
    	strcat(command_buf , if_name );    	     
    	
    }	
    else if ( command == DEL )    
    {
    	unsigned long netmask_ul = htonl(0xffffff00);
	strcat(command_buf, "del -net ");
    	if ( exec_env == REAL_NETWORK )
    	{
    	    dest_ipv4.s_addr = ( dest_ipv4.s_addr & netmask_ul );
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4_netid , addr_buf , sizeof(addr_buf) ) );
        }
        else 
        {
        	 NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );	  
    	     ssdd_fmt.s_addr = (ssdd_fmt.s_addr & netmask_ul);    	     
    	     strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );    	     
    	}
	strcat(command_buf , " netmask ");
    	strcat(command_buf , "0xffffff00 "); /* default netmask used in NCTUns */    	     
    	
    }    
    else if ( command == CHANGE )
    { 
         /* THE better approach is deleteing first and then add .*/
    	 unsigned long netmask_ul = htonl(0xffffff00);
	strcat(command_buf, "del -net ");
    	 if ( exec_env == REAL_NETWORK )
    	{
    	    dest_ipv4.s_addr = ( dest_ipv4.s_addr & netmask_ul );
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4 , addr_buf , sizeof(addr_buf) ) );
        }
        else 
        {	  
    	     NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	     ssdd_fmt.s_addr = (ssdd_fmt.s_addr & netmask_ul);    	     
    	     strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );    	     
    	}
	strcat(command_buf , " netmask ");
    	 strcat(command_buf , "0xffffff00 >/dev/null 2>&1"); /* default netmask used in NCTUns */    	     
    	 
    	 {
      	     int res;

#ifdef MY_DEBUG
	     printf("**3** pid: %d %s\n", getpid(), command_buf);
#endif

    	     if ( (res=system(command_buf)) <0 )
    	         perror("error:");    	
         }    
         bzero(command_buf , sizeof(command_buf));
         strcpy(command_buf , "route ");
    	 strcat(command_buf , "-n add -net ");
    	 if ( exec_env == REAL_NETWORK)
    	     strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4_netid , addr_buf , sizeof(addr_buf) ) );
    	 else
    	 {
    	     NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	     strcat(command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );
    	 }
	strcat(command_buf , " netmask ");
    	 strcat(command_buf , "0xffffff00 "); /* default netmask used in NCTUns */
	strcat(command_buf , " dev ");
    	 strcat(command_buf , if_name );
    	 
    }    
    else ;    
    {
    	int res;

#ifdef MY_DEBUG
	printf("**4** pid: %d %s\n", getpid(), command_buf);
#endif

    	if ( (res=system(command_buf)) <0 )
    	   perror("error:");    	
    }
    return 1;
}

int NCTUns_rt_entry_make( struct in_addr src , struct in_addr dst , struct in_addr *ssdd_fmt )
{
    char *ss_mask = "255.255.0.0" , *dd_mask="0.0.255.255";
    unsigned long ss_mask_ul , dd_mask_ul;
    
    ss_mask_ul = inet_addr ( ss_mask );
    dd_mask_ul = inet_addr ( dd_mask );    
    ssdd_fmt->s_addr =  ( ( (src.s_addr>>16) & ss_mask_ul ) | (dst.s_addr & dd_mask_ul ) );    
    
    return 1;
}

long randomize_trigger_event_cnt(int interval)
{
   long i ; 
   i= rand()%interval+1 ;
   return i;
}

void make_ssdd(char *ssdd, const char *ip1, const char *ip2)
{
	int i, j;
	int cnt = 0;

	for(i=0; i<strlen(ip1); i++) {
		ssdd[i] = ip1[i];
		if( ip1[i] == '.' ) {
			cnt++;
		}

		if( cnt >= 2 ) {
			break;
		}
	}

	for(j=0, cnt=0; j<strlen(ip2); j++) {
		if( cnt >= 2 ) {
			ssdd[++i] = ip2[j];
			continue;
		}

		if( ip2[j] == '.' ) {
			cnt++;
		}

	}

	ssdd[++i] = '\0';
}

int is_tun_up(unsigned long int s_addr)
{
	struct ifi_info* head = NULL , *ptr;
	int ret = 0, err;

	head = get_ifi_info( AF_INET , 1 );

	ptr = head;
	while( ptr != NULL ) {
		if( s_addr == ((struct sockaddr_in *)ptr->ifi_addr)->sin_addr.s_addr ) {
			break;
		}
		ptr = ptr->ifi_next;
	}

	if( ptr == NULL ) {
		/* should not happen */
		return false;
	}

	err = syscall_NCTUNS_misc(syscall_NSC_misc_GET_TUN, (unsigned long)ptr->ifi_name, (unsigned long)&ret, 0);

	if( ret == 0x1000 ) {
		return true;
	} else if( ret == 0x2000 ) {
		return false;
	} else {
		/* should not happen */;
		printf("syscall not supported: %d\n", ret);
		return false;
	}
}

void get_attached_if_states()
{
	int i;

	for(i=0 ; i<num_of_attached_interfaces; i++) {
		attached_if_states[i] = is_tun_up(attached_interfaces[i].s_addr) ? UP : DOWN;
	}
}

void dump_attached_if_states()
{
	int i;

	for(i=0 ; i<num_of_attached_interfaces; i++) {
		printf("%s: ", inet_ntoa(attached_interfaces[i]));
		printf(attached_if_states[i] == UP ? "UP\n" : "DOWN\n");
	}
}

void check_attached_if_states()
{
	int state;
	int i, j;

	for(i=0 ; i<num_of_attached_interfaces; i++) {
		state = is_tun_up(attached_interfaces[i].s_addr) ? UP : DOWN;
		
		if( attached_if_states[i] == UP && state == DOWN ) {
			// UP -> DOWN, delete the corresponding local routing entry
			unsigned long netmask = htonl(0xffffff00);

			for(j=1; j<=max_current_used_index; j++) {
			/*
				struct in_addr t;
				t.s_addr = attached_interfaces[i].s_addr&netmask;
				printf("%s ", inet_ntoa(t));
				t.s_addr = routing_table[j].next_hop_ipv4.s_addr&netmask;
				printf("%s\n", inet_ntoa(t));
				*/

				if( (attached_interfaces[i].s_addr&netmask) == 
				    (routing_table[j].dest_ipv4.s_addr&netmask) ) {
				    	routing_table[j].status = DOWN;
					routing_table[j].metric = INFINITY;
					routing_table[j].changed_flag = 1;
					trigger_event_cnt = randomize_trigger_event_cnt(5);
					system_rt_entry_manipulate(
						DEL, attached_interfaces[i],
						routing_table[j].dest_ipv4,
						routing_table[j].next_hop_ipv4, NCTUNS);
					break;
				}
			}
		} else if( attached_if_states[i] == DOWN && state == UP ) {
			// DOWN -> UP, recover the corresponding local routing entry
			unsigned long netmask = htonl(0xffffff00);

			for(j=1; j<=max_current_used_index; j++) {
			/*
				struct in_addr t;
				t.s_addr = attached_interfaces[i].s_addr&netmask;
				printf("%s ", inet_ntoa(t));
				t.s_addr = routing_table[j].next_hop_ipv4.s_addr&netmask;
				printf("%s\n", inet_ntoa(t));
				*/

				if( (attached_interfaces[i].s_addr&netmask) == 
				    (routing_table[j].dest_ipv4.s_addr&netmask) ) {
				    	routing_table[j].status = UP;
					routing_table[j].next_hop_ipv4.s_addr = attached_interfaces[i].s_addr;
					routing_table[j].metric = 0;
					routing_table[j].changed_flag = 1;
					trigger_event_cnt = randomize_trigger_event_cnt(5);
					system_rt_entry_manipulate_for_local_if_initialize(
						ADD, attached_interfaces[i],
						attached_interfaces[i],
						attached_interfaces[i], NCTUNS);
					break;
				}
			}
		}

		attached_if_states[i] = state;
	}

	//dump_entry();
	//dump_attached_if_states();
}

void show_my_ip()
{
    char buf[16];
    
    printf("%s", inet_ntop(AF_INET, &attached_interfaces[0].s_addr, buf, sizeof(buf)));
}
