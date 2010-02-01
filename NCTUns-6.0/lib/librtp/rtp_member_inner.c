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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include "rtp_member_inner.h"
#include "rtp_session_api.h"

extern rtperror get_member_by_ssrc(session_id sid, u_int32 ssrc, member_t **member);

double	recent_lossrate[RTP_MAX_NUM_SESSION];
u_int32	recent_ssrc[RTP_MAX_NUM_SESSION];

/********************************************************************/


/* init_seq */
void
init_seq(source_t *s, rtp_hdr_t hdr){
	
	s->base_seq 		= hdr.seq;
	s->max_seq 		= hdr.seq;
	s->bad_seq 		= RTP_SEQ_MOD + 1;	/* so seq == bad_seq is false */
	s->cycles 		= 0;
	s->received 		= 0;
	s->received_prior 	= 0;
	s->expected_prior 	= 0;
	/* other initialization */
	//transit
	//jitter
	//
} /* init_seq */


/* update_seq */
int
update_seq(source_t *s, rtp_hdr_t hdr){
	
	u_int16 udelta = hdr.seq - s->max_seq;
	
	/*
	 * Source is not valid until MIN_SEQUENTIAL packets with
 	 * sequential sequence numbers have been received.
 	 */
 	if (s->probation) {
 		
		/* packet is in sequence */
		if (hdr.seq == s->max_seq + 1) {
			s->probation--;
			s->max_seq = hdr.seq;
			if (s->probation == 0) {
				init_seq(s, hdr);
				s->received++;
				return 1;
			}
		} 
		else {
			s->probation = MIN_SEQUENTIAL - 1;
			s->max_seq = hdr.seq;
		}
		return 0;
	} 
	else if (udelta < MAX_DROPOUT) {
		/* in order, with permissible gap */
		if (hdr.seq < s->max_seq) {
			/*
			 * Sequence number wrapped - count another 64K cycle.
			 */
			s->cycles += RTP_SEQ_MOD;
		}
		s->max_seq = hdr.seq;
	} 
	else if (udelta <= RTP_SEQ_MOD - MAX_MISORDER) {
		/* the sequence number made a very large jump */
		if (hdr.seq == s->bad_seq) {
			/*
			* Two sequential packets -- assume that the other side
			* restarted without telling us so just re-sync
			* (i.e., pretend this was the first packet).
			*/
			init_seq(s, hdr);
		}
		else {
			s->bad_seq = (hdr.seq + 1) & (RTP_SEQ_MOD-1);
			return 0;
		}
	} 
	else {
		/* duplicate or reordered packet */
	}
	s->received++;
	return 1;
	
} /* update_seq */


/* find_in_conflict_list(sid, from) */
int 
find_in_conflict_list(session *rs, struct sockaddr *from){
	
	conflict_list_t		*c;
	struct sockaddr_in	*check_in, *from_in;
	
	c = rs->conflict_list;
	
	check_in = (struct sockaddr_in *) &(c->conflict_addr);
	from_in = (struct sockaddr_in *) (from);
	
	while (c) {
		if (check_in->sin_family == from_in->sin_family &&
		    check_in->sin_addr.s_addr == from_in->sin_addr.s_addr &&
		    check_in->sin_port == from_in->sin_port) {
		    	/* source transport address is found in the list of conflicting data or 
		    	   control source transport addresses */
#if RTP_DEBUG	
			printf("MSG	: find_in_conflict_list - match conflict address whose ssrc = %u\n", c->ssrc);
#endif
		    	return 1;
		}
		c = c->next;
	}
	
#if RTP_DEBUG	
	printf("MSG	: find_in_conflict_list - no match any conflict address\n");
#endif
	return 0;
		
} /* find_in_conflict_list */


/* update_conflict_info */
void 
update_conflict_info(session *rs, struct sockaddr *from, struct timeval *now){
		
	conflict_list_t *c;
	struct sockaddr_in	*check_in, *from_in;
	
	c = rs->conflict_list;
	
	check_in = (struct sockaddr_in *) &(c->conflict_addr);
	from_in = (struct sockaddr_in *) (from);
	
	while (c) {
		if (check_in->sin_family == from_in->sin_family &&
		    check_in->sin_addr.s_addr == from_in->sin_addr.s_addr &&
		    check_in->sin_port == from_in->sin_port) {
		    	
		    	/* mark current time */
		    	c->last_occur_time = *now;
		    	
#if RTP_DEBUG	
		    	printf("MSG	: update_conflict_info - update ssrc = %u\n", c->ssrc);
#endif		    	
		    	/* reset conflict count */
		    	c->count = 0;
		    	
		    	break;
		}
		c = c->next;
	}
} /* update_conflict_info */
 

/* log_collision */
void 
log_collision(session *rs, struct sockaddr *from, struct timeval *now){
	
	conflict_list_t *c;
	struct sockaddr_in	*check_in, *from_in;
		    		
	c = (conflict_list_t *) malloc(sizeof(conflict_list_t));
	
	c->ssrc = rs->rtp_sendpkt.rtphdr.ssrc;
	
	check_in = (struct sockaddr_in *) &(c->conflict_addr);
	from_in = (struct sockaddr_in *) (from);
	
	c->last_occur_time = *now;
	
#if RTP_DEBUG	
	printf("MSG	: log_collision - log ssrc = %u\n, time.tv_sec = %ld, time.tv_usec = %ld\n", c->ssrc, c->last_occur_time.tv_sec, c->last_occur_time.tv_usec);
#endif
	
	check_in->sin_family = from_in->sin_family;
	check_in->sin_addr.s_addr = from_in->sin_addr.s_addr;
	check_in->sin_port = from_in->sin_port;
	
	c->next = rs->collsion_log;
	rs->collsion_log = c;
	
} /* log_collision */


/* new_collision */
void 
new_collision(session *rs, struct sockaddr *from, struct timeval *now){
	
	conflict_list_t *c;
	struct sockaddr_in	*check_in, *from_in;
	
	c = (conflict_list_t *) malloc(sizeof(conflict_list_t));
	
	c->ssrc = rs->rtp_sendpkt.rtphdr.ssrc;

	check_in = (struct sockaddr_in *) &(c->conflict_addr);
	from_in = (struct sockaddr_in *) (from);
		
	c->last_occur_time = *now;
	
#if RTP_DEBUG	
	printf("MSG	: new_collision - log ssrc = %u\n, time.tv_sec = %ld, time.tv_usec = %ld\n", c->ssrc, c->last_occur_time.tv_sec, c->last_occur_time.tv_usec);
#endif
	
	check_in->sin_family = from_in->sin_family;
	check_in->sin_addr.s_addr = from_in->sin_addr.s_addr;
	check_in->sin_port = from_in->sin_port;
	
	c->next = rs->conflict_list;
	rs->conflict_list = c;
	
} /* new_collision */


/* 
 * update_member_info - update last_rtp(rtcp)_recv_time.
 *
 * update_source_info - update jitter, specified in A.8 of RFC-3550
 *
 * As each data packet arrives, the jitter estimate is updated:
 * int transit = arrival - r->ts;	// r->ts, the timestamp from the incoming packet.
 * int d = transit - s->transit;	// arrival, the current time in the same units.
 * s->transit = transit;		// s->transit holds the relative transit time for the previous packet
 * if (d < 0) d = -d;			// s->jitter holds the estimated jitter.
 *							// D(i, j) = (Rj - Ri) - (Sj - Si) = (Rj - Sj) - (Ri - Si)
 * s->jitter += (1./16.) * ((double)d - s->jitter);	// J(i) = J(i - 1) + [ ( |D(i - 1, i)| - J(i - 1) ) / 16 ]
 *
 * a reception report block (to which rr points) is generated for this member, the current jitter estimate is returned:
 * rr->jitter = (u_int32) s->jitter;
 */
void
update_member_info(session *rs, member_t *member, int is_rtp, struct timeval *now){
	
	if (is_rtp) {
		/* receiver rtp packet */
		
		member->last_rtp_recv_time.tv_sec = now->tv_sec;
		member->last_rtp_recv_time.tv_usec = now->tv_usec;
#if RTP_DEBUG	
		printf("MSG	: update_member_info - %u last_rtp_recv_time = %lf\n", member->ssrc, timeval_to_double(member->last_rtp_recv_time));//DEBUG
#endif		
		/* update jitter */
		if (member->source.jitter == -1) {
			member->source.jitter = 0;
			member->source.transit = timeval_to_rtptime(&(member->last_rtp_recv_time), member->rtp_recvpkt.rtphdr.pt, rs->rtp_timerate) - (member->rtp_recvpkt.rtphdr.ts);
#if RTP_DEBUG	
			printf("MSG	: jitter - first time , arrival = %u\n", timeval_to_rtptime(&(member->last_rtp_recv_time), member->rtp_recvpkt.rtphdr.pt, rs->rtp_timerate));//DEBUG
			printf("MSG	: jitter - first time , rtp in header = %u\n", member->rtp_recvpkt.rtphdr.ts);//DEBUG
			printf("MSG	: jitter - first time , member->source.transit = %u\n", member->source.transit);//DEBUG
#endif
		}
		else {
			u_int32 transit = timeval_to_rtptime(&(member->last_rtp_recv_time), member->rtp_recvpkt.rtphdr.pt, rs->rtp_timerate) - (member->rtp_recvpkt.rtphdr.ts);
			int d = transit - member->source.transit;
#if RTP_DEBUG	
			printf("MSG	: jitter - transit , arrival = %u\n", timeval_to_rtptime(&(member->last_rtp_recv_time), member->rtp_recvpkt.rtphdr.pt, rs->rtp_timerate));//DEBUG
			printf("MSG	: jitter - transit , rtp in header = %u\n", member->rtp_recvpkt.rtphdr.ts);//DEBUG
			printf("MSG	: jitter - transit = %u, member->source.transit = %u, d = %i\n", transit, member->source.transit, d);//DEBUG
#endif
			member->source.transit = transit;
			if (d < 0) {
				d = -d;
			}
			member->source.jitter += (1./16.) * ((double)d - member->source.jitter);
#if RTP_DEBUG	
			printf("MSG	: jitter - (double)d - member->source.jitter = %lf, member->source.jitter = %ld\n", ((double)d - member->source.jitter), member->source.jitter);//DEBUG
#endif
		}		
	}
	else {
		/* receiver rtcp packet */
		member->last_rtcp_recv_time.tv_sec = now->tv_sec;
		member->last_rtcp_recv_time.tv_usec = now->tv_usec;
#if RTP_DEBUG	
		printf("MSG	: update_member_info - member[%u] update recv_rtcp_time.tv_sec=%ld, recv_rtcp_time.tv_usec=%ld", member->ssrc, member->last_rtcp_recv_time.tv_sec, member->last_rtcp_recv_time.tv_usec);//DEBUG
#endif
	}

} /* update_member_info */


double 
get_recent_recv_loosrate(session_id sid, u_int32 *ssrc){
	*ssrc = recent_ssrc[sid];
	return recent_lossrate[sid];
}
			
	
/* update_rr */
rtperror update_rr(session_id sid, member_t *rr_member, rtcp_rr_t *rtcp_rr, double ntp_cur, member_t *rtcp_member){

	double			delay, lsr, dlsr, fraction, sec;
	long			s, f;
	int			ori_null = FALSE;
	
	char 			*LOC_IP, *FOR_IP, *ORI_IP;
	struct sockaddr_in	*original_in, *forward_in;
	rtperror		err = 0;
	session 		*rs;
	
	/* get the session of session_list[sid] to " rs " */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
		
	if (rtcp_member->fromaddr[0].sa_family != AF_UNSPEC) {
		forward_in = (struct sockaddr_in *) &(rtcp_member->fromaddr[0]);
	}
	else if (rtcp_member->fromaddr[1].sa_family != AF_UNSPEC) {
		forward_in = (struct sockaddr_in *) &(rtcp_member->fromaddr[1]);
	}		

	if (rr_member->fromaddr[0].sa_family != AF_UNSPEC) {
		original_in = (struct sockaddr_in *) &(rr_member->fromaddr[0]);
	}
	else if (rr_member->fromaddr[1].sa_family != AF_UNSPEC) {
		original_in = (struct sockaddr_in *) &(rr_member->fromaddr[1]);
	}
	else {
#if RTP_DEBUG	
		printf("MSG	: original_in is NULL\n");
#endif
		ori_null = TRUE;
	}

	memcpy(&(rr_member->rtcp_rr), rtcp_rr, sizeof(rtcp_rr_t));
	
	if (ori_null) {
		return 0;
	}
	
	FOR_IP = strdup(inet_ntoa(forward_in->sin_addr));		
	LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
	ORI_IP = strdup(inet_ntoa(original_in->sin_addr));
	
#if RTP_DEBUG	
	printf("MSG	: update_rr - FOR_IP = %s, LOC_IP = %s, ORI_IP = %s\n", FOR_IP, LOC_IP, ORI_IP);
#endif
	
	s = rr_member->rtcp_rr.lsr >> 16;
	
	f = rr_member->rtcp_rr.lsr & 0x0000ffff;

#if RTP_DEBUG	
	printf("MSG	: update_rr - s = %ld, f = %ld\n", s, f);
#endif
	fraction = ((double) f / 65536.);
	sec = (double) s;
	lsr = sec + fraction;
	
	dlsr = ((double) (rr_member->rtcp_rr.dlsr) / 65536.);
	
#if RTP_DEBUG	
	printf("MSG	: update_rr - lsr = %lf, dlsr = %lf\n", lsr, dlsr);
#endif
	
	delay = ntp_cur - lsr - dlsr;

	recent_lossrate[sid]	= ((double) rr_member->rtcp_rr.fraction / 256.);
	recent_ssrc[sid]	= rr_member->ssrc;
#if RTP_DEBUG	
	printf("MSG	: recent_lossrate[sid] = %lf\n", recent_lossrate[sid]);
#endif
	
	if (LOG_RECV_RTCP) {
			
		FILE	*fp, *f1, *f2, *f3, *f4;
		char	openlost[80], lost[10] = ".pktlost";
		char	openjitter[80], jitter[10] = ".jitter";
		char	opendelay[80], ppdelay[10] = ".delay";
		char	openlossrate[80], lossrate[15] = ".pktlossrate";
		char	forward[20];
		char	tmp[20];
		char	open[40];
	
		//sprintf(forward, "%u", ssrc);
		sprintf(forward, "%s", FOR_IP);
		
		//sprintf(tmp, "%u", member->rtcp_rr.ssrc);
		sprintf(tmp, "%s", ORI_IP);
			
		//sprintf(openlost, "%u", rs->rtp_sendpkt.rtphdr.ssrc);
		sprintf(openlost, "%s", LOC_IP);
		strcat(openlost, "-");
		strcat(openlost, forward);
		strcat(openlost, "-");
		strcat(openlost, tmp);
		strcat(openlost, lost);
			
		//sprintf(openjitter, "%u", rs->rtp_sendpkt.rtphdr.ssrc);
		sprintf(openjitter, "%s", LOC_IP);
		strcat(openjitter, "-");
		strcat(openjitter, forward);
		strcat(openjitter, "-");
		strcat(openjitter, tmp);
		strcat(openjitter, jitter);
			
		//sprintf(opendelay, "%u", rs->rtp_sendpkt.rtphdr.ssrc);
		sprintf(opendelay, "%s", LOC_IP);
		strcat(opendelay, "-");
		strcat(opendelay, forward);
		strcat(opendelay, "-");
		strcat(opendelay, tmp);
		strcat(opendelay, ppdelay);
			
		//sprintf(openlossrate, "%u", rs->rtp_sendpkt.rtphdr.ssrc);
		sprintf(openlossrate, "%s", LOC_IP);
		strcat(openlossrate, "-");
		strcat(openlossrate, forward);
		strcat(openlossrate, "-");
		strcat(openlossrate, tmp);
		strcat(openlossrate, lossrate);
								
		if ( (f1 = fopen(openlost, "r+")) == NULL) {	// file is not exist.
			f1 = fopen(openlost, "a+");
                        if (f1 != NULL) {
                                fprintf(f1, "#\tNTP_initial_time: %lf\n", ntp_cur);
				fprintf(f1, "#\tThis file includes - cumulative number of packets lost from IP:%s\n", LOC_IP);
				fprintf(f1, "#\tUNIT:\tTIME\tcumulative number of packets lost\n");
                                fprintf(f1, "\t\t%d\t\t%d\n", 0, rr_member->rtcp_rr.lost);
                        }
                        else {
                                fprintf(stderr, "can't open file - %s\n", openlost);
                        }
		}
		else {
			char    *token, line[100];
			fgets(line, 100, f1);
			token = strtok(line, " \t\n");
			token = strtok(NULL, " \t\n");
			token = strtok(NULL, " \t\n");
			fclose(f1);
			f1 = fopen(openlost, "a+");
			if (f1 != NULL) {
				fprintf(f1, "\t\t%lf\t%d\n", (ntp_cur - atof(token)), rr_member->rtcp_rr.lost);
				fflush(f1);
			}
			else {
				fprintf(stderr, "can't open file - %s\n", openlost);
			}
		}	
		fclose(f1);

                if ( (f2 = fopen(openjitter, "r")) == NULL) {     // file is not exist.
                        f2 = fopen(openjitter, "a+");
                        if (f2 != NULL) {
                                fprintf(f2, "#\tNTP_initial_time: %lf\n", ntp_cur);
                                fprintf(f2, "#\tThis file includes - interarrival jitter from IP:%s\n", LOC_IP);
                                fprintf(f2, "#\tUNIT:\tTIME\tinterarrival jitter\n");
                                fprintf(f2, "\t\t%d\t\t%lu\n", 0, rr_member->rtcp_rr.jitter);
                                fflush(f2);
                        }
                        else {
                                fprintf(stderr, "can't open file - %s\n", openjitter);
                        }
                }
                else {
                        char    *token, line[100];
			fgets(line, 100, f1);
                        token = strtok(line, " \t\n");
                        token = strtok(NULL, " \t\n");
                        token = strtok(NULL, " \t\n");
			fclose(f2);
			f2 = fopen(openjitter, "a+");
			if (f2 != NULL) {
				fprintf(f2, "\t\t%lf\t%lu\n", (ntp_cur - atof(token)), rr_member->rtcp_rr.jitter);
				fflush(f2);
			}
			else {
				fprintf(stderr, "can't open file - %s\n", openjitter);
			}			
		}
		fclose(f2);

                if ( (f3 = fopen(opendelay, "r")) == NULL) {     // file is not exist.
                        f3 = fopen(opendelay, "a+");
                        if (f3 != NULL) {
                                fprintf(f3, "#\tNTP_initial_time: %lf\n", ntp_cur);
                                fprintf(f3, "#\tThis file includes - round-trip propagation delay from IP:%s\n", LOC_IP);
                                fprintf(f3, "#\tUNIT:\tTIME\tround-trip propagation delay\n");
                                fprintf(f3, "\t\t%d\t\t%d\n", 0, 0);
                                fflush(f3);
                        }
                        else {
                                fprintf(stderr, "can't open file - %s\n", opendelay);
                        }
                }
                else {
                        char    *token, line[100];
			fgets(line, 100, f1);
                        token = strtok(line, " \t\n");
                        token = strtok(NULL, " \t\n");
                        token = strtok(NULL, " \t\n");
                        fclose(f3);
			f3 = fopen(opendelay, "a+");
			if (f3 != NULL) {
				fprintf(f3, "\t\t%lf\t%lf\n", (ntp_cur - atof(token)), delay);
				fflush(f3);
			}
			else {
				fprintf(stderr, "can't open file - %s\n", opendelay);
			}
		}			
		fclose(f3);

                if ( (f4 = fopen(openlossrate, "r")) == NULL) {     // file is not exist.
                        f4 = fopen(openlossrate, "a+");
                        if (f4 != NULL) {
                                fprintf(f4, "#\tNTP_initial_time: %lf\n", ntp_cur);
                                fprintf(f4, "#\tThis file includes - fraction lost from IP:%s\n", LOC_IP);
                                fprintf(f4, "#\tUNIT:\tTIME\tfraction lost\n");
                                fprintf(f4, "\t\t%d\t\t%lf\n", 0, ((double) rr_member->rtcp_rr.fraction / 256.));
                                fflush(f4);
                        }
                        else {
                                fprintf(stderr, "can't open file - %s\n", openlossrate);
                        }
                }
                else {
                        char    *token, line[100];
			fgets(line, 100, f1);
                        token = strtok(line, " \t\n");
                        token = strtok(NULL, " \t\n");
                        token = strtok(NULL, " \t\n");
                        fclose(f4);
			f4 = fopen(openlossrate, "a+");
			if (f4 != NULL) {
				fprintf(f4, "\t\t%lf\t%lf\n", (ntp_cur - atof(token)), ((double) rr_member->rtcp_rr.fraction / 256.));
				fflush(f4);
			}
			else {
				fprintf(stderr, "can't open file - %s\n", openlossrate);
			}
		}			
		fclose(f4);
		
		sprintf(open, "%s", LOC_IP);
		strcat(open, "_rtcp_recv.log");	
		fp = fopen(open, "a+");
		if (fp != NULL) {
			fprintf(fp, "\n");
			fprintf(fp, "member->rtcp_rr.ssrc = %lu\n", rr_member->rtcp_rr.ssrc);
			fprintf(fp, "MSG	: member->rtcp_rr.fraction = %u\n", rr_member->rtcp_rr.fraction);
			fprintf(fp, "MSG	: member->rtcp_rr.lost = %d\n", rr_member->rtcp_rr.lost);
			fprintf(fp, "MSG	: member->rtcp_rr.last_seq = %lu\n", rr_member->rtcp_rr.last_seq);
			fprintf(fp, "MSG	: member->rtcp_rr.jitter = %lu\n", rr_member->rtcp_rr.jitter);
			fprintf(fp, "MSG	: member->rtcp_rr.lsr = %lu\n", rr_member->rtcp_rr.lsr);
			fprintf(fp, "MSG	: member->rtcp_rr.dlsr = %lu\n", rr_member->rtcp_rr.dlsr);
			fprintf(fp, "MSG	: round-trip propagation delay (A - LSR - DLSR) = %lf\n", delay);
			fprintf(fp, "-------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
	}
	
	free(FOR_IP);
	free(LOC_IP);
	free(ORI_IP);
	
	//printf("MSG	: receive_rtcp_pkt - sec = %lf, fraction = %lf\n", sec, fraction);
	//printf("MSG	: update_rr - SSRC= %u, now = %lf, lsr = %lf, dlsr = %lf, round-trip delay = %lf, total = %ld\n", rr_member->rtcp_rr.ssrc, ntp_cur, lsr, dlsr, delay, total);

	return err;
	
} /* update_rr */
			
