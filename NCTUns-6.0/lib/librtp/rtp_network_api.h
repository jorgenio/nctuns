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


#ifndef _RTP_NETWORKAPI_H
#define _RTP_NETWORKAPI_H


/********************************************************************/


#include "rtp_network_inner.h"


/********************************************************************/


rtperror rtp_add_dest_list(session_id sid, char *addr, u_int16 port, u_int8 ttl, protocol_t rtp_p, protocol_t rtcp_p);

rtperror rtp_del_dest_list(session_id sid, char *addr, u_int16 port, u_int8 ttl);
	
rtperror rtp_get_dest_list(session_id sid, address_info_t **sendaddr);

rtperror rtp_add_sour_list(session_id sid, char *addr, u_int16 port, protocol_t rtp_p, protocol_t rtcp_p);

rtperror rtp_del_sour_list(session_id sid);

rtperror rtp_get_sour_list(session_id sid, char *addr, u_int16 *port, protocol_t *rtp_p, protocol_t *rtcp_p);

rtperror rtp_open_connection(session_id sid);

rtperror rtp_close_connection(session_id sid, char *reason);

rtperror rtp_get_sour_rtpsocket(session_id sid, socktype *skt);

rtperror rtp_get_sour_rtcpsocket(session_id sid, socktype *skt);

rtperror rtp_send(session_id sid, int8 marker, int32 addts, int16 pt, int8 *payload, int plen);

rtperror rtcp_send(session_id sid, event_type type);

rtperror on_receive(session_id sid, socktype skt, char *buf, int *blen);


/********************************************************************/


#endif /* _RTP_NETWORKAPI_H */

