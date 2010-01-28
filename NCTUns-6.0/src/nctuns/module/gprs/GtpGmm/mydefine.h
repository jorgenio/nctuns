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

#include <gprs/include/mytype.h>
#ifndef MY_Define
#define MY_Define

//this structure is used during transmitting control messages

struct create_PDP{
	char    control[30];
	ushort  nsapi;
	uchar   qos;
	ulong   imsi;
	ulong   tlli;
	ulong   TEID;
};
struct deactivate_type{
	char    control[30];
	uchar	type;//0:attach 1:detach 3:re-attach
	ushort	nsapi;
	uchar	qos;
	ulong   imsi;
	ulong	tlli;
};
struct activate_type{
	char	control[30];
	uchar	type;//0:attach 1:MS detach 2:NetWork detach
	ushort	nsapi;
	uchar	qos;
	ulong	imsi;
	ulong	tlli;
	ulong	msisdn;
	ulong	ms_ip;//ms's ip
};
struct ip_structure{
	char	control[30];
	uchar	msisdn;
	ulong 	imsi;
	ulong	ptmsi;
	ulong	ip;
	uchar	tid;
	ushort  nsapi;
	uchar   qos;
	ulong   tlli;
	char	cause[30];
	ulong	ms_ip;
	                
};

struct GTP_header{
	uchar	Version;
	uchar	PT;//Protocol Type
	uchar	E;//Extension Header flag
	uchar	S;//Sequence Number flag
	uchar	PN;//N-PDU Number flag
	uchar	msg_type;//message type
	ushort	len;//lengh
	ulong	TEID;//Tunnel Endpoint Identifier
	ushort	Seq_no;//Sequence Number
	uchar	N_PDU_no;//N-PDU Number flag
};
struct route_msg{
	char    control[30];
	uchar	type;//0:attach 1:detach 3:re-attach
	ushort	nsapi;
	uchar	qos;
	ulong   imsi;
	ulong	tlli;
	ulong	ptmsi;
	ulong 	ptmsi_sig;//ptmsi_signature
	ulong	RAI;//routing area identity
	ushort	CI;//cell identity	
	ulong	TEID;
	//ushort  port_id;
};
struct return_msg{
	char	control[30];
	
};
struct control_msg{
	char    control[30];
	uchar	type;//0:attach 1:detach 3:re-attach
	ushort	nsapi;
	uchar	qos;
	ulong   imsi;
	ulong	tlli;
	ulong   ptmsi;
	ulong   ptmsi_sig;//ptmsi_signature
	ulong	msisdn;
	ulong	ms_ip;
	ulong   RAI;//routing area identity
	ushort  CI;//cel
	//uchar	classmark;
	ulong	TEID;
	uchar	TID;
	u_int64_t delay_time;
	
};
/*Define the MS storage memory*/
struct Paging_resp{
	char	control[30];
	ulong imsi;
	ulong	ptmsi;
	//ulong	ptmsi_sig;
	ulong	tlli;
};
struct Paging_ps{
	char  control[30];
	uchar type;//x06 for paging ps
	ulong imsi;
	ulong RAI;
	ushort qos;
	ulong ptmsi;
	ulong tlli;
};
struct under_route_msg{
	char	control[30];
	ulong	RAI;//routing area identity
	ushort	CI;//cell identity	
};
struct intra_route_msg{
	char	control[30];
	ulong	imsi;
	ulong	tlli;
	ulong	ptmsi;
	ulong 	ptmsi_sig;//ptmsi_signature
	ushort	CI;//cell identity	
};
struct in_standby{
	char    control[30];
	ulong   imsi;
};
struct route_accept{
	char    control[30];
	ulong	imsi;
	ulong	tlli;
	ulong   ptmsi;
	ulong   ptmsi_sig;//ptmsi_signature
};
struct attach_accept{
	char    control[30];
	ulong	imsi;
	ulong   ptmsi;
	ulong   ptmsi_sig;//ptmsi_signature
	ulong	RAI;
	ulong	tlli;
};
struct detach_accept{
	char    control[30];
	ulong	imsi;
	ulong   ptmsi;
	ulong   ptmsi_sig;//ptmsi_signature
	ulong	tlli;
};
struct NK_detach_type{
	char    control[30];
	uchar	type;//0:attach 1:detach 3:re-attach
	ulong	imsi;
	ulong   ptmsi;
	ulong   ptmsi_sig;//ptmsi_signature
	ulong	tlli;
};
struct NK_type{
	char control[30];
	ulong TEID;
};
struct detach_type{
	char    control[30];
	uchar	type;//0:attach 1:detach 3:re-attach
	ushort	nsapi;
	uchar	qos;
	ulong   imsi;
	ulong   ptmsi;
	ulong   ptmsi_sig;//ptmsi_signature
	ulong 	tlli;
};
struct ms_storage{
	char	control[30];
	uchar	type;//0:attach 1:MS detach 2:NetWork detach 3:identify request
	ushort	nsapi;
	uchar	qos;
	ulong	imsi;
	ulong	tlli;
	ulong	ptmsi;
	ulong 	ptmsi_sig;//ptmsi_signature
	ulong	msisdn;
	ulong	ms_ip;//ms's ip
	ulong	RAI;//routing area identity
	ushort	CI;//cell identity	
	//uchar	classmark;
	//uchar	update_type;//0:normal RA update 1:periodic RA update
};
/*Define the sndcp acked header */
struct sndcp_ack{
	uchar	X;
	uchar	F;
	uchar 	T;
	uchar	M;
	uchar	NSAPI;
	uchar	DCOMP;
	uchar	PCOMP;
};

/*Define the sndcp unacked header */
struct sndcp_unack{
	uchar	X;
	uchar	F;
	uchar	T;
	uchar	M;
	uchar	NSAPI;
	uchar    DCOMP;
	uchar    PCOMP;
	uchar	seg_no; //segmentation number
	uchar	N_PDU;
	uchar	E_N_PDU; //extended N_PDU number
	        	
};
struct link_status{
	ushort	nsapi;
	uchar	qos;
	uchar	linkstatus; // 0:not linked 1:linked
	struct link_status *next;
};

/*Define the LLC message from LLC layer*/
struct LLC_msg{
	char		control[30];//control signal
	uchar		status; //identify some error condition occur in LLC layer 0:error occurs 1:no error
	ulong		TLLI;//LLC put TLLI in the field,if TLLI put 0 in it representing failure
	ushort		nsapi;
	uchar		qos;
	
};

struct upper_msg{
	char		control[30];
};
struct doingAction{
	int	s_time; /*user-specified time*/
	char	action[30];
	doingAction *next;
};
#endif
