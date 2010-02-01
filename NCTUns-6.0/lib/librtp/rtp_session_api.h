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


#ifndef _RTP_SESSIONAPI_H
#define _RTP_SESSIONAPI_H


/********************************************************************/


#include "rtp_session_inner.h"


/********************************************************************/


rtperror rtp_create(session_id *sid);

rtperror rtp_delete(session_id sid);

// call by rtp_send

rtperror build_rtp_pkt(session_id sid, int8 marker, int16 pt, char *payload, int *rtplen, int plen, char *sendbuf);

rtperror rtcp_start(session_id sid);

rtperror rtcp_stop(session_id sid, char *reason);

rtperror build_rtcp_pkt(session_id sid, char *buf, int *rtcplen, event_type type);

rtperror rtp_set_session_bw(session_id sid, double session_bw);

rtperror rtp_set_sdes_item(session_id sid, int sdes_type, char *data, int interval);

char *get_sdes_item(member_t *s, int type);

rtperror receive_rtp_pkt(session_id sid, char *buf, int len, struct sockaddr *from, socklen_t *formlen);

rtperror receive_rtcp_pkt(session_id sid, char *buf, int len, struct sockaddr *from, socklen_t *formlen);

event_type type_of_event(session_id sid);

rtperror update_by_send_rtp(session_id, int32 addts, int plen);

rtperror rtp_set_timerate(session_id sid, int timerate);

rtperror get_rtcp_timeout(session_id sid, struct timeval *timeout);

rtperror rtp_check_on_expire();


/********************************************************************/


#endif /* _RTP_SESSIONAPI_H */

