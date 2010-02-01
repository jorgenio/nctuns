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


#include <time.h>		// clock_t for linux
#include <sys/time.h>		// clock_t for linux
#include <sys/utsname.h>	// utsname
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>		// srand48()
#include <stdio.h>

#include "rtp_api.h"
#include "global.h" 		/* from RFC 1321 */
#include "md5.h"    		/* from RFC 1321 */
#include "rtp_avp.h"

/********************************************************************/


/* for MD5 */
#define MD_CTX 	 MD5_CTX
#define MDInit 	 MD5Init
#define MDUpdate MD5Update
#define MDFinal  MD5Final


/* The process first time use the librtp */
static int firstopen_rtp = TRUE;

/* The list of session for users to operate the particular sessions */
session	**session_list;


/********************************************************************/


/* 
 * validate_session - Test and verify this session is valid or not.
 */
rtperror 
validate_session(session_id sid){
	
	if ( (sid >= RTP_MAX_NUM_SESSION) || (!session_list[sid]) ) {// NULL
		return RTP_UNKNOWN_SESSION;
	}
	
	return 0;	
	
} /* validate_session */


/* 
 * get_session - return the rtp session "rs" from session_list[sid] 
 * if sid is valid, else returns error.
 */
rtperror 
get_session(session_id sid, session **rs){
	
	rtperror err = 0;

	err = validate_session(sid);
    	if (err > 0) {
    		return err;
	}
	
	*rs = session_list[sid];
	
	return err;
	
} /* get_session */


/*
 * init_random_seed - Initialize random number generators with a seed. 
 */
void 
init_random_seed(){
	
	struct timeval cur;
	
	gettimeofday(&cur, NULL);
	srand48((int) cur.tv_usec);
	
} /* init_random_seed */


/* 
 * md_32 - Random generate SSRC which specified in RFC 3550.
 */
static u_long 
md_32(unsigned char *string, int length){
	
	MD_CTX context;
	union {
		char 	c[16];
		u_long 	x[4];
	} digest;

	u_long r;
	int i;
	MDInit (&context);
	MDUpdate (&context, string, length);
	MDFinal ((unsigned char *)&digest, &context);
	r = 0;
	for (i = 0; i < 3; i++) {
		r ^= digest.x[i];
	}
	
	return r;
	
} /* md_32 */


/* 
 * random32 - Return random unsigned 32-bit quantity. Use 'type' argument if
 * 	      you need to generate several different values in close succession.
 */
u_int32 
random32(int type){
	
	struct {
		int 		type;
		struct 	timeval tv;
		clock_t 	cpu;
		pid_t 		pid;
		u_long 		hid;
		uid_t 		uid;
		gid_t 		gid;
		struct utsname 	name;

	} s;
	gettimeofday(&s.tv, 0);
	uname(&s.name);
	s.type	= type;
	s.cpu	= clock();
	s.pid	= getpid();
//	because all hosts in simulator are on the same machine, so we change to get pid.
	s.hid	= (long)getpid();
//	s.hid	= gethostid();
	s.uid	= getuid();
	s.gid	= getgid();

	return md_32((unsigned char *)&s, sizeof(s));
	
} /* random32 */


/* 
 * double_to_timeval - convert the time form from double to timeval struct.
 */
struct timeval 
double_to_timeval(double time){
	
	struct timeval	t;
	long		long_time;
	
	long_time	= (long) time;
	t.tv_sec	= long_time;
	t.tv_usec	= (long) (1000000 * (time - long_time));

	//t.tv_usec = ((long) (time * 10000000.) + 1 ) / 10;
	/* Below three lines are for running on NCTUns only!... */
	if (!t.tv_sec && !t.tv_usec) {	// i.e. t = 0.0
		t.tv_usec++;
	}
	
	return t;
	
} /* double_to_timeval */


/*
 * timeval_to_double - convert the time form from timeval struct to double.
 */
double
timeval_to_double(struct timeval time){

	double	t;
	long	sec, usec;
	
	sec	= time.tv_sec;
	usec	= time.tv_usec;

	t = sec + ( (double) usec / 1000000. );
		
	return t;
	
} /* timeval_to_double */


/* 
 * add_timeval - return a new time which equal to ( time + addtime ) in timeval structure.
 */ 
struct timeval 
add_timeval(struct timeval *time, struct timeval *addtime){
	
	struct timeval newtime;
	
	newtime.tv_sec	= time->tv_sec + addtime->tv_sec;
	newtime.tv_usec	= time->tv_usec + addtime->tv_usec;
	
	/* check tv_usec if it is overflow */
	if (newtime.tv_usec >= 1000000) {
		newtime.tv_sec++;
		newtime.tv_usec -= 1000000;
	}
	
	return newtime;
	
} /* add_timeval */


/* 
 * time_expire - returns TRUE if time of "t" is before the time of "now". 
 */
int 
time_expire(struct timeval *t, struct timeval *now){
	
	if (t->tv_sec > now->tv_sec) {
 		return FALSE;
	}
	else if (t->tv_sec < now->tv_sec) {
		return TRUE;
	}
	else if (t->tv_usec > now->tv_usec) {
		return FALSE;
	}
	else {
		return TRUE;
	}

} /* time_expire */


/*
 * timeval_to_ntp - convert time form from timeval to ntp.
 */
ntp_t
timeval_to_ntp(struct timeval t){

	ntp_t ntp;

	ntp.sec	= t.tv_sec + NTP_OFFSET;
	ntp.frac= (t.tv_usec << 12) + (t.tv_usec << 8) - ((t.tv_usec * 1825) >> 5);

	return ntp;

} /* timeval_to_ntp */


/* 
 * init_rtp_session - call by rtp_reate(), session creation && initialization.
 * 
 * If it is called first time, it will create "RTP_MAX_NUM_SESSION" 
 * (defined in rtp_config.h) number of sessions for session_list,
 * the session_list is controled by each process which is using librtp,
 * otherwise, it only malloc a session for session_list[s].
 */
rtperror 
init_rtp_session(session_id *sid, session *rs){
	
	rtperror	err = 0;
	session_id	new_id;	/* get new identify for this session */	
	
	if (firstopen_rtp) {
		
		/* init_rtp_create() is called first time */
		firstopen_rtp = FALSE;
		
		/* allocate session_list in size of " RTP_MAX_NUM_SESSION " into memory */
		session_list = (session **) calloc(RTP_MAX_NUM_SESSION, sizeof(session *));

		/* session_list initialization */
		for (new_id = 0; new_id < RTP_MAX_NUM_SESSION; new_id++) {
			session_list[new_id] = NULL;
		}
		
		new_id = 0;
	}
	else { 
		/* there maybe a NULL session_list[index] that we can reuse */
		for (new_id = 0; new_id < RTP_MAX_NUM_SESSION; new_id++) {
			/* find the free session */
			if (!session_list[new_id]) { // NULL
				break;
			}
		}
	}

	session_list[new_id] = rs;
	
	if ( (new_id >= RTP_MAX_NUM_SESSION) || (!session_list[new_id]) ) { // no free session or NULL
		return SESSION_OUT_OF_RANGE;
	}
	
	*sid = new_id;
	
/* initialization of this session */

#if RTP_DEBUG	
	printf("MSG	: init_rtp_session - get session from session_list[%d]=%p\n", new_id, session_list[new_id]); //DEBUG
#endif
	
/* session info. */
	session_list[new_id]->rtpopen 	= TRUE;
	session_list[new_id]->rtpstatus = SENDRECV;	// this should be set by SDP in future.
	session_list[new_id]->rtcpopen 	= TRUE;
	session_list[new_id]->connecting= FALSE;
	session_list[new_id]->sid	= new_id;
	
	session_list[new_id]->sendlist 	= NULL;
	session_list[new_id]->recvlist 	= NULL;
	
	session_list[new_id]->rtp_sourceaddr.sin_family	= AF_UNSPEC;
	session_list[new_id]->rtcp_sourceaddr.sin_family= AF_UNSPEC;

	session_list[new_id]->last_send_rtp_time.tv_sec	= 0;
	session_list[new_id]->last_send_rtp_time.tv_usec= 0;
	
/* transmission info. about this session */
	session_list[new_id]->send_rtppkt_num    = 0;
	session_list[new_id]->send_rtpbytes_num  = 0;
	session_list[new_id]->send_rtcppkt_num   = 0;
	session_list[new_id]->send_rtcpbytes_num = 0;
	
/* initialization when local info. */
	
	/* current local info. */
	session_list[new_id]->rtp_sendpkt.rtphdr.version	= RTP_VERSION;
	session_list[new_id]->rtp_sendpkt.rtphdr.p		= 0;
	session_list[new_id]->rtp_sendpkt.rtphdr.x		= 0;
	session_list[new_id]->rtp_sendpkt.rtphdr.cc 		= 0;
	session_list[new_id]->rtp_sendpkt.rtphdr.m		= 0;
	session_list[new_id]->rtp_sendpkt.rtphdr.pt 		= 0;

	/* Initialize random number generators with a random seed. */
	init_random_seed();
	
	session_list[new_id]->rtp_sendpkt.rtphdr.ssrc	= random32((int) sid);
	session_list[new_id]->rtp_sendpkt.rtphdr.ts	= random32((int) sid);
	session_list[new_id]->rtp_sendpkt.rtphdr.seq	= random32((int) sid) % 65536;
	
#if RTP_DEBUG	
	printf("MSG	: init_rtp_session - session ssrc = %u, timestamp = %u, seqnum = %u\n", rs->rtp_sendpkt.rtphdr.ssrc, rs->rtp_sendpkt.rtphdr.ts, rs->rtp_sendpkt.rtphdr.seq); //DEBUG_ONLY
	
	fflush(stdout);	
#endif

/* build local rtcp infomation */

	rs->session_bw		= RTP_DEFAULT_SESSION_BW;
	rs->rtcp_bw_fraction	= RTCP_BW_FRACTION;
	rs->sender_bw_fraction	= RTCP_SENDER_BW_FRACTION;
	
	/* for sdes, initial value = 1 because we send SDES item decided by 
	 * [ item intervals mod number_of_rtcp_send ] */
	rs->number_of_rtcp_send = 1;
	
	/* The average RTCP size is initialized to 128 octets which is 
	 * conservative (it assumes everyone else is generating SRs 
	 * instead of RRs: 20 IP + 8 UDP + 52 SR + 48 SDES CNAME). */
	rs->rtcp_rule.avg_rtcp_size	= 128;
	rs->rtcp_rule.pmembers		= 1;
	rs->rtcp_rule.members		= 1;
	rs->rtcp_rule.senders		= 0;
	rs->rtcp_rule.initial		= TRUE;
	rs->rtcp_rule.we_sent		= FALSE;
	rs->rtcp_rule.rtcp_bw		= (rs->session_bw * rs->rtcp_bw_fraction);
	
	rs->csrclist		= NULL;	
	rs->conflict_list	= NULL;
	rs->collsion_log	= NULL;
	rs->bye_reason		= NULL;
	rs->rtp_timerate	= 0;
	rs->send_sdes_list	= NULL;
	
	return err;	
	
} /* init_rtp_session */ 


/* free_session */
void
free_session(session_id sid){
	session_list[sid] = NULL;
} /* free_session */


/* 
 * timeval_to_rtptime - transfer current into rtp timestamp.
 */
u_int32 
timeval_to_rtptime(struct timeval *t, int pt, int rtp_timerate){
	
	//(u_int32) time = (u_int32) (t->tv_sec) * 1000000 + (u_int32) (t->tv_usec);
	u_int32 time = (t->tv_sec) * 1000000 + (t->tv_usec);
	
	if (rtp_timerate) {
		/* user specified first */
		//return ( (u_int32) ( time / rtp_timerate ) );
		return ( time / rtp_timerate );
	}
	else {
		if (RTP_TIMERATE[pt]) {
			//return ( (u_int32) ( time / RTP_TIMERATE[pt] ) );
			return ( time / RTP_TIMERATE[pt] );
		}
		else {
			fprintf(stderr, "UNKNOWN RTP TIMERATE!!, we assume rtp timeeate is 125\n");
			return 125;
		}	
	}
} /* timeval_to_rtptime */

