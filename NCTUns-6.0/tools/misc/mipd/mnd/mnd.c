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

/* #define DEBUG_MOBILEIP  */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <arpa/inet.h>

#include <ifaddrs.h>  /* Used by ip2tun(). */

#include "./packetFmt.h"

#define BUFSIZE        100
#define PORT_LISTEN    8118
#define DEFAULT_LIVETIME 2000000  /* microseconds. */
/* packetType :
   ADVERTISE --> advertisement packet from agent.
   CONFIRM --> confirm packet from Mnd.    
   REGISTER--> registration from FA to HA.
   UNKNOWN --> not mobileIP packet. */
/* typedef enum{ADVERTISE, REPLY, REPLY_ACK, REGISTER, REG_REPLY, UNKNOWN} PacketType;  */

static void sig_alarm(int);
int         parseConfig(int, char **);
int         printConfFileUsage();
void        initSockets();
int         handleListenSocket(int);
PacketType  typeOfPacket(char *, struct in_addr *);
int         isAdvertisement(char *, struct sockaddr_in *);
int         sendReplyToAgent(char *);
int         modifyRouteTBL(struct in_addr);
int         ip2tun(struct in_addr *, char*);
int         Sendto(int, void *, size_t, int, const struct sockaddr *, socklen_t);

int                  listenSocket, replySocket;
struct sockaddr_in   localAddr, HA_addr, /* HA_addr is the inner IP of HA */
	             currentAgent, tempAgent;
struct sockaddr_in   bcast_addr;
unsigned int         advertiseValid = 0;
unsigned int         globalt = 0;

int
main(int argc, char *argv[])
{
	int rc;
	fd_set readmask;
	/* pending */
	currentAgent.sin_addr.s_addr = 0;

	parseConfig(argc, argv);

	if(signal(SIGALRM, sig_alarm) == SIG_ERR){
		printf( "[X] signal() error.\n");
		exit(1);
	}
	ualarm(1, 0);

        /* Initialize broadcast socket. */
        initSockets();
        /* printf( "Sockets have been created successfully.\n"); */

        while(1){  
		FD_ZERO(&readmask);
		FD_SET(listenSocket, &readmask);
/* #define DEBUG_MND_SELECT */
#ifdef DEBUG_MND_SELECT
		printf( "mnd: Start to select().\n");
#endif
		rc = select(replySocket+1,  /*pending: last socket!?*/
				(fd_set *)&readmask, NULL, NULL, NULL);
#ifdef DEBUG_MND_SELECT
		printf( "mnd: End of select(), rc=%d, errno:%d\n", rc, errno);
#endif
		if(rc < 0){
			if(errno ==EINTR){
				/* printf( "errno == EINTR.\n"); */
				continue;
			}
			printf( "[X] rc < 0. \n");
			exit(1);
		}
		if(FD_ISSET(listenSocket, &readmask)){
			handleListenSocket(listenSocket);
		}
        }

        return 0;
}

/* Ensure not to exit(default action) when timer runs out. */
static void 
sig_alarm(int signo)
{

	if(advertiseValid != 0) advertiseValid -= 200000;

	signal(SIGALRM, sig_alarm);
	ualarm(200000, 0);

}

/* Parse the config file of MN. 
 * Including the HA addr of MN and addr used to broadcast . */
int  
parseConfig(int argc, char *argv[])
{
	FILE *conf;
	char token1[30], token2[30], token3[30], token4[30];
	if(argc != 2){
		printf( "[MobileIP] mnd Usage:\n");
		printf( "\tmnd confFile\n");
		exit(1);
	}

	if((conf=fopen(argv[1], "r")) == NULL){
		printf( "Open MN configure file fail.\n");
		exit(1);
	}

	/* pending */
	fscanf(conf, "%s%s", token1, token2);
	if(!strcmp(token1, "HA"))
	{
		HA_addr.sin_addr.s_addr = inet_addr(token2);
	}else{
		printf( "Wrong MN config file format.\n");
		exit(1);
	}
	fscanf(conf, "%s%s", token3, token4);
	if(!strcmp(token3, "MN"))
	{
		localAddr.sin_addr.s_addr = inet_addr(token4);
	}else{
		printConfFileUsage();
		exit(1);
	}

	if(fclose(conf) == EOF){  
		printf( "Close MN config file fail.\n");
		/* Though close fail, but we continue to execute. */
	}
	return 0;
}

int
printConfFileUsage()
{
	printf("MobileIP: MN config file format:\n");
	printf("\tHA ip\n");
	printf("\tMN ip\n\n");
	printf("Example: IP of this MN is 1.0.3.2, the wireless interface IP of its HA is 1.0.3.1.\n");
	printf("\tHA 1.0.3.1\n");
	printf("\tMN 1.0.3.2\n");

	return 0;
}

/* Initialize broadcast socket(send "solicitation" to agent) and 
 * listen socket(receive "reply" from agent)
 * reply socket(send "reply" to agent) */
void 
initSockets()
{
	struct sockaddr_in addr;

	/* Initialize listen for reply socket. */
	listenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listenSocket < 0){
		printf( "[X] mnd: Failure in creating listen socket.\n");
		exit(1);
	}
	memset(&addr, 0, sizeof(addr));
	addr.sin_family      = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port        = htons(PORT_LISTEN);

	if(bind(listenSocket, (struct sockaddr *)&addr, sizeof(addr)) < 0){
		printf( "[X] Failure in bind() of listen socket.\n");
		exit(1);
	}

	/* Initialize send reply socket. */
	replySocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(replySocket < 0){
		printf( "[X] Failure in creating reply socket.\n");
		exit(1);
	}
}

/* Handle when receive reply from agent. */
int
handleListenSocket(int listenSock)
{
	int                n, isAtHome;
	struct in_addr     agentOaddr, agentIaddr; /* agent outer/inner addr. */
	struct sockaddr_in from;
	socklen_t          from_len;
	char      buf[BUFSIZE];

	from_len = sizeof(from);
/* #define DEBUG_MN_LISTENSOCKET */
#ifdef DEBUG_MN_LISTENSOCKET
	printf( "Listen socket: Starting to receive advertisement from agent.\n");
#endif
	n = recvfrom(listenSock, buf, BUFSIZE, 0, 
			(struct sockaddr *)&from, &from_len);
#ifdef DEBUG_MN_LISTENSOCKET
	printf( "[O] Listen socket: End to receive advertisement from agent.\n");
#endif
	if(n < 0){
		printf( "[X] Receiving advertisement fails.\n");
		exit(1);
	}

	if(typeOfPacket(buf, &(from.sin_addr)) == ADVERTISE){
		/* During live time, broadcasts from other agents will be 
		 * ignored. */
		if(((struct mipAdvertisePkt *)buf)->agentInnerAddr.s_addr 
			                 != currentAgent.sin_addr.s_addr){ 
			if(advertiseValid != 0){
#ifdef DEBUG_ADVERTISE
				printf( "Skip advertisement from %s\n", 
			  	                 inet_ntoa(((struct mipAdvertisePkt *)buf)->agentInnerAddr)); 
#endif
				return 0;
			}else{
#ifdef DEBUG_ADVERTISE
				printf( "[32mchange agent to %s[m\n", 
			  	                 inet_ntoa(((struct mipAdvertisePkt *)buf)->agentInnerAddr)); 
#endif
				/* continue sending reply. */
			}
		}else{
#ifdef DEBUG_ADVERTISE
			printf( "In the area of same agent(%s).\n", inet_ntoa(currentAgent.sin_addr)); 
#endif
			advertiseValid = DEFAULT_LIVETIME; /* start the timer.*/
		}

#undef DEBUG_MN_LISTENSOCKET 
#ifdef DEBUG_MN_LISTENSOCKET
		for(i=0 ; i<BUFSIZE ; i++){
			printf( "%02X ", (unsigned char)buf[i]);
		}
		printf( "\nFrom_addr: %08X, %d bytes received.\n", ntohl(from.sin_addr.s_addr), n);
#endif
		agentOaddr = ((struct mipAdvertisePkt *)buf)->agentOuterAddr;
		agentIaddr = ((struct mipAdvertisePkt *)buf)->agentInnerAddr;
		isAtHome = (agentIaddr.s_addr == HA_addr.sin_addr.s_addr);
		/* printf( "agentIaddr: %s\n", inet_ntoa(agentIaddr));*/

		/* Modify routing table to send reply back to agent. */
		modifyRouteTBL(agentIaddr);
		/* Different sleeping delay may decrease the probility of collision. */
		srandom(localAddr.sin_addr.s_addr);
		usleep(20000+(random()/100000));
		/* printf( "mnd: after modifyRTBL: %s", 
		inet_ntoa(((struct mipAdvertisePkt *)buf)->agentInnerAddr)); */
		sendReplyToAgent(buf); 

	}else if(typeOfPacket(buf, &(from.sin_addr)) == REPLY_ACK){
#ifdef DEBUG_REPLY_ACK
		printf( "\tmnd receives REPLY_ACK.\n");
#endif
		agentIaddr = ((struct mipReplyAckPkt *)buf)->agentInnerAddr;
		currentAgent.sin_addr.s_addr = agentIaddr.s_addr;
		tempAgent.sin_addr.s_addr = ((struct mipReplyAckPkt *)buf)->agentOuterAddr.s_addr;
		advertiseValid = DEFAULT_LIVETIME; /* start the timer.*/
	}else{
		printf( "[X] Receive garbage packet(%d) from agent.\n",
				((struct mipPkt *)buf)->type);
	}

	return 0;
}

PacketType 
typeOfPacket(char *p, struct in_addr *fromAddr)
{
	struct mipPkt * p1;
	p1 =(struct mipPkt *)p; /* pending problem. */


	/* pending */
	if(p1->type == 9){               /* pending */
		return ADVERTISE;        /* Advertisement packet from agent. */
	/* }else if((p1->type == 13) && (p1->mnaddr.s_addr == fromAddr->s_addr)){ */
	}else if(p1->type == 13){
		return REPLY;            /* Reply packet from MN. */
	}else if(p1->type == 99){
		return REPLY_ACK;        /* Reply Confirm packet from agent. */
	}else if(p1->type == 1){
		return REG_REQ;         /* Registration from FA. */
	}else if(p1->type == 3){
		return REG_REPLY;        /* Reg reply from HA. */
	}else{
		return UNKNOWN;          /* not a mobileIP packet, but bind */  
	}                                /* to port 8118. */
}

int  
isAdvertisement(char *packet, struct sockaddr_in *fromAddr)
{
	struct mipPkt *p;
	p = (struct mipPkt *)packet;

	/* if((p->type == 9) && (p->agentInnerAddr.s_addr == fromAddr->sin_addr.s_addr)) */
	if(p->type == 9)
		return 1;
	else
		return 0;
}

/* After receiving some advertisements, MN choose one agent as temporary home 
 * and send reply to it. */
int
sendReplyToAgent(char *advPkt)
{
	struct sockaddr_in     addrChosenAgent;
	struct mipReplyPkt     replyPacket;
	struct mipAdvertisePkt *aPkt;
	socklen_t addr_len;

	aPkt = (struct mipAdvertisePkt *)advPkt;

	memset(&addrChosenAgent, 0, sizeof(addrChosenAgent));
	addrChosenAgent.sin_family      = AF_INET;
	addrChosenAgent.sin_port        = htons(PORT_LISTEN);
	/* printf( "\tAddr of chosen agent: %08X\n", aPkt->agentInnerAddr.s_addr); */
	/* printf( "\tAddr of chosen agent: %s\n", 
		               inet_ntoa(aPkt->agentInnerAddr)); */
	addrChosenAgent.sin_addr.s_addr = aPkt->agentInnerAddr.s_addr;
	/* p = (unsigned char *)&(addrChosenAgent.sin_addr.s_addr);
	q = (unsigned char *)&(localAddr.sin_addr.s_addr);
	p[0] = q[2]; p[1] = q[3]; */
/* printf( "\tAddr to send reply: %08X\n", addrChosenAgent.sin_addr.s_addr);  */

	replyPacket.type    = 13;
	replyPacket.mnaddr  = localAddr.sin_addr;
	replyPacket.haddr   = HA_addr.sin_addr;
	/* pending. */
	replyPacket.agentInnerAddr = aPkt->agentInnerAddr;
	/* replyPacket.preagent = currentAgent.sin_addr; */
	replyPacket.preagent = tempAgent.sin_addr;
	replyPacket.lifetime = 100;
#ifdef DEBUG_SEND_REPLY
	printf( "\tmnd: Start to send reply to agents(%08X).\n", 
			addrChosenAgent.sin_addr.s_addr);
#endif

	addr_len = sizeof(addrChosenAgent);
	if(Sendto(replySocket, &replyPacket, sizeof(replyPacket), 0,
			(struct sockaddr *)&addrChosenAgent, addr_len) < 0){
		printf( "[X] mnd: Failure in sendto() of reply.\n");
		exit(1);
	}else{
#ifdef DEBUG_REPLY_SYM
		printf( "R ");
#endif
	}
 
	return 0;
}

int  
modifyRouteTBL(struct in_addr agent_inner_addr)
{
	int  i;
	unsigned char *p, *q;
	int  argC_add = 5; /* add -net x.x.x.x -interface tunx. */
	int  argC_del = 3; /* delete -net x.x.x.x */
	char route_add1[5][25] ={"add","-net","","-interface",""};
	char route_add2[5][25] ={"add","-net","","-interface",""};
	char route_del1[3][25] ={"del","-net",""};
	char route_del2[3][25] ={"del","-net",""};
	char *route_a1[5], *route_a2[5], *route_d1[3], *route_d2[3];

	char delRouteCmd1[200];
	char delRouteCmd2[200];
	char addRouteCmd1[200];
	char addRouteCmd2[200];

#if 0
	/* printf( "mnd: Modifying the routing table for sending reply.\n"); */
	for(i=0 ; i< argC_del ; i++){
		route_d1[i] = &route_del1[i][0];
		route_d2[i] = &route_del2[i][0];
	}
	for(i=0 ; i< argC_add ; i++){
		route_a1[i] = &route_add1[i][0];
		route_a2[i] = &route_add2[i][0];
	}

	p = (unsigned char *)&(localAddr.sin_addr.s_addr);
	q = (unsigned char *)&(agent_inner_addr.s_addr);

	sprintf(route_del1[2], "%u.%u.%u.0/24", p[2], p[3], q[2]);
	sprintf(route_add1[2], "%u.%u.%u.0/24", p[2], p[3], q[2]);
#endif


	/* printf( "mnd: Modifying the routing table for sending reply.\n"); */
	for(i=0 ; i< argC_del ; i++){
		route_d1[i] = &route_del1[i][0];
		route_d2[i] = &route_del2[i][0];
	}
	for(i=0 ; i< argC_add ; i++){
		route_a1[i] = &route_add1[i][0];
		route_a2[i] = &route_add2[i][0];
	}

	p = (unsigned char *)&(localAddr.sin_addr.s_addr);
	q = (unsigned char *)&(agent_inner_addr.s_addr);

	sprintf(route_del1[2], "%u.%u.%u.0/24", p[2], p[3], q[2]);
	sprintf(route_add1[2], "%u.%u.%u.0/24", p[2], p[3], q[2]);
	sprintf(route_del2[2], "%u.%u.%u.0/24", q[2], q[3], p[2]);
	sprintf(route_add2[2], "%u.%u.%u.0/24", q[2], q[3], p[2]);
	/* printf( "mnd,localip: %d.%d.%d.%d, innerip: %d.%d.%d.%d\n",
			p[0], p[1], p[2], p[3], q[0], q[1], q[2], q[3]); */
	/* printf( "mnd,localip: %s, agent innerip: %s\n", 
			inet_ntoa(localAddr.sin_addr), 
			inet_ntoa(agent_inner_addr));  */
	/* printf( "mnd: route_add: %s, %s\n", route_add1[2], route_add2[2]); */

	/* printf( "mnd: tun for local: %s, for innerip: %s\n", 
			inet_ntoa(localAddr.sin_addr), inet_ntoa(agent_inner_addr));  */
	ip2tun(&(localAddr.sin_addr), route_add1[4]); 
	/*strncpy(route_add1[4], "tun9", 10);*/

	ip2tun(&(agent_inner_addr),   route_add2[4]); 
	/*if(agent_inner_addr.s_addr == inet_addr("1.0.4.1"))
		strncpy(route_add2[4], "tun6", 10);
	else
		strncpy(route_add2[4], "tun8", 10);*/

	/* printf( "mnd, tun for add: %s, %s\n", route_add1[4], route_add2[4]); */

	sprintf(delRouteCmd1, "%s%s",
	                     "route del -net ",
	                     route_add1[2]
	);
	sprintf(addRouteCmd1, "%s%s%s%s%s",
	                     "route add -net ",
	                     route_add1[2],
	                     " dev ",
	                     route_add1[4],
			     " 2>/dev/null"
	);
	sprintf(delRouteCmd2, "%s%s",
	                     "route del -net ",
	                     route_add2[2]
	);
	sprintf(addRouteCmd2, "%s%s%s%s%s",
	                     "route add -net ",
	                     route_add2[2],
	                     " dev ",
	                     route_add2[4],
			     " 2>/dev/null"
	);

//printf("[31m[mnd]addRouteCmd1: %s, addRouteCmd2: %s[m\n", addRouteCmd1, addRouteCmd2);
	system(delRouteCmd1);
	system(addRouteCmd1);
	system(delRouteCmd2);
	system(addRouteCmd2);

	return 0;
}

int      
ip2tun(struct in_addr *addr, char* tunName)
{
	struct ifaddrs *head, *buf;

	if(getifaddrs(&head) != 0){
		printf( "[X] error in getifaddrs().\n");
		freeifaddrs(head);
		return 1;
	}

	buf = head;
#define IFADDR ((struct sockaddr_in *)((*buf).ifa_addr))
	while(buf){
		if(IFADDR) {
			if((IFADDR->sin_family == AF_INET) 
		      	&& (addr->s_addr == (IFADDR->sin_addr.s_addr))){
				strcpy(tunName, (*buf).ifa_name);/* lack of check */
				freeifaddrs(head);
				return 0;
			}
		}
		buf = (*buf).ifa_next;
	}

	freeifaddrs(head);

	return 1;
}

int        
Sendto(int fd, void *buf, size_t n, int f, 
		const struct sockaddr *sin, socklen_t len)
{
	int m;

sendAgain:
	m = sendto(fd, (char *)buf, n, f, sin, len);
	if((m < 0) && (errno == ENOBUFS)){
		/* printf("\t errno = ENOBUFS. t: %d", globalt/1000); */
		usleep(1000);
		errno = 0;
		goto sendAgain;
	}else
		return m;
}
