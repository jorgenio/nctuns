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

#include "mac802_16_meshbs.h"

#include <list>
#include <nctuns_api.h>
#include <timer.h>
#include <math.h>
#include "frame_mngr.h"
#include "mesh_connection.h"
#include "net_entry_mngr.h"
#include "sch/ctrl/info.h"
#include "sch/ctrl/scheduler.h"
#include "sch/data/scheduler.h"
#include "util_map.h"
#include "../library.h"


#define VERBOSE_LEVEL   MSG_INFO
#include "../verbose.h"

MODULE_GENERATOR(mac802_16_MeshBS);

mac802_16_MeshBS::mac802_16_MeshBS(
		uint32_t type, uint32_t id, plist* pl, const char* name)
: mac802_16_MeshSS(type, id, pl, name)
, _NodeCfgFile(NULL)
, _nf_tx_opp(0)
, _nf_dsch_tx_opp(0)
, _csch_data_fraction(0)
, _nf_sch_frame_idx(0)
{
	vBind("NodeConfig", &_NodeCfgFile);
	/*
	 * TODO: Following parameters should be read from tcl file.
	 */
	_nf_tx_opp = DEFAULT_TXOPPS_NUM;
	_nf_dsch_tx_opp = 8;
	_csch_data_fraction = 0;
	_nf_sch_frame_idx = 2;
}

mac802_16_MeshBS::~mac802_16_MeshBS()
{
	for ( ; !nodeList.empty(); nodeList.pop_front())
		delete nodeList.front();
}

int
mac802_16_MeshBS::init()
{
	assert(_NodeCfgFile);
	assert(_nf_tx_opp > 0);
	assert(_nf_dsch_tx_opp > 0);
	assert(_csch_data_fraction >= 0);
	assert(_nf_sch_frame_idx > 0);

	mac802_16_MeshSS::init();

	set_node_id(get_nid());
	net_entry_mngr()->set_sync_hop_count(0);

	fr_mngr()->set_nf_tx_opp(_nf_tx_opp);
	fr_mngr()->set_nf_dsch_tx_opp(_nf_dsch_tx_opp);
	fr_mngr()->set_csch_data_fraction(_csch_data_fraction);
	fr_mngr()->set_nf_sch_frame_idx(_nf_sch_frame_idx);

	fr_mngr()->set_cur_frame(0);

	CtrlConnection->SetSrcNodeID(node_id());

	int dataSymbols  = symbolsPerFrame() - fr_mngr()->nf_ctrl_symbol();
	fr_mngr()->set_slot_size((int) ceil(dataSymbols / 256.0));
	fr_mngr()->set_nf_slot(dataSymbols / fr_mngr()->slot_size());

	NSLOBJ_DEBUG("dataSymbols = %d, slot_size = %d, nf_slot = %d\n",
			dataSymbols, fr_mngr()->slot_size(), fr_mngr()->nf_slot());

	readConfig();

	update_node_status(TxOppUtilizationCounter::msh_ncfg, get_nid(), node_stat_blk_t::functional);
	update_node_status(TxOppUtilizationCounter::msh_dsch, get_nid(), node_stat_blk_t::functional);

	set_operational();

	uint64_t timeInTick;
	MILLI_TO_TICK(timeInTick, frame_duration());
	timerFrame->start(0, timeInTick);

        ncfg_scheduler()->start(fr_mngr()->ncfg_opp_seq_end() + 1, fr_mngr()->max_ncfg_tx_opp());
        dsch_scheduler()->start(fr_mngr()->dsch_opp_seq_end() + 1, fr_mngr()->max_dsch_tx_opp());
	data_scheduler()->start();

	return 1;
}

int
mac802_16_MeshBS::recv(Event* ep)
{
	Packet*			recvBurst;
	struct hdr_generic*	hg;
	struct mgmt_msg*	mm;

	unsigned int	plen;
	int		cid;
	int		len;
	int		BurstLen;
	char*		AdaptedBurst;
	char*		ptr;
	uint16_t	tx_node_id;

	unsigned int crc;
	std::vector<DataConnection*>::iterator iter;

	assert(ep && (recvBurst = (Packet*)ep->DataInfo_));

	fr_mngr()->update_last_signal_info(
			(PHYInfo*)recvBurst->pkt_getinfo("phyInfo"));

	AdaptedBurst = (char*)recvBurst->pkt_sget();
	BurstLen = recvBurst->pkt_getlen();

	NSLOBJ_DEBUG("burstLen=%d\n", BurstLen);

	for (ptr = AdaptedBurst;
	     ptr + sizeof(struct hdr_generic) < AdaptedBurst + BurstLen;
	     ptr += plen) {
		hg = (struct hdr_generic *) ptr;
		GHDR_GET_CID(hg, cid);
		GHDR_GET_LEN(hg, plen);
		if (plen < sizeof(struct hdr_generic) ||
		    hcs(ptr, sizeof(struct hdr_generic)) || plen == 0) {
			//printf("Header Broken: HCS is not match\n");
			break;
		}

		len = plen;

		NSLOBJ_DEBUG("Extract PDU: CID=%d LEN=%d\n", cid, plen);

		if (hg->ci) {
			len -= 4;
			crc = crc32(0, ptr, len);
			if (memcmp(&crc, ptr + len, 4) != 0) {
				printf("CRC Error (%08x)\n", crc);
				continue;
			}
		}
		if ((hg->type & 0x20) == 0)	// if not Mesh type
		{
			continue;
		}

		memcpy(&tx_node_id, ptr + sizeof(struct hdr_generic),
		       sizeof(u_int16_t));
		class Mesh_connection connection(cid);

		NSLOBJ_DEBUG("CID = %d  =>  LinkID=%d NetID=%d\n", cid,
				connection.link_id(), connection.logic_network_id());

		if (hg->ht == 1) {
			continue;
		}

		if (connection.link_id() == 0xfe) {
			/*
			 * Sponsor Channel
			 */
			mm = (struct mgmt_msg*)(ptr + sizeof(struct hdr_generic) + 2);
			if (!procBaseStation(mm, cid, len - sizeof(struct hdr_generic) - 2, 0))
				procSubscriberStation(hg);
		} else if (connection.link_id() ==
				Mesh_connection::ALL_NET_BROADCAST)
			procSubscriberStation(hg);

		else if (connection.type() ==
				Mesh_connection::TYPE_MAC_MANAGEMENT) {
			in_addr_t srcip;
			mm = (mgmt_msg*)net_entry_mngr()->extract(
					ptr + sizeof(hdr_generic) + 2, len, srcip, inet_address());
			if (mm)
				procBaseStation(mm, cid, len, srcip);
			else
				procSubscriberStation(hg);
		} else
			procSubscriberStation(hg);
	}

	freePacket(ep);
	return 0;
}

void
mac802_16_MeshBS::readConfig()
{
	char fn[256];
	FILE *fp;
	int node, imac[6];
	char buffer[1024], mac[6];

	memset(fn, 0, 256);
	char* config_file_dir = GetConfigFileDir();
	snprintf(fn, sizeof(fn), "%s%s", config_file_dir, _NodeCfgFile);
	//free(config_file_dir);


	if ((fp = fopen(fn, "r"))) {

		while (!feof(fp) && fgets(buffer, sizeof(buffer), fp)) {

			sscanf(buffer, "%d %02x:%02x:%02x:%02x:%02x:%02x\n", &node,
					imac + 0, imac + 1, imac + 2, imac + 3,
					imac + 4, imac + 5);

			mac[0] = imac[0];
			mac[1] = imac[1];
			mac[2] = imac[2];
			mac[3] = imac[3];
			mac[4] = imac[4];
			mac[5] = imac[5];

			nodeList.push_back(new MSHNode((u_char *) mac, node));
		}
		fclose(fp);
	}
	else {

		NSLOBJ_FATAL("opening configuration file failed.\n");
	}
}

int
mac802_16_MeshBS::procBaseStation(
		mgmt_msg* recvmm, int cid, int len, in_addr_t srcip)
{
	NSLOBJ_DEBUG("recv->type=%d\n", recvmm->type);

	ifmgmt *ifmm = NULL;

	if (recvmm->type == MG_REGREQ) {
		NSLOBJ_INFO("REGREQ len = %d\n", len);
		ifmm = procREGREQ(recvmm, cid, len);
	}
	if (ifmm) {
		if (srcip) {
			Event *ep;
			Packet *UpPkt;
			UpPkt = net_entry_mngr()->tunnel(*ifmm, srcip, inet_address());
			// Push to data queue or pass to upper routine module
			UpPkt->pkt_setflow(PF_RECV);
			ep = createEvent();
			ep->DataInfo_ = UpPkt;
			put(ep, recvtarget_);
		} else {
			SponsorConnection->Insert(ifmm);
		}
		return 1;
	}
	return 0;
}

ifmgmt*
mac802_16_MeshBS::procREGREQ(struct mgmt_msg * recvmm, int cid, int len)
{
	ifmgmt *ifmm;
	int t = 0;

	u_char fhmac[21], fmanage, fipm, farq, fcrc;
	u_char vmac[6];
	u_int16_t fncid, Nid;

	ifmm = new ifmgmt((u_char *) recvmm, len, 0);

	while ((t = ifmm->getNextType()) != 0) {
		NSLOBJ_DEBUG("Type = %d\t", t);
		switch (t) {
		case 2:
			ifmm->getNextValue(&fmanage);
			NSLOBJ_DEBUG("manage = %d\n", fmanage);
			break;
		case 3:
			ifmm->getNextValue(&fipm);
			NSLOBJ_DEBUG("IP mode = %d\n", fipm);
			break;
		case 6:
			ifmm->getNextValue(&fncid);
			NSLOBJ_DEBUG("# of UCID = %d\n", fncid);
			break;
		case 10:
			ifmm->getNextValue(&farq);
			NSLOBJ_DEBUG("ARQ Support = %d\n", farq);
			break;
		case 12:
			ifmm->getNextValue(&fcrc);
			NSLOBJ_DEBUG("CRC Support = %d\n", fcrc);
			break;
		case 18:
			ifmm->getNextValue(vmac);
			NSLOBJ_DEBUG("MAC = %02x:%02x:%02x:%02x:%02x:%02x\n",
			       vmac[0], vmac[1], vmac[2], vmac[3], vmac[4],
			       vmac[5]);
			break;
		case 149:
			ifmm->getNextValue(fhmac);
			NSLOBJ_DEBUG("HMAC = ...\n");
			break;

		default:
			NSLOBJ_DEBUG("Non-implement Type (%d)\n", t);
			break;
		}
	}
	delete ifmm;

	NSLOBJ_INFO("Mesh BS Node dump nodelist (listsize =%u). \n",
	       nodeList.size());

	
	for (std::list<MSHNode*>::iterator it = nodeList.begin();
			it != nodeList.end(); it++) {
		DEBUG_FUNC((*it)->dump());

		if ((*it)->Equal(vmac)) {
			(*it)->setEnable(true);
			Nid = (*it)->getNodeID();
			NSLOBJ_INFO("Find Node ID = %d\n", Nid);

			ifmm = new ifmgmt(MG_REGRSP, 0);	// Sec 11.8

			ifmm->appendTLV(2, 1, 0U);
			ifmm->appendTLV(3, 1, 0U);
			ifmm->appendTLV(6, 2, fncid);
#if 0
			ifmm->appendTLV(10, 2, AttrARQ);
			ifmm->appendTLV(12, 2, AttrCRC);
#endif
			ifmm->appendTLV(19, 2, Nid);
			ifmm->appendTLV(149, 21, fhmac);
			return ifmm;
		}
	}
	assert(0);
	return NULL;
}
