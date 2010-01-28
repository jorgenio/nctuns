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
#include <mesh/meshsw.h>
#include <ethernet.h>
#include <netinet/in.h>
#include <packet.h>
#include <mbinder.h>

#include <fstream>
#include <stdlib.h>
#include <string>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

#define SPAN_MULTICAST "\xff\xff\x0\x0\x0\x0"
#define SPAN_BROADCAST "\xff\xff\xff\xff\xff\xff"
#define MAGICN 0x3F

#define MSW_printf if (0)printf
#define HOP_printf if (0)printf

MODULE_GENERATOR(MeshSW);


MeshSW::MeshSW(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
   

        /* initialize port */
	PortList = NULL;
	RealPortList = NULL;
	num_port = 0;

	/* switch should not buffer any pkt in s_queue or r_queue */
        s_flowctl = DISABLED;
        r_flowctl = DISABLED;

	/* initialize switch table */
        swTable = (swTbl *)malloc(sizeof(swTbl));
	swTable->name = new char[30];
	strcpy(swTable->name, "normal switch table");
        swTable->entrys = 0;
        swTable->head = swTable->tail = 0;

	/*====================================================
	 * initialize permanent switch table, this is specially
	 * used by AP.
	 *====================================================*/
	pmntSwTable = (swTbl *)malloc(sizeof(swTbl));
	pmntSwTable->name = new char[30];
	strcpy(pmntSwTable->name, "permanent switch table");
	pmntSwTable->entrys = 0;
	pmntSwTable->head = pmntSwTable->tail = 0;

	/* bind variable */
	vBind("flushInterval", &flushInterval);
	vBind("HelloTime", &HelloTime);
	vBind("MaxAge", &MaxAge);
	vBind("ForwardDelay", &ForwardDelay);

	SWITCH_MODE = NULL;
	SpanningTreeProtocol = NULL;
	MaxAge=3;
	HelloTime=1; 
	ForwardDelay=15;
}

MeshSW::~MeshSW() {

}

int MeshSW::findSwTbl(u_char *mac_, SwPort *port_, swTbl *targetTbl) {


	u_int64_t currentTime = GetCurrentTime();

	if( targetTbl == swTable )
		for ( mapTbl *mt = targetTbl->head, *mtLast = 0; mt; 
			mtLast = mt, mt = mt->next ) {
			if ( !bcmp(mt->mac, mac_, 6) ) {
				if ( mt->port == port_ ) {
					mt->timestamp = currentTime + flushInterval_; 
					return 1;
				}
				else {
					/* delete this entry because it is invalid */
			{
				char buf[256];
				macaddr_to_str(mt->mac,buf);
				MSW_printf("%s invalid delete %s %d %d\n",get_name(), buf, mt->port->nexthop[5], port_->nexthop[5]);
			}
					delSwTbl(mtLast, mt, targetTbl);
					return 0;
				}
			}
		}
	else if( targetTbl == pmntSwTable )
		for ( mapTbl *mt = targetTbl->head, *mtLast = 0; mt; 
			mtLast = mt, mt = mt->next ) {
			if ( !bcmp(mt->mac, mac_, 6) ) {
				if(  mt->port == port_ )
					return 1;
				else {
					/* delete this entry because it is invalid */
			{
				char buf[256];
				macaddr_to_str(mt->mac,buf);
				MSW_printf("%s invalid2 delete %s\n",get_name(), buf);
			}
					delSwTbl(mtLast, mt, targetTbl);
					return 0;
				}
			}
		}

	return 0;
}

SwPort *MeshSW::findport(u_char *mac_, swTbl *targetTbl, u_char* nhop ) {

	//u_int64_t currentTime = GetCurrentTime();

	if( targetTbl == swTable )
		for ( mapTbl *mt = targetTbl->head; mt; mt = mt->next ) {
			if ( !bcmp(mt->mac, mac_, 6) ){
				//if (nhop != NULL) memcpy(nhop, mt->port->nexthop, 6);
				return mt->port;
			}
		}
	else if( targetTbl == pmntSwTable )
		for ( mapTbl *mt = targetTbl->head; mt; mt = mt->next ) {
			if ( !bcmp(mt->mac, mac_, 6) ) {
				return mt->port;
			}
		}

	return 0;
}


int MeshSW::addSwTbl(u_char *mac_, SwPort *port_, swTbl *targetTbl) {

	char 		buf[256];
	if ( targetTbl->entrys >= SW_TBL_MAX_ENTRYS ) {
		printf("%s is full.\n", targetTbl->name);
		return -1;
	}

	/* this record already exists, so needn't insert. */
	if ( findSwTbl(mac_, port_, targetTbl) )
		return 1;
	if ( findSwTbl(mac_, port_, pmntSwTable) )
		return 1;
	macaddr_to_str(mac_,buf);

	/* if not exists, establish the record. */
	mapTbl *newRecord = (mapTbl *)malloc(sizeof(mapTbl));
	bzero(newRecord,sizeof(mapTbl));
	bcopy(mac_, newRecord->mac, 6);
	newRecord->port = port_;

	/*
	SwPort	*tmpSwPort;
	for ( tmpSwPort = PortList; 
	      tmpSwPort != NULL ; 
	      tmpSwPort = tmpSwPort->nextport) {
		if ( tmpSwPort->port && (tmpSwPort == port_) )
			break;
        }*/
	newRecord->portNum = (u_int8_t)port_->portNum;
	newRecord->timestamp = GetCurrentTime() + flushInterval_;
	newRecord->next = 0;

	/* insert the record */
	if ( targetTbl->head && targetTbl->tail ) {
		/* switch table already has records */
		targetTbl->tail->next = newRecord;
		targetTbl->tail = newRecord;
	} else {
		/* this is the first record inserting into switch table */
		targetTbl->head = newRecord;
		targetTbl->tail = newRecord;
	}
	targetTbl->entrys++;

	return 1;
}

int MeshSW::delSwTbl(mapTbl *lastEntry, mapTbl *delEntry, swTbl *targetTbl) {

	if ( targetTbl->entrys <= 0 ) {
		printf("%s is empty.\n", targetTbl->name);
		return -1;
	} else 
	if ( targetTbl->entrys == 1 ) {
		targetTbl->head = 0;
		targetTbl->tail = 0;
		targetTbl->entrys = 0;
	} else {
		if ( delEntry == targetTbl->head ) {
			/* delete entry is head of switch table */
			targetTbl->head = targetTbl->head->next;
			delEntry->next = 0;
		} else 
		if ( delEntry == targetTbl->tail ) {
			/* delete entry is tail of switch table */
			targetTbl->tail = lastEntry;
			targetTbl->tail->next = 0;
		} else {
			/* delete entry is in one middle site */
			lastEntry->next = delEntry->next;
			delEntry->next = 0;
		}
		targetTbl->entrys--;
	}
	free(delEntry);

	return 1;
}


int MeshSW::getSrcDstMac(u_char *src, u_char *dst, ePacket_ *pkt) {

	Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);

	/* decapsulate ether header */
	struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

	/* get ether src and ether dst */
	bcopy(eh->ether_shost, src, 6);
	bcopy(eh->ether_dhost, dst, 6);

        return 1;
}


int MeshSW::dumpSwTable() {
	SwPort *tmpSwPort;
	char buf[128];
	
	for (tmpSwPort = PortList; tmpSwPort; tmpSwPort= tmpSwPort->nextport){
		macaddr_to_str(tmpSwPort->nexthop,buf);
		printf("%s state = %d\n", buf, tmpSwPort->PortState);
	}
	printf ("%s root is %d\n",get_name(), SpanTreeTbl0.RootID);
        return 1;
}


/* main behaviors of switch are in send function */
int MeshSW::send(ePacket_ *pkt, MBinder *frm) {

        u_char          srcMac[6], dstMac[6],nextMac[6],sendMac[6],recvMac[6];
	MBinder		*frmport;
	SwPort		*tmpSwPort;
	ePacket_	*pkt_;
	Packet		*pk = (Packet*)pkt->DataInfo_;

	frmport = 0;
	getSrcDstMac(sendMac, recvMac, pkt);
	if (*(pk->pkt_get()+sizeof(struct ether_header)) == MAGICN){
		pk->pkt_seek(sizeof(struct ether_header) + 1);
		getSrcDstMac(srcMac, dstMac, pkt);
	}else{
		bcopy(sendMac,srcMac,6);
		bcopy(recvMac,dstMac,6);
	}

	for(  tmpSwPort = PortList; 
	     tmpSwPort != NULL ; 
	     tmpSwPort = tmpSwPort->nextport) {

		if (bcmp(tmpSwPort->nexthop, sendMac, 6) == 0) {
			frmport = tmpSwPort->port;
			break;
		}
	}


	/* 
	 *  Specially implemented for AP module. If dst is ETHER_MULTICAST1
	 *  (multicast address defined in ethernet.h), just learn it but
	 *  doesn't broadcast it.
	 */
	if ( !bcmp(recvMac, ETHER_MULTICAST1, 6) ) {
		Packet *pkt_ = (Packet *)pkt->DataInfo_;

		if( pkt_->pkt_getinfo("AddEn") )
			addSwTbl(srcMac, tmpSwPort, pmntSwTable);
		else if( pkt_->pkt_getinfo("DelEn") )
	        	for ( mapTbl *mt = pmntSwTable->head, *mtLast = 0; mt;
					mtLast = mt, mt = mt->next ) {
               			if ( !bcmp(mt->mac, srcMac, 6) ) {
                        		delSwTbl(mtLast, mt, pmntSwTable);
					break;
				}
        		}

		return 1;
	}

	if( SWITCH_MODE && !strcmp(SWITCH_MODE,"RunLearningBridgeProtocol") ){
		if (tmpSwPort){
			
			if (!findSwTbl(srcMac,tmpSwPort, swTable) && srcMac[5] == 1){
	                    char buf[256],buf2[256];
	                    macaddr_to_str(srcMac,buf);
	                    macaddr_to_str(dstMac,buf2);
		            //printf("%s : srcmac = %s dstmac = %s \n", get_name() , buf , buf2 );
			}
			addSwTbl(srcMac, tmpSwPort, swTable);
			
		}

{
	//char buf[256],buf2[256];
	//macaddr_to_str(srcMac,buf);
	//macaddr_to_str(dstMac,buf2);
	//MSW_printf("%s add %s -> %s %d\n",get_name(),buf,buf2,pk->pkt_getlen());
}


	}

        if ( SwPort *port_ = findport(dstMac, pmntSwTable,nextMac) ) {
{
	char buf[256],buf2[256];
	macaddr_to_str(dstMac,buf);
	macaddr_to_str(srcMac,buf2);
	MSW_printf("%s local send %s -> %s %d\n",get_name(),buf,buf2,pk->pkt_getlen());
}
                put(pkt, port_->port);
		return 1;
	}
        else if ( SwPort *port_ = findport(dstMac, swTable, nextMac) ) {
		if(bcmp ((const void*)port_->nexthop, (const void*)srcMac,6) != 0) {	
			
					pkt_ = pkt_copy(pkt);	
					pk = (Packet*)pkt_->DataInfo_;
					if (port_->portType == PT_ADHOC){
						u_char* ptr = (u_char *)pk->pkt_malloc(sizeof(struct ether_header) + 1);
						ptr[sizeof(struct ether_header)] = MAGICN; /* magic number */
						struct ether_header *eh = (struct ether_header*)ptr;
						memcpy(eh->ether_dhost, port_->nexthop, 6);
						memcpy(eh->ether_shost, port_->iface, 6);
						eh->ether_type = htons(0x0001);
						
						// hop count information
						unsigned char hopcount;
						char *p;
						p = pk->pkt_getinfo("hopcount");
						hopcount = p? *p : 0;
						hopcount++;
						pk->pkt_addinfo("hopcount", (char*)&hopcount,1);
					}else{
						unsigned char* hopcount;
						char buf[256];
						macaddr_to_str(dstMac,buf);
						hopcount = (unsigned char*)pk->pkt_getinfo("hopcount");
						if (pk->pkt_getlen() > 100)HOP_printf("%s send hop = %d to %s(%d) %d\n",get_name(), hopcount?*hopcount:0, buf,dstMac[5],srcMac[5]);
					}
{
	char buf[256],buf2[256];
	macaddr_to_str(port_->nexthop,buf);
	macaddr_to_str(dstMac,buf2);
	//MSW_printf("%s normal send %s -> %s %d\n",get_name(),buf,buf2,pk->pkt_getlen());
}
                			put(pkt_, port_->port);

			return 1;
		}
		else {
			freePacket(pkt);
			return 1;
		}
        }else if (bcmp(dstMac, "\0\0\0\0\0\0",6)){

		/* not found. we should broadcast it. */
		
		
		MBinder* sendlist[100];
		int send_num = 0;
		bzero(sendlist ,sizeof(sendlist));
		for( tmpSwPort = PortList; 
		     tmpSwPort != NULL ; 
		     tmpSwPort = tmpSwPort->nextport) 
		{
                        if((!strcmp(SpanningTreeProtocol,"off") || 
                            tmpSwPort->PortState!=0)) 
                       	{
				if( ( (tmpSwPort->portType == PT_ADHOC && bcmp(tmpSwPort->nexthop,"\0\0\0\0\0\0",6) ) ||
						(tmpSwPort->portType== PT_INFRA) ||
						(tmpSwPort->portType == PT_FIXED && frm->myModule() != tmpSwPort->port->bindModule()) )&&
					bcmp(tmpSwPort->nexthop,srcMac,6) && 
					bcmp(tmpSwPort->nexthop,sendMac,6)  
				)
				{
					if (tmpSwPort->portType == PT_ADHOC ){
				 		pkt_ = pkt_copy(pkt);
						pk = (Packet*)pkt_->DataInfo_;
						u_char* ptr = (u_char *)pk->pkt_malloc(sizeof(struct ether_header) + 1);
						ptr[sizeof(struct ether_header)] = MAGICN; 
						struct ether_header *eh = (struct ether_header*)ptr;
						memcpy(eh->ether_dhost, tmpSwPort->nexthop, 6);
						memcpy(eh->ether_shost, tmpSwPort->iface, 6);
						eh->ether_type = htons(0x0001);
{
	char buf[256],buf2[256];
	macaddr_to_str(dstMac,buf);
	macaddr_to_str(tmpSwPort->nexthop,buf2);
	MSW_printf("1 %d %s broadcast %s -> %s %d\n",tmpSwPort->portType,tmpSwPort->port->bindModule()->get_name(),buf,buf2,pk->pkt_getlen());
}
						put(pkt_, tmpSwPort->port);
					}else if (tmpSwPort->portType == PT_INFRA || tmpSwPort->portType == PT_FIXED){
						
						int k;
						for (k = 0; k < send_num; k++)
							if (sendlist[k]->bindModule() == tmpSwPort->port->bindModule())break;
						
						if (k ==  send_num){

							sendlist[send_num++] = tmpSwPort->port;
{
	char buf[256],buf2[256];
	macaddr_to_str(srcMac,buf);
	macaddr_to_str(dstMac,buf2);
	MSW_printf("23 %d %s -> %s broadcast %s -> %s %d\n",tmpSwPort->portType, tmpSwPort->port->bindModule()->get_name(),
			frm->myModule()->get_name(),buf,buf2,pk->pkt_getlen());
}
					 		pkt_ = pkt_copy(pkt);
							pk = (Packet*)pkt_->DataInfo_;
							put(pkt_, tmpSwPort->port);
						}
					}
                               
                         	}
			}		
		}
		

/*
		for( tmpSwPort = RealPortList; 
		     tmpSwPort != NULL ; 
		     tmpSwPort = tmpSwPort->nextport) 
		{
					if (tmpSwPort->portType == PT_ADHOC ){
				 		pkt_ = pkt_copy(pkt);
						pk = (Packet*)pkt_->DataInfo_;
						char* ptr = (char *)pk->pkt_malloc(sizeof(struct ether_header) + 1);
						ptr[sizeof(struct ether_header)] = MAGICN; 
						struct ether_header *eh = (struct ether_header*)ptr;
						memcpy(eh->ether_dhost, "0xff0xff0xff0xff0xff0xff", 6);
						memcpy(eh->ether_shost, tmpSwPort->iface, 6);
						eh->ether_type = htons(0x0001);
{
	char buf[256],buf2[256];
	macaddr_to_str(srcMac,buf);
	macaddr_to_str(tmpSwPort->nexthop,buf2);
	MSW_printf("1 %d %s broadcast %s -> %s %d\n",tmpSwPort->portType,tmpSwPort->port->bindModule()->get_name(),buf,buf2,pk->pkt_getlen());
}
					}else if (tmpSwPort->portType == PT_INFRA || tmpSwPort->portType == PT_FIXED){
						
{
	char buf[256],buf2[256];
	macaddr_to_str(srcMac,buf);
	macaddr_to_str(dstMac,buf2);
	MSW_printf("23 %d %s -> %s broadcast %s -> %s %d\n",tmpSwPort->portType, tmpSwPort->port->bindModule()->get_name(),
			frm->myModule()->get_name(),buf,buf2,pk->pkt_getlen());
}
					 		pkt_ = pkt_copy(pkt);
							pk = (Packet*)pkt_->DataInfo_;
					}
					put(pkt_, tmpSwPort->port);
                               
		}
*/
		freePacket(pkt);
		
		return 1;
	}
		
        return 1;
}


int MeshSW::recv(ePacket_ *pkt) {
	return 1;
}

int MeshSW::get(ePacket_ *pkt, MBinder *frm) {

	SwPort          	*tmpSwPort,*realSwPort, *typeSwPort;
	MBinder         	*frmport = 0;
	int             	frmNum=0,frmType=0;
	Packet			*p;
	struct ether_header	*eh;
	u_char			*data;


      	GET_PKT(p, pkt);
        eh = (ether_header *)p->pkt_get();
	data = (u_char*)eh + sizeof(ether_header);

	for(tmpSwPort=PortList;tmpSwPort!= NULL ;
	   tmpSwPort=tmpSwPort->nextport)
	{	
		if (tmpSwPort->port->bindModule() == frm->myModule()){
	                frmNum=tmpSwPort->portNum;
	                frmType=tmpSwPort->portNum;
			typeSwPort = tmpSwPort;
		}
	    	if ( bcmp((const void*)eh->ether_shost,(const void*)tmpSwPort->nexthop,6) == 0)
	    	{
	            	frmport = tmpSwPort->port;
	                frmNum=tmpSwPort->portNum;
	                frmType=tmpSwPort->portNum;
	                break;
	    	}
	}
	
	for(realSwPort=RealPortList;realSwPort!= NULL ;
	   realSwPort=realSwPort->nextport)
	{	
		if (realSwPort->port->bindModule() == frm->myModule()){
			break;
		}
	}

	/* if no such SwPort , add it*/
	if(tmpSwPort == NULL && frmType != PT_INFRA){
		num_port++;

		
{
	char buf[256],buf2[256];
	macaddr_to_str(eh->ether_shost,buf);
	macaddr_to_str(eh->ether_dhost,buf2);
	MSW_printf("%s NEW SwPort %s->%s\n",get_name(),buf,buf2);
}
		MBinder* tmpMBinder = new MBinder(this);
		assert(tmpMBinder);
		tmpMBinder->bind_to(frm->myModule());
		
		
		tmpSwPort = (SwPort *)malloc(sizeof(struct SwPort));
		if (*((char*)eh+sizeof(ether_header)) == MAGICN){
			tmpSwPort->portType = PT_ADHOC;
			tmpSwPort->portNum = frmNum;
			tmpSwPort->iface = GET_REG_VAR(frm->myModule()->get_port(),"MAC",u_char*);
			assert(tmpSwPort->iface);
		}else{
			tmpSwPort->portNum = frmNum;
			tmpSwPort->portType = frmType;
			tmpSwPort->iface = GET_REG_VAR(frm->myModule()->get_port(),"MAC",u_char*);
			assert(tmpSwPort->iface);
		}
			
		tmpSwPort->port = tmpMBinder;
		tmpSwPort->nextport = PortList;
		tmpSwPort->PortState = 1;
		tmpSwPort->Bridge = 0;
		tmpSwPort->RootID = 0;
		tmpSwPort->Cost = 0;

		memcpy(tmpSwPort->nexthop,eh->ether_shost,6);

		PortList = tmpSwPort;				
	}else if (tmpSwPort == NULL){
		addSwTbl(eh->ether_shost, typeSwPort, pmntSwTable);
		tmpSwPort = typeSwPort;
	}

	if (p->pkt_getlen() == sizeof(ether_header)){
		freePacket(pkt);
		return 0;
	}

        if(!bcmp(eh->ether_dhost,SPAN_MULTICAST,6) || 
			(data[0] != MAGICN && tmpSwPort->portType == PT_ADHOC ))
        {   
		if (realSwPort == NULL){
			printf("ERROR! not found real port\n");
              		freePacket(pkt);
			return 0 ;
		}
			
              	p->pkt_seek(sizeof(struct ether_header));

              	if(p->pkt_getlen()==4)
               		GetSpanTreeChange(p,frmport,tmpSwPort,frmNum);
              	else if (p->pkt_getlen() > 4)
               		GetSpanTreePacket(p,frmport,tmpSwPort,frmNum);

              	freePacket(pkt);
              	return 0;
        }
     
            
		
	/* change pkt flow to SEND_FLOW since switch is the top module */
  	if( !strcmp(SpanningTreeProtocol,"off") || tmpSwPort->PortState != 0 )  
           /* if Spanning Tree not close PORT,it can be got */
        {
         	if( !strcmp(SpanningTreeProtocol,"on") )
          	if( STABLE == '0' )
          	{
		MSW_printf("%s not route packet\n",get_name());
           		freePacket(pkt);
           		return 0;
          	}

		if ( tmpSwPort->portType == PT_ADHOC && bcmp(eh->ether_dhost,tmpSwPort->iface,6) != 0){
			freePacket(pkt);
			return 0;
		}
		
		//MSW_printf("%s route packet\n",get_name());
	 	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	 	pkt_->pkt_setflow(PF_SEND);
	 	return send(pkt, frm);
        }
        else 
        {
{
	char buf[256],buf2[256];
	macaddr_to_str(tmpSwPort->nexthop,buf);
	macaddr_to_str(eh->ether_dhost,buf2);
	MSW_printf("%s free route %s->%s %d\n",get_name(),buf,buf2, tmpSwPort->PortState );
}
            	freePacket(pkt);
            	return 0;
         }


	MSW_printf("%s i don't know what to do\n",get_name());
	

}


int MeshSW::parseLine(char *line, char *mac, int *portNum) {

	if ( line[0] == '#' ) return 0;

        char 	*tmp;
        tmp = strtok(line, " \t\r\n\b");
	if ( tmp == NULL ) return(-1);
        strcpy(mac, tmp);
	tmp = strtok(NULL, " \t\r\n\b");
	if ( tmp == NULL ) return(-1);
	*portNum = atoi(tmp);

	return 1;
}

int MeshSW::init() {

	SwPort 		*tmpSwPort;

	/* export variable */
	EXPORT("switch-table", E_RONLY);

	/*
	 *  <SWITCH_MODE>
	 *
	 *	RunLearningBridgeProtocol:
	 *		Capacity
	 *		1.Learning bridge.
	 *		2.Flush switch table.
	 *
	 */


	/* set default value if user doesn't specify */
	if ( !SWITCH_MODE ) {
		SWITCH_MODE = (char *)malloc( 30*sizeof(char) );
		strcpy(SWITCH_MODE, "RunLearningBridgeProtocol");		
	}

	if ( !flushInterval ) flushInterval = 3000; // 3000 millisecond

	/* transfer flush interval unit from second to tick */
        MILLI_TO_TICK(flushInterval_, (u_int64_t)flushInterval);

	/* setup switch based on switch mode */
	if( SWITCH_MODE && !strcmp(SWITCH_MODE,"RunLearningBridgeProtocol") ) {
		/* set timer function to regularly check switch table */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(MeshSW, flushSwTbl);
		flushTimer.setCallOutObj(this, type);
		flushTimer.start(flushInterval_, 0);
	}

        if (!SpanningTreeProtocol) {
            SpanningTreeProtocol = (char*)malloc(sizeof(char)*10);
            bzero(SpanningTreeProtocol,sizeof(char)*10);
            memcpy(SpanningTreeProtocol,"on" , 3);
        }
 
        if(SWITCH_MODE && !strcmp(SWITCH_MODE,"RunLearningBridgeProtocol")
                                     && !strcmp(SpanningTreeProtocol,"on") )
        {

          /* STABLE means the spanning tree stable(1)  or not(0) */
		STABLE='0';
		STABLEage=0;
		RootDeath=0;        

		if( !MaxAge )	
			MaxAge = 3;
		if( !HelloTime )
			HelloTime = 1; 
		if( !ForwardDelay )
			ForwardDelay = 15;
  
		SpanTimeOut = 1 * HelloTime;
		SEC_TO_TICK(SpanTimeOutTick, SpanTimeOut);          
		SEC_TO_TICK(HelloTimeTick, HelloTime);
		MILLI_TO_TICK(ForwardDelayTick, ForwardDelay);

		/* Set the Topology not changed */
		TCNFlag='0';
		TCAFlag='0';
		FlushSwTblFlag='0';

		/* Inital the Spanning Tree Port */

		for( tmpSwPort = PortList;
		     tmpSwPort != NULL ;
		     tmpSwPort = tmpSwPort->nextport) {
			tmpSwPort->PortState = 0;
			tmpSwPort->SwSignal = 0;
			tmpSwPort->RootID = 0;
			tmpSwPort->Cost = 0;
			tmpSwPort->Bridge = 0;
			tmpSwPort->Age = 0;
			tmpSwPort->iface = GET_REG_VAR( tmpSwPort->portNum,"MAC",u_char*);
			assert(tmpSwPort->iface);
		}
		
		for( tmpSwPort = RealPortList;
		     tmpSwPort != NULL ;
		     tmpSwPort = tmpSwPort->nextport) {
			tmpSwPort->iface = GET_REG_VAR( tmpSwPort->portNum,"MAC",u_char*);
		}

		/* Initial the Spanning Tree Record */
		SpanTreeTbl0.RootID = get_nid();
		SpanTreeTbl0.Cost = 0;
		SpanTreeTbl0.Bridge = get_nid();
		SpanTreeTbl0.Age = 0;
		bzero(SpanTreeTbl0.RootPort,6);
	
		BASE_OBJTYPE(type);
	 	/* Set Spanning tree timer = Hello Time */
		type = POINTER_TO_MEMBER(MeshSW, SendSpanTreePacket);
		SpanTimer.setCallOutObj(this,  type);
		SpanTimer.start(GetNodeCurrentTime(get_nid())+100,HelloTimeTick);
	
		/* Set Spanning tree time out function */
		type = POINTER_TO_MEMBER(MeshSW, CheckTimeOut);
		TimeOutTimer.setCallOutObj(this,type);
		TimeOutTimer.start(GetNodeCurrentTime(get_nid())+
					SpanTimeOutTick, SpanTimeOutTick);
	 }

	 return(1);  

}

int MeshSW::flushSwTbl() {

	/* take away entry which expires */
	u_int64_t currentTime = GetCurrentTime();
        for ( mapTbl *mt = swTable->head, *mtLast = 0, *mtNext = 0; mt; ) {
		if ( currentTime >= mt->timestamp ) {
			mtNext = mt->next;
			{
				char buf[256];
				macaddr_to_str(mt->mac,buf);
				MSW_printf("%s flush %s\n",get_name(), buf);
			}
			delSwTbl(mtLast, mt, swTable);
			mt = mtNext;
		} else {
			mtLast = mt;
			mt = mt->next;
		}
        }

	/* set timer function to regularly check switch table */
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(MeshSW, flushSwTbl);
        flushTimer.setCallOutObj(this, type);
	flushTimer.start(flushInterval_, 0);

	return 1;
}

  
int MeshSW::command(int argc, const char *argv[]) {

    NslObject                       *obj;

    if ( argc == 4 ) {

        bool uncanonical_module_name_flag = 0;

        /* The "." sign is not allowed as part of a module name. */
        if ( strstr(argv[3],".") ) {
            uncanonical_module_name_flag = 1;
        }
        /* The RegTable->lookup_Instance() with two parameter version
        * should use the canonical name of a module. If an illegal
        * module name is found, we should return immediately.
        * A canonical module name should start with
        * a prefix "NODE." As such, for any module names
        * without this prefix, this function simply returns
        * the original string as its output because the
        * name translation process may fail with incorrect
        * input names.
        */
        if (strncasecmp("NODE", argv[3], 4))
            uncanonical_module_name_flag = 1;

        /* A canonical module name should start with
        * a prefix "NODE." As such, for any module names
        * without this prefix, this function simply returns
        * the original string as its output because the
        * name translation process may fail with incorrect
        * input names.
        */

        /* Connectivity */
        if (!uncanonical_module_name_flag) {

            obj = RegTable_.lookup_Instance(get_nid(), argv[3]);
            if (!obj) {
                printf("The instance of %s does not exist.\n\n", argv[3]);
                return(-1);
            }
        }
    }
    else {
        /* on-line get switch table */
        if ( !strcmp(argv[0], "Get")&&(argc==2)) {
            if (!strcmp(argv[1], "switch-table")) {
                //dumpSwTable();
                return 1;
            }
        }
    }


    /* support port should be added here */
    MBinder		*tmpMBinder;
    SwPort		*tmpSwPort;
    u_int32_t	portNum;

    if (!strncmp(argv[1], "port", 4)) {
        sscanf(argv[1], "port%d", &portNum);

        num_port++;


        tmpMBinder = new MBinder(this);
        assert(tmpMBinder);
        tmpMBinder->bind_to(obj);

        tmpSwPort = (SwPort *)malloc(sizeof(struct SwPort));
        bzero(tmpSwPort->nexthop,6);
        tmpSwPort->portNum = (u_int8_t)portNum;
        tmpSwPort->portType = (u_int8_t)portNum;
        tmpSwPort->port = tmpMBinder;
        tmpSwPort->nextport = PortList;

        PortList = tmpSwPort;

        // real port list

        tmpMBinder = new MBinder(this);
        assert(tmpMBinder);
        tmpMBinder->bind_to(obj);

        tmpSwPort = (SwPort *)malloc(sizeof(struct SwPort));
        bzero(tmpSwPort->nexthop,6);
        tmpSwPort->portNum = (u_int8_t)portNum;
        tmpSwPort->portType = (u_int8_t)portNum;
        tmpSwPort->port = tmpMBinder;
        tmpSwPort->nextport = RealPortList;

        RealPortList = tmpSwPort;
    }
    else if (!strcmp(argv[1], "sendtarget"))
        sendtarget_->bind_to(obj);
    else if(!strcmp(argv[1], "recvtarget"))
        recvtarget_->bind_to(obj);
    else {
        printf("MeshSW::command(): Invalid command: %s %s %s %s.\n",argv[0],argv[1],argv[2],argv[3]);
        return(-1);
    }

    return(1);
}


int MeshSW::SpanTreeChange() {

	Packet			*pkt_;
	struct TopChangeINFO	*topchangeinfo;
	struct ether_header	*eh;
	Event_			*ep,*ep_cpy;
	SwPort			*tmpSwPort;

	STABLE = '0';
	printf("[33m%s[m is un STABLE\n",get_name());
	STABLEage = 0;

	ep = createEvent();
	pkt_ = new Packet;
	pkt_->pkt_setflow(PF_SEND);

	topchangeinfo = (struct TopChangeINFO *)pkt_->
				pkt_malloc(sizeof(struct TopChangeINFO));
	topchangeinfo->Protocol = 0;
	topchangeinfo->Version = (unsigned char)0;
	topchangeinfo->Message = (unsigned char)128;

	eh = (struct ether_header *)pkt_->
				pkt_malloc(sizeof(struct ether_header));
	memcpy(eh->ether_dhost, SPAN_MULTICAST, 6);
	eh->ether_type = htons(0x0001);
	ep->DataInfo_ = (void *)pkt_;

	if( bcmp(SpanTreeTbl0.RootPort,"\0\0\0\0\0\0",6) != 0 ) {
		for( tmpSwPort = PortList;
		     tmpSwPort != NULL ;
		     tmpSwPort = tmpSwPort->nextport) {
			if( bcmp(tmpSwPort->nexthop, SpanTreeTbl0.RootPort,6) == 0 )                                                
				break;
   	         }

		ep_cpy = pkt_copy(ep);
		pkt_ = (Packet*)ep_cpy->DataInfo_;
		eh = (struct ether_header*)pkt_->pkt_get();
		memcpy(eh->ether_shost, tmpSwPort->iface, 6);
		if (tmpSwPort->portType == PT_ADHOC){
			memcpy(eh->ether_dhost, tmpSwPort->nexthop, 6);
		}
		
		put(ep_cpy, tmpSwPort->port);
       
	}

        freePacket(ep);

	return 1;
}

int MeshSW::SendSpanTreePacket(Event_ *eventp){

	Packet			*pkt_,*pk;
        SpanINFO		*spaninfo;
        struct ether_header	*eh;
        Event_			*ep;
        Event_			*ep2;
        u_char			*OpenPort;
	ePacket_		*pkt_2;
	SwPort			*tmpSwPort;


	/* For the Topology Change */
	if( TCNFlag == '1' ) {
		if( bcmp(SpanTreeTbl0.RootPort,"\0\0\0\0\0\0",6) == 0 ) 
			SpanTreeChange();
		TCAFlag = '1';
	}
	else
		TCAFlag = '0';


	for (tmpSwPort = RealPortList; tmpSwPort != NULL; tmpSwPort = tmpSwPort->nextport){
		if (tmpSwPort->portType == PT_ADHOC){
			ep = createEvent();
			pkt_ = new Packet;
			pkt_->pkt_setflow(PF_SEND);
			eh = (struct ether_header*) pkt_->pkt_malloc(sizeof(struct ether_header));
			memcpy(eh->ether_dhost, SPAN_BROADCAST, 6);
			memcpy(eh->ether_shost, tmpSwPort->iface, 6);
			eh->ether_type = htons(0x0001);
			ep->DataInfo_ = (void *)pkt_;
			put(ep,tmpSwPort->port);
		}
	}
	
	ep = createEvent();
	pkt_ = new Packet;
	pkt_->pkt_setflow(PF_SEND);
	spaninfo = (SpanINFO *)pkt_->pkt_malloc(sizeof(SpanINFO));
        
	spaninfo->RootID = SpanTreeTbl0.RootID;
	spaninfo->Cost = SpanTreeTbl0.Cost;
	spaninfo->Bridge = get_nid();
         
	if( TCAFlag == '1' )
		spaninfo->TCAReservedTC = (unsigned char)128;
	else
		spaninfo->TCAReservedTC = (unsigned char)0;

	if(FlushSwTblFlag == '1' )
	{
		spaninfo->TCAReservedTC = spaninfo->TCAReservedTC +
						(unsigned char)1;
		FlushSwTblFlag = '0';
	}
       
	eh = (struct ether_header *)pkt_->pkt_malloc(sizeof(struct ether_header));
	memcpy(eh->ether_dhost, SPAN_MULTICAST, 6);
	eh->ether_type = htons(0x0001);
	ep->DataInfo_ = (void *)pkt_;


	//MSW_printf("%s SendSpanTreePacket\n",get_name());
	for( tmpSwPort = PortList;
		     tmpSwPort != NULL ;
		     tmpSwPort = tmpSwPort->nextport ) {

		if (bcmp(tmpSwPort->nexthop,"\0\0\0\0\0\0",6) == 0)continue;
		
		if( SpanTreeTbl0.RootID == (int)get_nid() ) {                           
			pkt_2 = pkt_copy(ep);
			pk = (Packet*)pkt_2->DataInfo_;
			eh = (struct ether_header*)pk->pkt_get();
			memcpy(eh->ether_shost, tmpSwPort->iface, 6);
			if (tmpSwPort->portType == PT_ADHOC){
				memcpy(eh->ether_dhost, tmpSwPort->nexthop, 6);
			}
			put(pkt_2, tmpSwPort->port);

			if( tmpSwPort->PortState ==0 ) {
				BASE_OBJTYPE(type);                  
				type = POINTER_TO_MEMBER(MeshSW, OpenSpanTreePort);

				ep2 = createEvent();
				OpenPort = (u_char*)malloc(6);
				bcopy(tmpSwPort->nexthop,OpenPort,6);
				setObjEvent(ep2, GetNodeCurrentTime(get_nid())
					+ForwardDelayTick,0,this,type,OpenPort);
			}
		}
		else if( tmpSwPort->RootID == 0 &&
			 tmpSwPort->Cost == 0 &&
			 tmpSwPort->Bridge == 0 ) {
				pkt_2 = pkt_copy(ep);
				pk = (Packet*)pkt_2->DataInfo_;
				eh = (struct ether_header*)pk->pkt_get();
				memcpy(eh->ether_shost, tmpSwPort->iface, 6);
				if (tmpSwPort->portType == PT_ADHOC){
					memcpy(eh->ether_dhost,tmpSwPort->nexthop, 6);
				}
				put(pkt_2, tmpSwPort->port);               
                  	
				if( tmpSwPort->PortState == 0 ) {
					BASE_OBJTYPE(type);
					type = POINTER_TO_MEMBER(MeshSW,
							OpenSpanTreePort);
					ep2 = createEvent();
					OpenPort = (u_char*)malloc(6);
					bcopy(tmpSwPort->nexthop,OpenPort,6);
                      		
					setObjEvent(ep2,GetNodeCurrentTime(get_nid())
						+ForwardDelayTick,0,this,type,OpenPort);
				}
		}
             	else if( bcmp(SpanTreeTbl0.RootPort,tmpSwPort->nexthop,6) != 0)  {
              
			if( (SpanTreeTbl0.RootID < tmpSwPort->RootID ) ||
			    ( (SpanTreeTbl0.RootID == tmpSwPort->RootID ) 
			      && (SpanTreeTbl0.Cost < tmpSwPort->Cost) ) ||
			    ( (SpanTreeTbl0.RootID == tmpSwPort->RootID )
			      && (SpanTreeTbl0.Cost == tmpSwPort->Cost)
			      && ((int)get_nid() < tmpSwPort->Bridge) ) ) {

				pkt_2 = pkt_copy(ep);
				pk = (Packet*)pkt_2->DataInfo_;
				eh = (struct ether_header*)pk->pkt_get();
				memcpy(eh->ether_shost, tmpSwPort->iface, 6);
				if (tmpSwPort->portType == PT_ADHOC){
					memcpy(eh->ether_dhost, tmpSwPort->nexthop, 6);
				}
				put(pkt_2, tmpSwPort->port);

                      		if( tmpSwPort->PortState == 0 ) {
					BASE_OBJTYPE(type);
					type = POINTER_TO_MEMBER(MeshSW, OpenSpanTreePort);
					ep2 = createEvent();
					OpenPort = (u_char*)malloc(6);
					bcopy(tmpSwPort->nexthop,OpenPort,6);
					setObjEvent(ep2, GetNodeCurrentTime(get_nid())
						+ ForwardDelayTick, 0, this, type, OpenPort);
				}

			}else if( tmpSwPort->PortState == 1 ) {
			char buf[256];
			macaddr_to_str(tmpSwPort->nexthop,buf);
			MSW_printf("%s close %s\n",get_name(),buf);
					tmpSwPort->PortState = 0;
					TCNFlag = '1';
					SpanTreeChange();
			}

		}
             	else if( bcmp(SpanTreeTbl0.RootPort,tmpSwPort->nexthop,6) == 0) {
                 			
			if( tmpSwPort->PortState == 0 ) {
				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(MeshSW, OpenSpanTreePort);

				ep2 = createEvent();
				OpenPort = (u_char*)malloc(6);
				bcopy(tmpSwPort->nexthop,OpenPort,6);
				setObjEvent(ep2, GetNodeCurrentTime(get_nid())
					+ForwardDelayTick, 0, this, type, OpenPort);
			}
		}
	}

	if( bcmp(SpanTreeTbl0.RootPort,"\0\0\0\0\0\0",6) == 0 ) {
		if( TCNFlag == '1' || TCAFlag == '1' || FlushSwTblFlag == '1') {
			TCNFlag = '0'; 
			TCAFlag = '0'; 
			FlushSwTblFlag = '0';
		}
	}

	RootDeath=0;
	freePacket(ep);

	return 1;
}


int MeshSW::GetSpanTreeChange(Packet *pkt_, MBinder *frm,
				SwPort *ThisPort, int frmNum) {
	
	struct TopChangeINFO	*topchangeinfo;

	topchangeinfo = (struct TopChangeINFO *)pkt_->pkt_get();
	TCNFlag = '1';
	
	//MSW_printf("%s GetSpanTreeChange\n",get_name());
	if( bcmp(SpanTreeTbl0.RootPort,"\0\0\0\0\0\0",6) == 0 ) {
		flushSwTbl();
		FlushSwTblFlag = '1';
		TCNFlag = '1';
	}
  		
	return 0;
}

int MeshSW::GetSpanTreePacket(Packet *pkt_, MBinder *frm, 
				SwPort *ThisPort, int frmNum) {

   	
	struct SpanINFO		*spaninfo;
	int			RootID_;
	int			Cost_;
	int			Bridge_;
	unsigned char		TCAReservedTC_;
	int			frmportNum = 0; 
                   

	frmportNum = frmNum;
	ThisPort->SwSignal = 1;
	ThisPort->Age = 0;
 
	
	spaninfo = (struct SpanINFO *)pkt_->pkt_get();
	RootID_ = spaninfo->RootID;
	Cost_ = spaninfo->Cost;
	Bridge_ = spaninfo->Bridge;
	TCAReservedTC_ = spaninfo->TCAReservedTC;
	//MSW_printf("%s GetSpanTreePacket %d %d\n",get_name(),RootID_,Bridge_);

	if( bcmp(SpanTreeTbl0.RootPort, ThisPort->nexthop,6) == 0) {
		if( (TCAReservedTC_ / 128) == 1 )/* Notify the children to clear TCN */
		 	TCNFlag = '0';

		if( (TCAReservedTC_ % 128) == 1 )/* Notify the children to clear SwTbl */
		{
			flushSwTbl();
			FlushSwTblFlag = '1';
		}
	}

	/* If it's the first record in port , we must write down it */
	if( ThisPort->Bridge == 0 ) {
		ThisPort->RootID = RootID_;
		ThisPort->Cost = Cost_;
		ThisPort->Bridge = Bridge_;
	}
	else if( ThisPort->Bridge == Bridge_ ) { 
		ThisPort->RootID = RootID_;
		ThisPort->Cost = Cost_;
 	    	
		if( bcmp(SpanTreeTbl0.RootPort, ThisPort->nexthop,6) == 0) {
			char buf[256];
			macaddr_to_str(ThisPort->nexthop,buf);
			SpanTreeTbl0.RootID = ThisPort->RootID;
			SpanTreeTbl0.Cost = ThisPort->Cost + 1;
		}    
	}
	else if( (ThisPort->RootID > RootID_) ||
		 ( (ThisPort->RootID == RootID_)
		  && (ThisPort->Cost > Cost_) ) ||
		 ( (ThisPort->RootID == RootID_)
		  && (ThisPort->Cost == Cost_)
		  && (ThisPort->Bridge > Bridge_) ) ) {
		ThisPort->RootID = RootID_;
		ThisPort->Cost = Cost_;
		ThisPort->Bridge = Bridge_;
	}

 	if( ThisPort->Bridge != 0 ) {
		if( (SpanTreeTbl0.RootID > ThisPort->RootID) ||
		    ( (SpanTreeTbl0.RootID == ThisPort->RootID)
		    && (SpanTreeTbl0.Cost > (ThisPort->Cost + 1)) ) ||
		    ( (SpanTreeTbl0.RootID == ThisPort->RootID)
		    && (SpanTreeTbl0.Cost == (ThisPort->Cost + 1))
		    && (SpanTreeTbl0.Bridge > ThisPort->Bridge) ) ) {
			
			bcopy(ThisPort->nexthop,SpanTreeTbl0.RootPort,6);
			char buf[256];
			macaddr_to_str(ThisPort->nexthop,buf);
			SpanTreeTbl0.RootID = ThisPort->RootID;
			SpanTreeTbl0.Cost = ThisPort->Cost+1;
			SpanTreeTbl0.Bridge = ThisPort->Bridge;
		}
	}

	return 0;
}

int MeshSW::OpenSpanTreePort(Event_ *ep) {

	u_char		*OpenPort;
	int 		NoDataPort = 0;
	int 		HighOrderPort = 0;
	SwPort		*tmpSwPort;
	MBinder		*frmport;
	

	OpenPort = (u_char*)ep->DataInfo_;
	frmport = 0;
	
	for (tmpSwPort = PortList; tmpSwPort != NULL; tmpSwPort = tmpSwPort->nextport){
		if (bcmp(tmpSwPort->nexthop,OpenPort,6)==0)break;
	}

	if( tmpSwPort->RootID == 0 && tmpSwPort->Cost == 0
				&&tmpSwPort->Bridge == 0 )
		NoDataPort = 1;

 	
	if( (SpanTreeTbl0.RootID < tmpSwPort->RootID) ||
	    ( (SpanTreeTbl0.RootID == tmpSwPort->RootID)
	    && (SpanTreeTbl0.Cost < tmpSwPort->RootID) ) ||
	    ( (SpanTreeTbl0.RootID == tmpSwPort->RootID)
	    && (SpanTreeTbl0.Cost == tmpSwPort->RootID)
	    && (SpanTreeTbl0.Bridge < tmpSwPort->Bridge) ) )
		HighOrderPort = 1;

 	
	if( bcmp(SpanTreeTbl0.RootPort ,OpenPort,6) == 0 ||
	    SpanTreeTbl0.RootID == (int)get_nid() ||
	    NoDataPort == 1 || 
	    HighOrderPort == 1 ) {

		if( tmpSwPort->PortState == 0 ) {
			char buf[256];
			macaddr_to_str(tmpSwPort->nexthop,buf);
			MSW_printf("%s open %s\n",get_name(),buf);
			tmpSwPort->PortState = 1;
			TCNFlag = '1';
			SpanTreeChange();
		}
	}
 		
	freeEvent(ep);
	return 1;
}

int MeshSW::CheckTimeOut() {

	unsigned int		RootPortDeath=0;
	SwPort		*tmpSwPort; 
    	

	if( STABLEage < MaxAge )
		STABLEage++;

	if( (STABLE == '0') && (STABLEage >= MaxAge) ){
		dumpSwTable();
		MSW_printf("[31m%s[m is STABLE\n",get_name());
		STABLE = '1';
	}

    	
	for( tmpSwPort = PortList;
	     tmpSwPort != NULL ;
	     tmpSwPort = tmpSwPort->nextport ) {

		if( tmpSwPort->Age < MaxAge ) {
			tmpSwPort->Age++;

			if( tmpSwPort->Age >= MaxAge ) {
				if( tmpSwPort->PortState == 0 ) {
			char buf[256];
			macaddr_to_str(tmpSwPort->nexthop,buf);
			MSW_printf("%s open s %s\n",get_name(),buf);
					tmpSwPort->PortState = 1;
					TCNFlag = '1';
					SpanTreeChange();
				}
  	     			
				tmpSwPort->RootID = 0;
				tmpSwPort->Cost = 0;
				tmpSwPort->Bridge = 0;
				tmpSwPort->Age = MaxAge;
				tmpSwPort->SwSignal = 0; 

				if( bcmp(SpanTreeTbl0.RootPort,tmpSwPort->nexthop, 6) == 0)
					RootPortDeath = 1;
			}
		} 
	}

	if( RootPortDeath == 1 ) {
		SpanTreeTbl0.RootID = get_nid();
		SpanTreeTbl0.Cost = 0;
		SpanTreeTbl0.Bridge = get_nid();
		bzero(SpanTreeTbl0.RootPort,6);
	}          

	return 1;
}
