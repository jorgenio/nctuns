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
 * ospf.cpp:
 * a simplified router program that uses link-state algorithm to manipulate
 * system routing table. Due to some considerations, The details of 
 * implementation does not fully comply with rfc 2328. For example, 
 * (1) This program uses broadcast instead of multicast to send packets. 
 * (2) This program sends OSPF packets via UDP rather than directly over IP.  
 *
 * In the version revised in Nov. , the routing daemon starts and waits 
 * for DEFAULT_START_TO_FUNCTION_TIME period to function to make sure that 
 * it can receive hello packets broadcasted by its active neighbors. 
 * This design helps to increase the speed of convergence of a topology.
 */

#include <stdio.h>
#include <signal.h>
#include <string.h>
#include "unproute.h"
#include "ospf_impl.h"
#include "unp.h"
#include <nctuns_syscall.h>
#include <sys/time.h>

#define MY_DEBUG

/* for attached interfaces */
/*
int    			num_of_attached_interfaces;
struct in_addr		attached_interfaces[MAX_NUM_OF_ATTACHED_INTERFACES];
int			attached_if_status[MAX_NUM_OF_ATTACHED_INTERFACES];
*/
node_t*			myself;
unsigned long  		my_ospf_id;
byte   			start_to_function = false;
byte			topology_changed = false;
//byte			compute_enable = false;
byte			processing_incoming_message = false;
FILE* 			logfile;

/* node list in the atonomous system */
NodeList  	node_list;
static 		ulong seq_no = 1 ; 
int 		single_source_shortest_path_compute ( int src_index );
int 		node_list_expansion();
int 		node_id_mapping_search(ulong ospf_id );
int 		broadcast_hello_msg( int sockfd );
int 		display_precede();
int 		forward_LSA( int sockfd , char* msg_clone , ulong msglen , ulong receiving_if );
int 		flood_LSA( int sockfd );
//int 		dump_LSA( LSA_header_t *t );
int 		show_ip(ulong num);
int 		show_ip(ulong num , FILE* file_handle );
int 		get_nexthop_to_a_specified_node( int srcid , int nodeid ) ;
/* routingtable instance */
RoutingTable routingtable;

int sockfd , mode=30; /* 30 specifies periodic mode with interval 30 secs */
/* Timer-related variables */
int interval = 1 ; /* the smallest granularity is usually one sec */    	
/* periodic advertising interval */
int advertising_interval=30;
static ulong tick_cnt = 0 ; 

/* broadcasting function */
//int broadcast(int sockfd , char* msg , int msglen , struct in_addr dest_ip , const char* netmask );
int broadcast(int sockfd , char* msg , int msglen , ulong dest_ipv4 , const char* netmask );
long randomize_trigger_event_cnt(int interval);

/**** prototypes of funtions for routing table manipulation ****/
int NCTUns_rt_entry_make( struct in_addr src , struct in_addr dst , struct in_addr *ssdd_fmt );
int system_rt_entry_manipulate( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env );
int system_rt_entry_manipulate_for_local_if_initialize( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env );
struct ifi_info* get_ifi_info( int family , int doaliases );

void make_ssdd(char *ssdd, const char *ip1, const char *ip2);
bool is_tun_up(unsigned long int s_addr); 
void get_attached_if_states();
void dump_attached_if_states();
bool is_attached_if_states_changed();
int  num_of_active_attached_if();
void dump_hello_pkt(my_hello_pkt_t *pkt);
void dump_LSA(LSA_header_t *header, ulong from);
void dump_node_list();
void dump_weight();
void dump_info();
bool set_interface_state_in_node_list(ulong except_ipv4, ulong ipv4, byte state);
void restore_local_if_rt_entry(ulong ipv4);
void set_node_interface_state_with_netmask(node_t *n, ulong ipv4, byte state);
void set_neighbor_interface_state(neighbor_t *nb, ulong ipv4, byte state);

/* err_quit( char* str , unsigned int exit_value ) */
int err_quit( const char* str , unsigned int exit_value )
{    
    fprintf( stderr , "%s\n", str );
    exit( exit_value );
    return 1;
}

/* The return value indicates if the ip is associated with my interfaces. */
int is_my_interfaces(struct in_addr ipv4)
{
    /* replace the original global varibles */
    int		num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr	*attached_interfaces = myself->interface_list;

    int		i;

    for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    {       
       if ( ipv4.s_addr == attached_interfaces[i].ipv4_addr )
           return i;
       else 
           fprintf( logfile , "i=%d\n",i);  
    }
    return -1;
}

int get_connected_my_interfaces(struct in_addr ipv4 , unsigned long netmask )
{
    /* replace the original global varibles */
    int		num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr	*attached_interfaces = myself->interface_list;

    int i;
    struct in_addr ipv4_netid;

    ipv4_netid.s_addr = ( ipv4.s_addr & netmask );
    for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    {
       struct in_addr local_if_netid;
       local_if_netid.s_addr = ( attached_interfaces[i].ipv4_addr & netmask );
       
       if ( ipv4_netid.s_addr == local_if_netid.s_addr )
       {   
           return i;
       }
    }
    return -1;
}

ulong get_connected_neighbor_interface( ulong neighbor_ospfid , ulong netmask )
{  
    /* replace the original global varibles */
    int		num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr	*attached_interfaces = myself->interface_list;

    node_t* neighbor_handle = node_list.search( neighbor_ospfid );
    int num_of_neighbor_ifs = neighbor_handle->num_of_interfaces;	

    for ( int i=0 ; i<num_of_attached_interfaces ; ++i )
    {
    	ulong local_if_netid = ( attached_interfaces[i].ipv4_addr & netmask );
    	for ( int j=0 ; j<num_of_neighbor_ifs ; ++j )
    	{
    	     ulong neighbor_ifs_netid   = ( neighbor_handle->interface_list[j].ipv4_addr & netmask );
    	     if ( local_if_netid == neighbor_ifs_netid )
    	         return neighbor_handle->interface_list[j].ipv4_addr;
    	}
    }

    return 0; /* if the ospfid was not found in the neighbor list, use the default route */
}	

void clean_routing_table(int)
{
     if ( perm ) {
        delete perm;
        perm = 0;
     }
     if ( distance ) {
        delete distance;
        distance = 0;
     }     
     if ( precede ) {
        delete precede;
        precede = 0;
     }
     if ( node_id_mapping ) {
        delete node_id_mapping;
	node_id_mapping = 0;
     }
     
     //int num_of_nodes = node_list.get_num_of_nodes();
     
     if ( weight )
     {
         for ( unsigned int i=0 ; i<cur_table_size ; ++i )   
         {
            if ( weight[i] ) {
            	delete weight[i];
	    	weight[i] = NULL;
	    }
         }
         delete weight;
         weight = NULL;
     }    
     fclose(logfile);
     fclose(debugfile);

     exit(1);     
}

/* checking_aliveness() examines the states of node_list() */
int checking_aliveness()
{
    neighbor_t	*nb_handle = myself->neighbors->get_head();
    int		num_of_nbs = myself->neighbors->get_num_of_neighbors();

    for ( int i=0 ; i<num_of_nbs ; ++i , nb_handle=nb_handle->next )
    {
    	if ( nb_handle->inactive_timer > 0 )
    	    --nb_handle->inactive_timer;    	
    	else
    	    continue;   
    	    
    	if ( nb_handle->inactive_timer <= 0 )
    	{    	    
    	    nb_handle->state = DOWN;
    	    topology_changed = true;
        }
    }   

    node_t	*ptr = myself->next;
    //int		num_of_nodes = node_list.get_num_of_nodes();

    for ( ; ptr; ptr=ptr->next )
    {
    	if ( ptr->inactive_timer > 0 )
	    --ptr->inactive_timer;
	else
	    continue;

	if ( ptr->inactive_timer <= 0 )
	{
	    ptr->state = DOWN;
	    topology_changed = true;
	}
    }

    return 1;
}

void timer_handler ( int signo )
{
    bool b = false;

    if ( ++tick_cnt >= 0xffffffff )
    {
    	tick_cnt %= 4;
    }

    switch( tick_cnt ) {
    /*
	case 39:
	case 40:
	case 41:
	case 42:
	case 47:
		printf("[tick_cnt = %d]\n", tick_cnt);
		dump_node_list();
		dump_weight();
		dump_info();
		routingtable.dump(my_ospf_id);
		break;
		*/
/*
	case 12:
	case 20:
	case 30:
	case 40:
	case 41:
	case 42:
	case 47:
	case 55:
	case 75:
	case 100:
	case 104:
	case 120:
		printf("[tick_cnt = %d]\n", tick_cnt);
		dump_node_list();
		dump_weight();
		dump_info();
		routingtable.dump(my_ospf_id);
		break;
*/
		
	default:
		fprintf(debugfile, "[tick_cnt = %lu]\n", tick_cnt);
		//dump_weight();
	/*
		dump_node_list();
		dump_weight();
		dump_info();
		*/
		break;
    }

    if ( processing_incoming_message )
    {
	signal( SIGALRM , timer_handler ); 	
	alarm( interval );
    	fprintf(debugfile, "[timer_handler skipped]\n");
	return;
    }

    //checking_aliveness(); 

    struct in_addr t;
    t.s_addr = my_ospf_id;

    if ( !start_to_function && (tick_cnt >= DEFAULT_START_TO_FUNCTION_TIME) )
    {   
        start_to_function = true;       
	//printf("%s flood LSA!\n", inet_ntoa(t));
	flood_LSA( sockfd );
	b = true;
    }

    if ( start_to_function )
    { 
	bool attached_if_states_changed = is_attached_if_states_changed();

	checking_aliveness();

    	if ( tick_cnt%DEFAULT_LSA_INTERVAL == 0 || attached_if_states_changed || routingtable.entry_changed )
    	{
	    if ( !b ) {
		//printf("%s flood LSA!\n", inet_ntoa(t));
    	        flood_LSA( sockfd );
	    }

	    routingtable.entry_changed = false;
    	}

    	if ( tick_cnt%DEFAULT_HELLO_INTERVAL == 0 )
    	{    	
	    //printf("%s broadcast hello msg!\n", inet_ntoa(t));
       	    broadcast_hello_msg( sockfd );    
	    //checking_aliveness();

/*
    	    if ( start_to_function ) 
     	    {
                checking_aliveness();        
    	    }

            compute_enable = true;
	    */
    	}
    }

    signal( SIGALRM , timer_handler ); 	
    alarm( interval );
}


/**** process_msg() deals with messages received from socket ****/
int process_msg(char* msg, int msglen, struct sockaddr_in* dest, int sockfd)
{
	/* replace the original global varibles */
	int		num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr		*attached_interfaces = myself->interface_list;

	OspfPktHeader	ospf_pkt;    
    	int		version, type;

	if( ospf_pkt.header_deserialize(msg) < 0 ) {
		printf("A pointer points to NULL or unrecognized packet header\n");
		return -1;
	}

	/* Why should I filter it  again? Strange. */
	for(int i=0; i<num_of_attached_interfaces; i++) {
	    if( attached_interfaces[i].ipv4_addr == ospf_pkt.get_router_id() ) {
	    	return 0;
	    }
	}

	version = ospf_pkt.get_version();
	type = ospf_pkt.get_type() ;    

	if( version == 2 ) {
		/* ospf version 2 */

		if( type == 1 ) {
			/* get normal hello packet */
			;
		} else if ( type == MY_HELLO ) {
			/* hello packet contains attached interface ip */
			int		changed = false ;   	   
			my_hello_pkt_t*	pkt;

			pkt = (my_hello_pkt *) (msg + 24);

#ifdef MY_DEBUG
			fprintf(debugfile, "[Receive a hello pkt]\n");
			dump_hello_pkt(pkt);
#endif

			/* ignore the field examination in this simplest version */

			neighbor_t* nb_handle = myself->neighbors->search(pkt->ospf_id);

			if( !nb_handle ) {
				/* receive a new neighbor's hello packet */
#ifdef MY_DEBUG
				fprintf(debugfile, "[A new neighbor's hello pkt!]\n");
#endif

				/* add to my neighbor list */
				neighbor_t* r = new neighbor_t;

				r->state = UP;
				r->inactive_timer = pkt->hello_interval;
				r->DD_sequence_number = 0 ; 
				r->neighbor_ospf_id = pkt->ospf_id;
				r->neighbor_pri = pkt->priority;
				r->neighbor_options = pkt->options;
				r->neighbor_attached_interface_ipv4 = dest->sin_addr.s_addr ;
				r->neighbor_num_of_interfaces = pkt->num_of_interfaces;
				r->neighbor_interface_list = new If_Addr[pkt->num_of_interfaces];	           

				ulong* nb_ipv4_p = (ulong *) (msg + 44);
				byte* nb_state_p = (byte *) (msg + 44 + sizeof(ulong)*pkt->num_of_interfaces);

				for(unsigned int i=0; i<pkt->num_of_interfaces; ++i, nb_ipv4_p++, nb_state_p++)
				{
				     memcpy(&r->neighbor_interface_list[i].ipv4_addr, nb_ipv4_p, 4);
				     memcpy(&r->neighbor_interface_list[i].state, nb_state_p, 1);
				}	           
	           
				if ( myself->neighbors->add( r ) < 0 )
				{
					fprintf(debugfile, "My neighbor adding failed\n");	           
					return -1;
				}

				/* add my neighbor into my node list */
			node_t* ntmp = node_list.search( pkt->ospf_id );
			if( ntmp != NULL ) {
				ntmp->cost = 1;
			} else {

				node_t* node = new node_t;

				node->state = UP;
				node->inactive_timer = 2*DEFAULT_LSA_INTERVAL;
				node->LSA_sequence_num = 0;
				node->ospf_id = pkt->ospf_id;
				node->num_of_interfaces = r->neighbor_num_of_interfaces;
				node->interface_list = new If_Addr[node->num_of_interfaces];
				node->cost = 1;
				node->nexthop = dest->sin_addr.s_addr; /* meaningless field */
				node->next = NULL ;   
				node->neighbors = new NeighborList;                                      

				/* the first entry of neighbor's neighbor list should store my information */
				neighbor_t *r1 = new neighbor_t; 
				r1->state = UP;
				//r1->inactive_timer = pkt->hello_interval;	/* meaningless field */
				r1->inactive_timer = 0;
				r1->neighbor_ospf_id = my_ospf_id;
				//r1->neighbor_attached_interface_ipv4 = 
				r1->neighbor_num_of_interfaces = num_of_attached_interfaces;
				r1->neighbor_interface_list = new If_Addr[ r1->neighbor_num_of_interfaces ];

				for ( int i=0 ; i<num_of_attached_interfaces ; ++i ) {
					r1->neighbor_interface_list[i].ipv4_addr = attached_interfaces[i].ipv4_addr;
					r1->neighbor_interface_list[i].state = attached_interfaces[i].state;
				}
                   
				if ( node->neighbors->add( r1 ) < 0 )
					fprintf(debugfile, "[Error]: adding a new node failed\n");

				for ( unsigned int i=0 ; i<node->num_of_interfaces ; ++i ) {
					node->interface_list[i].ipv4_addr = r->neighbor_interface_list[i].ipv4_addr;
					node->interface_list[i].state = r->neighbor_interface_list[i].state;
				}

				if ( node_list.add( node ) < 0 )
					fprintf(debugfile, "[Error]: adding a new node failed\n");

			    }

				changed = true ;
			} else {
				/* received a hello packet from an already-known host */
#ifdef MY_DEBUG
				fprintf(debugfile, "[A hello pkt from an already-known host]\n");
#endif

				if( nb_handle->state == DOWN ) {
					nb_handle->state = UP;	      	   
					fprintf(debugfile, "Neighbor's state is changed, but its ifs' states are not changed!!\n");
					fprintf(debugfile, "This may be caused by greedy traffic generators, which overwhelm the control packets needed for normal operations.\n");
					for(int i=0; i<num_of_attached_interfaces; i++) {
						restore_local_if_rt_entry(attached_interfaces[i].ipv4_addr);
					}
					changed = true ;
				}

				nb_handle->inactive_timer = pkt->hello_interval ;

				node_t* n = node_list.search( pkt->ospf_id );

				if( n->state == DOWN ) {
					n->state = UP;
					changed = true;
				}
	       	   
				ulong* nb_ipv4_p = (ulong *) (msg + 44);
				byte* nb_state_p = (byte *) (msg + 44 + sizeof(ulong)*pkt->num_of_interfaces);

				bool b;
					
				for(unsigned int i=0; i<pkt->num_of_interfaces; i++, nb_ipv4_p++, nb_state_p++) {
					b = set_interface_state_in_node_list(0, *nb_ipv4_p, *nb_state_p);

					if( b ) {
						changed = b;
					}
				}

			}

			if( !topology_changed ) {
				topology_changed = changed;
			}

		} else if ( type == 2 ) /* get a database description */
			fprintf(logfile,"OSPFD: Receive OSPF packet with type being %d\nThis simplified daemon would not exploit this kind information\n.", type);
		else if ( type == 3 ) /* get a link state request */
			fprintf(logfile,"OSPFD: Receive OSPF packet with type being %d\nThis simplified daemon would not exploit this kind information\n.", type);
		else if ( type == 4 ) /* get a link state update */
			fprintf(logfile,"OSPFD: Receive OSPF packet with type being %d\nThis simplified daemon would not exploit this kind information\n.", type);
		else if ( type == 5 ) /* get a link state ack */
			fprintf(logfile,"OSPFD: Receive OSPF packet with type being %d\nThis simplified daemon would not exploit this kind information\n.", type);
		else if ( type == MY_LSA ) {
			char* msg_clone = new char[msglen];              

			if( !msg_clone ) {
				printf("Error: no available buffer for duplicating LSA content\n");
				return -1;
			}

			bzero(msg_clone, msglen);
			memcpy(msg_clone, msg, msglen);
              
			int recv_index = get_connected_my_interfaces(dest->sin_addr , htonl(0xffffff00) );
			
			if( recv_index < 0 ) {
				printf("Error: No connected recv if is found\n");
				return -1;
			}

			ulong recv_if = attached_interfaces[recv_index].ipv4_addr; 
         
			LSA_header_t *header = (LSA_header_t*) (msg + 24) ;

#ifdef MY_DEBUG
			fprintf(debugfile, "receive LSA from %s\n", inet_ntoa(dest->sin_addr));
			dump_LSA(header, ospf_pkt.get_router_id());
#endif
              
			neighbor_t	*t = myself->neighbors->search(header->ipv4_src_ospf_id);
			node_t		*node = node_list.search( header->ipv4_src_ospf_id );
		
			if ( !node )  {
				/* A new node is found. Abnormal!! */
				return 0;

				printf("[A new node is found -- should not happened]\n");
				node = new node_t;
				node->state = UP ;
				node->inactive_timer = 2*DEFAULT_LSA_INTERVAL;
				node->LSA_sequence_num = header->seq_no ;
				node->ospf_id = header->ipv4_src_ospf_id ;
				node->num_of_interfaces = header->num_of_interfaces ;
				node->interface_list = new If_Addr[node->num_of_interfaces];
				node->cost = header->cost ;
				node->nexthop = dest->sin_addr.s_addr ;
				node->next = NULL ;   
				node->neighbors = new NeighborList ;                   
					 
				/*  	 ipv4_p			 state_p
				 *  __________________________________________________________
				 * |        |           |           |		 |	      |
				 * | header | if_ipv4[] | nb_ipv4[] | if_state[] | nb_state[] |
				 * |________|___________|___________|____________|____________|
				 */

				int 	num_of_ifs = header->num_of_interfaces;
				int	num_of_nbs = header->num_of_neighbors;
				ulong	*ipv4_p = (ulong *) (msg + 44);  
				byte	*state_p = ((byte *) ipv4_p) + 4*(num_of_ifs+num_of_nbs);

				/* the node's interface_list */
				for ( int i=0 ; i<num_of_ifs ; ++i , ++ipv4_p , ++state_p )
				{
					memcpy(&node->interface_list[i].ipv4_addr, ipv4_p, 4);
					memcpy(&node->interface_list[i].state, state_p, 1);
				}
                   
		   		/* the node's neighbors */
				for ( int i=0 ; i< num_of_nbs ; ++i , ++ipv4_p , ++state_p )
				{                   	
					neighbor_t* neighbor_p = new neighbor_t;
					neighbor_p->state = *state_p;
					neighbor_p->inactive_timer = 0;
					neighbor_p->neighbor_ospf_id = *ipv4_p;
					neighbor_p->neighbor_num_of_interfaces = 1;
					neighbor_p->neighbor_interface_list = new If_Addr[neighbor_p->neighbor_num_of_interfaces];
					neighbor_p->next = NULL ; 

					/* should traverse the node_list to discover all the ifs of this node's nbs */
					neighbor_p->neighbor_interface_list[0].ipv4_addr = *ipv4_p;
					neighbor_p->neighbor_interface_list[0].state = *state_p;

					node->neighbors->add( neighbor_p );
								
					/* if the node's neighbor is not in the node list , add it */

					node_t *tmp = node_list.search( neighbor_p->neighbor_ospf_id );

					if ( !tmp )
					{
						tmp = new node_t;
						tmp->state = *state_p;

						if( tmp->state == UP ) {
							tmp->inactive_timer = 2*DEFAULT_LSA_INTERVAL;
						} else {
							tmp->inactive_timer = 0;
						}
						
						tmp->LSA_sequence_num = 0 ;
						tmp->ospf_id = neighbor_p->neighbor_ospf_id ;
						tmp->num_of_interfaces = 1 ;
						tmp->interface_list = new If_Addr[node->num_of_interfaces] ;
						tmp->interface_list[0].ipv4_addr = tmp->ospf_id ;
						tmp->interface_list[0].state = UP ;	/* assume it is UP, problem? */
						tmp->cost = header->cost + 1 ;		/* it is the node's neighbor */
						tmp->nexthop = dest->sin_addr.s_addr ;
						tmp->next = NULL ;   
						tmp->neighbors = new NeighborList ;                            

						neighbor_t* n_p = new neighbor_t;

						n_p->state = UP ;
						n_p->inactive_timer = 0;
						memcpy ( &n_p->neighbor_ospf_id , &header->ipv4_src_ospf_id , 4 );
						n_p->next = NULL ; 
						n_p->neighbor_num_of_interfaces = 0;
						n_p->neighbor_interface_list = NULL;

						tmp->neighbors->add( n_p );

						if ( node_list.add( tmp ) < 0 )
						{    
							fprintf (logfile,"OSPFD of");show_ip(my_ospf_id, logfile);
							fprintf (logfile,": node adding failed\n");
							return -1;                   	
						}
					}
				}

				if ( node_list.add( node ) < 0 )
				{    
					fprintf (debugfile, "Error: node adding failed\n");
					return -1;                   	
				}
				
				topology_changed = true ;                   
				forward_LSA( sockfd , msg_clone , msglen , recv_if );

			} else {
				/* an existing node */
#ifdef MY_DEBUG
				fprintf(debugfile, "An existing node\n");
#endif
				
				int changed = false;
				int node_state_changed = false;


				if ( node->state == DOWN )
				{              	        
					node->state = UP ;
					changed = true ;
					node_state_changed = true;
#ifdef MY_DEBUG
					fprintf(debugfile, "(1)\n");
#endif
				}

				node->inactive_timer = 2*DEFAULT_LSA_INTERVAL;

				/* examine the sequence number :
				 * define the sequnce number as the next one we expect.
				 * the action of increasing seq no. is performed in forward_LSA();
				 */
				if ( (node->LSA_sequence_num < header->seq_no) ||
				     ((node->LSA_sequence_num >= 0xfffffff0) && (header->seq_no <= 10)) ) {

					int 	num_of_ifs = header->num_of_interfaces;
					int	num_of_nbs = header->num_of_neighbors;
					ulong	*ipv4_p = (ulong *) (msg + 44);  
					byte	*state_p = ((byte *) ipv4_p) + 4*(num_of_ifs+num_of_nbs);

					if ( node->num_of_interfaces != (unsigned int)num_of_ifs ) {
						changed = true;
#ifdef MY_DEBUG
						fprintf(debugfile, "(2)\n");
#endif

						/* reset pointers */
						ipv4_p = (ulong *) (msg + 44);  
						state_p = ((byte *) ipv4_p) + 4*(num_of_ifs+num_of_nbs);

						node->num_of_interfaces = num_of_ifs;

						delete node->interface_list;
						node->interface_list = new If_Addr[node->num_of_interfaces];

						for(unsigned int i=0; i<node->num_of_interfaces; i++, ipv4_p++, state_p++)
						{
							/*
							set_interface_state_in_node_list(0, *ipv4_p, *state_p);
							*/
							node->interface_list[i].ipv4_addr = *ipv4_p;
							node->interface_list[i].state = *state_p;
						}	                    

					} else {
						bool if_changed = false;
					
						for(unsigned int i=0; i<node->num_of_interfaces; i++, ipv4_p++, state_p++)
						{
							if ( node->interface_list[i].ipv4_addr != *ipv4_p ||
							     node->interface_list[i].state != *state_p )
							{
								if_changed = true;
								changed = true;
#ifdef MY_DEBUG
								fprintf(debugfile, "(3)\n");
#endif
								break;
							}
						}

						if( if_changed ) {
							//ulong netmask = htonl(0xffffff00);

							/* reset pointers */
							ipv4_p = (ulong *) (msg + 44);  
							state_p = ((byte *) ipv4_p) + 4*(num_of_ifs+num_of_nbs);
							delete node->interface_list;
							node->interface_list = new If_Addr[node->num_of_interfaces];

							for(int i=0; i<num_of_ifs; i++, ipv4_p++, state_p++)
							{
								struct in_addr tmp_addr;
								int idx;

								node->interface_list[i].ipv4_addr = *ipv4_p;
								node->interface_list[i].state = *state_p;

								tmp_addr.s_addr = *ipv4_p;
								idx = get_connected_my_interfaces(
										tmp_addr, htonl(0xffffff00));
								if( t ) {
								    if( idx != -1 ) {
								        /* 
								         * One of its interface is connected with my 
									 * idx-th interface.
								         */
								        t->state = *state_p;

								        if( t->state == DOWN ) {
								            t->inactive_timer = 0;
								        }
									
								        set_neighbor_interface_state(t, *ipv4_p, *state_p);
								    }
								}
							}	                    
						}
					}

					node->LSA_sequence_num = header->seq_no ;

					if( node->cost != header->cost || node->nexthop != dest->sin_addr.s_addr ) {
						if( node->cost > header->cost ) {
							node->cost = header->cost ;
							node->nexthop = dest->sin_addr.s_addr;
							changed = true;
#ifdef MY_DEBUG
							fprintf(debugfile, "(4)\n");
#endif
						}
					}

					//ulong	num_of_nb = node->neighbors->get_num_of_neighbors();

					ipv4_p	= (ulong *) (msg + 44);  
					state_p = ((byte *) ipv4_p) + 4*(num_of_ifs+num_of_nbs);

					ipv4_p += num_of_ifs;
					state_p += num_of_ifs;

         	       	   		neighbor_t* tmp_nb;

       					for ( int i=0 ; i<num_of_nbs ; ++i , ++ipv4_p , ++state_p ) 
       					{              	               
       						tmp_nb = node->neighbors->search(*ipv4_p);

       						if ( tmp_nb ) {
							/* cclin: here, we need consider if
							 * the reported neighboring node of the
							 * node issuing this LSA is my one-hop 
							 * neighbor. If so, ospfd should ignore it
							 * because its state can be obtained from
							 * the node's hello message or LSA. In such
							 * a way, the convergency process can be
							 * completed sooner.
							 */
                                                        node_t* n_handle = node_list.search(tmp_nb->neighbor_ospf_id);

							if (n_handle->cost > 1 ) {
								if( tmp_nb->state != *state_p ) {
									changed = true;
#ifdef MY_DEBUG
									fprintf(debugfile, "(5)\n");
#endif
								}
							}

       							tmp_nb->state = *state_p;
       						
       						} 
						else {
       							changed = true ;
#ifdef MY_DEBUG
							fprintf(debugfile, "(6)\n");
#endif

       							/* a new neighbor should be added into the list */
       							neighbor_t* neighbor_p = new neighbor_t;

       							if ( !neighbor_p )
       							{
       								printf("Error: node adding failed\n");
       								return -1;                   	
       							}

                  	           			bzero( neighbor_p , sizeof(neighbor_t) );
       							neighbor_p->state = *state_p ; 
							neighbor_p->inactive_timer = 0;
       							neighbor_p->neighbor_ospf_id = *ipv4_p ;
       							neighbor_p->next = NULL ; 

       							node->neighbors->add( neighbor_p );                   	                              	           
       							node_t* n_handle = node_list.search( *ipv4_p );

       							if ( n_handle ) {
       								neighbor_p->neighbor_num_of_interfaces = n_handle->num_of_interfaces;
       								neighbor_p->neighbor_interface_list = new If_Addr[neighbor_p->neighbor_num_of_interfaces];
       							
								for(unsigned int j=0; 
       								    j<neighbor_p->neighbor_num_of_interfaces; j++) {
       								    	neighbor_p->neighbor_interface_list[j].ipv4_addr = n_handle->interface_list[j].ipv4_addr;
       								    	neighbor_p->neighbor_interface_list[j].state = n_handle->interface_list[j].state;
       								}
       							
       							} 
							else {
       								neighbor_p->neighbor_num_of_interfaces = 1 ; /* not correct just initial */
       								neighbor_p->neighbor_interface_list = 
									new If_Addr[neighbor_p->neighbor_num_of_interfaces];

       								neighbor_p->neighbor_interface_list[0].ipv4_addr = 
									neighbor_p->neighbor_ospf_id; 

       								neighbor_p->neighbor_interface_list[0].state = UP; /* assumed */

       								n_handle = new node_t;
       								bzero( n_handle , sizeof(node_t) );

       								n_handle->state = UP ;	/* problem? */
       								n_handle->LSA_sequence_num = 0 ;
       								n_handle->ospf_id = *ipv4_p ;
       								n_handle->num_of_interfaces = 1 ; /* initial value */

       								n_handle->interface_list = new If_Addr[node->num_of_interfaces];
       								n_handle->interface_list[0].ipv4_addr = n_handle->ospf_id;
       								n_handle->interface_list[0].state = *state_p;
       												       
       								n_handle->cost = header->cost + 1 ; /* it is the node's neighbor */

       								n_handle->nexthop = dest->sin_addr.s_addr ;
       								n_handle->next = NULL ;   
       								n_handle->neighbors = new NeighborList ;
       																	  
       								neighbor_t* n_p = new neighbor_t;
       								n_p->state = UP ;                   	             
								n_p->inactive_timer = 0;
       								n_p->neighbor_ospf_id = header->ipv4_src_ospf_id ;
       								n_p->next = NULL ; 
       								n_p->neighbor_num_of_interfaces = 0;
								n_p->neighbor_interface_list = NULL;

       								n_handle->neighbors->add( n_p );
       												     
       								if ( node_list.add( n_handle ) < 0 )
       								{     
       									fprintf (debugfile, "Error: node adding failed\n");
       									return -1;                   	
       								}                   	
             	                   			}
             	               			}
             	           		}                      

					node_t		*ptr = myself;	/* head of the node_list */
					neighbor_t	*nb_p;

					for( ; ptr; ptr=ptr->next) {

						nb_p = ptr->neighbors->search(node->ospf_id);

						if( nb_p ) {

							if( nb_p->neighbor_num_of_interfaces != node->num_of_interfaces ) {
							
								if( nb_p->neighbor_interface_list ) {
									delete nb_p->neighbor_interface_list;
								}

								nb_p->neighbor_num_of_interfaces = node->num_of_interfaces;
								nb_p->neighbor_interface_list 
									= new If_Addr[nb_p->neighbor_num_of_interfaces];

								for(unsigned int i=0; i<nb_p->neighbor_num_of_interfaces; i++) {

									nb_p->neighbor_interface_list[i].ipv4_addr =
										node->interface_list[i].ipv4_addr;
									
									nb_p->neighbor_interface_list[i].state =
										node->interface_list[i].state;
								}
							}
						}
					}

		        		forward_LSA( sockfd , msg_clone , msglen , recv_if );

					if( !topology_changed ) {
		            			topology_changed = changed;
					}
                    		}
                    		else
                    		{
#ifdef MY_DEBUG
                       			fprintf(debugfile, "<drop the LSA> expected seq no = %lu , but get seq no = %lu\n", 
                              			node->LSA_sequence_num+1 , header->seq_no );    
#endif
                    		}
              		}
         	}	
         	else 
             		fprintf( logfile,"We get a OSPF pkt with unknown type = %d\n", type );
    	} 
    	else 
        	fprintf(logfile,"unidentified version or not OSPF packet\n");
        
#ifdef MY_DEBUG
	fprintf(debugfile, "\n**** topology changed = %d ****\n\n", topology_changed);
	//dump_node_list();
#endif

	return 1;
}

/**** main function ****/ 
int main(int argc, char** argv)
{
   /* for UDP and socket */
   struct sockaddr_in	server_addr;

   /* replace the original global varibles */
   int    		num_of_attached_interfaces;
   struct in_addr	attached_interfaces[MAX_NUM_OF_ATTACHED_INTERFACES];

   /* misellaneous varibles */
   int    		i;      
   
   /* check the validity of arguments . If they are valid,
    * parse them to obtain interfaces attached to the 
    * router. 
    *
    * usage: ./ospfd num_of_interfaces interface1_ip [interface2_ip ... etc.]
    */   

   if ( argc < 3 )
      err_quit("usage: ./ospfd num_of_interfaces interface1_ip [interface2_ip..]\n" , 2 );
   
   /* interprete the arguments */   
   
   if ( ( atoi(argv[1]) ) >= 1 )
      num_of_attached_interfaces = atoi(argv[1]);
   else   
      err_quit("A router should have at least 2 interfaces.\n" , 2 );
      
   for ( i=0 ; i<num_of_attached_interfaces ; ++i )
   {
      if ( !argv[i+2] )
      	  err_quit("the number of interfaces doesn't match the number of the specified addresses" , 2 );
      
      fprintf( stdout , "detecting interface %s attached\n", argv[i+2] );
      inet_pton( AF_INET , argv[i+2] , &attached_interfaces[i] );
   }

   /* determine which IP address to be router's ospf_id */
   my_ospf_id = attached_interfaces[0].s_addr ;
   for ( int i=1 ; i<num_of_attached_interfaces ; ++i )
   {
   	if ( my_ospf_id > attached_interfaces[i].s_addr )
   	    my_ospf_id = attached_interfaces[i].s_addr;
   }   
   
   struct in_addr ha;
   char debug_filename[50];

   ha.s_addr = my_ospf_id;

   strcpy(debug_filename, inet_ntoa(ha));
   strcat(debug_filename, ".log");
   debugfile = fopen(debug_filename, "w");
        
   /* the first element of my node list is myself !*/
   {
       node_t* node = new node_t;
       node->state = UP ;
       node->LSA_sequence_num = 0 ;
       node->ospf_id = my_ospf_id ;
       node->num_of_interfaces = num_of_attached_interfaces ;
       //node->interfaces = new ulong[node->num_of_interfaces];
       node->interface_list = new If_Addr[node->num_of_interfaces];
       node->cost = 0 ;
       node->nexthop = htonl( 0 ) ;
       node->next = NULL ;   
       node->neighbors = new NeighborList ;                   
                   
       for (unsigned int i=0 ; i<node->num_of_interfaces ; ++i )
       {
           //node->interfaces[i] = attached_interfaces[i].s_addr ;
           node->interface_list[i].ipv4_addr = attached_interfaces[i].s_addr;
       }                
       if ( node_list.add( node ) < 0 )
           fprintf (logfile,"adding myself to node list failed\n");
   }

   myself = node_list.get_head();

   /* add system routing entries of local interfaces to inject packets into the correct tunnel interface. */
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

   /* get attached interfaces' states */
   get_attached_if_states();
   //dump_attached_if_states();

#ifdef MY_DEBUG
   //dump_node_list();
#endif

   /* setup signal handler */
   signal( SIGHUP  , clean_routing_table);
   signal( SIGINT  , clean_routing_table);
   signal( SIGTERM , clean_routing_table);
   signal( SIGSEGV , clean_routing_table);
   signal( SIGBUS  , clean_routing_table);

   
   /* create a socket that will listen OSPF_PORT */
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
   server_addr.sin_port        = htons(OSPF_PORT);
   
   if ( bind(sockfd, (struct sockaddr*)&server_addr , sizeof(server_addr) ) <0 )
   {
       fprintf(stderr,"Cannot bind the OSPF port\n");
       exit(2);
   }   

   /* **** Initializing phase *****
    *
    * (1)Send Hello message to neighbors:
    *
    */
    
    broadcast_hello_msg( sockfd ) ;    	
    signal( SIGALRM , timer_handler ); 	
    alarm( interval );
    
    /* **** normal phase ****
    * (2)Obtain the initialization responses from neighbors.
    *    Then receive requests and identify whether it is for 
    *    all of the table or part of of the table. 
    */
    
    {
    	int res ;
    	unsigned int len , msglen;
    	char msgbuf[512] , log_filename[50];
    	struct sockaddr_in neighbor_addr ;    	
    	fd_set read_set;    	
    	
    	{    	   
    	   char tmp[20];
    	   struct in_addr myospfid;
    	   bzero( log_filename , sizeof(log_filename) );
    	   myospfid.s_addr = my_ospf_id;
    	   strcpy( log_filename , "OSPF_LOG");    	   
    	   strcat( log_filename , inet_ntop( AF_INET , &myospfid , tmp , sizeof(tmp) ) );    	   
    	}

        if ( !(logfile=fopen(log_filename,"w")) )
        {
            perror("OSPFD: FAILED TO CREATE LOG FILE ");
            return -1;           
        }
        else
            printf("open log file OK\n");

    	FD_ZERO( &read_set );
    	
    	printf("\n\nOSPFD: OSPF daemon is running now ......\n\n");
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
    	                          (struct sockaddr*)&neighbor_addr , &len );
    	     
    	       /* filter the broadcast packet which is sent from local interfaces */
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
    	       
	       /* simple lock */
	       processing_incoming_message = true;

    	       res = process_msg( msgbuf , msglen , &neighbor_addr , sockfd );    	       

    	       if ( res <0 )
    	           printf("OSPFD: an error occurs when performing process_msg() \n");    	       
    	 
    	       /* NOTICE:
                * The manipulation of routing table, the node list, and neighbor lists of each node
                * are moved here, to reduce the number of LSAs and to avoid the corruption of these
                * structures if time_handler() and process_msg() compete for manipulating them. The 
                * problem may be occurred in the previous version.
                */
               if ( topology_changed && start_to_function )
               {
                   if ( node_list_expansion() < 0  )
                   {        
                       printf("Warning: Data strucure expansion failed\n"); 
                   }
                   	
                   if ( single_source_shortest_path_compute (0) < 0 )
                   {
                       printf("Warning: Shortest path tree building failed\n");            
                   }
        
		   /* move to timer_handler */
		   /*
                   if ( routingtable.entry_changed )                   
		   {
                       flood_LSA( sockfd );         
		   }
		   */
               }

               //routingtable.entry_changed = false;
               topology_changed = false;
               //compute_enable = false;
	       processing_incoming_message = false;
    	    }    	         	    
    	}/* end of waiting while loop */		
    }/* end of receiving responses from neighbors */
   fclose(logfile);
   fclose(debugfile);
   return 1;	
} 
 
//int broadcast(int sockfd , char* msg , int msglen , struct in_addr dest_ip , const char* netmask )
int broadcast(int sockfd , char* msg , int msglen , ulong dest_ipv4 , const char* netmask )
{
    struct sockaddr_in broadcast_addr;
    unsigned long subnet_broadcast_stuff_ul;
    struct in_addr dest_ip;

    dest_ip.s_addr = dest_ipv4;
    
    if ( !msg || !netmask )
       return -2;    
    
    if ( msg <=0 )
       return -3; 
    
    subnet_broadcast_stuff_ul = inet_addr(netmask);
    subnet_broadcast_stuff_ul = (~subnet_broadcast_stuff_ul ) ;
    
    broadcast_addr.sin_family = AF_INET ;
    broadcast_addr.sin_port   = htons(OSPF_PORT);
    {
    	int res;
    	char addr_buf[16];
        
        broadcast_addr.sin_addr.s_addr = (dest_ip.s_addr | subnet_broadcast_stuff_ul);
        fprintf(debugfile, "len = %d, brd_addr = %s ", msglen, inet_ntoa(broadcast_addr.sin_addr));
        res = sendto( sockfd , msg , msglen , 0 , (struct sockaddr*)&broadcast_addr , sizeof(broadcast_addr) );
	fprintf(debugfile, "res = %d\n", res);
        
        if ( res<0 )
        {
            fprintf(logfile,"Broadcasting message on %s failed\n", 
                   inet_ntop(AF_INET , &broadcast_addr.sin_addr , addr_buf , sizeof(addr_buf) )  );
                      
            perror("reason");               
            return -1;
        }
    }
    
    return 1;
}

int system_rt_entry_manipulate( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env )
{
    /* replace the original global varibles */
    int		num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr	*attached_interfaces = myself->interface_list;

    char command_buf[100] , addr_buf[16];
    int  route_corresponding_if_founded = 0 ; 
    struct in_addr ssdd_fmt , local_corresponding_if , dest_ipv4_netid ;
	char ssdd[32], buf1[32], buf2[32];

    {
	    const char *mask = "0.0.255.0";
	    unsigned long mask_ul, u1, u2, u3;
	    
	    mask_ul = inet_addr ( mask );
/*
	    printf("%s ", inet_ntoa(src_ipv4));
	    printf("%s ", inet_ntoa(dest_ipv4));
	    printf("%s\n", inet_ntoa(nexthop_ipv4));
*/

	    u1 = src_ipv4.s_addr & mask_ul;
	    u2 = dest_ipv4.s_addr & mask_ul;
	    u3 = nexthop_ipv4.s_addr & mask_ul;

	    if( u1 == u2 && u2 == u3 ) {
	    	//printf(" = = \n");
		return 1;
	    }
    }

    {
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;
	const char *mask = "0.0.255.0";
	unsigned long mask_ul, u1, u2, u3;
	    
	mask_ul = inet_addr ( mask );
	u1 = dest_ipv4.s_addr & mask_ul;
	u2 = nexthop_ipv4.s_addr & mask_ul;

    	for(int i=0 ; i<num_of_attached_interfaces; i++) {
		u3 = attached_interfaces[i].ipv4_addr & mask_ul;
		if( u1 == u3 && u1 == u3) {
			//printf(" b_d %s\n", inet_ntoa(dest_ipv4));
			return 1;
		}
	}
    }

    {
    	int i;
    	struct in_addr netmask;
    	unsigned long nexthop_netid_ul , local_if_netid_ul;
    	netmask.s_addr = htonl(0xffffff00);
    	nexthop_netid_ul  = ( nexthop_ipv4.s_addr & netmask.s_addr );
  	    dest_ipv4_netid.s_addr   = ( dest_ipv4.s_addr & netmask.s_addr );
    	for ( i=0 ; i<num_of_attached_interfaces ; ++i )
    	{
    	    local_if_netid_ul = ( attached_interfaces[i].ipv4_addr & netmask.s_addr) ;
    	    if ( nexthop_netid_ul == local_if_netid_ul )
    	    {
    	        local_corresponding_if.s_addr = attached_interfaces[i].ipv4_addr;
    	        route_corresponding_if_founded = 1;
    	    }
    	}
    }
    
    if ( !route_corresponding_if_founded )
    {
    	fprintf(logfile,"error: the nexthop is not directly connected to me!\n");
    	fprintf(logfile,"the entry I just made would be incorrect! please check system rt table.\n");
    }
    /* note: the command of ssdd and nexthop is wrong , need to be corrected */
    bzero( command_buf , sizeof(command_buf) );
    strcpy(command_buf , "route ");
    
    if ( command == ADD )
    {             
    	strcat(command_buf , "-n add -net ");    	     
    	     
    	if ( exec_env == REAL_NETWORK)
	{
    	    strcat( command_buf , inet_ntop( AF_INET , &dest_ipv4 , addr_buf , sizeof(addr_buf) ) );
	}
    	else
    	{
    	    NCTUns_rt_entry_make( src_ipv4 , dest_ipv4_netid , &ssdd_fmt );
    	    strcat( command_buf , inet_ntop( AF_INET , &ssdd_fmt , addr_buf , sizeof(addr_buf) ) );
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
    	strcat(command_buf , "del -net ");
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
    	 strcat(command_buf , "del -net ");
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
	fprintf(debugfile, "**1** pid: %d %s\n", getpid(), command_buf);
#endif
	//printf("**1** %d %s\n", getpid(), command_buf);
    	     if ( (res=system(command_buf)) <0 )
    	         perror("error:");    	


	if( res != 0 ) {
		fprintf(debugfile, "1............... oh =_=\n");
	}
         
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
	fprintf(debugfile, "**2** pid: %d %s\n", getpid(), command_buf);
#endif
	//printf("**2** %d %s\n", getpid(), command_buf);
    	if ( (res=system(command_buf)) <0 )
    	   perror("error:");    	

	if( res != 0 ) {
		fprintf(debugfile, "2............... oh =_=\n");
	}
    }
    return 1;
}

int system_rt_entry_manipulate_for_local_if_initialize( int command , struct in_addr src_ipv4 , struct in_addr dest_ipv4 , struct in_addr nexthop_ipv4 , int exec_env )
{
    /* replace the original global varibles */
    int		num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr	*attached_interfaces = myself->interface_list;

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
    	    local_if_netid_ul = ( attached_interfaces[i].ipv4_addr & netmask.s_addr) ;
    	    if ( nexthop_netid_ul == local_if_netid_ul ) /* Nexthop should be interfaces connected directly */
    	    {
    	    	struct sockaddr_in *sa;
    	        local_corresponding_if.s_addr = attached_interfaces[i].ipv4_addr;
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
    	fprintf(logfile,"error: the nexthop is not directly connected to me!\n");
    	fprintf(logfile,"the entry I just made would be incorrect! please check system rt table.\n");
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
    	strcat(command_buf , " dev " );
    	strcat(command_buf , if_name );    	         	
    }	
    else if ( command == DEL )    
    {
    	unsigned long netmask_ul = htonl(0xffffff00);
    	strcat(command_buf , "del -net ");
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
         /* The better approach is deleteing first and then add .*/
    	 unsigned long netmask_ul = htonl(0xffffff00);
    	strcat(command_buf , "del -net ");
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
	fprintf(debugfile, "**3** pid: %d %s\n", getpid(), command_buf);
#endif
	//printf("**3** %d %s\n", getpid(), command_buf);
    	     if ( (res=system(command_buf)) <0 )
    	         perror("error:");    	


	if( res != 0 ) {
		fprintf(debugfile, "3............... oh =_=\n");
	}

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
    	 strcat(command_buf , " dev " );
    	 strcat(command_buf , if_name );    	 
    }    
    else ;    
    {
    	int res;
#ifdef MY_DEBUG
	fprintf(debugfile, "**4** %s\n", command_buf);
#endif
	//printf("**4** %s %s\n", getpid(), command_buf);
    	if ( (res=system(command_buf)) <0 )
    	   perror("error:");    	


	if( res != 0 ) {
		fprintf(debugfile, "4............... oh =_=\n");
	}

    }
    return 1;
}

int NCTUns_rt_entry_make( struct in_addr src , struct in_addr dst , struct in_addr *ssdd_fmt )
{
    const char *ss_mask = "255.255.0.0" , *dd_mask="0.0.255.255";
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

int finish_computing( ulong *perm , ushort num_of_elem )
{
    for ( int i=0 ; i<num_of_elem ; ++i )
    {
       if ( !perm[i] )
       {
       	   node_t * p = node_list.search( node_id_mapping[i] );
       	   if ( !p )
       	      return true; /* if this occurs , stop computing since the table building is incorrect */
       	      
       	   if ( p->state == DOWN )
       	      continue;
       	   else
              return false ;
       }
    }       
    return true;
}

/* compute the shortest path for a single source to every existing node. 
 *
 * weight[][] array is a table representing the topology graph originated from the 
 * node_list of a topology. We don't operate entries in node_list directly since
 * the computation is possible to be interrupted by timer_handler. To avoid race-
 * condition, we expand the table seperately.
 *
 * perm[] array is used to indicate whether a node has included into the shortest path
 * with source src_index. if all of the nodes are indicated , the computation is completed.
 *
 * distance[] array is used to store the cost from source to a specified node.
 * precede[]  array is used to store the index of the preceding node for node i in the shortest path .
 */


/*  get_nexthop_to_a_specfied_node() traverses the precede[] array, to 
 *  manipulate the routing entries that can be computed by the shortest-path tree.
 */
int get_nexthop_to_a_specified_node( int srcid , int nodeid ) 
{
    int index;

    if ( (index = precede[nodeid]) == INFINITY )
        return -1;
    
    struct in_addr src_ipv4 , dest_ipv4 , nexthop_ipv4;    

    if ( index == srcid )
    {        
        node_t *src_handle = node_list.get_head();
        int num_of_local_ifs = src_handle->num_of_interfaces;            
        node_t *dst_handle = node_list.search( node_id_mapping[nodeid] );
        int num_of_dest_ifs = dst_handle->num_of_interfaces;
        /* if connecting interface is not the one with nodeid, we should 
         * process it 
         */
        
        int nexthop_ospfid = node_id_mapping[nodeid];
        //node_t* nexthop_handle = node_list.search( nexthop_ospfid );        
              
        nexthop_ipv4.s_addr = 
               get_connected_neighbor_interface( nexthop_ospfid , htonl(0xffffff00) );
             
        if ( nexthop_ipv4.s_addr == 0 )
        {
            fprintf(logfile,"Error: no connecting interface found in nexthop. it should be a neighbor!\n");
            return -1;
        }
        
        for ( int i=0 ; i<num_of_local_ifs ; ++i )
        {
       	     //src_ipv4.s_addr = src_handle->interfaces[i];
       	     src_ipv4.s_addr = src_handle->interface_list[i].ipv4_addr;
             for ( int j=0 ; j<num_of_dest_ifs ; ++j )
             {
            	 dest_ipv4.s_addr = dst_handle->interface_list[j].ipv4_addr;
            	 #ifdef __DEBUG
            	 fprintf( logfile , "src if = ");show_ip(src_ipv4.s_addr , logfile );
            	 fprintf( logfile , "dst if = ");show_ip(dest_ipv4.s_addr, logfile );fprintf(logfile,"\n");
            	 #endif
            	 
            	 if ( get_connected_my_interfaces (dest_ipv4 , htonl(0xffffff00) ) < 0 ) 
            	 {
                     //system_rt_entry_manipulate( CHANGE , src_ipv4 , dest_ipv4 , nexthop_ipv4 , NCTUNS );	      
                    
            	     dest_ipv4.s_addr = dst_handle->interface_list[j].ipv4_addr;
		     /*
		     printf("[1]rt: from %s", inet_ntoa(src_ipv4));
		     printf(" to %s", inet_ntoa(dest_ipv4));
		     printf(" via %s cost = %d\n", inet_ntoa(nexthop_ipv4), distance[nodeid]);
		     */
                     //routingtable.update_entry(src_ipv4.s_addr, dest_ipv4.s_addr, nexthop_ipv4.s_addr, distance[i]);
                     routingtable.update_entry(src_ipv4.s_addr, dest_ipv4.s_addr, nexthop_ipv4.s_addr, distance[nodeid]);
                 }
             }
        } 
        
        return 1;     	
    }
    else
        ;
    
    int compute_cnt = 0 ;        
        
    while ( 1 )
    {        
        if ( precede[index] == (ulong)srcid )
        {
            int nexthop_ospfid = node_id_mapping[index];
            node_t* nexthop_handle = node_list.search( nexthop_ospfid );        
            nexthop_ipv4.s_addr = 0 ;
        
            for ( unsigned int i=0 ; i< nexthop_handle->num_of_interfaces ; ++i )
            {
               nexthop_ipv4.s_addr = 
                   //get_connected_neighbor_interface( nexthop_handle->interfaces[i], htonl(0xffffff00) );
                   get_connected_neighbor_interface( nexthop_handle->interface_list[i].ipv4_addr, htonl(0xffffff00) );
               if ( nexthop_ipv4.s_addr !=0 )
                  break;
            }
            
            /* need to two level loop to manipulate all src interfaces versus all
             * dest interfaces entries */
            
            node_t *src_handle = node_list.get_head();
            int num_of_local_ifs = src_handle->num_of_interfaces;            
            node_t *dst_handle = node_list.search( node_id_mapping[nodeid] );
            int num_of_dest_ifs = dst_handle->num_of_interfaces;
             
            for ( int i=0 ; i<num_of_local_ifs ; ++i )
            {
            	//src_ipv4.s_addr = src_handle->interfaces[i];
            	src_ipv4.s_addr = src_handle->interface_list[i].ipv4_addr;
            	for ( int j=0 ; j<num_of_dest_ifs ; ++j )
            	{
            	     dest_ipv4.s_addr = dst_handle->interface_list[j].ipv4_addr;
		     /*
		     printf("[2]rt: from %s", inet_ntoa(src_ipv4));
		     printf(" to %s", inet_ntoa(dest_ipv4));
		     printf(" via %s cost = %d\n", inet_ntoa(nexthop_ipv4), distance[nodeid]);
		     */
                     //system_rt_entry_manipulate( CHANGE , src_ipv4 , dest_ipv4 , nexthop_ipv4 , NCTUNS );	      
                     //routingtable.update_entry( src_ipv4.s_addr , dest_ipv4.s_addr , nexthop_ipv4.s_addr , distance[i] );
                     
		     routingtable.del_entry(src_ipv4.s_addr, dest_ipv4.s_addr);
                     routingtable.update_entry( src_ipv4.s_addr , dest_ipv4.s_addr , nexthop_ipv4.s_addr , distance[nodeid] );
            	}
            }      	
            
            return index;
        }
        index = precede[index];
        if ( index == INFINITY )
        {
            fprintf(logfile,"unreacheable node !\n");
            return -1;
        }
        
        if ((unsigned int)compute_cnt++ > cur_table_size )
            return -1; /* this is an assersion to avoid infinit loop */
    }    
    return 1;
}
 
int single_source_shortest_path_compute ( int src_index )
{
     ulong current_node , smallest_distance , new_distance , current_distance , preferred_next_node ;

     /* clear the old data */
     if ( perm ) {
         delete perm;
	 perm = 0;
     }
     if ( distance ) {
         delete distance;
         distance = 0;
     }
     if ( precede ) {
         delete precede;
         precede = 0;
     }
     
     perm     = new ulong[cur_table_size];
     distance = new ushort[cur_table_size];
     precede  = new ulong[cur_table_size];
     
     if ( (!perm) || (!distance) || (!precede) )
     {
     	fprintf(logfile,"no available buffer\n");
     	return -1;
     }

     /* initialize data structures */
     bzero( perm    , sizeof(ulong)*cur_table_size );     

     for (unsigned int i=0 ; i<cur_table_size ; ++i )
     {
     	  precede[i]  = INFINITY ;
          distance[i] = INFINITY ;
     }
     
     perm[src_index] = INCLUDED;
     distance[src_index] = 0 ;
     current_node = src_index ;
     preferred_next_node = src_index ;
     
     while ( finish_computing( perm , cur_table_size ) == false )
     {     	
     	smallest_distance = INFINITY ;
     	current_distance = distance[ current_node ];
     
     	for (unsigned int i=0 ; i<cur_table_size ; ++i )
     	{
     	    /* if node i is not included into the shortest path , compute its cost
     	     * via current_node and compare with the value computed via the old one.
     	     */
     	    if ( perm[i] == NOT_INCLUDED )
     	    {
     	     	new_distance = current_distance + weight[current_node][i];
     	    	if ( new_distance < distance[i] )
     	    	{
     	    	    distance[i] = new_distance;
     	    	    precede[i] = current_node;     
     	    	}
     	    	if ( distance[i] < smallest_distance )
     	    	{  
     	    	    /* The node which has the smallest distance from src to node i in this depth ,
     	    	     * will be elected as preferred next node in the shortest path 
     	    	     */
     	    	    smallest_distance = distance[i] ;
     	    	    preferred_next_node = i ;
     	    	}
     	    }
     	}/* end of for loop */     	
     	
     	/* if (current_node == preferred_next_node) but not all members have been in perm[] , then
     	 * perform dfs search. This is caused by that the traverse of a path has reached its end, 
     	 * now search any other node which has the smallest distance,that is, has preceder as one of 
     	 * the nodes in the previous traversed path.
     	 */
     	if ( current_node == preferred_next_node )
     	{
     	     int flag = false;
     	     for (unsigned int i=0 ; i<cur_table_size ; ++i )
     	     {
     	     	 if ( (perm[i] == NOT_INCLUDED) && (precede[i]!= INFINITY) )
     	     	 {
     	     	     preferred_next_node = i;
     	     	     flag = true;
     	     	 }
     	     }
     	     if ( !flag )
     	        break; /* if there are no nodes matching this requirement , we traverse the whole tree.
     	                * since any other nodes we cannot traverse will be unreachable.
     	                */
     	}
     	
     	
     	current_node = preferred_next_node;     	
     	perm[ current_node ] = INCLUDED;
     }/* end of while */          
     
#ifdef MY_DEBUG
     dump_info();
#endif

     for (unsigned int i=1 ; i<cur_table_size ; ++i )
         get_nexthop_to_a_specified_node( 0 , i );


     return 1;
}

/* prepare the required table for shortest path computing */
int node_list_expansion()
{    
    int num_of_nodes = node_list.get_num_of_nodes();    

    //node_list.dump(0);

    if ( weight )
    {
    	for (unsigned int i=0 ; i< cur_table_size ; ++i ) {
             delete weight[i] ; 
             weight[i] = NULL;
	}
        
	delete weight;
        weight = NULL;
    }   

    if ( node_id_mapping ) {
        delete node_id_mapping;
    	node_id_mapping = NULL;
    }
        
    weight = new ulong*[num_of_nodes];

    for ( int i=0 ; i<num_of_nodes ; ++i ) {
        weight[i] = new ulong[num_of_nodes];
    }    
    
    cur_table_size = num_of_nodes;
    
    /* initialization */
    for (unsigned int i=0 ; i<cur_table_size ; ++i)
    {
        for (unsigned int j=0 ; j<cur_table_size ; ++j ) 
        {
            if ( i == j )  
                weight[i][j] = 0 ;
            else
                weight[i][j] = INFINITY ;        	
        }
    }    
    
    /* node id mapping */
    node_id_mapping = new ulong[cur_table_size];

    if ( !node_id_mapping )
        printf("Warning: node_id_mapping array failed to generate!\n");

    node_t* ptr = node_list.get_head();

    for (unsigned int i=0 ; i<cur_table_size ; ++i )
    {    	
    	node_id_mapping[i] = ptr->ospf_id;
    	ptr = ptr->next;    
    }    
    
    ptr = node_list.get_head();    

    /* this loop set up neighboring adjacency */   
    for ( unsigned int i=0 ; i<cur_table_size ; ++i , ptr=ptr->next )
    {    	
	int num_of_ifs 		= ptr->num_of_interfaces;
	int num_of_neighbors 	= ptr->neighbors->get_num_of_neighbors();
	long netmask 		= htonl(0xffffff00);
	
	for (int j=0; j<num_of_ifs; j++) {
		//done = false;
		if( ptr->interface_list[j].state == DOWN ) {
			continue;
		}

		neighbor_t* n_ptr = ptr->neighbors->get_head();

		for(int k=0; k<num_of_neighbors; k++, n_ptr=n_ptr->next) {
			if( n_ptr->state == DOWN ) {
				fprintf(debugfile, "(1)\n");
				continue;
			}
		
			for(unsigned int l=0; l<n_ptr->neighbor_num_of_interfaces; l++) {
				if( n_ptr->neighbor_interface_list[l].state == DOWN ) {
					fprintf(debugfile, "(2)\n");
					continue;
				}
				
				struct in_addr tmp;
				tmp.s_addr = ptr->interface_list[j].ipv4_addr;
				fprintf(debugfile, "%s ", inet_ntoa(tmp));
				tmp.s_addr = n_ptr->neighbor_interface_list[l].ipv4_addr;
				fprintf(debugfile, " %s", inet_ntoa(tmp));

				if( (ptr->interface_list[j].ipv4_addr&netmask) == (n_ptr->neighbor_interface_list[l].ipv4_addr&netmask) ) {
					int n_index = node_id_mapping_search(n_ptr->neighbor_ospf_id);

					weight[i][n_index] = 1;
					//done = true;
					fprintf(debugfile, " matched\n");
					break;
				}
				fprintf(debugfile, "\n");
			}

/*
			if( done ) {
				break;
			}
*/
		}
	}
    }

    fprintf(debugfile, "my god la\n");
    dump_node_list();
    dump_weight();

#ifdef MY_DEBUG
    dump_weight();
#endif

    return 1;
}

int node_id_mapping_search(ulong ospf_id )
{
    //int num_of_nodes = node_list.get_num_of_nodes();
    for (unsigned int i=0 ; i< cur_table_size ; ++i )
    {
       if ( node_id_mapping[i] == ospf_id )
           return i;
    }
    return -1;
}

int display_precede()
{
    //int num_of_nodes = node_list.get_num_of_nodes();
    
    if ( !perm )
    	fprintf(logfile,"perm was not allocated\n");            
    if ( !distance )
        fprintf(logfile,"distance was not allocated\n");    
    if ( !precede )
    {
        fprintf(logfile,"precede was not allocated\n");
        return -1;
    }    
    
    fprintf( logfile , "\n**** dump preceding array ****\n ");
    for (unsigned int i=0 ; i<cur_table_size ; ++i )
    {
       fprintf(logfile,"precede[%d]'s ospfid = ", i );show_ip( node_id_mapping[i] ,logfile);       
       fprintf(logfile,"its preceding node ip = "); 
       if ( (precede[i] == 10000) && (i == 0) )
          fprintf(logfile,"myself\n" );
       else if ( precede[i] == 10000 )
          fprintf(logfile,"unreachable\n");
       else 
          show_ip( node_id_mapping[ precede[i] ] , logfile);fprintf(logfile,"\n");
    }
    return 1;
}

/* my hello packet is composed of some fields of OSPF required header and 
 * the IP addresses of interfaces which are attached to this router.
 */
int broadcast_hello_msg(int sockfd)
{
	/* replace the original global varibles */
	int		num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr		*attached_interfaces = myself->interface_list;
	OspfPktHeader	pkt_header;

	char		sendbuf[512]; /* The size of a OSPF following UDP msg */
	int		msgsize; 
	my_hello_pkt_t	payload;
	const char	*class_c_netmask = "255.255.255.0";

	pkt_header.set_type(MY_HELLO);        

	memset(sendbuf, 0, sizeof(sendbuf));
	msgsize = 44 + 5*num_of_attached_interfaces;
	pkt_header.set_packet_length(msgsize);

	payload.netmask = htonl(0xffffff00);
	//payload.hello_interval = 2*DEFAULT_HELLO_INTERVAL;
	payload.hello_interval = 12;
	payload.priority = 1;
	payload.options  = 0;
	payload.ospf_id = my_ospf_id;
	payload.num_of_interfaces = num_of_attached_interfaces;

	/* Broadcast on all my active interfaces (UP) */
	for(int i=0; i<num_of_attached_interfaces; i++) {
		char	*header_p; 
		int 	res;

		if( attached_interfaces[i].state == DOWN ) {
			continue;
		}
	
		pkt_header.set_router_id(attached_interfaces[i].ipv4_addr);  
		pkt_header.set_area_id(attached_interfaces[i].ipv4_addr);  

		/* hello pkt payload */
		memcpy(sendbuf+24, &payload, sizeof(my_hello_pkt_t));

		/*  _________________________________ 
		 * |        |           |            |
		 * | header | if_ipv4[] | if_state[] |
		 * |________|___________|____________|
		 */

		for(int j=0; j<num_of_attached_interfaces; j++) {
			memcpy(sendbuf+44+j*4, &(attached_interfaces[j].ipv4_addr), 4); 
			memcpy(sendbuf+44+num_of_attached_interfaces*4+j, 
				&(attached_interfaces[j].state), 1);
    	   	}

		/* ospf header */
		header_p = pkt_header.header_serialize();
		memcpy(sendbuf, header_p, 24);

		struct in_addr t;
		t.s_addr = attached_interfaces[i].ipv4_addr;
		fprintf(debugfile, "broadcast hello msg on %s\n", inet_ntoa(t));

		res = broadcast(sockfd, sendbuf, msgsize, attached_interfaces[i].ipv4_addr, class_c_netmask); 

		if( res < 0 ) {
			fprintf(debugfile, "broadcasting hello message failed\n");
		}
	}

	return 0;
}

int flood_LSA(int sockfd)
{    
	/* replace the original global varibles */
	int			num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr			*attached_interfaces = myself->interface_list;

	OspfPktHeader		pkt_header;
	char			sendbuf[512]; /* The size of a OSPF following UDP msg. */    	
	LSA_header_t		header;
	int			msgsize;
	const char		*class_c_netmask = "255.255.255.0";
	ulong			num_of_nbs = myself->neighbors->get_num_of_neighbors();
	int			num_of_ipv4_addrs = num_of_attached_interfaces + num_of_nbs;
	    
	pkt_header.set_type(MY_LSA);

	if( ++seq_no >= 0xffffffff ) {
		 seq_no = 1; 
	}
    
	memset(sendbuf, 0, sizeof(sendbuf));  	

	msgsize = 44 + 5*num_of_ipv4_addrs; /* ospf pkt header + LSA header size */
	pkt_header.set_packet_length(msgsize);
	    
	header.type = 0; /* router's LSA */
	header.cost = 1;
	header.ipv4_src_ospf_id = my_ospf_id;
	header.seq_no = seq_no; 
	header.num_of_neighbors  = num_of_nbs;
	header.num_of_interfaces = num_of_attached_interfaces;

	ulong	*ipv4_ptr	= (ulong *) (sendbuf + 44) ;
	byte	*state_ptr	= (byte *) (((byte *) ipv4_ptr) + 4*num_of_ipv4_addrs);

	/*  	ipv4_ptr		state_ptr
	 *  __________________________________________________________
	 * |        |           |           |		 |	      |
	 * | header | if_ipv4[] | nb_ipv4[] | if_state[] | nb_state[] |
	 * |________|___________|___________|____________|____________|
	 */

	/* add my interfaces */
	for(int i=0; i<num_of_attached_interfaces; i++, ipv4_ptr++, state_ptr++) {
        	memcpy(ipv4_ptr, &(attached_interfaces[i].ipv4_addr), 4);
		memcpy(state_ptr, &(attached_interfaces[i].state), 1);
	}

	neighbor_t *ptr	= myself->neighbors->get_head();    

	/* add my neighbors */
	for( ; ptr; ptr=ptr->next, ipv4_ptr++, state_ptr++) {
		memcpy(ipv4_ptr, &(ptr->neighbor_ospf_id), 4);
		memcpy(state_ptr, &(ptr->state), 1);
	}

	/* Broadcast on all my active interfaces (UP) */
	for(int i=0; i<num_of_attached_interfaces; i++) {
		char	*header_p;
		int	res;
	
		if( attached_interfaces[i].state == DOWN ) {
			continue;
		}

		pkt_header.set_router_id(attached_interfaces[i].ipv4_addr);  
		pkt_header.set_area_id(attached_interfaces[i].ipv4_addr);  
		   
		header_p = pkt_header.header_serialize();
		memcpy(sendbuf, header_p, 24);   /* ospf header */    	   
		memcpy(sendbuf+24, &header, 20); /* LSA  header */       

		res = broadcast(sockfd, sendbuf, msgsize, attached_interfaces[i].ipv4_addr, class_c_netmask);

		if( res < 0 ) {
				printf("Flooding LSA failed\n");
		}
	}

	return 0;
}

int forward_LSA( int sockfd , char* msg_clone , ulong msglen , ulong receiving_if )
{
    /* replace the original global varibles */
    int		num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr	*attached_interfaces = myself->interface_list;

    /* examine the LSA seq_no to determine whether it should be forwarded */
    LSA_header_t *header;
    header = (LSA_header_t*)( msg_clone + 24 );   
    ++header->cost ;

    node_t* node_p = NULL ;    

    if ( !(node_p = node_list.search(header->ipv4_src_ospf_id) ) )
    {
    	printf("Waring: Forward a LSA with unknown ospf id\n");

    	if ( msg_clone ) {
    	   delete msg_clone;
           msg_clone = NULL;
        }

    	return -1;
    }
    
    /* this is a rough checking for seqence number problem */
    
    struct in_addr t;
    t.s_addr = header->ipv4_src_ospf_id ;
    int n_index = get_connected_my_interfaces( t , htonl(0xffffff00) );
    	 
    const char* class_c_netmask = "255.255.255.0";

    for ( int i=0 ; i<num_of_attached_interfaces ; ++i )
    {
    	if ( (i != n_index) && (attached_interfaces[i].ipv4_addr != receiving_if ))
    	{
       		int res;

       	        res=broadcast(sockfd, msg_clone , msglen , attached_interfaces[i].ipv4_addr , class_c_netmask ); 

                if ( res < 0 )
                {
         		fprintf(logfile,"OSPFD of ");show_ip(my_ospf_id,logfile);fprintf(logfile,":Forwarding LSA failed\n");
        	}
        }
    }    
    
 
    if ( msg_clone ) {
    	delete msg_clone;
    	msg_clone = NULL;
    }          

    return 1;
}

/*
int dump_LSA( LSA_header_t *t )
{
    char buf[16];
    struct in_addr addr;
    LSA_header_t *p = t;
    fprintf( logfile , "\n****** dump LSA content *****\n");
    fprintf ( logfile , "type =%d cost =%d " , t->type , t->cost );
    addr.s_addr = t-> ipv4_src_ospf_id ;
    fprintf ( logfile , "ospf id = %s \n" , inet_ntop(AF_INET , &addr , buf , 16 ) );
    fprintf ( logfile , " num_of_if = %d , num_of_neighbors = %d \n" , t->num_of_interfaces , t->num_of_neighbors );
    
    fprintf ( logfile , "interfaces addr: \n");
    ulong* ptr = (ulong*)( ++p );
    for ( int i=0 ; i<t->num_of_interfaces ; ++i )
    {
    	memcpy ( &addr.s_addr ,  ptr , 4 );
    	fprintf ( logfile , " %s \n" , inet_ntop(AF_INET , &addr , buf , 16 ) );
    	ptr++;
    }
    
    fprintf ( logfile, "neighbor ospf id : \n");
    for ( int i=0 ; i<t->num_of_neighbors ; ++i )
    {
    	memcpy ( &addr.s_addr , ptr  , 4 );
    	fprintf ( logfile , " %s \n" , inet_ntop(AF_INET , &addr , buf , 16 ) );
    	ptr ++ ;
    }
    fprintf( logfile , "\n****** End of dumping LSA content *****\n");
}
*/

int show_ip(ulong num)
{
    char buf[16];
    struct in_addr addr;
    addr.s_addr = num;
    printf(" %s " , inet_ntop(AF_INET , &addr , buf , 16 ) );
    return 1;
}

int show_ip(ulong num , FILE* file_handle )
{
    char buf[16];
    struct in_addr addr;
    addr.s_addr = num;
    fprintf( file_handle , " %s " , inet_ntop(AF_INET , &addr , buf , 16 ) );
    return 1;
}

void make_ssdd(char *ssdd, const char *ip1, const char *ip2)
{
	unsigned int i, j;
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

bool is_tun_up(unsigned long int s_addr)
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
		printf("%d\n", ret);
		return false;
	}
}

void get_attached_if_states()
{
	/* replace the original global varibles */
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;

	for(int i=0 ; i<num_of_attached_interfaces; i++) {
		attached_interfaces[i].state = is_tun_up(attached_interfaces[i].ipv4_addr) ? UP : DOWN;
	}
}

void dump_attached_if_states()
{
	/* replace the original global varibles */
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;

	struct in_addr tmp_addr;

	for(int i=0 ; i<num_of_attached_interfaces; i++) {
		tmp_addr.s_addr = attached_interfaces[i].ipv4_addr;
		fprintf(debugfile, "%s: ", inet_ntoa(tmp_addr));
		fprintf(debugfile, attached_interfaces[i].state == UP ? "UP\n" : "DOWN\n");
	}
}

bool is_attached_if_states_changed()
{
	/* replace the original global varibles */
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;

	int	state;
	bool	changed = false;

    	for(int i=0 ; i<num_of_attached_interfaces; i++) {

		state = is_tun_up(attached_interfaces[i].ipv4_addr) ? UP : DOWN;

		if( attached_interfaces[i].state != state ) {

			attached_interfaces[i].state = state;
			changed = true;

			neighbor_t *nb = myself->neighbors->get_head();
			int num_of_nbs = myself->neighbors->get_num_of_neighbors();
			ulong	netmask = htonl(0xffffff00);

			for(int j=0; j<num_of_nbs; j++, nb=nb->next) {

				for(unsigned int k=0; k<nb->neighbor_num_of_interfaces; k++) {

					if( (attached_interfaces[i].ipv4_addr&netmask) ==
					    (nb->neighbor_interface_list[k].ipv4_addr&netmask) ) {

						nb->state = state;

						if( nb->state == DOWN ) {
							nb->inactive_timer = 0;
						}

						break;
					}
				}
			}

			/* DOWN => UP */
			if( state == UP ) {
				restore_local_if_rt_entry(attached_interfaces[i].ipv4_addr);
			}
		}
	}

	return changed;
}

int num_of_active_attached_if()
{
	/* replace the original global varibles */
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;
	int	num = 0;

	for(int i=0; i<num_of_attached_interfaces; i++) {
		if( attached_interfaces[i].state == UP ) {
			num++;
		}
	}

	return num;
}

void dump_hello_pkt(my_hello_pkt_t *pkt)
{
	struct in_addr	tmp_addr;
	ulong		*nb_ipv4_p = (ulong *) (((char *) pkt) + 20);
	byte		*nb_state_p = ((byte *) pkt) + 20 + sizeof(ulong)*pkt->num_of_interfaces;

	tmp_addr.s_addr = my_ospf_id;
	fprintf(debugfile, "===================================\n");
	fprintf(debugfile, "== Hello Pkt received by %s ==\n\n", inet_ntoa(tmp_addr));

	tmp_addr.s_addr = pkt->ospf_id;
	fprintf(debugfile, "\tfrom %s\n", inet_ntoa(tmp_addr));

	fprintf(debugfile, "\t# of ifs = %lu", pkt->num_of_interfaces);

	for(unsigned int i=0; i<pkt->num_of_interfaces; i++, nb_ipv4_p++, nb_state_p++) {
		tmp_addr.s_addr = *nb_ipv4_p;
		fprintf(debugfile, " (%s %s)", inet_ntoa(tmp_addr), *nb_state_p==UP ? "UP" : "DOWN");
	}

	fprintf(debugfile, "\n");
}

void dump_LSA(LSA_header_t *header, ulong from)
{
	/*  	 ipv4_p			 state_p
	 *  __________________________________________________________
	 * |        |           |           |		 |	      |
	 * | header | if_ipv4[] | nb_ipv4[] | if_state[] | nb_state[] |
	 * |________|___________|___________|____________|____________|
	 */

	struct in_addr	tmp_addr;
	int 		num_of_ifs = header->num_of_interfaces;
	int		num_of_nbs = header->num_of_neighbors;
	ulong		*ipv4_p = (ulong *) (((char *) header) + 20);
	byte		*state_p = ((byte *) ipv4_p) + 4*(num_of_ifs+num_of_nbs);

	tmp_addr.s_addr = my_ospf_id;
	fprintf(debugfile, "===================================\n");
	fprintf(debugfile, "===== LSA received by %s =====\n\n", inet_ntoa(tmp_addr));

	tmp_addr.s_addr = from;
	fprintf(debugfile, "\tfrom %s\n", inet_ntoa(tmp_addr));

	tmp_addr.s_addr = header->ipv4_src_ospf_id;
	fprintf(debugfile, "\tsrc %s\n", inet_ntoa(tmp_addr));

	fprintf(debugfile, "\tcost = %d\n", header->cost);

	fprintf(debugfile, "\t# of ifs = %d", num_of_ifs);

	for(int i=0; i<num_of_ifs; i++, ipv4_p++, state_p++) {
		tmp_addr.s_addr = *ipv4_p;
		fprintf(debugfile, " (%s %s)", inet_ntoa(tmp_addr), *state_p==UP ? "UP" : "DOWN");
	}

	fprintf(debugfile, "\n");

	fprintf(debugfile, "\t# of nbs = %d", num_of_nbs);

	for(int i=0; i<num_of_nbs; i++, ipv4_p++, state_p++) {
		tmp_addr.s_addr = *ipv4_p;
		fprintf(debugfile, " (%s %s)", inet_ntoa(tmp_addr), *state_p==UP ? "UP" : "DOWN");
	}

	fprintf(debugfile, "\n\n");
}

void dump_node_list()
{
	node_t *ptr = node_list.get_head();
	struct in_addr tmp_addr;
	int num = 0;

	if( !ptr ) {
		fprintf(debugfile, "Error: ptr == NULL\n");
		return;
	} else if( ptr != myself ) {
		fprintf(debugfile, "Error: head != myself\n");
		return;
	}

	tmp_addr.s_addr = ptr->ospf_id;
	fprintf(debugfile, "===================================\n");
	fprintf(debugfile, "====== node_list of %s =======\n\n", inet_ntoa(tmp_addr));

	for( ; ptr!=NULL; ptr=ptr->next, num++) {
		tmp_addr.s_addr = ptr->ospf_id;
		fprintf(debugfile, "\tnode #%s: %s\n", inet_ntoa(tmp_addr), ptr->state==UP ? "UP" : "DOWN");
		fprintf(debugfile, "\tcost: %lu\n", ptr->cost);
		fprintf(debugfile, "\tinactive timer: %lu\n", ptr->inactive_timer);
		tmp_addr.s_addr = ptr->nexthop;
		fprintf(debugfile, "\tnexthop: %s\n", inet_ntoa(tmp_addr));
		fprintf(debugfile, "\t\t# of ifs = %lu", ptr->num_of_interfaces);

		for(unsigned int i=0; i<ptr->num_of_interfaces; i++) {
			tmp_addr.s_addr = ptr->interface_list[i].ipv4_addr;
			fprintf(debugfile, " (%s %s)", inet_ntoa(tmp_addr), ptr->interface_list[i].state==UP ? "UP" : "DOWN");
		}

		fprintf(debugfile, "\n");
		fprintf(debugfile, "\t\t# of nbs = %lu\n", ptr->neighbors->get_num_of_neighbors());

		neighbor_t *nb = ptr->neighbors->get_head();

		for( ; nb; nb=nb->next) {
			tmp_addr.s_addr = nb->neighbor_ospf_id;
			fprintf(debugfile, "\t\t\t%s %s %lu:", 
				inet_ntoa(tmp_addr), nb->state==UP ? "UP" : "DOWN", nb->inactive_timer);

			for(unsigned int i=0; i<nb->neighbor_num_of_interfaces; i++) {
				tmp_addr.s_addr = nb->neighbor_interface_list[i].ipv4_addr;
				fprintf(debugfile, " (%s %s)", inet_ntoa(tmp_addr), nb->neighbor_interface_list[i].state==UP ? "UP" : "DOWN");
			}
			
			fprintf(debugfile, "\n");
		}

		fprintf(debugfile, "\n");
	}

	if( node_list.get_num_of_nodes() != (u_int32_t)num ) {
		fprintf(debugfile, "Error: # of nodes mismatch\n");
	}

}

void dump_weight()
{
	node_t *ptr = node_list.get_head();
	struct in_addr tmp_addr;

	fprintf(debugfile, "          ");

	for(; ptr; ptr=ptr->next) {
		tmp_addr.s_addr = ptr->ospf_id;
		fprintf(debugfile, "[%8s] ", inet_ntoa(tmp_addr));
	}

	fprintf(debugfile, "\n");

	ptr = node_list.get_head();

	for(unsigned int i=0; i<cur_table_size; i++, ptr=ptr->next) {
		tmp_addr.s_addr = ptr->ospf_id;
		
		fprintf(debugfile, "[%8s] ", inet_ntoa(tmp_addr));

		for(unsigned int j=0; j<cur_table_size; j++) {
			fprintf(debugfile, "%10lu ", weight[i][j]);
		}

		fprintf(debugfile, "\n");
	}

}

void dump_info()
{
	fprintf(debugfile, "perm:     ");

	for(unsigned int i=0; i<cur_table_size; i++) {
		fprintf(debugfile, "%8lu ", perm[i]);
	}
	
	fprintf(debugfile, "\n");

	fprintf(debugfile, "distance: ");

	for(unsigned int i=0; i<cur_table_size; i++) {
		fprintf(debugfile, "%8d ", distance[i]);
	}
	
	fprintf(debugfile, "\n");
	
	fprintf(debugfile, "precede:  ");

	for(unsigned int i=0; i<cur_table_size; i++) {
		fprintf(debugfile, "%8lu ", precede[i]);
	}
	
	fprintf(debugfile, "\n");
}

bool set_interface_state_in_node_list(ulong except_ipv4, ulong ipv4, byte state)
{
	node_t *ptr = node_list.get_head();
	int changed = false;

	/* node */
	for( ; ptr!=NULL; ptr=ptr->next) {
		if( ptr->ospf_id == except_ipv4 ) {
			continue;
		}
	
		/* interface_list */
		for(unsigned int i=0; i<ptr->num_of_interfaces; i++) {
			if( ptr->interface_list[i].ipv4_addr == ipv4 && 
			    ptr->interface_list[i].state != state ) {
				ptr->interface_list[i].state = state;
				changed = true;
			}
		}

		neighbor_t *nb = ptr->neighbors->get_head();

		/* neighbors */
		for( ; nb; nb=nb->next) {
			for(unsigned int i=0; i<nb->neighbor_num_of_interfaces; i++) {
				if( nb->neighbor_interface_list[i].ipv4_addr == ipv4 &&
				    nb->neighbor_interface_list[i].state != state ) {
					nb->neighbor_interface_list[i].state = state;
					changed = true;
				}
			}
		}
	}

	return changed;
}

void restore_local_if_rt_entry(ulong ipv4)
{
    /* replace the original global varibles */
    int			num_of_attached_interfaces = myself->num_of_interfaces;
    If_Addr		*attached_interfaces = myself->interface_list;

    struct in_addr	src_ipv4, dest_ipv4, nexthop_ipv4;
    neighbor_t		*nb;
    ulong		netmask = htonl(0xffffff00);

    /* find local rt entry */
    nb = myself->neighbors->get_head();

    for( ; nb; nb=nb->next) {
		if( (ipv4&netmask) == (nb->neighbor_attached_interface_ipv4&netmask) ) {
			break;
		}
    }

/* Bug Fixed:
 *   If the subnet of the IP address cannot be found from the neighbor list,
 *   nb will be a null pointer.
 *   In this situation, we can ignore this IP address for re-building the
 *   routing entry for this subnet.
 * By YM Huang
 */
    if(!nb)
	return;

    for(int i=0; i<num_of_attached_interfaces; i++) {
		if( attached_interfaces[i].state == DOWN ) {
			continue;
		}
    
		src_ipv4.s_addr		= attached_interfaces[i].ipv4_addr;
		dest_ipv4.s_addr	= ipv4;
		nexthop_ipv4.s_addr	= ipv4;
    
		struct in_addr tmp_addr;
		fprintf(debugfile, "delete entry: %s", inet_ntoa(src_ipv4));
		tmp_addr.s_addr = nb->neighbor_attached_interface_ipv4;
		fprintf(debugfile, " to %s\n", inet_ntoa(tmp_addr));

		routingtable.del_entry(src_ipv4.s_addr, nb->neighbor_attached_interface_ipv4);
       	        system_rt_entry_manipulate_for_local_if_initialize(
			CHANGE, src_ipv4, dest_ipv4, nexthop_ipv4, NCTUNS);
    }

}

void set_node_interface_state_with_netmask(node_t *n, ulong ipv4, byte state)
{
	/* replace the original global varibles */
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;

	ulong	netmask = htonl(0xffffff00);

	for(int i=0; i<num_of_attached_interfaces; i++) {
		if( (attached_interfaces[i].ipv4_addr&netmask) == (ipv4&netmask) ) {
			attached_interfaces[i].state = state;
			break;
		}
	}
}

void set_neighbor_interface_state(neighbor_t *nb, ulong ipv4, byte state)
{
	/* replace the original global varibles */
	int	num_of_attached_interfaces = myself->num_of_interfaces;
	If_Addr	*attached_interfaces = myself->interface_list;

	int	num_of_nb_ifs = nb->neighbor_num_of_interfaces;
	ulong	netmask = htonl(0xffffff00);

	for(int i=0; i<num_of_nb_ifs; i++) {

		if( nb->neighbor_interface_list[i].ipv4_addr == ipv4 ) {

			for(int j=0; j<num_of_attached_interfaces; j++) {

				if( (attached_interfaces[j].ipv4_addr&netmask) == (ipv4&netmask) &&
				    // attached_interfaces[j].state == UP &&
				    nb->neighbor_interface_list[i].state == DOWN &&
				    state == UP ) {
					restore_local_if_rt_entry(attached_interfaces[j].ipv4_addr);
					break;
				}
			}

			nb->neighbor_interface_list[i].state = state;
			break;
		}
	}
}
