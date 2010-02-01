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
 * The file contains network api for librtp.
 */

/********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>   	/* timeval */
#include <sys/types.h>
#include <unistd.h>     	/* get..() */
#include <rpcsvc/ypclnt.h>   	/* YP */
#include <netdb.h>      	/* gethostname() */
#include <sys/socket.h> 	/* AF_UNSPEC */

#include <string.h>		/* strcpy */
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <errno.h>

#include "rtp_network_api.h"
#include "rtp_member_api.h"	// for ssrc_hash;
#include "rtp_session_api.h"


/********************************************************************/


/* Illegal address */
static const u_int32 ILLEGAL_ADDR = -1;

/* If there is a new address in the sendlist, new_send_addr will be set 
 * to TRUE, then we can initialize the connection when RTPSend() or 
 * RTPOpenConnect() is called during the session. */
static int new_send_addr = FALSE;

/* Flag is true if receive address is new when RTPSetRecvAddr() is called. */
static int new_recv_addr = FALSE;

/* Assign free rtp port linearlly, starting from PORT_LOWER which 
 * specified in RTPConfig.h */
static u_int16 getport = PORT_LOWER;

/* if recvlist's port = 0, we choose a port to bind to the host */
static int dynamic_port = FALSE;


/********************************************************************/


/*
 * rtp_add_dest_list - Create sockets for RTP and RTCP.
 *
 * Adds a destination for sending data either unicast or multicast.
 * TTL for unicast is 0 (no meaning).
 * Port is in host byte order, never with zero port number.
 * We won't add the address into sendlist if it already exists in list.
 */
rtperror
rtp_add_dest_list(session_id sid, char *addr, u_int16 port, u_int8 ttl, protocol_t rtp_p, protocol_t rtcp_p){
	
	session		*rs;
	rtperror	err = 0;
	char		*DST_IP;	// DEBUG only
	struct in_addr	sendaddr;
	address_info_t	*ai, *check;	/* address information */
	
	/* port number zero is not allowed */
	if (!port) { // 0
		return RTP_BAD_PORT;
	}
	
	/* get the RTP session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	/* malloc an address_holder to insert into the send_addr_list */
	ai = (address_info_t *) malloc (sizeof(address_info_t));
	if (!ai) { //NULL
		return	RTP_CANT_ALLOC_MEM;
	}
	
	/* translate address, return in network byte order */
	sendaddr = host2ip(addr);
	if (sendaddr.s_addr == ILLEGAL_ADDR) { // illegal address
		free(ai);
		return RTP_BAD_ADDR;
	}
	
	/* we assume it's a RTCP port if port is odd */
	if (port % 2) {
		port--;
	}
	
	/* set values of address, port to RTPsession */
	ai->living = TRUE;
	ai->addr = sendaddr;
	ai->port = htons(port);
	
	/* if address is multicast, setup the ttl 
	if (IsMulticast(sendaddr)) {
					
		DST_IP = strdup(inet_ntoa(sendaddr)); //DEBUG_ONLY
		printf("MSG	: rtp_add_dest_sendlist - %s is multicast\n", DST_IP); //DEGBUG_ONLY
		ai->ttl = ttl;
	}	
	else {*/
		/* set ttl = 0 if the "sendaddr" is unicast */
		//ttl = 0;
		//ai->ttl = ttl;
	//}
	
	/* set underlying protocol is tcp or udp */
	ai->rtp_protocol = rtp_p;
	ai->rtcp_protocol = rtcp_p;
	
	/* now we just set rtpskt && rtcpskt to zero, we setup socket description to them */
	ai->rtpskt = FALSE;
	ai->rtcpskt = FALSE;
	
	/* check if the address is alreay allocated in send_addr_list */
	check = rs->sendlist;
	
	while (check) {
		/* the address is repeat */
		if (  	(check->addr.s_addr == sendaddr.s_addr) && 
			(check->port == htons(port)) && 
			(check->ttl == ttl) ) {
			
			/* address is repeated, so we won't add it into the sendlist */			
			free(ai);						
			return REPEAT_SEND_ADDR_LIST;
		}	
		check = check->next;
	}		
	
	//DEBUG_ONLY
	DST_IP = strdup(inet_ntoa(sendaddr)); 
	if (ai->rtp_protocol == udp) {
#if RTP_DEBUG	
		printf("MSG	: rtp_add_dest_sendlist - we set address:%s , port:%d, ttl:%d, rtp_protocol:udp\n", DST_IP, port, ttl);
#endif
	}
	else {
#if RTP_DEBUG	
		printf("MSG	: rtp_add_dest_sendlist - we set address:%s , port:%d, ttl:%d, rtp_protocol:tcp\n", DST_IP, port, ttl);
#endif
	}
	free(DST_IP);
	//DEBUG_ONLY
	
	/* Add the address to the first of SendAddr list */
	ai->next = rs->sendlist;
	rs->sendlist = ai;
	if (!rs->sendlist) { //NULL
		return ADD_SENDLIST_FAIL;
	}
	
	/* there is a new address in sendlist */
	new_send_addr = TRUE;
	
	return err;
	
} /* rtp_add_dest_list */	


/* 
 * rtp_del_dest_list - Removes the item holded by address_info_t and close the socket.
 *
 * Note that if it is unicast, we delete the address without condidering ttl,
 * the function will also close the associated sockets.
 */
rtperror 
rtp_del_dest_list(session_id sid, char *addr, u_int16 port, u_int8 ttl){
	
	address_info_t  *ai, *previous;
	struct	in_addr	sendaddr;
	rtperror 	err = 0;
	session 	*rs;
	
	/* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}
	
	if (!rs->sendlist) { // NULL
		return NO_RTP_SEND_ADDR_LIST;
	}
	
	if (port % 2) {
		port--;
	}
	
	/* host to network */
	sendaddr = host2ip(addr);	
	if (sendaddr.s_addr == ILLEGAL_ADDR) {
		/* sendaddr is a invalid address */
		return RTP_BAD_ADDR;
	}
	
	/* if sendaddr is unicast, set ttl = 0 */
	//if (!is_multicast(sendaddr)) {
		//ttl = 0;
	//}
	
	previous = NULL;
	ai = rs->sendlist;	
	
	while (ai) {
		if (	(ai->addr.s_addr == sendaddr.s_addr) && 
		     	(ai->port == htons(port)) && 
			(ai->ttl == ttl) ) {
			
			/* match up */
			
			if (previous) {
				previous->next = ai->next;
			}
			else {
				rs->sendlist = ai->next;
			}
			
			err = socket_close(ai->rtpskt);
			if (err > 0) {
				return err;
			}

			err = socket_close(ai->rtcpskt);
			if (err > 0) {
				return err;
			}
			
#if RTP_DEBUG	
			printf("MSG	: rtp_del_dest_sendlist - removing the address : %s\n", addr); // DEBUG_ONLY
#endif
			
			free(ai);			

			return err;
		}
		previous = ai;
		ai = ai->next;
	}
	
	return NO_MATCH_ADDR;
	
} /* rtp_del_dest_list */


/* 
 * rtp_get_dest_list - Get the address and port in host order.
 */
rtperror 
rtp_get_dest_list(session_id sid, address_info_t **sendaddr){
	
	rtperror	err = 0;
	session		*rs; 
	
	/* get the RTPsession of RTPsessionlist[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}
	
	if (!rs->sendlist) { // NULL
		return NO_RTP_SEND_ADDR_LIST;
	}
	else {
		*sendaddr = rs->sendlist;
	}
	
	return err;	
						
} /* rtp_get_dest_list */  



/* 
 * rtp_add_sour_list - Setup receving address and port with socket that 
 *                     the library listens for incoming packets.
 *
 * it is called only when session is not connecting.
 * each session only has one recvice address.
 *
 * unicast :
 *           if address is NULL, we bind it with INADDR_ANY.
 *           if port number is zero, we random allocate a port number in host byte order.
 * multicast:
 *           the address is a address of the multicast group to listen to.
 *
 * If session change it's source transport address, SSRC MUST be chosen again.
 * This is why we can't change source transport address during the session.
 * New SSRC will be chosen when we recall the RTPOpenConnect().
 */
rtperror 
rtp_add_sour_list(session_id sid, char *addr, u_int16 port, protocol_t rtp_p, protocol_t rtcp_p){
	
	address_info_t	*ai;
	struct	in_addr	recvaddr;
	session 	*rs;
	rtperror	err = 0;
	char		*DST_IP;//DEBUG
	
	/* get the session of session_list[sid] to " rs " */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	/* the function should not be called during connecting */
	if (rs->connecting) {
		return RTP_ALREADY_FIXED;
	}		

	/* malloc a new address structure */
	ai = (address_info_t *) malloc(sizeof(address_info_t));
	if (!ai) {
		return RTP_CANT_ALLOC_MEM;
	}
		
	/* translate address, return in network byte order */
	recvaddr = host2ip(addr);	
	if (recvaddr.s_addr == ILLEGAL_ADDR) {
		/* invalid address */
		free(ai);
		return RTP_BAD_ADDR;
	}
	
	/* we assume it's a RTCP port if port is odd */
	if (port % 2) {
		port--;			
	}
	
	if (!rs->recvlist) {
		/* recvlist is allocated before */
		free(rs->recvlist);
		rs->recvlist = NULL;
		
		/* if a source changes its source transport address, it 
		 * "may" also choose a new SSRC identifier to avoid being 
		 * interpreted as a looped source. (This is not must because 
		 * in some applications of RTP sources may be expectedto 
		 * change addresses during a session.) */
		
		/* u can change a new ssrc here if u want */
	}
	
	/* Add the address to the first of RecvAddr list */
	ai->next = rs->recvlist;
	rs->recvlist = ai;
	if (!rs->recvlist) { //NULL
		return ADD_RECVLIST_FAIL;
	}
	
	ai->addr = recvaddr;
	
	if (!addr) { // NULL
#if RTP_DEBUG	
		printf("MSG	: recvaddr = INADDR_ANY\n"); // DEBUG_ONLY
#endif
		ai->addr.s_addr = INADDR_ANY; // 0
	}
	
	ai->port = htons(port);
	ai->ttl = 0;
	ai->rtp_protocol = rtp_p;
	ai->rtcp_protocol = rtcp_p;
	
	// DEBUG_ONLY
	DST_IP = strdup(inet_ntoa(recvaddr));
	if (rtp_p == udp) {
#if RTP_DEBUG	
		printf("MSG	: rtp_add_sour_recvlist - recvaddr:%s, port:%d, protocol:udp\n", DST_IP, port);//DEBUG
#endif
	}
	else {
#if RTP_DEBUG	
		printf("MSG	: rtp_add_sour_recvlist - recvaddr:%s, port:%d, protocol:tcp\n", DST_IP, port);//DEBUG
#endif
	}
	free(DST_IP);
	// DEBUG_ONLY
	
	/* new recv_addr in recvlist */
	new_recv_addr = TRUE;
	
	return err;
	
} /* rtp_add_sour_recvlist */


/* 
 * rtp_del_sour_list - delete recvive address
 *
 * after function is called , we won't receive any RTP packet, 
 * but the RTCP which is the next port is still working!!!
 * we can't delete the receive address during the sessions.
 * PS. each session we only one translation address to receive packet
 */
rtperror 
rtp_del_sour_list(session_id sid){
	
	rtperror	err = 0;
	session 	*rs; 
	
	/* get the session of session_list[sid] to " rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	if (!rs->recvlist) { // NULL
		return NO_RTP_RECV_ADDR_LIST;
	}
	
	/* the function should not be called during connecting */
	if (rs->connecting) {
		return RTP_ALREADY_FIXED;
	}
	
	err = socket_close(rs->recvlist->rtpskt);
	if (err > 0) {
		return err;
        }
	
	err = socket_close(rs->recvlist->rtcpskt);
        if (err > 0) {
		return err;
        }
	
	free(rs->recvlist);
	rs->recvlist = NULL;
    
	/* there is no new recv_addr in recvlist */
	new_recv_addr = FALSE;

	return err;
         	
} /* rtp_del_sour_list */


/*
 * rtp_get_sour_list - get the address and port in host order if it exist, 
 * 			   else return a WARNING of rtperror - "NO_RTP_RECV_ADDR_LIST".
 *
 * port is for rtp, ( port + 1 ) is for rtcp.
 */
rtperror 
rtp_get_sour_list(session_id sid, char *addr, u_int16 *port, protocol_t *rtp_p, protocol_t *rtcp_p){
	
	rtperror	err = 0;
	session		*rs; 
	
	/* get the RTPsession of RTPsessionlist[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;	
	}
	
	if (!rs->recvlist) { // NULL
		return NO_RTP_RECV_ADDR_LIST;
	}
	
	addr = strdup(inet_ntoa(rs->recvlist->addr));
	*port = ntohs(rs->recvlist->port);
	*rtp_p = rs->recvlist->rtp_protocol;
	*rtcp_p = rs->recvlist->rtcp_protocol;
	
	return err;	
						
} /* rtp_get_sour_list */  


/* 
 * rtp_open_connection - open RTP connection
 *
 * if port of destination == 0, 
 * we assign port by linearly increasing for finding free ports.
 */
rtperror 
rtp_open_connection(session_id sid){

	rtperror 	   err = 0;
	session  	   *rs; 
	address_info_t 	   *ai;
	int		   recv = 1, send = 0;/* type of OpenConnect */
	
	/* get the RTPsession of RTPsessionlist[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;	
	}
	
	if (rs->connecting) {
		return RTP_CONNECT_ALREADY_OPENED;
	}
	
/* now, we set the recvlist if it exist */

	if (new_recv_addr) { // new_recv_addr is true also means that ( rs->recvlist != NULL )
		
	/* now, we need to gets ports to bind */
		
#if RTP_DEBUG	
		printf("MSG	: rtp_open_connection - new receiver!\n");//DEBUG
#endif		
      		while (getport < PORT_UPPER - 1) {      		
      		/* here we handle tcp or udp only */
      		
      		/* dynamic ports in the dynamic range from 5004 to 65536 */
			if (!rs->recvlist->port) {// port == 0 
#if RTP_DEBUG	
				printf("MSG	: rtp_open_connection - dynamic port open!!\n");//DEBUG
#endif

				dynamic_port = TRUE;
				rs->recvlist->port = getport;	
			}
			else {
				dynamic_port = FALSE;
			}
			
      			/* open recv_addr connection */
      			err = open_connection(sid, rs->recvlist, recv); 	
      			if (err == RTP_CANT_BIND_SOCKET) {	/* bind error */
				/* if dynamic_port is true, try other ports */	
				if (dynamic_port) {
					getport += 2;
					rs->recvlist->port = 0;
				
					err = socket_close(rs->recvlist->rtpskt);
					if (err) {
						return err;
					}
					
					err = socket_close(rs->recvlist->rtcpskt);
					if (err) {
						return err;
					}

					continue;
				}
				/* port is specified && can't bind it */
				else {
					err = socket_close(rs->recvlist->rtpskt);
					if (err) {
						return err;
					}
					
					err = socket_close(rs->recvlist->rtcpskt);
					if (err) {
						return err;
					}

      					return RTP_CANT_BIND_SOCKET;
				}
			}
			/* OK */
			else if (!err) { 
				break;
			}
			/* other rtperrors */
        		else {
				err = socket_close(rs->recvlist->rtpskt);
				if (err) {
					return err;
				}
					
				err = socket_close(rs->recvlist->rtcpskt);
				if (err) {
					return err;
				}

				return err;
			}
		} /* while, getting port */
		
		/* if there is not enough ports we can use */
		if (getport >= PORT_UPPER - 1) {
			return RTP_PORT_EXHAUST;
		}

		/* Allow reuse of the address and port 
		if (IsMulticast(rs->recvlist->addr)) { // Multicast 

			DST_IP = strdup(inet_ntoa(rs->recvlist->addr)); //DEBUG_ONLY
			printf("MSG	: recvlist addr is multicast = %s\n", DST_IP); //DEBUG_ONLY
						
			// Every member of the session is a member of the multicast session 
			memset(&s, 0, sizeof(s));
			s.sin_family = AF_INET;
			s.sin_addr = rs->recvlist->addr;
			s.sin_port = rs->recvlist->port;
	
			if( (JoinMulticast(rs->recvlist->rtpskt, &s)) < 0) {
				SocketClose (rs->recvlist->rtpskt);
				SocketClose (rs->recvlist->rtcpskt);					
				return RTP_CANT_SET_SOCKOPT;
			}
	
			if( (JoinMulticast(rs->recvlist->rtcpskt, &s)) < 0) {
				SocketClose (rs->recvlist->rtpskt);
				SocketClose (rs->recvlist->rtcpskt);					
				return RTCP_CANT_SET_SOCKOPT;
			}
		}*/	
			
		new_recv_addr = FALSE;
		
		/* change session status 
		if (rs->rtpstatus == INACTIVE) {
			rs->rtpstatus = RECVONLY;
		}
		else if (rs->rtpstatus == SENDONLY) {
			rs->rtpstatus = SENDRECV;
		}
		*/
		
  	} /* new_recv_addr */

/* now, we set the sendlist if it exist */
	if (new_send_addr) {	// new_send_addr is not NULL
  		
#if RTP_DEBUG	
  		printf("MSG	: rtp_open_connection - add new sender\n");//DEBUG
#endif
  		
  		ai = rs->sendlist;
		
  		while (ai) {	
  		
			err = open_connection(sid, ai, send);
			if (err > 0) {
				return err;
			}
		
			/* determine the address is multicast or not 
			if (IsMulticast(ai->addr)) 
			{
				DST_IP = strdup(inet_ntoa(ai->addr)); //DEBUG_ONLY
				printf("MSG	: ;sendlist addr is multicast = %s\n", DST_IP); //DEBUG_ONLY	
							
				// Set multicast TTL if needed 
				
				// RTP 
				if ( SetTTL(ai->rtpskt, ai->ttl) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					return RTP_CANT_SET_SOCKOPT;
				}
				
				// RTCP
				if ( SetTTL(ai->rtcpskt, ai->ttl) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					return RTCP_CANT_SET_SOCKOPT;		
				}
				
				// for rtp_sourceaddr, rtcp_sourceaddr that,
				 * sending in multicast	- The local source addresses for RTP and RTCP 
				 * sending in unicast	- AF_UNSPEC address family for loopback detection.
				 //		
				if ( GetSocketName(ai->rtpskt, &(rs->rtp_sourceaddr)) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTP_CANT_GET_SOCKOPT;				
				}
				
				if ( GetSocketName(ai->rtcpskt, &(rs->rtcp_sourceaddr)) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTCP_CANT_GET_SOCKOPT;				
				}
			} // IsMulticast 
			*/
			ai = ai->next; 	
  		
		} /* while (ai) */
		
  		new_send_addr = FALSE;
		
		/* change session status 
		if (rs->rtpstatus == INACTIVE) {
			rs->rtpstatus = SENDONLY;
		}
		else if (rs->rtpstatus == RECVONLY) {
			rs->rtpstatus = SENDRECV;
		}
		*/
		
  	} /* new_send_addr */
	
  	rs->connecting = TRUE;
	
	if (rs->rtcpopen) {// if it is flase, we only receive rtcp packet without sending rtcp packet.
		err = rtcp_start(sid);
		if (err > 0) {
			return err;
		}
	}
	else {
		return NO_RTCP_WARNING;
	}
	
  	return err;
  	
} /* rtp_open_connection */


/* 
 * rtp_close_connection - close rtp connection.
 */
rtperror 
rtp_close_connection(session_id sid, char *reason){

	address_info_t 	*ai;
	session 	*rs;
	rtperror 	err = 0;
	
	/* get the session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err) {
		return err;	
	}
	
#if RTP_DEBUG	
	printf("MSG	: rtp_close_connection - rs->connecting = %d\n", rs->connecting);//DEBUG_ONLY
	fflush(stdout);//DEBUG_ONLY
#endif
	
	/* the connection is already closed */
	if (!rs->connecting) {
		return RTP_CONNECT_ALREADY_CLOSED;
	}
	
	/* auto	close rtcp */
	
	if (err > 0) {
		return err;
	}
	
	if (rs->recvlist) {
		
#if RTP_DEBUG	
		printf("MSG	: rs->recvlist->rtpskt = %d\n", rs->recvlist->rtpskt);//DEBUG_ONLY
#endif
		
		/* close RTP session socket */
		if (rs->recvlist->rtpskt){		
		
			if ( (close(rs->recvlist->rtpskt)) < 0) {
				return RTP_CANT_CLOSE_SESSION;
			}
		}
	
#if RTP_DEBUG	
		printf("MSG	: rs->recvlist->rtpskt in RTPCloseConnect:%d\n", rs->recvlist->rtpskt);//DEBUG_ONLY

		printf("MSG	: rs->recvlist->rtcpskt = %d\n", rs->recvlist->rtcpskt);//DEBUG_ONLY
#endif			
		/* close RTCP session socket */
		if (rs->recvlist->rtcpskt) {
	
			if ( (close(rs->recvlist->rtcpskt)) < 0) {
				return RTCP_CANT_CLOSE_SESSION;
			}
		}
	
#if RTP_DEBUG	
		printf("MSG	: rs->recvlist->rtcpskt in RTPCloseConnect:%d\n", rs->recvlist->rtcpskt);//DEBUG_ONLY
#endif
	}
	
	ai = rs->sendlist;
	
	while (ai) {

		if ( close(ai->rtpskt) < 0) {
			return RTP_CANT_CLOSE_SESSION;
		}
		
#if RTP_DEBUG	
		printf("MSG	: close rs->sendlist at address :%p\n", ai);//DEBUG_ONLY		
		printf("MSG	: rs->sendlist in RTPCloseConnect:%d\n", ai->rtpskt);//DEBUG_ONLY
#endif			
		if ( close(ai->rtcpskt) < 0) {
			return RTCP_CANT_CLOSE_SESSION;
		}
		
#if RTP_DEBUG	
		printf("MSG	: rs->sendlist in RTPCloseConnect:%d\n", ai->rtcpskt);//DEBUG_ONLY
#endif		
		ai = ai->next;
	}

	rs->connecting = FALSE;

	return err;
	
} /* rtp_close_connection */


/* 
 * rtp_get_sour_rtpsocket - return the rtp socket of the session.
 */
rtperror
rtp_get_sour_rtpsocket(session_id sid, socktype *skt){

	session		*rs;
	rtperror	err = 0;
	
	/* get the session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;	
	}
	
	if (!rs->connecting) {
		return RTP_NO_CONNECTION;
	}
	
	*skt = rs->recvlist->rtpskt;
	
	if (*skt <= 0) {
		return INVALID_SKT;
	}
	
#if RTP_DEBUG	
	printf("MSG	: rtp_get_sour_rtpsocket - rtp socket = %d\n", *skt);//DEBUG
#endif	
	return err;

} /* rtp_get_sour_rtpsocket */


/* 
 * rtp_get_sour_rtcpsocket - return the rtcp socket of the session.
 */
rtperror
rtp_get_sour_rtcpsocket(session_id sid, socktype *skt){

	session		*rs;
	rtperror	err = 0;

	/* get the session of session_list[sid] to " rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;	
	}
	
	if (!rs->connecting) {
		return RTP_NO_CONNECTION;
	}
	
	*skt = rs->recvlist->rtcpskt;

	if (*skt <= 0) {
		return INVALID_SKT;
	}
		
#if RTP_DEBUG	
	printf("MSG	: rtcp_get_sour_rtpsocket - rtcp socket = %d\n", *skt);//DEBUG
#endif
	
	return err;
		
} /* rtp_get_sour_rtcpsocket */


/* 
 * rtp_send - Send RTP packet
 *
 * addts	: the imcrement of timestamp.
 * pt		: pauload type identifier .
 * payload	: the pointer points to buffer.
 * plen		: the data length in the buffer.
 */
rtperror 
rtp_send(session_id sid, int8 marker, int32 addts, int16 pt, int8 *payload, int plen){
	
	rtperror		err = 0;
	session			*rs;
	address_info_t		*ai;
	char			*ptr, *DST_IP;
	char			sendbuf[RTP_MAX_PKT_SIZE];
	struct sockaddr_in	sa;
	socklen_t		salen;
	struct sockaddr		*s;
	int			send = 0;	
	int			result;	/* return value of sendto()*/
	int			rtplen;	/* rtp pkt length */
	
	/* get the session of session_list[sid] to " rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;	
	}
		
	/* if there is new address add into sendlist */
	if (new_send_addr) {
  		
#if RTP_DEBUG	
  		printf("MSG	: new_send_addr in RTPSend is TRUE, we sholud add it into sendlist\n");//DEBUG_ONLY
#endif
  		
  		ai = rs->sendlist;
  		
  		while (ai) {
			//for TCP connect only
			//memset(&sa, 0, sizeof(sa));	
			//sa.sin_family = AF_INET;
			//sa.sin_addr = ai->addr;
			//sa.sin_port = ai->port;
  			
			err = open_connection(sid, ai, send);
			if (err > 0) {
				return err;
			}
			
			/* determine the address is multicast or not 
			if (IsMulticast(ai->addr)) 
			{
				DST_IP = strdup(inet_ntoa(rs->recvlist->addr));
				printf("MSG	: sendlist addr is multicast of RTPSend = %s\n", DST_IP);
								
				// Set multicast TTL if needed 
				
				// RTP 
				if ( SetTTL(ai->rtpskt, ai->ttl) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTP_CANT_SET_SOCKOPT;
				}
				
				// RTCP 
				if ( SetTTL(ai->rtcpskt, ai->ttl) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
			 
					return RTCP_CANT_SET_SOCKOPT;		
				}
				
				// for rtp_sourceaddr, rtcp_sourceaddr that,
				 * sending in multicast	- The local source addresses for RTP and RTCP 
				 * sending in unicast	- AF_UNSPEC address family for loopback detection.
				 //		
				if ( GetSocketName(ai->rtpskt, &(rs->rtp_sourceaddr)) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTP_CANT_GET_SOCKOPT;				
				}
				
				if ( GetSocketName(ai->rtcpskt, &(rs->rtcp_sourceaddr)) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTCP_CANT_GET_SOCKOPT;				
				}
			} // if		
			*/
			ai = ai->next; 					
  		} // while
  		new_send_addr = FALSE;
  	} /* new_send_addr */	
	
	ptr = sendbuf;
	
	/* build rtp header, "rtplen" parameter can get the rtp_header length */
	//err = build_rtp_pkt(sid, marker, addts, pt, &sendbuf, &rtplen);
	err = build_rtp_pkt(sid, marker, pt, (char *)payload, &rtplen, plen, sendbuf);
	if (err > 0) { 
		return err;
	}
	
	/* update states before send because there may has no address we need to send */
	err = update_by_send_rtp(sid, addts, plen);
	if (err > 0) {
		return err;
	}
	
	/* the send_addr_list should not be NULL */
	if (!rs->sendlist) {
		return RTP_NO_SENDLIST;
	}
	
	ai = rs->sendlist;	
	
	/*send packet to all addresses in send_addr_list */
	while (ai){
		if (ai->living) {
		
			if (ai->rtp_protocol == udp) {
				
				memset(&sa, 0, sizeof(sa));
				sa.sin_family = AF_INET;
				sa.sin_addr = ai->addr;
				
				DST_IP = strdup(inet_ntoa(ai->addr));				
				sa.sin_port = ai->port;
#if RTP_DEBUG	
				printf("MSG	: rtp_send - sending address = %s, port = %d, on socket %d\n", DST_IP, ntohs(ai->port), ai->rtpskt);//DEBUG_ONLY
#endif
				free(DST_IP);
				
				s = (struct sockaddr *) &sa;
				salen = sizeof(sa);
				
				result = sendto(ai->rtpskt, sendbuf, rtplen, 0, s, salen);
				
				if (result < 0) {
					/* the address may failed, we should handle the event and send other
					 * addresses.
					 */
					fprintf(stderr, "ERROR	: rtp_send - errno = %d\n", errno);
					
					//ENOBUFS         55              /* No buffer space available */
					//     [ENOBUFS]          The system was unable to allocate an internal buffer.
					//                        The operation may succeed when buffers become avail-
					//                        able.
					/*
					if (errno == 55) {
						while (result < 0 && try_again <= 3) {
							usleep(1);
							result = sendto(ai->rtpskt, sendbuf, rtplen, 0, s, salen);

							if (result < 0) {
								try_again++;
								printf("ERROR	: rtp_send - errno = %d\n", errno);
							}
						}
						if (result < 0) {
							return RTP_SEND_ERR;
						}
					}
					*/
				}
			}
			else if (ai->rtp_protocol == tcp) {
				// UNDO
			}
			else {
				return RTP_UNKNOWN_PROTOCOL;
			}
		} // if
		ai = ai->next;	
	} // while
	
	/* delete the address if senderrcallback using */
		
	return err;
				
} /* rtp_send */


/* 
 * rtcp_send - send rtcp packet.
 */
rtperror 
rtcp_send(session_id sid, event_type type){
	
	rtperror 	   err = 0;
	session 	   *rs;
	address_info_t     *ai;
	struct sockaddr_in sa;
	socklen_t 	   salen;
	struct sockaddr    *s;
	int 		   send = 0;	
	char		   *DST_IP;
	char 		   sendbuf[RTCP_MAX_PKT_SIZE];
	/* rtcp pkt length */
	int 		   rtcplen, result;
	
	/* get the session of session_list[sid] to " rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
/* if there is new address add into sendlist */
	if (new_send_addr) {

#if RTP_DEBUG	
		printf("MSG	: rtcp_send - new_send_addr in RTCPSend is TRUE, we sholud add it into sendlist\n");//DEBUG_ONLY
#endif

		ai = rs->sendlist;

	  	while (ai) {	
			//memset(&sa, 0, sizeof(sa));	
			//sa.sin_family = AF_INET;
			//sa.sin_addr = ai->addr;
			//sa.sin_port = ai->port;

			err = open_connection(sid, ai, send);
			if (err > 0) {
				return err;
			}
			
			/* determine the address is multicast or not 
			if (IsMulticast(ai->addr))
			{
				DST_IP = strdup(inet_ntoa(rs->recvlist->addr));
				printf("MSG	: sendlist addr is multicast of RTPSend = %s\n", DST_IP);

				// Set multicast TTL if needed 
				
				// RTP 
				if ( SetTTL(ai->rtpskt, ai->ttl) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);	
			
					return RTP_CANT_SET_SOCKOPT;
				}
				
				// RTCP 
				if ( SetTTL(ai->rtcpskt, ai->ttl) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTCP_CANT_SET_SOCKOPT;	
				}
				
				// for rtp_sourceaddr, rtcp_sourceaddr that,
				// sending in multicast	- The local source addresses for RTP and RTCP
				// sending in unicast	- AF_UNSPEC address family for loopback detection.
				 //
				if ( GetSocketName(ai->rtpskt, &(rs->rtp_sourceaddr)) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);			
					
					return RTP_CANT_GET_SOCKOPT;			
				}
				
				if ( GetSocketName(ai->rtcpskt, &(rs->rtcp_sourceaddr)) < 0) {
					SocketClose(ai->rtpskt);
					SocketClose(ai->rtcpskt);
					free(ai);		
					
					return RTCP_CANT_GET_SOCKOPT;	
				}
			}*/
			ai = ai->next; 			
		}
		new_send_addr = FALSE;
	} /* new_send_addr */	

	/* check bandwidth is 0 or not */
	if (!rs->session_bw || !rs->rtcp_bw_fraction) {
		return NO_RTCP_WARNING;
	}

	/* build rtcp packet */
	err = build_rtcp_pkt(sid, sendbuf, &rtcplen, type);
	if (err > 0) {
		return err;
	}

	ai = rs->sendlist;
	
	/*send packet to all addresses in send_addr_list */
	while (ai){
		if (ai->living) {

			if (ai->rtcp_protocol == udp) {
				
				struct timeval send;
				
				memset(&sa, 0, sizeof(sa));
				sa.sin_family = AF_INET;
				sa.sin_addr = ai->addr;
				DST_IP = strdup(inet_ntoa(ai->addr));
		
				sa.sin_port = htons(ntohs(ai->port) + 1);

#if RTP_DEBUG	
				printf("MSG	: rtcp_send - [%u] sending address = %s, port = %d, on socket %d\n", rs->rtp_sendpkt.rtphdr.ssrc, DST_IP, ntohs(sa.sin_port), ai->rtcpskt);//DEBUG_ONLY
#endif
				free(DST_IP);
				
				s = (struct sockaddr *) &sa;
				salen = sizeof(sa);
				
				result = sendto(ai->rtcpskt, sendbuf, rtcplen, 0, s, salen);
				
				gettimeofday(&send, NULL);
#if RTP_DEBUG	
				printf("MSG	: rtcp_send - real time to send = %lf\n", timeval_to_double(send));//DEBUG
				
				printf("MSG	: rtcp_send - sending result = %d\n", result);//DEBUG_ONLY
#endif				
				if (result < 0) {
					/* the address may failed, we should handle the event and send other
					 * addresses.
					 */
					
					fprintf(stderr, "MSG	: rtcp sendto errno = %d\n", errno);
					//EMSGSIZE        40              /* Message too long */
					//return RTCP_SEND_ERR;				
				}
			}
			else if (ai->rtcp_protocol == tcp) {
				// UNDO
			}
			else {
				return RTP_UNKNOWN_PROTOCOL;
			}
		} // if
		ai = ai->next;	
	} // while
	
	/* should free the address in send_addr_list */
	
	/* update the sending status */
	rs->send_rtcpbytes_num += rtcplen;
	rs->send_rtcppkt_num++;
	
	/* for sdes */
	rs->number_of_rtcp_send++;

	return err;
	
} /* rtcp_send */


/* 
 * on_receive - Receive whether a RTP packet or a RTCP packet
 *		specified in A.7 of RFC-3550.
 */
rtperror 
on_receive(session_id sid, socktype skt, char *buf, int *blen){
	
	rtperror 	   err = 0;
	session 	   *rs;
	struct sockaddr_in *from_in;
	int 		   result, is_rtp;
	socklen_t 	   fromlen;
	struct sockaddr    from;
		
	/* get the session of session_list[sid] to " rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;	
	}
	
	/* check if the skt is either RTP or RTCP */
	if (skt == rs->recvlist->rtpskt) {
		is_rtp = 1;
	}
	else if (skt == rs->recvlist->rtcpskt) {
		is_rtp = 0;
	}
	else {
		return RTP_SKT_NOMATCH;
	}
	
	fromlen = sizeof(from);
	//printf("MSG	: on_receive - is_rtp = %d, socket = %d\n", is_rtp, skt);//DEBUG_ONLY
	//printf("MSG	: on_receive - buf address = %p, len = %d\n", buf, *blen);//DEBUG_ONLY
	
	memset(buf, '\0', *blen);
	
	result = recvfrom(skt, buf, *blen, 0, &from, &fromlen);
	
	//printf("MSG	: on_receive - we get %d bytes in on_receive\n", result);//DEBUG_ONLY
	//fflush(stdout);//DEBUG_ONLY
	
	if (result < 0) {
		return RTP_SOCKET_READ_FAILURE;
	}
	
	/* receive bytes may bigger then buffer size */	
	if (result == *blen) {
		fprintf(stderr, "The user buffer is not enough to receive RTP/RTCP packet\n");
	}
	
	from_in = (struct sockaddr_in *) (&from);
	
	/* the packet we receive is rtp or rtcp, s is for lookback checking 
	if (is_rtp) {
		s = (struct sockaddr_in *)&(rs->rtp_sourceaddr);
	}
	else {
		s = (struct sockaddr_in *)&(rs->rtcp_sourceaddr);
	}
	*/
	
	/* What we do depends on whether we have left the group, and are
	 * waiting to send a BYE (TypeOfEvent(e) == EVENT_BYE) or an RTCP
	 * report. specified in A.7 of RFC 3550. */
	
	if (is_rtp) {
		err = receive_rtp_pkt(sid, buf, result, &from, &fromlen);
		if (err) {
			return err;
		}
	}
	else {
		err = receive_rtcp_pkt(sid, buf, result, &from, &fromlen);
		if (err) {
			return err;
		}
	}

	return err;
	
} /* on_receive */

