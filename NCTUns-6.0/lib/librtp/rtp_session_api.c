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
 * The file contains RTP APIs for session controling.
 */


/********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rtp_member_api.h"
#include "rtp_session_api.h"
#include "rtp_network_api.h"


/********************************************************************/


/* the queue we can put scheduled event into it.*/
extern rtp_eq *rtpeq;


/********************************************************************/


/* 
 * rtp_create - create a rtp session. 
 *
 * the function won't open any socket.
 * the function assign an unique value "sid" (session identity) to each session.
 * the function setup some basic values, but some value still should be setup by user.
 */
rtperror 
rtp_create(session_id *sid){
	
	session		*rs;
	rtperror	err = 0;
	
	rs = (session *) malloc(sizeof(session));
	if (!rs){
		/* rs == NULL */
		return RTP_CANT_ALLOC_MEM;
	}
	
	/* add a RTP session "rs" into session_list and initialization for user to control */
	err = init_rtp_session(sid, rs);
	if (err > 0) {
		return err;
	}

	/* construct the member table */
	err = init_member(*sid);
	if (err > 0) {
		return err;
	}

	/* add local member into the last position of memberlist, local member is always valid. */
	err = add_new_member((*sid), rs->rtp_sendpkt.rtphdr.ssrc, NULL, valid, FALSE, FALSE);
	if (err > 0) {
		return err;
	}
	
	return err;
		
} /* rtp_create */


/* 
 * rtp_delete - Destroy the session, also close the connection if it is still connecting. 
 */
rtperror 
rtp_delete(session_id sid){
	
	rtperror	err = 0;
	session 	*rs;
	member_t	*member, *tmp;
	int		i, j;
	conflict_list_t	*conflict, *c;
	address_info_t 	*temp, *next;	/* remove the sender list if it exists */	
	csrc_t		*csrc, *remove;
	
	/* get the RTPsession of RTPsessionlist[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	/* if it still in connection, close the connection */
	if (rs->connecting) {		
		err = rtp_close_connection(sid, "connection is colsed by rtp_delete()");
		if (err > 0) {
			return err;
		}
	}
	
	/* remove the receiver list if it exists */
	if (rs->recvlist) { // not NULL
		free(rs->recvlist);
		rs->recvlist = NULL;
	}
	
	/* remove the sender list if it exists */
	temp = rs->sendlist;	
	
	while (temp) { // not NULL
		next = temp;
		temp = temp->next;
		free(next);	
		next = NULL;
	}
	
	/* free all members in the session */
	for (i = 0; i < HASH_SIZE; i++) {
		
		tmp = rs->member_table[i];
		if (tmp) {
			member = tmp;
			tmp = tmp->next;
			
			/* free sdes info. */
			for (j = 0; j < 9; j ++) {
				if (member->sdes.item[j]) {
					free(member->sdes.item[j]);
					member->sdes.item[j] = NULL;
				}
			}
			
			free(member);	
		}
	}
	
	csrc = rs->csrclist;
	while (csrc) {
		remove = csrc;
		csrc = csrc->next;
		free(remove);
	}
				
	conflict = rs->conflict_list;
	if (conflict) {
		c = conflict;
		conflict = conflict->next;
		free(c);
		c = NULL;
	}	
	
	conflict = rs->collsion_log;
	if (conflict) {
		c = conflict;
		conflict = conflict->next;
		free(c);
		c = NULL;
	}	
		
	if (rs->bye_reason) {
		free(rs->bye_reason);
		rs->bye_reason = NULL;
	}
	
	rs->conflict_list = NULL;
	
	if (rs->send_sdes_list) {
		send_sdes_list_t *sdes, *remove;
		sdes = rs->send_sdes_list;
		while (sdes) {
			remove = sdes;
			sdes = sdes->next;
			free(remove);
		}
	}
	
	if (rs->send_rb_list) {
		send_rb_list_t *rb, *remove;
		rb = rs->send_rb_list;
		while (rb) {
			remove = rb;
			rb = rb->next;
			free(remove);
			remove = NULL;
			if (rb == rs->send_rb_list) {
				break;
			}
		}
	}

	/* free session */
	free(rs);
	free_session(sid);

#if RTP_DEBUG	
	printf("MSG	: rtp_delete - success\n");
#endif

	return err;	
	
} /* rtp_delete */


/* 
 * build_rtp_pkt - build rtp header
 *
 * now, default is no encryption, no extension, no padding
 * marker	: marker bit .
 * addts	: the increment of timestamp.
 * pt		: the payload type idenfitier.
 * buf		: thr pointer points to the buffer [ &sendbuf ].
 */
rtperror 
build_rtp_pkt(session_id sid, int8 marker, int16 pt, char *payload, int *rtplen, int plen, char *sendbuf){

	int 		rtphdr_len;
	session		*rs;
	char		*ptr;
	rtperror	err = 0;
        int		cc;
        csrc_t		*csrc;
        
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

/* below we handle the rtp header part */
	
	/* check CSRC count shoule be range in 0~15 */
	if ((rs->rtp_sendpkt.rtphdr.cc) > RTP_MAX_CC) {
		return CSRC_COUNT_EXCEED;
	}
	
	/* the rtp header length = rtp_hdr_t - CSRC[0] + CSRC_counts */
	rtphdr_len = (sizeof(rtp_hdr_t)) + (sizeof(u_int32) * rs->rtp_sendpkt.rtphdr.cc);
	
	/* if u set the new value of CSRC count, note that do not overflow the buffer size. */
	if (rtphdr_len > RTP_MAX_PKT_SIZE) {
		return RTP_PKT_OUT_OF_RANGE;
	}
	
	/* 這裡要 offset 長度為 rtphdr_len 給 rtp header 用 */
	*rtplen = rtphdr_len;
	
	/* now, we fill in fields of rtp header */
	ptr = sendbuf;

	/* no encrypt, no padding, no extension. */
	
	rs->rtp_sendpkt.rtphdr.m	= marker;
	rs->rtp_sendpkt.rtphdr.pt	= pt;
	
	memcpy(ptr, &(rs->rtp_sendpkt.rtphdr), rtphdr_len);
	
	/* move pointer to the end of ssrc */
	ptr += rtphdr_len;
	
	/* if CSRC count > 0, build csrc in rtp header */
	cc = rs->rtp_sendpkt.rtphdr.cc;
	csrc = rs->csrclist;
	if (cc) {
		if (!csrc) {
			return CSRC_ERROR;
		}
		
		memcpy(ptr, (char *) csrc, sizeof(u_int32));
		ptr += sizeof(u_int32);
		
		cc--;
		csrc = csrc->next;
		
#if RTP_DEBUG	
		printf("MSG	: ptr (+ csrc)in BuildRTPHeader : %p", ptr);//DEBUG_ONLY
#endif
	}				
	
	/* if user wnat to log the rtp header info of sender 
	 * we can set LOG_SEND_RTP in RTPConfig.h, default LOG_SEND_RTP is FALSE.
	 */
	if (LOG_SEND_RTP) {
		FILE	*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtp_send.log");

                fp = fopen(open, "a+");		
		if (fp != NULL) {	
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.version = %u\n", rs->rtp_sendpkt.rtphdr.version);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.p = %u\n", rs->rtp_sendpkt.rtphdr.p);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.x = %u\n", rs->rtp_sendpkt.rtphdr.x);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.cc = %u\n", rs->rtp_sendpkt.rtphdr.cc);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.m = %u\n", rs->rtp_sendpkt.rtphdr.m);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.pt = %u\n", rs->rtp_sendpkt.rtphdr.pt);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.seq = %u\n", rs->rtp_sendpkt.rtphdr.seq);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.ts = %lu\n", rs->rtp_sendpkt.rtphdr.ts);
			fprintf(fp, "MSG	: BuildRTPHeader rtphdr.ssrc = %lu\n", rs->rtp_sendpkt.rtphdr.ssrc);
			fprintf(fp, "------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "MSG	: can't open the file - %s\n", open);//DEBUG_ONLY
		}
		fclose(fp);
		free(LOC_IP);
	}

/* below we handle rtp payload part */

	/* insert payload data */
	//memset(ptr , '\0', plen);
	memcpy(ptr, payload, plen);
	
	*rtplen += plen;
	
	return err;
	
} /* build_rtp_pkt */


/* 
 * rtcp_start - Initialization and startup RTCP mechanism.
 *
 * librtp will generate rtcp packet automatically.
 */
rtperror 
rtcp_start(session_id sid){
	
	rtperror	err = 0;
	session		*rs;
	struct timeval	interval;
	
        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	/* check the rtcp transmission is permited or not */
	if (rs->session_bw && rs->rtcp_bw_fraction) {

		rs->number_of_rtcp_send		= 1;

		/* set sender timeout mechanism */
		rs->last_timeout.tv_sec		= 0;
		rs->last_timeout.tv_usec	= 0;
		rs->sender_timeout.tv_sec	= 0;
		rs->sender_timeout.tv_usec	= 0;

		/* get the rtcp interval*/
		err =  rtcp_interval(sid, &interval);	
		if (err > 0) {
			return err;
		}
		
		/* first time, we get now simply. */
		gettimeofday(&(rs->rtcp_rule.last_rtcp_send), NULL);
	
		/* set next time to send rtcp */
		rs->rtcp_rule.next_rtcp_send = add_timeval(&(rs->rtcp_rule.last_rtcp_send), &interval);

		/* schedule next recp packet to send */
		rtcp_schedule(sid, EVENT_REPORT, &(rs->rtcp_rule.next_rtcp_send));
		
		rs->rtcp_rule.initial = FALSE;
	}
	else {
		return NO_RTCP_WARNING;
	}

	rs->rtcpopen = TRUE;
	
	return err;	
	
} /* rtcp_start */


/* 
 * rtcp_stop - stop send other rtcp packet, and send a BYE rtcp packet. 
 */
rtperror 
rtcp_stop(session_id sid, char *reason){

	rtperror	err = 0;
	session		*rs;
	struct timeval	now;
        rtp_eq		*eq, *tmp, *previous;
        
        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}

	gettimeofday(&now, NULL);
	
	if (reason) {
		rs->bye_reason = (char *) malloc(sizeof(char) * (strlen(reason) + 1) );
		strcpy(rs->bye_reason, reason);
		rs->bye_reason[strlen(reason)] = '\0';
	}
	
	eq = rtpeq;
	previous = NULL;
	
	/* free all event in queue. */
	while (eq) {
		tmp = eq;
		eq = eq->next;
		if (tmp->sid == sid) {
			/* free this event */
			if (previous) {
				previous->next = eq;
				free(tmp);
				tmp = NULL;
			}
			else {
				rtpeq = eq;
				free(tmp);
				tmp = NULL;
			}
		}
		else {
			previous = tmp;
		}
	}
	
	err = rtcp_schedule(sid, EVENT_BYE, &now);
	if (err > 0) {
		return err;
	}
	
	return err;
		
} /* rtcp_stop */


/* 
 * build_rtcp_pkt - Build RTCP Header
 *
 * If there are too many sources to fit all the necessary RR packets into one compound RTCP packet without
 * exceeding the maximum transmission unit (MTU) of the network path, then only the subset that
 * will fit into one MTU should be included in each interval. The subsets should be selected
 * round-robin across multiple intervals so that all sources are reported. by RFC 3550.
 */
rtperror 
build_rtcp_pkt(session_id sid, char *buf, int *rtcplen, event_type type){
	
	rtperror	err = 0;
	session		*rs;
	char		*ptr;
	int		remainder = RTCP_MAX_PKT_SIZE;
	struct timeval	now;
	double		current;
	send_rb_list_t	*original_rb;

        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	original_rb = rs->send_rb_list;

	gettimeofday(&now, NULL);	
	current = timeval_to_double(now);
	
#if RTP_DEBUG	
	printf("MSG	: build_rtcp_pkt - now = %lf\n", current);
#endif
	
	/* Build rtcp packet  */
	if (type == EVENT_BYE) {
		
		int bye_len;
		int sdes_len;
		
#if RTP_DEBUG	
		printf("MSG	: build_rtcp_pkt() - start BYE~\n");
#endif
		
		bye_len = get_bye_len(rs);
		remainder -= bye_len;
		sdes_len = get_sdes_len(rs);
		remainder -= sdes_len;
		if (remainder < 0) {
			return BUFFER_NOT_ENOUGH;
		}
	
		*rtcplen = 0;			
		ptr = buf;

		if (rs->rtcp_rule.we_sent) {
			
			err = build_sr(sid, &ptr, rtcplen, original_rb, &remainder, &now);

			while ( (rs->send_rb_list != original_rb) && (rs->send_rb_list->next != original_rb)
				&& (err == TOO_MANY_REPORT_BLOCK) ) {	// add other blocks in additional RRs with round-robin mechanism.
				err = build_rr(sid, &ptr, rtcplen, original_rb, &remainder, &now);
			}
			if (err == BUFFER_NOT_ENOUGH_LENGTH) {
				fprintf(stderr, "Switch on Round-Robin for RTCP.\n");
			}
			else if (err > 0) {
				return err;
			}
		}
		else {
			err = build_rr(sid, &ptr, rtcplen, original_rb, &remainder, &now);

			while ( (rs->send_rb_list != original_rb) && (rs->send_rb_list->next != original_rb)
				&& (err == TOO_MANY_REPORT_BLOCK) ) {	// add other blocks in additional RRs with round-robin mechanism.
				err = build_rr(sid, &ptr, rtcplen, original_rb, &remainder, &now);
			}
			if (err == BUFFER_NOT_ENOUGH_LENGTH) {
				fprintf(stderr, "Switch on Round-Robin for RTCP.\n");
			}
			else if (err > 0) {
				return err;
			}
		}
		
		/* build SDES Packet */
		err = build_sdes(sid, &ptr, rtcplen);
		if (err > 0) {
			return err;
		}
	
		err = build_bye(sid, &ptr, rtcplen, &remainder);
		if (err > 0) {
			return err;
		}
	}
	else if (type == EVENT_REPORT) {
		
		int sdes_len;
		
#if RTP_DEBUG	
		printf("MSG	: build_rtcp_pkt() - start RTCP~\n");
#endif	
		sdes_len = get_sdes_len(rs);
		remainder -= sdes_len;
		if (remainder < 0) {
			return BUFFER_NOT_ENOUGH;
		}
				
		*rtcplen = 0;
		
		ptr = buf;
	
		if (rs->rtcp_rule.we_sent) {
			
			err = build_sr(sid, &ptr, rtcplen, original_rb, &remainder, &now);

			while ( (rs->send_rb_list != original_rb) && (rs->send_rb_list->next != original_rb)
				&& (err == TOO_MANY_REPORT_BLOCK) ) {	// add other blocks in additional RRs with round-robin mechanism.
				err = build_rr(sid, &ptr, rtcplen, original_rb, &remainder, &now);
			}
			if (err == BUFFER_NOT_ENOUGH_LENGTH) {
				fprintf(stderr, "Switch on Round-Robin for RTCP.\n");
			}
			else if (err > 0) {
				return err;
			}
		}
		else {
			err = build_rr(sid, &ptr, rtcplen, original_rb, &remainder, &now);

			while ( (rs->send_rb_list != original_rb) && (rs->send_rb_list->next != original_rb)
				&& (err == TOO_MANY_REPORT_BLOCK) ) {	// add other blocks in additional RRs with round-robin mechanism.
				err = build_rr(sid, &ptr, rtcplen, original_rb, &remainder, &now);
			}
			if (err == BUFFER_NOT_ENOUGH_LENGTH) {
				fprintf(stderr, "Switch on Round-Robin for RTCP.\n");
			}
			else if (err > 0) {
				return err;
			}
		}
		
		/* build SDES Packet */
		err = build_sdes(sid, &ptr, rtcplen);
		if (err > 0) {
			return err;
		}
		
		rs->rtcp_rule.avg_rtcp_size = (1./16.) * (*rtcplen) + (15./16.) * rs->rtcp_rule.avg_rtcp_size;
		
#if RTP_DEBUG	
		printf("MSG	: build_rtcp_pkt - rs->rtcp_rule.avg_rtcp_size = %lf, *rtcplen = %d\n", rs->rtcp_rule.avg_rtcp_size, *rtcplen);//DEBUG
#endif
		
	} // event_type == EVENT_REPORT or EVENT_BYE
	else {
		return UNKNOWN_EVENT_TYPE;
	}
	
#if RTP_DEBUG	
	printf("MSG	: build_rtcp_pkt - the rtcp first 16-bit : %0x, v = %u, p = %u, count = %u, pt = %u, length = %u\n", *(u_int16 *)buf, 
	rtcp_common->version, rtcp_common->p, rtcp_common->count, rtcp_common->pt, rtcp_common->length);//DEBUG 
#endif
	
	return err;
	
} /* build_rtcp_pkt */


/* 
 * rtp_set_session_bw - set session bandwidth for application.
 *
 * Note, " session_bw " parameter unit we get is K-bit/sec(in SDP, RFC 2327), 
 * we transfer unit to bytes/second for rtcp_bw(in RTP, RFC 3550).
 */
rtperror 
rtp_set_session_bw(session_id sid, double session_bw) {
	
	rtperror	err = 0;
	session		*rs;

        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	rs->session_bw = ( ( (int) session_bw / 8. ) * 1000);
	rs->rtcp_rule.rtcp_bw = (rs->session_bw * rs->rtcp_bw_fraction);
#if RTP_DEBUG	
	printf("rs->rtcp_rule.rtcp_bw = %lf, session_bw = %lf, rs->session_bw = %d\n", rs->rtcp_rule.rtcp_bw, session_bw, rs->session_bw);//DEBUG
#endif
	
	return err;
	
} /* rtp_set_session_bw */


/*
 * rtp_set_sdes_item - set context of SDES item.
 */
rtperror 
rtp_set_sdes_item(session_id sid, int sdes_type, char *data, int interval){
	
	rtperror	err = 0;
	member_t	*member = NULL;
	session		*rs;
	
	if (!data) {
		return NULL_WARNING;
	}

	if (sdes_type > RTCP_SDES_PRIV || sdes_type < RTCP_SDES_CNAME) {
		return UNKNOWN_SDES_TYPE;
	}
	
	/* Note that the text can be no longer than 255 octets, but this is consistent with the need 
	 * to limit RTCP bandwidth consumption. */
	if (strlen(data) + 1 > RTP_MAX_SDES ) {
		return OVER_MAX_SDES;
	}
	
        /* get the the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	err = get_member_by_ssrc(rs, rs->rtp_sendpkt.rtphdr.ssrc, &member);
	if (!member) {
		return NO_LOCAL_MEMBER;
	}
		
	if (member->sdes.length[sdes_type]) {
		/* change item info. */
		free(member->sdes.item[sdes_type]);
	}
	member->sdes.length[sdes_type] = strlen(data) + 1;
	
	member->sdes.item[sdes_type] = (char *) malloc( sizeof(char) * (strlen(data) + 1) );
	strcpy(member->sdes.item[sdes_type], data);
	member->sdes.item[sdes_type][strlen(data)] = '\0';
	
	member->sdes.intervals[sdes_type] = interval;

#if RTP_DEBUG	
	printf("MSG	: rtp_set_sdes_item - = %s with interval %d.\n", member->sdes.item[sdes_type], member->sdes.intervals[sdes_type]);//DEBUG_ONLY
#endif
	
	return err;
	
} /* rtp_set_sdes_item */

	
/*
 * type_of_event - return the next type of rtcp pakcet we will send at tn
 */
event_type
type_of_event(session_id sid){
	
	rtp_eq *tmp;
	tmp = rtpeq;
	
	while (tmp) {
		if (tmp->sid != sid) {
			tmp = tmp->next;
		}
		else {
#if RTP_DEBUG	
			printf("MSG	: type_of_event - type is [%d]\n", tmp->type);
#endif

			return tmp->type;
		}
	}
	
#if RTP_DEBUG	
	printf("MSG	: type_of_event - type is UNKNOWN!!\n");
#endif

	return EVENT_UNKNOWN;
	
} /* type_of_event */


/* 
 * receive_rtp_pkt - Process when we revceiving a RTP Packet.
 */
rtperror 
receive_rtp_pkt(session_id sid, char *buf, int len, struct sockaddr *from, socklen_t *formlen){
	
	rtperror 	err = 0;
	session		*rs;
	rtp_pkt_t	rtppkt;
	int		rtphdr_len;
	member_t	*member;
	char		*ptr;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	memcpy(&(rtppkt.rtphdr), buf, sizeof(rtp_hdr_t));
	
	/* A.1 RTP Data Header Validity Checks */
	err = rtp_hdr_validity_check(sid, &(rtppkt.rtphdr));
	if (err > 0) {
		return err;
	}

#if RTP_DEBUG	
	printf("MSG	: receive_rtp_pkt - we get a rtp packet whose ssrc = %u!!\n", rtppkt.rtphdr.ssrc);//DEBUG
#endif
	
	/* algorithm specified in section 8.2 of RFC-3550 */
	err = rtp_collision_resolution_loop_detection(sid, rtppkt, from);
	if ( err && (err != LOCAL_SOURCE_NEW_CONFLICT) ) {
		/* if there is new collision, we still handle incoming process.	*/
		/* else we abort processing of data packet or control element	*/
		return err;
	}
	
	/* the rtp packet we receive is ok, I guess... */
	ptr = buf;

	err = get_member_by_ssrc(rs, rtppkt.rtphdr.ssrc, &member);
	if (err) {
		fprintf(stderr, "receive_rtp_pkt - %s\n", RTPStrError(err));
	}
	
	//memset(&(member->rtp_recvpkt.rtphdr), '\0', sizeof(rtp_hdr_t));
	//memcpy(&(member->rtp_recvpkt.rtphdr), ptr, sizeof(rtp_hdr_t));
	//member->rtp_recvpkt.rtphdr = (rtp_hdr_t *) ptr;
	
	rtphdr_len = (sizeof(rtp_hdr_t)) + (sizeof(u_int32) * rtppkt.rtphdr.cc);
	
	//rs->rtp_recvpkt = &rtppkt;
	//memcpy(rs->rtp_recvpkt.rtphdr, rtppkt.rtphdr, rtphdr_len);
	
	ptr += rtphdr_len;
	
	member->rtp_recvpkt.plen = len - rtphdr_len;

	//memset(&(member->rtp_recvpkt.pdata), '\0', MAX_PAYLOAD_LENGTH);
	memcpy(&(member->rtp_recvpkt.pdata), ptr, member->rtp_recvpkt.plen);
	//rs->rtp_recvpkt.pdata = ptr;
		
	/* no extension, no padding, no encryption */
	//memcpy(rs->rtp_recvpkt.pdata, ptr, (len - rtphdr_len));

	//rs->rtp_recvpkt.plen = len + ((int)(rs->rtp_sendpkt->rtphdr) - (int)(rs->rtp_sendpkt->pdata));

	/* if user want to log the rtp header info of receiver
	 * we can set LOG_RECV_RTP in RTPConfig.h
	 * default value is TRUE
	 */
	if (LOG_RECV_RTP) {
		FILE 		*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup( inet_ntoa ( rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtp_recv.log");

                fp = fopen(open, "a+");
		if (fp != NULL) {
			fprintf(fp, "Receive RTP packet\n");
			fprintf(fp, "RTPGetPacket version = %u\n", member->rtp_recvpkt.rtphdr.version);
			fprintf(fp, "RTPGetPacket padding = %u\n", member->rtp_recvpkt.rtphdr.p);
			fprintf(fp, "RTPGetPacket extension = %u\n", member->rtp_recvpkt.rtphdr.x);
			fprintf(fp, "RTPGetPacket cc = %u\n", member->rtp_recvpkt.rtphdr.cc);
			fprintf(fp, "RTPGetPacket m = %u\n", member->rtp_recvpkt.rtphdr.m);
			fprintf(fp, "RTPGetPacket pt = %u\n", member->rtp_recvpkt.rtphdr.pt);
			fprintf(fp, "RTPGetPacket seq = %u\n", member->rtp_recvpkt.rtphdr.seq);
			fprintf(fp, "RTPGetPacket timestamp = %lu\n", member->rtp_recvpkt.rtphdr.ts);
			fprintf(fp, "RTPGetPacket ssrc = %lu\n", member->rtp_recvpkt.rtphdr.ssrc);
			fprintf(fp, "-------------------------------------------------------\n");
			//printf("RTPGetPacket csrc[0] = %32u\n", rtppkt->rtphdr.csrc[0]);
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - recvlog\n");
		}
		fclose(fp);
		free(LOC_IP);
	}

	member->recv_rtppkt_num++;
	member->recv_rtpbytes_num += member->rtp_recvpkt.plen;
		
#if RTP_DEBUG	
	printf("MSG	: update_member_info - recv_rtppkt_num = %d, recv_rtpbytes_num = %d\n", member->recv_rtppkt_num, member->recv_rtpbytes_num);//DEBUG
#endif

	return err;
	
} /* receive_rtp_pkt */


/* 
 * receive_rtcp_pkt - Process when we revceiving a RTCP Packet.
 */
rtperror 
receive_rtcp_pkt(session_id sid, char *buf, int len, struct sockaddr *from, socklen_t *formlen){

	rtperror 	err = 0;
	session		*rs;
	char		*ptr, *end;
	rtcp_t		rtcppkt;
	int		i, tmp;
	int		is_bye = FALSE;
	struct timeval	now;
	ntp_t 		ntp;
	double		ntp_cur, cur, fraction, sec;	// only take the middle 32-bit of NTP timestamp
	long		s, f;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	gettimeofday(&now, NULL);
	
	ntp = timeval_to_ntp(now);
	
	cur = timeval_to_double(now);
	
	s = (ntp.sec & 0x0000ffff);
	
	f = ntp.frac >> 16;
	
#if RTP_DEBUG	
	printf("MSG	: receive_rtcp_pkt - s = %ld, f = %ld\n", s, f);
#endif
	
	fraction = ((double) f / 65536.);
	sec = (double) s;
	ntp_cur = sec + fraction;
	
#if RTP_DEBUG	
	printf("MSG	: receive_rtcp_pkt - ntp_cur.sec = %u, ntp_cur.frac = %u\n", ntp.sec, ntp.frac);
	printf("MSG	: receive_rtcp_pkt - cur = %lf, sec = %lf, fraction = %lf\n", cur, sec, fraction);
#endif
	
	/* A.2 RTCP Header Validity Checks */
	err = rtcp_hdr_validity_check(buf, len, &is_bye);
	if (err > 0) {
		return err;
	}
	
	if (is_bye) {	// A.7 Computing the RTCP Transmission Interval
		rs->rtcp_rule.avg_rtcp_size = (1./16.) *len + (15./16.) * (rs->rtcp_rule.avg_rtcp_size);
	}
	
	ptr = buf;
	end = buf + len;
	
	while (ptr < end) {

		rtcppkt.common = (rtcp_common_t *) ptr;

		i = 0;
		
		switch(rtcppkt.common->pt){
			
			case RTCP_SR:

				ptr += sizeof(rtcp_common_t);
					
				rtcppkt.r.sr.rtcp_sr = (rtcp_sr_t *) ptr;
				ptr += sizeof(rtcp_sr_t);
			
				tmp = rtcppkt.common->count;
				
				while (tmp) {
					rtcppkt.r.sr.rr[i] = (rtcp_rr_t *) ptr;
					ptr += sizeof(rtcp_rr_t);
					
					tmp --;
					i++;
				}
				
				err = sr_update_member_info(sid, rtcppkt, from, formlen, ntp_cur, cur);
				if (err < 0) {
					fprintf(stderr, "receive_rtcp_pkt - %s\n", RTPStrError(err));
				}	
				else if (err > 0) {
					return err;
				}
	
				break;
				
			case RTCP_RR:

				ptr += sizeof(rtcp_common_t);

				rtcppkt.r.rr.ssrc = (u_int32 *) ptr;
				ptr += sizeof(u_int32);
				
				tmp = rtcppkt.common->count;
				
				while (tmp) {
					rtcppkt.r.rr.rr[i] = (rtcp_rr_t *) ptr;
					ptr += sizeof(rtcp_rr_t);
					
					tmp --;
					i++;
				}				
				
				err = rr_update_member_info(sid, rtcppkt, from, formlen, ntp_cur);
				if (err < 0) {
					fprintf(stderr, "receive_rtcp_pkt - %s\n", RTPStrError(err));
				}	
								
				break;
				
			case RTCP_SDES:

				/////////////
				if (LOG_RECV_RTCP) {
					FILE 		*fp;
                			char    *LOC_IP, open[40];

		                	LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
        		        	sprintf(open, "%s", LOC_IP);
                			strcat(open, "_rtcp_recv.log");

                			fp = fopen(open, "a+");

					if (fp != NULL) {
						fprintf(fp, "SDES:\n");
						fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
						fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
						fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
						fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
						fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);			
						fflush(fp);
					}
					else {
						fprintf(stderr, "can't open file - %s\n", open);
					}
					fclose(fp);
					free(LOC_IP);
				}
				/////////////
	
				err = sdes_update_member_info(sid, ptr);
				if (err < 0) {
					fprintf(stderr, "receive_rtcp_pkt - %s\n", RTPStrError(err));
				}	
				
				ptr += (rtcppkt.common->length + 1) * 4;
						
				break;
				
			case RTCP_BYE:
								
				/////////////
				if (LOG_RECV_RTCP) {
					FILE 		*fp;
			                char    *LOC_IP, open[40];

		                	LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                			sprintf(open, "%s", LOC_IP);
                			strcat(open, "_rtcp_recv.log");

                			fp = fopen(open, "a+");

					if (fp != NULL) {
						fprintf(fp, "BYE:\n");
						fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
						fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
						fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
						fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
						fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);			
						fflush(fp);
					}
					else {
						fprintf(stderr, "can't open file - recv_rtcplog\n");
					}
					fclose(fp);
					free(LOC_IP);
				}
				/////////////
	
				if (type_of_event(sid) == EVENT_REPORT) {
	
					err = bye_update_member_info(sid, ptr);
					if (err < 0) {
						fprintf(stderr, "receive_rtcp_pkt - %s\n", RTPStrError(err));
					}
									
					if (rs->rtcp_rule.members < rs->rtcp_rule.pmembers) {
						
						struct timeval	now;
						gettimeofday(&now, NULL);
						
						err = reverse_reconsideration(sid, EVENT_REPORT, &now);
						if (err > 0) {
							return err;
						}
					}					
				}
				else if (type_of_event(sid) == EVENT_BYE) {
					rs->rtcp_rule.members++;					
				}				
				
				ptr += (rtcppkt.common->length + 1) * 4;
				
				break;
				
			case RTCP_APP:
			
				//UNDO
				
				break;
				
			default:
				break;
				//ignore = TRUE;
#if RTP_DEBUG	
				printf("MSG	: we ignore rtcp packet whose type is unknown.\n");
#endif
		}
	}
	
	if (!is_bye) {	// A.7 Computing the RTCP Transmission Interval
		rs->rtcp_rule.avg_rtcp_size = (1./16.) *len + (15./16.) * (rs->rtcp_rule.avg_rtcp_size);
	}

	return err;
	
} /* receive_rtcp_pkt */


/* 
 * update_by_send_rtp - update info. after sending rtp packet.
 */
rtperror
update_by_send_rtp(session_id sid, int32 addts, int plen){

	rtperror	err = 0;
	session		*rs;
	member_t	*member;
	
        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
		
	/* adapt to send next packet */
	rs->rtp_sendpkt.rtphdr.seq++;
	rs->rtp_sendpkt.rtphdr.ts += addts;

	/* update trans_info. */
	rs->send_rtpbytes_num += plen; // payload bytes only!!
	rs->send_rtppkt_num++;
	
#if RTP_DEBUG	
	printf("MSG	: update_by_send_rtp - send_rtpbytes_num = %d, send_rtppkt_num = %d\n", rs->send_rtpbytes_num, rs->send_rtppkt_num);//DEBUG
#endif

	gettimeofday(&(rs->last_send_rtp_time), NULL);
	
	/* get local member */
	err = get_member_by_ssrc(rs, rs->rtp_sendpkt.rtphdr.ssrc, &member);
	if (!member) { // NULL
		/* there is no member table in this session */
		return NO_EXIST_MEMBER_TABLE;
	}
	else {
		/* update the sending status */
		if (!member->is_sender) {
			member->is_sender = TRUE;
			rs->rtcp_rule.we_sent = TRUE;
			rs->rtcp_rule.senders++;
		}
	}
			
	return err;

} /* update_by_send_rtp */


/* rtp_set_timerate */
rtperror rtp_set_timerate(session_id sid, int timerate){
	
	rtperror	err;
	session		*rs;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}
	
	rs->rtp_timerate = timerate;
#if RTP_DEBUG	
	printf("MSG	: rtp_set_timerate - timerate = %d\n", rs->rtp_timerate);
#endif
	
	return 0;
		
} /* rtp_set_timerate */


/* 
 * get_rtcp_timeout - get the time of most recent rtcp event of session.
 */
rtperror
get_rtcp_timeout(session_id sid, struct timeval *timeout){
	
	rtp_eq	*eq;
	
	//*timeout = double_to_timeval(0.);
	timeout->tv_sec = 0;
	timeout->tv_usec = 0;
	
	eq = rtpeq;
	
	while (eq) {
		if (eq->sid == sid) {
			*timeout = eq->timeout;
			break;
		}
		eq = eq->next;
	}
	
#if RTP_DEBUG	
	printf("MSG	: rtp_get_timeout - we get timeout.tv_sec =%ld, timeout.tv_usec = %ld\n", timeout->tv_sec, timeout->tv_usec);
#endif
	
	return 0;
	
} /* get_rtcp_timeout */


/* rtp_check_on_expire */
rtperror 
rtp_check_on_expire(){
	
	rtperror err = 0;
	
	err = check_on_expire();
	
	return err;
	
} /* rtp_check_on_expire */
