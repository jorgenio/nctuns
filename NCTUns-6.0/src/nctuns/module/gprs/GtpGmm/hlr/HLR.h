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

#ifndef __NCTUNS_HLR_h__
#define __NCTUNS_HLR_h__

#include <object.h>
#ifndef LINUX
#include <nlist.h>
#endif

#include <gprs/include/mytype.h>

struct  ip_array{
	ulong ip;
	ulong  tlli;
	ushort nsapi;
	uchar   tid;
	uchar   qos;
	ulong   imsi;
	ulong   msisdn;
	ulong	ptmsi;
	ushort	ca;
	ulong	ra;	                                
	ip_array *next;
};

struct HLR_record{
	ulong   tlli;
	ushort  nsapi;
	ulong	ms_ip;
	uchar   tid;
        ulong   ggsnip;
        ulong	sgsnip;
        uchar   qos;
        ulong   imsi;
        ulong	ptmsi;
        ulong   msisdn;
        ushort	ca;//cell area
        ulong	ra;//routing area
        HLR_record *next;
};
class HLR {
	private:
		HLR_record	*HLR_rd;
		//HLR_record	*head_r;	
        public:
        	HLR();
        	~HLR();
        	int		see_if_exist(ulong tlli);
        	int		add_record(HLR_record *h_record);
        	int		delete_record(HLR_record *h_record);
        	ulong		modify_record(HLR_record *h_record);        
		ip_array 	*search_record(ulong *ip_imsi);
		int		exist_record(ulong *imsi);                                	
		ulong		search_imsi(ulong *ptmsi);
		ulong		getSGSNIP(ulong RAI);
		
};

#endif
