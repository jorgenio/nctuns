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

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <strings.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/descriptor/common.h>
#include <satellite/dvb_rcs/common/rcst_info.h>
#include <satellite/dvb_rcs/ncc/ncc_ctl.h>
#include "sp_ctl.h"
#include <packet.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

extern RegTable                 RegTable_;

MODULE_GENERATOR(Sp_ctl)
/*
 * constructor
 */
Sp_ctl::Sp_ctl(u_int32_t type , u_int32_t id , struct plist* pl , const char* name) 
:NslObject(type , id , pl , name) {
	
	_PATs = new Pat_circleq();
	_PMTs = new Pmt_circleq();
	_NITs = new Nit_circleq();
	_INTs = new Int_circleq();

	_Pmt_number = 0;
	_Pat_number = 0;
	_Nit_number = 0;
	_Int_number = 0;
	_Rcst_info_num = 0;
	/*
	 * bind generation interval of tables 
	 */
	vBind("PATGenInterval" , &_PATGenInterval);	
	vBind("PMTGenInterval" , &_PMTGenInterval);	
	vBind("NITGenInterval" , &_NITGenInterval);	
	vBind("INTGenInterval" , &_INTGenInterval);	

	_node_type = NODE_ID_SP;
	REG_VAR("NODE_TYPE", &_node_type);

}

int
Sp_ctl::_parse_nodeid_cfg(char *filename, list<dvbrcs_node_id> &global_system_node_id)
{
	global_system_node_id.clear();
	FILE    *nodeid_cfg;
	if (!(nodeid_cfg = fopen(filename, "r"))) {
		printf("[RCST_CTL] Warning: Cannot open file %s", filename);
		assert(0);
	}
	else {

		char    line[200];
		char    buf[200];
		while (fgets(line, 200, nodeid_cfg)) {
			dvbrcs_node_id	one_dvbrcs_node_id;
			if (sscanf(line, " DVB-RCS:%s", buf)) {
				char*   tok;
				tok = strtok(buf, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.sat_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.feeder_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.gw_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.sp_nid)));
				tok = strtok(NULL, "_");
				assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.ncc_nid)));
					
				while ((tok = strtok(NULL, "_"))) {
					struct rcst_node_id	one_rcst_node_id;
					assert(sscanf(tok, "%u", &one_rcst_node_id.rcst_nid));
					one_dvbrcs_node_id.rcst_nid_list.push_back(one_rcst_node_id);
				}
			}
			else if (sscanf(line, " #%s", buf))
				continue;

			global_system_node_id.push_back(one_dvbrcs_node_id);
		}
		fclose(nodeid_cfg);
		return 1;
	}
}


/*
 * destructor
 */
Sp_ctl::~Sp_ctl() {

	delete _PATs;
	delete _PMTs;
	delete _NITs;
	delete _INTs;
}

/*
 * init function
 */
int
Sp_ctl::init() {

	char filename[100];

	Nit						*nit_;
	Pat 						*pat_;
	Int						*int_;
	Pmt 						*pmt_for_int , *pmt_for_mpe;

	/*
	 * get ncc_ctl module pointer
	 */
	/* get ncc_node_id */
	Rcst_info_list		*rcst_info_list;

	sprintf(filename, "%s.dvbrcs.nodeid", GetScriptName());

	if (_parse_nodeid_cfg(filename, _node_id_cfg))
	{
		uint32_t	ncc_node_id;
		char		ncc_ctl_module_name[100];

		for (list<dvbrcs_node_id>::iterator it = _node_id_cfg.begin();
		     it != _node_id_cfg.end(); it++)
		{
			if (it->sp_nid == get_nid())
			{
				ncc_node_id = it->ncc_nid;
			}
		}

		sprintf(ncc_ctl_module_name, "Node%u_NCC_CTL_LINK_1", ncc_node_id);

		assert(_ncc_module = (Ncc_ctl*) RegTable_.lookup_Instance(ncc_node_id, ncc_ctl_module_name));

		//_ncc_module = InstanceLookup(ncc_node_id, "NCC_CTL");
		assert(_ncc_config = &(_ncc_module->ncc_config));

		rcst_info_list = &(_ncc_module->rcst_info_list);

		REG_VAR("NCC_CONFIG", _ncc_config);
	}
	else
		assert(0);


	/*
	 * parse rcst.config
	 */
	_Parse_rcst_info();

	/*
	 * creat PAT, NIT, INT, PMTs and specify default information to them
	 * then, we have to add the tables into their own table circleqs.
	 */

	/*
	 * NIT initialization
	 * creat network name descriptor and linkage descriptor, then specify some information. 
	 * Finally, add the descriptors to NIT table.
	 */
	nit_ = new Nit(DVBS2_NETWORK_ID , INIT_VER_NUM , CUR_INDICATOR);
	Nit_transport_stream_info nit_added_ts_info;
	nit_added_ts_info.set_transport_stream_id(TS_ID);
	nit_added_ts_info.set_original_network_id(DVBS2_NETWORK_ID);
	nit_->add_transport_info(&nit_added_ts_info);

	Linkage_descriptor link_des(TS_ID , DVBS2_NETWORK_ID , INT_PRO_NUM , 0x0B , IP_MAC_PLAT_NUM);
	link_des.set_platform_id(0 , IP_MAC_PLAT_ID);
	link_des.creat_platform_name_loop(0 , 1);
	link_des.set_platform_name_variable(0 , 0 , "369" , "NCTUNS_PLAT_NAME");
	nit_->add_network_loop_des((Descriptor*)&link_des);

	_NITs->add_nit(nit_);
	delete nit_;
	_Nit_number++;

	/*
	 * INT initialization
	 * we have to add target ip slash descriptors and ip mac stream location descriptors according to rcst_info.
	 * Finally, add the descriptors to INT table.
	 */

	int_ = new Int(INIT_VER_NUM , CUR_INDICATOR , IP_MAC_PLAT_ID , FIRST_ACTION);

	/* rcst_info */
	for (int i = 0 ; i < _Rcst_info_num ; i++) {

		u_long 		ipaddr;
		uint8_t 	com_tag;

		IPv4_format ipv4;
		ipaddr = _RCST_info[i]._Rcst_IPaddr;
		ipv4.IPv4_addr = ipaddr;
		/* no subnet specification */
		ipv4.IPv4_slash_mask = 32;

		/* com_tag = PID - 0x0100 */
		com_tag = (_RCST_info[i]._PID - 0x0100);
	
		Target_ip_slash_descriptor ip_des(1 , &ipv4);

		Ip_mac_stream_location_descriptor str_loc_des(DVBS2_NETWORK_ID , DVBS2_NETWORK_ID , 
							      TS_ID , TRF_PRO_NUM , com_tag);

		int_->add_target_and_operational_descriptor(&ip_des , &str_loc_des);
	}	
	_INTs->add_int(int_);
	delete int_;
	_Int_number++;

	/*
	 * PMT circleq initialization
	 * add the first PMT table which specify information of INT service 
	 * and add the second PMT table which specify information of TCP/IP traffic
	 */

	/* for int service */
	pmt_for_int = new Pmt(INT_PRO_NUM , INIT_VER_NUM , CUR_INDICATOR);
	Pmt_frame_info added_pmt_info;
	added_pmt_info.set_stream_type(PRIVATE_SECTION);
	added_pmt_info.set_elementary_pid(INT_PID);
	pmt_for_int->add_frame_info(added_pmt_info);

	Data_broadcast_id_descriptor data_bro_id_des(0x0B , IP_MAC_PLAT_NUM);
	data_bro_id_des.set_Platform_id_info(0 , IP_MAC_PLAT_ID , 0x01 , 0 , 0);
	pmt_for_int->add_es_info_descriptor(INT_PID , (Descriptor*)&data_bro_id_des);

	/* for mpe service */
	pmt_for_mpe = new Pmt(TRF_PRO_NUM , INIT_VER_NUM , CUR_INDICATOR);
	pmt_for_mpe->set_pcr_pid(0);
	
	
	for (int i = 0 ; i < _Rcst_info_num ; i++) {

		uint16_t rcst_pid;
	
		rcst_pid = _RCST_info[i]._PID;

		Pmt_frame_info info_cpy;
		info_cpy.set_stream_type(MPE_STREAM);
		info_cpy.set_elementary_pid(rcst_pid);
		pmt_for_mpe->add_frame_info(info_cpy);

		/* component tag = PID - 0x0100 */
		Stream_identifier_descriptor str_id_cpy(rcst_pid - 0x0100);
		pmt_for_mpe->add_es_info_descriptor(rcst_pid , (Descriptor*)&str_id_cpy);
	}	
	
	_PMTs->add_pmt(pmt_for_int);
	_Pmt_number++;
	delete pmt_for_int;
	_PMTs->add_pmt(pmt_for_mpe);
	_Pmt_number++;
	delete pmt_for_mpe;

	/*
	 * PAT initialization
	 * add NIT and INT information to pat_
	 */
	pat_ = new Pat(TS_ID , INIT_VER_NUM , CUR_INDICATOR);
	pat_->add_pat_ass_info(NIT_PRO_NUM , NIT_PID);
	pat_->add_pat_ass_info(INT_PRO_NUM , INT_PMT_PID);
	pat_->add_pat_ass_info(TRF_PRO_NUM , TRF_PMT_PID);
	_PATs->add_pat(pat_);

	_Pat_number++;
	delete pat_;

	

	uint64_t	interval_in_tick = _ncc_config->superframe_duration;
	_PmtTimer.setCallOutObj(this , (int (NslObject::*)(Event_ *))&Sp_ctl::GenPMT);
	_PmtTimer.start(0 , interval_in_tick);
	_PatTimer.setCallOutObj(this , (int (NslObject::*)(Event_ *))&Sp_ctl::GenPAT);
	_PatTimer.start(0 , interval_in_tick);
	_IntTimer.setCallOutObj(this , (int (NslObject::*)(Event_ *))&Sp_ctl::GenINT);
	_IntTimer.start(0 , interval_in_tick);
	_NitTimer.setCallOutObj(this , (int (NslObject::*)(Event_ *))&Sp_ctl::GenNIT);
	_NitTimer.start(0 , interval_in_tick);
	

	return (NslObject::init());
}

/*
 * send function
 */
int
Sp_ctl::send(ePacket_ *Epkt) {

	Packet		*p;
	struct ip*	iph;
	u_long		dst_ipaddr;
	int		rcst_pid;
	Dvb_pkt		*dvb_pkt;

	p = (Packet*)Epkt->DataInfo_;
	assert(p);

	/*
	 * get destination ip address of the pkt, and find which pid it has in RCST_info.
	 */
	iph = (struct ip*)p->pkt_sget();
	dst_ipaddr = iph->ip_dst;
	

	struct in_addr sin;
	sin.s_addr = dst_ipaddr;
	if ((rcst_pid = get_rcst_pid(dst_ipaddr)) < 0) {
		printf("\e[1;33m====>%s\e[m\n", inet_ntoa(sin));
		printf("==> warning: This pkt have no pid!!!\n");
		freePacket(Epkt);
		return (1);
	}
	else
	{
		/*
		 * convert original pkt to dvb pkt and set it's pid
		 * finally, free original pkt.
		 */
		dvb_pkt = new Dvb_pkt();
		assert(dvb_pkt);
		dvb_pkt->convert_from_nctuns_pkt(p);
		dvb_pkt->pkt_settype(PKT_RAWDATA);
		dvb_pkt->pkt_setflag(FLOW_SEND);
		delete p;
		(dvb_pkt->pkt_getfwdinfo())->pid = rcst_pid;
		Epkt->DataInfo_ = dvb_pkt;
		
		return (NslObject::send(Epkt));
	}
}

/*
 * recv function
 */
int
Sp_ctl::recv(ePacket_ *Epkt) {

	Dvb_pkt		*dvb_pkt;
	Packet		*pkt;
	int		pkt_type;


	dvb_pkt = (Dvb_pkt*)Epkt->DataInfo_;
	pkt_type = dvb_pkt->pkt_gettype();

	switch (pkt_type) {
		/*
		 * it is tcp/ip data, we have to judge that the next hop of pkt is outside (to internet)
		 * ,or inside (sent to other hosts which are beyond Rcsts.
		 */
		case (PKT_RAWDATA): {

			pkt = dvb_pkt->convert_to_nctuns_pkt();
			delete dvb_pkt;
			Epkt->DataInfo_ = pkt;

			return (NslObject::recv(Epkt));

		}
		/*
		 * it is control table, then drop it.
		 */
		case (PKT_RCSMAC):
		case (PKT_RCSSYNC):
		{
			dvb_freePacket(Epkt);
			break;
		}

		default:
			assert(0);
			dvb_freePacket(Epkt);
	}
	return (1);
}

/*
 * _Parse_rcst_info function
 * Usage : parse rcst information (such as mac address, ip address, group_id, logon_id, PID of each rcst and 
 * IP addresses of all hosts beyond every rcst) from specific file
 */
int
Sp_ctl::_Parse_rcst_info() { 

	FILE	 	*fp;
	char 		line[200], buf[200];
	char 		host_ipaddr[200];
	int		host_num_;
	u_long		rcst_ip;
	u_long		host_ip;
	uint32_t	pid, node_id;
	char		filename[200];
	
	/*
	 * find the rcst.config in config file dir
	 */
	snprintf(filename, 200, "%s.dvbrcs.sp.%d", GetScriptName(), get_nid());
 
	if (!(fp = fopen(filename, "r")))
	{
		fprintf(stderr, "%s: Open config file(%s) fails.\n", get_name(), filename);
		exit(EXIT_FAILURE);
	}

	/*
	 * parse rcst.config file and set Rcst_info
	 */
	while (fgets(line , sizeof(line) , fp) != NULL) 
	{
		if (sscanf(line, " #%s", buf))
		{
			// This is a comment line. Skip it.
			continue;
		}
		else if (sscanf(line, " RCST: %s", buf))
		{
			char*	tok;


			tok = strtok(buf, "_");
			assert(sscanf(tok, "%u", &node_id));

			tok = strtok(NULL, "_");
			string_to_ipv4addr(tok , &rcst_ip);

			tok = strtok(buf, "_");
			assert(sscanf(tok, "%x", &pid));

			_RCST_info[_Rcst_info_num]._node_id = node_id;
			_RCST_info[_Rcst_info_num]._Rcst_IPaddr = rcst_ip;
			_RCST_info[_Rcst_info_num]._PID = pid;
			_RCST_info[_Rcst_info_num].host_num = 0;
			_Rcst_info_num++;
		}
		else if (sscanf(line, " HOST: %s", host_ipaddr))
		{
			host_num_ = _RCST_info[_Rcst_info_num-1].host_num;
			string_to_ipv4addr(host_ipaddr , &host_ip);
			_RCST_info[_Rcst_info_num-1]._Hosts_IPaddr[host_num_] = host_ip;
			_RCST_info[_Rcst_info_num-1].host_num++;
		}
		else
		{
			printf("SP configuration file format error!!\n");
			assert(0);
		}
	}
	fclose(fp);

	return -1;
}

/*
 * GenPAT function
 * Usage : send PATs periodically
 */
int
Sp_ctl::GenPAT(Event_ *pEvn) { 

	Dvb_pkt	*pPkt;
	Pat *cpy;
	

	if (_Pat_number == 0)
		return -1;

	/*
	 * send all PATs in _PATs circleq
	 * but only PAT table to be sent presently, because we assuem only one transport stream in our system.
	 */
	delete (pEvn);
	for (int i = 0 ; i < _Pat_number ; i++) {

		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		cpy = _PATs->get_sequencing_pat(i);


		pPkt = new Dvb_pkt();
		assert(pPkt);
		pPkt->pkt_attach((void*)cpy , UNKNOWN_LEN);
		pPkt->pkt_settype(PKT_TABLE);
		pPkt->pkt_setflag(FLOW_SEND);
		(pPkt->pkt_getfwdinfo())->pid = PAT_PID;
		pEvn = createEvent();
		assert(pEvn);
		pEvn->DataInfo_ = pPkt;

		NslObject::send(pEvn);
	}
	return 1;
}

/*
 * GenPMT function
 * Usage : send PMTs periodically
 */
int
Sp_ctl::GenPMT(Event_ *pEvn) { 

	Dvb_pkt		*pPkt;
	Pmt 		*cpy;
	uint16_t 	pro_num;
	uint16_t 	pmt_pid;
	Pat		*pat;


	if (_Pmt_number == 0)
		return -1;

	/*
	 * get PAT table which transport stream id is TS_ID
	 */
	pat = _PATs->get_pat(INIT_VER_NUM , TS_ID);
	/*
	 * send all PMTs of _PMTs circleq
	 */
	delete (pEvn);
	for (int i = 0 ; i < _Pmt_number ; i++) {

		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		cpy = _PMTs->get_sequencing_pmt(i);

		pro_num = cpy->get_program_number();
		pmt_pid = pat->get_PMT_pid(pro_num);
	
		pPkt = new Dvb_pkt();
		assert(pPkt);
		pPkt->pkt_attach((void*)cpy , UNKNOWN_LEN);
		pPkt->pkt_settype(PKT_TABLE);
		pPkt->pkt_setflag(FLOW_SEND);
		(pPkt->pkt_getfwdinfo())->pid = pmt_pid;
		pEvn = createEvent();
		assert(pEvn);
		pEvn->DataInfo_ = pPkt;

		/*
	 	 * last lookup of PAT, therefore we have to free it.
		 */
		if (i == _Pmt_number - 1)
			delete pat;

		NslObject::send(pEvn);
	}
	return 1;
}

/*
 * GenNIT function
 * Usage : send NITs periodically
 */
int
Sp_ctl::GenNIT(Event_ *pEvn) { 

	Dvb_pkt	*pPkt;
	Nit *cpy;

	if (_Nit_number == 0)
		return -1;

	/*
	 * send all NITs in _NITs circleq
	 * but only NIT table to be sent presently, because we assume only one network in our system.
	 */
	delete (pEvn);
	for (int i = 0 ; i < _Nit_number ; i++) {

		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		cpy = _NITs->get_sequencing_nit(i);

		pPkt = new Dvb_pkt();
		assert(pPkt);
		pPkt->pkt_attach((void*)cpy , UNKNOWN_LEN);
		pPkt->pkt_settype(PKT_TABLE);
		pPkt->pkt_setflag(FLOW_SEND);
		(pPkt->pkt_getfwdinfo())->pid = NIT_PID;
		pEvn = createEvent();
		assert(pEvn);
		pEvn->DataInfo_ = pPkt;

		NslObject::send(pEvn);
	}
	return 1;
}

/*
 * GenINT function
 * Usage : send INTs periodically
 */
int
Sp_ctl::GenINT(Event_ *pEvn) { 

	Dvb_pkt	*pPkt;
	Int *cpy;
	
	if (_Int_number == 0)
		return -1;

	/*
	 * send all INTs in _INTs circleq
	 * but only INT table to be sent presently, because we assume only one IP_MAC platform in our system.
	 */
	delete (pEvn);
	for (int i = 0 ; i < _Int_number ; i++) {

		/*
	 	 * new novel packet struct of dvb and attatch table to data payload of this packet,
		 * then set PID for mpeg2_ts module
	 	 */
		cpy = _INTs->get_sequencing_int(i);

		pPkt = new Dvb_pkt();
		assert(pPkt);
		pPkt->pkt_attach((void*)cpy , UNKNOWN_LEN);
		pPkt->pkt_settype(PKT_TABLE);
		pPkt->pkt_setflag(FLOW_SEND);
		(pPkt->pkt_getfwdinfo())->pid = INT_PID;
		pEvn = createEvent();
		assert(pEvn);
		pEvn->DataInfo_ = pPkt;
	
		NslObject::send(pEvn);
	}

	return 1;
}

/*
 * get_rcst_pid function
 * Usage : get PID according to host's dst_ipaadr and search _RCST_info
 */
int16_t
Sp_ctl::get_rcst_pid(u_long dst_ipaddr) { 

	for (int i = 0 ; i < _Rcst_info_num ; i++) 
	{
		if (!memcmp(&((_RCST_info[i])._Rcst_IPaddr) , &dst_ipaddr , 4))
		{
			return (_RCST_info[i]._PID);
		}

		for (int j = 0 ; j < _RCST_info[i].host_num ; j++)
		{
			if (!memcmp(&((_RCST_info[i])._Hosts_IPaddr[j]) , &dst_ipaddr , 4))
				return _RCST_info[i]._PID;

		}
	}
	return -1;
}

/*
 * set_rcst_info function
 * non-implement yet.
 */
int
Sp_ctl::set_rcst_info() {

	return 1;
}

/*
 * set_PAT function
 * non-implement yet.
 */
int
Sp_ctl::set_PAT() {

	return 1;
}

/*
 * set_PMT function
 * non-implement yet.
 */
int
Sp_ctl::set_PMT() {

	return 1;
}

/*
 * set_INT function
 * non-implement yet.
 */
int
Sp_ctl::set_INT() {

	return 1;
}

/*
 * set_NIT function
 * non-implement yet.
 */
int
Sp_ctl::set_NIT() {

	return 1;
}
