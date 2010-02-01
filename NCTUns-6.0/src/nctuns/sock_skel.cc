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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <errno.h>

#include "sock_skel.h"
#include "nctuns_syscall.h"

typedef unsigned short u_short; 
extern int      errno;
u_short 	portbase = 0;   /* port base, for non-root servers */
const int 	on = 1;                                                                                                                                    
/*------------------------------------------------------------------------
 * passivesock - allocate & bind a server socket using TCP or UDP
 *------------------------------------------------------------------------
 */

int passivesock(const char *service, const char *transport, int qlen)
/*
 * Arguments:
 *      service   - service associated with the desired port
 *      transport - transport protocol to use ("tcp" or "udp")
 *      qlen      - maximum server request queue length
 */
{
        struct servent  *pse;   /* pointer to service information entry */
        struct protoent *ppe;   /* pointer to protocol information entry*/
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int    s, type;         /* socket descriptor and socket type    */
                                                                                                                                                             
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = INADDR_ANY;
                                                                                        
        /* Map service name to port number */ 
        if ( (pse = getservbyname(service, transport)) )
                sin.sin_port = htons(ntohs((u_short)pse->s_port) + portbase);
        else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 ) {
                printf("can't get \"%s\" service entry\n", service);
		exit(1);
	}
                                  
        /* Map protocol name to protocol number */
        if ( (ppe = getprotobyname(transport)) == 0) {
                printf("can't get \"%s\" protocol entry\n", transport);
		exit(1);
	}
   
        /* Use protocol to choose a socket type */
        if (strcmp(transport, "udp") == 0)
                type = SOCK_DGRAM;
        else
                type = SOCK_STREAM;

        /* Allocate a socket */
        s = socket(PF_INET, type, ppe->p_proto);
        if (s < 0) {
                printf("can't create socket: %s\n", strerror(errno));
		exit(1);
        }                                                                                                                                  
	if ( setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on)) != 0 ) {
		printf("set SO_REUSEADDR failed\n");
		exit(1);
	}
        if (type == SOCK_STREAM) {
          if (setsockopt(s, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on)) != 0 ) {
                printf("set TCP_NODELAY failed\n");
                exit(1);
          }
        }
        /* Bind the socket */
        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                printf("can't bind to %s port: %s\n", service,
                        strerror(errno));
		exit(1);
	}
        if (type == SOCK_STREAM && listen(s, qlen) < 0) {
                printf("can't listen on %s port: %s\n", service,
                        strerror(errno));
		exit(1);
	}
        return s;
}

/*------------------------------------------------------------------------
 * connectsock - allocate & connect a socket using TCP or UDP
 *------------------------------------------------------------------------
 */
int connectsock(const char *host, const char *service, const char *transport, const int cancel_nodeID)
/*
 * Arguments:
 *      host      - name of host to which connection is desired
 *      service   - service associated with the desired port
 *      transport - name of transport protocol to use ("tcp" or "udp")
 *      cancel_nodeID - whether cancel the nodeID for this socket
 */
{
        struct hostent  *phe;   /* pointer to host information entry    */
        struct servent  *pse;   /* pointer to service information entry */
        struct protoent *ppe;   /* pointer to protocol information entry*/
        struct sockaddr_in sin; /* an Internet endpoint address         */
        int     s, type;        /* socket descriptor and socket type    */
                                                                                                                                                             
        memset(&sin, 0, sizeof(sin));
        sin.sin_family = AF_INET;

        /* Map service name to port number */
        if ( (pse = getservbyname(service, transport)) )
                sin.sin_port = pse->s_port;
        else if ( (sin.sin_port = htons((u_short)atoi(service))) == 0 ) {
                printf("can't get \"%s\" service entry\n", service);
		exit(1);
        }                                                                                     
        /* Map host name to IP address, allowing for dotted decimal */
        if ( (phe = gethostbyname(host)) )
                memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
        else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE ) {
                printf("can't get \"%s\" host entry\n", host);
		exit(1);
	}                                             

        /* Map transport protocol name to protocol number */
        if ( (ppe = getprotobyname(transport)) == 0) {
                printf("can't get \"%s\" protocol entry\n", transport);
		exit(1);
	}
        /* Use protocol to choose a socket type */
        if (strcmp(transport, "udp") == 0)
                type = SOCK_DGRAM;
        else
                type = SOCK_STREAM;
        /* Allocate a socket */
        s = socket(PF_INET, type, ppe->p_proto);
        if (s < 0){
                printf("can't create socket: %s\n", strerror(errno));
		exit(1);
        }
	/* cancel nodeID for this socket */
	if (cancel_nodeID) {
		if (syscall_NCTUNS_cancel_socknodeID(s) < 0) {
			printf("can't cancel nodeID for socket = %d\n",
				s);
			exit(1);
		}
	}

        /* Connect the socket */
        if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
                printf("can't connect to %s.%s: %s\n", host, service,
                        strerror(errno));
		exit(1);
	}
        return s;
}

/*------------------------------------------------------------------------
 * passiveUDP - create a passive socket for use in a UDP server
 *------------------------------------------------------------------------
 */
int passiveUDP(const char *service)
/*
 * Arguments:
 *      service - service associated with the desired port
 */
{
        return passivesock(service, "udp", 0);
}

/*------------------------------------------------------------------------
 * passiveTCP - create a passive socket for use in a TCP server
 *------------------------------------------------------------------------
 */
int passiveTCP(const char *service, int qlen)
/*
 * Arguments:
 *      service - service associated with the desired port
 *      qlen    - maximum server request queue length
 */
{
        return passivesock(service, "tcp", qlen);
}

/*------------------------------------------------------------------------
 * connectTCP - connect to a specified TCP service on a specified host
 *------------------------------------------------------------------------
 */
int connectTCP(const char *host, const char *service, const int cancel_nodeID)
/*
 * Arguments:
 *      host    - name of host to which connection is desired
 *      service - service associated with the desired port
 *      cancel_nodeID - whether cancel the nodeID for this socket
 */
{
        return connectsock( host, service, "tcp", cancel_nodeID);
}

/*------------------------------------------------------------------------
 * connectUDP - connect to a specified UDP service on a specified host
 *------------------------------------------------------------------------
 */
int connectUDP(const char *host, const char *service, const int cancel_nodeID)
/*
 * Arguments:
 *      host    - name of host to which connection is desired
 *      service - service associated with the desired port
 *      cancel_nodeID - whether cancel the nodeID for this socket
 */
{
        return connectsock(host, service, "udp");
}

/*------------------------------------------------------------------------
 * readn -  Must read "n" bytes from a descriptor. 
 *------------------------------------------------------------------------
 */
int readn(int fd, char *vptr, int n)
{
        int     nleft, nread;
        char    *ptr;
                                                                                
        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nread = read(fd, ptr, nleft)) < 0) {
                        if (errno == EINTR)
                                nread = 0;
                        else
                                return(-1);
                } else if (nread == 0)
                        break;
                                                                                
                nleft -= nread;
                ptr   += nread;
        }
        return(n - nleft);
}

/*------------------------------------------------------------------------
 * writen -  Must write "n" bytes to a descriptor. 
 *------------------------------------------------------------------------
 */
int writen( int fd, char *vptr, int n)
{
        int     nleft,nwritten;
        char    *ptr;

        ptr = vptr;
        nleft = n;
        while (nleft > 0) {
                if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
                        if (errno == EINTR)
                                nwritten = 0;
                        else
                                return(-1);
                }
                nleft -= nwritten;
                ptr   += nwritten;
        }
        return(n);
}

