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

#ifndef __NCTUNS_relay_h__
#define __NCTUNS_relay_h__

#include <stdio.h>
#include <object.h>
#include <gprs/GtpGmm/mydefine.h>
#include <event.h>
#include <gprs/include/mytype.h>
#include <gprs/GtpGmm/HLR/HLR.h>

typedef struct RelayPort {
	u_int8_t	portNum;
	MBinder		*port;
	RelayPort	*nextport;
} RelayPort;

struct VLR{
	ulong		imsi;
	ulong		ptmsi;
	ulong		ptmsi_sig;
	ulong		tmsi;
	ulong		RAI;
	ushort		CI;
	u_int8_t	bssidNum;
	VLR	*next;
};

class GprsSw : public NslObject {

 private: 
	
	u_int8_t		sgsnport;		
	RelayPort		*PortList;
	u_int32_t		num_port;
	u_long			RAI;
	char			*tmp_RAI;
	//every SGSN's VLR table
	VLR		*VLR_table_h;

 public:
 	GprsSw(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~GprsSw();   

	int 			init(); 
	int			recv(ePacket_ *pkt);
	int 			command(int argc, const char *argv[]); 
	//int			changeVLR(control_msg *msg);
	int			update_VLR(ms_storage *msg);
	int			cancel_old_VLR_rd(ulong ip,control_msg *msg);
        int                     get(ePacket_ *pkt, MBinder *frm);
        int                     send(ePacket_ *pkt, MBinder *frm);
        int			Location_Update_req(route_msg *msg);
        int			Delete_old_VLR_rd(ulong imsi);
        ulong			getSGSNIP();
        int			initial_maptable();
        

}; 
 

#endif /* __NCTUNS_relay_h__ */
