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
 * rtp error(warning) message control.
 */

/********************************************************************/


#ifndef _RTP_ERROR_H
#define _RTP_ERROR_H


/********************************************************************/


/* rtp error(or warning) that librtp returns to the user */
typedef int rtperror; 


/********************************************************************/


/*
 * define rtperror both error and warning types
 * error > 0 , warning < 0 
 */

#define REPEAT_SEND_ADDR_LIST          	 -1
#define NO_RTP_SEND_ADDR_LIST          	 -2
#define NO_MATCH_ADDR                  	 -3
#define NO_RTP_RECV_ADDR_LIST          	 -4
#define RTP_CONNECT_ALREADY_OPENED     	 -5
#define RTP_CONNECT_ALREADY_CLOSED     	 -6
#define RTP_NO_SENDLIST                	 -7
#define NO_RTCP_WARNING                	 -8
#define RTP_UNKNOWN_TYPE              	 -9
#define RTP_NO_ENOUGH_BW	      	-10
#define RTP_ALREADY_OPEN	      	-11
#define RTP_ALREADY_CLOSE             	-12
#define RTCP_ALREADY_OPEN             	-13
#define RTCP_ALREADY_CLOSE            	-14
#define RTP_CANT_MATCH_MEMBER	      	-15
#define CLOSE_RTCP_WARNING	        -16
#define NULL_WARNING			-17
#define RTP_BAD_ADDR                   	-18
#define RTP_BAD_PORT                   	-19
#define RTCP_BAD_ADDR                  	-20
#define RTCP_BAD_PORT                  	-21
#define RTP_NO_CONNECTION		-22
#define NO_EVENT_IN_QUEUE		-23
#define UNKNOWN_SDES_TYPE		-24
#define OVER_MAX_SDES			-25
#define NO_SEND_SDES_LIST		-26
#define RTP_SKT_NOMATCH			-27
#define RTP_INVALID_VERSION		-28
#define RTP_INVALID_PT			-29
#define RTCP_INVALID			-30
#define OTHER_SOURCE_CONFLICT		-31
#define LOCAL_SOURCE_OLD_CONFLICT	-32
#define LOCAL_SOURCE_NEW_CONFLICT	-33
#define SOURCE_STILL_INVALID		-34
#define BUFFER_NOT_ENOUGH_LENGTH	-35
#define TOO_MANY_REPORT_BLOCK		-36
#define WE_ARE_SENDING_BYE		-37
#define SDES_COUNT_ERROR		-38
#define NO_MEMBER_DELETED		-39
#define MEMBER_ALREADY_EXIST		-40
#define RTP_ALREADY_FIXED              	-41

#define RTP_CANT_ALLOC_MEM               1
#define ADD_SENDLIST_FAIL		 2
#define RTP_UNKNOWN_SESSION            	 3
#define RTP_SOCKET_CLOSE_FAIL            4
#define RTP_SOCKET_CONNECT_FAIL		 5
#define INVALID_SKT			 6
#define NO_EXIST_MEMBER_TABLE		 7
#define RTP_BAD_PORT_ZERO              	 8
#define RTP_CANT_GET_SOCKET            	 9
#define RTP_CANT_BIND_SOCKET           	10
#define RTP_CANT_SET_SOCKOPT           	11
#define RTP_CANT_GET_SOCKOPT           	12
#define RTCP_CANT_GET_SOCKET           	13
#define RTCP_CANT_BIND_SOCKET          	14
#define RTCP_CANT_SET_SOCKOPT          	15
#define RTCP_CANT_GET_SOCKOPT          	16
//#define NO_EVENT_IN_QUEUE		17
#define RTP_CANT_CLOSE_SESSION         	18
#define RTCP_CANT_CLOSE_SESSION        	19
#define RTP_CONNECT_COLSED             	20
#define RTCP_CONNECT_COLSED            	21
#define RTP_SEND_ERR                   	22
#define CSRC_COUNT_EXCEED              	23
#define RTP_PKT_OUT_OF_RANGE           	24
#define ADD_MEMBER_FAIL                	25
#define RTP_SOCKET_READ_FAILURE        	26
#define BUFFER_NOT_ENOUGH              	27
#define RTP_UNKNOWN_PROTOCOL           	28
#define SESSION_OUT_OF_RANGE           	29
#define RTP_PORT_EXHAUST               	30
#define RTP_CANT_REUSE_ADDR	       	31
#define RTP_CANT_REUSE_PORT	       	32
#define ADD_RECVLIST_FAIL	       	33
#define CSRC_ERROR			34
#define NO_LOCAL_MEMBER			35
#define UNKNOWN_EVENT_TYPE              36
#define RTCP_SEND_ERR			37
#define NUM_OF_SENDER_ERR               38



/********************************************************************/


/* returns the reason why error(warning) occurs  */
char *RTPStrError(rtperror err);


/********************************************************************/


#endif /* _RTP_ERROR_H */

