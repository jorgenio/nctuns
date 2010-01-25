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

#include "msobject_NT.h"

using namespace std;
using namespace MR_MacAddress_NT;
using namespace MR_ServiceFlow_NT;
using namespace MR_Timer_NT;
using namespace MR_MSbase_NT;

//msObject_NT::msObject_NT(uint8_t *pMac, int pBasicCID, int pPriCID, int pSecCID, mac802_16j_NT_PMPBS *pBS)
msObject_NT::msObject_NT(uint8_t *pMac, int pBasicCID, int pPriCID, int pSecCID, mac802_16j_NT_PMPBS *pBS, mac802_16j_NT_PMPMS *pMS)
{

	logRidvan(TRACE,"-->%d	msObject_NT::msObject_NT ",get_nid());
	address()->copy_from(pMac, 6); // MAC address
	BasicCID    = pBasicCID;
	PriCID      = pPriCID;
	SecCID      = pSecCID;
	servingBS   = pBS;
    accessStation = pBS;
    myMS        = pMS;
	targetBS    = NULL;

	ServerLevelPredict    = 3;   // No service level prediction available
	PeriodRangingInterval = 200; // unit in frames
	ResourceRetainTime    = 20;  // uint in 100 ms
	ResourceStatus        = Serving;
	ScanDuration          = 0;
	ScanTimes             = 0;
	InterleaveInt         = 0;
	ScanIteration         = 0;
	ScanFlag              = false;
	ChangeToScanCnt       = -2;
	DSApending            = broadcastCID;  // broadcast CID: No pending DSA-RSP Pending

	MnConnections[0] = new ManagementConnection(pBasicCID);
	MnConnections[1] = new ManagementConnection(pPriCID);
	MnConnections[2] = pSecCID ? new ManagementConnection(pSecCID) : NULL;
	timerResourceRetain = new timerObj;

	assert(MnConnections[0] && MnConnections[1] && timerResourceRetain);

	BASE_OBJTYPE(mem_func);
	mem_func = POINTER_TO_MEMBER(msObject_NT, T7);
	timer_mngr()->set_func_t(7u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(msObject_NT, T10);
	timer_mngr()->set_func_t(10u, this, mem_func);

	mem_func = POINTER_TO_MEMBER(msObject_NT, ResourceClear);
	timerResourceRetain->setCallOutObj(this, mem_func);
}

msObject_NT::~msObject_NT()
{

	logRidvan(TRACE,"-->%d	msObject_NT::~msObject_NT ",get_nid());
	delete MnConnections[0];
	delete MnConnections[1];

	if (MnConnections[2] != NULL)
		delete MnConnections[2];

	if (targetBS != NULL)
		delete targetBS;

	delete timerResourceRetain;
}

int msObject_NT::handle(mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::handle ",get_nid());
	Connection *conn = NULL;

	if (cid == BasicCID)   // Basic CID
	{
		pCurrentConnect = MnConnections[0];

		if (recvmm->type == MG_SBCREQ)  // SBC-REQ
		{
			procSBCREQ(recvmm, cid, len);
		}
		if (recvmm->type == MG_MOB_MSHOREQ) // MOB_MSHO-REQ
		{
			procMOB_MSHOREQ(recvmm, cid, len);
		}
		if (recvmm->type == MG_MOB_HOIND) // MOB_HO-IND
		{
			procMOB_HOIND(recvmm, cid, len);
		}
		if (recvmm->type == MG_MOB_SCNREQ) // MOB_SCN-REQ
		{
			procMOB_SCNREQ(recvmm, cid, len);
		}
	}
	else if (cid == PriCID)   // Primary CID
	{
		pCurrentConnect = MnConnections[1];
		if (recvmm->type == MG_REGREQ)  // REG-REQ
		{
			procREGREQ(recvmm, cid, len);

			if (DSApending == broadcastCID)
			{
				Sflow = servingBS->GetProvisionedFlow(address()->buf());
				if (Sflow != NULL)
				{
					conn = servingBS->CreateDataConnection(this);
                    //printf("msObject_NT dataconn = %d\n",conn->getCID());
					DSApending = conn->getCID();
					SendDSAREQ(conn->getCID());
					Sflow->SetConnectionID(conn->getCID());
					conn->SetFID(Sflow->GetSfindex());
				}

                if(accessStation != servingBS)
                {
                    mac802_16j_NT_PMPRS *pRS = (mac802_16j_NT_PMPRS *)(accessStation);
                    pRS->DLRelayDtConnections.push_back(new DataConnection(conn->getCID()));
                    pRS->ULRelayDtConnections.push_back(new DataConnection(conn->getCID()));
                    pRS->ULRelayReceiverList.push_back(new ConnectionReceiver(conn->getCID()));
                    pRS->DLRelayReceiverList.push_back(new ConnectionReceiver(conn->getCID()));
                    logRidvan(TRACE,"msobject data connecton %d created ms %s",conn->getCID(), this->address()->str());
                }
			}
		}
		if (recvmm->type == MG_DSARSP)   // DSA-RSP
		{
			DSApending = broadcastCID;
			timer_mngr()->cancel_t(7);
			procDSARSP(recvmm, cid, len);
		}
		if (recvmm->type == MG_MOB_SCNREP)  // MOB_SCN-REP
		{
			procMOB_SCNREP(recvmm, cid, len);
		}
	}
	else if (cid == SecCID)   // Secondary CID
	{
		;
	}
	return 0;
}

/*
 * Wait for DSA/DSC/DSD Response timeout
 */
void msObject_NT::T7()
{

	logRidvan(TRACE,"-->%d	msObject_NT::T7 ",get_nid());
	printf("msObject_NT::%s()\n", __func__);

	SendDSAREQ(DSApending);
}

/*
 * Wait for Transaction End timeout
 */
void msObject_NT::T10()
{

	logRidvan(TRACE,"-->%d	msObject_NT::T10 ",get_nid());
	;//printf("\nmsObject_NT::%s()\n", __func__);
}

/*
 * Resource retain timeout or MS attached to Target BS, serving BS shall clear MS information
 */
void msObject_NT::ResourceClear()
{

	logRidvan(TRACE,"-->%d	msObject_NT::ResourceClear ",get_nid());
	//printf("%d: msObject_NT::%s()\n", get_nid(), __func__);
	ResourceStatus = NeedClear;
	timerResourceRetain->cancel();
}

/*
 * Spec 16e. Figure 67 --Negotiate Basic Capabilities--BS
 */
void msObject_NT::procSBCREQ(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procSBCREQ ",get_nid());
	ifmgmt *ifmm_req        = NULL;
	ifmgmt *ifmm_rsp        = NULL;
	uint8_t type               = 0;
	uint8_t fbas            = 0;
	uint8_t fmaxpower[4]    = "";
	uint8_t fctpower        = 0;
	uint8_t ffftsize        = 0;
	uint16_t fdemod         = 0;
	uint8_t fmod            = 0;
	uint8_t fpersup         = 0;
	uint16_t fstg           = 0;

	//printf("\nTime:%llu:msObject_NT::%s()\n", GetCurrentTime(), __func__);

	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, 0);

	// Spec. 11.8
	while ((type = ifmm_req->getNextType()) != 0)
	{
		switch (type) {
			case 1:
				ifmm_req->getNextValue(&fbas);
				//            printf("bandwidth allocation = %d\n", fbas);
				break;

			case 2:
				ifmm_req->getNextValue(&fstg);
				//            printf("TTG, RTG = 0x%x\n", fstg);
				break;

			case 3:
				ifmm_req->getNextValue(fmaxpower);
				//           printf("maxpower = %d %d %d %d\n", fmaxpower[0], fmaxpower[1], fmaxpower[2], fmaxpower[3]);
				break;

			case 147:
				ifmm_req->getNextValue(&fctpower);
				//           printf("current power = %d\n", fctpower);
				break;

			case 150:
				ifmm_req->getNextValue(&ffftsize);
				//           printf("fft size = %d\n", ffftsize);
				break;

			case 151:
				ifmm_req->getNextValue(&fdemod);
				//            printf("demodulator = %d\n", fdemod);
				break;

			case 152:
				ifmm_req->getNextValue(&fmod);
				//            printf("modulator = %d\n", fmod);
				break;

			case 154:
				ifmm_req->getNextValue(&fpersup);
				//            printf("permutation support = %d\n", fpersup);
				break;

			default:
				break;
		}
	}

	delete ifmm_req;

	//printf("\nTime:%llu:msObject_NT::SendSBCRSP() cid=%d\n", GetCurrentTime(),cid);
	// SBC-RSP
	ifmm_rsp = new ifmgmt(MG_SBCRSP, 0);
	ifmm_rsp->appendTLV(1, 1, fbas);
	ifmm_rsp->appendTLV(2, 2, fstg);
	ifmm_rsp->appendTLV(3, 4, fmaxpower);
	ifmm_rsp->appendTLV(147, 1, fctpower);
	ifmm_rsp->appendTLV(150, 1, ffftsize);
	ifmm_rsp->appendTLV(151, 2, fdemod);
	ifmm_rsp->appendTLV(152, 1, fmod);
	ifmm_rsp->appendTLV(154, 1, fpersup);

	pCurrentConnect->Insert(ifmm_rsp);
}

/*
 *    Section 6.3.2.3.10
 */
void msObject_NT::SendDSAREQ(uint16_t cid)
{

	logRidvan(TRACE,"-->%d	msObject_NT::SendDSAREQ ",get_nid());
	ifmgmt *ifmm        = NULL;
	ifTLV *tmptlv       = NULL;
	uint8_t fhmac[21]   = "";

	//printf("\nTime:%llu:msObject_NT::%s()\n", GetCurrentTime(), __func__);

	ifmm = new ifmgmt(MG_DSAREQ, 2);
	ifmm->appendField(2, 1234);    // Transaction ID

	tmptlv = new ifTLV();
	tmptlv->Add(2, 2, cid);    // CID 11.13.2
	// CS specification, Spec 11.13.19.1
	// 1: Packet, IPv4
	// 2: Packet, IPv6
	// 3: Packet, 802.3/Ethernet
	// 4: Packet, 802.1Q VLAN
	// 5: Packet, IPv4 over 802.3/Ethernet
	tmptlv->Add(28, 1, servingBS->getCSType());

	ifmm->appendTLV(145, tmptlv->getLen(), (void *) tmptlv->getBuffer());    // 145:Uplink service flow
	ifmm->appendTLV(149, 21, fhmac);    // hmac
	logRidvan(WARN,"msobject send DSAREQ nid:%d cid:%d mac:%s",get_nid(),cid,this->address()->str());
	pCurrentConnect->Insert(ifmm);

	resetTimerT(7u);
	delete tmptlv;
}

/*
 * Figure 70
 */
void msObject_NT::procREGREQ(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procREGREQ ",get_nid());
	ifmgmt *ifmm_req    = NULL;
	ifmgmt *ifmm_rsp    = NULL;
	int type            = 0;
	uint8_t fmanage     = 0;
	uint8_t fipm        = 0;
	uint8_t farq        = 0;
	uint8_t fmobile     = 0;
	uint16_t fulncid    = 0;
	uint16_t fdlncid    = 0;
	uint16_t fclass     = 0;
	uint8_t HORetransmitInterval = 10; // FIXME

	//printf("\nTime:%llu:msObject_NT::%s()\n", GetCurrentTime(), __func__);

	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, 0);

	while ((type = ifmm_req->getNextType()) != 0)
	{
		//printf("Type = %d\t", type);
		switch (type) {
			case 2:
				ifmm_req->getNextValue(&fmanage);
				//printf("manage = %d\n", fmanage);
				break;

			case 3:
				ifmm_req->getNextValue(&fipm);
				//printf("IP mode = %d\n", fipm);
				break;

			case 6:
				ifmm_req->getNextValue(&fulncid);
				//printf("Number of Transport UL CIDs Support = %d\n", fulncid);
				break;

			case 7:
				ifmm_req->getNextValue(&fclass);
				//printf("Classification Support = 0x%x\n", fclass);
				break;

			case 10:
				ifmm_req->getNextValue(&farq);
				//printf("ARQ Support = %d\n", farq);
				break;

			case 15:
				ifmm_req->getNextValue(&fdlncid);
				//printf("Number of Transport DL CIDs Support = %d\n", fdlncid);
				break;

			case 31:
				ifmm_req->getNextValue(&fmobile);
				//printf("Mobility features Support = %d\n", fmobile);
				break;
		}
	}
	delete ifmm_req;

	ifmm_rsp = new ifmgmt(MG_REGRSP, 0);    // Sec 11.8

	ifmm_rsp->appendTLV( 2, 1, fmanage);
	ifmm_rsp->appendTLV( 3, 1, fipm);
	ifmm_rsp->appendTLV( 6, 2, fulncid);
	ifmm_rsp->appendTLV( 7, 2, fclass);
	ifmm_rsp->appendTLV(10, 1, farq);
	ifmm_rsp->appendTLV(15, 2, fdlncid);
	ifmm_rsp->appendTLV(28, 2, ResourceRetainTime); // unit in 100 ms
	ifmm_rsp->appendTLV(30, 1, HORetransmitInterval); // unit in frames
	ifmm_rsp->appendTLV(31, 1, fmobile);

	pCurrentConnect->Insert(ifmm_rsp);
}

void msObject_NT::procDSARSP(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procDSARSP ",get_nid());
	ifmgmt *ifmm_rsp    = NULL;
	ifmgmt *ifmm_ack    = NULL;
	int type            = 0;
	uint16_t ftransid   = 0;
	uint8_t fcc         = 0;
	uint8_t fhmac[21]   = "";

	//printf("\nTime:%llu:msObject_NT::%s()\n", GetCurrentTime(), __func__);

	ifmm_rsp = new ifmgmt((uint8_t *) recvmm, len, 3);
	ifmm_rsp->extractBitField(16, (uint8_t *)&ftransid);
	ifmm_rsp->extractBitField( 8, &fcc);

	while ((type = ifmm_rsp->getNextType()) != 0)
	{
		//printf("Type = %d\t", type);
		switch (type) {
			case 149:
				ifmm_rsp->getNextValue(fhmac);
				//printf("HMAC = ...\n");
				break;

			default:
				break;
		}
	}
	delete ifmm_rsp;

	ifmm_ack = new ifmgmt(MG_DSAACK, 3);    // Spec 16e. 11.13
	ifmm_ack->appendField(2, ftransid);     // Transcation ID
	ifmm_ack->appendField(1, 0);            // Confirmation Code: OK, Table 384
	ifmm_ack->appendTLV(149, 21, fhmac);    // HMAC

	pCurrentConnect->Insert(ifmm_ack);
	resetTimerT(10u);
}

void msObject_NT::procMOB_MSHOREQ(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procMOB_MSHOREQ ",get_nid());
	ifmgmt *ifmm_req            = NULL;
	ifmgmt *ifmm_rsp            = NULL;
	int ifmmLen                 = 0;
	uint8_t CfgChangeCnt        = 0;
	uint8_t index               = 0;
	uint8_t N_BS_Index          = 0;
	uint8_t N_BS_Full           = 0;
	uint8_t N_curr_BSs          = 0;
	uint8_t ServiceLevelPredict = 0;
	uint8_t ArrivalTimeDiff     = 0;
	uint8_t padding             = 0;
	uint8_t rsv                 = 0;
	int tmpBits                 = 0;
	uint8_t ArrTimeInd          = 0;
	uint8_t tempBSID            = 0;
	uint8_t BS_CINR             = 0;
	uint8_t BS_RSSI             = 0;
	uint8_t RelativeDelay       = 0;
	uint8_t BS_RTD              = 0;
	NeighborMRBSs_NT *tmpList        = new NeighborMRBSs_NT();
	NbrMRBS_NT *tmpNbrMRBS_NT             = NULL;
	uint8_t neighborBS_ID[6]    = "";

	//printf("\n\e[1;33mTime:%llu BS %s()\e[0m\n", GetCurrentTime(), __func__);

	/*
	 * Extract MOB_MSHO-REQ Message
	 */
	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, len - 1); // Here, the (len - 1) is the maximum length of mgmt_msg, not field size
	ifmm_req->extractBitField( 8, &tmpList->ReportMetric);
	ifmm_req->extractBitField( 8, &N_BS_Index);
	tmpBits += 16;
	//printf("=======================\n");
	//printf("== N_BS_Index: %d\n", N_BS_Index);

	if (N_BS_Index != 0)
	{
		ifmm_req->extractBitField( 8, &CfgChangeCnt);
		tmpBits += 8;
	}

	for (int i = 0;i < N_BS_Index;i++)
	{
		ifmm_req->extractBitField( 8, &index);

		tmpNbrMRBS_NT = new NbrMRBS_NT(-1, servingBS->NeighborMRBSList->nbrBSs_Index[index]->addr->buf());
		tmpNbrMRBS_NT->Index = index;

		ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->PreambleIndex);
		//printf("=====[%d] Channel ID: %d\n", i, tmpNbrMRBS_NT->PreambleIndex);

		tmpBits += 16;

		if ((tmpList->ReportMetric & 0x01) == 1)
		{
			ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->CINR);
			//printf("=====[%d] CINR: %d\n", i, tmpNbrMRBS_NT->CINR);
			tmpBits += 8;
		}

		if (((tmpList->ReportMetric & 0x02) >> 1) == 1)
		{
			ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->RSSI);
			tmpBits += 8;
		}

		if (((tmpList->ReportMetric & 0x04) >> 2) == 1)
		{
			ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->RelativeDelay);
			tmpBits += 8;
		}

		ServiceLevelPredict = 0;
		ArrTimeInd = 0;
		ifmm_req->extractBitField( 3, &ServiceLevelPredict);
		ifmm_req->extractBitField( 1, &ArrTimeInd);

		tmpBits += 4;

		if (ArrTimeInd == 1)
		{
			ArrivalTimeDiff = 0;
			ifmm_req->extractBitField( 4, &ArrivalTimeDiff);
			tmpBits += 4;
		}

		tmpList->nbrBSs_Index.push_back(tmpNbrMRBS_NT);
	}

	ifmm_req->extractBitField(8, &N_BS_Full);
	tmpBits += 8;
	//printf("== N_BS_Full: %d\n", N_BS_Full);

	for (int i = 0;i < N_BS_Full;i++)
	{
		ifmm_req->extractBitField(48, neighborBS_ID);

		tmpNbrMRBS_NT = new NbrMRBS_NT(-1, neighborBS_ID);

		ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->PreambleIndex);

		tmpBits += 56;

		if ((tmpList->ReportMetric & 0x01) == 1)
		{
			ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->CINR);
			tmpBits += 8;
		}

		if (((tmpList->ReportMetric & 0x02) >> 1) == 1)
		{
			ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->RSSI);
			tmpBits += 8;
		}

		if (((tmpList->ReportMetric & 0x04) >> 2) == 1)
		{
			ifmm_req->extractBitField( 8, &tmpNbrMRBS_NT->RelativeDelay);
			tmpBits += 8;
		}

		ServiceLevelPredict = 0;
		ArrTimeInd = 0;
		ifmm_req->extractBitField( 3, &ServiceLevelPredict);
		ifmm_req->extractBitField( 1, &ArrTimeInd);
		tmpBits += 8;

		if (ArrTimeInd == 1)
		{
			ArrivalTimeDiff = 0;
			ifmm_req->extractBitField( 4, &ArrivalTimeDiff);
			tmpBits += 4;
		}

		tmpList->nbrBSs_Full.push_back(tmpNbrMRBS_NT);
	}

	ifmm_req->extractBitField( 3, &N_curr_BSs);
	ifmm_req->extractBitField( 1, &padding);
	//printf("== padding: %d\n", padding);
	//printf("== N_curr_BSs: %d\n", N_curr_BSs);
	tmpBits += 4;

	// FIXME
	for (int i = 0; i < N_curr_BSs;i++)
	{
		ifmm_req->extractBitField( 4, &tempBSID);
		tmpBits += 4;

		if ((tmpList->ReportMetric & 0x01) == 1)
		{
			ifmm_req->extractBitField( 8, &BS_CINR);
			tmpBits += 8;
			//printf("=====[%d] BS_CINR: %d\n", i, BS_CINR);
		}

		if (((tmpList->ReportMetric & 0x02) >> 1) == 1)
		{
			ifmm_req->extractBitField( 8, &BS_RSSI);
			tmpBits += 8;
		}

		if (((tmpList->ReportMetric & 0x04) >> 2) == 1)
		{
			ifmm_req->extractBitField( 8, &RelativeDelay);
			tmpBits += 8;
		}

		if (((tmpList->ReportMetric & 0x08) >> 3) == 1)
		{
			ifmm_req->extractBitField( 8, &BS_RTD);
			tmpBits += 8;
		}
	}

	if (tmpBits % 8 != 0)
	{
		ifmm_req->extractBitField(8 - tmpBits % 8, &padding);
		ifmmLen = tmpBits / 8 + 1;
	}
	else
	{
		ifmmLen = tmpBits / 8;
	}

	delete ifmm_req;
	//printf("=======================\n");

	/*
	 * TLV encoding Information
	 */
	if ((tmpBits / 8) != (len - 1))
	{
		int type = 0;

		ifmm_req = new ifmgmt((uint8_t *) recvmm, len, ifmmLen);

		while ((type = ifmm_req->getNextType()) != 0)
		{
			switch (type) {
				case 149:
					uint8_t hmac[21] = "";
					ifmm_req->getNextValue(hmac);
					//printf("HMAC/CMAC Tuple ...\n");
					break;
			}
		}
		delete ifmm_req;
	}

	/**************************************************************
	 * FIXME
	 * When BS received MSHO-REQ, BS shall choose some recommended
	 * BSs that MS can perform handover to one of them or reject
	 * this HO request.
	 **************************************************************/

	/*
	 * Compute all field Bits
	 */
	N_BS_Index  = 0;
	N_BS_Full   = 0;
	N_curr_BSs  = 0;
	padding     = 0;
	tmpBits     = 0;

	uint8_t Mode                = 0; // 0b000: HO request
	uint8_t HO_operation_mode   = 1; // 0:Recommended  1:Mandatory
	uint8_t N_Recommended       = 1;
	uint8_t ResourceRetainFlag  = 0; // 0:Release      1:Retain
	uint8_t HO_ID_Indicator     = 0;
	uint8_t HO_auth_Indicator   = 0;
	uint8_t ActionTime          = 0;

	tmpBits += 8;

	if (Mode == 0x00)
	{
		tmpBits += 17;

		for (int i = 0;i < N_Recommended;i++)
		{
			tmpBits += 74;

			if (HO_ID_Indicator == 1)
				tmpBits += 8;

			tmpBits += 5;

			if (HO_auth_Indicator == 1)
				tmpBits += 8;
		}
	}
	else if (Mode == 0x01)
		;
	else if (Mode == 0x02)
		;
	else if (Mode == 0x03)
		;
	else if (Mode == 0x04)
		;
	else if (Mode == 0x05)
		;
	else if (Mode == 0x06)
		;
	else if (Mode == 0x07)
		;
	else
		;

	tmpBits += 8;

	if (tmpBits % 8 != 0)
		ifmmLen = tmpBits / 8 + 1;
	else
		ifmmLen = tmpBits / 8;

	/*
	 * Generate MOB_BSHO-RSP Message
	 */
	uint8_t Network_Assisted_HO_Support = 1;
	uint8_t HO_ID                       = 0;
	uint8_t HO_auth_support             = 0;

	ServiceLevelPredict = 3;

	ifmm_rsp = new ifmgmt(MG_MOB_BSHORSP, ifmmLen);

	ifmm_rsp->appendBitField( 3, Mode);
	ifmm_rsp->appendBitField( 5, rsv);

	if (Mode == 0x00)
	{
		ifmm_rsp->appendBitField( 1, HO_operation_mode);
		ifmm_rsp->appendBitField( 8, N_Recommended);
		ifmm_rsp->appendBitField( 1, ResourceRetainFlag);
		ifmm_rsp->appendBitField( 7, rsv);

		if (CfgChangeCnt == servingBS->NeighborMRBSList->NBRADV_CfgCount)
		{
			for (uint32_t i = 0;i < tmpList->nbrBSs_Index.size();i++)
			{
				ifmm_rsp->appendBitField(48, tmpList->nbrBSs_Index[i]->addr->buf());
				ifmm_rsp->appendBitField( 8, tmpList->nbrBSs_Index[i]->PreambleIndex);
				ifmm_rsp->appendBitField( 8, ServiceLevelPredict);
				ifmm_rsp->appendBitField( 8, tmpList->nbrBSs_Index[i]->HO_Optimize);
				ifmm_rsp->appendBitField( 1, Network_Assisted_HO_Support);
				ifmm_rsp->appendBitField( 1, HO_ID_Indicator);

				if (HO_ID_Indicator == 1)
				{
					ifmm_rsp->appendBitField( 8, HO_ID);
				}

				ifmm_rsp->appendBitField( 1, HO_auth_Indicator);
				ifmm_rsp->appendBitField( 4, rsv);

				if (HO_auth_Indicator == 1)
				{
					ifmm_rsp->appendBitField( 8, HO_auth_support);
				}
			}
		}
	}
	else if (Mode == 0x01)
		;
	else if (Mode == 0x02)
		;
	else if (Mode == 0x03)
		;
	else if (Mode == 0x04)
		;
	else if (Mode == 0x05)
		;
	else if (Mode == 0x06)
		;
	else if (Mode == 0x07)
		;
	else
		;

	ifmm_rsp->appendBitField( 8, ActionTime);

	if (tmpBits % 8 != 0)
	{
		ifmm_rsp->appendBitField(8 - tmpBits % 8, padding);
	}

	delete tmpList;

	/*
	 * TLV encoded information
	 */
	pCurrentConnect->Insert(ifmm_rsp);
}

void msObject_NT::procMOB_SCNREQ(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procMOB_SCNREQ ",get_nid());
	ifmgmt *ifmm_req        = NULL;
	ifmgmt *ifmm_rsp        = NULL;
	int ifmmLen             = 0;
	uint8_t scanDuration    = 0;
	uint8_t interleave      = 0;
	uint8_t scanIterate     = 0;
	uint8_t CfgChangeCnt    = 0;
	uint8_t N_RecBS_Index   = 0;
	uint8_t N_RecBS_Full    = 0;
	uint8_t padding         = 0;
	uint8_t *NBR_BS_Index   = NULL;
	uint8_t *scanType_Index = NULL;
	uint8_t **NBR_BS_ID     = {NULL};
	uint8_t *scanType_Full  = NULL;
	int tmpBits             = 0;

	//printf("\nTime:%llu:MS(%d): msObject_NT::%s()\n", GetCurrentTime(), get_nid(), __func__);

	/*
	 * Extract MOB_SCN-REQ Message
	 */
	ifmm_req = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm_req->extractBitField( 8, &scanDuration);
	ifmm_req->extractBitField( 8, &interleave);
	ifmm_req->extractBitField( 8, &scanIterate);
	ifmm_req->extractBitField( 8, &N_RecBS_Index);
	tmpBits += 32;

	if (N_RecBS_Index != 0)
	{
		ifmm_req->extractBitField( 8, &CfgChangeCnt);

		NBR_BS_Index   = new uint8_t [N_RecBS_Index];
		scanType_Index = new uint8_t [N_RecBS_Index];
		memset(NBR_BS_Index, 0, N_RecBS_Index);
		memset(scanType_Index, 0, N_RecBS_Index);

		tmpBits += 8;
	}

	for (int i = 0;i < N_RecBS_Index;i++)
	{
		ifmm_req->extractBitField( 8, &NBR_BS_Index[i]);
		ifmm_req->extractBitField( 3, &scanType_Index[i]);

		tmpBits += 11;
	}

	ifmm_req->extractBitField(8, &N_RecBS_Full);
	tmpBits += 8;

	if (N_RecBS_Full != 0)
	{
		NBR_BS_ID = new uint8_t* [N_RecBS_Full];

		for (int i = 0;i < N_RecBS_Full;i++)
		{
			NBR_BS_ID[i] = new uint8_t [48];
			memset(NBR_BS_ID[i], 0, 48);
		}
		scanType_Full = new uint8_t [N_RecBS_Full];
		memset(scanType_Full, 0, N_RecBS_Full);
	}

	for (int i = 0;i < N_RecBS_Full;i++)
	{
		ifmm_req->extractBitField(48, NBR_BS_ID[i]);
		ifmm_req->extractBitField( 3, &scanType_Full[i]);

		tmpBits += 51;
	}

	if (tmpBits % 8 != 0)
	{
		ifmm_req->extractBitField(8 - tmpBits % 8, &padding);
		ifmmLen = tmpBits / 8 + 1;
	}
	else
	{
		ifmmLen = tmpBits / 8;
	}

	delete ifmm_req;

	/*
	 * TLV encoding Information
	 */
	if ((tmpBits / 8) != (len - 1))
	{
		int type = 0;
		uint8_t hmac[21] = "";

		ifmm_req = new ifmgmt((uint8_t *) recvmm, len, ifmmLen);

		while ((type = ifmm_req->getNextType()) != 0)
		{
			switch (type) {
				case 149:
					ifmm_req->getNextValue(hmac);
					break;
			}
		}
		delete ifmm_req;
	}

	// Check if need to reject
	if ((CfgChangeCnt !=servingBS->NeighborMRBSList->NBRADV_CfgCount || N_RecBS_Index == 0) && N_RecBS_Full == 0)
	{
		scanDuration = 0;
	}

	/*
	 * Compute all field bits
	 */
	uint8_t repMode   = 0x0; // 0b00: No report
	uint8_t rsv       = 0;   // Reserve
	uint8_t repPeriod = 0;   // Not period
	uint8_t repMetric = 0x0; // Empty metric

	tmpBits = 32;

	if (scanDuration != 0)
	{
		tmpBits += 32;

		// Check MOB_NBR-ADV CfgChangeCnt
		if (CfgChangeCnt == servingBS->NeighborMRBSList->NBRADV_CfgCount)
		{
			if (N_RecBS_Index != 0)
				tmpBits += 8;

			for (int i = 0;i < N_RecBS_Index;i++)
			{
				tmpBits += 11;

				if (scanType_Index[i] == 0x2 || scanType_Index[i] == 0x3)
					tmpBits += 24;
			}
		}

		tmpBits += 8;

		for (int i = 0;i < N_RecBS_Full;i++)
		{
			tmpBits += 51;

			if (scanType_Full[i] == 0x2 || scanType_Full[i] == 0x3)
				tmpBits += 24;
		}

		if (tmpBits % 8 != 0)
		{
			ifmmLen = tmpBits / 8 + 1;
		}
		else
		{
			ifmmLen = tmpBits / 8;
		}
	}

	/*
	 * Generate MOB_SCN-RSP Message
	 */
	uint8_t startFrame     = 0; // 0 means that start scanning at next frame
	uint8_t rendezvousTime = 0;
	uint8_t CDMA_code      = 0;
	uint8_t TxopOffset     = 0;

	ifmm_rsp = new ifmgmt(MG_MOB_SCNRSP, ifmmLen);

	ifmm_rsp->appendBitField( 8, scanDuration);
	ifmm_rsp->appendBitField( 2, repMode);
	ifmm_rsp->appendBitField( 6, rsv);
	ifmm_rsp->appendBitField( 8, repPeriod);
	ifmm_rsp->appendBitField( 8, repMetric);

	if (scanDuration != 0)
	{
		// Check MOB_NBR-ADV CfgChangeCnt
		if (CfgChangeCnt == servingBS->NeighborMRBSList->NBRADV_CfgCount)
		{
			ifmm_rsp->appendBitField( 4, startFrame);
			ifmm_rsp->appendBitField( 1, rsv);
			ifmm_rsp->appendBitField( 8, interleave);
			ifmm_rsp->appendBitField( 8, scanIterate);
			ifmm_rsp->appendBitField( 3, padding);
			ifmm_rsp->appendBitField( 8, N_RecBS_Index);

			/* MS Scanning Related */
			ScanDuration    = scanDuration;
			InterleaveInt   = interleave;
			ScanIteration   = scanIterate;
			ChangeToScanCnt = startFrame;

			if (N_RecBS_Index != 0)
			{
				ifmm_rsp->appendBitField( 8, servingBS->NeighborMRBSList->NBRADV_CfgCount);
			}

			for (int i = 0;i < N_RecBS_Index;i++)
			{
				ifmm_rsp->appendBitField( 8, NBR_BS_Index[i]);
				ifmm_rsp->appendBitField( 3, scanType_Index[i]);

				if (scanType_Index[i] == 0x2 || scanType_Index[i] == 0x3)
				{
					ifmm_rsp->appendBitField( 8, rendezvousTime);
					ifmm_rsp->appendBitField( 8, CDMA_code);
					ifmm_rsp->appendBitField( 8, TxopOffset);
				}
			}
		}

		ifmm_rsp->appendBitField( 8, N_RecBS_Full);

		for (int i = 0;i < N_RecBS_Full;i++)
		{
			ifmm_rsp->appendBitField(48, NBR_BS_ID[i]);
			ifmm_rsp->appendBitField( 3, scanType_Full[i]);

			if (scanType_Full[i] == 0x2 || scanType_Full[i] == 0x3)
			{
				ifmm_rsp->appendBitField( 8, rendezvousTime);
				ifmm_rsp->appendBitField( 8, CDMA_code);
				ifmm_rsp->appendBitField( 8, TxopOffset);
			}
		}

		if (tmpBits % 8 != 0)
		{
			ifmm_rsp->appendBitField(8 - tmpBits % 8, padding);
		}
	}

	/*
	 * TLV encoded information
	 */

	pCurrentConnect->Insert(ifmm_rsp);

	// free memory
	if (N_RecBS_Index != 0)
	{
		delete [] NBR_BS_Index;
		delete [] scanType_Index;
	}

	if (N_RecBS_Full != 0)
	{
		for (int i = 0;i < N_RecBS_Full;i++)
		{
			delete NBR_BS_ID[i];
		}
		delete [] NBR_BS_ID;
		delete [] scanType_Full;
	}
}

void msObject_NT::procMOB_SCNREP(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procMOB_SCNREP ",get_nid());
	ifmgmt *ifmm              = NULL;
	int ifmmLen               = 0;
	uint8_t repMode           = 0;
	uint8_t Comp_NBR_BSID_IND = 0;
	uint8_t N_curr_BSs        = 0;
	uint8_t rsv               = 0;
	uint8_t repMetric         = 0;
	uint8_t N_BS_Index        = 0;
	uint8_t N_BS_Full         = 0;
	uint8_t tempBSID          = 0;
	uint8_t BS_CINR           = 0;
	uint8_t BS_RSSI           = 0;
	uint8_t RelativeDelay     = 0;
	uint8_t BS_RTD            = 0;
	uint8_t CfgChangeCnt      = 0;
	uint8_t neighborBS_Index  = 0;
	uint8_t neighborBS_ID[6]  = "";
	int tmpBits               = 0;

	/*
	 * Extract MOB_SCN-REP Message
	 */
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1);
	ifmm->extractBitField( 1, &repMode);
	ifmm->extractBitField( 1, &Comp_NBR_BSID_IND);
	ifmm->extractBitField( 3, &N_curr_BSs);
	ifmm->extractBitField( 3, &rsv);
	ifmm->extractBitField( 8, &repMetric);
	tmpBits += 16;

	// N_current_BSs
	for (int i = 0;i < N_curr_BSs;i++)
	{
		ifmm->extractBitField( 4, &tempBSID);
		ifmm->extractBitField( 4, &rsv);
		tmpBits += 8;

		if ((repMetric & 0x1) == 1)
		{
			ifmm->extractBitField( 8, &BS_CINR);
			tmpBits += 8;
		}

		if (((repMetric & 0x2) >> 1) == 1)
		{
			ifmm->extractBitField( 8, &BS_RSSI);
			tmpBits += 8;
		}

		if (((repMetric & 0x4) >> 2) == 1)
		{
			ifmm->extractBitField( 8, &RelativeDelay);
			tmpBits += 8;
		}

		if (((repMetric & 0x8) >> 3) == 1)
		{
			ifmm->extractBitField( 8, &BS_RTD);
			tmpBits += 8;
		}
	}

	// N_Neighbor_BS_Index
	ifmm->extractBitField( 8, &N_BS_Index);
	tmpBits += 8;

	if (N_BS_Index != 0)
	{
		ifmm->extractBitField( 8, &CfgChangeCnt);
		tmpBits += 8;
	}

	for (int i = 0;i < N_BS_Index;i++)
	{
		ifmm->extractBitField( 8, &neighborBS_Index);
		tmpBits += 8;

		if ((repMetric & 0x1) == 1)
		{
			ifmm->extractBitField( 8, &BS_CINR);
			tmpBits += 8;
		}

		if (((repMetric & 0x2) >> 1) == 1)
		{
			ifmm->extractBitField( 8, &BS_RSSI);
			tmpBits += 8;
		}

		if (((repMetric & 0x4) >> 2) == 1)
		{
			ifmm->extractBitField( 8, &RelativeDelay);
			tmpBits += 8;
		}
	}

	// N_Neighbor_BS_Full
	ifmm->extractBitField( 8, &N_BS_Full);

	for (int i = 0;i < N_BS_Full;i++)
	{
		ifmm->extractBitField(48, neighborBS_ID);
		tmpBits += 48;

		if ((repMetric & 0x1) == 1)
		{
			ifmm->extractBitField( 8, &BS_CINR);
			tmpBits += 8;
		}

		if (((repMetric & 0x2) >> 1) == 1)
		{
			ifmm->extractBitField( 8, &BS_RSSI);
			tmpBits += 8;
		}

		if (((repMetric & 0x4) >> 2) == 1)
		{
			ifmm->extractBitField( 8, &RelativeDelay);
			tmpBits += 8;
		}
	}

	ifmmLen = tmpBits / 8;

	delete ifmm;

	/*
	 * TLV encoded Information
	 */
	if ((tmpBits / 8) != (len - 1))
	{
		int type = 0;
		uint8_t hmac[21] = "";

		ifmm = new ifmgmt((uint8_t *) recvmm, len, ifmmLen);

		while ((type = ifmm->getNextType()) != 0)
		{
			switch (type) {
				case 149:
					ifmm->getNextValue(hmac);
					break;
			}
		}

		delete ifmm;
	}
}

void msObject_NT::procMOB_HOIND(struct mgmt_msg *recvmm, int cid, int len)
{

	logRidvan(TRACE,"-->%d	msObject_NT::procMOB_HOIND ",get_nid());
	ifmgmt *ifmm    = NULL;
	int ifmmLen     = 0;
	uint8_t rsv     = 0;
	uint8_t Mode    = 0;
	uint8_t padding = 0;
	int tmpBits     = 0;
	uint64_t ticks  = 0;
	uint8_t HO_IND_type   = 0;
	uint8_t targetBSID[6] = "";
	uint8_t PreambleIndex = 0;
	uint8_t Ranging_Params_valid = 0;

	//printf("\n\e[1;33mTime:%llu BS %s()\e[0m\n", GetCurrentTime(), __func__);
	//printf("============================\n");
	/*
	 * Extract MOB_HO-IND Message
	 */
	ifmm = new ifmgmt((uint8_t *) recvmm, len, len - 1); // Here, the (len - 1) is the maximum length of mgmt_msg, not field size

	ifmm->extractBitField( 6, &rsv);
	//printf("== rsv: %d\n", rsv);

	ifmm->extractBitField( 2, &Mode);
	//printf("== Mode: %d\n", Mode);

	tmpBits += 8;

	if (Mode == 0x0)
	{
		ifmm->extractBitField( 2, &HO_IND_type);
		//printf("== HO_IND_type: %d\n", HO_IND_type);

		ifmm->extractBitField( 2, &Ranging_Params_valid);
		//printf("== Ranging_Params_valid: %d\n", Ranging_Params_valid);

		ifmm->extractBitField( 4, &rsv);
		//printf("== rsv: %d\n", rsv);

		tmpBits += 8;

		if (HO_IND_type == 0) // Serving BS release
		{
			ifmm->extractBitField(48, targetBSID);
			//printf("== targetBSID: %d:%d:%d:%d:%d:%d\n", targetBSID[0], targetBSID[1], targetBSID[2], targetBSID[3], targetBSID[4], targetBSID[5]);

			tmpBits += 48;

			targetBS = new NbrMRBS_NT(-1, targetBSID); // FIXME

			// Start the Resource retain timer
			MILLI_TO_TICK(ticks, ResourceRetainTime * 100);
			timerResourceRetain->start(ticks, 0);
			ResourceStatus = PerformHO;

			printf("\e[1;35mTime:%llu ===== MS ResourceStatus change to PerformHO ===== ... BS won't allocate slot to this MS\e[0m\n", GetCurrentTime());
		}
		else if (HO_IND_type == 1) // cancel HO
		{
			timerResourceRetain->cancel();
		}
	}

	if (Mode == 0x1)
	{
		;
	}

	if (Mode == 0x2)
	{
		;
	}

	ifmm->extractBitField(8, &PreambleIndex);
	//printf("== PreambleIndex: %d\n", PreambleIndex);

	tmpBits += 8;

	if (tmpBits % 8 != 0)
	{
		ifmm->extractBitField(8 - tmpBits % 8, &padding);
		//printf("== padding: %d\n", padding);
		ifmmLen = tmpBits / 8 + 1;
	}
	else
	{
		ifmmLen = tmpBits / 8;
	}

	delete ifmm;

	//printf("============================\n");

	/*
	 * TLV encoding Information
	 */
	if ((tmpBits / 8) != (len - 1))
	{
		int type = 0;
		uint8_t hmac[21] = "";

		ifmm = new ifmgmt((uint8_t *) recvmm, len, ifmmLen);

		while ((type = ifmm->getNextType()) != 0)
		{
			switch (type) {
				case 149:
					ifmm->getNextValue(hmac);
					break;
			}
		}

		delete ifmm;
	}
}

void msObject_NT::Dump()
{

	logRidvan(TRACE,"-->%d	msObject_NT::Dump ",get_nid());
	list<DataConnection *>::iterator it;

	printf("Dump Subscribe Station %d Info: MacAddr = %s, diuc=%d uiuc=%d\n", 0, address()->str(), diuc, uiuc);
	for (int i = 0; i < 3; i++)
	{
		if (MnConnections[i])
		{
			printf("\tMnConnection[%d], CID=%d\n", i, MnConnections[i]->getCID());
		}
	}
	for (it = DtConnections.begin(); it != DtConnections.end();it++)
	{
		printf("\tDataConnection[], CID=%d\n", (*it)->getCID());
	}
	printf("\n");
}
