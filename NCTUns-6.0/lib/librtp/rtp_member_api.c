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
 * the file contains API to control members infomation in the session.
 */


/********************************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rtp_member_api.h"
#include "rtp_session_api.h"


/********************************************************************/


/*
 * init_member - Initialization the members info. structure of RTP session 
 *
 * Here we simply construct a hash table with size "HASH_SIZE".
 */
rtperror 
init_member(session_id sid){
	
	rtperror 	err = 0;
	session 	*rs;
	int 		i;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	else if (err < 0) {
		fprintf(stderr, "init_member - %s\n", RTPStrError(err));
	}
	
	/* allocate memberlist in size of " HASH_SIZE " into memory */
	rs->member_table = (member_t **) calloc (HASH_SIZE, sizeof(member_t *));
	
	/* memberlist initialization */
	for (i = 0; i < HASH_SIZE; i++) {
		rs->member_table[i] = NULL;
	}
	
	return err;
	
} /* init_member */


/*
 * ssrc_hash - return the entry where the member located.
 *
 * u can change the hash structure and algo if u want. 
 */
int
ssrc_hash(u_int32 ssrc){
	
	/* here we simply use "mod" */
	return (ssrc % HASH_SIZE);
	
} /* ssrc_hash */


/* 
 * add_new_member - Add new member into member_table.
 *
 * The function update rs->rtcp_rule->members (members++)
 * if the sockaddr of member is NULL.
 */
rtperror
add_new_member(session_id sid, u_int32 ssrc, struct sockaddr *fromaddr, memberstatus status, int is_rtp, int contributor){
	
	rtperror		err = 0;
	session			*rs;	
	member_t		*member = NULL;
	int 			i;
	
	/* get the session of session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	member= (member_t *) malloc(sizeof(member_t));
	
	/* add member into the first position of entry in member_table */
	i = ssrc_hash(ssrc);
	member->next = rs->member_table[i];
	rs->member_table[i] = member;
	
	if (!rs->member_table[i]) { //NULL
		return ADD_MEMBER_FAIL;
	}
	
	member->ssrc 		= ssrc;
	member->collide 	= FALSE;
	member->is_sender 	= is_rtp;
	member->status		= status;
	member->source.jitter	= -1;	/* init purpose */
	member->contributor	= contributor;

	member->ntp_sec		= 0;
	member->ntp_frac	= 0;
	member->rtp_ts 		= 0;
	member->psent 		= 0;
	member->osent		= 0;
	
#if RTP_DEBUG	
	printf("MSG	: add_new_member - add new member at member_table[%d], ssrc = %u, is_rtp = %d, status = %d, is_contributor =%d\n"
		, i, member->ssrc, member->is_sender, member->status, member->contributor);	//DEBUG_ONLY
#endif
	
	member->last_rtcp_recv_time.tv_sec	= 0;
	member->last_rtcp_recv_time.tv_usec	= 0;
	
	member->last_rtp_recv_time.tv_sec	= 0;
	member->last_rtp_recv_time.tv_usec	= 0;
	
	member->last_sr_recv_time.tv_sec	= 0;
	member->last_sr_recv_time.tv_usec	= 0;
	
	member->recv_rtppkt_num		= 0;
	member->recv_rtpbytes_num	= 0;
		
	for (i = 0; i < 2; i++) {
		/* fromaddr[0] is for rtp */
		if ( (fromaddr) && (!is_rtp == i) ) {
#if RTP_DEBUG	
			char	*IP;//DEBUG
#endif
			
			memset(&(member->fromaddr[i]), '\0', sizeof(struct sockaddr));
			memcpy(&(member->fromaddr[i]), fromaddr, sizeof(struct sockaddr));
			
#if RTP_DEBUG	
			from_in = (struct sockaddr_in *) &(member->fromaddr[i]);//DEBUG
			IP = strdup(inet_ntoa(from_in->sin_addr));//DEBUG
			printf("MSG	: add_new_member - member->fromaddr[%d] = %s\n", i, IP);//DEBUG
			free(IP);//DEBUG
#endif
		}
		else {
			member->fromaddr[i].sa_family = AF_UNSPEC;
		}
	}

	if (rs->rtp_sendpkt.rtphdr.ssrc == ssrc) { 
		// add local member's ssrc into send_sdes_list
		err = add_send_sdes_list(sid, ssrc);
		if (err > 0) {
			return err;
		}
	}
	
	/* initial SDES items */
	for (i = 1; i < 9; i++) {
		member->sdes.intervals[i] = 0;
		member->sdes.length[i] = 0;
		member->sdes.item[i] = NULL;
	}
	/* "cname" should be included in any RTCP packet */
	member->sdes.intervals[1] = 1;
	
	return err;
	
} /* add_new_member */


/*
 * get_member_by_ssrc - return the address of the member in the memory if it exist. else return NULL.
 */
rtperror
get_member_by_ssrc(session *rs, u_int32 ssrc, member_t **member) {

	rtperror	err = 0;	
	member_t 	*mb = NULL;
	int 		i = ssrc_hash(ssrc);
	
	*member = NULL;
	
	mb = rs->member_table[i];
	
	while (mb) {
		if (mb->ssrc == ssrc) {
			*member = mb;
			break;
		}
		mb = mb->next;
	}
	
	if (*member == NULL) {
		//fprintf(stderr, "MSG	: get_member_by_ssrc - no match with %u\n", ssrc);
		return RTP_CANT_MATCH_MEMBER;
	}
	
	return err;
	
} /* get_member_by_ssrc */


/* delete_member_by_ssrc */
rtperror 
delete_member_by_ssrc(session_id sid, u_int32 ssrc){
	
	rtperror 	err = 0;
	member_t 	*previous = NULL, *member = NULL;
	int 		i = ssrc_hash(ssrc), j;
	session		*rs;

	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}	
		
	member = rs->member_table[i];
	
	while (member) {
		if (member->ssrc == ssrc) {
			if (!previous) {
				rs->member_table[i] = member->next;
			}
			else {
				previous->next = member->next;
			}
			
			for (j = 0; j < 9; j ++) {
				if (member->sdes.item[j]) {
					free(member->sdes.item[j]);
					member->sdes.item[j] = NULL;
				}
			}			
			
			if (member->is_sender) {
				rs->rtcp_rule.senders--;

				if (member->ssrc == rs->rtp_sendpkt.rtphdr.ssrc) {
					rs->rtcp_rule.we_sent = FALSE;
					rs->rtcp_rule.initial = FALSE;
				}
			}

			rs->rtcp_rule.members--;
			
#if RTP_DEBUG	
			printf("MSG	: delete_member_by_ssrc - delete member whose ssrc = %u\n", ssrc);
#endif
			
			free(member);
			
			return 0;
		}
		previous = member;
		member = member->next;			
	}	
		
#if RTP_DEBUG	
	printf("MSG	: delete_member_by_ssrc -  no match with ssrc = %u\n", ssrc);
#endif

	return NO_MEMBER_DELETED;

} /* delete_member_by_ssrc */


/* 
 * sr_update_member_info - update info. when are receive a SR RTCP packet.
 */
rtperror 
sr_update_member_info(session_id sid, rtcp_t rtcppkt, struct sockaddr *from, socklen_t *formlen, double ntp_cur, double cur){
	
	//u_int32		ssrc;
	member_t	*rr_member = NULL, *rtcp_member;
	int		i, tmp;
	rtperror	err = 0;
	session		*rs;
	
	/* get the session of session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	//ssrc = rtcppkt.r.sr.rtcp_sr->ssrc;
	
	/* algorithm specified in section 8.2 of RFC-3550 */
	err = rtcp_collision_resolution_loop_detection(sid, rtcppkt, from, rtcppkt.r.sr.rtcp_sr->ssrc);
	if ( err && (err != LOCAL_SOURCE_NEW_CONFLICT) ) {
		/* if there is new collision, we still handle incoming process.	*/
		/* else we abort processing of data packet or control element	*/
		return err;
	}
	
	err = get_member_by_ssrc(rs, rtcppkt.r.sr.rtcp_sr->ssrc, &rtcp_member);
	if (err) {
		fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s\n", RTPStrError(err));
	}
	if (!rtcp_member) {
		fprintf(stderr, "no rtcp_member, must stop process!!\n");//DEBUG
		exit(1);//DEBUG
	}	

	rtcp_member->last_sr_recv_time = double_to_timeval(cur);
	
	rtcp_member->ntp_sec	= rtcppkt.r.sr.rtcp_sr->ntp_sec;
	rtcp_member->ntp_frac	= rtcppkt.r.sr.rtcp_sr->ntp_frac;
	rtcp_member->rtp_ts 	= rtcppkt.r.sr.rtcp_sr->rtp_ts;
	rtcp_member->psent 	= rtcppkt.r.sr.rtcp_sr->psent;
	rtcp_member->osent	= rtcppkt.r.sr.rtcp_sr->osent;

#if RTP_DEBUG	
	printf("MSG	: sr_update_member_info - receive SR from ssrc = %u\n", rtcppkt.r.sr.rtcp_sr->ssrc);
#endif
	
	/////////////
	if (LOG_RECV_RTCP) {
		FILE 	*fp;
                char    *LOC_IP, open[40];
                                       
		LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
		sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_recv.log");

		fp = fopen(open, "a+");
		if (fp != NULL) {
			fprintf(fp, "SR:\n");
			
			fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
			fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
			fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
			fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
			fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);
						
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->ssrc = %lu\n", rtcppkt.r.sr.rtcp_sr->ssrc);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->ntp_sec = %lu\n", rtcppkt.r.sr.rtcp_sr->ntp_sec);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->ntp_frac = %lu\n", rtcppkt.r.sr.rtcp_sr->ntp_frac);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->rtp_ts = %lu\n", rtcppkt.r.sr.rtcp_sr->rtp_ts);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->psent = %lu\n", rtcppkt.r.sr.rtcp_sr->psent);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->osent = %lu\n", rtcppkt.r.sr.rtcp_sr->osent);
			fprintf(fp, "--------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
	/////////////
	/*		
	if (!member->is_sender) {
		member->is_sender = TRUE;
		rs->rtcp_rule.senders++;
	}
	*/
	tmp = rtcppkt.common->count;
	i = 0;
	
#if RTP_DEBUG	
	printf("MSG	: sr_update_member_info - rtcppkt.common->count = %d\n", tmp);//DEBUG
#endif
				
	while (tmp) {
		
		rr_member = NULL;
		
		err = get_member_by_ssrc(rs, rtcppkt.r.sr.rr[i]->ssrc, &rr_member);
		if (err > 0) {
			fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s\n", RTPStrError(err));
		}
		
		if (!rr_member) { // member == NULL
		
			//err = add_new_member(sid, rtcppkt.r.sr.rr[i]->ssrc, NULL, valid, TRUE, FALSE);
			err = add_new_member(sid, rtcppkt.r.sr.rr[i]->ssrc, NULL, valid, FALSE, FALSE);
			//應該是真正收到 rtp packet 才算 senders 吧 ??
			if (err > 0) {
				return err;
			}
				
			err = get_member_by_ssrc(rs, rtcppkt.r.sr.rr[i]->ssrc, &rr_member);
			if (err) {
				fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s\n", RTPStrError(err));
			}
			if (!rr_member) {
				fprintf(stderr, "no rr_member, must stop process!!\n");//DEBUG
				exit(1);
			}	
		}

		err = update_rr(sid, rr_member, rtcppkt.r.sr.rr[i], ntp_cur, rtcp_member);
		if (err) {
			return err;
		}

		tmp--;
		i++;
	}
	
	return err;	
	
} /* sr_update_member_info */


/* rr_update_member_info */
rtperror 
rr_update_member_info(session_id sid, rtcp_t rtcppkt, struct sockaddr *from, socklen_t *formlen, double ntp_cur){
	
	member_t	*rr_member = NULL, *rtcp_member;
	rtperror	err = 0;
	session		*rs;
	u_int32		ssrc;
	int		i, tmp;
	
	/* get the session of session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	memcpy(&ssrc, rtcppkt.r.rr.ssrc, sizeof(u_int32));
	
	/* algorithm specified in section 8.2 of RFC-3550 */
	err = rtcp_collision_resolution_loop_detection(sid, rtcppkt, from, ssrc);
	if ( err && (err != LOCAL_SOURCE_NEW_CONFLICT) ) {
		/* if there is new collision, we still handle incoming process.	*/
		/* else we abort processing of data packet or control element	*/
		return err;
	}
	
	err = get_member_by_ssrc(rs, ssrc, &rtcp_member);
	if (err) {
		fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s\n", RTPStrError(err));
	}
	if (!rtcp_member) {
		fprintf(stderr, "no rtcp_member, must stop process!!\n");//DEBUG
		exit(1);
	}	

	tmp = rtcppkt.common->count;
	i = 0;

#if RTP_DEBUG	
	printf("MSG	: rr_update_member_info - receive RR from ssrc = %u\n", *(rtcppkt.r.rr.ssrc));
	
	printf("MSG	: rr_update_member_info - rtcppkt.common->count = %d\n", tmp);
#endif
	
	/////////////
	if (LOG_RECV_RTCP) {
		FILE 		*fp;
		char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_recv.log");

		fp = fopen(open, "a+");
		
		if (fp != NULL) {
			fprintf(fp, "RR:\n");
			fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
			fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
			fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
			fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
			fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);			
			fprintf(fp, "rtcppkt.r.rr.ssrc = %lu\n", *(rtcppkt.r.rr.ssrc));
			fprintf(fp, "--------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
				
	while (tmp) {
		
		rr_member = NULL;
		
		err = get_member_by_ssrc(rs, rtcppkt.r.rr.rr[i]->ssrc, &rr_member);
		if (err > 0) {
			return err;
		}
		
#if RTP_DEBUG	
		printf("MSG	: rr_update_member_info - rtcppkt.r.rr.rr[%d]->ssrc=%u\n", i, rtcppkt.r.rr.rr[i]->ssrc);
#endif
		
		if (!rr_member) { // member == NULL
		
			err = add_new_member(sid, rtcppkt.r.rr.rr[i]->ssrc, NULL, valid, FALSE, FALSE);
			if (err > 0) {
				return err;
			}
				
			err = get_member_by_ssrc(rs, rtcppkt.r.rr.rr[i]->ssrc, &rr_member);
			if (err) {
				fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s\n", RTPStrError(err));
			}
			if (!rr_member) {
				fprintf(stderr, "no rr_member, must stop process!!\n");//DEBUG
				exit(1);
			}
		}
			
		err = update_rr(sid, rr_member, rtcppkt.r.rr.rr[i], ntp_cur, rtcp_member);
		if (err > 0) {
			return err;
		}
				
		tmp--;
		i++;
	}
				
	return 0;
		
} /* rr_update_member_info */


/* sdes_update_member_info */
rtperror 
sdes_update_member_info(session_id sid, char *ptr){
	
	member_t	*member = NULL;
	rtperror	err = 0;
	session		*rs;
	
	rtcp_t		*r;
	int		count;
	rtcp_sdes_t	*sd;
	rtcp_sdes_item_t *rsp, *rspn, *end;
	rtcp_common_t	*rtcp_common;
	
	/* get the session of session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
		
	r = (rtcp_t *) ptr;
	rtcp_common = (rtcp_common_t *) ptr;
		
	count = rtcp_common->count;
	sd = &r->r.sdes;
	end = (rtcp_sdes_item_t *) ((u_int32 *)r + rtcp_common->length + 1);
	
	while (--count >= 0) {
		
		rsp = &sd->item[0];
		if (rsp >= end) {
			break;
		}	
	
		err = get_member_by_ssrc(rs, sd->src, &member);
		if (err) {
			return err;
		}
		
#if RTP_DEBUG	
		printf("MSG	: sdes_update_member_info - receive SDES from ssrc = %u\n", sd->src);
#endif

		/////////////
		if (LOG_RECV_RTCP) {
			FILE 	*fp;
	                char    *LOC_IP, open[40];

	                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
	                sprintf(open, "%s", LOC_IP);
	                strcat(open, "_rtcp_recv.log");
	
	                fp = fopen(open, "a+");

			if (fp != NULL) {		
				fprintf(fp, "sd->src = %lu\n", sd->src);
				fflush(fp);
			}
			else {
				fprintf(stderr, "can't open file - %s\n", open);
			}
			fclose(fp);
			free(LOC_IP);
		}
		/////////////
		
		if (!member) {/////////////////////////////////////////////////////////////////////
#if RTP_DEBUG	
			printf("MSG	: sdes_update_member_info - SDES get NULL member, must stop process!\n");
#endif
			exit(1);
		}
	
		for (; rsp->type; rsp = rspn ) {
			rspn = (rtcp_sdes_item_t *) ((char*) rsp + rsp->length + 2);
			
			if (rspn >= end) {
				rsp = rspn;
				break;
			}
			
			member->sdes.length[rsp->type] = rsp->length;

			/* store the new SDES information for that member. */
			member->sdes.item[rsp->type] = (char *) malloc( sizeof(char) * (rsp->length) );
			strncpy(member->sdes.item[rsp->type], rsp->data, rsp->length);		
		
			/////////////
			if (LOG_RECV_RTCP) {
				FILE 		*fp;
	                        char    *LOC_IP, open[40];

        	                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                	        sprintf(open, "%s", LOC_IP);
                        	strcat(open, "_rtcp_recv.log");

                        	fp = fopen(open, "a+");
					
				if (fp != NULL) {
					fprintf(fp, "member->sdes.length[%d] = %u\n", rsp->type, member->sdes.length[rsp->type]);
					fprintf(fp, "member->sdes.item[%d] = %s\n", rsp->type, member->sdes.item[rsp->type]);		
					fflush(fp);
				}
				else {
					fprintf(stderr, "can't open file - %s\n", open);
				}
				fclose(fp);
				free(LOC_IP);
			}
			/////////////
		
		}
		sd = (rtcp_sdes_t *) ((u_int32 *)sd + (((char *)rsp - (char *)sd) >> 2) + 1);
	}
	
	if (count >= 0) {
		/* invalid packet format */
		return SDES_COUNT_ERROR;
	}		
		
	return err;
		
} /* sdes_update_member_info */


/* bye_update_member_info */
rtperror 
bye_update_member_info(session_id sid, char *ptr){
	
	rtperror	err = 0;
	session		*rs;
	u_int32		*ssrc;
	rtcp_t		*r;
	int		count;
	rtcp_common_t	*rtcp_common;
	
	/* get the session of session_list[sid] to "s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	r = (rtcp_t *) ptr;	
	rtcp_common = (rtcp_common_t *) ptr;
	count = rtcp_common->count;
	ptr += sizeof(rtcp_common_t);
	
	while (--count >= 0) {
		
		ssrc = (u_int32 *) ptr;
		
#if RTP_DEBUG	
		printf("MSG	: bye_update_member_info - receive BYE from ssrc = %u\n", ssrc);
#endif		
		/////////////
		if (LOG_RECV_RTCP) {
			FILE 		*fp;
				
			fp = fopen("recv_rtcplog", "a+");
			
			if (fp != NULL) {		
				fprintf(fp, "bye ssrc = %lu\n", *ssrc);
				fflush(fp);
			}
			else {
				fprintf(stderr, "can't open file - recv_rtcplog\n");
			}
			fclose(fp);
		}
		/////////////
				
                err = del_send_sdes_list(sid, *ssrc);
                if (err > 0) {
                	return err;
                }

                err = del_send_rb_list(sid, *ssrc);
                if (err > 0) {
                        return err;
                }
		
		err = delete_member_by_ssrc(sid, *ssrc);
		if (err < 0) {
			fprintf(stderr, "bye_update_member_info - %s\n", RTPStrError(err));
		}
		
		ptr += sizeof(u_int32);
	}
	
	//len = ((char *) r + (rtcppkt.common->length + 1) * 4) - ptr;

	/////////////
	if (LOG_RECV_RTCP) {
		FILE 	*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_recv.log");

                fp = fopen(open, "a+");
	
		if (fp != NULL) {		
			fprintf(fp, "bye msg = %s\n", ptr);
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - recv_rtcplog\n");
		}
		fclose(fp);
		free(LOC_IP);
	}
	/////////////
			
#if RTP_DEBUG	
	printf("MSG	: bye msg = %s\n", ptr);
#endif
		
	return err;
		
} /* bye_update_member_info */


/* 
 * rtp_collision_resolution_loop_detection - Processing if rtp collsion or lool detecting.
 *
 * specified in section 8.2 of RFC-3550.
 */
rtperror 
rtp_collision_resolution_loop_detection(session_id sid, rtp_pkt_t rtppkt, struct sockaddr *from){
	
	rtperror		err = 0;
	session			*rs;
	member_t		*mb = NULL;
	int			new_source = FALSE;
	struct sockaddr_in	*from_in, *check_in;
	struct timeval		now;
	member_t 		*member = NULL;
	
	/* get the session of session_list[sid] to "s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	//check_in = (struct sockaddr_in *) &(member->fromaddr[is_rtcp]);
	gettimeofday(&now, NULL);
	
	err = get_member_by_ssrc(rs, rtppkt.rtphdr.ssrc, &member);
	if (err < 0) {
		//fprintf(stderr, "rtp_collision_resolution_loop_detection1 - %s\n", RTPStrError(err));
	}
	
	if (member) {

		from_in = (struct sockaddr_in *) from;
		check_in = (struct sockaddr_in *) &(member->fromaddr[0]);
		
		/* Identifier is found in the table */

		//if (	( member->fromaddr[0].sa_family == AF_UNSPEC  && 
		//	  member->fromaddr[1].sa_family != AF_UNSPEC ) ) {
		//	因為rr 可能會新增...
		if (member->fromaddr[0].sa_family == AF_UNSPEC) {			  	
			/* table entry was created on receipt of a control packet and 
			   this is the first data packet or vice versa */
			   
			/* store the source transport address from this packet */
			member->fromaddr[0] = *from;
			
#if RTP_DEBUG	
			printf("MSG	: rtp_collision_resolution_loop_detection - old member - case 1~~\n");//DEBUG

			printf("MSG	: rtcp_collision_resolution_loop_detection - check_in->sin_port = %d\n", check_in->sin_port);//DEBUG
			printf("MSG	: rtcp_collision_resolution_loop_detection - from_in->sin_port = %d\n", from_in->sin_port);//DEBUG
#endif			
		}
		else if ( check_in->sin_family != from_in->sin_family ||
			  check_in->sin_addr.s_addr != from_in->sin_addr.s_addr ||
			  check_in->sin_port != from_in->sin_port) {
			
			if(check_in->sin_family != from_in->sin_family) {
#if RTP_DEBUG	
				printf("MSG	: rtp_collision_resolution_loop_detection - sin_family NO MATCH\n");//DEBUG
#endif
			}	

			if(check_in->sin_addr.s_addr != from_in->sin_addr.s_addr) {
#if RTP_DEBUG	
				printf("MSG	: rtp_collision_resolution_loop_detection - sin_addr.s_addr NO MATCH\n");//DEBUG
#endif
			}	

			if(check_in->sin_port != from_in->sin_port) {
				//printf("MSG	: rtp_collision_resolution_loop_detection - sin_port NO MATCH\n");

#if RTP_DEBUG	
				printf("MSG	: rtp_collision_resolution_loop_detection - check_in->sin_port = %d\n", ntohs(check_in->sin_port));//DEBUG
				printf("MSG	: rtp_collision_resolution_loop_detection - from_in->sin_port = %d\n", ntohs(from_in->sin_port));//DEBUG	
#endif	
			}	
									
#if RTP_DEBUG	
			printf("MSG	: rtp_collision_resolution_loop_detection - old member - case 2~~\n");//DEBUG
#endif			

			/* source transport address from the packet does not match
			   the one saved in the table entry for this identifier */
			
			if (rtppkt.rtphdr.ssrc != rs->rtp_sendpkt.rtphdr.ssrc) {
				
				/* source identifier is not the participant's own	*/
				
				/* abort processing of data packet or control element	*/
				/* here we simply keep the old source			*/
				/* MAY choose a different policy to keep new source	*/
				err = OTHER_SOURCE_CONFLICT;
			}
		
			/* A collision or loop of the participant's own packets */
			
			else if (find_in_conflict_list(rs, from)) {
				
#if RTP_DEBUG	
				printf("MSG	: rtp_collision_resolution_loop_detection - ssrc is in the conflict list.\n");//DEBUG
#endif
				
				/* mark current time in conflicting address list entry && reset conflict count to 0 */
				update_conflict_info(rs, from, &now);	

				/* abort processing of data packet or control element	*/
				err = LOCAL_SOURCE_OLD_CONFLICT;						
			}
		
			/* New collision, change SSRC identifier */
			
			else {
				char	*reason = "ssrc collision";
				
#if RTP_DEBUG	
				printf("MSG	: rtp_collision_resolution_loop_detection - ssrc collision!!!!!!!!!!!!!!!!!\n");//DEBUG
#endif

				/* send an RTCP BYE packet with the old SSRC identifier */
				
				/* log occurrence of a collision */
				log_collision(rs, from, &now);
					
				/* create a new entry in the conflicting data or control
		   		source transport address list and mark current time	*/
				new_collision(rs, from, &now);
					
				/* send an RTCP BYE packet with the old SSRC identifier */
				err = rtcp_stop(sid, reason);	// EVENT_BYE, &now);
				if (err > 0) {
					return err;
				}
				
				err = delete_member_by_ssrc(sid, rs->rtp_sendpkt.rtphdr.ssrc);
				if (err < 0) {
					fprintf(stderr, "delete_member_by_ssrc - %s\n", RTPStrError(err));
				}
					
				/* choose a new SSRC identifier */
				
				do {	
					/* Initialize random number generators with a random seed. */
					init_random_seed();
		
					/* initialization */
					rs->rtp_sendpkt.rtphdr.ssrc = random32(sid);
					//rs->rtp_sendpkt.rtphdr.ts = random32(sid);
					//rs->rtp_sendpkt.rtphdr.seq = random32(sid) % 65536;
					
#if RTP_DEBUG	
					printf("MSG	: local new SSRC = %u\n", rs->rtp_sendpkt.rtphdr.ssrc); //DEBUG_ONLY
					//printf("MSG	: timestamp = %u\n", rs->rtp_sendpkt.rtphdr.ts); //DEBUG_ONLY
					//printf("MSG	: seqnum = %u\n", rs->rtp_sendpkt.rtphdr.seq); //DEBUG_ONLY
#endif
					
					err = get_member_by_ssrc(rs, rs->rtp_sendpkt.rtphdr.ssrc, &mb);
					if (err < 0) {
						fprintf(stderr, "rtp_collision_resolution_loop_detection2 - %s\n", RTPStrError(err));
					}
					
				} while(mb);	// if mb != NULL, it means there is another ssrc collision with the other member

				rs->send_rtppkt_num	= 0;
				rs->send_rtpbytes_num	= 0;
				rs->send_rtcppkt_num	= 0;	// i guess
				rs->send_rtcpbytes_num	= 0;	// i guess

				/* create a new entry in the source identifier table with the old SSRC plus 
				the source transport address from the data or control packet being processed */
				if (rs->rtcp_rule.we_sent) {
					err = add_new_member(sid, rs->rtp_sendpkt.rtphdr.ssrc, from, valid, TRUE, FALSE);
				}
				else {
					err = add_new_member(sid, rs->rtp_sendpkt.rtphdr.ssrc, from, valid, FALSE, FALSE);
				}	
				if (err > 0) {
					return err;
				}	
				//init_seq(&(member->source), rtppkt.rtphdr);
				//member->source.max_seq 	= rtppkt.rtphdr.seq - 1;
				//member->source.probation= MIN_SEQUENTIAL;
									
				/* re-start the rtcp transmission */
				if (rs->rtcpopen) {
					err = rtcp_start(sid);
					if (err > 0) {
						return err;
					}
				}
				
				err = LOCAL_SOURCE_NEW_CONFLICT;;					
			}
		}
		else {
#if RTP_DEBUG	
			printf("MSG	: rtp_collision_resolution_loop_detection - old member - case 3~~\n");//DEBUG

			//printf("MSG	: rtcp_collision_resolution_loop_detection - check_in->sin_port = %d\n", ntohs(check_in->sin_port));
			//printf("MSG	: rtcp_collision_resolution_loop_detection - from_in->sin_port = %d\n", ntohs(from_in->sin_port));
#endif
			// everything is ok.		
		}
	} // if member != NULL
	else {
		/* NewMember, SSRC or CSRC identifier is not found in the source identifier table */
		new_source = TRUE;
		
#if RTP_DEBUG	
		printf("MSG	: rtp_collision_resolution_loop_detection - new member~~\n");//DEBUG
#endif		
		if (type_of_event(sid) != EVENT_BYE) {	// specified in A.7 of RFC-3550
			
			/* create a new entry storing the data or control source transport address, 
			the SSRC or CSRC and other state */
			
			err = add_new_member(sid, rtppkt.rtphdr.ssrc, from, invalid, TRUE, FALSE);
			if (err > 0) {
				return err;
			}
	
			/* get new member which we just create */
			err = get_member_by_ssrc(rs, rtppkt.rtphdr.ssrc, &member);
			if (err < 0) {
				fprintf(stderr, "rtp_collision_resolution_loop_detection3 - %s\n", RTPStrError(err));
			}
			
			init_seq(&(member->source), rtppkt.rtphdr);
			member->source.max_seq 	= rtppkt.rtphdr.seq - 1;
			member->source.probation= MIN_SEQUENTIAL;
		}
		else {
			return WE_ARE_SENDING_BYE;
			// do something ?? 
		}
				
	} // member == NULL

	/* update sequence */
	if (update_seq(&(member->source), rtppkt.rtphdr)) {
		/* now, the ssrc is valid */
		
		memberstatus s = valid;
		
		if (member->status != s) {
			/* We add members in rtcp_rule if the status of member is "invalid" */
			member->status = s;
			rs->rtcp_rule.members++;
			rs->rtcp_rule.senders++;
			member->is_sender = TRUE;
			err = add_send_rb_list(sid, member->ssrc);
			if (err > 0) {
				return err;
			}
		}

		if (member->is_sender == FALSE) {
			/* we add senders if the is_sender of member is "FALSE" */
#if RTP_DEBUG	
			printf("MSG	: the member [%u] send rtp packet who is still not sender now, so we add sender\n", member->ssrc);//DEBUG
#endif

			rs->rtcp_rule.senders++;
			member->is_sender = TRUE;
			err = add_send_rb_list(sid, member->ssrc);
			if (err > 0) {
				return err;
			}
		}		
	}
	else {
		/* ssrc is still " INVALID " at present */
		
		if (!PROMISCUOUS_MODE) {
			/* Those invalid packets may be discarded or they may be stored and delivered once 
			   validation has been achieved if the resulting delay is acceptable. */
			/* here we simply discard these invalid packets. */
			return SOURCE_STILL_INVALID;
		}
	}	

	/* below here, the srrc is valid or we are in "PROMISCUOUS_MODE" */	
	/* 不放上面是因為有可能是 "PROMISCUOUS_MODE" 情形 */

	if (member) {
		member->rtp_recvpkt.rtphdr.version = rtppkt.rtphdr.version;
		member->rtp_recvpkt.rtphdr.p 	= rtppkt.rtphdr.p;
		member->rtp_recvpkt.rtphdr.x 	= rtppkt.rtphdr.x;
		member->rtp_recvpkt.rtphdr.cc	= rtppkt.rtphdr.cc;
		member->rtp_recvpkt.rtphdr.m 	= rtppkt.rtphdr.m;
		member->rtp_recvpkt.rtphdr.pt 	= rtppkt.rtphdr.pt;
		member->rtp_recvpkt.rtphdr.seq 	= rtppkt.rtphdr.seq;
		member->rtp_recvpkt.rtphdr.ts 	= rtppkt.rtphdr.ts;
		member->rtp_recvpkt.rtphdr.ssrc	= rtppkt.rtphdr.ssrc;
	}
	else {
		fprintf(stderr, "rtp_collision_resolution_loop_detectio ERROR!!\n");//DEBUG
		exit(1);//DEBUG
	}	
		
	/* update member info. NOTE, this doesn't update the "last_sr_recv_time" and "jitter" info. */
	update_member_info(rs, member, TRUE, &now);
	
	/* don't forget CSRC situation */
	
	return err;
	
} /* rtp_collision_resolution_loop_detection */


/*
 * rtp_timeout - Timeout the inactive memebers or senders.
 *
 * The participant must perform this check at least once per RTCP 
 * transmission interval. 
 */
rtperror 
rtp_timeout(session_id sid, struct timeval *now){
	
	rtperror	err = 0;
	session		*rs;
	member_t	*member, *previous, *tmp;
	int		i = 0, n;
	double 		temp, t, rtcp_min_time = RTCP_MIN_TIME;	
	struct timeval	member_timeout;
	conflict_list_t *c = NULL, *remove = NULL, *pre = NULL;
	
	double		rtcp_bw;

	/* get the session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}	
	
	
	/* At occasional intervals, the participant must check to see if any of the other participants time
	 * out. To do this, the participant computes the deterministic (without the randomization factor)
	 * calculated interval Td for a receiver, that is, with we sent false. Any other session member who has
	 * not sent an RTP or RTCP packet since time tc - MTd (M is the timeout multiplier, and defaults
	 * to 5) is timed out. This means that its SSRC is removed from the member list, and members is
	 * updated. A similar check is performed on the sender list. Any member on the sender list who has
	 * not sent an RTP packet since time tc - 2T (within the last two RTCP report intervals) is removed
	 * from the sender list, and senders is updated. */
 
	rtcp_bw = rs->rtcp_rule.rtcp_bw;
	n = rs->rtcp_rule.members;
	
	if (rs->rtcp_rule.initial) {
		rtcp_min_time /= 2;
	}
	
	if ( (rs->rtcp_rule.senders) <= (rs->rtcp_rule.members) * RTCP_SENDER_BW_FRACTION ) {
		if (rs->rtcp_rule.we_sent) {
			rtcp_bw *= RTCP_SENDER_BW_FRACTION;
			n = (rs->rtcp_rule.senders);
		}
		else {
			rtcp_bw *= RTCP_RCVR_BW_FRACTION;
			n -= (rs->rtcp_rule.senders);
		}
	}

	t = (rs->rtcp_rule.avg_rtcp_size) * n / rtcp_bw;
	
	if (t < rtcp_min_time) {
		t = rtcp_min_time;
	}
	
	t = t * TIMEOUT_MULTIPLIER;
	
	temp = timeval_to_double(*now);
	
#if RTP_DEBUG	
	printf("MSG	: rtp_timeout(member) time - now = %lf, interval = %lf, ", temp, t);
#endif
	
	temp -= t;
	
#if RTP_DEBUG	
	printf("result = %lf\n", temp);
#endif
		
	/* time -> the member timeout critical */
	member_timeout = double_to_timeval(temp);
	
	member = rs->member_table[0];
	previous = NULL;

	while (i < HASH_SIZE) {
		if (member && member->ssrc != rs->rtp_sendpkt.rtphdr.ssrc) {
			tmp = member;
			member = member->next;
			if (tmp->is_sender) {
				/* check sender status if we received rtp packet before */
				if ( ( (rs->sender_timeout.tv_sec != 0) && (rs->sender_timeout.tv_usec != 0) ) // sender_timeout != 0.0
				   && time_expire(&(tmp->last_rtp_recv_time), &(rs->sender_timeout)) ) {
					
#if RTP_DEBUG	
					printf("MSG	: rtp_timeout - timeout sender = %u, last_rtp_recv_time = %lf, sender_timeout = %lf\n", tmp->ssrc, timeval_to_double(tmp->last_rtp_recv_time), timeval_to_double(rs->sender_timeout));
					fflush(stdout);
#endif
					
					tmp->is_sender = FALSE;
					rs->rtcp_rule.senders--;

					err = del_send_sdes_list(sid, tmp->ssrc);
					if (err > 0) {
						return err;
					}

					err = del_send_rb_list(sid, tmp->ssrc);
					if (err > 0) {
						return err;
					}
				}	
			}

			if (timeval_to_double(member_timeout) > 0) {
				if (  time_expire( &(tmp->last_rtcp_recv_time), &(tmp->last_rtp_recv_time) ) ) {
					/* last one packet is rtp if we received rtp packet before */
					if ( (tmp->last_rtp_recv_time.tv_sec || tmp->last_rtp_recv_time.tv_usec) 
					   && time_expire(&(tmp->last_rtp_recv_time), &member_timeout) ) {

#if RTP_DEBUG	
						printf("MSG	: rtp_timeout - timeout user = %u, last_rtp_recv_time = %lf\n", tmp->ssrc, timeval_to_double(tmp->last_rtp_recv_time));
						fflush(stdout);
#endif
						
						err = del_send_sdes_list(sid, tmp->ssrc);
						if (err > 0) {
							return err;
						}
						
						err = del_send_rb_list(sid, tmp->ssrc);
						if (err > 0) {
							return err;
						}
						
						err = delete_member_by_ssrc(sid, tmp->ssrc);
						if (err < 0) {
							fprintf(stderr, "delete_member_by_ssrc - %s\n", RTPStrError(err));
						}
						
					}
				}
				else  {
					/* last one packet is rtcp if we received rtcp packet before */
					if ( (tmp->last_rtcp_recv_time.tv_sec || tmp->last_rtcp_recv_time.tv_usec) 
					   && time_expire(&(tmp->last_rtcp_recv_time), &member_timeout) ) {

#if RTP_DEBUG	
						printf("MSG	: rtp_timeout - timeout user = %u, last_rtcp_recv_time = %lf\n", tmp->ssrc, timeval_to_double(tmp->last_rtcp_recv_time));
						fflush(stdout);
#endif						
						err = del_send_sdes_list(sid, tmp->ssrc);
						if (err > 0) {
							return err;
						}
						
						err = del_send_rb_list(sid, tmp->ssrc);
						if (err > 0) {
							return err;
						}
						
						err = delete_member_by_ssrc(sid, tmp->ssrc);
						if (err < 0) {
							fprintf(stderr, "delete_member_by_ssrc - %s\n", RTPStrError(err));
						}
					}			
				}
			}
		}
		else if (member && member->ssrc == rs->rtp_sendpkt.rtphdr.ssrc
		        && ( (rs->sender_timeout.tv_sec != 0) && (rs->sender_timeout.tv_usec) != 0)) {
			/* only test if local is a sender or not */
			if ( member->is_sender && (rs->last_send_rtp_time.tv_sec || rs->last_send_rtp_time.tv_usec) 
			   && time_expire(&(rs->last_send_rtp_time), &(rs->sender_timeout)) ) {
#if RTP_DEBUG	
				printf("MSG	: rtp_timeout - %u,last_rtp_send_time=%lf\n", rs->rtp_sendpkt.rtphdr.ssrc, timeval_to_double(rs->last_send_rtp_time));
				printf("MSG	: rtp_timeout - timeout local sender: %u\n", rs->rtp_sendpkt.rtphdr.ssrc);
#endif

				member->is_sender = FALSE;
				rs->rtcp_rule.we_sent = FALSE;
				rs->rtcp_rule.senders--;
			}
			member = member->next;	
		}	
		else {
			i++;
			if (i < HASH_SIZE) {
				member = rs->member_table[i];
				previous = NULL;
			}	
		}
	}

	/* timeout conflict elemnet if it pass 10 intervals without any collision */
	c = rs->conflict_list;

	while (c) {
		if (c->count >= 10) {
		    	remove = c;
			if (c == rs->conflict_list) {
				rs->conflict_list = c->next;
			}
		}
		else {
			pre = c;
		}
		c = c->next;
		if (remove) {
#if RTP_DEBUG	
			printf("MSG	: rtp_timeout - timeout conflict address whose ssrc = %u\n", remove->ssrc);
#endif

			free(remove);
			remove = NULL;
			if (pre) {
				pre->next = c;
			}
		}		
	}

	return err;
	
} /* rtp_timeout */


/* find_next_sender(sid, member) */
rtperror
find_next_sender(session_id sid, int *hash, member_t **member, member_t **sender){
	
	rtperror 	err;
	session 	*rs;
	member_t	*mb = NULL;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}

	while (!mb && (*hash) < HASH_SIZE) {
		
		if (!(*member)) { // NULL
			(*hash)++;
			*member = rs->member_table[*hash];
		}
		else {
			if ((*member)->is_sender) {
				mb = *member;
			}
			else {
				*member = (*member)->next;
			}
		}
	}
	
	*sender = mb;
	
	if (*sender == NULL) {
#if RTP_DEBUG	
		printf("MSG	: find_next_sender - returns  NULL!!\n");
#endif
	}
	else {
#if RTP_DEBUG	
		printf("MSG	: find_next_sender - returns member whose ssrc = %u!!\n", (*sender)->ssrc);
#endif
	}
	
	return 0;	
		
} /* find_next_sender */


/* rtcp_collision_resolution_loop_detection */
rtperror 
rtcp_collision_resolution_loop_detection(session_id sid, rtcp_t rtcppkt, struct sockaddr *from, u_int32 ssrc){
	
	rtperror		err = 0;
	session			*rs;
	member_t		*mb = NULL;
	int			new_source = FALSE;
	struct sockaddr_in	*from_in, *check_in;
	struct timeval		now;
	member_t		*member = NULL;
	
	/* get the session of session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	/* get new member which we just create */
	err = get_member_by_ssrc(rs, ssrc, &member);
	if (err) {
		/* maybe new participant */
		//fprintf(stderr, "rtcp_collision_resolution_loop_detection - %s\n", RTPStrError(err));
	}
				
	//check_in = (struct sockaddr_in *) &(member->fromaddr[is_rtcp]);
	gettimeofday(&now, NULL);
						
	if (member) {

		from_in = (struct sockaddr_in *) from;
		check_in = (struct sockaddr_in *) &(member->fromaddr[1]);
		
#if RTP_DEBUG	
		printf("MSG	: rtcp_collision_resolution_loop_detection - old member~~\n");//DEBUG
#endif
			
		/* Identifier is found in the table */

		//if (	( member->fromaddr[1].sa_family == AF_UNSPEC  && 
		//	  member->fromaddr[0].sa_family != AF_UNSPEC ) ) {
		//	因為 rr 啥都沒....所以先佔用下面方法
		if (member->fromaddr[1].sa_family == AF_UNSPEC) {			  	
			/* table entry was created on receipt of a control packet and 
			   this is the first data packet or vice versa */
			   
			/* store the source transport address from this packet */
			member->fromaddr[1] = *from;
			
#if RTP_DEBUG	
			printf("MSG	: rtcp_collision_resolution_loop_detection - old member - case 1~~\n");//DEBUG
			printf("MSG	: rtcp_collision_resolution_loop_detection - check_in->sin_port = %d\n", ntohs(check_in->sin_port));//DEBUG
			printf("MSG	: rtcp_collision_resolution_loop_detection - from_in->sin_port = %d\n", ntohs(from_in->sin_port));//DEBUG
#endif			
		}
		else if ( check_in->sin_family != from_in->sin_family ||
			  check_in->sin_addr.s_addr != from_in->sin_addr.s_addr ||
			  check_in->sin_port != from_in->sin_port) {

			if(check_in->sin_family != from_in->sin_family) {
#if RTP_DEBUG	
				printf("MSG	: rtcp_collision_resolution_loop_detection - sin_family NO MATCH\n");//DEBUG
#endif
			}	

			if(check_in->sin_addr.s_addr != from_in->sin_addr.s_addr) {
#if RTP_DEBUG	
				printf("MSG	: rtcp_collision_resolution_loop_detection - sin_addr.s_addr NO MATCH\n");//DEBUG
#endif
			}	

			//if(check_in->sin_port != from_in->sin_port) {
				//printf("MSG	: rtcp_collision_resolution_loop_detection - sin_port NO MATCH\n");
#if RTP_DEBUG	
				printf("MSG	: rtcp_collision_resolution_loop_detection - check_in->sin_port = %d\n", check_in->sin_port);//DEBUG
				printf("MSG	: rtcp_collision_resolution_loop_detection - from_in->sin_port = %d\n", from_in->sin_port);//DEBUG
			//}	
						
			printf("MSG	: rtcp_collision_resolution_loop_detection - old member - case 2~~\n");//DEBUG
#endif			
			/* An identifier collision or a loop is indicated */
		
			if (rs->rtp_sendpkt.rtphdr.ssrc != ssrc) {
				
				/* OPTIONAL error counter step 
				if (source identifier is from an RTCP SDES chunk containing a CNAME item that 
				    differs from the CNAME in the table entry) {
					count a third-party collision;
				} else {
					count a third-party loop;
				}
				*/
				/* abort processing of data packet or control element	*/
				/* MAY choose a different policy to keep new source	*/
				err = OTHER_SOURCE_CONFLICT;
			}
		
			/* A collision or loop of the participant's own packets */
			
			else if (find_in_conflict_list(rs, from)) {
				
#if RTP_DEBUG	
				printf("MSG	: rtcp_collision_resolution_loop_detection - ssrc is in the conflict list.\n");//DEBUG
#endif				
				/* mark current time in conflicting address list entry && reset conflict count to 0 */
				update_conflict_info(rs, from, &now);	

				/* abort processing of data packet or control element	*/
				err = LOCAL_SOURCE_OLD_CONFLICT;						
			}
		
			/* New collision, change SSRC identifier */
			
			else {
				char	*reason = "ssrc collision";
				
#if RTP_DEBUG	
				printf("MSG	: rtcp_collision_resolution_loop_detection - ssrc collision!!!!!!!!!!!!!!!!!\n");//DEBUG
#endif
				/* send an RTCP BYE packet with the old SSRC identifier */
				
				/* log occurrence of a collision */
				log_collision(rs, from, &now);
					
				/* create a new entry in the conflicting data or control
		   		source transport address list and mark current time	*/
				new_collision(rs, from, &now);
					
				/* send an RTCP BYE packet with the old SSRC identifier */
				err = rtcp_stop(sid, reason);	// EVENT_BYE, &now);
				if (err > 0) {
					return err;
				}
				
				err = delete_member_by_ssrc(sid, rs->rtp_sendpkt.rtphdr.ssrc);
				if (err < 0) {
					fprintf(stderr, "delete_member_by_ssrc - %s\n", RTPStrError(err));
				}
					
				/* choose a new SSRC identifier */
				
				do {	
					/* Initialize random number generators with a random seed. */
					init_random_seed();
		
					/* initialization */
					rs->rtp_sendpkt.rtphdr.ssrc = random32(sid);
					//rs->rtp_sendpkt.rtphdr.ts = random32(sid);
					//rs->rtp_sendpkt.rtphdr.seq = random32(sid) % 65536;
					
#if RTP_DEBUG	
					printf("MSG	: rtcp_collision_resolution_loop_detection - new SSRC = %u\n", rs->rtp_sendpkt.rtphdr.ssrc); //DEBUG_ONLY
					//printf("MSG	: timestamp = %u\n", rs->rtp_sendpkt.rtphdr.ts); //DEBUG_ONLY
					//printf("MSG	: seqnum = %u\n", rs->rtp_sendpkt.rtphdr.seq); //DEBUG_ONLY
#endif					
					err = get_member_by_ssrc(rs, rs->rtp_sendpkt.rtphdr.ssrc, &mb);
					if (err) {
						fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s\n", RTPStrError(err));
					}					
					
				} while(mb);	// if mb != NULL, it means there is another ssrc collision with the other member

				rs->send_rtppkt_num	= 0;
				rs->send_rtpbytes_num	= 0;
				rs->send_rtcppkt_num	= 0;	// i guess
				rs->send_rtcpbytes_num	= 0;	// i guess			

				/* create a new entry in the source identifier table with the old SSRC plus 
				the source transport address from the data or control packet being processed */
				if (rs->rtcp_rule.we_sent) {
					err = add_new_member(sid, rs->rtp_sendpkt.rtphdr.ssrc, from, valid, TRUE, FALSE);
				}
				else {
					err = add_new_member(sid, rs->rtp_sendpkt.rtphdr.ssrc, from, valid, FALSE, FALSE);
				}
				if (err > 0) {
					return err;
				}	
				//init_seq(&(member->source), rtppkt.rtphdr);
				//member->source.max_seq 	= rtppkt.rtphdr.seq - 1;
				//member->source.probation= MIN_SEQUENTIAL;
									
				/* re-start the rtcp transmission */
				if (rs->rtcpopen) {
					err = rtcp_start(sid);
					if (err > 0) {
						return err;
					}
				}
									
				err = LOCAL_SOURCE_NEW_CONFLICT;;					
			}
		}
		else {
#if RTP_DEBUG	
			printf("MSG	: rtcp_collision_resolution_loop_detection - old member - case 3~~\n");//DEBUG

			printf("MSG	: rtcp_collision_resolution_loop_detection - check_in->sin_port = %d\n", ntohs(check_in->sin_port));//DEBUG
			printf("MSG	: rtcp_collision_resolution_loop_detection - from_in->sin_port = %d\n", ntohs(from_in->sin_port));//DEBUG
#endif			
			// everything is ok.
		}
	} // if member != NULL
	else {
		/* NewMember, SSRC or CSRC identifier is not found in the source identifier table */
		new_source = TRUE;
		
#if RTP_DEBUG	
		printf("MSG	: rtcp_collision_resolution_loop_detection - new member~~\n");//DEBUG
		fflush(stdout);
#endif		
		if (type_of_event(sid) != EVENT_BYE) {	// specified in A.7 of RFC-3550
			
			/* create a new entry storing the data or control source transport address, 
			the SSRC or CSRC and other state */
			
			err = add_new_member(sid, ssrc, from, valid, FALSE, FALSE);
			if (err > 0) {
				return err;
			}
			
			/* get new member which we just create */
			err = get_member_by_ssrc(rs, ssrc, &member);
			if (err) {
				fprintf(stderr, "rtcp_collision_resolution_loop_detectio - %s, process is terminated\n", RTPStrError(err));
				exit(1);
			}
			
			rs->rtcp_rule.members++;
		}
		else {
			return WE_ARE_SENDING_BYE;
			// do something ?? 
		}
				
	} // member == NULL
	
	/* update member info. NOTE, this soesn't update the "last_sr_recv_time" and "jitter" info. */
	update_member_info(rs, member, FALSE, &now);
	
	return err;

} /* rtcp_collision_resolution_loop_detection */


/*
 * add_send_sdes_list - Add the ssrc of member into list.
 * 
 * the SDES items of member in list will be sent by local member in RTCP packet.
 */
rtperror
add_send_sdes_list(session_id sid, u_int32 ssrc){
	
	rtperror		err = 0;
	session			*rs;
	
	/* get the rtp session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	if (!(rs->send_sdes_list)) {	// NULL, first time to add ssrc into list.
		rs->send_sdes_list = (send_sdes_list_t *) malloc (sizeof(send_sdes_list_t));
		rs->send_sdes_list->ssrc = ssrc;
		rs->send_sdes_list->next = NULL;
	
#if RTP_DEBUG	
		printf("add_send_sdes_list - %u\n", rs->send_sdes_list->ssrc);
#endif
		return 0;
	}
	else {
		send_sdes_list_t *sdes;
		sdes = rs->send_sdes_list;
		while (sdes->ssrc != ssrc) {	// ssrc in list must be unique.
			if (sdes->next) {
				sdes = sdes->next;
			}
			else {	// add it into the end of list
				send_sdes_list_t *new_sdes;
				new_sdes = (send_sdes_list_t *) malloc (sizeof(send_sdes_list_t));
				new_sdes->ssrc = ssrc;
				new_sdes->next = NULL;
				sdes->next = new_sdes;
				
				return 0;
			}
		}
#if RTP_DEBUG	
		printf("MSG	: add_send_sdes_list - no match with %u\n", ssrc);	
#endif
		return MEMBER_ALREADY_EXIST;
	}
	
} /* add_send_sdes_list */


/*
 * del_send_sdes_list - Delete the ssrc of member into list.
 *
 * The SDES items of member in list will be sent by local member in RTCP packet.
 */
rtperror
del_send_sdes_list(session_id sid, u_int32 ssrc){
	
	rtperror		err = 0;
	session			*rs;
	send_sdes_list_t	*sdes = NULL, *previous = NULL;
	
	/* get the rtp session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	sdes = rs->send_sdes_list;
	
	while (sdes) {
		if (sdes->ssrc == ssrc) {
			if (!previous) {
				rs->send_sdes_list = sdes->next;
				free(sdes);
					
				fprintf(stderr, "MSG	: del_send_sdes_list - del ssrc: %lu\n", ssrc);	
				return err;
			}
			else {
				previous->next = sdes->next;
				free(sdes);
				
				fprintf(stderr, "MSG	: del_send_sdes_list - del ssrc: %lu\n", ssrc);	
				return err;
			}
		}
		previous = sdes;
		sdes = sdes->next;
	}
	
	fprintf(stderr, "MSG	: del_send_sdes_list - no match with %lu\n", ssrc);
	return RTP_CANT_MATCH_MEMBER;
	
} /* del_send_sdes_list */



/*
 * add_send_rb_list - Add the ssrc of member into list..
 */
rtperror
add_send_rb_list(session_id sid, u_int32 ssrc){
	
	rtperror		err = 0;
	session			*rs;
	
	/* get the rtp session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}

#if RTP_DEBUG	
	printf("before add_send_rb_list - ssrc = %u\n", ssrc);				
#endif

	if (!(rs->send_rb_list)) {	// NULL, first time to add ssrc into list.
		rs->send_rb_list = (send_rb_list_t *) malloc (sizeof(send_rb_list_t));
		rs->send_rb_list->ssrc = ssrc;
		rs->send_rb_list->next = rs->send_rb_list;
		
#if RTP_DEBUG	
		printf("after add_send_rb_list - rb->ssrc = %u\n", rs->send_rb_list->ssrc);				
#endif
		return err;
	}
	else {
		send_rb_list_t *rb;
		rb = rs->send_rb_list;
		while (rb->ssrc != ssrc) {	// ssrc in list must be unique.
			if (rb->next != rs->send_rb_list) {
				rb = rb->next;
			}
			else {	// add it into the last position of rs->send_rb_list
				send_rb_list_t *new_rb;
				rb = rs->send_rb_list;
				new_rb = (send_rb_list_t *) malloc (sizeof(send_rb_list_t));
				new_rb->ssrc = ssrc;
				new_rb->next = rb->next;
				rb->next = new_rb;

#if RTP_DEBUG	
				printf("after add_send_rb_list - rb->ssrc = %u\n", new_rb->ssrc);				
#endif
				return 0;
			}
		}
#if RTP_DEBUG	
		printf("MSG	: add_send_rb_list - no match with %u\n", ssrc);	
#endif
		return MEMBER_ALREADY_EXIST;
	}
	
} /* add_send_rb_list */


/*
 * del_send_rb_list - Delete the ssrc of member into list.
 */
rtperror
del_send_rb_list(session_id sid, u_int32 ssrc){
	
	rtperror		err = 0;
	session			*rs;
	send_rb_list_t		*rb = NULL, *previous = NULL;
	
	/* get the rtp session of session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	rb = rs->send_rb_list;
	
	while (rb) {
		if (rb->ssrc == ssrc) {
			if (!previous) {
				rb->next = NULL;
				free(rb);
				rs->send_rb_list = NULL;
					
				return err;
			}
			else {
				previous->next = rb->next;
				free(rb);
				
				return err;
			}
		}
		previous = rb;
		rb = rb->next;
		if (rb == rs->send_rb_list) {//cycle
			break;
		}
	}

	fprintf(stderr, "MSG	: del_send_rb_list - no match with %lu\n", ssrc);	
	return RTP_CANT_MATCH_MEMBER;
	
} /* del_send_rb_list */

