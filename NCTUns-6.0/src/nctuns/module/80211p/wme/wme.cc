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

/*	Version Information. (WME Lite)
 *
 *	Only support one antenna.
 *	WAVE Device can't be user and provider at the same time.
 *	Provider service and User Service only support one action(add).
 *	CCH service not support
 *	user-service only support normal channel change.(not support immediate/indefinite access)
 *	
 *	2009 April Wei-Jyun Hong
 */


#include <stdlib.h>
#include <assert.h>
#include <object.h>
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <exportStr.h>
#include <ip.h>
#include <ethernet.h>
#include <80211p/wme/wme.h>
#include <80211p/mac/mac_80211p.h>
#include <nctuns-dep/node.h>
#include <mac-802_11.h>
#include <mbinder.h>
#include <command_server.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

MODULE_GENERATOR(WME);
WME::WME(u_int32_t type, u_int32_t id, struct plist* pl, const char *name) : NslObject(type, id, pl, name)
{
	CCH 		= 	178;
	SCH[0]		=	174;
	SCH[1]		=	175;
	SCH[2]		=	176;
	SCH[3]		=	180;
	SCH[4]		=	181;
	SCH[5]		=	182;

	bzero(&mib, sizeof(struct MIB));
	bzero(&user_list, sizeof(struct User_List));
	bzero(&sch_state, sizeof(struct SCH_State));
	bzero(&wsa, sizeof(struct WSA));
	wme_primitive_q	=	NULL;
	device_status	=	none;
	//vBind_mac("mac", my_mac_addr);
	my_mac_addr	=	NULL;
	recv_mac_addr	=	NULL;
	wsa_data	=	NULL;
	vBind("cfg_file", &primitive_file);

	wsa.wsa_type = 0;
	wsa.wsa_hdr.wave_ver = 1;
	//wsa.wsa_hdr.aid
	wsa.wsa_hdr.aid_len = 0;
	wsa.wsa_hdr.hdr_len = 4 + wsa.wsa_hdr.aid_len;	//wave version + header contents + repeats/persince + aid length + aid
	wsa.wsa_hdr.hdr_cont = 0;

	wsa.pst.provider_count = 0;
	wsa.pst.channel_count = 0;
	global_repeats = 0;

	//tcl 更新 加這兩項
	mib.local_info.wsm_forwarder_port = 5000;
	mib.local_info.wsm_max_len = 1400;
}

WME::~WME()
{
	//USER LIST 和 WSA 要 free

	while(wme_primitive_q != NULL)
	{
		struct WME_Primitive_Q	*tmp;
		tmp = wme_primitive_q;
		wme_primitive_q	= wme_primitive_q->next;
		free(tmp);
	}

	if(wsa_data != NULL)
		delete wsa_data;
	
	while(user_list.joined_count > 0)
	{
		struct ServiceIndex *tmp_service_index;

		tmp_service_index = user_list.joined_service;
		user_list.joined_service = user_list.joined_service->next;
		delete tmp_service_index;
		user_list.joined_count--;
	}
	
	while(wsa.pst.provider_count > 0)
	{
		struct PST_Entry *tmp_pst_entry;
		tmp_pst_entry = wsa.pst.pst_first;
		wsa.pst.pst_first = wsa.pst.pst_first->next;
		delete tmp_pst_entry;
		wsa.pst.provider_count --;
	}

	while(wsa.pst.channel_count > 0)
	{
		struct WSA_ChannelInfo *tmp_ci;
		tmp_ci = wsa.pst.ci_first;
		wsa.pst.ci_first = wsa.pst.ci_first->next;
		delete tmp_ci;
		wsa.pst.channel_count --;
	}

}
int WME::init()
{
	extern cmd_server * cmd_server_;

	nid = get_nid();
	my_mac_addr =  GET_REG_VAR1(get_portls(), "MAC", u_char *);
	if(my_mac_addr == NULL)
		exit(0);

	//printf("my_mac_addr(%d) = (%x:%x:%x:%x:%x:%x) \n", nid, my_MACaddr[0], my_MACaddr[1], my_MACaddr[2], my_MACaddr[3], my_MACaddr[4], my_MACaddr[5]);
	stack_port = get_port();
	cmd_server_->moduleCreateRegInfo(nid, stack_port/*, this*/);

	//for read config file test
	//char *test = new(char[40]);
	//primitive_file = test;
	//strcpy(primitive_file, "test.primitive");
	wme_primitive_q = read_primitive_setting();
	//delete test;

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(WME, set_primitive_timer);
	primitive_t.setCallOutObj(this, type);

	type = POINTER_TO_MEMBER(WME, set_wsa_timer);
	wsa_t.setCallOutObj(this, type);

	if(wme_primitive_q != NULL)
		primitive_t.start((wme_primitive_q->time - GetCurrentTime()), 0);

	return 1;
}

int WME::set_wsa_timer(Event_ *e)
{
	int repeats, persistence;

	repeats = wsa.wsa_hdr.PR/2 + 1;
	if(repeats > 8)
		repeats = 8;

	persistence = wsa.wsa_hdr.PR % 2;

	if(global_repeats == 0)
	{
		if(persistence == 0)
		{
			wsa_send_t[0] = (1 + (rand() % 49)) * 10000; //1~49 ms
			wsa_t.start(wsa_send_t[0], 0);
		}
		else	//generate repeats number from 1 to 49
		{
			int i, j;
			uint64_t max;

			for(i=0; i< repeats; i++)
			{
				wsa_send_t[i] = (1 + (rand() % 49)) * 10000;
				for(j = 0; j<i; j++)
					if(wsa_send_t[j] == wsa_send_t[i])
					{
						wsa_send_t[i] = (1 + (rand() % 49)) * 10000;
						j = 0;
					}
			}

			for(i=0; i< repeats; i++)
			{
				max = wsa_send_t[i];
				for(j = i+1; j< repeats; j++)
				{
					if(wsa_send_t[j] > max)
					{
						max = wsa_send_t[j];
						wsa_send_t[j] = wsa_send_t[i];
						wsa_send_t[i] = max;
					}
				}
			}
			
			global_repeats = repeats;
			wsa_t.start(wsa_send_t[global_repeats-1], 0);
		}
	}
	else
	{
		if(persistence == 1)
		{
			global_repeats --;
			if(global_repeats == 0)
				wsa_t.start(1000000 - wsa_send_t[0], 0);	//sand wsa on next CCH
			else
				wsa_t.start(wsa_send_t[global_repeats -1] - wsa_send_t[global_repeats], 0); //send on this CCH
		}
		
		send_wsa();
	}

	return 1;
}

int WME::set_primitive_timer(Event_ *e)
{
	struct WME_Primitive_Q *tmp;
	tmp = wme_primitive_q;
	wme_primitive_q = wme_primitive_q->next;

	if(tmp->wme_primitive_type == provider_service_req)
		wme_provider_service_req(tmp);
	else if(tmp->wme_primitive_type == user_service_req)
		wme_user_service_req(tmp);
	else if(tmp->wme_primitive_type == wsm_service_req)
		wme_wsm_service_req(tmp);
	else
		wme_cch_service_req(tmp);

	delete tmp;

	if(wme_primitive_q != NULL)
		primitive_t.start((wme_primitive_q->time - GetCurrentTime()), 0);

	return 1;
}

void WME::wme_provider_service_req(struct WME_Primitive_Q *tmp)
{
	if(tmp->action == add)
	{
		struct ServiceIndex *tmp_index = user_list.joined_service;
		
		if(device_status == user)	// not support user and provider in one device at the some time (at this version)
			return;

		while(tmp_index != NULL)
		{
			if(mib.user_service_info[tmp_index->index].psid == tmp->provider_service_req.psid)
			{
				printf("[WME] provider.service.req falt(add), this device has joined the same WBSS of PSID\n");
				return;
			}

			tmp_index = tmp_index->next;
		}

		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.provider_service_info[i].service_state == used && mib.provider_service_info[i].psid == tmp->provider_service_req.psid)
			{
				printf("[WME] provider.service.req falt(add), PSID has been registered\n");
				return;
			}

		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.provider_service_info[i].service_state == empty)		//record this request to mib add doing this request
			{
				//	set mib
				mib.provider_service_info[i].wsa_type = 0;		//only support non-secured WSA
				mib.provider_service_info[i].psid = tmp->provider_service_req.psid;
				memcpy(mib.provider_service_info[i].psc, tmp->provider_service_req.psc, MaxProviderServiceContext);
				mib.provider_service_info[i].app_priority =  tmp->provider_service_req.app_priority;
				memcpy(mib.provider_service_info[i].recipient_mac, tmp->provider_service_req.reciptent_mac_addr, 6);
				mib.provider_service_info[i].channle_selection = tmp->provider_service_req.channle_selection;
				mib.provider_service_info[i].persistence = tmp->provider_service_req.persistence;
				mib.provider_service_info[i].repeats = tmp->provider_service_req.repeats;
				mib.provider_service_info[i].ip_service = tmp->provider_service_req.ip_service;
				memcpy(mib.provider_service_info[i].ipv6_addr, tmp->provider_service_req.ipv6_addr, 16);
				mib.provider_service_info[i].service_port = tmp->provider_service_req.service_port;
				memcpy(mib.provider_service_info[i].provider_mac_addr, tmp->provider_service_req.provider_mac_addr, 6);
				mib.provider_service_info[i].service_state = used;

				//	set service && WSA
				recv_mac_addr = mib.provider_service_info[i].recipient_mac;
				start_provider_service(i);
				return;
			}

		printf("[WME] provider.service.req(add) falt, MIB full\n");
	}
	else if(tmp->action == change)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.provider_service_info[i].service_state == used && mib.provider_service_info[i].psid == tmp->provider_service_req.psid)
			{
				memcpy(mib.provider_service_info[i].psc, tmp->provider_service_req.psc, MaxProviderServiceContext);
				mib.provider_service_info[i].app_priority =  tmp->provider_service_req.app_priority;
				memcpy(mib.provider_service_info[i].recipient_mac, tmp->provider_service_req.reciptent_mac_addr, 6);
				mib.provider_service_info[i].channle_selection = tmp->provider_service_req.channle_selection;
				mib.provider_service_info[i].persistence = tmp->provider_service_req.persistence;
				mib.provider_service_info[i].repeats = tmp->provider_service_req.repeats;
				mib.provider_service_info[i].ip_service = tmp->provider_service_req.ip_service;
				memcpy(mib.provider_service_info[i].ipv6_addr, tmp->provider_service_req.ipv6_addr, 16);
				mib.provider_service_info[i].service_port = tmp->provider_service_req.service_port;
				memcpy(mib.provider_service_info[i].provider_mac_addr, tmp->provider_service_req.provider_mac_addr, 6);
				mib.provider_service_info[i].service_state = used;

				//	change service and WSA
				recv_mac_addr = mib.provider_service_info[i].recipient_mac;
				modify_wsa(i);		//未完成
				return;
			}

		printf("[WME] provider.service.req(change) falt, can't find the same PSID in MIB\n");
	}
	else if(tmp->action == del)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.provider_service_info[i].service_state == used && mib.provider_service_info[i].psid == tmp->provider_service_req.psid)
			{
				bzero(&(mib.provider_service_info[i]), sizeof(struct ProviderServiceInfo));

				//	delete service and WSA
				del_wsa(tmp->provider_service_req.psid);		//未完成
				return;
			}

		printf("[WME] provider.service.req(del) falt, can't find the same PSID in MIB\n");
	}
}
void WME::wme_user_service_req(struct WME_Primitive_Q *tmp)
{
	if(tmp->action == add)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.user_service_info[i].service_state == used && mib.user_service_info[i].psid == tmp->user_service_req.psid)
			{
				printf("[WME] user.service.req falt, PSID has registered\n");
				return;
			}

		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.user_service_info[i].service_state == empty)
			{
				//	set mib
				mib.user_service_info[i].psid = tmp->user_service_req.psid;
				memcpy(mib.user_service_info[i].psc, tmp->user_service_req.psc, MaxProviderServiceContext);
				memcpy(mib.user_service_info[i].source_mac_addr, tmp->user_service_req.source_mac_addr, 6);
				memcpy(mib.user_service_info[i].aid, tmp->user_service_req.aid, MaxAdvertiserIdentifier);
				mib.user_service_info[i].channel_info.channel_num = tmp->user_service_req.channel_num;
				mib.user_service_info[i].immediate_access = tmp->user_service_req.immediate_access;
				mib.user_service_info[i].indefinite_access = tmp->user_service_req.indefinite_access;
				mib.user_service_info[i].service_state = used;

				//	set service && WSA
				return;
			}

		printf("[WME] user.service.req(add) falt, MIB full\n");
	}
	else if(tmp->action == del)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.user_service_info[i].service_state == used && mib.user_service_info[i].psid == tmp->user_service_req.psid)
			{
				bzero(&(mib.user_service_info[i]), sizeof(struct UserServiceInfo));

				//	delete service
				return;
			}

		printf("[WME] user.service.req(del) falt, can't find the same PSID in MIB\n");
	}
}
void WME::wme_wsm_service_req(struct WME_Primitive_Q *tmp)
{
	if(tmp->action == add)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.wsm_service_info[i].service_state == used && mib.wsm_service_info[i].psid == tmp->wsm_service_req.psid)
			{
				printf("[WME] wsm.service.req falt, PSID has registered\n");
				return;
			}

		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.wsm_service_info[i].service_state == empty)		//record this request to mib add doing this request
			{
				//	set mib
				mib.wsm_service_info[i].psid = tmp->wsm_service_req.psid;
				mib.wsm_service_info[i].service_state = used;

				//	set service && WSA
				return;
			}

		printf("[WME] wsm.service.req(add) falt, MIB full\n");
	}
	else if(tmp->action == del)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.wsm_service_info[i].service_state == used && mib.wsm_service_info[i].psid == tmp->wsm_service_req.psid)
			{
				bzero(&(mib.wsm_service_info[i]), sizeof(struct WsmServiceInfo));

				//	delete service
				return;
			}

		printf("[WME] wsm.service.req(del) falt, can't find the same PSID in MIB\n");
	}
}
void WME::wme_cch_service_req(struct WME_Primitive_Q *tmp)
{
	if(tmp->action == add)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.cch_service_info[i].service_state == used && mib.cch_service_info[i].app_priority == tmp->cch_service_req.app_priority)
			{
				printf("[WME] cch.service.req falt, application priority has registered\n");
				return;
			}

		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.cch_service_info[i].service_state == empty)		//record this request to mib add doing this request
			{
				//	set mib
				mib.cch_service_info[i].app_priority = tmp->cch_service_req.app_priority;
				mib.cch_service_info[i].service_state = used;

				//	set service && WSA
				return;
			}

		printf("[WME] cch.service.req(add) falt, MIB full\n");
	}
	else if(tmp->action == del)
	{
		for(int i=0; i< MaxServceInfoEntry; i++)
			if(mib.cch_service_info[i].service_state == used && mib.cch_service_info[i].app_priority == tmp->cch_service_req.app_priority)
			{
				bzero(&(mib.cch_service_info[i]), sizeof(struct CchServiceInfo));

				//	delete service
				return;
			}

		printf("[WME] cch.service.req(del) falt, can't find the same PSID in MIB\n");
	}
}

void WME::start_provider_service(int index)
{
	//	check channel
	if(sch_state.count == 1)//antenna_num )
		for(int i=0; i<NumOfSch; i++)
			if(sch_state.used[i] == 0 && mib.provider_service_info[index].channle_selection == SCH[i])
			{
				printf("[WME] Start WBSS falt, can't not access channel\n");
				bzero(&(mib.provider_service_info[index]), sizeof(struct ProviderServiceInfo));
				return;
			}

	for(int i=0; i<NumOfSch; i++)
		if(mib.provider_service_info[index].channle_selection == SCH[i])
		{
			if(sch_state.used[i] == 0)
				sch_state.count ++;
			sch_state.used[i] ++;
			break;
		}

	//	change state
	if(device_status == none)
		device_status = provider;
	/*if(device_status == user)
	  device_status = p_u;*/

	//	WSA
	if(mib.provider_service_info[index].persistence == true)
		wsa.wsa_hdr.PR = 1 + mib.provider_service_info[index].repeats * 2;
	wsa.pst.provider_count++;

	struct PST_Entry *tmp_pst_entry = new(struct PST_Entry);

	bzero(tmp_pst_entry, sizeof(struct PST_Entry));
	tmp_pst_entry->next = wsa.pst.pst_first;
	wsa.pst.pst_first = tmp_pst_entry;

	tmp_pst_entry->provider_len = 0;

	tmp_pst_entry->provider_cont = 0;
	tmp_pst_entry->provider_len += 1;

	tmp_pst_entry->psid = mib.provider_service_info[index].psid;
	tmp_pst_entry->provider_len += 4;

	tmp_pst_entry->psc_len = strlen(mib.provider_service_info[index].psc);
	tmp_pst_entry->provider_len += 1;

	strcpy(tmp_pst_entry->psc, mib.provider_service_info[index].psc);
	tmp_pst_entry->provider_len += tmp_pst_entry->psc_len;

	tmp_pst_entry->app_priority = mib.provider_service_info[index].app_priority;
	tmp_pst_entry->provider_len += 1;

	//recipient MAC Address
	//
	tmp_pst_entry->channel_num = mib.provider_service_info[index].channle_selection;
	tmp_pst_entry->provider_len += 1;

	if(mib.provider_service_info[index].ip_service == true)
		wsa.wra_len = 58;
	else
		wsa.wra_len = 0;

	u_char tmp_chr[16];
	bzero(tmp_chr, 16);

	memcpy(tmp_pst_entry->ipv6_addr, mib.provider_service_info[index].ipv6_addr, 16);
	if(memcmp(tmp_pst_entry->ipv6_addr, tmp_chr, 16) != 0)
	{
		tmp_pst_entry->provider_cont += 2;
		tmp_pst_entry->provider_len += 16;
	}

	tmp_pst_entry->service_port = mib.provider_service_info[index].service_port;
	if(tmp_pst_entry->service_port != 0)
	{
		tmp_pst_entry->provider_cont += 4;
		tmp_pst_entry->provider_len += 2;
	}


	memcpy(tmp_pst_entry->provider_mac_addr, mib.provider_service_info[index].provider_mac_addr, 6);
	if(memcmp(tmp_pst_entry->provider_mac_addr, tmp_chr, 6) != 0)
	{
		tmp_pst_entry->provider_cont += 16;
		tmp_pst_entry->provider_len += 6;
	}

	if(wsa.pst.channel_count == 0)
	{
		struct WSA_ChannelInfo *tmp_ci = new(struct WSA_ChannelInfo);
		bzero(tmp_ci, sizeof(struct WSA_ChannelInfo));

		tmp_ci->next = wsa.pst.ci_first;
		wsa.pst.ci_first = tmp_ci;
		wsa.pst.channel_count = 1;
		tmp_ci->channel_len = 5;
		tmp_ci->channel_cont = 0;
		tmp_ci->channel_num = mib.provider_service_info[index].channle_selection;
	}

	generate_wsa();
	((mac_80211p *)InstanceLookup(nid, stack_port, "MAC80211p"))->start_channel_switch(mib.provider_service_info[index].channle_selection, 0, mib.provider_service_info[index].psid);

	global_repeats = 0;
	set_wsa_timer(NULL);

}

void WME::send_wsa()
{
	ePacket_                *epkt;
	Packet                  *pkt;
	struct ether_header     *eh;
	struct hdr_mac802_11    *mac_hdr;
	char                    *data;
	u_char 			timing_info[8];
	u_char 			tmp_chr[6];

	/*	WAVE advertisement
	 *	
	 *	| MAC Header | Timing Info | WSA |
	 */

	epkt = createPacket();
	GET_PKT(pkt, epkt);

	bzero(tmp_chr, 6);
	bzero(timing_info, 8);
	//ether header
	eh = (struct ether_header *)pkt->pkt_malloc(sizeof(struct ether_header));
	if(memcmp(recv_mac_addr, tmp_chr, 6) == 0)
		memcpy(eh->ether_dhost, ETHER_BROADCAST, 6);
	else
		memcpy(eh->ether_dhost, recv_mac_addr, 6);
	memcpy(eh->ether_shost, my_mac_addr, 6);
	eh->ether_type = htons(1);

	mac_hdr = (struct hdr_mac802_11 *)pkt->pkt_malloc(sizeof(struct hdr_mac802_11));
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_Beacon;
	mac_hdr->dh_fc.fc_type = MAC_Type_Management;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;

	memcpy((char *)mac_hdr->dh_addr1,(char *)eh->ether_dhost, 6);
	memcpy((char *)mac_hdr->dh_addr2,(char *)my_mac_addr, 6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)my_mac_addr, 6);

	data = pkt->pkt_sattach(sizeof(timing_info) + wsa_len);
	pkt->pkt_sprepend(data, sizeof(timing_info) + wsa_len);

	memcpy(data, timing_info, sizeof(timing_info));
	memcpy((data + sizeof(timing_info)), wsa_data, wsa_len);

	put(epkt, sendtarget_);
}

void WME::recv_wsa(Packet *pkt)
{
	struct ether_header     *eh;
	char *			data;
	struct WSA		tmp_wsa;
	int			index, i, j;

	if( device_status == provider)
		return;

	bzero(&tmp_wsa, sizeof(struct WSA));
	index = 0;

	eh = (struct ether_header *)pkt->pkt_get();
	data = pkt->pkt_sget();


	data +=  8;	//not support timing info
	// wsa type
	tmp_wsa.wsa_type = (uint8_t)*(data + index);
	index += 1;

	//wsa header
	tmp_wsa.wsa_hdr.wave_ver = (uint8_t)*(data + index);
	index += 1;
	tmp_wsa.wsa_hdr.hdr_len = (uint8_t)*(data + index);
	index += 1;
	tmp_wsa.wsa_hdr.hdr_cont = (uint8_t)*(data + index);
	index += 1;
	tmp_wsa.wsa_hdr.PR = (uint8_t)*(data + index);
	index += 1;
	//tmp_wsa.wsa_hdr.transmit_power		not support in WME Lite
	//tmp_wsa.wsa_hdr.tarnsmit_location		not support in WME Lite
	tmp_wsa.wsa_hdr.aid_len = (uint8_t)*(data + index);
	index += 1;
	memcpy(tmp_wsa.wsa_hdr.aid, data + index, tmp_wsa.wsa_hdr.aid_len);
	index += tmp_wsa.wsa_hdr.aid_len;

	//PST
	tmp_wsa.pst.provider_count = (uint8_t)*(data + index);
	index += 1;
	//PST entry
	for(i=0; i<tmp_wsa.pst.provider_count; i++)
	{
		struct PST_Entry tmp_pst_entry;

		tmp_pst_entry.provider_len = (uint8_t)*(data + index);
		index += 1;
		tmp_pst_entry.provider_cont = (uint8_t)*(data + index);
		index += 1;
		tmp_pst_entry.psid = (uint32_t)*(data + index);
		index += 4;
		tmp_pst_entry.psc_len = (uint8_t)*(data + index);
		index += 1;
		memcpy(tmp_pst_entry.psc, data + index, tmp_pst_entry.psc_len);
		tmp_pst_entry.psc[tmp_pst_entry.psc_len] = 0;
		index += tmp_pst_entry.psc_len; 
		tmp_pst_entry.app_priority = (uint8_t)*(data + index);
		index += 1;
		tmp_pst_entry.channel_num = (uint8_t)*(data + index);
		index += 1;
		if(tmp_pst_entry.provider_cont & 2)
		{
			memcpy(tmp_pst_entry.ipv6_addr, (u_char *)(data + index), 16);
			index += 16;
		}
		if(tmp_pst_entry.provider_cont & 4)
		{
			tmp_pst_entry.service_port = (uint16_t)*(data + index);;
			index += 2;
		}
		if(tmp_pst_entry.provider_cont & 8)
			index += 1;
		if(tmp_pst_entry.provider_cont & 16)
		{
			memcpy(tmp_pst_entry.provider_mac_addr, (u_char *)(data + index), 6);
			index += 6;
		}

		for(j = 0; j< MaxServceInfoEntry; j++)			// finnd the match psid for available service
			if(mib.available_service_info[j].service_state == used && mib.available_service_info[j].psid == tmp_pst_entry.psid)
				break;
		if(j == MaxServceInfoEntry)				// find empty available service info in mib
			for(j = 0; j< MaxServceInfoEntry; j++)
				if(mib.available_service_info[j].service_state == empty)
					break;
		if(j == MaxServceInfoEntry)
		{
			printf("[WME recv_wsa] Receive WSA has new service, but no empty available_service_info to store it.\n");
			continue;
		}

		mib.available_service_info[j].psid = tmp_pst_entry.psid;
		strcpy(mib.available_service_info[j].psc, tmp_pst_entry.psc);
		mib.available_service_info[j].app_priority = tmp_pst_entry.app_priority;
		mib.available_service_info[j].channel_info.channel_num = tmp_pst_entry.channel_num;
		memcpy(mib.available_service_info[j].souece_mac_addr, eh->ether_shost, 6);

		memcpy(mib.available_service_info[j].aid, tmp_wsa.wsa_hdr.aid, tmp_wsa.wsa_hdr.aid_len);
		mib.available_service_info[j].aid[tmp_wsa.wsa_hdr.aid_len] = 0;

		memcpy(mib.available_service_info[j].ipv6_addr, tmp_pst_entry.ipv6_addr, 16);
		mib.available_service_info[j].service_port = tmp_pst_entry.service_port;
		memcpy(mib.available_service_info[j].provider_mac_addr, tmp_pst_entry.provider_mac_addr, 6);
		mib.available_service_info[j].service_state = used;

		mib.available_service_info[j].repeats = tmp_wsa.wsa_hdr.PR/2;
		mib.available_service_info[j].cch_recvs ++;
	}

	tmp_wsa.pst.channel_count = (uint8_t)*(data + index);
	index += 1;

	for(i=0; i<tmp_wsa.pst.channel_count; i++)
	{
		index += (1 + ((uint8_t)*(data + index)));	//channel length + channel info
	}
	//Channel info
	//WRA not support

}

void WME::generate_wsa()
{
	int wsa_index;
	struct PST_Entry        *tmp_pst_entry;
	struct WSA_ChannelInfo  *tmp_ci;
	char *tmp_wsa;

	if(wsa_data != NULL)
	{
		delete wsa_data;
		wsa_data = NULL;
	}

	wsa_len = 2 + wsa.wsa_hdr.hdr_len;	//wsa_type + wsa header length + wsa header

	wsa_len += 1;				//wsa provider count

	tmp_pst_entry = wsa.pst.pst_first;
	while(tmp_pst_entry != NULL)		//wsa pst_entry
	{
		wsa_len += (tmp_pst_entry->provider_len + 1); //pst entry + pst length
		tmp_pst_entry = tmp_pst_entry->next;
	}

	wsa_len += 1;				//channel count

	tmp_ci =  wsa.pst.ci_first;
	while(tmp_ci != NULL)
	{
		wsa_len += (tmp_ci->channel_len + 1); //channel length + channel info
		tmp_ci = tmp_ci->next;
	}

	wsa_len += (1 + wsa.wra_len); 	//wra length + wra


	tmp_wsa = new(char[wsa_len]);
	bzero(tmp_wsa, wsa_len);
	wsa_index = 0;

	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_type), 1, wsa_index, wsa_len);
	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_hdr.wave_ver), 1, wsa_index, wsa_len);
	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_hdr.hdr_len), 1, wsa_index, wsa_len);
	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_hdr.hdr_cont), 1, wsa_index, wsa_len);
	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_hdr.PR), 1, wsa_index, wsa_len);
	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_hdr.aid_len), 1, wsa_index, wsa_len);
	wsa_index = fill_data(tmp_wsa, &(wsa.wsa_hdr.aid), wsa.wsa_hdr.aid_len, wsa_index, wsa_len);

	wsa_index = fill_data(tmp_wsa, &(wsa.pst.provider_count), 1, wsa_index, wsa_len);
	tmp_pst_entry = wsa.pst.pst_first;
	while(tmp_pst_entry != NULL)
	{
		wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->provider_len), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->provider_cont), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->psid), 4, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->psc_len), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, tmp_pst_entry->psc, tmp_pst_entry->psc_len, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->app_priority), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->channel_num), 1, wsa_index, wsa_len);
		if(tmp_pst_entry->provider_cont & 2)
			wsa_index = fill_data(tmp_wsa, tmp_pst_entry->ipv6_addr, 16, wsa_index, wsa_len);
		if(tmp_pst_entry->provider_cont & 4)
			wsa_index = fill_data(tmp_wsa, &(tmp_pst_entry->service_port), 2, wsa_index, wsa_len);
		if(tmp_pst_entry->provider_cont & 16)
			wsa_index = fill_data(tmp_wsa, tmp_pst_entry->provider_mac_addr, 6, wsa_index, wsa_len);

		tmp_pst_entry = tmp_pst_entry->next;
	}

	wsa_index = fill_data(tmp_wsa, &(wsa.pst.channel_count), 1, wsa_index, wsa_len);
	tmp_ci = wsa.pst.ci_first;
	while(tmp_ci != NULL)
	{
		wsa_index = fill_data(tmp_wsa, &(tmp_ci->channel_len), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_ci->channel_cont), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_ci->channel_num), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_ci->adaptable), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_ci->data_rate), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(tmp_ci->txpwr_level), 1, wsa_index, wsa_len);

		tmp_ci = tmp_ci->next;
	}

	wsa_index = fill_data(tmp_wsa, &(wsa.wra_len), 1, wsa_index, wsa_len);

	if(wsa.wra_len > 0)
	{
		wsa_index = fill_data(tmp_wsa, &(wsa.wra.wra_cont), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(wsa.wra.router_lifetime), 2, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, wsa.wra.ip_prefix, 16, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, &(wsa.wra.prefix_len), 1, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, wsa.wra.default_gateway, 16, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, wsa.wra.gateway_mac_addr, 6, wsa_index, wsa_len);
		wsa_index = fill_data(tmp_wsa, wsa.wra.primary_dns, 16, wsa_index, wsa_len);
		if(wsa.wra.wra_cont & 2)
			wsa_index = fill_data(tmp_wsa, wsa.wra.second_dns, 16, wsa_index, wsa_len);
	}

	if(wsa_index != wsa_len)
	{
		printf("WSA frame error. Please check WSA field and length.\n");
		assert(0);
	}

	wsa_data = tmp_wsa;
}

int WME::fill_data(void *dst, void *src, int size, int index, int max)
{
	if(size + index > max)
	{
		printf("[FILL WSA] size error\n");
		assert(0);
	}

	memcpy(((char *)dst) + index, src, size);
	return (index + size);
}

int WME::get_line(int fd, char *mesg, int n)
{
	int i, rc;
	for(i=0 ; i < n-1 ; i++)
	{
		if((rc = read(fd, &mesg[i], 1)) == 1)
		{
			if(mesg[i] == '\n')
			{
				mesg[i] = 0;
				return i;
			}
		}
		else if(rc == 0)
		{
			mesg[i] = 0;
			return i;
		}
		else
			return 0;
	}
	mesg[i] = 0;
	return i;
}

int WME::get_token(char *str, char **token, char *param)
{
	char *cmd_line;
	int i;

	if(str[0] == '#')
		return 0;
	cmd_line = strtok(str, "#");
	token[0] = strtok(cmd_line, param);

	if(token[0] == NULL)
		return 0;

	for(i=1; i< 10; i++ )
	{
		token[i] = strtok(NULL, param);
		if(token[i] == NULL)
			break;
	}

	return i;
}

struct WME_Primitive_Q *WME::read_primitive_setting()
{
	char *work_dir, file_dir[400], str_line[200], *token[10];
	struct WME_Primitive_Q *first, *end, *tmp;
	int fd, token_num;
	int set_nid, set_sib, set_cdb;

	work_dir = getenv("NCTUNS_WORKDIR");

	if(work_dir != NULL)
		strcpy(file_dir, work_dir);
	else
		return NULL;

	if(file_dir[strlen(file_dir)-1] != '/')
	{
		file_dir[strlen(file_dir)+1] = 0;
		file_dir[strlen(file_dir)] = '/';
	}

	if(primitive_file == NULL)
		return NULL;

	strcpy(file_dir+strlen(file_dir), primitive_file);
	//printf("\nOpen WAVE Application Configure file %s success.\n", file_dir);
	fd = open(file_dir, O_RDONLY);

	first = end = NULL;
	if(fd > 0)
	{
		set_nid = set_sib = set_cdb = 0;
		while(get_line(fd, str_line, 200) > 0)
		{
			if(strlen(str_line)==0)
				continue;
			token_num = get_token(str_line, token, (char *)" \t");

			if( token_num <= 0 )
				continue;
			if(set_sib == 1)
			{
				if(strcmp(token[0], "SIB_End") == 0)
					set_sib = -1;
				else if(set_nid == 0)
				{
					if(strcmp(token[0], "NID") == 0 && token_num == 2 && atoi(token[1])==nid)
						set_nid = nid;
				}
				else if(set_cdb == 0)
				{
					if(strcmp(token[0], "NID") == 0 && token_num == 2)
					{
						if(atoi(token[1])==nid)
							set_nid = nid;
						else
							set_nid = 0;
					}
					else if(strcmp(token[0], "CDB") == 0)
						set_cdb = 1;
				}
				else
				{
					if(strcmp(token[0], "CDE") == 0)
						set_cdb = 0;
					else
					{
						if(set_cdb == 1)
						{
							tmp = new(struct WME_Primitive_Q);
							bzero(tmp, sizeof(struct WME_Primitive_Q));
							set_cdb = 2;
						}
						if(set_cdb == 2)
						{
							if(strcmp(token[0], "Time") == 0)		//sort the service primitive queue
							{
								struct WME_Primitive_Q *sort_1, *sort_2;
								
								tmp->time = atoll(token[1]);
								
								if(first == NULL)
									first = end = tmp;
								else
								{
									sort_1 = sort_2 = first;
									while(sort_2 != NULL)
									{
										if(tmp->time < sort_2->time)
										{
											tmp->next = sort_2;
											if(sort_2 == first)
												first = tmp;
											else
												sort_1->next = tmp;
											
											break;
										}
										else
										{
											sort_1 = sort_2;
											sort_2 = sort_2->next;
										}
									}
									if(sort_2 == NULL)
									{
										end->next = tmp;
										end = tmp;
									}
								}

							}
							else if(strcmp(token[0], "Primitive") == 0)
							{
								if(strcmp(token[1], "provider_service_req") == 0)
									tmp->wme_primitive_type = provider_service_req;
								else if(strcmp(token[1], "user_service_req") == 0)
									tmp->wme_primitive_type = user_service_req;
								else if(strcmp(token[1], "wsm_service_req") == 0)
									tmp->wme_primitive_type = wsm_service_req;
								else if(strcmp(token[1], "cch_service_req") == 0)
									tmp->wme_primitive_type = cch_service_req;
							}
							else if(strcmp(token[0], "Action") == 0)
							{
								if(strcmp(token[1], "add") == 0)
									tmp->action = add;
								else if(strcmp(token[1], "del") == 0)
									tmp->action = del;
								else if(strcmp(token[1], "change") == 0)
									tmp->action = change;
								set_cdb = 3;
							}
						}
						else if(set_cdb == 3)
						{
							if(tmp->wme_primitive_type == provider_service_req)
							{
								if(strcmp(token[0], "PSID") == 0)
									(tmp->provider_service_req).psid = atol(token[1]);
								if(strcmp(token[0], "PSC") == 0)
								{
									char *psc_tmp;
									psc_tmp = strtok(token[1], "\"");
									if(psc_tmp == NULL)
										psc_tmp = (char *)"";
									strcpy((tmp->provider_service_req).psc, psc_tmp);
								}
								if(strcmp(token[0], "AppPriority") == 0)
									(tmp->provider_service_req).app_priority = atoi(token[1]);
								if(strcmp(token[0], "Channel") == 0)
									(tmp->provider_service_req).channle_selection = atoi(token[1]);
								if(strcmp(token[0], "Persistence") == 0)
								{
									if(atoi(token[1]) == 1)
										(tmp->provider_service_req).persistence = true;
									else
										(tmp->provider_service_req).persistence = false;
								}
								if(strcmp(token[0], "Repeats") == 0)
									(tmp->provider_service_req).repeats = atoi(token[1]);
								if(strcmp(token[0], "IPService") == 0)
								{
									if(atoi(token[1]) == 1)
										(tmp->provider_service_req).ip_service = true;
									else
										(tmp->provider_service_req).ip_service = false;
								}	
								if(strcmp(token[0], "IPAddr") == 0)
								{
									char *ip_tmp[5];
									get_token(token[1], ip_tmp, (char *)".");
									for(int i=0; i<4; i++)
										(tmp->provider_service_req).ipv6_addr[i] = atoi(ip_tmp[i]);
								}
								if(strcmp(token[0], "ServicePort") == 0)
									(tmp->provider_service_req).service_port = atoi(token[1]);
								if(strcmp(token[0], "MacAddr") == 0)
								{
									char *mac_tmp[7];
									get_token(token[1], mac_tmp, (char *)":");
									for(int i=0; i<6; i++)
										(tmp->provider_service_req).provider_mac_addr[i] = atoi(mac_tmp[i]);
								}
								if(strcmp(token[0], "RecipientMacAddr") == 0)
								{
									char *mac_tmp[7];
									get_token(token[1], mac_tmp, (char *)":");
									for(int i=0; i<6; i++)
										(tmp->provider_service_req).reciptent_mac_addr[i] = atoi(mac_tmp[i]);
								}
							}
							else if(tmp->wme_primitive_type == user_service_req)
							{
								if(strcmp(token[0], "PSID") == 0)
									(tmp->user_service_req).psid = atol(token[1]);
								if(strcmp(token[0], "PSC") == 0)
								{
									char *psc_tmp;
									psc_tmp = strtok(token[1], "\"");
									if(psc_tmp == NULL)
										psc_tmp = (char *)"";
									strcpy((tmp->user_service_req).psc, psc_tmp);
								}
								if(strcmp(token[0], "Channel") == 0)
									(tmp->user_service_req).channel_num = atoi(token[1]);
								if(strcmp(token[0], "SourceMac") == 0)
								{
									char *mac_tmp[7];
									get_token(token[1], mac_tmp, (char *)":");
									for(int i=0; i<6; i++)
										(tmp->user_service_req).source_mac_addr[i] = atoi(mac_tmp[i]);
								}
								if(strcmp(token[0], "AID") == 0)
								{
									char *aid_tmp;
									aid_tmp = strtok(token[1], "\"");
									if(aid_tmp == NULL)
										aid_tmp = (char *)"";
									strcpy((tmp->user_service_req).aid, aid_tmp);
								}

								if(strcmp(token[0], "ImmediateAccess") == 0)
								{
									if(atoi(token[1]) == 1)
										(tmp->user_service_req).immediate_access = true;
									else
										(tmp->user_service_req).immediate_access = false;
								}
								if(strcmp(token[0], "IndefiniteAccess") == 0)
								{
									if(atoi(token[1]) == 1)
										(tmp->user_service_req).indefinite_access = true;
									else
										(tmp->user_service_req).indefinite_access = false;
								}
							}
							else if(tmp->wme_primitive_type == wsm_service_req)
							{
								if(strcmp(token[0], "PSID") == 0)
									(tmp->wsm_service_req).psid = atol(token[1]);
							}
							else if(tmp->wme_primitive_type == cch_service_req)
							{
								if(strcmp(token[0], "AppPriority") == 0)
									(tmp->cch_service_req).app_priority = atoi(token[1]);
							}

						}
					}
				}
			}
			else if(set_sib == 0)
			{
				if(strcmp(token[0], "SIB_Begin") == 0)
					set_sib = 1;
				else
					set_sib = -1;
			}
			else	//sib = -1
				break;
		}
		close(fd);
	}
	else
		printf("WME : Open Applocation config file error\n");

	return first;
}

void WME::send_wsm(struct WSM_Header wsm_header, char *wsm_data)
{
	ePacket_		*epkt;
	Packet			*pkt;
	struct ether_header     *eh;
	hdr_mac802_11       	*mac_hdr;
	char			*data;
	int			wsmp_len, wsm_index;

	epkt = createPacket();
	GET_PKT(pkt, epkt);
	eh = (struct ether_header *)pkt->pkt_malloc(sizeof(struct ether_header));
	memcpy(eh->ether_dhost, ETHER_BROADCAST, 6);
	memcpy(eh->ether_shost, my_mac_addr, 6);
	eh->ether_type = htons(0x88DC);

	/*
	 * Fill the mac header and push the packet to MAC module
	 */
	mac_hdr = (struct hdr_mac802_11 *)pkt->pkt_malloc(sizeof(struct hdr_mac802_11));
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_QoS_Data;
	mac_hdr->dh_fc.fc_type = MAC_Type_Data;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_more_frag  = 0;
	mac_hdr->dh_fc.fc_retry      = 0;
	mac_hdr->dh_fc.fc_pwr_mgt    = 0;
	mac_hdr->dh_fc.fc_more_data  = 0;
	mac_hdr->dh_fc.fc_wep        = 0;
	mac_hdr->dh_fc.fc_order      = 0;
	// AD_HOC support only need to add INFRASTRUCTURE
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;
	memcpy((char *)mac_hdr->dh_addr1,(char *)eh->ether_dhost,6);
	memcpy((char *)mac_hdr->dh_addr2,(char *)eh->ether_shost,6);

	wsmp_len = 13 + wsm_header.wsm_len;
	if(wsm_header.header_cont & 64)
		wsmp_len += 8;

	wsm_index = 0;
	data = pkt->pkt_sattach(wsmp_len);
	pkt->pkt_sprepend(data, wsmp_len);

	wsm_index = fill_data(data, &(wsm_header.wsm_version), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.header_len), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.header_cont), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.security_type), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.channel_num), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.data_rate), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.txpwr_level), 1, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.psid), 4, wsm_index, wsmp_len);
	if(wsm_header.header_cont & 64)
		wsm_index = fill_data(data, &(wsm_header.expiry_time), 8, wsm_index, wsmp_len);
	wsm_index = fill_data(data, &(wsm_header.wsm_len), 2, wsm_index, wsmp_len);
	wsm_index = fill_data(data, wsm_data , wsm_header.wsm_len, wsm_index, wsmp_len);

	//printf(">>>>>>nid (%d) recv WSM from agent and send it to mac\n", nid);
	put(epkt, sendtarget_);
}

int WME::send(ePacket_ *epkt)			// only for send ip packet
{
	struct ether_header     *eh;
	hdr_mac802_11       	*mac_hdr;
	Packet			*pkt;

	assert(epkt);
	assert(epkt->DataInfo_);

	if(device_status == none)		// only when device join a WBSS or provide a WBSS, it can send IP data
	{
		freePacket(epkt);
		return 1;
	}

	GET_PKT(pkt, epkt);

	eh = (struct ether_header *)pkt->pkt_get();

	if( !bcmp(eh->ether_dhost,ETHER_MULTICAST2,6) )
		return 1;

	mac_hdr = (struct hdr_mac802_11 *)pkt->pkt_malloc(sizeof(struct hdr_mac802_11));
	mac_hdr->dh_fc.fc_subtype = MAC_Subtype_QoS_Data;
	mac_hdr->dh_fc.fc_type = MAC_Type_Data;
	mac_hdr->dh_fc.fc_protocol_version = MAC_ProtocolVersion;
	mac_hdr->dh_fc.fc_more_frag  = 0;
	mac_hdr->dh_fc.fc_retry      = 0;
	mac_hdr->dh_fc.fc_pwr_mgt    = 0;
	mac_hdr->dh_fc.fc_more_data  = 0;
	mac_hdr->dh_fc.fc_wep        = 0;
	mac_hdr->dh_fc.fc_order      = 0;
	// AD_HOC support only need to add INFRASTRUCTURE
	mac_hdr->dh_fc.fc_from_ds = BIT_0;
	mac_hdr->dh_fc.fc_to_ds = BIT_0;
	memcpy((char *)mac_hdr->dh_addr1,(char *)eh->ether_dhost, 6);
	memcpy((char *)mac_hdr->dh_addr2,(char *)my_mac_addr, 6);
	memcpy((char *)mac_hdr->dh_addr3,(char *)eh->ether_shost, 6);

	//pkt->pkt_addinfo("PSID", (char *)&wme_mib.application_status_table[GlobalAppIndex].dot3AstProviderServiceIdentifier, 4);

	return(put(epkt, sendtarget_));
}

void WME::recv_wsm(char *data)
{
	extern cmd_server * cmd_server_;
	struct WSM_Header wsm_header;
	char *wsm_data;
	int index;
	// check the WSM service info

	// if ok recv WSM to command server
	index = 0;

	wsm_header.wsm_version = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.header_len = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.header_cont = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.security_type = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.channel_num = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.data_rate = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.txpwr_level = *((uint8_t *)(data+index));
	index += 1;
	wsm_header.psid = *((uint32_t *)(data+index));
	index += 4;

	/*
	 *	If device want  to recv WSM it need to reg the psid in WSM Service Info
	 */

	if(wsm_header.psid != 0)				//check WSM service table
	{
		int i;
		for(i=0; i<MaxServceInfoEntry; i++)
			if(mib.wsm_service_info[i].service_state == used && mib.wsm_service_info[i].psid == wsm_header.psid)
				break;

		if(i == MaxServceInfoEntry)			//didn't find the match psid for WSM
			return;
	}

	if(wsm_header.header_cont & 64)
	{
		wsm_header.expiry_time = *((uint64_t *)(data+index));
		index += 8;
	}
	wsm_header.wsm_len = *((uint16_t *)(data+index));
	index += 2;

	wsm_data = (data + index);

	cmd_server_->recvWSMtoAgent(nid, wsm_header, wsm_data);
}

int WME::recv(ePacket_ *epkt)
{
	Packet			*pkt;
	struct ether_header     *eh;

	assert(epkt&&epkt->DataInfo_);
	GET_PKT(pkt, epkt);
	eh=(struct ether_header *)pkt->pkt_get();

	if((ntohs(eh->ether_type) == 1))// WSA
	{
		recv_wsa(pkt);
		freePacket(epkt);
		//printf(">>>>>>nid (%d)recv WSA from mac %x:%x:%x:%x:%x:%x and length = %d\n", nid, eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2], eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5], p->pkt_getlen());
	}
	else if((ntohs(eh->ether_type) == 0x88DC))//ether type = WSMP
	{
		int nodeType = 0;
		// get node type to check if it's a large-scale node
		((Node *)nodelist[nid])->getNodeType(nodeType);
		if((nodeType == LARGE_SCALE_CAR) || (nodeType == TRAFFIC_LIGHT_CONTROLLER)) {
			/* It's a large-scale node
			 * send this packet to node module (large-scale)
			 */
			((Node *)nodelist[nid])->nodeRecv(epkt);
		}
		else {
			// normal agent client
			recv_wsm(pkt->pkt_sget());
		}

		freePacket(epkt);

		//printf(">>>>>>nid (%d) recv WSM from mac and send it to Agent\n", nid);

	}
	else if((ntohs(eh->ether_type) == 0x0800))	//ether type = IP
	{
		if(device_status == none )//not provider or user 
			freePacket(epkt);
		else if(device_status == user)
		{
			struct ServiceIndex *tmp_service_index;
			tmp_service_index = user_list.joined_service;

			while(tmp_service_index != NULL)
			{
				if(memcmp(eh->ether_shost, mib.available_service_info[tmp_service_index->index].souece_mac_addr, 6) == 0)	//user can recv ip data only from provider which joined
					return(put(epkt, recvtarget_));

				tmp_service_index = tmp_service_index->next;
			}
			/*if(memcmp(eh->ether_shost, mib.available_service_info[user_list.max_priority_index].souece_mac_addr, 6) == 0)	//user can recv ip data only from provider which joined
				return(put(epkt, recvtarget_));*/
			
			freePacket(epkt);
		}
		else			//provider
			return(put(epkt, recvtarget_));
	}
	else				// other packet type
		freePacket(epkt);
	return 1;
	/*
	   if((memcmp(wme_mib.provider_service_info[GlobalAppIndex].dot3PstMacAddress , eh->ether_shost, 6) != 0) && (GlobalAppType == user))
	// user will not recv non join Provider's packet
	{
	printf("<<<<<<<<<WME recv other provider's packet %x:%x:%x:%x:%x:%x\n", eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2], eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5]);
	freePacket(pkt);
	return 1;
	}*/

	//		u_int32_t *Provider_PSID = (u_int32_t *)p->pkt_getinfo("PSID");

	//		if((Provider_PSID == NULL) || (wme_mib.application_status_table[GlobalAppIndex].dot3AstProviderServiceIdentifier != *Provider_PSID))
	//		{
	/*
	   if(Provider_PSID == NULL)
	   printf(">>>>>[WME recv] PKT INFO PSID is NULL\n");
	   else
	   printf(">>>>>[WME recv] it is not my join WBSS's PSID(%x)\n", *Provider_PSID);
	   */
	//			freePacket(pkt);
	//			return 1;
	//		}

	//printf("<<<<<<<<<WME(%d) recv joined provider's packet %x:%x:%x:%x:%x:%x and length = %d\n", nid, eh->ether_shost[0], eh->ether_shost[1], eh->ether_shost[2], eh->ether_shost[3], eh->ether_shost[4], eh->ether_shost[5], p->pkt_getlen());
}

void WME::MLME_CHANNELINACTIVITY_indication(int channel_num)
{
}

void WME::to_cch()	// cch_to_cch or cch_to_sch
{
	int i, j;
	struct ServiceIndex   *tmp_service_index;
	
	/*	
	 *		user service list index clear
	 *		available service table maintain
	 *		cancle the invalid user service
	 */

	tmp_service_index = user_list.joined_service;
	while(tmp_service_index != NULL)
	{
		user_list.joined_service = (user_list.joined_service)->next;
		delete tmp_service_index;
		tmp_service_index = user_list.joined_service;
	}

	//user_list.max_priority = 0;
	//user_list.joined_count = 0;
	bzero(&user_list, sizeof(struct User_List));


	for(i=0; i<MaxServceInfoEntry; i++)
	{
		if(mib.available_service_info[i].service_state == used)
		{
			if(mib.available_service_info[i].cch_recvs == 0)	//didn't recv WSA in CCH clear it
				bzero(&mib.available_service_info[i], sizeof(struct AvailableServiceInfo));	
			else
			{
				mib.available_service_info[i].cch_recvs = 0;

				if(device_status == provider)
					continue;
				//find the max priority for interesting available service
				//if user intersting service is in this available service add it to user_list
				for(j=0; j<MaxServceInfoEntry; j++)
					if(mib.user_service_info[j].psid == mib.available_service_info[i].psid)
					{
						tmp_service_index = new (struct ServiceIndex);
						tmp_service_index->index = i;
						tmp_service_index->next = user_list.joined_service;
						user_list.joined_service = tmp_service_index;
						user_list.joined_count ++;

						if(mib.available_service_info[i].app_priority > user_list.max_priority)
						{
							user_list.max_priority = mib.available_service_info[i].app_priority;
							user_list.max_priority_index = i;
							user_list.max_priority_channel =  mib.available_service_info[i].channel_info.channel_num;
						}
						break;
					}

			}
		}
	}

	/*
	 *		device can join tow or more service
	 */

	if(device_status == provider)
		return;


	if(user_list.joined_service != NULL)
	{
		struct ServiceIndex	*tmp_service_index_1, *tmp_service_index_2;
		device_status = user;
		tmp_service_index_1 = tmp_service_index_2 = user_list.joined_service;
		while(tmp_service_index_2 != NULL)
		{
			//check the channel number
			if(mib.available_service_info[tmp_service_index_2->index].channel_info.channel_num != mib.available_service_info[user_list.max_priority_index].channel_info.channel_num)
			{
				if(tmp_service_index_2 == user_list.joined_service)
				{
					user_list.joined_service = tmp_service_index_1->next;
					delete tmp_service_index_2;
					tmp_service_index_1 = tmp_service_index_2 = user_list.joined_service;
				}
				else
				{
					tmp_service_index_1->next = tmp_service_index_2->next;
					delete tmp_service_index_2;
					tmp_service_index_2 = tmp_service_index_1->next;
				}
				user_list.joined_count --;
			}
			else
			{
				tmp_service_index_1 = tmp_service_index_2;
				tmp_service_index_2 = tmp_service_index_2->next;
			}
		}
		
		if(user_list.joined_count == 0)
			assert(0);
		((mac_80211p *)InstanceLookup(nid, stack_port, "MAC80211p"))->start_channel_switch(user_list.max_priority_channel, 0, 0);
	}
	else
	{
		((mac_80211p *)InstanceLookup(nid, stack_port, "MAC80211p"))->pause_channel_switch();
		device_status = none;
	}
}

void WME::modify_wsa(int index)
{
}

void WME::del_wsa(uint32_t psid)
{
	struct PST_Entry	*tmp_pst_entry_1, *tmp_pst_entry_2;
	//struct WSA_ChannelInfo	*tmp_ci_entry_1, *tmp_ci_entry_2;	only support one antenna

	tmp_pst_entry_1 = tmp_pst_entry_2 = wsa.pst.pst_first;

	while(tmp_pst_entry_2 != NULL)
	{
		if(tmp_pst_entry_2->psid == psid)
		{
			if(wsa.pst.pst_first == tmp_pst_entry_2)
				wsa.pst.pst_first = NULL;
			else
				tmp_pst_entry_1->next = tmp_pst_entry_2->next;

			wsa.pst.provider_count --;
			
			for(int i=0; i<NumOfSch; i++)
				if(tmp_pst_entry_2->channel_num == SCH[i])
				{
					sch_state.used[i] --;
					if(sch_state.used[i] == 0)
						sch_state.count --;
					break;
				}
			
			delete tmp_pst_entry_2;
			
			if(wsa.pst.provider_count == 0)
			{
				//delete WSA
				delete wsa.pst.ci_first;
				wsa.pst.ci_first = NULL;
				wsa.pst.channel_count = 0;
				wsa.wra_len = 0;
				//cancel CH change and back to CCH
				((mac_80211p *)InstanceLookup(nid, stack_port, "MAC80211p"))->pause_channel_switch();
				wsa_t.cancel();
				global_repeats = 0;
				device_status = none;
			}
			else
				generate_wsa();

			break;
		}
		else
		{
			tmp_pst_entry_1 = tmp_pst_entry_2;
			tmp_pst_entry_2 = tmp_pst_entry_2->next;
		}
	}
}
