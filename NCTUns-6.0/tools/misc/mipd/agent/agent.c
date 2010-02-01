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

/* #define DEBUG_MOBILEIP   */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>

#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>

#ifndef __FAVOR_BSD
#define __FAVOR_BSD 1
#include <netinet/tcp.h>
#include <netinet/udp.h>
#endif  /* __FAVOR_BSD */

#include <arpa/inet.h>

#include <ifaddrs.h>   /* Used by ip2tun(). */

#include "linklist.h"
#include "mnlinklist.h"
#include "vlinklist.h"
#include "blinklist.h"
#include "packetFmt.h" /* format of MobileIP control packet. */
#include "nctuns_mip.h"
#include "parseconf.h"
#include "route.h" 

#include <limits.h>
#include <linux/types.h>
#include <linux/netfilter_ipv4.h>
#include <nctuns_syscall.h>
#include <nctuns_divert.h>

#define BUFSIZE            1800 /* pending, how much needed? */
#define BUFSIZE_LISTEN      500 /* used for small-sized control packets */

#define PORT_LISTEN        8118
#define PORT_DIVERT        8119
#define PORT_IPINUDP_ON_FA 8121
#define PORT_RIGISTER      8122
#define PORT_ROTUN_SOCKET  8123
#define NCTUNS_SUBNET_CNT   20

#define REPLY_LIFTTIME       5
/* #define REPLY_ACK_LIFTTIME   5 */
#define ROUPDATE_RETRIES     3



static void sig_alarm(int);

void       initSockets();

int        handleListenSocket(int);
int        handleDivertSocket(int);
int        handleRoSocket(int);
int        handleIpInUdpSocketOnFA(int);

PacketType typeOfPacket(char *, struct in_addr *);

int        isChildMn(char *);
int        chgCOAofMN(char *, struct in_addr *, MnState);
int        queryCOA(struct in_addr *, struct in_addr *, MnState *); 

int        genOuterIPList(LLIST *, LLIST *, LLIST *);
int        addToVlist(struct in_addr *, struct in_addr *);
struct vlinkListEntry *  
           isInVlist(char *);
struct in_addr *        
           queryBindif(struct in_addr *); 
int        updateBlist(char *);

int        bindUpdate(struct in_addr *, struct in_addr *, struct in_addr *, int,
		                                                           int);
int        rebindUpdate(struct in_addr *, struct in_addr *, struct in_addr *, 
		                                         struct in_addr *, int);
int        bindAck(struct in_addr *, struct in_addr *);
int        bindWarn(struct in_addr *, struct in_addr *, struct in_addr *);

int        ackToMnd(struct in_addr *, struct in_addr *);

int        addBcastRoute(struct in_addr, struct sockaddr_in);
int        modifyRouteTBL_home(struct in_addr);
int        modifyRouteTBL_foreign(struct in_addr);

int        discardPacket(char *, int);
int        registerToHA(struct mipReplyPkt *);
int        regReplyToFA(struct mipRegPkt *);

int        shForward(char *, struct in_addr *, int);
int        specialTun(char *, int);

int        turnOnDivertOnHA(LLIST *, MNLLIST *);

/* get tun name through ip bind on it. */
int        ip2tun(struct in_addr *, char *);

int        genIpfwStr(char *, char *, char *);
unsigned short 
           in_cksum(unsigned short *,int);
int        Sendto(int, void *, size_t, int, const struct sockaddr *, socklen_t);

/* outerIP_list: list of wired network interfaces
   innerIP_list: list of wireless networks interfaces
   mn_list     : list of MNs belonging to this agent. 
   vlist       : list of MNs currently attaching to this agent. 
   bindlist    : */
LLIST       *allIP_list, *outerIP_list, *innerIP_list; 
MNLLIST     *mn_list; 
VLLIST      *vlist; 
BLLIST      *bindlist;
					      
int                     listenSocket, divertSocket, 
	                ipInUdpSocketOnFA, registerSocket, rawSocket,
			roSocket, bcastFd; 
int                     portDivert;

unsigned int            advertiseDuration = 500000;  /* microseconds. */
unsigned int            vlistLifetime     = 2000000; /* microseconds. */
unsigned int            blistLifetime     = 15;      /* seconds. */
//unsigned int            registerTimer = 0; /* seconds. */
//int                     pendingReg = 0;   /* unfinished registration exists? */

struct sockaddr_in      bcastAddr; 
struct in_addr          coAddr;

int                     roupdatet = 0, roupdate_cnt = 0;
unsigned int            globalt = 0, 
	                roupdatef = 0, 
	                roupdate_timerLifetime=1000000,
			ro_enable = 0; 
struct in_addr          roupdate_target, roupdate_mn, roupdate_coa, roupdate_haddr; /* for re-sending the non-ack bind update msg. */
unsigned int            roupdate_lifet;


int
main(int argc, char *argv[])
{
	int     rc;
	fd_set  readmask;


	allIP_list   = newList();
	outerIP_list = newList();
	innerIP_list = newList();
	mn_list      = newMNList();
	vlist        = newVList();
	bindlist     = newBList();

	memset(&bcastAddr, 0, sizeof(bcastAddr));
	parseConfig(argc, argv, &portDivert, &ro_enable, 
			&bcastAddr, &coAddr, mn_list, allIP_list, innerIP_list);

	genOuterIPList(allIP_list, innerIP_list, outerIP_list);

#ifdef DEBUG_PRINTLIST
	printMNList("MNList: ", mn_list); 
	printList("outerIPlist: ", outerIP_list);
	printList("InnerIPlist: ", innerIP_list);
#endif


	/* Initialize sockets. */
	initSockets();

	turnOnDivertOnHA(outerIP_list, mn_list);

	srand(coAddr.s_addr);
	usleep(rand()%10000);

        if(signal(SIGALRM, sig_alarm) == SIG_ERR){
		printf( "signal() error.\n");
		exit(1);
	}
        ualarm(1, 0);

	/* sleep(1);  */
	while(1){
		FD_ZERO(&readmask);
		FD_SET(listenSocket, &readmask);
		if(ro_enable)
			FD_SET(roSocket, &readmask);
		FD_SET(ipInUdpSocketOnFA, &readmask);
		FD_SET(divertSocket, &readmask);

		rc = select(divertSocket+1, 
				(fd_set *)&readmask, NULL, NULL, NULL);
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
		if(FD_ISSET(ipInUdpSocketOnFA, &readmask)){
			handleIpInUdpSocketOnFA(ipInUdpSocketOnFA);
		}

		if(ro_enable)
			if(FD_ISSET(roSocket, &readmask))
				handleRoSocket(roSocket);

		if(FD_ISSET(divertSocket, &readmask)){
			handleDivertSocket(divertSocket);
		}
	}

	return 0;
}

static void 
sig_alarm(int signo)
{
	socklen_t               bcast_len;
	struct sockaddr_in      bcast_addr;
	struct mipAdvertisePkt  bcastBuf;
	struct linkListEntry    *innerip;
	struct vlinkListEntry   *ventry, *pre1 = NULL;
	struct blinkListEntry   *bentry, *pre2 = NULL;
	unsigned char           *p, *q;


	globalt += advertiseDuration;

	if(ro_enable){
		if(roupdatef == 1){
			if(roupdate_cnt < ROUPDATE_RETRIES){
				if((roupdatet - advertiseDuration) <= 0){
#ifdef DEBUG_REBINDUPDATE
printf( "rebindUpdate().\n");
#endif
					roupdate_cnt++;
					rebindUpdate(&roupdate_target, &roupdate_mn, &roupdate_coa, &roupdate_haddr, roupdate_lifet);
					roupdatet = roupdate_timerLifetime;
				}else{
					roupdatet -= advertiseDuration;
#ifdef DEBUG_REBINDUPDATE
printf( "roupdatet: %d\n", roupdatet/1000);
#endif
				}
			}else {
				roupdatef = 0;
			}
		}
	}
	/* If there are pending registration(), continue the reg timer.
	 * Once timeout takes place, retransmit the reg control packet. */
	/* if(pendingReg != 0){
		if((--registerTimer) == 0){
		}
	} */

	/* Additionally create a variable to avoid modifying global var.*/
	memset(&bcast_addr, 0, sizeof(bcast_addr));
        bcast_addr.sin_family      = AF_INET;
        bcast_addr.sin_port        = htons(PORT_LISTEN);
	/* bcast_addr.sin_addr.s_addr = bcastAddr.sin_addr.s_addr; */
	bcast_addr.sin_addr.s_addr = inet_addr("1.0.255.255");

	/* broadcast through each inner interface. */
	rewindList(innerIP_list);
	while((innerip = getCurrentEntry(innerIP_list)) != NULL){
		/* Use agent inner ip and bcast addr to form s.s.d.d IP. */
		p = (unsigned char *)&(bcast_addr.sin_addr.s_addr);
		q = (unsigned char *)&(innerip->addr.s_addr);
		p[0] = q[2] ; p[1] = q[3]; p[2] = q[2] ; p[3] = 0xff;
		//p[0] = q[0] ; p[1] = q[1]; p[2] = q[2] ; p[3] = 0xff;
//printf( "bcast_addr: %x\n", bcast_addr.sin_addr.s_addr);

		/* Fill data payload in broadcast packet. */
		bcastBuf.type          = 9;
		/* bcastBuf.agentOuterAddr.s_addr = outerIP_list->head->addr.s_addr;  */
		bcastBuf.agentOuterAddr.s_addr = coAddr.s_addr; 
		bcastBuf.agentInnerAddr.s_addr = innerip->addr.s_addr;
		bcastBuf.lifetime      = 10;

		bcast_len = sizeof(bcast_addr);

		/* printf( "innerip: %s, bcast_addr: %s\n", 
			                      inet_ntoa(innerip->addr), 
			                      inet_ntoa(bcast_addr.sin_addr));*/
		
		usleep(10000);

		/* printf( "actually bcast_addr: %s\n", 
			                      inet_ntoa(bcast_addr.sin_addr));*/
		if(Sendto(bcastFd, &bcastBuf, sizeof(bcastBuf), 0, 
				(struct sockaddr *)&bcast_addr, bcast_len) < 0){
			printf( "[X] Failure in sendto() of broadcast from agent.\n");
			exit(1);
		}/*else
			printf( "[O] send broadcast successfully.\n"); */
#ifdef DEBUG_REBINDUPDATE
		printf( "A ");
#endif
	}

	/* update lifetime of each vlist entry, if expired, delete it. */
	rewindVList(vlist);
	while((ventry = getCurrentVEntry(vlist)) != NULL){
		ventry->lifetime -= advertiseDuration;
		if(ventry->lifetime <= 0){
			delVEntry(vlist, ventry, pre1);
			/* printf( "\n\t delete (%08X) from vlist of %08X\n", ventry->mnaddr.s_addr, outerIP_list->head->addr.s_addr); */
#ifdef DEBUG_VLIST_DEL
			printf( "\n\t %08X delete (%08X, %08X, %08X, %d) from vlist\n", coAddr.s_addr, ventry->mnaddr.s_addr, ventry->bindif.s_addr,
					ventry->haddr.s_addr, ventry->lifetime);
#endif
		}
		pre1 = ventry;
	}

	/* update lifetime of each bindlist entry, if expired, delete it. */
	pre2 = NULL;
	rewindBList(bindlist);
	while((bentry = getCurrentBEntry(bindlist)) != NULL){
		bentry->lifetime -= advertiseDuration;
		if(bentry->lifetime <= 0){
			delBEntry(bindlist, bentry, pre2);
			/* printf( "\n\t delete (%08X) from bindlist of %08X\n", bentry->mnaddr.s_addr, outerIP_list->head->addr.s_addr); */
#ifdef DEBUG_BLIST_DEL
			printf( "\n\t %08X delete (%08X, %08X, %08X, %ld) from bindlist of \n", coAddr.s_addr, bentry->mnaddr.s_addr, 
				      bentry->coa.s_addr, 
				      bentry->haddr.s_addr,
				      bentry->lifetime);
#endif
		}
		pre2 = bentry;
	}

	signal(SIGALRM, sig_alarm);
	ualarm(advertiseDuration, 0); /* restart the timer */

}

void 
initSockets()
{
	const int on = 1;
	struct sockaddr_in bindAddr;


	/* Initialize broadcast socket. */
	bcastFd = socket(AF_INET, SOCK_DGRAM, 0);
	if(bcastFd < 0){
		printf( "[X] agent: Failure in creating bcast socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Creating bcast socket successfully.\n");
#endif

	if(setsockopt(bcastFd, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0){
		printf( "[X] agent: Failure in setsockopt() of bcast.\n");
		exit(1);
	}

	/* Initialize listen socket. */
	listenSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(listenSocket < 0){
		printf( "[X] agent: Failure in creating listen socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Creating listen socket successfully.\n");
#endif

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family      = AF_INET;
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddr.sin_port        = htons(PORT_LISTEN);

	if(bind(listenSocket, (struct sockaddr *)&bindAddr, sizeof(bindAddr)) < 0)
	{
		printf( "[X] agent: Failure in bind() of listen socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Binding listen socket successfully.\n");
#endif


	/* Initialize registration socket. */
	registerSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(registerSocket < 0){
		printf( "[X] agent: Failure in creating registration socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Creating register socket successfully.\n");
#endif
    /***********************************************************************
     * add by cklai : When the agent needs to send a registration packet   *
     *                to HA by registerSocket, use the coAddr to prevent   *
     *                this packet losing at wireless side.                 *
     ***********************************************************************/
    memset(&bindAddr, 0, sizeof(bindAddr));
    bindAddr.sin_family      = AF_INET;
    bindAddr.sin_addr.s_addr = coAddr.s_addr;
    bindAddr.sin_port        = htons(PORT_RIGISTER);

    if(bind(registerSocket, (struct sockaddr *)&bindAddr, sizeof(bindAddr)) < 0)
    {
        printf( "[X] agent: Failure in bind() of register socket.\n");
        exit(1);
    }
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Binding register socket successfully.\n");
#endif
    /****************************************************************/

	/* Initialize raw socket. */
	rawSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(rawSocket < 0){
		printf( "[X] agent: Failure in creating raw socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Creating raw socket successfully.\n");
#endif

	if(setsockopt(rawSocket, IPPROTO_IP, IP_HDRINCL, (char *)&on, 
				                          sizeof(on)) < 0){
		printf( "[X] agent: Failure in setsockopt of raw socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: setsockopt of raw socket successfully.\n");
#endif

	/* Initialize route optimization socket. */
	roSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(roSocket == -1){
		printf( "[X] agent: Failure in creating ro socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else		
		printf( "[O] agent: Creating ro socket successfully.\n");
#endif
	/*if(setsockopt(roSocket, IPPROTO_IP, IP_HDRINCL, 
				(char *)&on, sizeof(on)) < 0){
		printf( "Failure in setsockopt() of ro socket.\n");
		exit(1);
	}*/
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family      = AF_INET;
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddr.sin_port        = htons(PORT_ROTUN_SOCKET);

	if(bind(roSocket, (struct sockaddr *)&bindAddr, 
				              sizeof(bindAddr)) < 0)
	{
		printf( "[X] agent: Failure in bind() of ro socket.\n");
		exit(1);
	}

	/* Initialize ipInUdp socket on FA. */
	ipInUdpSocketOnFA = socket(AF_INET, SOCK_DGRAM, 0);
	if(ipInUdpSocketOnFA == -1){
		printf( "[X] agent: Failure in creating ipInUdp socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else		
		printf( "[O] agent: Creating ipInUdp socket successfully.\n");
#endif

	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family      = AF_INET;
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddr.sin_port        = htons(PORT_IPINUDP_ON_FA);

	if(bind(ipInUdpSocketOnFA, (struct sockaddr *)&bindAddr, 
				              sizeof(bindAddr)) < 0)
	{
		printf( "[X] agent: Failure in bind() of ipInUdp socket on FA.\n");
		exit(1);
	}

	/* Initialize divert socket. */
	divertSocket = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if(divertSocket == -1){
		printf( "[X] agent: Failure in creating divert socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Creating divert socket successfully.\n");
#endif

	if(setsockopt(divertSocket, IPPROTO_IP, IP_HDRINCL, 
			(char *)&on, sizeof(on)) < 0){
		printf( "Failure in setsockopt() of divert socket.\n");
		exit(1);
	}
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family      = AF_INET;
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddr.sin_port        = htons(portDivert);

	if(bind(divertSocket, (struct sockaddr *)&bindAddr, 
			                         sizeof(bindAddr)) < 0)
	{
		printf( "[X] agent: Failure in bind() divert socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Binding divert socket successfully.\n");
#endif


	return;

}

int 
handleListenSocket(int listenSock)
{
	int                 n;
	struct sockaddr_in from;
	socklen_t          from_len;
	char      buf[BUFSIZE_LISTEN];
	struct vlinkListEntry *e;


	from_len = sizeof(from);
	
/* #define DEBUG_LISTENSOCKET  */
	n = recvfrom(listenSock, buf, BUFSIZE, 0, (struct sockaddr *)&from, 
			                                        &from_len);
		
	if(n < 0){
		printf( "[X] agent: Listen Socket, Failure in recvfrom.\n");
		exit(1);
	}

#ifdef DEBUG_LISTENSOCKET
	for(i=0 ; i<n ; i++){
		printf( "%02X ", (unsigned char)buf[i]);
	}
	printf( "\tFrom_addr: %08X, %d bytes received.\n", 
			       ntohl(from.sin_addr.s_addr), n);
#endif

	if(typeOfPacket(buf, &(from.sin_addr)) == REPLY){
		/* printf( "\tagent(%08X) receives reply :%u\n", 
			outerIP_list->head->addr.s_addr, globalt/1000); */
#ifdef DEBUG_RECEIVE_REPLY
		printf( "\t[31magent(%08X) receives reply :%u[m\n", 
				coAddr.s_addr, globalt/1000);
#endif

		if(isChildMn(buf)){      /* MN is in the area of HA,   */

			/* if the MN is already in the vlist, but the reply*/
			if(addToVlist(&(((struct mipReplyPkt *)buf)->mnaddr), 
			   &(((struct mipReplyPkt *)buf)->agentInnerAddr))){
				if(chgCOAofMN(buf, &coAddr, ATHOME) != 0)
					printf( "[X] chgCOAofMN() on HA fails.\n");
				if(ro_enable && 
					(((struct mipReplyPkt *)buf)->preagent.s_addr != coAddr.s_addr)){
					/* for smooth handoff. */
					/* printf( " %08X smooth handoff bindUpdate to %8X.\n", 
					outerIP_list->head->addr.s_addr,
					((struct mipReplyPkt *)buf)->preagent.s_addr); */
#ifdef DEBUG_MN_ATHOME
					printf( " %08X smooth handoff bindUpdate to %8X.\n", 
					coAddr.s_addr,
					((struct mipReplyPkt *)buf)->preagent.s_addr);
#endif
					bindUpdate(&(((struct mipReplyPkt *)buf)->preagent), &(((struct mipReplyPkt *)buf)->mnaddr), &(((struct mipReplyPkt *)buf)->haddr), 0, 1);
				}


				/* usleep(10000); */
				ackToMnd(&(from.sin_addr), &(((struct mipReplyPkt *)buf)->agentInnerAddr));
				modifyRouteTBL_home(((struct mipReplyPkt *)buf)->mnaddr); 
#ifdef DEBUG_MN_ATHOME
				printf( "\n\tMN is ATHOME.\n");
#endif
			}else{
#ifdef DEBUG_MN_ATHOME
				printf( "%08X skip reply from %08X.\n",
						coAddr.s_addr,
						((struct mipReplyPkt *)buf)->mnaddr.s_addr);
#endif
			}

		}else{     /* MN is in the area of FA. */
			if((e = isInVlist(buf))){
#ifdef DEBUG_MN_AWAY
				printf( "\tAlready in FA visit list.\n");
#endif
				e->lifetime = vlistLifetime; /* refresh the lifetime. */
			}else{
				//registerTimer = 3;
				if(ro_enable &&
					(((struct mipReplyPkt *)buf)->preagent.s_addr != coAddr.s_addr)){
					/* pending: for smooth handoff. */
#ifdef DEBUG_MN_AWAY
					printf( "smooth handoff bindUpdate to %8X.\n", 
					((struct mipReplyPkt *)buf)->preagent.s_addr);
#endif
					bindUpdate(&(((struct mipReplyPkt *)buf)->preagent), &(((struct mipReplyPkt *)buf)->mnaddr), &(((struct mipReplyPkt *)buf)->haddr), 1, 1);
				}
#ifdef DEBUG_MN_AWAY
				printf( "\tFA(%08X) starts to register to HA. :%u\n", coAddr.s_addr, globalt/1000);
#endif
				registerToHA((struct mipReplyPkt *)buf);
				/* ackToMnd and addVList is postponed until
				 * reg_reply is received. */
			}
		}
	}else if(typeOfPacket(buf, &(from.sin_addr)) == REG_REQ){
#ifdef DEBUG_REG_REQ
		printf( "\tHA receives register from FA. :%u\n", globalt/1000);
#endif
		if(chgCOAofMN(buf, &(((struct mipRegPkt *)buf)->coaddr), 
					FOREIGN) != 0)
				printf( "[X] chgCOAofMN() on FA fails.\n");
		regReplyToFA((struct mipRegPkt *)buf);
	}else if(typeOfPacket(buf, &(from.sin_addr)) == REG_REPLY){
#ifdef DEBUG_REG_REPLY
		printf( "\tFA(%08X) receives register reply from HA. :%u\n", coAddr.s_addr, globalt/1000);
#endif
		addToVlist(&(((struct mipRegReplyPkt *)buf)->mnaddr), 
			   &(((struct mipRegReplyPkt *)buf)->agentInnerAddr)); 
#ifdef DEBUG_REG_REPLY
		printf( "\t\tmnaddr obtained from reg_reply: %08X.\n", 
			    ((struct mipRegReplyPkt *)buf) ->mnaddr.s_addr); 
#endif
		modifyRouteTBL_foreign(((struct mipRegReplyPkt *)buf)->mnaddr);
		/* usleep(10000); */
		ackToMnd(&(((struct mipRegReplyPkt *)buf)->mnaddr), 
				&(((struct mipRegReplyPkt *)buf)->agentInnerAddr));
#ifdef DEBUG_REG_REPLY
		printf( "\n\tMN is FOREIGN.\n");
#endif
	/* smooth handoff, bind update msg from new agent. */
	}else if(ro_enable && (typeOfPacket(buf, &(from.sin_addr)) == RO_UPDATE)){
		/* printf( "\n\t %08X receives RO_UPDATE from %08X !!!\n",
				              outerIP_list->head->addr.s_addr, 
				              from.sin_addr.s_addr); */
		if(!searchMNEntry(mn_list, &(((struct mipROupdatePkt *)buf)->mnaddr))){
/* #define DEBUG_RO_UPDATE */
#ifdef DEBUG_RO_UPDATE
			printf( "\n\t %08X receives RO_UPDATE from %08X, lifetime: %d sec !!!\n",
				              coAddr.s_addr, 
				              from.sin_addr.s_addr,
					      ((struct mipROupdatePkt *)buf)->lifetime);
#endif
			/* updateBlist(buf, &(from.sin_addr)); */
			updateBlist(buf);

		}else{
		/* The HA should maintain the MN info including the current
		 * agent that MN attachs, so the smooth handoff bindUpdate 
		 * could be discarded. */
#undef DEBUG_RO_UPDATE
#ifdef DEBUG_RO_UPDATE
			printf( "HA(%08X) receives bindUpdate from another agent(%08X), the bindUpdate could be discarded.\n", coAddr.s_addr, from.sin_addr.s_addr);
#endif
		}
		/* The bindUpdate sent between agents should be acked. */
		bindAck(&(from.sin_addr), &(((struct mipROupdatePkt *)buf)->mnaddr));

	}else if(ro_enable && (typeOfPacket(buf, &(from.sin_addr)) == RO_WARN)){
		bindUpdate(&(((struct mipROwarnPkt*)buf)->target),
				&(((struct mipROwarnPkt*)buf)->mnaddr), NULL, 0, 0);
/* #define DEBUG_RO_WARN  */
#ifdef DEBUG_RO_WARN
			printf( "\n\t %08X receives RO_WARN from %08X !!!\n",
				              coAddr.s_addr, 
				              from.sin_addr.s_addr);
#endif

	}else if(ro_enable && (typeOfPacket(buf, &(from.sin_addr)) == RO_ACK)){
		/* printf( "\t %08X receives RO_ACK from %08X.\n", 
				              outerIP_list->head->addr.s_addr, 
				              from.sin_addr.s_addr); */
/* #define RO_ACK */
#ifdef RO_ACK
		printf( "\t [34m%08X receives RO_ACK from %08X.[m\n", 
				              coAddr.s_addr, 
				              from.sin_addr.s_addr);
#endif
		roupdatef = 0;

	}else if(typeOfPacket(buf, &(from.sin_addr)) == UNKNOWN){
		discardPacket(buf, n);
	}

	return 0;
}

int 
handleDivertSocket(int divertSock)
{
	int                n, m;
	struct sockaddr_in sin, from, coa;
	struct in_addr     *dstMn, *bindif;
	MnState            state;
	socklen_t          from_len, coa_len;
	char      buf[BUFSIZE];
	struct udpiphdr    *uihdr;
	struct ip          *ip;
	unsigned char      *p, *q;


	from_len = sizeof(from);

	n = recvfrom(divertSock, buf, BUFSIZE, 0, (struct sockaddr *)&from, 
			                                       &from_len);
		
	if(n < 0){
		printf( "\n[X] agent: Divert socket,Failure in recvfrom.\n");
		exit(1);
/* #define DEBUG_DIVERTSOCKET */
#ifdef DEBUG_DIVERTSOCKET
	}else if(n >= 1450){
		printf( "\n\t%d bytes received.\n", n);
#endif
	}
#if 0
	ip = (struct ip*)buf;
	sin.sin_addr = ip->ip_dst;

		m = Sendto(rawSocket, (char *)buf, n, 0x0, 
				     (struct sockaddr *)&sin, sizeof(sin));
		if(m != n){
			printf( "[X] Failure in sendto() of DIVERT socket to MN, m= %d\n", m);
		}else{
			printf( "\t%d bytes sent.\n", n);
		}
#endif

/* #undef DEBUG_DIVERTSOCKET  */
/* #define DEBUG_DIVERTSOCKET */
#ifdef DEBUG_DIVERTSOCKET
	printf("diverSocket: recvfrom()\n");
	for(i=0 ; i<48 ; i++){
		if(i%2 == 0) printf( " ");
		printf( "%02X", (unsigned char)buf[i]);
		if((i+1)%16 == 0) printf( "\n");
	}
	printf( "\tfrom_addr: %08X \n", ntohl(from.sin_addr.s_addr));
#endif

	memset(&coa, 0, sizeof(coa));
	dstMn = dstFrmPkt(buf);
	dstMn = ssdd2dst(dstMn);
	if(queryCOA(dstMn, &(coa.sin_addr), &state) != 0)
		printf( "[X] queryCOA() fails, MN(%x) is away.\n", dstMn->s_addr);

	if(state == AWAY)
		return discardPacket(buf, n);
	else if(state == ATHOME){
		uihdr = (struct udpiphdr *)buf;
		ip    = (struct ip *)buf;

#ifdef DEBUG_RAWSOCKET_ON_HA_SYM
		printf( ". ");
#endif
		/* Pass the packet to MN "directly" through raw socket.  */
		memset((char *)&sin, 0, sizeof(sin));
		sin.sin_addr = ip->ip_dst; 
		sin.sin_family        = AF_INET;
		sin.sin_port          = 0; /* useless!? */
		/* Send diverted packet to MN using s.s.d.d format */
		/* dstMn  = ssdd2dst(&(ip->ip_dst)); */
		bindif = queryBindif(dstMn);
		if(bindif == NULL){
			/* printf( "[X] divert: bindif is null.\n");
			printList("vlist: ", vlist); */
			free(dstMn);
			/* pending, just drop? */
			return 0;
			/* exit(1); */
		}
		free(dstMn);

		/* p = (char *)&(agentInnerAddr.sin_addr.s_addr); */
		p = (unsigned char *)&(bindif->s_addr);
		q = (unsigned char *)&(sin.sin_addr.s_addr);
		q[0] = p[2]; q[1] = p[3];
		/* printf( "\tHA send diverted packet to MN: %08X(s.s.d.d format)\n", sin.sin_addr.s_addr); */
		ip->ip_dst = sin.sin_addr; 
		/*uihdr->ui_sum = 0;
		if(uihdr->ui_sum = in_cksum((u_short *)uihdr, n) == 0)
			uihdr->ui_sum = 0xffff;*/
		ip->ip_sum = (unsigned short)in_cksum((unsigned short *)ip, 
			                               	sizeof(struct ip));
/* #define DEBUG_RAWSOCKET_ON_HA */
#ifdef DEBUG_RAWSOCKET_ON_HA
		printf( "\tdst in the header of raw packet: %08X\n", 
			                              ip->ip_dst.s_addr);
		printf( "Raw packet after modified.\n");
		for(i=0 ; i<48 ; i++){
			if(i%2 == 0) printf( " ");
			printf( "%02X", (unsigned char)buf[i]);
			if((i+1)%16 == 0) printf( "\n");
		}
		printf( "\n");
#endif

		m = Sendto(rawSocket, (char *)buf, n, 0x0, 
				     (struct sockaddr *)&sin, sizeof(sin));
		if(m != n){
			printf( "[X] Failure in sendto() of DIVERT socket to MN, m= %d\n", m);
#ifdef DEBUG_RAWSOCKET_ON_HA
		}else{
			printf( "\t%d bytes sent.\n", n);
#endif
		}

		return 0;

	}else{ /* State == FOREIGN */


		/* bindUpdate(srcFrmPkt(buf), dstFrmPkt(buf)); */
		if(ro_enable)
			bindUpdate(srcFrmPkt(buf), dstMn, NULL, 0, 0);
		free(dstMn);
		/* uihdr = (struct udpiphdr *)buf; */
		ip   = (struct ip*)buf;

		/* Strange!!! The ip_len captured by divert socket is host order. */
		ip->ip_len = htons(ip->ip_len);
		ip->ip_off = htons(ip->ip_off);
	
		/* Data captured by divert socket is consist of header and 
		 * datapayload. It will be wholely sent to FA through UDP. */
		coa_len = sizeof(coa);
		coa.sin_family = AF_INET;
		coa.sin_port   = htons(PORT_IPINUDP_ON_FA);

		m = Sendto(listenSocket, (char *)buf, n, 0x0, 
				(struct sockaddr *)&coa, coa_len);
		if(m != n){
			printf( "[X] Failure in sending diverted data to FA.\n");
/* #define DEBUG_MTU */
#ifdef DEBUG_MTU
			for(i=0 ; i<48 ; i++){
				if(i%2 == 0) printf( " ");
				printf( "%02X", (unsigned char)buf[i]);
				if((i+1)%16 == 0) printf( "\n");
			}
			printf("%d bytes fails to be sent, errno: %d\n", n, errno);
#endif
		}

		return 0;
	}
}

int        
handleRoSocket(int roSock)
{
	int                n, m;
	struct sockaddr_in from, sin;
	struct in_addr     *dstMn, *bindif;
	struct blinkListEntry *e;
	socklen_t          from_len;
	char      buf[BUFSIZE];
	struct udpiphdr    *uihdr;
	struct ip          *ip;
	char               *p, *q; /* used to translate IP to ssdd format. */


	from_len = sizeof(from);

	n = recvfrom(roSock, buf, BUFSIZE, 0, 
			          (struct sockaddr *)&from, &from_len);

	if(n < 0){
		printf( "\n[X] ro socket: Failure in recvfrom.\n");
		exit(1);
	}else{
/* #define DEBUG_ROSOCKET */
#ifdef DEBUG_ROSOCKET
		printf( "\t%d bytes received.\n", n);
#endif
	}

#ifdef DEBUG_ROSOCKET
	for(i=0 ; i<n ; i++){
		if(i%2 == 0) printf( " ");
		printf( "%02X", (unsigned char)buf[i]);
		if((i+1)%16 == 0) printf( "\n");
	}
	printf( "\tfrom_addr: %08X \n", ntohl(from.sin_addr.s_addr));
#endif
	uihdr = (struct udpiphdr *)buf;
	ip    = (struct ip *)buf;

	/* Pass the ro tunneled packet to MN through raw socket.  */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_addr   = ip->ip_dst; 
	sin.sin_family = AF_INET;
	sin.sin_port   = htons(atol("3000")); /* useless!? */
	/* Send tunneled packet to MN using s.s.d.d format */
	dstMn  = ssdd2dst(&(ip->ip_dst));
	bindif = queryBindif(dstMn);

	/* if dst of the tunneled pkt is not in the visit list. */
	if(bindif == NULL){
		if((e = searchBEntry(bindlist, dstMn)) != NULL){
/* #define DEBUG_RO_SMOOTH_HANDOFF */
#ifdef DEBUG_RO_SMOOTH_HANDOFF
			printf( "\t%08X sends bindWarn to %08X.", coAddr.s_addr, e->haddr.s_addr);
#endif
			bindWarn(&(e->haddr), dstMn, &(ip->ip_src));
			shForward(buf, &(e->coa), n); 
		}else{
			/* If the binding list has no entry for the RO tunnel pkt, we check the mn list. Suppose that entry exists in the mn list, the bindWarn pkt needn't to be sent. This HA directly sends bindUpdate to the source end of the RO tunnel. */
			if(searchMNEntry(mn_list, dstMn) != NULL){
				/* bindUpdate(&(ip->ip_src), dstMn, &(e->haddr), 0, 0); */
				bindUpdate(&(ip->ip_src), dstMn, NULL, 0, 0);
			}else{
#ifdef DEBUG_RO_SPECIALTUN_SYM
				printf( "S ");
#endif
			}
			/* specialTun(buf, n); */
		}
	}else{
		p = (char *)&(bindif->s_addr);
		q = (char *)&(sin.sin_addr.s_addr);
		q[0] = p[2]; q[1] = p[3];
		/* printf( "\tagent send ro tunneled packet to MN: %08X(s.s.d.d format)\n", sin.sin_addr.s_addr); */
		ip->ip_dst = sin.sin_addr; 
		/*uihdr->ui_sum = 0;
		if(uihdr->ui_sum = in_cksum((u_short *)uihdr, n) == 0)
			uihdr->ui_sum = 0xffff;*/
		ip->ip_sum = (unsigned short)in_cksum((unsigned short *)ip, 
			                               	sizeof(struct ip));
/* #define DEBUG_RO_RAWSOCKET */
#ifdef DEBUG_RO_RAWSOCKET
		printf( "\tdst in the header of raw packet: %08X\n", 
			                              ip->ip_dst.s_addr);
		printf( "Raw packet after modified.\n");
		for(i=0 ; i<n ; i++){
			if(i%2 == 0) printf( " ");
			printf( "%02X", (unsigned char)buf[i]);
			if((i+1)%16 == 0) printf( "\n");
		}
		printf( "\n");
#endif

#ifdef DEBUG_RO_SYM
		printf( "RO ");
#endif
		m = Sendto(rawSocket, (char *)buf, n, 0x0, (struct sockaddr *)&sin, 
			                                       sizeof(sin));
		if(m != n){
			printf( "[X] Failure in sendto() of ro raw socket, m= %d\n", m);
		}else{
#ifdef DEBUG_RO_RAWSOCKET
			printf( "\t%d bytes sent.\n", n);
#endif
		}
	}
	free(dstMn);

	return 0;
}

int        
handleIpInUdpSocketOnFA(int ipInUdpSockOnFA)
{
	int                n, m;
	struct sockaddr_in from, sin;
	struct in_addr     *dstMn, *bindif;
	socklen_t          from_len;
	unsigned char      buf[BUFSIZE];
	struct udpiphdr    *uihdr;
	struct ip          *ip;
	char               *p, *q; /* used to translate IP to ssdd format. */


	from_len = sizeof(from);

	n = recvfrom(ipInUdpSockOnFA, buf, BUFSIZE, 0, 
			          (struct sockaddr *)&from, &from_len);

	if(n < 0){
		printf( "\n[X] IpInUDP socket on FA: Failure in recvfrom.\n");
		exit(1);
	}else{
/* #define DEBUG_IPINUDPSOCKET */
#ifdef DEBUG_IPINUDPSOCKET
		printf( "\t%d bytes received.\n", n);
#endif
	}

#ifdef DEBUG_IPINUDPSOCKET
printf("IPinIP, recvfrom()\n");
	for(i=0 ; i<48 ; i++){
		if(i%2 == 0) printf( " ");
		printf( "%02X", (unsigned char)buf[i]);
		if((i+1)%16 == 0) printf( "\n");
	}
	printf( "\tfrom_addr: %08X \n", ntohl(from.sin_addr.s_addr));
#endif
	uihdr = (struct udpiphdr *)buf;
	ip    = (struct ip *)buf;

	/* Pass the packet to MN through raw socket.  */
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_addr = ip->ip_dst; 
	sin.sin_family        = AF_INET;
	sin.sin_port          = 0; /* useless!? */

	/* Send diverted packet to MN using s.s.d.d format */
	dstMn  = ssdd2dst(&(ip->ip_dst));
	bindif = queryBindif(dstMn);
	if(bindif == NULL){
		/* printf( "[X] IPINUDP(%08X): bindif is null.\n",
				outerIP_list->head->addr.s_addr); */
		free(dstMn);
		return 1;
		/* exit(1); */
	}
	free(dstMn);
	p = (char *)&(bindif->s_addr);
	q = (char *)&(sin.sin_addr.s_addr);
	q[0] = p[2]; q[1] = p[3];
	/* printf( "\tFA send diverted packet to MN: %08X(s.s.d.d format)\n", sin.sin_addr.s_addr); */
	ip->ip_dst = sin.sin_addr; 
	ip->ip_off = htons(ip->ip_off);
	/*uihdr->ui_sum = 0;
	if(uihdr->ui_sum = in_cksum((u_short *)uihdr, n) == 0)
		uihdr->ui_sum = 0xffff;*/
	ip->ip_sum = (unsigned short)in_cksum((unsigned short *)ip, 
			                               	sizeof(struct ip));
/* #define DEBUG_RAWSOCKET_ON_FA*/
#ifdef DEBUG_RAWSOCKET_ON_FA
	printf( "\tdst in the header of FA raw packet:%08X\n", 
			                              ip->ip_dst.s_addr);
	printf( "Raw packet after modified on FA.\n");
	for(i=0 ; i<48 ; i++){
		if(i%2 == 0) printf( " ");
		printf( "%02X", (unsigned char)buf[i]);
		if((i+1)%16 == 0) printf( "\n");
	}
	printf( "\n");
#endif

#ifdef DEBUG_RAWSOCKET_ON_FA_SYM
	printf( "- ");
#endif
	m = Sendto(rawSocket, (char *)buf, n, 0x0, (struct sockaddr *)&sin, 
			                                       sizeof(sin));
	if(m != n){
		printf( "[X] Failure in sendto() of IPINUDP socket, m= %d\n", m);
	}else{
#ifdef DEBUG_RAWSOCKET_ON_FA
		printf( "\t%d bytes sent.\n", n);
#endif
	}

	return 0;
}

PacketType 
typeOfPacket(char *p, struct in_addr *fromAddr)
{
	struct mipPkt * p1;
	p1 =(struct mipPkt *)p; /* pending problem. */


	/* printf( "type of received pkt: %d\n", p1->type); */

	/* pending */
	if(p1->type == 9){
		return ADVERTISE;        /* Advertisement packet from agent. */
	/* }else if((p1->type == 13) && (p1->mnaddr.s_addr == fromAddr->s_addr)){ */
	}else if(p1->type == 13){
		return REPLY;            /* Reply packet from MN. */
	/* }else if((p1->type == 99) && (p1->mnaddr.s_addr == fromAddr->s_addr)){ */
	}else if(p1->type == 99){
		return REPLY_ACK;        /* Reply Confirm packet from agent. */
	}else if(p1->type == 1){
		return REG_REQ;         /* Registration from FA. */
	}else if(p1->type == 3){
		return REG_REPLY;        /* Reg reply from HA. */
	}else if(p1->type == 18){
		return RO_UPDATE;        
	}else if(p1->type == 16){
		return RO_WARN;        
	}else if(p1->type == 19){
		return RO_ACK;        
	}else{
		return UNKNOWN;          /* not a mobileIP packet, but bind */  
	}                                /* to port 8118. */
}

int 
isChildMn(char *packet)
{
	struct mipReplyPkt *p;


	p = (struct mipReplyPkt *)packet;

	if(searchMNEntry(mn_list, &(p->mnaddr)) != NULL){
/* #define DEBUG_ISCHILDMND */
#ifdef DEBUG_ISCHILDMND
		printf( "The source of packet is child of this agent.\n");
#endif
		return 1;
	}else{
#ifdef DEBUG_ISCHILDMND
		printf( "The source of packet is not child of this agent.\n");
#endif
		return 0;
	}

}

int        
chgCOAofMN(char *packet, struct in_addr *mn_coa, MnState mn_state)
{
	struct mipRegPkt  *p;
	struct mnlinkListEntry    *cur;


	p = (struct mipRegPkt *)packet;

	rewindMNList(mn_list);
	while( (cur = getCurrentMNEntry(mn_list)) != NULL){
		if(cur->mnaddr.s_addr == p->mnaddr.s_addr){
/* #define DEBUG_CHANGE_COA_OF_MN  */
#ifdef DEBUG_CHANGE_COA_OF_MN
			printf( "\n\tchgCOAofMN(), mn: %08X, old_COA: %08X, new_COA: %08X, old_state: %d, new_state: %d\n\n", 
				p->mnaddr.s_addr, 
				cur->coa.s_addr,
				mn_coa->s_addr, 
				cur->state, 
				mn_state);
#endif
			cur->coa.s_addr = mn_coa->s_addr;
			cur->state      = mn_state;

			return 0;
		}
	}

	return 1;
}

int        
queryCOA(struct in_addr *mn, struct in_addr *mn_coa, MnState *mn_state)
{
	struct in_addr          mnaddr;
	struct mnlinkListEntry  *cur;
	unsigned char           *p;


	mnaddr.s_addr = mn->s_addr;
	p = (unsigned char *)&(mnaddr.s_addr);
	p[0] = 1; p[1] = 0;
	
	rewindMNList(mn_list);
	while((cur = getCurrentMNEntry(mn_list)) != NULL){
/* #define DEBUG_QUERY_MN_LIST  */
#ifdef DEBUG_QUERY_MN_LIST
		printf( "\tmn in list: %s, %s, %d\n", 
				inet_ntoa(cur->mnaddr),
				inet_ntoa(cur->coa),
				cur->state
				);
#endif
		if(cur->mnaddr.s_addr == mnaddr.s_addr){
#ifdef DEBUG_QUERY_MN_LIST
			printf( "find entry in MN list.\n");
#endif
			if( (cur->state == AWAY) || (cur->state == NA) ){
				*mn_state = cur->state;
#ifdef DEBUG_QUERY_MN_LIST
			printf( "entry in MN list is NULL.\n");
#endif
				return 1;
			}
			mn_coa->s_addr = cur->coa.s_addr;
			*mn_state = cur->state;

			return 0;
		}
	}

#ifdef DEBUG_QUERY_MN_LIST
	printf( "entry is not found in MN list.\n");
#endif
	return 1;
}

int        
genOuterIPList(LLIST *allIP_list, LLIST *innerIP_list, LLIST *outerIP_list)
{
	struct linkListEntry *temp;


	rewindList(allIP_list);
	while((temp = getCurrentEntry(allIP_list)) != NULL){
		rewindList(innerIP_list);
		if(searchEntry(innerIP_list, &(temp->addr)) == NULL)
			addEntry2(outerIP_list, &(temp->addr));
	}

	return 0;
}

int        
addToVlist(struct in_addr *mn, struct in_addr *inneraddr)
{
	struct vlinkListEntry *cur;
	/* struct mipRegReplyPkt *p;


	p = (struct mipRegReplyPkt *)packet; */

	rewindVList(vlist);
	while((cur = getCurrentVEntry(vlist)) != NULL){
		if(cur->mnaddr.s_addr == mn->s_addr){
			if(cur->bindif.s_addr == inneraddr->s_addr){
				/* in the area of same agent, refresh the lifetime. */
				cur->lifetime = vlistLifetime;
				return 0; 
			}else{
				/* enter an area of new agent. */
				cur->bindif.s_addr = inneraddr->s_addr;
				cur->lifetime = vlistLifetime;
				return 1;
			}
		}
	}

	/* no match entry is found, add it. */
	if(!addVEntry2(vlist, mn, inneraddr, NULL, vlistLifetime))
		return 2;
	else
		return -1;

}

struct vlinkListEntry *
isInVlist(char *packet)
{
	struct vlinkListEntry *e;
	struct mipReplyPkt *p;


	p = (struct mipReplyPkt *)packet;


	if((e = searchVEntry(vlist, &(p->mnaddr))) != NULL){
#ifdef DEBUG_IS_IN_VLIST
		printf( "The agent has finished registration for the MN that sends reply before.\n");
#endif
		return e;
	}else{
#ifdef DEBUG_IS_IN_VLIST
		printf( "The MN that sends reply is not in visit list of this agent.\n");
#endif
		return NULL;
	}
}

struct in_addr * 
queryBindif(struct in_addr *mn)
{
	struct vlinkListEntry *result;


	if((result = searchVEntry(vlist, mn)) != NULL){
		/* printf("queryBindif: found @%p.\n", &(result->coa)); */
		return &(result->bindif);
	}else{
		/* printf("queryBindif: not found.\n"); */
		return NULL;
	}
}

int
updateBlist(char *updatepkt)
{
	unsigned int          found = 0; 
	unsigned long         ro_lifet;
	struct mipROupdatePkt *upkt;
	struct in_addr        ro_mnaddr, ro_coaddr, ro_haddr;
	struct blinkListEntry  *temp;


	upkt = (struct mipROupdatePkt *)updatepkt;
	
	ro_mnaddr = upkt->mnaddr;
	ro_coaddr = upkt->coaddr;
	ro_haddr  = upkt->haddr;
	ro_lifet  = 1000000 * (upkt->lifetime); /* transform sec into microsec. */

	rewindBList(bindlist);
	while((temp = getCurrentBEntry(bindlist)) != NULL){
		if(temp->mnaddr.s_addr == ro_mnaddr.s_addr){
#ifdef DEBUG_UPDATE_BLIST
			printf( "bindlist entry(%08X, %08X) is found, modify it.\n", temp->mnaddr.s_addr, temp->coa.s_addr);
#endif
			temp->coa.s_addr = ro_coaddr.s_addr;
			temp->lifetime   = ro_lifet;
			/* pending */
			/* temp->haddr.s_addr = ro_haddr.s_addr; */
			found = 1;
			break;
		}
	}
	if(found == 0){
#ifdef DEBUG_UPDATE_BLIST
		printf( " %08X create new bindlist(%08X, %08X, %08X, %ld) entry.\n", coAddr.s_addr, ro_mnaddr.s_addr, ro_coaddr.s_addr, ro_haddr.s_addr,
				                                      ro_lifet);
#endif
		addBEntry2(bindlist, &ro_mnaddr, &ro_coaddr, &ro_haddr, 
				                              ro_lifet);
		/* turnOnDivert(if_list, &ro_mnaddr); */
	}

	return 0;
}

int 
ackToMnd(struct in_addr *mn, struct in_addr *inneraddr)
{
	char *p, *q;
	struct in_addr         *bindif;
	struct sockaddr_in     mnaddr;
	socklen_t              mnaddr_len;
	struct mipReplyAckPkt  replyAckPacket;


	memset(&mnaddr, 0, sizeof(mnaddr));
	mnaddr.sin_family = AF_INET;
	mnaddr.sin_port   = htons(PORT_LISTEN);
	mnaddr.sin_addr   = *mn;
	mnaddr_len = sizeof(mnaddr);

	/* Fill the data field of reply ack packet to MN. */
	replyAckPacket.type             = 99;
	replyAckPacket.agentOuterAddr.s_addr = coAddr.s_addr;
	replyAckPacket.agentInnerAddr.s_addr = inneraddr->s_addr; 
	replyAckPacket.lifetime         = REPLY_LIFTTIME;

	/* Reply ack to MN using s.s.d.d format */
	bindif = queryBindif(mn);
	if(bindif == NULL){
		printf( "[X] ackToMnd: bindif is null.\n");
		exit(1);
	}
	p = (char *)&(bindif->s_addr);
	q = (char *)&(mnaddr.sin_addr.s_addr);
	q[0] = p[2]; q[1] = p[3];
#ifdef ACK_TO_MND
	printf( "reply ack to:%08X(s.s.d.d format)\n", mnaddr.sin_addr.s_addr);
#endif

	if(Sendto(listenSocket, &replyAckPacket, sizeof(replyAckPacket), 0, 
				(struct sockaddr *)&mnaddr, mnaddr_len) < 0){
		printf( "Failure in sendto() of reply ack.\n");
		exit(1);
	}

	return 0;
}

int 
discardPacket(char *p, int n)
{
	printf( "Packet is discarded.\n");
	return 0;
}

int  
addBcastRoute(struct in_addr innerip, struct sockaddr_in baddr)
{
	char tunName[10];
	char ssdd[30];
	char addCmdString[100]="route add ";
	char delCmdString[100]="route delete ";
	unsigned char *p, *q;


	p = (unsigned char *)&(baddr.sin_addr.s_addr);
	q = (unsigned char *)&(innerip.s_addr);
	sprintf(ssdd, "%u.%u.%u.%u", q[2], q[3], p[2], p[3]);

	if(ip2tun(&innerip, tunName) != 0)
		printf( "[X] ip2tun() in adBcastRoute() fails.\n"); 
	/*if(innerip.s_addr == inet_addr("1.0.4.1"))
		strncpy(tunName, "tun6", 10);
	else
		strncpy(tunName, "tun8", 10);*/


	strcat(delCmdString, ssdd);
	strcat(delCmdString, " 2>/dev/null");

	strcat(addCmdString, ssdd);
	strcat(addCmdString, " dev ");
	strcat(addCmdString, tunName);
	strcat(addCmdString, " 2>/dev/null");

/* #define DEBUG_ADD_BCAST_ROUTE */
#ifdef DEBUG_ADD_BCAST_ROUTE
	printf( "agent[%x] addBcastRoute() addCmd: %s, delCmd: %s\n", coAddr.s_addr, addCmdString, delCmdString);
#endif

	system(delCmdString); /* don't care "not found" error. */
	if(system(addCmdString) != 0){
		printf( "[X] Failure in system() of addBcastRoute().\n");
		exit(1);
	}

	return 0;
}

int 
modifyRouteTBL_home(struct in_addr addr_mn)
{
	int  i;
	unsigned char  *p, *q;
	struct in_addr *bindif, tmp_gw;


	char mn_tun[10];
	char ha_innerTun[10];
	char temp_get[30];
	char temp1_chg1[30];
	char temp2_chg1[30];
	char temp1_chg2[30];
	char temp2_chg2[30];
	char cacheFlushCmdString[100]="echo 0 > /proc/sys/net/ipv4/route/flush";
/*
	char gateway[30];
	char temp1_chg3[30];
	char temp2_chg3[30];
	// char ha_innerAddr[30];
	int  argC_chg1 = 4;
	int  argC_chg2 = 5;
	int  argC_chg3 = 5;
*/
/*
	int  argC_get1 = 3; // get -net x.x.x.x
	char *argV_chg3[5] ={"change","-net","","-interface", ""};
*/
	char *argV_chg1[4] ={"change","-net","",""};
	char *argV_chg2[5] ={"change","-net","","-interface", ""};
	char *argV_get1[3] ={"get","-net",""};

	char addRouteCmd[200];


	/* printf( "HA: Modifying the routing table.\n"); */
	argV_get1[2] = temp_get;
	argV_chg1[2] = temp1_chg1;
	argV_chg1[3] = temp2_chg1;
	argV_chg2[2] = temp1_chg2;
	argV_chg2[4] = temp2_chg2;
	bindif = queryBindif(&addr_mn);
	if(bindif == NULL){
		printf( "[X] modifyRTBL_home: bindif is null.\n");
		exit(1);
	}

	tmp_gw = *bindif;
	p = (unsigned char *)&(addr_mn.s_addr);
	q = (unsigned char *)&(tmp_gw.s_addr);
	/* q = (unsigned char *)&(agentInnerAddr.sin_addr.s_addr); */
	/* sprintf(ha_innerAddr, "%u.%u.%u.%u", q[0], q[1], q[2], q[3]); */
	if(ip2tun(&addr_mn, mn_tun) != 0)
		printf( "[X] ip2tun() in modifyRoute_home() fails.\n");
	/*strncpy(mn_tun, "tun9", 10);*/

	if(ip2tun(bindif, ha_innerTun) != 0)
		printf( "[X] ip2tun() in modifyRoute_home() fails.\n"); 
	/*strncpy(ha_innerTun, "tun6", 10); */


	for(i = 1 ; i<NCTUNS_SUBNET_CNT ; i++){ /* skip subnet 255. */
		sprintf(argV_get1[2], "%u.%u.%u.0/24", p[2], p[3], i);
		/* printf( "getCmd: %s ", argV_get1[2]); */
		if(i == q[2]) {
			sprintf(addRouteCmd, "%s%s%s%s%s", 
			        "route add -net ", 
				argV_get1[2],
				" dev ", mn_tun, " 2>/dev/null");
#undef DEBUG_LINUX
#ifdef DEBUG_LINUX
			printf("agent[%x] modifyRouteTBL_home(), addRouteCmd: %s\n", coAddr.s_addr, addRouteCmd);		
#endif
			system(addRouteCmd);
		}else {
			// gateway IP is in ssdd format
			q[0] = p[2]; q[1] = p[3];

			sprintf(addRouteCmd, "%s%s%s%s%s",
			        "route add -net ", 
				argV_get1[2],
				" gw ",
			        inet_ntoa(tmp_gw),
				" 2>/dev/null"
			);
#ifdef DEBUG_LINUX
			printf("agent[%x] modifyRouteTBL_home(), addRouteCmd: %s\n", coAddr.s_addr, addRouteCmd);		
#endif
			system(addRouteCmd);
		}
	}

	system(cacheFlushCmdString);
	return 0;
}

int 
modifyRouteTBL_foreign(struct in_addr addr_mn)
{
	int  i;
	unsigned char *p, *q;
	struct in_addr *bindif;

	char mn_tun[10];
	char fa_innerTun[10];
	char temp_get[30];
	char temp1_chg1[30];
	char temp2_chg1[30];
	char temp1_chg2[30];
	char temp2_chg2[30];
	char temp1_chg3[30];
	char temp2_chg3[30];
	char fa_innerAddr[30];
	char cacheFlushCmdString[100]="echo 0 > /proc/sys/net/ipv4/route/flush";
/*
	char gateway[30];
	int  argC_chg1 = 4;
	int  argC_chg2 = 5;
	int  argC_chg3 = 5;
	int  argC_get1 = 3; // get -net x.x.x.x
*/
	char *argV_chg1[4] ={"change","-net","",""};
	char *argV_chg2[5] ={"change","-net","","-interface", ""};
	char *argV_chg3[5] ={"change","-net","","-interface", ""};
	char *argV_get1[3] ={"get","-net",""};

	char addRouteCmd[200];
	char delRouteCmd[200];


	/* printf( "FA: Modifying the routing table.\n"); */


	argV_get1[2] = temp_get;
	argV_chg1[2] = temp1_chg1;
	argV_chg1[3] = temp2_chg1;
	argV_chg2[2] = temp1_chg2;
	argV_chg2[4] = temp2_chg2;
	argV_chg3[2] = temp1_chg3;
	argV_chg3[4] = temp2_chg3;
	bindif = queryBindif(&addr_mn);
	if(bindif == NULL){
		printf( "[X] modifyRTBL_foreign: bindif is null.\n");
		exit(1);
	}
	p = (unsigned char *)&(addr_mn.s_addr);
	q = (unsigned char *)&(bindif->s_addr);
	sprintf(fa_innerAddr, "%u.%u.%u.%u", p[2], p[3], q[2], q[3]);
	if(ip2tun(&addr_mn, mn_tun) != 0)
		printf( "[X] ip2tun() in modifyRoute_foreign() fails.\n");
	/*strncpy(mn_tun, "tun9", 10);*/

	if(ip2tun(bindif, fa_innerTun) != 0)
		printf( "[X] ip2tun() in modifyRoute_foreign() fails.\n");
	/*strncpy(fa_innerTun, "tun8", 10);*/


	/* Change route entries for FA to MN. */
	sprintf(argV_chg3[2], "%u.%u.%u.0/24", q[2], q[3], p[2]);
	sprintf(argV_chg3[4], "%s", fa_innerTun);
	sprintf(delRouteCmd, "%s%s", 
		             "route del -net ",
		             argV_chg3[2]
		);
	sprintf(addRouteCmd, "%s%s%s%s%s", 
		             "route add -net ",
		             argV_chg3[2],
		             " dev ",
		             fa_innerTun,
			     " 2>/dev/null"
		);
#undef DEBUG_LINUX
#ifdef DEBUG_LINUX
			printf("agent[%x] modifyRouteTBL_foreign(), route for FA to MN, addRouteCmd: %s\n", coAddr.s_addr, addRouteCmd);		
#endif
	system(delRouteCmd);
	system(addRouteCmd);

	/* Change route entries for MN to other subnets. */
	for(i = 1 ; i<NCTUNS_SUBNET_CNT ; i++){ /* skip subnet 255. */
		sprintf(argV_get1[2], "%u.%u.%u.0/24", p[2], p[3], i);
		/* printf( "getCmd: %s", argV_get1[2]); */
		if(i == q[2]) {
			sprintf(delRouteCmd, "%s%s", 
			        "route del -net ", 
				argV_get1[2]);
			sprintf(addRouteCmd, "%s%s%s%s%s", 
			        "route add -net ", 
				argV_get1[2],
				" dev ", mn_tun, " 2>/dev/null");
#ifdef DEBUG_LINUX
printf("agent[%x] modifyRouteTBL_foreign(), addRouteCmd: %s\n", coAddr.s_addr, addRouteCmd);		
#endif
			system(delRouteCmd);
			system(addRouteCmd);
		}else {
			sprintf(delRouteCmd, "%s%s", 
			        "route del -net ", 
				argV_get1[2]);
			sprintf(addRouteCmd, "%s%s%s%s%s",
			        "route add -net ", 
				argV_get1[2],
				" gw ",
			        fa_innerAddr,
				" 2>/dev/null"
			);
#ifdef DEBUG_LINUX
printf("agent[%x] modifyRouteTBL_foreign(), addRouteCmd: %s\n", coAddr.s_addr, addRouteCmd);		
#endif
			system(delRouteCmd);
			system(addRouteCmd);
		}
	}

	system(cacheFlushCmdString);
	return 0;
}


/* FA uses this function and content of confirmPacket to register to HA. */
int        
registerToHA(struct mipReplyPkt *replyPkt)
{
	int                    n;
	struct mipRegPkt       regPacket;
	struct sockaddr_in     ha;


	/* fill the registration packet */
	regPacket.type     = 1;
	regPacket.mnaddr   = replyPkt->mnaddr;
	regPacket.haddr    = replyPkt->haddr;;
	/* regPacket.coaddr   = outerIP_list->head->addr; */
	regPacket.coaddr   = coAddr;
	/* pending */
	regPacket.agentInnerAddr = replyPkt->agentInnerAddr;
	regPacket.lifetime = 100;

	memset(&ha, 0, sizeof(ha));
	ha.sin_family = AF_INET;
	ha.sin_addr   = replyPkt->haddr;
	ha.sin_port   = htons(PORT_LISTEN);

#ifdef DEBUG_REG_TO_HA
	printf("Registering to HA: %08X.\n", ha.sin_addr.s_addr);
#endif
	n = Sendto(registerSocket, &regPacket, sizeof(regPacket), 0,
				(struct sockaddr *)&ha, sizeof(ha)); 
	if(n < 0){
		printf( "[X] Failure in sendto() of registration.\n");
		exit(1);
	}

	return 0;
}


/* After registration packet been received by HA, HA replies to FA. */
int 
regReplyToFA(struct mipRegPkt *reg)
{
	int                    n;
	struct mipRegReplyPkt  regReply;
	struct sockaddr_in     fa;


	regReply.type     = 3;
	regReply.mnaddr   = reg->mnaddr;        
	regReply.haddr    = reg->haddr;
	regReply.agentInnerAddr = reg->agentInnerAddr;
	regReply.lifetime = 100;

	fa.sin_family = AF_INET;
	fa.sin_addr   = reg->coaddr;
	fa.sin_port   = htons(PORT_LISTEN);

#ifdef DEBUG_REGREPLY_TO_FA
	printf( "Sending regReply to FA: %08X.\n", fa.sin_addr.s_addr);
#endif
	n = Sendto(registerSocket, &regReply, sizeof(regReply), 0,
				(struct sockaddr *)&fa, sizeof(fa)); 
	if(n < 0){
		printf( "[X] Failure in sendto() of registration reply.\n");
		exit(1);
	}

	return 0;
}

/* Turn on divert socket on HA by ipfw. */
int        
turnOnDivertOnHA(LLIST *outer_if_list, MNLLIST *mn_ip_list)
{
	struct mnlinkListEntry *mnip; 
	struct linkListEntry   *outerip;
	char ssdd[100];


	/* pending problem: may flush other rules needed. */
	/* if(system("ipfw -f flush") != 0)
		printf( "\tsystem() in \"ipfw -f flush\" fails.\n"); */

	rewindMNList(mn_ip_list);
	while( (mnip = getCurrentMNEntry(mn_ip_list)) != NULL){
		rewindList(outer_if_list);
		while( (outerip = getCurrentEntry(outer_if_list)) != NULL){

			ip2ssdd(outerip->addr, mnip->mnaddr, ssdd);
{
        		struct divert_rule dr;
        		int len=sizeof(struct divert_rule);
                                                                                
        		//flush
        		//syscall(259, DIVERT_FLUSH, 0, 0, 0, 0);
                                                                                
        		//add head
        		dr.proto = IPPROTO_IP;
        		dr.srcip = 0;
//#define DEBUG_DIVERT
#ifdef DEBUG_DIVERT
printf("[31mfilter dst_ip: %s, strlen: %d[m\n", ssdd, strlen(ssdd));
#endif
        		dr.smask = 0;
        		dr.dstip = inet_addr(ssdd);
        		dr.dmask = 0xffffffff;
        		dr.sport = 0;
        		dr.dport = 0;
        		syscall_NCTUNS_divert(syscall_NSC_divert_ADDHEAD, divertSocket, NF_IP_FORWARD, &dr, len);
                                                                                
        		//show info
        		syscall_NCTUNS_divert(syscall_NSC_divert_INFO, 0, NF_IP_FORWARD, 0, 0);
}

		}
	}

	return 0;
}

/* search name of tun that given IP bind on .*/
int      
ip2tun(struct in_addr *addr, char* tunName)
{
	struct ifaddrs *head, *buf;


	/* buf = malloc(300);  */
	//*buf = malloc(sizeof(struct ifaddrs)); /* pending */
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
	/* free(buf); */

	return 1;
}

int
genIpfwStr(char *ssdd, char *tun, char *cmdstr)
{
	/* @@@ */
	sprintf(cmdstr, "%s%d%s%s%s%s", "ipfw add divert ", portDivert, 
			        " ip from any to ", ssdd, " via ", tun);

	return 0;
}

unsigned short in_cksum(unsigned short *addr,int len)
{
        register int sum = 0;
        u_short answer = 0;
        register u_short *w = addr;
        register int nleft = len;


        /*
         * Our algorithm is simple, using a 32 bit accumulator (sum), we add
         * sequential 16 bit words to it, and at the end, fold back all the
         * carry bits from the top 16 bits into the lower 16 bits.
         */
        while (nleft > 1)  {
                sum += *w++;
                nleft -= 2;
        }

        /* mop up an odd byte, if necessary */
        if (nleft == 1) {
                *(u_char *)(&answer) = *(u_char *)w ;
                sum += answer;
        }

        /* add back carry outs from top 16 bits to low 16 bits */
        sum = (sum >> 16) + (sum & 0xffff);     /* add hi 16 to low 16 */
        sum += (sum >> 16);                     /* add carry */
        answer = ~sum;                          /* truncate to 16 bits */
        return(answer);
}

int        
bindUpdate(struct in_addr *target, struct in_addr *dstMn, struct in_addr *ha, int bUtype, int ack)
{
	int                    n;
	struct sockaddr_in     sin;
	MnState                state;
	struct mipROupdatePkt  roupdate;


	/* pending */
	if(target->s_addr == 0)
		return 0;

	roupdate.type   = 18;
	roupdate.mnaddr = *dstMn;
	roupdate.lifetime = blistLifetime; /* seconds. */

	/* pending */
	/* This bindupdate is sent from HA. */
	if(bUtype == 0){
		queryCOA(dstMn, &(roupdate.coaddr), &state);
		roupdate.haddr.s_addr = coAddr.s_addr;

	/* This smooth handoff bindUpdate is sent between agents. */
	}else if(bUtype == 1){
		roupdate.coaddr.s_addr = coAddr.s_addr;
		roupdate.haddr.s_addr = ha->s_addr;
	}

	if(ack){
		/* start the RO_UPDATE retransmission timer. */
		roupdatef = 1;
		roupdate_cnt = 1;
		roupdatet = roupdate_timerLifetime;
		roupdate_target.s_addr = target->s_addr;
		roupdate_mn.s_addr     = dstMn->s_addr;
		roupdate_coa.s_addr    = roupdate.coaddr.s_addr;
		roupdate_haddr.s_addr  = roupdate.haddr.s_addr;
		roupdate_lifet         = roupdate.lifetime;
	}

	memset(&sin, 0, sizeof(sin));
	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = target->s_addr;
	sin.sin_port        = htons(PORT_LISTEN);


	n = Sendto(listenSocket, &roupdate, sizeof(roupdate), 0, 
			(struct sockaddr *)&sin, sizeof(sin));
	if(n < 0){
		printf( "[X] Failure in sendto() of roupdate.\n");
		exit(1);
	}else{
		/* printf( "bindUpdate: %d bytes sent.\n", n); */
#ifdef DEBUG_BU_SYM
		printf( "bU ");
#endif
	}

	return 0;
}

int        
rebindUpdate(struct in_addr *target, struct in_addr *mn, struct in_addr *coa, struct in_addr *haddr, int lifet)
{
	int                    n;
	struct sockaddr_in     sin;
	struct mipROupdatePkt  roupdate;


	roupdate.type          = 18;
	roupdate.mnaddr        = *mn;
	roupdate.coaddr.s_addr = coa->s_addr;
	roupdate.haddr.s_addr  = haddr->s_addr;
	roupdate.lifetime      = lifet;

	memset(&sin, 0, sizeof(sin));
	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = target->s_addr;
	sin.sin_port        = htons(PORT_LISTEN);

	n = Sendto(listenSocket, &roupdate, sizeof(roupdate), 0, 
			(struct sockaddr *)&sin, sizeof(sin));
	if(n < 0){
		printf( "[X] Failure in sendto() of reROupdate.\n");
		exit(1);
	}
#ifdef DEBUG_REBU_SYM
	else
		/* printf( "bindUpdate: %d bytes sent.\n", n); */
		printf( "bUr ");
#endif

	return 0;
}

int        
bindAck(struct in_addr *a, struct in_addr *mn)
{
	int                  n;
	struct mipROackPkt   roack;
	struct sockaddr_in   sin;


	roack.type          = 19;
	roack.mnaddr.s_addr = mn->s_addr;

	sin.sin_family = AF_INET;
	sin.sin_port   = htons(PORT_LISTEN); 
	sin.sin_addr.s_addr = a->s_addr;

	n = Sendto(listenSocket, &roack, sizeof(roack), 0, 
			(struct sockaddr *)&sin, sizeof(sin));
	if(n < 0){
		printf( "[X] Failure in sendto() of roupdate.\n");
		exit(1);
	}
#ifdef DEBUG_BA_SYM
	else
	/* printf( "bindAck: %d bytes sent.\n", n); */
	/* #define DEBUG_BA_SYM */
		printf( "bA \n");
#endif
	return 0;
}

int        
bindWarn(struct in_addr *ha, struct in_addr *mn, struct in_addr *chost)
{
	int                  n;
	struct sockaddr_in   sin;
	struct mipROwarnPkt  rowarn;


	rowarn.type   = 16;
	rowarn.mnaddr = *mn;
	rowarn.target = *chost;
	/* queryCOA(dstMn, &(rowarn.coaddr), &state); */

	memset(&sin, 0, sizeof(sin));
	sin.sin_family      = AF_INET;
	sin.sin_addr.s_addr = ha->s_addr;
	sin.sin_port        = htons(PORT_LISTEN);

	n = Sendto(listenSocket, &rowarn, sizeof(rowarn), 0, 
			(struct sockaddr *)&sin, sizeof(sin));
	if(n < 0){
		printf( "[X] Failure in sendto() of rowarn, errno: %d.\n", errno);
		/* exit(1); */
	}
#ifdef DEBUG_BW_SYM
	else
		printf( "bW ");
#endif
		/* printf( "%d bytes sent.\n", n); */

	return 0;
}

int        
shForward(char *pkt, struct in_addr *binding, int size)
{
	int m;
	struct ip *ip;
	struct sockaddr_in coa;
	socklen_t coa_len;


	ip = (struct ip*)pkt;

	/* Strange!!! The ip_len captured by divert socket is host order. */
#ifdef DEBUG_SH_SYM
	printf( "f ");
#endif
	/* ip->ip_len = htons(ip->ip_len);
	ip->ip_off = htons(ip->ip_off); */

	/* Data captured by divert socket is consist of header and 
	 * datapayload. It will be wholely sent to FA through UDP. */
	coa_len = sizeof(coa);
	coa.sin_family = AF_INET;
	coa.sin_addr   = *binding;
	if(ro_enable == 0)
		coa.sin_port   = htons(PORT_IPINUDP_ON_FA);
	else
		coa.sin_port   = htons(PORT_ROTUN_SOCKET);

	m = Sendto(listenSocket, (char *)pkt, size, 0x0, 
			(struct sockaddr *)&coa, coa_len);
	if(m != size)
		printf( "[X] Failure in sedning of smooth handoff forwarding.\n");

	return 0;
}

int        
specialTun(char *pkt, int size)
{
	int m;
	struct ip *ip;
	struct sockaddr_in sin;
	socklen_t sin_len;

	ip = (struct ip*)pkt;

	printf( "s ");
	/* Strange!!! The ip_len captured by divert socket is host order. */
	/* ip->ip_len = htons(ip->ip_len);
	ip->ip_off = htons(ip->ip_off); */

	/* Data captured by divert socket is consist of header and 
	 * datapayload. It will be wholely sent to FA through UDP. */
	sin_len = sizeof(sin);
	sin.sin_family = AF_INET;
	/* sin->sin_port   = htons(PORT_IPINUDP_ON_FA); */

	/* pendign */
	m = Sendto(listenSocket, (char *)pkt, size, 0x0, 
			(struct sockaddr *)&sin, sin_len);
	if(m != size)
		printf( "[X] Failure in sending of special tunnel.\n");

	return 0;
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
#ifdef DEBUG_NOBUFS_SYM
		printf( "E ");
#endif
		usleep(1000);
		errno = 0;
		goto sendAgain;
	}else
		return m;
}
