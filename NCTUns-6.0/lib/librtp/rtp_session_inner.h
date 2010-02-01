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


#ifndef _RTP_SESSION_INNER_H
#define _RTP_SESSION_INNER_H


/********************************************************************/


#include "rtp_api.h"
#include "rtp_error.h"


/********************************************************************/


rtperror rtcp_interval(session_id sid, struct timeval *interval);

rtperror rtcp_schedule(session_id sid, event_type type, struct timeval *t);

rtperror rtcp_on_expire(struct timeval *current);

rtperror reverse_reconsideration(session_id sid, event_type type, struct timeval *current);

rtperror build_sdes(session_id sid, char **buf, int *rtcplen);

rtperror build_sr(session_id sid, char **buf, int *rtcplen, send_rb_list_t *original_rb, int *remainder, struct timeval *cur);

rtperror build_rr(session_id sid, char **buf, int *rtcplen, send_rb_list_t *original_rb, int *remainder, struct timeval *cur);

rtperror build_report_block(session_id sid, rtcp_rr_t *block, member_t *member, struct timeval *cur);

rtperror build_bye(session_id sid, char **buf, int *rtcplen, int *remainder);

rtperror rtp_hdr_validity_check(session_id sid, rtp_hdr_t *rtphdr);

rtperror rtcp_hdr_validity_check(char *buf, int l, int *is_bye);

int get_sdes_len(session *rs);

int get_bye_len(session *rs);

rtperror check_on_expire();

/********************************************************************/


#endif /* _RTP_SESSION_INNER_H */
