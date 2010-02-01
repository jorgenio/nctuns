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


#include "rtp_error.h" 


/********************************************************************/


char *RTPStrError(rtperror err){

	switch(err){

		case REPEAT_SEND_ADDR_LIST:
			return "WARNING	: The address is repeat in sendlist of this session.\n";

		case RTP_CANT_ALLOC_MEM:		
			return "ERROR	: Memory allocation for RTP failed.";
	
		case ADD_SENDLIST_FAIL:
			return "ERROR	: Fail to add the new address into the sendlist.";

		case ADD_RECVLIST_FAIL:
			return "ERROR	: Fail to add the new address into the recvlist.";
	
		case NO_SEND_SDES_LIST:
			return "WARNING	: No SDES is added into RTCP packet.\n";
	
		case RTP_UNKNOWN_SESSION:          
			return "ERROR	: Session may destroyed or never created, operation failed.";

		case RTP_BAD_ADDR:                 
			return "WARNING	: Invalid RTP address specified, operation failed.\n";

		case RTP_BAD_PORT:                 
			return "WARNING	: Invalid RTP port specified, operation failed.\n";							

		case RTCP_BAD_ADDR:                 
			return "WARNING	: Invalid RTCP address specified, operation failed.\n";

		case RTCP_BAD_PORT:                 
			return "WARNING	: Invalid RTCP port specified, operation failed.\n";		

		case RTP_BAD_PORT_ZERO:
			return "ERROR	: Port number zero not allowed.";

		case RTP_CANT_GET_SOCKET:		
			return "ERROR	: Couldn't obtain RTP socket.";
			
		case RTP_CANT_BIND_SOCKET:
			return "ERROR	: RTP can't bind RTP(or RTCP) socket.";
		
		case RTP_CANT_SET_SOCKOPT:		
			return "ERROR	: Unable to set RTP socket option";
		
		case RTP_CANT_GET_SOCKOPT:
			return "ERROR	: Couldn't get RTP source address";	
		
		case RTCP_CANT_GET_SOCKET:
			return "ERROR	: Couldn't obtain RTCP socket";
			
		case RTCP_CANT_BIND_SOCKET:
			return "ERROR	: RTCP can't bind RTCP socket";						
 		
 		case RTCP_CANT_SET_SOCKOPT:
 			return "ERROR	: Unable to set RTCP socket option";
 		
 		case RTCP_CANT_GET_SOCKOPT:	
 			return "ERROR	: Couldn't get RTCP source address";
 		
 		case RTP_ALREADY_FIXED:
 			return "WARNING	: It's not allowed to change variable of recvlist during the session is connecting.\n";
 		
 		case NO_RTP_RECV_ADDR_LIST:	
 			return "WARNING	: The recvlist of session is NULL.\n";
 		
 		case RTP_CONNECT_ALREADY_OPENED:
 			return "WARNING	: Session connection is already opened.\n";
 		
 		case RTP_CONNECT_ALREADY_CLOSED:
 			return "WARNING	: Session connection is already closed.\n";
 				
 		case NO_RTP_SEND_ADDR_LIST:
 			return "WARNING	: The sendlist of session is NULL\n";
 		
 		case NO_MATCH_ADDR:
 			return "WARNING	: We can't match up the address.\n";	
 		
 		case RTP_CANT_CLOSE_SESSION:
 			return "ERROR	: The RTP session can't be closed.";

 		case RTCP_CANT_CLOSE_SESSION:
 			return "ERROR	: The RTCP session can't be closed.";
 			 					
 		case RTP_CONNECT_COLSED:
 			return "ERROR	: the RTP session is not connecting";

 		case RTCP_CONNECT_COLSED:
 			return "ERROR	: the RTCP session is not connecting";
 			 				 					
 		case RTP_SEND_ERR:
 			return "ERROR	: RTP send error.";

 		case RTCP_SEND_ERR:
 			return "ERROR	: RTCP send error.";
 			 				 				
 		case CSRC_COUNT_EXCEED:
 			return "ERROR	: CSRC count shoule be range in 0~15."; 
 						
 		case RTP_PKT_OUT_OF_RANGE:
 			return "ERROR	: The size of RTP packet in buffer is out of range.";
 			
 		case RTP_SKT_NOMATCH:
 			return "WARNING	: socket is not the same as rtp(rtcp) socket.\n";	
 		
 		case RTP_SOCKET_READ_FAILURE:
 			return "ERROR	: Could not read from rtp(rtcp) socket.";
 		
 		case BUFFER_NOT_ENOUGH:
 			return "ERROR	: buffer is not big enough";
 		
 		case RTP_UNKNOWN_PROTOCOL:
 			return "ERROR	: Unknown type of underlying protocl.";
 		
 		case SESSION_OUT_OF_RANGE:
 			return "ERROR	: There are too many rtp sessions in a single process";
 		
 		case RTP_PORT_EXHAUST:
 			return "ERROR	: There is no free ports we can used.";
 						
 		case RTP_NO_SENDLIST:
 			return "WARNING	: There is no address we can sendto.\n";
 		
 		case NO_RTCP_WARNING:
 			return "WARNING	:  RTCP switch off.\n";
 		
		case RTP_UNKNOWN_TYPE:
			return "WARNING	: unknown type of action of RTPevent in RTPqueue\n";
		
		case RTP_CANT_REUSE_ADDR:
			return "ERROR	: setsockopt error, we can't reuse the address.";
		
		case RTP_CANT_REUSE_PORT:
			return "ERROR	: setsockopt error, we can't reuse the port.";
		
		case RTP_NO_ENOUGH_BW:
			return "WARNING	: total bandwidth is not enough for all active sessions.\n";
 	
		case RTP_ALREADY_OPEN:
			return "WARNING	: the rtp of this session is already opened.\n";

		case RTP_ALREADY_CLOSE:
			return "WARNING	: the rtp of this session is already closed.\n";
		
		case RTCP_ALREADY_OPEN:
			return "WARNING	: the rtcp of this session is already opened.\n";

		case RTCP_ALREADY_CLOSE:
			return "WARNING	: the rtcp of this session is already closed.\n";			

		case RTP_CANT_MATCH_MEMBER:
			return "WARNING	: We can't match up the specified member.\n";
		
		case MEMBER_ALREADY_EXIST:
			return "WARNING	: The specified member we operating already exists.\n";
		
		case RTP_INVALID_VERSION:
			return "WARNING	: the version field in the rtp header is invalid.(!=2), we ignore it.\n";
		
		case RTP_INVALID_PT:
			return "WARNING	: the payload_type field in the rtp header is invalid.(==SR || RR), we ignore it.\n";
		
		//case RTP_NO_RECEIVER:
		//	return "there is no receiver initialization to receive rtp or rtcp packet";
		
		case ADD_MEMBER_FAIL:
			return "ERROR	: The operation of member-adding failed.";
		
		case CLOSE_RTCP_WARNING:
			return "WARNING	: There is no RTCP functionality in this rtp session.\n";
		
		case UNKNOWN_EVENT_TYPE:
			return "WARNING	: The type of event is unknown.\n";
	
		case NUM_OF_SENDER_ERR:
			return "ERROR	: Return error because senders count is not correct.";

		case UNKNOWN_SDES_TYPE:
			return "WARNING	: The type of SDES is invalid, operation failed.\n";

		case NULL_WARNING:
			return "WARNING	: Someting is null, operation failed\n";
		
		case RTP_SOCKET_CLOSE_FAIL:
			return "ERROR	: Socket close error.";	

		case RTP_SOCKET_CONNECT_FAIL:
			return "ERROR	: Socket connect error.";
				
		case RTP_NO_CONNECTION:
			return "WARNING	: The connection of session is closed at present, operation failed.\n";		
		
		case INVALID_SKT:
			return "ERROR	: The socket value that we get is invalid.";
		
		case NO_EXIST_MEMBER_TABLE:
			return "ERROR	: member_table is not allocated in this session at present.";
				
		case NO_EVENT_IN_QUEUE:
			return "WARNING	: These is no event in the rtp schedule queue.\n";
		
		case OVER_MAX_SDES:
			return "WARNING	: the length is longer than sdes item max size(255), operation failed.\n";
			
		case RTCP_INVALID:
			return "WARNING	: something wrong with rtcp packet format.\n";
		
		case OTHER_SOURCE_CONFLICT:
			return "WARNING	: some source(ssrc is not myself) is conflict.\n";
		
		case LOCAL_SOURCE_OLD_CONFLICT:
			return "WARNING	: A collision or loop of the participant's own packets.\n";
		
		case LOCAL_SOURCE_NEW_CONFLICT:
			return "WARNING	: New collision, change SSRC identifier,\n";
		
		case SOURCE_STILL_INVALID:
			return "WARNING	: The source is still invalid at present.\n";
		
		case BUFFER_NOT_ENOUGH_LENGTH:
			return "WARNING	: The length of buffer is not long enough to include all report blocks.\n";
		
		case TOO_MANY_REPORT_BLOCK:
			return "WARNING	: The number of rtport block is over than 31.\n";
		
		case CSRC_ERROR:
			return "ERROR	: Something wrong with csrc count.";

		case WE_ARE_SENDING_BYE:
			return "WARNING	: We omit the packet when we are going to send BYE packet.\n";
			
		case SDES_COUNT_ERROR:
			return "WARNING	: SDES_COUNT_ERROR.\n";
		
		case NO_MEMBER_DELETED:
			return "WARNING	: NO_MEMBER_DELETED.\n";
		
		case NO_LOCAL_MEMBER:
			return "ERROR	: We can't get the local member.";
				
		default:
			return "Unknown RTP Error";
	}
} /* RTPStrErr */			

