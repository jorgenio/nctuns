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

#ifndef	__NCTUNS_sp_ctl_h__
#define	__NCTUNS_sp_ctl_h__

#include <list>
#include <map>
#include <stdint.h>
#include <timer.h>
#include <event.h>
#include <nctuns_api.h>
#include <regcom.h>
#include <ip.h>
#include <tcp.h>
#include <udp.h>

#include <satellite/dvb_rcs/common/descriptor/linkage_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/network_name_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/ip_mac_platform_name_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/ip_mac_platform_provider_name_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/target_ip_slash_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/ip_mac_stream_location_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/data_broadcast_id_descriptor.h>
#include <satellite/dvb_rcs/common/descriptor/stream_identifier_descriptor.h>

#include <satellite/dvb_rcs/common/si_config.h>
#include <satellite/dvb_rcs/common/table/table.h>
#include <satellite/dvb_rcs/common/table/pat_table.h>
#include <satellite/dvb_rcs/common/table/pmt_table.h>
#include <satellite/dvb_rcs/common/table/nit_table.h>
#include <satellite/dvb_rcs/common/table/int_table.h>
#include <satellite/dvb_rcs/common/table/pat_table_q.h>
#include <satellite/dvb_rcs/common/table/pmt_table_q.h>
#include <satellite/dvb_rcs/common/table/nit_table_q.h>
#include <satellite/dvb_rcs/common/table/int_table_q.h>
#include <satellite/dvb_rcs/common/si_config.h>
#include <satellite/dvb_rcs/common/ncc_config.h>
#include <satellite/dvb_rcs/rcst/rcst_ctl.h>

#define	SEND_TCP_LOG
#define	RECV_TCP_LOG

#define MAX_RCST_NUMBER		65536
#define MAX_HOST_NUMBER		1000
#define NOTHING			0

using std::map;

class Ncc_ctl;
/*
 * record all rcsts' information, sp controller will handle 
 * some actions dependenting on this RCST_info.
 */
class Rcst_infor {
 
friend class Sp_ctl;

 private:
	uint32_t	_node_id;
	u_long		_Rcst_IPaddr;
	uint16_t	_PID;
	u_long		_Hosts_IPaddr[MAX_HOST_NUMBER];
	int		host_num;
};

class Sp_ctl : public NslObject {

 private:
	timerObj	_PatTimer;
	timerObj	_PmtTimer;
	timerObj	_NitTimer;
	timerObj	_IntTimer;

	double		_PATGenInterval;
	double		_PMTGenInterval;
	double		_NITGenInterval;
	double		_INTGenInterval;

	Pat_circleq	*_PATs;	
	Pmt_circleq	*_PMTs;
	Nit_circleq	*_NITs;
	Int_circleq	*_INTs;
	int		_Pat_number;
	int		_Pmt_number;
	int		_Nit_number;
	int		_Int_number;
	int		_Rcst_info_num;
	map <uint16_t, Rcst_infor> _RCST_info;

	dvb_node_type	_node_type;

	/*
	 * get ncc_ctl module pointer
	 */
	Ncc_ctl       		*_ncc_module;
	Ncc_config		*_ncc_config;

	/* 
	 * for store xxx.dvbrcs.nodeid file
	 */
	list<dvbrcs_node_id>		_node_id_cfg;

	/*
	 * private function
	 */
	int		_Parse_rcst_info();
	int16_t		get_rcst_pid(u_long dst_ipaddr);

	int		GenPAT(Event_ *pEvn);
	int		GenPMT(Event_ *pEvn);
	int		GenNIT(Event_ *pEvn);
	int		GenINT(Event_ *pEvn);
	int		_parse_nodeid_cfg(char *path, list<dvbrcs_node_id> &node_id);

 public:
	Sp_ctl();
	Sp_ctl(u_int32_t type , u_int32_t id , struct plist* pl , const char *name);
	~Sp_ctl();

	int		init();
	int		send(ePacket_ *Epkt);
	int		recv(ePacket_ *Epkt);
	int		set_rcst_info();
	int		set_PAT();
	int		set_PMT();
	int		set_INT();
	int		set_NIT();
};


#endif /* __NCTUNS_sp_ctl_h__ */
