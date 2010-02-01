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
 * The file contains network system call with rtp.
 */


/********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>   	/* timeval */
#include <unistd.h>     	/* get..() */
#include <sys/utsname.h>	/* utsname */
#include <netdb.h>      	/* gethostname() */
#include <sys/socket.h> 	/* AF_UNSPEC */

#include <string.h>		/* strcpy */
#include <sys/param.h>
#include <sys/uio.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <err.h>
#include <errno.h>
#include <ctype.h>

#include "rtp_network_inner.h"
#include "rtp_session_api.h"
#include "rtp_member_api.h"


/********************************************************************/


/* 
 * host2ip - return IP address given host name 'host'.
 * 	     if 'host' is not valid, set to INADDR_ANY. return value is in network byte order
 */
struct in_addr 
host2ip(char *host)
{	
	struct in_addr 		in;
	register struct hostent *hep;

	/* Strip leading white space. */
	if (host) {
		while (*host && isspace((int)*host)) { 
			host++;  
		}
	}

	/* Check whether this is a dotted decimal. */
	in.s_addr = INADDR_ANY;
	
	if (!host){
	}
	else if ((in.s_addr = inet_addr(host)) != -1) {
	}
	/* Attempt to resolve host name via DNS. */
	else if ((hep = gethostbyname(host))) {
		in = *(struct in_addr *)(hep->h_addr_list[0]);
	}
	/* As a last resort, try YP. 
	else {
		static char *domain = 0;  // YP domain 
		char *value;              // key value 
		int value_len;            // length of returned value 

		if (!domain) {
			yp_get_default_domain(&domain);
		}
		
		if (yp_match(domain, "hosts.byname", host, strlen(host), &value, &value_len) == 0) {
			in.s_addr = inet_addr(value);
		}	
	}
	*/
	return in;
	
} /* host2ip */


/* 
 * is_multicast - determine if the address is multicast or not.
 * 		 unlike IN_MULTICAST, take a struct in_addr in network byte order. 
 */
int 
is_multicast(struct in_addr addr){
	
        unsigned int maddr = ntohl(addr.s_addr);

        if (((maddr >> 28) & 0xf) == 0xe) {
                return 1;
        }
        else {
                return 0;
        }  
} /* IsMulticast */


/*
 * socket_close - close socket.
 */ 
rtperror
socket_close(socktype skt){
	
	int result;
	
	result = close(skt);

	if (result < 0) {
		fprintf(stderr, "close errno:%d\n", errno);
		return RTP_SOCKET_CLOSE_FAIL;
	}	
	else {
		return 0;
	}
		
} /* socket_close */


/* 
 * socket_connect - connect socket.
 */
rtperror
socket_connect(socktype skt, struct sockaddr_in *s){
	
	int result;

	result = connect(skt, (struct sockaddr *) s, sizeof(struct sockaddr_in));

	if (result < 0) {
		fprintf(stderr, "connect errno:%d\n", errno);
		return RTP_SOCKET_CONNECT_FAIL;
	}
	else {
		return 0;
	}
	
} /* socket_connect */


/* 
 * open_connection - create socket for rtp connection.
 *
 * if int type == 0, ai == sendlist. else ai == recvlist.
 */
rtperror
open_connection(session_id sid, address_info_t *ai, int type){

	int			result;
	struct	sockaddr_in	s;
	member_t		*member;
	struct	sockaddr	*from;
	session			*rs;
	rtperror		err = 0;
	
	if (type) {//receiver
#if RTP_DEBUG	
		printf("FUNC	: open_connection - open receiver connection\n");//DEBUG
#endif
	}
	else {
#if RTP_DEBUG	
		printf("FUNC	: open_connection - open sender connection\n");//DEBUG
#endif
	}
			
	/* get the session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
		
	/* create rtp socket depending on underlying protocol */
	if (ai->rtp_protocol == udp) {

		ai->rtpskt = socket(AF_INET, SOCK_DGRAM, 0);
		if (ai->rtpskt < 0) {
			return RTP_CANT_GET_SOCKET;
		}
		
#if RTP_DEBUG	
		printf("MSG	: open_connection - udp rtpskt = %d\n", ai->rtpskt);//DEBUG_ONLY
#endif
	}	
	else if (ai->rtp_protocol == tcp) { // ai->prototype == tcp
		
		ai->rtpskt = socket(AF_INET, SOCK_STREAM, 0);
		if (ai->rtpskt < 0) {
			return RTP_CANT_GET_SOCKET;
		}
		
#if RTP_DEBUG	
		printf("MSG	: open_connection - tcp rtpskt = %d\n", ai->rtpskt);//DEBUG_ONLY
#endif			
		// connect UNDO
	}
	else {
		return RTP_UNKNOWN_PROTOCOL;
	}
	
	/* create rtcp socket depending on underlying protocol */	
	if (ai->rtcp_protocol == udp) {
		ai->rtcpskt = socket(AF_INET, SOCK_DGRAM, 0);
		if (ai->rtcpskt < 0) {
			return RTP_CANT_GET_SOCKET;
		}
		
#if RTP_DEBUG	
		printf("MSG	: open_connection - udp rtcpskt = %d\n", ai->rtcpskt);//DEBUG_ONLY
#endif
	}
	else if (ai->rtcp_protocol == tcp) { // ai->prototype == tcp
		
		ai->rtcpskt = socket(AF_INET, SOCK_STREAM, 0);
		if (ai->rtcpskt < 0) {
			return RTP_CANT_GET_SOCKET;
		}
		
#if RTP_DEBUG	
		printf("MSG	: open_connection - tcp rtcpskt = %d\n", ai->rtcpskt);//DEBUG_ONLY
#endif		
		// connect undo	
	} 
	else {
		return RTP_UNKNOWN_PROTOCOL;
	}
	
	/* ai = recvlist */
	if (type) {
		/* reuse addr */
		/*
		result = setsockopt(ai->rtpskt, SOL_SOCKET, SO_REUSEADDR, (char*) &n, sizeof(n));
		if (result) {
			return RTP_CANT_REUSE_ADDR;
		}
			
		result = setsockopt(ai->rtcpskt, SOL_SOCKET, SO_REUSEADDR, (char*) &n, sizeof(n));
		if (result) {
			return RTP_CANT_REUSE_ADDR;
		}
		*/	
		/* reuse port */
		/*
		result = setsockopt(ai->rtpskt, SOL_SOCKET, 15, (char*) &n, sizeof(n));	// SO_REUSEPORT 15  
		if (result) {
			return RTP_CANT_REUSE_PORT;
		}
			
		result = setsockopt(ai->rtcpskt, SOL_SOCKET, 15, (char*) &n, sizeof(n)); // SO_REUSEPORT 15 
		if (result) {
			return RTP_CANT_REUSE_PORT;
		}
		*/	
		/* bind sockets */
		memset(&s, 0, sizeof(s));
		s.sin_family = AF_INET;
			
		s.sin_addr = ai->addr;
		s.sin_port = ai->port;
		
#if RTP_DEBUG	
		DST_IP = strdup(inet_ntoa(s.sin_addr)); //DEBUG_ONLY
		printf("MSG	: open_connection - recv INADDR_ANY = %s\n", DST_IP);//DEBUG
		printf("MSG	: open_connection - recv rtp port = %d\n", ntohs(s.sin_port)); //DEBUG_ONLY
		free(DST_IP);//DEBUG
#endif		
		/* we bind to INADDR_ANY if the address is NULL or multicast */
		if ( (!(ai->addr.s_addr)) ) { // || IsMulticast(s.sin_addr) ) {
			s.sin_addr.s_addr = INADDR_ANY;
#if RTP_DEBUG	
			printf("MSG	: open_connection - recv addr = INADDR_ANY\n");
#endif
		}
			
		/* bind rtp socket */
		result = bind(ai->rtpskt, (struct sockaddr *) &s, sizeof(struct sockaddr));
		
#if RTP_DEBUG	
		printf("MSG	: open_connection - bind rtp result = %d\n", result); //DEBUG_ONLY
#endif
		
		if (result < 0) {

			result = errno;
			
			/* IP address can't match the local interface */
			if (result == EADDRNOTAVAIL) {
				/* kernel decide the IP address with local interface */
				s.sin_addr.s_addr = INADDR_ANY;
					
				result = bind(ai->rtpskt, (struct sockaddr *) &s, sizeof(struct sockaddr));
#if RTP_DEBUG	
				printf("MSG	: open_connection - rebind rtp result = %d", result); //DEBUG_ONLY
#endif
				if (result < 0) {
					result = errno;
				}
			}
				
			/* port is used by the other process */
			if (result == EADDRINUSE) {
				return RTP_CANT_BIND_SOCKET;
			}
			else if (result < 0) {
				fprintf(stderr, "open_connection - bind rtp failed, terminated process.");
				exit(1);	// bad idea...
			}
		}	
		
		member = NULL;	
		/* update local rtp source address */
		err = get_member_by_ssrc(rs, rs->rtp_sendpkt.rtphdr.ssrc, &member);
		if (!member) {
			return RTP_CANT_MATCH_MEMBER;
		}

		from = (struct sockaddr *)&s;
		memcpy(&(member->fromaddr[0]), from, sizeof(struct sockaddr));
		
		/* bind rtcp socket */
		s.sin_port = htons(ntohs(ai->port) + 1);
		
#if RTP_DEBUG	
		printf("MSG	: open_connection - recv rtcp port = %d\n", ntohs(s.sin_port)); //DEBUG_ONLY
#endif			
		result = bind(ai->rtcpskt, (struct sockaddr *) &s, sizeof(struct sockaddr));
			
#if RTP_DEBUG	
		printf("MSG	: open_connection - bind rtcp result = %d\n", result); //DEBUG_ONLY
#endif
		
		if (result < 0) {
			
			result = errno;
			
			/* IP address can't match the local interface */
			if (result == EADDRNOTAVAIL) {
				/* kernel decide the IP address with local interface */
				s.sin_addr.s_addr = INADDR_ANY;
					
				result = bind(ai->rtcpskt, (struct sockaddr *) &s, sizeof(struct sockaddr));
#if RTP_DEBUG	
				printf("MSG	: open_connection - rebind rtcp result = %d", result); //DEBUG_ONLY
#endif				
				if (result < 0) {
					result = errno;
				}
			}
			
			/* port is used by the other process */
			if (result == EADDRINUSE) {
				return RTCP_CANT_BIND_SOCKET;
			}
			else if (result < 0) {
				fprintf(stderr, "open_connection - bind rtcp failed, terminated process.");
				exit(1);	// bad idea...
			}
		}

		/* update local rtcp source address */
		from = (struct sockaddr *)&s;
		memcpy(&(member->fromaddr[1]), from, sizeof(struct sockaddr));
				
	} // if (type)

	return err;
	
} /* open_connection */

