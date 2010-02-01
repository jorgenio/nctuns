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
#include <netinet/in.h>
#include <arpa/inet.h>

#include "rtp_network_api.h"	// rtp_delete need to close connection if session is still in connecting.
#include "rtp_member_api.h"
#include "rtp_session_inner.h"


/********************************************************************/


/* the queue we can put event which is scheduled into it.*/
rtp_eq *rtpeq = NULL;


/********************************************************************/


/* 
 * rtcp_interval() - computes the deterministic calculated interval, 
 *                   which specified in RFC 3550.
 * algo.
 * 	if ( senders <= 1/4 members ) {
 * 		if (we_sent == TRUE )
 * 			constant C = avg+rtcp+size / ( 1/4 rtcp_bw ), constant n = number of senders.
 * 		else 
 * 			constant C = avg+rtcp+size / ( 3/4 rtcp_bw ), constant n = number of receivers.
 * 	}
 * 	else
 * 		constant C = avg+rtcp+size / rtcp_bw , constant n = total number of members.
 * 	if ( initial == true )
 * 		Tmin = 2.5
 * 	else
 * 		Tmin = 5
 * 	Td = max ( Tmin, n * C )
 * 	T = Td * random ( 0.5~1.5 )
 * 	return ( T / 1.21828 )
 */
rtperror
rtcp_interval(session_id sid, struct timeval *interval){
	
	rtperror	err = 0;
	session 	*rs;
	double 		t;	/* interval */
	int 		n;	/* no. of members for computation */
	double 		rtcp_min_time = RTCP_MIN_TIME;	/* RTCP_MIN_TIME defult is 5. */
	double		rtcp_bw;
	
        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	rtcp_bw = rs->rtcp_rule.rtcp_bw;
	
	/* 
	 * Very first call at application start-up uses half the min
	 * delay for quicker notification while still allowing some time
	 * before reporting for randomization and to learn about other
	 * sources so the report interval will converge to the correct
	 * interval more quickly.
	 */
	if (rs->rtcp_rule.initial) {
		rtcp_min_time /= 2;
	}

	/*
	 * Dedicate a fraction of the RTCP bandwidth to senders unless
	 * the number of senders is large enough that their share is
	 * more than that fraction.
	 */
	n = rs->rtcp_rule.members;
	
#if RTP_DEBUG	
	printf("MSG	: rtcp_interval - rs->rtcp_rule->members = %d\n", rs->rtcp_rule.members);//DEBUG
	printf("MSG	: rtcp_interval - rs->rtcp_rule->senders = %d\n", rs->rtcp_rule.senders);//DEBUG
#endif
	
	/* adjust rtcp_bw to */
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
	
	/*
	 * The effective number of sites times the average packet size is
	 * the total number of octets sent when each site sends a report.
	 * Dividing this by the effective bandwidth gives the time
	 * interval over which those packets must be sent in order to
	 * meet the bandwidth target, with a minimum enforced. In that
	 * time interval we send one report so this time is also our
	 * average time between reports.
	 */
	t = (rs->rtcp_rule.avg_rtcp_size) * n / rtcp_bw;
	
#if RTP_DEBUG	
	printf("MSG	: rtcp_interval - before rtcp time = %f\n", t);//DEBUG
#endif
	
	if (t < rtcp_min_time) {
		t = rtcp_min_time;
	}
	
#if RTP_DEBUG	
	printf("MSG	: rtcp_interval - after rtcp time = %f\n", t);//DEBUG
#endif
	
	/*
	 * To avoid traffic bursts from unintended synchronization with
	 * other sites, we then pick our actual next report interval as a
	 * random number uniformly distributed between 0.5*t and 1.5*t.
	 */
	t = t * (drand48() + 0.5);
	
	//if (TIMER_RECONDSIDERATION) {
	t = t / COMPENSATION;
	//}
	
	//printf("MSG	: rtcp_interval - interval = %f\n", t);
	
	*interval = double_to_timeval(t);
	
	return err;
	
} /* rtcp_interval */


/* 
 * rtcp_schedule - schedule  event in order.
 *
 * Events of any session of the process are scheduled 
 * in the rtpeq(rtp event queue).
 */
rtperror 
rtcp_schedule(session_id sid, event_type type, struct timeval *t){
	
	rtperror	err = 0;
	session		*rs;
	rtp_eq		*eq, *tmp, *previous;
	int		last = FALSE;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err) {
		return err;
	}
	
	eq = (rtp_eq *) malloc(sizeof(rtp_eq));
	if (!eq) { //NULL
		return  RTP_CANT_ALLOC_MEM;
	}
	
	eq->sid = sid;
	eq->timeout = *t;
	eq->type = type;
	eq->next = NULL;
		
	/* if the "eq" is earliest event in RTPqueue, it will be placed at first one.
	 * else, it will be placed by time information.
	 */
	if (!rtpeq) {
		/* eq is the first event in RTPqueue */
		rtpeq = eq;
	}
	else { // not NULL, something in queue
		tmp = rtpeq;
		previous = NULL;
	
		/* timeout is TRUE if time "timeout" is earlier than the time "t" */
		while (time_expire(&(tmp->timeout), t)) {
			
			if (tmp->next) {
				previous = tmp;
				tmp = tmp->next;
			}
			else {	// "eq" is the latest event in RTPqueue
				tmp->next = eq;
				last = TRUE;
				break;
			}
		}
		
		if (!last) {
			/* time "t" is earlier than time "timeout" */
			eq->next = tmp;
			
			if (previous) {
				previous->next = eq;
			}
			else { // eq is the first event in RTPqueue
				rtpeq = eq;
			}
		}
	}

	/* update schedule */
	rs->rtcp_rule.next_rtcp_send = eq->timeout;

#if RTP_DEBUG	
	printf("MSG	: rtcp_schedule - new event_sid = %d, event_type = %d, \n", eq->sid, eq->type);//DEBUG
	printf("MSG	: rtcp_schedule - new event_timeout.tv_sec = %ld, event_timeout.tv_usec = %ld\n", rs->rtcp_rule.next_rtcp_send.tv_sec, rs->rtcp_rule.next_rtcp_send.tv_usec);//DEBUG
#endif
		
	/* check if something is on_expire */
	err = check_on_expire();
	if (err > 0) {
		return err;
	}
	
	return err;
	
} /* rtcp_schedule */


/*
 * rtcp_on_expire - it is called when the RTCP transmission timer expires.
 *		    we will send either rtcp packet or bye packet.
 */
rtperror
rtcp_on_expire(struct timeval *current){
	
	rtperror 	err = 0;
	session 	*rs;
	
	session_id	sid;
	event_type	type;
	struct timeval 	timeout;

	/* This function is responsible for deciding whether to send an
	 * RTCP report or BYE packet now, or to reschedule transmission.
	 * It is also responsible for updating the pmembers, initial, tp,
	 * and avg_rtcp_size state variables. This function should be
	 * called upon expiration of the event timer used by Schedule().
	 */
	
	struct timeval interval, now, tmp;	/* Interval */	
	
	memcpy(&now, current, sizeof(struct timeval));
	
	if (!rtpeq) {	/* queue shoud not be empty. */
		return NO_EVENT_IN_QUEUE;
	}
	
	sid = rtpeq->sid;
	type = rtpeq->type;
	timeout = rtpeq->timeout;
	
	/* free the event which id on_expire */
	if (rtpeq->next) {
		rtp_eq *eq = rtpeq;
		rtpeq = rtpeq->next;
		free(eq);
	}
	else {
		free(rtpeq);
		rtpeq = NULL;
	}
	
        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	/* In the case of a BYE, we use "timer reconsideration" to
	 * reschedule the transmission of the BYE if necessary 
	 */
	
	if (type == EVENT_BYE) {
		
		if (rs->rtcp_rule.initial && rs->send_rtppkt_num == 0) {
			/* no send any pkt before */
			return 0;
		}
		else {		
			/* get the rtcp interval*/
			err = rtcp_interval(sid, &interval);
			if (err > 0) {
				return err;
			}
			
			rs->rtcp_rule.next_rtcp_send = add_timeval(&(rs->rtcp_rule.last_rtcp_send), &interval);
					
			if ( time_expire( &(rs->rtcp_rule.next_rtcp_send), &now)) { // && (rs->num_bye < 50) ) { // rs->rtcp_rule->next_rtcp_send < now
			
#if RTP_DEBUG	
				printf("MSG	: rtcp_on_expire - next_to_send->timeout.tv_sec = %ld, next_to_send->timeout.tv_usec = %ld\n", rs->rtcp_rule.next_rtcp_send.tv_sec, rs->rtcp_rule.next_rtcp_send.tv_usec);//DEBUG
				printf("MSG	: rtcp_on_expire - now.tv_sec = %ld, now.tv_usec = %ld\n", now.tv_sec, now.tv_usec);//DEBUG
#endif
		
				rs->rtcp_rule.last_rtcp_send.tv_sec = now.tv_sec;
				rs->rtcp_rule.last_rtcp_send.tv_usec = now.tv_usec;
				
				err = rtcp_send(sid, EVENT_BYE);
				if (err) {
					return err;
				}
				else if (err < 0) {
					fprintf(stderr, "rtcp_on_expire1- %s\n", RTPStrError(err));
				}
			}
			else if ( time_expire( &(rs->rtcp_rule.next_rtcp_send), &now) && ( (rs->rtcp_rule.pmembers - rs->rtcp_rule.members) > 50) ) {

#if RTP_DEBUG	
				printf("MSG	: rtcp_on_expire - SEND BYE MEMBERS OVER 50!!!!!!!!!!!!!!!!!!!!\n");//DEBUG
				printf("MSG	: rtcp_on_expire - next_to_send->timeout.tv_sec = %ld, next_to_send->timeout.tv_usec = %ld\n", rs->rtcp_rule.next_rtcp_send.tv_sec, rs->rtcp_rule.next_rtcp_send.tv_usec);//DEBUG
				printf("MSG	: rtcp_on_expire - now.tv_sec = %ld, now.tv_usec = %ld\n", now.tv_sec, now.tv_usec);//DEBUG
#endif
				
				rs->rtcp_rule.last_rtcp_send = now;
				rs->rtcp_rule.members = TRUE;
				rs->rtcp_rule.pmembers = TRUE;
				rs->rtcp_rule.initial = TRUE;
				rs->rtcp_rule.we_sent = FALSE;
				rs->rtcp_rule.senders = 0;
				rs->rtcp_rule.avg_rtcp_size = (double) get_bye_len(rs);
				
				/* get the rtcp interval*/
				err = rtcp_interval(sid, &interval);
				if (err > 0) {
					return err;
				}
				
				rs->rtcp_rule.next_rtcp_send = add_timeval(&now, &interval);
				
				err = rtcp_schedule(sid, EVENT_BYE, &(rs->rtcp_rule.next_rtcp_send));
				if (err > 0) {
					return err;
				}								
				else if (err < 0) {
					fprintf(stderr, "rtcp_on_expire - %s\n", RTPStrError(err));
				}
			}	
			else {
#if RTP_DEBUG	
				printf("MSG	: rtcp_on_expire - don't send BYE at this TIME");//DEBUG
				printf("MSG	: rtcp_on_expire - next_to_send->timeout.tv_sec = %ld, next_to_send->timeout.tv_usec = %ld\n", rs->rtcp_rule.next_rtcp_send.tv_sec, rs->rtcp_rule.next_rtcp_send.tv_usec);//DEBUG
				printf("MSG	: rtcp_on_expire - now.tv_sec = %ld, now.tv_usec = %ld\n", now.tv_sec, now.tv_usec);//DEBUG
#endif
				
				err = rtcp_schedule(sid, EVENT_BYE, &(rs->rtcp_rule.next_rtcp_send));
				if (err > 0) {
					return err;
				}
				else if (err < 0) {
					fprintf(stderr, "rtcp_on_expire2- %s\n", RTPStrError(err));
				}
			}
		}
	}
	else if (type == EVENT_REPORT) {
		
		/* get the rtcp interval*/
		err = rtcp_interval(sid, &interval);
		if (err > 0) {
			return err;
		}
		
		rs->rtcp_rule.next_rtcp_send = add_timeval( &(rs->rtcp_rule.last_rtcp_send), &interval);
		
		if (rs->rtcp_rule.we_sent) {
			
			/* The reverse reconsideration algorithm described in Section 6.3.4 should be performed to possibly reduce 
			 * the delay before sending an SR packet. descripted in Sectoin 6.3.8 */
			if (rs->rtcp_rule.members < rs->rtcp_rule.pmembers) {
				err = reverse_reconsideration(sid, type, &now);
				if (err > 0) {
					return err;
				}
				else if (err < 0) {
					fprintf(stderr, "rtcp_on_expire3- %s\n", RTPStrError(err));
				}
			}
		}

#if RTP_DEBUG	
		printf("MSG	: rtcp_on_expire - next_to_send->timeout.tv_sec = %ld, next_to_send->timeout.tv_usec = %ld\n", rs->rtcp_rule.next_rtcp_send.tv_sec, rs->rtcp_rule.next_rtcp_send.tv_usec);//DEBUG
		printf("MSG	: rtcp_on_expire - now.tv_sec = %ld, now.tv_usec = %ld\n", now.tv_sec, now.tv_usec);//DEBUG
#endif
						
		if ( time_expire( &(rs->rtcp_rule.next_rtcp_send), &now) ) { // rs->rtcp_rule->next_rtcp_send < now
			
#if RTP_DEBUG	
			printf("MSG	: rtcp_on_expire - now, we are sending rtcp packet on [sid = %d, type = %d]\n", sid, type);//DEBUG
#endif
			
			err = rtcp_send(sid, EVENT_REPORT);
			if (err > 0) {
				return err;
			}
			else if (err < 0) {
				fprintf(stderr, "rtcp_on_expire4- %s\n", RTPStrError(err));
			}
			
			rs->rtcp_rule.last_rtcp_send = now;

			/* We must redraw the interval. Don't reuse the
			 * one computed above, since its not actually
			 * distributed the same, as we are conditioned
			 * on it being small enough to cause a packet to
			 * be sent */

			err = rtcp_interval(sid, &interval);
			if (err > 0) {
				return err;
			}
		
			tmp = add_timeval(&now, &interval);
			
			err = rtcp_schedule(sid, EVENT_REPORT, &tmp);
			if (err > 0) {
				return err;
			}
			else if (err < 0) {
				fprintf(stderr, "rtcp_on_expire5- %s\n", RTPStrError(err));
			}
			
			rs->rtcp_rule.initial = FALSE;
		
			rs->rtcp_rule.pmembers = rs->rtcp_rule.members;
			
			/* timeout member && sender per interval(only while we are sending rtcp packet) - start */
			err = rtp_timeout(sid, &now);
			if (err > 0) {
				return err;
			}
			else if (err < 0) {
				fprintf(stderr, "rtcp_on_expire7- %s\n", RTPStrError(err));
			}
		
			/* sender timeout for next interval */
			rs->sender_timeout.tv_sec =  rs->last_timeout.tv_sec;
			rs->sender_timeout.tv_usec =  rs->last_timeout.tv_usec;

#if RTP_DEBUG	
			printf("rtcp_on_expire - rs->sender_timeout = %lf\n", timeval_to_double(rs->sender_timeout));
#endif

			rs->last_timeout.tv_sec = now.tv_sec;
			rs->last_timeout.tv_usec = now.tv_usec;

#if RTP_DEBUG	
			printf("rtcp_on_expire - rs->last_timeout = %lf\n", timeval_to_double(rs->last_timeout));
#endif
			
			/* timeout member && sender per interval(only while we are sending rtcp packet) - end */
		}
		else {
			err = rtcp_schedule(sid, EVENT_REPORT, &(rs->rtcp_rule.next_rtcp_send));
			if (err > 0) {
				return err;
			}
			else if (err < 0) {
				fprintf(stderr, "rtcp_on_expire6- %s\n", RTPStrError(err));
			}
			rs->rtcp_rule.pmembers = rs->rtcp_rule.members;
		}
		
		/* timeout member && sender per interval 
		err = rtp_timeout(sid, &now);
		if (err > 0) {
			return err;
		}
		else if (err < 0) {
			fprintf(stderr, "rtcp_on_expire7- %s\n", RTPStrError(err));
		}
		
		// sender timeout for next interval 
		rs->sender_timeout.tv_sec =  rs->last_timeout.tv_sec;
		rs->sender_timeout.tv_usec =  rs->last_timeout.tv_usec;

		rs->last_timeout.tv_sec = now.tv_sec;
		rs->last_timeout.tv_usec = now.tv_usec;
		*/
	}

	return err;

} /* rtcp_on_expire */


/*
 * reverse_reconsideration - reconsideration algo.
 */
rtperror
reverse_reconsideration(session_id sid, event_type type, struct timeval *current){
	
	struct timeval	t; 
	double		tn, tc, tp;
	rtperror	err = 0;
	session		*rs;
	
        /* get the session from session_list[sid] to "rs" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	tn = timeval_to_double(rs->rtcp_rule.next_rtcp_send);
	tp = timeval_to_double(rs->rtcp_rule.last_rtcp_send);
	
	tc = timeval_to_double(*current);
	
	/* tn = tc + (members/pmembers) * (tn - tc) */
	tn = tc + (((double) rs->rtcp_rule.members) / (rs->rtcp_rule.pmembers)) * (tn - tc);
	
	/* tp = tc - (members/pmembers) * (tc - tp) */
	tp = tc - (((double) rs->rtcp_rule.members) / (rs->rtcp_rule.pmembers)) * (tc - tp);
	
	/* The next RTCP packet is rescheduled for transmission at time tn, which is now earlier. */
	t = double_to_timeval(tn);
	
#if RTP_DEBUG	
	printf("MSG	: reverse_reconsideration - new schedule time : timeout.tv_sec = %ld, timeout.tv_usec = %ld\n", t.tv_sec, t.tv_usec);//DEBUG
#endif
	
	rtcp_schedule(sid, type, &t);
	
	rs->rtcp_rule.pmembers = rs->rtcp_rule.members;

	return err;
	
} /* reverse_reconsideration */


/*
 * build_sdes - build SDES packet
 */
rtperror
build_sdes(session_id sid, char **buf, int *rtcplen){

	rtperror		err = 0;
	session			*rs;
	
	rtcp_common_t		*common;
	rtcp_sdes_t		*s;
	rtcp_sdes_item_t 	*rsp;
	send_sdes_list_t	*sdes_list = NULL;

	char			*ptr = (*buf);
	member_t 		*member = NULL;
	int			i, len, pad, num = 0;
	
#if RTP_DEBUG	
	printf("MSG	: build SDES~\n");
#endif
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	sdes_list = rs->send_sdes_list;
	if (!sdes_list) {
		return NO_SEND_SDES_LIST;
	}

	common = (rtcp_common_t *) ptr;
	ptr += 4;	/* rtcp_common_t size == 4 */
	
	
	/* get CSRC member to send SDES */
	/* i.e. num == rs->rtp_sendpkt_rtphdr.cc + 1 (ssrc) */

	if (LOG_SEND_RTCP) {
		FILE 	*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_send.log");

                fp = fopen(open, "a+");
	
		if (fp != NULL) {
			fprintf(fp, "SDES:\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
							
	while (sdes_list) {
		
		s = (rtcp_sdes_t *) ptr;
		
		err = get_member_by_ssrc(rs, sdes_list->ssrc, &member);
		if (err) {
			fprintf(stderr, "build_sdes - %s\n", RTPStrError(err));
		}
		sdes_list = sdes_list->next;
		
		s->src = member->ssrc;
		rsp = &s->item[0];
		
		for (i = 1; i < 9; i++) {
			/* Each chunk consists of an SSRC/CSRC identier followed by a list of zero or more items */
			/* if intervals is non-zero, means we will send this item in several intervals(CNAME must be 1) */
			if (member->sdes.intervals[i] && ( !(rs->number_of_rtcp_send % member->sdes.intervals[i]) ) ) {

				/* time to send this item */
					
				/* Each item consists of an 8-bit type field, 
				 * an 8-bit octet count describing the length of the text 
				 * (thus, not including this two-octet header), 
				 * and the text itself.*/
				if (member->sdes.length[i]) {
					rsp->type = i;
					rsp->length = member->sdes.length[i];
					memcpy(rsp->data, member->sdes.item[i], rsp->length);

					if (LOG_SEND_RTCP) {
						FILE 	*fp;
				                char    *LOC_IP, open[40];

               					LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                				sprintf(open, "%s", LOC_IP);
                				strcat(open, "_rtcp_send.log");

                				fp = fopen(open, "a+");
			
						if (fp != NULL) {
							fprintf(fp, "\n");
							fprintf(fp, "sdes[%d] = %s : length = %d\n", rsp->type, rsp->data, rsp->length);
							fprintf(fp, "-------------------------------------------------------\n");
							fflush(fp);
						}
						else {
							fprintf(stderr, "can't open file - %s\n", open);
						}
						fclose(fp);
						free(LOC_IP);
					}
						
					rsp = (rtcp_sdes_item_t *)&rsp->data[rsp->length];
				}	
				else {
					// may be other operation
				}
			}
		}
		
		/* Each chunk starts on a 32-bit boundary. */
		/* Note that this padding is separate from that indicated by the P bit in the RTCP header. */	
				
		/* terminate with end marker and pad to next 4-octet boundary */
		len = ((char *) rsp) - ptr;
		pad = 4 - (len & 0x3);
		ptr = (char *) rsp;
		while (pad--) *ptr++ = RTCP_SDES_END;
		num++;
	}
	
	*rtcplen += ptr - (*buf);
	
	len = (ptr - (*buf)) / 4 - 1;
	
	common->version = RTP_VERSION;
	common->p	= 0;
	common->pt	= RTCP_SDES;
	common->count	= num;
	common->length	= len;

	if (LOG_SEND_RTCP) {
		FILE 		*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_send.log");

                fp = fopen(open, "a+");

		if (fp != NULL) {
			fprintf(fp, "\n");
			fprintf(fp, "rtcppkt.common->version = %u\n", common->version);
			fprintf(fp, "rtcppkt.common->p = %u\n", common->p);
			fprintf(fp, "rtcppkt.common->pt = %u\n", common->pt);
			fprintf(fp, "rtcppkt.common->count = %u\n", common->count);
			fprintf(fp, "rtcppkt.common->length = %u\n", common->length);
			fprintf(fp, "-------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
	
#if RTP_DEBUG	
	printf("MSG	: build_sdes - SDES size = %d\n", (4 * (len + 1)));//DEBUG
#endif
		
	return err;
	
} /* build_sdes */


/* build_report_block */
rtperror 
build_report_block(session_id sid, rtcp_rr_t *block, member_t *member, struct timeval *cur){
	
	rtperror        err = 0;
	session		*rs;
	source_t	*s;
	double		current, last_sr;
	
	/* A.3 of RFC 3550 */
	u_int32		extended_max;
	u_int32		expected;
	int		lost;
	u_int32		expected_interval;
	u_int32		received_interval;
	int		lost_interval;
	int		fraction;
	u_int32		lsr;
	u_int32		dlsr;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	s = &(member->source);
	 
	/* A.3 Determining Number of Packets Expected and Lost */

	/* The number of packets expected can be computed by the receiver as the difference between 
	 * the highest sequence number received (s->max_seq) and the first sequence number received 
	 * (s->base_seq). Since the sequence number is only 16 bits and will wrap around, it is 
	 * necessary to extend the highest sequence number with the (shifted) count of sequence number 
	 * wraparounds (s->cycles). */
	extended_max = s->cycles + s->max_seq;
	expected = extended_max - s->base_seq + 1;
	
	/* The number of packets lost is dened to be the number of packets expected less the number of
	 * packets actually received. */
	lost = expected - s->received;

#if RTP_DEBUG	
	printf("MSG	: build_report_block - build block which member->ssrc = %u\n", member->ssrc);//DEBUG
	printf("MSG	: build_report_block - s->cycles = %u\t, s->max_seq = %u\t, extended_max = %u\t, s->base_seq = %u\t, expected = %u\t, s->received = %u\t, lost = %u\t, s->expected_prior = %u\t, s->received_prior = %u\n", s->cycles, s->max_seq, extended_max, s->base_seq, expected, s->received, lost, s->expected_prior, s->received_prior);//DEBUG
#endif

	/* Since this signed number is carried in 24 bits, it should be clamped at 0x7f for positive loss or
	 * 0x800000 for negative loss rather than wrapping around. 
	 * The fraction of packets lost during the last reporting interval (since the previous SR or RR packet
	 * was sent) is calculated from di1&ences in the expected and received packet counts across the
	 * interval, where expected_prior and received_prior are the values saved when the previous
	 * reception report was generated. */
	expected_interval = expected - s->expected_prior;
	s->expected_prior = expected;
	received_interval = s->received - s->received_prior;
	s->received_prior = s->received;
	lost_interval = expected_interval - received_interval;
	if (expected_interval == 0 || lost_interval <= 0) {
		fraction = 0;
	}
	else {
		fraction = (lost_interval << 8) / expected_interval;
	}
	
#if RTP_DEBUG	
	printf("MSG	: build_report_block - expected_interval = %u\t, received_interval = %u\t, lost_interval = %u\n", expected_interval, received_interval, lost_interval);//DEBUG
#endif
	
	/* lsr && dlsr */
	if (member->ntp_sec == 0 && member->ntp_frac == 0) {
		lsr = 0;
		dlsr = 0;
	}
	else {
		lsr = ( (member->ntp_sec << 16) | (member->ntp_frac >> 16) );		
		current = timeval_to_double(*cur);
		last_sr = timeval_to_double(member->last_sr_recv_time);
		
#if RTP_DEBUG	
		printf("MSG	: build_report_block - current=%lf, last_sr=%lf\n", current, last_sr);
#endif
		
		dlsr = (u_int32) ((current - last_sr) * 65536.);
	}
	
	block->ssrc 	= member->ssrc;
	block->fraction	= fraction;
	block->lost	= lost & 0x00ffffff;
	block->last_seq	= member->source.max_seq;
	block->jitter	= member->source.jitter;
	block->lsr	= lsr;
	block->dlsr	= dlsr;

	if (LOG_SEND_RTCP) {
		FILE 		*fp;
		char	*LOC_IP, open[40];

		LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
		sprintf(open, "%s", LOC_IP);
		strcat(open, "_rtcp_send.log");

		fp = fopen(open, "a+");	/* includes info. of all participants in this host. */		
		if (fp != NULL) {
			fprintf(fp, "\n");
			fprintf(fp, "block->ssrc = %lu\n", block->ssrc);
			fprintf(fp, "block->fraction = %u\n", block->fraction);
			fprintf(fp, "block->lost = %d\n", block->lost);
			fprintf(fp, "block->last_seq = %lu\n", block->last_seq);
			fprintf(fp, "block->jitter = %lu\n", block->jitter);
			fprintf(fp, "block->lsr = %lu\n", block->lsr);
			fprintf(fp, "block->dlsr = %lu\n", block->dlsr);
			fprintf(fp, "-------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
		
	return err;
	
} /* build_report_block */


/*
 * build_sr - build " sender report " rtcp packet.
 */
rtperror 
build_sr(session_id sid, char **buf, int *rtcplen, send_rb_list_t *original_rb, int *remainder, struct timeval *cur){
	
	rtperror	err = 0;
	session		*rs;
	rtcp_t		rtcppkt;
	char		*ptr = (*buf);
	ntp_t		ntp;
	member_t 	*member;
	int 		index = 0, len, tmp;
	send_rb_list_t	*rb;

        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	tmp = *remainder;
		
	if (tmp < (4 + 24 * 2) ) { // common + rtcp_sr + rr, at least send a reception report.
		return BUFFER_NOT_ENOUGH;
	}	
		
	rtcppkt.common = (rtcp_common_t *) ptr;
	ptr += 4;	/* rtcp_common_t size == 4 */
	tmp -= 4;
		
	rtcppkt.r.sr.rtcp_sr = (rtcp_sr_t *) ptr;
	ptr += 24;	/* rtcp_sr_t size == 24 */
	tmp -= 24;
			
	rtcppkt.r.sr.rtcp_sr->ssrc = rs->rtp_sendpkt.rtphdr.ssrc;
	
	/* ntp part */
	ntp = timeval_to_ntp(*cur);
	rtcppkt.r.sr.rtcp_sr->ntp_sec = ntp.sec;
	rtcppkt.r.sr.rtcp_sr->ntp_frac = ntp.frac;
		
	rtcppkt.r.sr.rtcp_sr->rtp_ts = rs->rtp_sendpkt.rtphdr.ts;
	rtcppkt.r.sr.rtcp_sr->psent = rs->send_rtppkt_num;
	rtcppkt.r.sr.rtcp_sr->osent = rs->send_rtpbytes_num;

	if (LOG_SEND_RTCP) {
		
		FILE 	*fp;
		char	*LOC_IP, open[40];

		LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
		sprintf(open, "%s", LOC_IP);
		strcat(open, "_rtcp_send.log");

		fp = fopen(open, "a+");	/* includes info. of all participants in this host. */		
		if (fp != NULL) {
			fprintf(fp, "SR:\n");
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->ssrc = %lu\n", rtcppkt.r.sr.rtcp_sr->ssrc);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->ntp_sec = %lu\n", rtcppkt.r.sr.rtcp_sr->ntp_sec);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->ntp_frac = %lu\n", rtcppkt.r.sr.rtcp_sr->ntp_frac);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->rtp_ts = %lu\n", rtcppkt.r.sr.rtcp_sr->rtp_ts);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->psent = %lu\n", rtcppkt.r.sr.rtcp_sr->psent);
			fprintf(fp, "rtcppkt.r.sr.rtcp_sr->osent = %lu\n", rtcppkt.r.sr.rtcp_sr->osent);
			fprintf(fp, "-------------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
	
	rb = rs->send_rb_list;

	if (rb) {
		do {
			err = get_member_by_ssrc(rs, rb->ssrc, &member);
			if (err) {
				fprintf(stderr, "build_sr - %s\n", RTPStrError(err));//DEBUG
				return err;
			}
			
			if (member->ssrc != rs->rtp_sendpkt.rtphdr.ssrc) {

				rtcppkt.r.sr.rr[index] = (rtcp_rr_t *) ptr;
				ptr += 24; /* rtcp_rr_t size == 24 */	
	                        
				err = build_report_block(sid, rtcppkt.r.sr.rr[index], member, cur);
				if (err > 0) {
					return err;
				}

				index++;
				tmp -= 24;
			}
			
			rb = rb->next;
		}
		while ( (index < MAX_REPORT_BLOCK) && (tmp >= 24) 
			&& (rb != original_rb) ); 
		rs->send_rb_list = rb;
	} // if (rb)
	
	/* The ength of this RTCP packet in 32-bit words minus one,
	 * inclding the header and any padding. */
	len = 6 + (index * sizeof(rtcp_rr_t) / 4);

	rtcppkt.common->version = 2;
	rtcppkt.common->p	= 0;
	rtcppkt.common->pt	= RTCP_SR;
	rtcppkt.common->count	= index;
	rtcppkt.common->length	= len;
	
#if RTP_DEBUG	
	printf("MSG	: build_sr - [%u] build sr, senders = %d, rtcppkt.common->count = %d\n", rtcppkt.r.sr.rtcp_sr->ssrc, rs->rtcp_rule.senders, rtcppkt.common->count);//DEBUG
#endif
	
	if (LOG_SEND_RTCP) {
		FILE 		*fp;
		char	*LOC_IP, open[40];

		LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
		sprintf(open, "%s", LOC_IP);
		strcat(open, "_rtcp_send.log");

		fp = fopen(open, "a+");
	
		if (fp != NULL) {
			fprintf(fp, "\n");
			fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
			fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
			fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
			fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
			fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);
			fprintf(fp, "-------------------------------------------------------\n\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
	
	*remainder = tmp;		// == (ptr - buf);	// == tmp 
	(*buf) = ((*buf) + 4 * (len + 1));	// == return p ?? check it!
	*rtcplen += 4 * (len + 1);
	
#if RTP_DEBUG	
	printf("MSG	: build_sr - SR size = %d\n", (4 * (len + 1)));//DEBUG
#endif
	
	if (tmp < 24) {
		return BUFFER_NOT_ENOUGH_LENGTH;
	}
	else if (index >= MAX_REPORT_BLOCK){
		return TOO_MANY_REPORT_BLOCK;
	}
	else {
		return err;
	}

} /* build_sr */


/*
 * build_rr - build " receiver report " rtcp packet
 */
rtperror 
build_rr(session_id sid, char **buf, int *rtcplen, send_rb_list_t *original_rb, int *remainder, struct timeval *cur){
	
	rtperror	err = 0;
	session		*rs;
	rtcp_t		rtcppkt;
	char		*ptr = (*buf);
	member_t 	*member;
	int 		index = 0, len, tmp;
	send_rb_list_t  *rb;
	
#if RTP_DEBUG	
	printf("MSG	: build RR~\n");
#endif
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}

	tmp = *remainder;
	
	if (tmp < (4 + 4)){
		return BUFFER_NOT_ENOUGH;
	}
		
	rtcppkt.common = (rtcp_common_t *) ptr;
	ptr += 4;	/* rtcp_common_t size == 4 */	
	tmp -= 4;		
	
	rtcppkt.r.rr.ssrc = (u_int32 *) ptr;
	ptr += 4;	/* ssrc size == u_int32 == 24 */	
	tmp -= 4;
		
	*(rtcppkt.r.rr.ssrc) = rs->rtp_sendpkt.rtphdr.ssrc;

	if (LOG_SEND_RTCP) {
		FILE 	*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_send.log");

                fp = fopen(open, "a+"); /* includes info. of all participants in this host. */
		if (fp != NULL) {
			fprintf(fp, "RR:\n");
			fprintf(fp, "rtcppkt.r.rr.ssrc = %lu\n", *(rtcppkt.r.rr.ssrc));
			fprintf(fp, "-------------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}

	rb = rs->send_rb_list;
        if (rb) {
                do {
                        err = get_member_by_ssrc(rs, rb->ssrc, &member);
                        if (err) {
                                fprintf(stderr, "build_rr - %s, rb->ssrc = %lu\n", RTPStrError(err), rb->ssrc);//DEBUG
                                return err;
                        }

                        if (member->ssrc != rs->rtp_sendpkt.rtphdr.ssrc) {

                                rtcppkt.r.rr.rr[index] = (rtcp_rr_t *) ptr;
                                ptr += 24; /* rtcp_rr_t size == 24 */

                                err = build_report_block(sid, rtcppkt.r.rr.rr[index], member, cur);
                                if (err > 0) {
                                        return err;
                                }

                                index++;
                                tmp -= 24;
                        }

                        rb = rb->next;
                }
                while ( (index < MAX_REPORT_BLOCK) && (tmp >= 24)
                        && (rb != original_rb) );
                rs->send_rb_list = rb;
        } // if (rb)
	
	/* The length of this RTCP packet in 32-bit words minus one,
	 * including the header and any padding. */
	len = 1 + (index * sizeof(rtcp_rr_t) / 4);
	
	rtcppkt.common->version = 2;
	rtcppkt.common->p	= 0;
	rtcppkt.common->pt	= RTCP_RR;
	rtcppkt.common->count	= index;
	rtcppkt.common->length	= len;
	
#if RTP_DEBUG	
	printf("MSG	: build_rr - rtcppkt.common->count = %d\n", rtcppkt.common->count);//DEBUG
#endif
	
	if (LOG_SEND_RTCP) {
		FILE 		*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_send.log");

                fp = fopen(open, "a+");
		if (fp != NULL) {
			fprintf(fp, "\n");
			fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
			fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
			fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
			fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
			fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);
			fprintf(fp, "-------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
		
	*remainder = tmp;		// (ptr - buf);
	(*buf) = ((*buf) + 4 * (len + 1));	// == return p ?? check it!
	*rtcplen += 4 * (len + 1);
	
#if RTP_DEBUG	
	printf("MSG	: build_rr - RR size = %d\n", (4 * (len + 1)));//DEBUG
#endif
	
        if (tmp < 24) {
                return BUFFER_NOT_ENOUGH_LENGTH;
        }
        else if (index >= MAX_REPORT_BLOCK){
                return TOO_MANY_REPORT_BLOCK;
        }
        else {
                return err;
        }

} /* build_rr */


/* build_bye */
rtperror 
build_bye(session_id sid, char **buf, int *rtcplen, int *remainder){
	
	rtperror	err = 0;
	session		*rs;
	char		*ptr = (*buf);
	int		len, tmp, pad, num = 1;	// num == 1 for SSRC
	rtcp_t		rtcppkt;
	csrc_t		*csrc;
	
#if RTP_DEBUG	
	printf("MSG	: build BYE~\n");
#endif
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	rtcppkt.common = (rtcp_common_t *) ptr;
	ptr += 4;	/* rtcp_common_t size == 4 */
	
	rtcppkt.r.bye.src = (u_int32 *) ptr;
	ptr += 4;	/* ssrc size == u_int32 == 24 */
		
	memcpy(rtcppkt.r.bye.src, &(rs->rtp_sendpkt.rtphdr.ssrc), sizeof(u_int32));

	if (LOG_SEND_RTCP) {
		FILE 	*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_send.log");

                fp = fopen(open, "a+");
		if (fp != NULL) {
			fprintf(fp, "BYE:\n");
			fprintf(fp, "rtcppkt.r.bye.src = %lu\n", *(rtcppkt.r.bye.src));
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}		

	/* CSRCs if exist */
	csrc = rs->csrclist;

	while (csrc) {

		memcpy(ptr, &(csrc->ssrc), sizeof(u_int32));
		ptr += 4;

		if (LOG_SEND_RTCP) {
			FILE 	*fp;
	                char    *LOC_IP, open[40];

	                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
	                sprintf(open, "%s", LOC_IP);
	                strcat(open, "_rtcp_send.log");

	                fp = fopen(open, "a+");
			if (fp != NULL) {
			fprintf(fp, "rtcppkt.r.bye.csrc = %lu\n", *(rtcppkt.r.bye.src));
			fflush(fp);
			}
			else {
				fprintf(stderr, "can't open file - %s\n", open);
			}
			fclose(fp);
			free(LOC_IP);
		}
		csrc = csrc->next;
		num++;
	}

	if (rs->bye_reason) {		
		tmp = strlen(rs->bye_reason);
		memcpy(ptr, rs->bye_reason, tmp);
		ptr += tmp;
	}

	tmp = tmp % 4;	// ptr % 4; ??
	if (tmp) {
		// padding to 32-bit 
		memset(ptr, '0', tmp);
		ptr += tmp;
	}

	// padding to 32-bit 
	len = ptr - (*buf);
	pad = 4 - (len & 0x3);
	while (pad--) *ptr++ = RTCP_SDES_END;	
	
	len = ( ptr - (*buf) ) / 4 - 1;
	
	rtcppkt.common->version = 2;
	rtcppkt.common->p	= 0;
	rtcppkt.common->pt	= RTCP_BYE;
	rtcppkt.common->count	= num;
	rtcppkt.common->length	= len;

	if (LOG_SEND_RTCP) {
		FILE 		*fp;
                char    *LOC_IP, open[40];

                LOC_IP = strdup(inet_ntoa(rs->recvlist->addr));
                sprintf(open, "%s", LOC_IP);
                strcat(open, "_rtcp_send.log");

                fp = fopen(open, "a+");
		if (fp != NULL) {
			fprintf(fp, "\n");
			fprintf(fp, "rtcppkt.common->version = %u\n", rtcppkt.common->version);
			fprintf(fp, "rtcppkt.common->p = %u\n", rtcppkt.common->p);
			fprintf(fp, "rtcppkt.common->pt = %u\n", rtcppkt.common->pt);
			fprintf(fp, "rtcppkt.common->count = %u\n", rtcppkt.common->count);
			fprintf(fp, "rtcppkt.common->length = %u\n", rtcppkt.common->length);
			fprintf(fp, "-------------------------------------------------------\n");
			fflush(fp);
		}
		else {
			fprintf(stderr, "can't open file - %s\n", open);
		}
		fclose(fp);
		free(LOC_IP);
	}
		
	*remainder -= (ptr - (*buf));
	(*buf) = ((*buf) + 4 * (len + 1));	// == return p ?? check it!
	*rtcplen += 4 * (len + 1);
	
#if RTP_DEBUG	
	printf("MSG	: build_bye - BYE size = %d\n", (4 * (len + 1)));//DEBUG
#endif
		
	return err;
		
} /* build_bye */


/*
 * rtp_hdr_validity_check - RTP Data Header Validity Checks
 *
 * specified in A.1 of RFC3550.
 */
rtperror 
rtp_hdr_validity_check(session_id sid, rtp_hdr_t *rtphdr){
	
	rtperror	err = 0;
	session		*rs;
	
        /* get the session from session_list[sid] to " s" */
	err = get_session(sid, &rs);
	if (err > 0) {
		return err;
	}
	
	/* RTP version field must equal 2. */
	if (rtphdr->version != RTP_VERSION) {
#if RTP_DEBUG	
		printf("MSG	: rtphdr->version = %u\n", rtphdr->version);
#endif
		return RTP_INVALID_VERSION;
	}

	/* The payload type must be known, and in particular it must not be equal to SR or RR. */
	if ((rtphdr->pt) >= 72 && (rtphdr->pt) <= 76) { // SR:200, RR:201.
#if RTP_DEBUG	
		printf("MSG	: rtppkt->rtphdr.pt = %u\n", rtphdr->pt);
#endif
		return RTP_INVALID_PT;
	}
	
	/* If the P bit is set, then the last octet of the packet must contain a valid octet count, in
	 * particular, less than the total packet length minus the header size. */
	if (rtphdr->p) {
		// omission
	}
	
	/* The X bit must be zero if the profile does not specify that the header extension mechanism
	 * may be used. Otherwise, the extension length field must be less than the total packet size
	 * minus the fixed header length and padding. */
	if (rtphdr->x) {
		// omission
	}

	/* The length of the packet must be consistent with CC and payload type (if payloads have a
	 * known length). */

#if RTP_DEBUG	
	printf("MSG	: rtp_hdr_validity_check - RTP header [%u] is OK\n", rtphdr->ssrc);//DEBUG
#endif	
	return err;
	
} /* rtp_hdr_validity_check */


/*
 * rtcp_hdr_validity_check - RTP Data Header Validity Checks
 *
rtperror
rtcp_hdr_validity_check(char *buf, int l, int *is_bye){
	
	rtperror	err;
	session		*rs;
	
	u_int32		len = (u_int32) (l / 4);// len	:length of compound RTCP packet in words 
	rtcp_t		*r = (rtcp_t *)buf;	// RTCP header 
	rtcp_t		*end;			// end of compound RTCP packet 
	rtcp_common_t	*rtcp_common = (rtcp_common_t *)buf;
	
	printf("MSG	: rtcp_hdr_validity_check - step 1\n");
	
	printf("MSG	: rtcp_hdr_validity_check - the rtcp first 16-bit : %0x, v = %u, p = %u, count = %u, pt = %u, length = %u\n", *(u_int16 *)r, 
		rtcp_common->version, rtcp_common->p, rtcp_common->count, rtcp_common->pt, rtcp_common->length); 
	
	if ( (*(u_int16 *)r & RTCP_VALID_MASK) != RTCP_VALID_VALUE) {
		// something wrong with packet format
		return RTCP_INVALID;
	}
	
	printf("MSG	: rtcp_hdr_validity_check - step 2\n");
	
	end = (rtcp_t *)((u_int32 *)r + len);
	
	do {
		r = (rtcp_t *)((u_int32 *)r + r->common->length + 1);
		if (r->common->pt == RTCP_BYE) {
			*is_bye = TRUE;
		}	
		
	} while (r < end && r->common->version == RTP_VERSION);

	printf("MSG	: rtcp_hdr_validity_check - step 3\n");
	
	if (r != end) {
		// something wrong with packet format 
		return RTCP_INVALID;
	}
	
	printf("MSG	: rtcp_hdr_validity_check - RTCP header is OK\n");
		
	return 0;
		
}  rtcp_hdr_validity_check */


/*
 * rtcp_hdr_validity_check - RTP Data Header Validity Checks
 */
rtperror
rtcp_hdr_validity_check(char *buf, int l, int *is_bye){
	
	rtperror	err = 0;
	
	u_int32		len = (u_int32) (l / 4);/* len	:length of compound RTCP packet in words */
	rtcp_t		*r = (rtcp_t *)buf;	/* RTCP header */
	rtcp_t		*end;			/* end of compound RTCP packet */
	rtcp_common_t	*rtcp_common = (rtcp_common_t *)buf;
	
#if RTP_DEBUG	
	printf("MSG	: rtcp_hdr_validity_check - the rtcp first 16-bit : %0x, v = %u, p = %u, count = %u, pt = %u, length = %u\n", *(u_int16 *)r, 
		rtcp_common->version, rtcp_common->p, rtcp_common->count, rtcp_common->pt, rtcp_common->length); 
#endif
	
	if ( (rtcp_common->version == RTP_VERSION) && ( rtcp_common->pt == RTCP_SR || rtcp_common->pt == RTCP_RR) && (rtcp_common->p == FALSE)) {
	}
	else {	// something wrong with packet format
#if RTP_DEBUG	
		printf("MSG	: rtcp_hdr_validity_check - version = %u, pt = %u, p = %u\n", rtcp_common->version, rtcp_common->pt, rtcp_common->p);
#endif
		return RTCP_INVALID;
	}

	end = (rtcp_t *)((u_int32 *)r + len);

#if RTP_DEBUG	
	printf("MSG	: rtcp_hdr_validity_check - len = %d\n", len);
	printf("MSG	: rtcp_hdr_validity_check - r = %p, end = %p\n", r, end);
#endif

	do {
		r = (rtcp_t *)((u_int32 *)r + rtcp_common->length + 1);
		
#if RTP_DEBUG	
		printf("MSG	: rtcp_hdr_validity_check - r = %p, end = %p, rtcp_common->length = %d\n", r, end, rtcp_common->length);
#endif
	
		rtcp_common = (rtcp_common_t *)r;
		if (rtcp_common->pt == RTCP_BYE) {
			*is_bye = TRUE;
		}	
		
	} while (r < end && rtcp_common->version == RTP_VERSION);

	if (r != end) {
		/* something wrong with packet format */
#if RTP_DEBUG	
		printf("MSG	: rtcp_hdr_validity_check - rtcp end error, r = %p, end = %p\n", r, end);
#endif
		return RTCP_INVALID;
	}
	
#if RTP_DEBUG	
	printf("MSG	: rtcp_hdr_validity_check - RTCP header is OK\n");
#endif
		
	return err;
		
} /* rtcp_hdr_validity_check */


/* 
 * get_sdes_len - get the length of sdes rtcp packet.
 */
int 
get_sdes_len(session *rs){
	
	int 			len = 0, i;
	send_sdes_list_t	*sdes;
	member_t		*member;

	sdes = rs->send_sdes_list;

	while (sdes) {
		if (get_member_by_ssrc(rs, sdes->ssrc, &member) < 0) {
			return -1;
		}
		for (i = 1; i < 9; i++) {
			if ( member->sdes.intervals[i] && !((rs->number_of_rtcp_send) % member->sdes.intervals[i]) ) {
				len += member->sdes.length[i];
			}
		}
		sdes = sdes->next;
	}

	return len;
	
} /* get_sdes_len */


/* 
 * get_bye_len - get the length of BYE packet.
 */
int 
get_bye_len(session *rs){
	
	int	len = 0, tmp = 0;
	csrc_t	*csrc;

	csrc = rs->csrclist;
	while (csrc) {
		tmp++;
		csrc = csrc->next;
	}

	len = 4 + 4 + tmp * 4;	/* bye rtcp header(4) + a SSRC(4) + CSRCs(tmp * 4) */
	
	if (rs->bye_reason) {
		tmp = strlen(rs->bye_reason);
		len += tmp;
	}
	
	tmp = tmp % 4;
	len += tmp;	/* padding */
	
	return len;
	
} /* get_bye_len */


/* check_on_expire - check if something is on_expire */
rtperror 
check_on_expire(){
	
	rtperror	err = 0;
	struct timeval	now;
	rtp_eq		*tmp;
	
	if (!rtpeq) {
#if RTP_DEBUG	
		printf("MSG	: check_on_expire - no new event~ \n");
#endif
		return 0;
	}

	gettimeofday(&now, NULL);
	
	tmp = rtpeq;

	while (time_expire(&(tmp->timeout), &now)) {

#if RTP_DEBUG	
		printf("MSG	: check_on_expire - event time expire!!, do it now~\n");//DEBUG_ONLY
#endif
		
		/* tmp is on_expire */
		err = rtcp_on_expire(&now);
		if (err > 0) {
			return err;
		}
		else if (err < 0) {
			fprintf(stderr, "rtcp_on_expire - %s\n", RTPStrError(err));
		}

		if (rtpeq) {	/* check next event if it exists. */
			tmp = rtpeq;
		}
		else { 
			break;
		}	
	}
	
	return err;
	
} /* check_on_expire */
