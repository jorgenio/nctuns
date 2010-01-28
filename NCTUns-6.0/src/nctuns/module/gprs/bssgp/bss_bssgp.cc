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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <maptable.h>
#include <gbind.h>
#include <phyInfo.h>
#include <iostream>
#include "bss_bssgp.h"
#include "pdu.h"
#include "leakbucket.h"
#include <gprs/include/bss_message.h>
#include <gprs/SndcpGmm/GPRS_mm_message.h>

extern RegTable                 RegTable_;

MODULE_GENERATOR(BssBSSGP);

BssBSSGP::BssBSSGP(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : GprsObject(type, id, pl, name), MaxVC(0xffff)
{

	/* bind variable */

	//BASE_OBJTYPE(type);
	//type = POINTER_TO_MEMBER(BssBSSGP, leak);
	//BVCBucket = new LeakBucket(this,type);
	//BASE_OBJTYPE(type2);
	//type2 = POINTER_TO_MEMBER(BssBSSGP, send);
	//MSCList = new CList(this,type2);
	/* register variable */
}


BssBSSGP::~BssBSSGP() {

	//delete BVCBucket;
	//delete MSCList;
}


int BssBSSGP::recv(ePacket_ *pkt) {
	//Packet			*p;
	
	//assert(pkt&&(p=(Packet *)pkt->DataInfo_));
	bss_message* bp = (bss_message*)pkt->DataInfo_;

	PDU* pdu = new PDU((unsigned char*)bp->bssgp_header);
	switch (pdu->type()){
		case FLOW_CONTROL_BVC_ACK:
			return 1;
			break;
		case DL_UNITDATA:
			unsigned int tlli = pdu->readIEDWord();
			unsigned char qos[3];
			bcopy(pdu->readIEValue(3),qos,3);
			PDU_FORMAT_CHK((tlli = pdu->readTLVDWord(IEI_PDU_LIFETIME)));
			//p->pkt_seek(20); // ? 20 bytes? 
			return (GprsObject::recv(pkt));
			break;
			
	}
	return 0;
}
       

int BssBSSGP::send(ePacket_ *pkt) {
	unsigned long tlli;

	//Packet* pkt_ = (Packet*) pkt->DataInfo_;
	bss_message* bp = (bss_message*) pkt->DataInfo_;

	//tlli = ((struct LLCOption*)bp->llc_option)->tlli;
    tlli = reinterpret_cast<llc_option*> (bp->llc_option)->tlli;
	/* add DL_UNITDATA header */
	PDU* pdu = new PDU();
	pdu->writeByte(UL_UNITDATA);
	pdu->writeTLVDWord(IEI_TLLI,tlli);
	pdu->writeTLVWord(IEI_PDU_LIFETIME,1000);

	unsigned char *p = (unsigned char*)malloc(sizeof(unsigned char[pdu->seek]));
	bp->bssgp_header = (char*)p;
	bcopy(pdu->_data,bp->bssgp_header,pdu->seek);
	bp->bssgp_header_len = pdu->seek;
	delete pdu;
	return(GprsObject::send(pkt));
}

void BssBSSGP::flowcontrol()
{
	ePacket_ *epkt  = createEvent();
	PDU* pdu = new PDU();
	pdu->writeByte(FLOW_CONTROL_BVC);
	unsigned char tag = 0;
	pdu->writeTLV(IEI_TAG,&tag,1);
	pdu->writeTLVWord(IEI_BVC_BUCKET_SIZE,D_BVC_BUCKET_SIZE);
	pdu->writeTLVWord(IEI_BUCKET_LEAK_RATE,D_BUCKET_LEAK_RATE );
	pdu->writeTLVWord(IEI_BMAX_DEFAULT_MS,D_BMAX_DEFAULT_MS );
	pdu->writeTLVWord(IEI_R_DEFAULT_MS,D_R_DEFAULT_MS );
	//Packet *pkt = new Packet;
	bss_message* bp = (bss_message*)malloc(sizeof(bss_message));
	bzero(bp,sizeof(bss_message));
	epkt->DataInfo_ = bp;
	bp->bssgp_header = (char*)malloc(pdu->seek);
	bcopy(pdu->_data, bp->bssgp_header, pdu->seek);
	//bp->bssgp_header = (char*)pdu->_data;
	bp->bssgp_header_len = pdu->seek;
	bp->flag |= PF_SEND;

	GprsObject::sendManage(epkt);
	delete pdu;
	//free(bp->bssgp_header);
	//freePacket(epkt);

	/*
	ePacket_ *epkt2  = createEvent();
	PDU* pdu2 = new PDU();
	pdu2->writeByte(FLOW_CONTROL_MS);
	tag = 0;
	pdu2->writeTLV(IEI_TAG,&tag,1);
	pdu2->writeTLVWord(IEI_MS_BUCKET_SIZE,D_MS_BUCKET_SIZE);
	pdu2->writeTLVWord(IEI_BUCKET_LEAK_RATE,D_BUCKET_LEAK_RATE);
	Packet *pkt2 = new Packet;
	ATTACH_PKT(pkt2,epk2t);
	tmp = (unsigned char*)pkt2->pkt_malloc(sizeof(unsigned char[pdu2->len]));
	bcopy(pdu2->_data, tmp, pdu2->len);
	delete pdu2;
	GprsObject::send(epkt2);
	*/
}

int BssBSSGP::init() {
	timerObj* timer = new timerObj();
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(BssBSSGP, flowcontrol);
	timer->setCallOutObj(this,type);
	int t1,t2;
	t1=5;
	SEC_TO_TICK(t2,t1);
	timer->start(0,t2);
	return(GprsObject::init());  
}
