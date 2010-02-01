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

/********************************************************************/


#ifndef _RTP_API_H
#define _RTP_API_H


/********************************************************************/


#include <sys/types.h>
#include <sys/time.h>
#include <netinet/in.h>	// struct sockaddr_in
#include <sys/socket.h>	// struct sockaddr

#include "rtp_error.h" 
#include "rtp_config.h"


/********************************************************************/


#define FALSE 0
#define TRUE 1


/* Type of rtp status */
#define SENDONLY 1
#define RECVONLY 2
#define SENDRECV 3
#define INACTIVE 4


/* Type sizes */
typedef long	int		int32;
typedef short	int		int16;
typedef signed	char		int8;

typedef unsigned long	int	u_int32;
typedef unsigned short	int	u_int16;
typedef unsigned char		u_int8;  


/* The session_id is the identity of session_list that we can operate */
typedef int32 session_id;


/* We use socktype to trace rtp/rtcp socket */
typedef int socktype;


/* RTP packet header */
typedef struct {
	
	unsigned int 	version:2; 	/* protocol version */
	unsigned int 	p:1; 		/* padding flag */
	unsigned int 	x:1; 		/* header extension flag */
	unsigned int 	cc:4; 		/* CSRC count */
	unsigned int 	m:1; 		/* marker bit */
	unsigned int 	pt:7; 		/* payload type */
	unsigned int 	seq:16; 	/* sequence number */
	u_int32 	ts; 		/* timestamp */
	u_int32 	ssrc; 		/* synchronization source */
	//u_int32 	*csrc; 		/* optional CSRC list */
	
} rtp_hdr_t;


/* RTP packet type */
typedef struct {
	
	rtp_hdr_t rtphdr; 			/* rtp header */
	char 	  pdata[MAX_PAYLOAD_LENGTH];	/* payload data */
	int 	  plen;				/* payload data length */
	
} rtp_pkt_t;


/* These define the RTCP packet types */	
#define RTCP_SR 	200
#define	RTCP_RR 	201
#define	RTCP_SDES 	202
#define	RTCP_BYE 	203
#define	RTCP_APP 	204


/* The item types of rtcp SDES packet */
#define	RTCP_SDES_END 	0
#define	RTCP_SDES_CNAME 1
#define	RTCP_SDES_NAME 	2
#define	RTCP_SDES_EMAIL 3
#define	RTCP_SDES_PHONE 4
#define	RTCP_SDES_LOC 	5
#define	RTCP_SDES_TOOL 	6
#define	RTCP_SDES_NOTE 	7
#define	RTCP_SDES_PRIV 	8


/* Big-endian mask for version, padding bit and packet type pair */
#define RTCP_VALID_MASK (0xc000 | 0x2000 | 0xfe)
#define RTCP_VALID_VALUE ((RTP_VERSION << 14) | RTCP_SR)


/* RTCP common header word */
typedef struct {
	
	unsigned int 	version:2; 	/* protocol version */
	unsigned int 	p:1; 		/* padding flag */
	unsigned int 	count:5; 	/* varies by packet type */
	unsigned int 	pt:8; 		/* RTCP packet type */
	unsigned int 	length:16; 	/* pkt len in words, w/o this word */

} rtcp_common_t;


/* Reception report block */
typedef struct {
	
	u_int32 	ssrc; 		/* data source being reported */
	unsigned int 	fraction:8;	/* fraction lost since last SR/RR */
	int 		lost:24; 	/* cumul. no. pkts lost (signed!) */
	u_int32 	last_seq; 	/* extended last seq. no. received */
	u_int32 	jitter; 	/* interarrival jitter */
	u_int32 	lsr; 		/* last SR packet from this source */
	u_int32 	dlsr; 		/* delay since last SR packet */
	
} rtcp_rr_t;


/* SDES item */
typedef struct {
	
	u_int8	type;		/* type of item (rtcp_sdes_type_t) */
	u_int8	length; 	/* length of item (in octets) */
	char	data[1];	/* text, not null-terminated */

} rtcp_sdes_item_t;


/* SR */
typedef struct {
	
	u_int32         ssrc;           /* sender generating this report */
	u_int32         ntp_sec;        /* NTP timestamp */
	u_int32         ntp_frac;
	u_int32         rtp_ts;         /* RTP timestamp */
	u_int32         psent;          /* packets sent */
	u_int32         osent;          /* octets sent */

} rtcp_sr_t;


/* One RTCP packet */
typedef struct {

	rtcp_common_t *common;	/* common header */

	union {
		/* sender report (SR) */
		struct {
			rtcp_sr_t	*rtcp_sr;
			rtcp_rr_t 	*rr[MAX_REPORT_BLOCK];	/* variable-length list */
		} sr;
		/* reception report (RR) */
		struct {
			u_int32 	*ssrc; 			/* receiver generating this report */
			rtcp_rr_t 	*rr[MAX_REPORT_BLOCK];	/* variable-length list */
		} rr;
		/* source description (SDES) */
		struct rtcp_sdes {
			u_int32			src;		/* first SSRC/CSRC */
			rtcp_sdes_item_t	item[1]; 	/* list of SDES items */
		} sdes;
		/* BYE */
		struct {
			u_int32 	*src;			/* list of sources */
			//u_int8	length;
		} bye;
	} r;
} rtcp_t;


typedef struct rtcp_sdes rtcp_sdes_t;


/* NTP timestamp structure */
typedef struct {

	u_int32 sec;
	u_int32 frac;

} ntp_t;

#define NTP_OFFSET 2208988800u



/*
 * Per-source state information
 *
 * A non-zero s->probation marks the source as not yet valid so the state may be discarded 
 * after a short timeout rather than a long one. 
 */
typedef struct {

	u_int16 max_seq; 	/* highest seq. number seen */
	u_int32 cycles; 	/* shifted count of seq. number cycles */
	u_int32 base_seq; 	/* base seq number */
	u_int32 bad_seq; 	/* last 'bad' seq number + 1 */
	u_int32 probation; 	/* sequ. packets till source is valid */
	u_int32 received; 	/* packets received */
	u_int32 expected_prior; /* packet expected at last interval */
	u_int32 received_prior; /* packet received at last interval */
	u_int32 transit;	/* relative trans time for prev pkt, ( R i - S i ). */
	u_int32 jitter; 	/* estimated jitter */
	/* ... */
	
} source_t;


/*
 * member status, indicated by sequence of packet we received.
 * if SSRC is timeout, valid->inactive, inactive->delete, invalid->delete
 * local member always is valid.
 */ 
typedef enum {

	inactive,
	valid,
	invalid,
	bye

} memberstatus;


/* Type of sdes */
typedef struct {

	/* intervals is only for local member to compute interval to send SDES item */
	/* 1~8, RTCP_SDES_CNAME~RTCP_SDES_PRIV, 0 is no meaning.(RTCP_SDES_END) */
	int  intervals[9];
	int  length[9];	
	char *item[9];
	
} sdes_t;


/* CONTRIBUTOR */
typedef struct contributor {
	u_int32	ssrc;
	struct contributor *next;
} csrc_t;


/* the member structure in rtp session  */
typedef struct mb {
	
	/* the SSRC of this member */
	u_int32 ssrc;

	/* true if collision */
	int collide;

	/* It is true if the member is also a sender */
	int is_sender;

	/* It is true if the member is a contributor */
	int contributor;
	
	/* status of the member(invalid or valid) */
	memberstatus status;
	
	/* the transport address of the member, 0 is for RTP, 1 is for RTCP. */
	struct sockaddr fromaddr[2];
	
	/* the most recent time that we received a RTP packet from this member */
	struct timeval last_rtp_recv_time;
	
	/* the most recent time that we received a RTCP packet from this member */
	struct timeval last_rtcp_recv_time;

	/* the most recent time that we received an SR packet from this member. */
	struct timeval last_sr_recv_time;

	/* the latest rtp header we receive from this member */
	rtp_pkt_t rtp_recvpkt;

	/* the accumulate information about rtp(rtcp) transmission */
	//trans_info_t trans_info;
		
	/* per-source state */
	source_t source;

	/* sdes item info. of the member */
	sdes_t sdes;
			
	struct mb *next;

	/* below is info. if the member is sender. */
	u_int32         ntp_sec;        /* NTP timestamp */
	u_int32         ntp_frac;
	u_int32         rtp_ts;         /* RTP timestamp */
	u_int32         psent;          /* packets sent */
	u_int32         osent;          /* octets sent */	
	
	rtcp_rr_t rtcp_rr;

	/* rtp recv_packets counts received by this member */
	u_int32 recv_rtppkt_num;

	/* rtp recv_bytes counts received by this member */
	u_int32 recv_rtpbytes_num;
		
	/* rtcp recv_packet counts received by this member */
	//u_int32 recv_rtcppkt_num;
	
	/* rtcp recv_bytes counts received by this member */
	//u_int32 recv_rtcpbytes_num;	
			
} member_t;


/* conflicting list */
typedef struct conflict {
	
	/* conflict ssrc */
	u_int32 ssrc;
	
	/* last conflict time */
	struct timeval last_occur_time;
	
	/* if no conflict in last 10 intervals we will remove it */ 
	int count;
	
	/* the transport address of the member, 0 is for RTP, 1 is for RTCP. */
	//struct sockaddr fromaddr[2];
	
	/* the address that conflict last time */
	struct sockaddr conflict_addr;
	
	struct conflict *next;
	
} conflict_list_t;


/* the type of underlying protocol */
typedef enum { 

	udp, 
	tcp 

} protocol_t;


/* address_infomation type
 *
 * a linked list of addresses, ports, and TTL's.
 * it is used to hold addresses to send(receive) to for RTP and RTCP packets.
 */
typedef struct address_info {

	/* Set to 0 if it should be deleted */
	int living;                 

	/* type of underlying protocol, default is udp */
	protocol_t rtp_protocol;
	protocol_t rtcp_protocol;
		
	/* Socket for data */
	socktype rtpskt;          
	
	/* socket for rtcp */
	socktype rtcpskt;         
	
	/* IP address, network order */	
	struct in_addr addr;   
	
	/* Port number, network order */   
	u_int16 port;
	
	/* ttl, userful in multicast only */         
	u_int8 ttl;
	
	/* next address in list */
	struct address_info *next;

} address_info_t;


/* the list of ssrc of members we need to send SDES rtcp packet */
typedef struct send_sdes {
	
	u_int32 	ssrc;
	struct send_sdes *next;
	
} send_sdes_list_t;


/* the list of ssrc we add into report blocks while sending rtcp packet*/
typedef struct send_rb {

	u_int32		ssrc;
	struct send_rb	*next;

} send_rb_list_t;


/* the info. for RTCP Packet Send and Receive Rules */
typedef struct {
	
	/* the last time an RTCP packet was transmitted */
	struct timeval last_rtcp_send;

	/* the next scheduled transmission time of an RTCP packet */
	struct timeval next_rtcp_send;
	
	/* the estimated number of session members at the time tn was last recomputed */
	int pmembers;

	/* the most current estimate for the number of session members */
	int members;

	/* the most current estimate for the number of senders in the session */
	int senders;
	
	/* The target RTCP bandwidth, in octets per second.
	 * i.e., the total bandwidth that will be used for RTCP packets by all members of this session. 
	 * This will be a specified fraction of the "session bandwidth" parameter supplied to the application at startup.
	 */
	double rtcp_bw;
	
	/* Flag that is true if the application has sent data since the 2nd previous RTCP report was transmitted.
	 */
	int we_sent;
	
	/* The average compound RTCP packet size, in octets, over all RTCP packets sent and received by this participant. 
	 * The size includes lower-layer transport and network protocol headers.
	 */
	double avg_rtcp_size;
	
	/* Flag that is true if the application has not yet sent an RTCP packet. */
	int initial;

} rtcp_rule_t;


/*
 * rtp_session - include the variables relative to rtp/rtcp session.
 */
typedef struct {
	
/* below is the info. in this rtp session */
	
	/* 
	 * A flag that is set true when the rtp session is enabled.
	 * if flag is FALSE, we neither send nor receive rtp packet. 
	 */
	int rtpopen;

	/*
	 * There are some types of rtp status: SENDONLY, RECVONLY, SENDRECV.
	 * SENDONLY : send rtp packet only.
	 * RECVONLY : recv rtp packet only.
	 * SENDRECV : send && recv rtp packet.
	 * the default value is SENDRECV.
	 */
	int rtpstatus;
	
	/* 
	 * A flag that is set true when rtcp is enabled.
	 * If flag is FALSE, we only receive rtcp packet only without sending rtcp packet.
	 */
	int rtcpopen;
		
	/* A flag that is set true when the connection is opened */
	int connecting; 

	/* The session identity, which is the index of the " session_list " */
	session_id sid;
	
	/* The list of address_info_t to send to */
	address_info_t *sendlist;

	/* the local address_info_t to listen to */
	address_info_t *recvlist;

	/* sending in multicast	- The local source addresses for RTP and RTCP 
	 * sending in unicast	- AF_UNSPEC address family for loopback detection.
	 */
	struct sockaddr_in rtp_sourceaddr;
	struct sockaddr_in rtcp_sourceaddr;

	/* the info. about current rtp packet we send in the session */
	/* this is alsi the local sender info. */
	rtp_pkt_t rtp_sendpkt;

	/* last time we send rtp packet */
	struct timeval last_send_rtp_time;

	/* rtp send_packets counts */
	u_int32 send_rtppkt_num;

	/* rtp send_bytes counts */
	u_int32 send_rtpbytes_num;

/* below is information about rtcp */
	
	/* rtcp send_packet counts */
	u_int32 send_rtcppkt_num;
	
	/* rtcp send_bytes counts */
	u_int32 send_rtcpbytes_num;
	
	/* bandwidth for the session */
	int session_bw;

	/* fraction of bandwidth for RTCP */
	float rtcp_bw_fraction;

	/* the fraction of (RTCP) bandwidth of senders */
	float sender_bw_fraction;

	/* The counter for sdes computes sending intervals */
	int number_of_rtcp_send;
	
	/* information for rtcp packet send and receive rule */
	rtcp_rule_t rtcp_rule;

/* below is information about member in this rtp session */

	/* CSRC contributor */
	csrc_t *csrclist;
			
	/* the members in this rtp session */
	member_t **member_table;

	/* the conflict list of this session */
	conflict_list_t *conflict_list;
	
	/* log occurrence of collisions */
	conflict_list_t	*collsion_log;
	
	/* bye reason */
	char *bye_reason;
	
	/* rtp_timerate of codec that we are using */
	int rtp_timerate;
	
	/* The list of ssrc which we need to add their SDES items into RTCP packet */
	send_sdes_list_t *send_sdes_list;
	
	/* The circular list of ssec we add into report blocks while sending 
	 * RTCP packet.	
	 * If there are too many active sources for the reports to fit in the 
	 * MTU, then a subset of the sources should be selected round-robin 
	 * over multiple intervals, we can done by using circular list. */
	send_rb_list_t	*send_rb_list;

	/* sender_timeout is the timeout os senders in session. */
	struct timeval last_timeout, sender_timeout;
	
} session;


/* 
 * RTP_time_information which we put into schedule
 */
typedef enum {
	
	EVENT_REPORT,
	EVENT_BYE_COLLIDE,
	EVENT_BYE_NOW,
	EVENT_BYE,
	EVENT_UNKNOWN
} event_type;


/* 
 * RTPeventqueue - which stores RTPevent in order with time
 */
typedef struct RTPeq {
	
	session_id sid;
	
	/* the time to execute event */
	struct timeval timeout;
	
	/* event that on_expire will execute */
	event_type type;
	
	struct RTPeq *next;
	
} rtp_eq;


/********************************************************************/


void init_random_seed();

u_int32 random32(int type);

struct timeval double_to_timeval(double time);

double timeval_to_double(struct timeval time);

struct timeval add_timeval(struct timeval *time, struct timeval *addtime);

int time_expire(struct timeval *t, struct timeval *now);

ntp_t timeval_to_ntp(struct timeval t);

rtperror validate_session(session_id sid);

rtperror get_session(session_id sid, session **rs);

rtperror init_rtp_session(session_id *sid, session *rs);

void free_session(session_id sid);

u_int32 timeval_to_rtptime(struct timeval *t, int pt, int rtp_timerate);


/********************************************************************/


#endif /* _RTP_API_H */ 

