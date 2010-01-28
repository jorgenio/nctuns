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
#include "pdu.h"
#include "leakbucket.h"
#include "sgsn_bssgp.h"
#include <gprs/include/bss_message.h>
#include <gprs/SndcpGmm/GPRS_mm_message.h>

extern RegTable                 RegTable_;

MODULE_GENERATOR(SgsnBSSGP);

CList::CList( NslObject* o, int (NslObject::*func)(Event_ *) )
{
	root = NULL;
	obj = o;
	memfunc = func;
}

void CList::setDefaultRate( double r)
{
	DefaultRate = r;
}

void CList::setDefaultBmax( int b)
{
	DefaultBmax = b;
}

CList::~CList()
{
	CListEntry* p1 = root, *p2;
	while( p1 != NULL){
		p2 = p1;
		p1 = p1->next;
		free(p2);
	}
}

LeakBucket* CList::Bucket(unsigned int id)
{
	CListEntry *p = root,*q = NULL;
	
	while ( p && p->id < id){
		q = p;
		p = p->next;
	}
	if ( p && p->id == id) return p->bucket;

	CListEntry *entry = new CListEntry;
	entry->bucket = new LeakBucket(obj,memfunc);
	entry->bucket->setRate(DefaultRate);
	entry->bucket->setBmax(DefaultBmax);
	entry->next = p == q ?NULL:p;
	entry->id = id;
	
	if (q) q->next = entry;
	else root = entry;

	printf("id %d bucket %p\n", id, entry);
	return entry->bucket;
}

int CList::remove(unsigned int id)
{
	CListEntry *p = root;
	while ( p->id < id){
		p = p->next;
	}
	if (p->id != id) return -1;
	
	CListEntry* tmp = p->next;
	p->bucket = tmp->bucket;
	p->id = tmp->id;
	p->next = tmp->next;
	free(tmp);
	return 0;
}

SgsnBSSGP::SgsnBSSGP(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : GprsObject(type, id, pl, name), MaxVC(0xffff)
{
	
	/* bind variable */
	//vBind("mode", &mode_);

	/* initiate log variables */
	//bw_ = NULL;
	//txState = rxState = false;
	//txBuf = rxBuf = NULL;

	BASE_OBJTYPE(type1);
	type1 = POINTER_TO_MEMBER(SgsnBSSGP, directSend);
	BVCBucket = new LeakBucket(this,type1);
	BASE_OBJTYPE(type2);
	type2 = POINTER_TO_MEMBER(SgsnBSSGP, leak);
	MSCList = new CList(this,type2);
	/* register variable */
}


SgsnBSSGP::~SgsnBSSGP() {

	delete BVCBucket;
	delete MSCList;
}


int SgsnBSSGP::recv(ePacket_ *pkt) {
	//Packet			*p;
	//double			SenderBw;
	//struct phyInfo		*phyinfo;
	unsigned long tlli;
	unsigned char tag;
	int bsize;
	int r;
	//unsigned char qos[3];
	unsigned char  lifetime[2];
	//unsigned char* tmp;
	bss_message *abp;
	ePacket_ *ackpkt;
	PDU *ack;
	LeakBucket* bucket;
	
	//assert(pkt&&(p=(Packet *)pkt->DataInfo_));
	//PDU* pdu = new PDU((unsigned char*)p->pkt_get());
	//
	bss_message* bp = (bss_message*)pkt->DataInfo_;
	PDU* pdu = new PDU((unsigned char*)bp->bssgp_header);

	switch (pdu->type()){
		case FLOW_CONTROL_BVC:
			tag = pdu->readTLVByte(IEI_TAG);
			bsize = pdu->readTLVWord(IEI_BVC_BUCKET_SIZE);
			r = pdu->readTLVWord(IEI_BUCKET_LEAK_RATE);
			/* set BVC leaking bucket */
			BVCBucket->setBmax(bsize  );
			BVCBucket->setRate((double)(r * 1000));
			
			bsize = pdu->readTLVWord(IEI_BMAX_DEFAULT_MS);
			r = pdu->readTLVWord(IEI_R_DEFAULT_MS);
			MSCList->setDefaultBmax(bsize );
			MSCList->setDefaultRate((double)(r * 1000));

			/* Free packet */
			delete pdu;
			free(bp->bssgp_header);
			//free(bp);
			freePacket(pkt);
			
			/* send ack */
			ack = new PDU();
			ack->writeByte(FLOW_CONTROL_BVC_ACK);
			ack->writeTLV(IEI_TAG, &tag, 1);
			ackpkt = createEvent();
			//apkt = new Packet;
			//ATTACH_PKT(apkt,ackpkt);
			//assert(apkt);
			abp = (bss_message*)malloc(sizeof(bss_message));
			memset(abp, 0, sizeof(bss_message));
			abp->bssgp_header = (char*)malloc(ack->seek);
			bcopy((char*)ack->_data,abp->bssgp_header,ack->seek);
			abp->bssgp_header_len = ack->seek;
			delete ack;

			abp->flag |= PF_SEND;
			ackpkt->DataInfo_ = abp;
			GprsObject::sendManage(ackpkt);
			return 1;

			break;
		case FLOW_CONTROL_MS:
			
			tlli = pdu->readTLVDWord(IEI_TLLI);
			tag = pdu->readTLVByte(IEI_TAG);
			bsize = pdu->readTLVWord(IEI_MS_BUCKET_SIZE);
			r = pdu->readTLVWord(IEI_BUCKET_LEAK_RATE);
			
			bucket = MSCList->Bucket(tlli);
			bucket->setBmax(bsize << 2);
			bucket->setRate(static_cast<double> (r >> 1));
			/* Free packet */
			delete pdu;
			free(bp->bssgp_header);
			//free(bp);
			freePacket(pkt);

			/* send ack */
			ack = new PDU();
			ack->writeByte(FLOW_CONTROL_MS_ACK);
			ack->writeTLVDWord(IEI_TLLI,tlli);
			ack->writeTLV(IEI_TAG, &tag, 1);
			ackpkt = createEvent();
			/*
			apkt = new Packet;
			ATTACH_PKT(apkt,ackpkt);
			assert(apkt);
			tmp = (unsigned char*)apkt->pkt_malloc(sizeof(unsigned char[ack->seek]));
			bcopy(ack->_data,tmp,ack->seek);
			delete ack;
			*/
			abp = new bss_message;
			memset(abp, 0, sizeof(abp));
			bcopy((char*)ack->_data,abp->bssgp_header,ack->seek);
			abp->bssgp_header_len = ack->seek;
			abp->flag |= PF_SEND;
			ackpkt->DataInfo_ = abp;
			
			delete abp;
			return GprsObject::sendManage(ackpkt);

			break;
		case UL_UNITDATA:
			tlli = pdu->readTLVDWord(IEI_TLLI);
			//bcopy(pdu->readIEValue(3),qos,3);
			pdu->readTLV(IEI_PDU_LIFETIME,(unsigned char*)lifetime);
			//PDU_FORMAT_CHK((pdu->readTLV(IEI_PDU_CELLID),cellid));
			/* extract PDU here and recv it */
			//PDU_FORMAT_CHK((pdu->readTLV(IEI_LLC_PDU),cellid));
			//apkt->pkt_seek(20); // ? 20 bytes? 
			return (GprsObject::recv(pkt));
			break;
			
	}
	return 1;
}
       

int SgsnBSSGP::send(ePacket_ *pkt) {
	
    assert(pkt);

    /* create a local copy for the sending pkt */
    //Event* new_ep = copy_gprs_pkt(pkt);
    //destroy_gprs_pkt(pkt);
    //pkt = new_ep;
    
    /* modified by jclin on 06/14/2004. Correct the structure type for llcop */
    llc_option* llcop  = NULL;
    unsigned long tlli = 0;

    bss_message* bpkt_ = (bss_message*) pkt->DataInfo_;

    llcop = reinterpret_cast<llc_option*> (bpkt_->llc_option);
    tlli = llcop->tlli;

    LeakBucket* bucket = MSCList->Bucket(tlli);

    int res = bucket->deliver(pkt,GetNodeCurrentTime(get_nid()));

    if (res == 0 ) {

	destroy_gprs_pkt(pkt);
	return 1;

    }
    else if ( res < 0 ) {

	destroy_gprs_pkt(pkt);
	return 1;
    }
    else
	;

    res = BVCBucket->deliver(pkt,GetNodeCurrentTime(get_nid())); 

    if ( res == 0 ) {

	destroy_gprs_pkt(pkt);
	return 1;

    }
    else if ( res < 0) {
	    
	destroy_gprs_pkt(pkt);
	return 1;

    }
    else
	 ;
    return directSend(pkt,false);
}

int SgsnBSSGP::directSend(ePacket_ *pkt,bool quepacket) {

	unsigned long tlli = 0;

	bss_message* bpkt_ = (bss_message*) pkt->DataInfo_;
	
	llc_option* llcop = reinterpret_cast<llc_option*> (bpkt_->llc_option);
	assert(llcop);

	tlli = llcop->tlli;

	if(bpkt_->bssgp_header_len == 0){

		/* add DL_UNITDATA header */
		PDU* pdu = new PDU();
		pdu->writeByte(DL_UNITDATA);
		pdu->writeV(tlli); // TLLI
		pdu->writeV((unsigned char)0x00); // Qos Profile

		bpkt_->bssgp_header = (char*)malloc(sizeof(char[pdu->seek]));
		bcopy(pdu->_data,bpkt_->bssgp_header,pdu->seek);
		//bpkt_->bssgp_header = strdup(pdu->_data,pdu->seek);
		bpkt_->bssgp_header_len = pdu->seek;
		delete pdu;
	
	}
	
	return(GprsObject::sendManage(pkt));
}

int SgsnBSSGP::leak(Event* pkt) {

	assert(pkt);

	int res = BVCBucket->deliver(pkt,GetNodeCurrentTime(get_nid())); 

	if ( res == 0 ) {

		destroy_gprs_pkt(pkt);
		return 1;

	}
	else if ( res < 0) {

		destroy_gprs_pkt(pkt);
		return 1;

	}
	else
		;


	return directSend(pkt,true);

}
