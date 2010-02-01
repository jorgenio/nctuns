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


#ifndef _RTP_MEMBERAPI_H
#define _RTP_MEMBERAPI_H


/********************************************************************/


#include "rtp_member_inner.h"


/********************************************************************/


rtperror init_member(session_id sid);

int ssrc_hash(u_int32 ssrc);

rtperror add_new_member(session_id sid, u_int32 ssrc, struct sockaddr *fromaddr, memberstatus status, int IsRTP, int contributor);

rtperror get_member_by_ssrc(session *rs, u_int32 ssrc, member_t **member);

rtperror delete_member_by_ssrc(session_id sid, u_int32 ssrc);

rtperror sr_update_member_info(session_id sid, rtcp_t rtcppkt, struct sockaddr *from, socklen_t *formlen, double ntp_cur, double cur);

rtperror rr_update_member_info(session_id sid, rtcp_t rtcppkt, struct sockaddr *from, socklen_t *formlen, double ntp_cur);

rtperror sdes_update_member_info(session_id sid, char *ptr);

rtperror bye_update_member_info(session_id sid, char *ptr);

rtperror rtp_collision_resolution_loop_detection(session_id sid, rtp_pkt_t rtppkt, struct sockaddr *from);

rtperror rtp_timeout(session_id sid, struct timeval *now);

rtperror find_next_sender(session_id sid, int *hash, member_t **member, member_t **sender);

rtperror rtcp_collision_resolution_loop_detection(session_id sid, rtcp_t rtcppkt, struct sockaddr *from, u_int32 ssrc);

rtperror add_send_sdes_list(session_id sid, u_int32 ssrc);

rtperror del_send_sdes_list(session_id sid, u_int32 ssrc);

rtperror add_send_rb_list(session_id sid, u_int32 ssrc);

rtperror del_send_rb_list(session_id sid, u_int32 ssrc);


/********************************************************************/


#endif /* _RTP_MEMBERAPI_H */

