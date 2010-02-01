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


#ifndef _RTP_MEMBER_INNER_H
#define _RTP_MEMBER_INNER_H


/********************************************************************/


#include "rtp_api.h"
#include "rtp_error.h"


/********************************************************************/


void init_seq(source_t *s, rtp_hdr_t hdr);

int update_seq(source_t *s, rtp_hdr_t hdr);

int find_in_conflict_list(session *rs, struct sockaddr *from);

void update_conflict_info(session *rs, struct sockaddr *from, struct timeval *now);

void log_collision(session *rs, struct sockaddr *from, struct timeval *now);

void new_collision(session *rs, struct sockaddr *from, struct timeval *now);

void update_member_info(session *rs, member_t *member, int is_rtp, struct timeval *now);

double get_recent_recv_loosrate(session_id sid, u_int32 *ssrc);

rtperror update_rr(session_id sid, member_t *rr_member, rtcp_rr_t *rtcp_rr, double ntp_cur, member_t *rtcp_member);

/********************************************************************/


#endif	/* _RTP_MEMBER_INNER_H */
