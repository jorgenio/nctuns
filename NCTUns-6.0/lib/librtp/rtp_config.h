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
 * The file contains some default parameter used by librtp.
 */

/********************************************************************/


#ifndef _RTP_CINFIG_H
#define _RTP_CINFIG_H


/********************************************************************/


#define RTP_DEBUG 0


/******************************
 * Below is relative to session 
 ******************************/


/* Maximum number of rtp sessions that each process can control */
#define RTP_MAX_NUM_SESSION 4


/* Maximum port number of all session */
#define PORT_UPPER 65536


/* Minimum port number of all session which suggested by RFC 3551 */
#define PORT_LOWER 5000


/* Default RTP port is 5004 which suggested by RFC 3551 */
#define RTP_DEFAULT_PORT 5004


/* 
 * Default bandwidth for each RTP session of the RTP process.
 * In octets per second.
 * Often, the session bandwidth is the sum of the nominal of 
 * the senders expected to be concurrently active.
 * 
 * BECAREFUL!! all participants must use the same value for the session 
 * bandwidth so that the same RTCP interval will be calculated.
 * 
 * Session bandwidth parameter is the product of the individual
 * sender's bandwidth times the number of participants, and the 
 * RTCP bandwidth is 5% of that.
 */
#define RTP_DEFAULT_SESSION_BW 2048


/* Log the sender's rtp header info if LOG_SEND_RTP is TRUE */
#define LOG_SEND_RTP 1


/* Log the receiver's rtp header info if LOG_RECV_RTP is TRUE */
#define LOG_RECV_RTP 1


/* Log the sender's rtcp header info if LOG_SEND_RTCP is TRUE */
#define LOG_SEND_RTCP 1


/* Log the receiver's rtcp header info if LOG_RECV_RTCP is TRUE */
#define LOG_RECV_RTCP 1


/**************************
 * Below is relative to rtp
 **************************/
 

/* Maximum numbers of rtp payload types  */
#define RTP_MAX_PAYLOAD_TYPES 128

 
/* Maximum size of RTP packet, default we define 10mb per packet. */
#define RTP_MAX_PKT_SIZE 40960


/* Maximum numbers of CSRC counts */
#define RTP_MAX_CC 15


/* RTP version default is 2 */
#define RTP_VERSION 2


/* We take the packet which's status may still is invalid if it is true. */
#define PROMISCUOUS_MODE 0


/* Maximun payload length in each rtp packet */
#define MAX_PAYLOAD_LENGTH (RTP_MAX_PKT_SIZE-(3+RTP_MAX_CC)*4)


/***************************
 * Below is relative to rtcp
 ***************************/
 

/* 
 * Maximum size of RTCP packet.
 * Because the default MTU is 1500, we simply reduce the bytes of 
 * IP+UDP header (8+20) in UDP, and IP+TCP header (20+20) in TCP.
 * so we simply choose 1460 for the max size of rtcp packet.
 */
#define RTCP_MAX_PKT_SIZE 1460

//#define RTCP_TCP_MAX_PKT_SIZE 1460


/* 
 * Turning off RTCP reception reports is NOT RECOMMENDED because
 * they are need reception quality feedback and congestion control.
 */
#define RTCP_CLOSE 0


/* 
 * It's RECOMMENDED that the fraction of the session bandwidth 
 * added for RTCP be fixed in 5%. by RFC 3550
 */
#define RTCP_BW_FRACTION 0.05



/* 
 * Fraction of the RTCP bandwidth to be shared among active senders.
 * (This fraction was chosen so that in a typical session with one
 *  or two active senders, the computed report time would be roughly 
 *  equal to the minimum report time so that we don't unnecessarily 
 *  slow down receiver reports.) 
 * The receiver fraction must be 1 - the sender fraction.
 * 
 * It's also RECOMMENDED that 1/4 of the RTCP bandwidth be dedicated 
 * to participants that are sending data.
 */
#define RTCP_SENDER_BW_FRACTION 0.25

#define RTCP_RCVR_BW_FRACTION (1 - RTCP_SENDER_BW_FRACTION)


/* 
 * The RECOMMENDED value for a fixed minimum interval is 5 seconds. 
 *
 * Minimum average time between RTCP packets from this site (in seconds).
 * This time prevents the reports from `clumping' when sessions are small 
 * and the law of large numbers isn't helping to smooth out the traffic. 
 * It also keeps the report interval from becoming ridiculously small 
 * during transient outages like a network partition.
 */
#define RTCP_MIN_TIME 5


/* 
 * To compensate for "timer reconsideration" converging to a value 
 * below the intended average.
 * 
 * The resulting value of T is divided by e - 3/2 = 1.21828 to compensate 
 * for the fact that the timer reconsideration algorithm converges to a 
 * value of the RTCP bandwidth below the intended average. by RFC 3550.
 */
#define COMPENSATION 1.21828


/*
 * librtp perform "timer reconsideration" if it is true.
 */
#define TIMER_RECONDSIDERATION 1


/* 
 * A participant MAY mark another size inactive, or delete it if not 
 * yet valid, if no RTP or RTCP packet has been received for a small 
 * value of RTCP report intervals. 5 is RECOMMENDED. by RFC 3550.
 */
#define TIMEOUT_MULTIPLIER 5


/* Maximum text length for SDES */
#define RTP_MAX_SDES 255 


/* 
 * A source is declared valid only after MIN_SEQUENTIAL packets have 
 * been received in sequence. 
 */
#define MIN_SEQUENTIAL 2


/* After a source is considered valid, the sequence number is considered 
 * valid if it is no more than MAX_DROPOUT ahead of s->max_seq nor more 
 * than MAX_MISORDER behind.
 *
 * Typical values for the parameters are shown, based on a maximum 
 * misordering time of 2 seconds at 50 packets/second and a maximum 
 * dropout of 1 minute.The dropout parameter MAX_DROPOUT should be a 
 * small fraction of the 16-bit sequence number space to give a reasonable 
 * probability that new sequence numbers after a restart will not fall in 
 * the acceptable range for sequence numbers from before the restart.
 */
#define MAX_DROPOUT 3000

#define MAX_MISORDER 100


/* Maximun number of report blocks in SR or RR packet */
#define MAX_REPORT_BLOCK 31


/* By init_seq */
#define RTP_SEQ_MOD (1<<16)


/*****************************
 * Below is relative to member
 *****************************/

/* Hash size of member_table */
#define HASH_SIZE 10


/********************************************************************/


#endif /* _RTP_CINFIG_H */

