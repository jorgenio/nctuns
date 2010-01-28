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
#include <object.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include <exportStr.h>
#include <arp/arp.h>
#include <ethernet.h>
#include <arp/if_arp.h>
#include <ip.h>
#include <netinet/in.h>
#include <sys/param.h>
#include <maptable.h>
#include <packet.h>
#include <gbind.h>

#include <fstream>
#include <stdlib.h>
#include <string.h>

using namespace std;

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

MODULE_GENERATOR(arp);

arp::arp(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{

	/* initialize arp table */
        arpTable = (arpTbl *)malloc(sizeof(arpTbl));
        arpTable->entrys = 0;
        arpTable->head = arpTable->tail = 0;

	/* bind variable */
	vBind("arpMode", &ARP_MODE);
	vBind("flushInterval", &flushInterval);
	vBind("ArpTableFileName", &fileName);
}


arp::~arp() {


}


inline u_long arp::getDstIp(ePacket_ *pkt) {

	/* get next hop ip */
	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	return(pkt_->rt_gateway());
}


u_char *arp::findArpTbl(u_long ipDst, int &recordExistButNoMac) {

	/* first check if ipDst is broadcast ip */
	u_char	*ip = (u_char *)&ipDst;
	if ( ip[3] == 255 ) {
		return((u_char *)ETHER_BROADCAST);
	}
	
	/* default situation is that record doesn't exist */
	recordExistButNoMac = 0;

	/* first, find normal arp table */
	u_int64_t currentTime = GetCurrentTime();
	for ( mapTbl *mt = arpTable->head; mt; mt = mt->next ) {
		if ( mt->ip == ipDst ) {
			if ( bcmp(mt->mac, ETHER_NULLADDR, 6) ) {
				/* this record has mac */
				mt->timestamp = currentTime + flushInterval_;
				return mt->mac;
			}
			else {
				/* this record has no mac */
				recordExistButNoMac = 1;
				return 0;
			}
		}
	}

	return 0;
}


int arp::addArpTbl(u_long ip, u_char *mac, ePacket_ *pkt, arpTbl *targetArpTbl) {

	if ( targetArpTbl->entrys >= ARP_TBL_MAX_ENTRYS ) {
                printf("Arp table is full.\n");
                return -1;
        }

	/* establish this record */
	mapTbl *newRecord = (mapTbl *)malloc(sizeof(mapTbl));
	newRecord->ip = ip;
	if ( mac )
		bcopy(mac, newRecord->mac, 6);
	else
		bzero(newRecord->mac, 6);
        newRecord->timestamp = GetCurrentTime() + flushInterval_;
	/* pkt buffer - arp table maintains a pkt buffer for every
	 * entry(only one buffer) for re-sending. */
	newRecord->pkt = pkt;
        newRecord->next = 0;

        /* insert the record */
        if ( targetArpTbl->head && targetArpTbl->tail ) {
                /* switch table already has records */
                targetArpTbl->tail->next = newRecord;
                targetArpTbl->tail = newRecord;
        } else {
                /* this is the first record inserting into switch table */
                targetArpTbl->head = newRecord;
                targetArpTbl->tail = newRecord;
        }
        targetArpTbl->entrys++;

	return 1;
}


int arp::delArpTbl(mapTbl *lastEntry, mapTbl *delEntry) {

        if ( arpTable->entrys <= 0 ) {
                printf("normal arp table is empty.\n");
                return -1;
        } else
        if ( arpTable->entrys == 1 ) {
                arpTable->head = 0;
                arpTable->tail = 0;
                arpTable->entrys = 0;
        } else {
                if ( delEntry == arpTable->head ) {
                        /* delete entry is head of arp table */
                        arpTable->head = arpTable->head->next;
                        delEntry->next = 0;
                } else
                if ( delEntry == arpTable->tail ) {
                        /* delete entry is tail of arp table */
                        arpTable->tail = lastEntry;
                        arpTable->tail->next = 0;
                } else {
                        /* delete entry is in one middle site */
                        lastEntry->next = delEntry->next;
                        delEntry->next = 0;
                }
                arpTable->entrys--;
        }

	if ( delEntry->pkt ) {
		Packet *p;
		ePacket_ *_pkt = pkt_copy(delEntry->pkt);
		GET_PKT(p, _pkt);
		if (-1 == p->pkt_callout(_pkt)) {
			// if failed, free the duplicated packet
//			printf("arp::delArpTbl: pkt_callout failed\n");
			freePacket(_pkt);
			_pkt = NULL;
		}
		freePacket(delEntry->pkt);
	}

        free(delEntry);

        return 1;
}


int arp::dumpArpTable() {

	struct ExportStr	*ExpStr;
	u_int32_t		row,column;
	char			tmpBuf[30];

	ExpStr = new ExportStr(2);
	sprintf(tmpBuf, "IP\t\tMAC\n");
	ExpStr->Insert_comment(tmpBuf);

        for ( mapTbl *mt = arpTable->head; mt; mt = mt->next ) {

		row = ExpStr->Add_row();
		column = 1;

                ipv4addr_to_str(mt->ip, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t\t");

		macaddr_to_str(mt->mac, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\n");

        }

	EXPORT_GET_SUCCESS(ExpStr);
        return 1;
}


int arp::atchMacHdr(ePacket_ *pkt, u_char *macDst_, u_short frameType_) {

        Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);
	
	/* construct ether header */
        struct ether_header	*eh = 
	  (struct ether_header *)pkt_->pkt_malloc(sizeof(struct ether_header));

	/* directly filling the header space of the pkt */
	bcopy(mac_, eh->ether_shost, 6);
	bcopy(macDst_, eh->ether_dhost, 6);
	eh->ether_type = htons(frameType_);
	
	return 1;
}


int arp::updatePktBuf(u_long ipDst, ePacket_ *pkt) {

/* arp table maintains a buffer space for each ip. so if
 * another pkt comes and has the same ip as the record that
 * already in arp table(ex.record is waiting for arp reply)
 * , we take new incoming pkt replacing old one cz we only
 * maintain one buffer space for each ip.
 */
	mapTbl *mt;

    for ( mt = arpTable->head; mt; mt = mt->next )
    	if ( mt->ip == ipDst ) break;

	if ( mt->pkt ) {
		Packet *p;
		ePacket_ *_pkt = pkt_copy(mt->pkt);
		GET_PKT(p, _pkt);
		if (-1 == p->pkt_callout(_pkt)) {
			// if failed, free the duplicated packet
//			printf("arp::updatePktBuf, pkt_callout failed\n");
			freePacket(_pkt);
			_pkt = NULL;
		}
		freePacket(mt->pkt);
	}
	mt->pkt = pkt;

	return 1;
}

int arp::arpRequest(u_long ipDst) {

	/* construct arp pkt, attach it to event */
	ePacket_ *pkt = createEvent();
	Packet	 *pkt_ = new Packet;
	assert(pkt_);
	arpPkt	*arpPkt_ = (arpPkt *)pkt_->pkt_malloc( sizeof(arpPkt) );
	assert(arpPkt_);
	ATTACH_PKT(pkt_, pkt);

	/* fill arp header */
	arpPkt_->ar_hrd = htons(ARPHRD_ETHER);
	arpPkt_->ar_pro = htons(ETHERTYPE_IP);
	arpPkt_->ar_hln = ETHER_ADDR_LEN;
	arpPkt_->ar_pln = 4;
	arpPkt_->ar_op = htons(ARPOP_REQUEST);
	bcopy(mac_, arpPkt_->ar_sha, ETHER_ADDR_LEN);
	bcopy(ip_, arpPkt_->ar_spa, 4);
	bcopy(&ipDst, arpPkt_->ar_tpa, 4);

	/* attach mac header */
	atchMacHdr(pkt, (u_char *)ETHER_BROADCAST, ETHERTYPE_ARP);

	return(NslObject::send(pkt));
}

int arppktcounter = 0;

int arp::send(ePacket_ *pkt) {

	/* first get destination ip address */
	u_long ipDst = getDstIp(pkt);

	/* search arp table, if found, just send the pkt. if
	 * not, send arp request.
	 */
	int	recordExistButNoMac;
	if ( u_char *macDst = findArpTbl(ipDst, recordExistButNoMac) ) {

		atchMacHdr(pkt, macDst, ETHERTYPE_IP);
		return(NslObject::send(pkt));

	} else {

		if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {
			/* if record exists but has no mac, we only
			 * need to update buffer space.
			 */
			if ( recordExistButNoMac ) {
				updatePktBuf(ipDst, pkt);
			} 
			else {
				addArpTbl(ipDst, 0, pkt, arpTable);
				return(arpRequest(ipDst));
			}
		}
		else {
			freePacket(pkt);
			return (1);
		}		
	}

	return 1;
}


int arp::pktIsArp(ePacket_ *pkt) {

	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	
	struct ether_header *eh = 
		(struct ether_header *)pkt_->pkt_get();
	
	if ( ntohs(eh->ether_type) == ETHERTYPE_ARP ) {
		/* arp pkt */
		return 1;
	}
	
	/* not arp pkt, strip ether header */
	pkt_->pkt_seek(sizeof(struct ether_header));
	return 0;
}

/* free arp update: look up the incoming arp pkt, check spa and sha.
 * if my arp table has the entry whose ip equal spa, I can update
 * my table freely.
 */
int arp::freeArpUpdate(ePacket_ *pkt) {

	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));
	
        mapTbl *mt;
        for ( mt = arpTable->head; mt; mt = mt->next )
                if ( !bcmp(&mt->ip, arpPkt_->ar_spa, 4) ) break;

	if ( mt ) {
		bcopy(arpPkt_->ar_sha, mt->mac, 6);
		mt->timestamp = GetCurrentTime() + flushInterval_;
		return 1;
	}

	return 0;
}


int arp::iAmTpa(ePacket_ *pkt) {

        Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

	return(bcmp(arpPkt_->ar_tpa, ip_, 4) == 0);
}

/* if no free arp update, do free arp learning. */
int arp::freeArpLearning(ePacket_ *pkt) {

        Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

	u_long spaUlong;
	bcopy(arpPkt_->ar_spa, &spaUlong, 4);
	return addArpTbl(spaUlong, arpPkt_->ar_sha, 0, arpTable);
}


u_short arp::getArpOp(ePacket_ *pkt) {

        Packet *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt *arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));

	return ntohs(arpPkt_->ar_op);
}


int arp::arpReply(ePacket_ *pkt) {
	ePacket_	*reply_ep;
	Packet		*reply_pkt;
	Packet		*req_pkt;
	arpPkt		*Reply_arpPkt_;
	arpPkt		*Req_arpPkt_;

	/* get arp request pkt */
        req_pkt = (Packet *)pkt->DataInfo_;
	Req_arpPkt_ = (arpPkt *)req_pkt->pkt_get(sizeof(struct ether_header));

	/* create reply pkt */
	reply_ep = createEvent();
	reply_pkt = new Packet;
	assert(reply_pkt);
	Reply_arpPkt_ = (arpPkt *)reply_pkt->pkt_malloc( sizeof(arpPkt) );
	assert(Reply_arpPkt_);
	ATTACH_PKT(reply_pkt, reply_ep);

	/* fill arp header */
	Reply_arpPkt_->ar_hrd = htons(ARPHRD_ETHER);
	Reply_arpPkt_->ar_pro = htons(ETHERTYPE_IP);
	Reply_arpPkt_->ar_hln = ETHER_ADDR_LEN;
	Reply_arpPkt_->ar_pln = 4;
	Reply_arpPkt_->ar_op = htons(ARPOP_REPLY);
	bcopy(Req_arpPkt_->ar_spa, Reply_arpPkt_->ar_tpa, 4);
	bcopy(Req_arpPkt_->ar_tpa, Reply_arpPkt_->ar_spa, 4);
	bcopy(Req_arpPkt_->ar_sha, Reply_arpPkt_->ar_tha, 6);
	bcopy(mac_, Reply_arpPkt_->ar_sha, 6);

	/* process ether header */
	atchMacHdr(reply_ep, (u_char *)Reply_arpPkt_->ar_tha, ETHERTYPE_ARP);

	freePacket(pkt);

	return(NslObject::send(reply_ep));
}


int arp::resumeSend(ePacket_ *pkt) {

        Packet  *pkt_ = (Packet *)pkt->DataInfo_;
	arpPkt	*arpPkt_ = (arpPkt *)pkt_->pkt_get(sizeof(struct ether_header));
	u_long	spaUlong;
	bcopy(arpPkt_->ar_spa, &spaUlong, 4);
	
        mapTbl *mt;
        for ( mt = arpTable->head; mt; mt = mt->next )
                if ( mt->ip == spaUlong ) break;

	freePacket(pkt);

	if ( mt->pkt ) {
		ePacket_ *pktTmp = mt->pkt;
		mt->pkt = 0;
		atchMacHdr(pktTmp, mt->mac, ETHERTYPE_IP);

		return(NslObject::send(pktTmp));
	}

	return 1;
}


int arp::recv(ePacket_ *pkt) {
	int	mergeFlag;

	if ( pktIsArp(pkt) ) {
		if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {
			mergeFlag = freeArpUpdate(pkt);
			if ( iAmTpa(pkt) ) {
				if ( !mergeFlag ) freeArpLearning(pkt);
				u_short ar_op = getArpOp(pkt);
				if ( ar_op == ARPOP_REQUEST )
					return(arpReply(pkt));
				else if ( ar_op == ARPOP_REPLY )
					return(resumeSend(pkt));
				else
					assert(0);
			}
			else {
				freePacket(pkt);
				return 1;
			}
		}
		else {
			freePacket(pkt);
			return 1;
		}
	}
	return(NslObject::recv(pkt));
}



int arp::parseLine(char *line, char *ip, char *mac) {

        if ( line[0] == '#' ) return(-1);

        char    *tmp;

        tmp = strtok(line, " \t\r\n\b");
	if ( tmp == NULL ) return(-1);
        strcpy(ip, tmp);

        tmp = strtok(NULL, " \t\r\n\b");
	if ( tmp == NULL ) return(-1);
        strcpy(mac, tmp);

        return(1);
}


int arp::StrToIP(char *str, u_long &ip) {

        u_char          *p = (u_char *)&ip;

        char *tmp;
        tmp = strtok(str, ". \n\r\t\b");
        p[0] = (u_char)atoi(tmp);
        tmp = strtok(NULL, ". \n\r\t\b");
        p[1] = (u_char)atoi(tmp);
        tmp = strtok(NULL, ". \n\r\t\b");
        p[2] = (u_char)atoi(tmp);
        tmp = strtok(NULL, ". \n\r\t\b");
        p[3] = (u_char)atoi(tmp);

        return(1);
}


int arp::init() {

        NslObject::init();

	/* export variable */
	EXPORT("arp-table", E_RONLY);

	/* get MAC address */
	mac_ = GET_REG_VAR1(get_portls(), "MAC", u_char *);

	/* get IP address */
	ip_ = GET_REG_VAR1(get_portls(), "IP", u_long *);

	/*
	 *  <ARP_MODE>	
	 *
	 *	RunARP:
	 *		Capability
	 *		1.Send ARP reauest.
	 *		2.Gratuitous ARP.
	 *		3.Flush ARP table.
	 *
	 *	KnowInAdvance:
	 *		Capability
	 *		1.No ARP reauest .
	 *		2.No gratuitous ARP.
	 *		3.No ARP table flush.			
	 */


	if ( !ARP_MODE ) {
		ARP_MODE = (char *)malloc( 16*sizeof(char) );
		strcpy(ARP_MODE , "RunARP");
	}

	if ( !flushInterval ) flushInterval = 3000; // 3000 ms

        /* transfer flush interval unit from millisecond to tick */
        MILLI_TO_TICK(flushInterval_, (u_int64_t)flushInterval);

	if( ARP_MODE && !strcmp(ARP_MODE, "KnowInAdvance") ) {
		char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(fileName) + 1);
		sprintf(FILEPATH,"%s%s",GetConfigFileDir(),fileName);

                ifstream tblFile(FILEPATH);
                if ( !tblFile ) {
                        printf("Can't read in specified arp table <%s>.\n"
				,FILEPATH);
                        printf("Automatically turn arpMode to normal arp\n");
                        strcpy(ARP_MODE, "RunARP");
                }
		else {
                	char    line[128];
                	u_char  mac__[6] = {0};
			u_long	ip__ = 0;
                	char    ip[20] = {0}, mac[20] = {0};
                	for ( tblFile.getline(line, 128); strlen(line) > 0;
                        	tblFile.getline(line, 128) )
                	{
                        	if ( parseLine(line, ip, mac) < 0 )
					continue;

				StrToIP(ip, ip__);
                       		str_to_macaddr(mac, mac__);
				addArpTbl(ip__, mac__, 0, arpTable);
                	}
                	tblFile.close();
			return(1);
		}
		free(FILEPATH);		
	}
	
	if( ARP_MODE && !strcmp(ARP_MODE, "RunARP") ) {
		/* set timer function to regularly check normal arp table */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(arp, flushArpTbl);
		flushTimer.setCallOutObj(this, type);
		flushTimer.start(flushInterval_, 0);
	}
	else {
		ARP_MODE = (char *)malloc( 16*sizeof(char) );
		strcpy(ARP_MODE , "RunARP");

		/* set timer function to regularly check normal arp table */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(arp, flushArpTbl);
		flushTimer.setCallOutObj(this, type);
		flushTimer.start(flushInterval_, 0);
	}

        return(1);
}	


int arp::flushArpTbl() {

        /* take away entry which expires */
	u_int64_t currentTime = GetCurrentTime();
        for ( mapTbl *mt = arpTable->head, *mtLast = 0, *mtNext = 0; mt; ) {
                if ( currentTime >= mt->timestamp ) {
                        mtNext = mt->next;
                        delArpTbl(mtLast, mt);
                        mt = mtNext;
                } else {
                        mtLast = mt;
                        mt = mt->next;
                }
        }

        /* set timer function to regularly check normal arp table */
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(arp, flushArpTbl);
        flushTimer.setCallOutObj(this, type);
        flushTimer.start(flushInterval_, 0);

        return 1;
}

  
int arp::command(int argc, const char *argv[]) {

	if ( argc == 2 ) {
		/* on-line get arp table */
		if ( !strcmp(argv[0], "Get")&&(argc>=2)) {
			if (!strcmp(argv[1], "arp-table")) {
				dumpArpTable();
				return(1);
			}
		}	
	}

	return(NslObject::command(argc, argv));
}
