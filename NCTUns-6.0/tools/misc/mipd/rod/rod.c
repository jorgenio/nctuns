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
#include <signal.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/errno.h>

#include <unistd.h>
#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>

#include <arpa/inet.h>


#include <ifaddrs.h>   /* Used by ip2tun(). */

#include "packetFmt.h" /* format of MobileIP control packet. */

#include "linklist.h"
#include "blinklist.h"
#include "nctuns_mip.h"

#include "parseconf.h"

#include <limits.h>
#include <linux/types.h>
#include <linux/netfilter_ipv4.h>
#include <nctuns_syscall.h>
#include <nctuns_divert.h>

#define BUFSIZE            1800 /* pending, how much needed? */
#define BUFSIZE_LISTEN      500 /* used for small-sized control packets */

#define PORT_LISTEN        8118
#define PORT_RO_DIVERT     8122
#define PORT_ROTUN_SOCKET  8123

static void sig_alarm(int);
void       initSockets();

int        handleListenSocket(int);
int        handleDivertSocket(int);

int        queryBinding(struct ip *, struct sockaddr_in *);
int        ro_tunnel(struct sockaddr_in *, char *, int);

u_char     typeOfPacket(char *, struct in_addr *);

int        turnOnDivertList(LLIST *, BLLIST *);
int        turnOnDivert(LLIST *, struct in_addr *);
int        ip2ssdd(struct in_addr, struct in_addr, char *);
/* get tun name through ip bound on it. */
int        ip2tun(struct in_addr *, char *);
int        gencmdstr(char *ssdd, char *tun, char *cmdstr);
unsigned short in_cksum(unsigned short *addr,int len);
int        Sendto(int, void *, size_t, int, const struct sockaddr *, socklen_t);

LLIST      *if_list;
BLLIST     *bindlist;
int        roDivertPort;

int        listenSocket, divertSocket, roSocket;

u_int      globalt = 0, alarmDuration = 500;
int        divertcnt=0;

int
main(int argc, char *argv[])
{
	int     rc;
	fd_set  readmask;
	int     count = 0 ;


	bindlist = newBList();
	if_list  = newList();

	/* printf("bindlist(%ld), if_list(%ld)\n", bindlist, if_list); */
	parseConfig(argc, argv, &roDivertPort, bindlist, if_list);


	/* printf("printlist. ");
	printf("bindlist(%ld):", bindlist);
	printList(bindlist);
	printf("if_list(%ld):", if_list);
	printList(if_list);
	exit(1); */

	initSockets();

	turnOnDivertList(if_list, bindlist);

        if(signal(SIGALRM, sig_alarm) == SIG_ERR){
		printf( "signal() error.\n");
		exit(1);
	}
        ualarm(1, 0);

	/* sleep(1);  */
	while(1){
		FD_ZERO(&readmask);
		FD_SET(listenSocket, &readmask);
		FD_SET(divertSocket, &readmask);

#ifdef DEBUG_AGENT_SELECT
		printf( "agent: Start to select().\n");
#endif
		rc = select(roSocket+1, (fd_set *)&readmask, NULL, NULL, NULL);
#ifdef DEBUG_AGENT_SELECT
		printf( "agent: End of select().\n");
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
		if(FD_ISSET(divertSocket, &readmask)){
			count++;
			/* printf("diverted count: %d\n", count); */
			handleDivertSocket(divertSocket);
		}
	}

	return 0;
}

static void 
sig_alarm(int signo)
{
	globalt += alarmDuration;

	signal(SIGALRM, sig_alarm);
	ualarm(alarmDuration, 0); /* restart the timer */

}
void 
initSockets()
{
	const int on = 1;
	struct sockaddr_in bindAddr;


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
	/* bindAddr.sin_port        = htons(PORT_RO_DIVERT); */
	bindAddr.sin_port        = htons(roDivertPort);

	if(bind(divertSocket, (struct sockaddr *)&bindAddr, 
				                         sizeof(bindAddr)) < 0){
		printf( "[X] agent: Failure in bind() divert socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else
		printf( "[O] agent: Binding divert socket successfully.\n");
#endif

	/* Initialize route optimization tunneling socket. */
	roSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(roSocket == -1){
		printf( "[X] agent: Failure in creating ipInUdp socket.\n");
		exit(1);
	}
#ifdef DEBUG_INITSOCKS
	else		
		printf( "[O] agent: Creating ipInUdp socket successfully.\n");
#endif
	/*if(setsockopt(divertSocketOnFA, IPPROTO_IP, IP_HDRINCL, 
				(char *)&on, sizeof(on)) < 0){
		printf( "Failure in setsockopt() of divert socket.\n");
		exit(1);
	}*/
	memset(&bindAddr, 0, sizeof(bindAddr));
	bindAddr.sin_family      = AF_INET;
	bindAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	bindAddr.sin_port        = htons(PORT_ROTUN_SOCKET);

	if(bind(roSocket, (struct sockaddr *)&bindAddr, 
				              sizeof(bindAddr)) < 0)
	{
		printf( "[X] agent: Failure in bind() of ipInUdp socket on FA.\n");
		exit(1);
	}

}

int        
handleListenSocket(int listenSock)
{
	int                n, found = 0;
	struct sockaddr_in from;
	struct in_addr     ro_mnaddr, ro_coaddr;
	struct blinkListEntry *temp;
	socklen_t          from_len;
	char      buf[BUFSIZE_LISTEN];


	from_len = sizeof(from);
	
/* #define DEBUG_LISTENSOCKET  */
#ifdef DEBUG_LISTENSOCKET
	printf( "ro: Listen socket, Start to recvfrom().\n");
#endif
	n = recvfrom(listenSock, buf, BUFSIZE, 0, (struct sockaddr *)&from, 
			                                        &from_len);
#ifdef DEBUG_LISTENSOCKET
	printf( "ro: Listen Socket, End of recvfrom().\n");
#endif
		
	if(n < 0){
		printf( "[X] ro: Listen Socket, Failure in recvfrom.\n");
		exit(1);
	}

#ifdef DEBUG_LISTENSOCKET
	for(i=0 ; i<n ; i++){
		printf( "%02X ", (unsigned char)buf[i]);
	}
	printf( "\tFrom_addr: %08X, %d bytes received.\n", 
			       ntohl(from.sin_addr.s_addr), n);
#endif

	if(typeOfPacket(buf, &(from.sin_addr)) == RO_UPDATE){
		ro_mnaddr = ((struct mipROupdatePkt *)buf)->mnaddr;
		ro_coaddr = ((struct mipROupdatePkt *)buf)->coaddr;
/* #define DEBUG_ROD_ROUPDATE */
#ifdef DEBUG_ROD_ROUPDATE
		printf( "[32mCH: receives RO_UPDATE msg from %08X.[m\n", from.sin_addr.s_addr);
		printf( "[32mmnaddr: %08X, coaddr: %08X in the RO_UPDATE msg.[m\n", ro_mnaddr.s_addr, ro_coaddr.s_addr);
#endif

		rewindBList(bindlist);
		while((temp = getCurrentBEntry(bindlist)) != NULL){
			if(temp->mnaddr.s_addr == ro_mnaddr.s_addr){
#ifdef DEBUG_ROD_ROUPDATE
				printf( "[32mbinding entry is already exist. old binding is %08X, replaced by %08X[m\n", temp->coa.s_addr, ro_coaddr.s_addr);
#endif
				temp->coa.s_addr = ro_coaddr.s_addr;
				found = 1;
				break;
			}
		}
		if(found == 0){
			/* printf( "create new binding entry(%s, %s).\n",
					inet_ntoa(ro_mnaddr), 
					inet_ntoa(ro_coaddr)); */
#ifdef DEBUG_ROD_ROUPDATE
			printf( "[32mcreate new binding entry(%08X, %08X).[m\n",
					ro_mnaddr.s_addr, 
					ro_coaddr.s_addr); 
#endif
			/* addEntry(bindlist, inet_ntoa(ro_mnaddr), 
					inet_ntoa(ro_coaddr)); */ 
			addBEntry2(bindlist, &ro_mnaddr, &ro_coaddr); 
			/* printf( "bindlist(%p): ", bindlist);
			printList(bindlist); */
			turnOnDivert(if_list, &ro_mnaddr);
		}

	/* }else if(typeOfPacket(buf, &(from.sin_addr)) == RO_WARN){ */
	}

	return 0;
}

int 
handleDivertSocket(int divertSock)
{
	int                n, m;
	struct sockaddr_in from, coa;
	socklen_t          from_len;
	char      buf[BUFSIZE];
	struct ip          *ip;


	from_len = sizeof(from);

/* #define DEBUG_DIVERTSOCKET1     */
#ifdef DEBUG_DIVERTSOCKET1
	printf( "agent: Divert socket: Start to recvfrom().\n");
#endif
	n = recvfrom(divertSock, buf, BUFSIZE, 0, (struct sockaddr *)&from, 
			                                       &from_len);

#ifdef DEBUG_DIVERTSOCKET1
	printf( "agent: Divert socket: End of recvfrom().\n");
#endif
		
	if(n < 0){
		printf( "\n[X] agent: Divert socket,Failure in recvfrom.\n");
		exit(1);
/* #define DEBUG_DIVERTSOCKET2 */
#ifdef DEBUG_DIVERTSOCKET2
	}else if(n >= 1450){
		printf( "\n\t%d bytes received.\n", n);
#endif
	}

#if 0
	divertcnt++;
	if((divertcnt%100000) == 0)
		printf("100000\n");
#endif

/* #define DEBUG_DIVERTSOCKET2 */
/* #undef DEBUG_DIVERTSOCKET2  */
#ifdef DEBUG_DIVERTSOCKET2
	for(i=0 ; i<48 ; i++){
		if(i%2 == 0) printf( " ");
		printf( "%02X", (unsigned char)buf[i]);
		if((i+1)%16 == 0) printf( "\n");
	}
	printf( "\tfrom_addr: %08X \n", ntohl(from.sin_addr.s_addr));
#endif
	ip   = (struct ip*)buf;

	memset(&coa, 0, sizeof(coa));
	if(queryBinding(ip, &coa) < 0){ // binding info not exist
		/* printf("[X] queryBinding error.\n"); */
		printf("[32mnb \n[m\n");
		m = Sendto(divertSock, buf, n, 0, (struct sockaddr *)&from, from_len);

		if(m != n){
			printf( "[X] Error in sending no-binding pkt.");
		}
	}else
		ro_tunnel(&coa, buf, n); 

	/* Sendto(divertSock, buf, n, 0, (struct sockaddr *)&from, from_len); */

	return 0;
}

int        
queryBinding(struct ip *ip, struct sockaddr_in *coa)
{
	struct in_addr *dst, *result;


	dst = ssdd2dst(&(ip->ip_dst));

	if((result = searchBEntry(bindlist, dst)) != NULL){
		coa->sin_addr.s_addr = result->s_addr;
		free(dst);
		return 0;
	}else{
		free(dst);
		return -1;
	}

	/* coa->sin_addr.s_addr = (searchEntry(bindlist, dst))->s_addr;

	return 0; */
}

int        
ro_tunnel(struct sockaddr_in *coa, char *buf, int size)
{
	struct ip *ip;
	socklen_t coa_len;
	int m;

	ip = (struct ip*)buf;

	/* Strange!!! The ip_len captured by divert socket is host order. */
#ifdef DEBUG_ROD_SYM
	printf( "[32m* [m");
#endif

	/* Data captured by divert socket is consist of header and 
	 * datapayload. It will be wholely sent to FA through UDP. */
	coa_len = sizeof(*coa);
	coa->sin_family = AF_INET;
	coa->sin_port   = htons(PORT_ROTUN_SOCKET);

	/* printf( "Sending diverted data to FA.\n"); */
	m = Sendto(roSocket, (char *)buf, size, 0x0, 
			(struct sockaddr *)coa, coa_len);
	if(m != size)
		printf( "[X] Failure in sending ro_tunnel data to agent. errno: %d\n", errno);

	return 0;
}

u_char 
typeOfPacket(char *p, struct in_addr *fromAddr)
{
	struct mipPkt * p1;
	p1 =(struct mipPkt *)p; /* pending problem. */


	/* pending */
	if(p1->type == 9){         /* pending */
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
	}else{
		return UNKNOWN;          /* not a mobileIP packet, but bind */  
	}                                /* to port 8118. */
}

/* Turn on divert socket on corresponding host by ipfw. */
int        
turnOnDivertList(LLIST *ifip_list, BLLIST *mn_ip_list)
{
	struct linkListEntry  *ifip;
	struct blinkListEntry *mnip;
	char ssdd[100];
	char tun[10];


	/* printf( "Turn on the divert socket on ro host by ipfw.\n"); */

	/* pending problem: may flush other rules needed. */
	/* if(system("ipfw -f flush") != 0)
		printf( "\tsystem() in \"ipfw -f flush\" fails.\n"); */

	rewindBList(mn_ip_list);
	while( (mnip = getCurrentBEntry(mn_ip_list)) != NULL){
		rewindList(ifip_list);
		while( (ifip = getCurrentEntry(ifip_list)) != NULL){
			ip2tun(&(ifip->addr), tun); 
			ip2ssdd(mnip->mnaddr, ifip->addr, ssdd);

{
        		struct divert_rule dr;
        		int len=sizeof(struct divert_rule);
                                                                                
                                                                                
        		//add head
        		dr.proto = IPPROTO_IP;
        		dr.srcip = 0;
        		dr.smask = 0;
        		dr.dstip = inet_addr(ssdd);
        		dr.dmask = 0xffffffff;
        		dr.sport = 0;
        		dr.dport = 0;
        		syscall_NCTUNS_divert(syscall_NSC_divert_ADDHEAD, divertSocket, NF_IP_LOCAL_OUT, &dr, len);
                                                                                
        		//show info
        		syscall_NCTUNS_divert(syscall_NSC_divert_INFO, 0, NF_IP_LOCAL_OUT, 0, 0);
}
		}
	}


	return 0;
}

int        
turnOnDivert(LLIST *ifip_list, struct in_addr *mn)
{
	struct linkListEntry *ifip;
	char ssdd[100];
	char tun[10];


#ifdef DEBUG_TURN_ON_DIVERT
	printf( "rod: Turn on the divert socket for new binding entry(%08X).\n", mn->s_addr);
#endif

	/* pending problem: may flush other rules needed. */
	/* if(system("ipfw -f flush") != 0)
		printf( "\tsystem() in \"ipfw -f flush\" fails.\n"); */

	rewindList(ifip_list);
	/* printf("ifip_list entry(%d)", ifip_list->no); */
	while( (ifip = getCurrentEntry(ifip_list)) != NULL){
		/* printf("ifip: %08X ", ifip->addr.s_addr); */
		ip2tun(&(ifip->addr), tun); 
		/* sprintf(tun, "%s", "tun"); */
		ip2ssdd(ifip->addr, *mn, ssdd);

{
        		struct divert_rule dr;
        		int len=sizeof(struct divert_rule);
                                                                                
                                                                                
        		//add head
        		dr.proto = IPPROTO_IP;
        		dr.srcip = 0;
        		dr.smask = 0;
        		dr.dstip = inet_addr(ssdd);
        		dr.dmask = 0xffffffff;
        		dr.sport = 0;
        		dr.dport = 0;
        		syscall_NCTUNS_divert(syscall_NSC_divert_ADDHEAD, divertSocket, NF_IP_LOCAL_OUT, &dr, len);
                                                                                
        		//show info
        		syscall_NCTUNS_divert(syscall_NSC_divert_INFO, 0, NF_IP_LOCAL_OUT, 0, 0);
}
	}

	return 0;
}

int
ip2ssdd(struct in_addr src, struct in_addr dst, char *ssdd)
{
	unsigned char *p, *q; /* "unsigned" char is needed! */


	p = (unsigned char *)&(src.s_addr);
	q = (unsigned char *)&(dst.s_addr);

	/* s.s.d.d format */
	sprintf(ssdd, "%u.%u.%u.%u", p[2], p[3], q[2], q[3]); 

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
	while(buf) {
		if(IFADDR){
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
gencmdstr(char *ssdd, char *tun, char *cmdstr)
{
	/* @@@ */
	/* sprintf(cmdstr, "%s%d%s%s%s%s", "ipfw add divert ", PORT_RO_DIVERT, 
			        " ip from any to", ssdd, "via ", tun); */
	sprintf(cmdstr, "%s%d%s%s%s%s", "ipfw add divert ", roDivertPort, 
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
