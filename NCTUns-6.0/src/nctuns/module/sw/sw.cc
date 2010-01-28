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
#include <scheduler.h>
#include <nodetype.h>
#include <sw/sw.h>
#include <ethernet.h>
#ifdef LINUX
#include <netinet/in.h>
#endif

#include <fstream>
#include <stdlib.h>
#include <string>
#include <exportStr.h>
#include <packet.h>
#include <mbinder.h>

extern RegTable                 RegTable_;
extern typeTable		*typeTable_;

#define SPAN_MULTICAST "\xff\xff\0\0\0\0"

MODULE_GENERATOR(sw);


sw::sw(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
   

        /* initialize port */
	PortList = NULL;
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
	vBind("SWITCH_MODE", &SWITCH_MODE);
	vBind("flushInterval", &flushInterval);
	vBind("SwitchTableFileName", &fileName);

	vBind("SpanningTreeProtocol", &SpanningTreeProtocol);
	vBind("HelloTime", &HelloTime);
	vBind("MaxAge", &MaxAge);
	vBind("ForwardDelay", &ForwardDelay);

	MaxAge=3;
	HelloTime=1; 
	ForwardDelay=15;
}

sw::~sw() {

}

int sw::findSwTbl(u_char *mac_, MBinder *port_, swTbl *targetTbl) {


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
					delSwTbl(mtLast, mt, targetTbl);
					return 0;
				}
			}
		}

	return 0;
}

MBinder *sw::findport(u_char *mac_, swTbl *targetTbl) {

	if( targetTbl == swTable )
		for ( mapTbl *mt = targetTbl->head; mt; mt = mt->next ) {
			if ( !bcmp(mt->mac, mac_, 6) )
				return mt->port;
		}
	else if( targetTbl == pmntSwTable )
		for ( mapTbl *mt = targetTbl->head; mt; mt = mt->next ) {
			if ( !bcmp(mt->mac, mac_, 6) ) {
				return mt->port;
			}
		}

	return 0;
}


int sw::addSwTbl(u_char *mac_, MBinder *port_, swTbl *targetTbl) {

	if ( targetTbl->entrys >= SW_TBL_MAX_ENTRYS ) {
		printf("%s is full.\n", targetTbl->name);
		return -1;
	}

	/* this record already exists, so needn't insert. */
	if ( findSwTbl(mac_, port_, targetTbl) )
		return 1;
	if ( findSwTbl(mac_, port_, pmntSwTable) )
		return 1;

	/* if not exists, establish the record. */
	mapTbl *newRecord = (mapTbl *)malloc(sizeof(mapTbl));
	bcopy(mac_, newRecord->mac, 6);
	newRecord->port = port_;

	int i;
	SwPort	*tmpSwPort;
	for ( i = 1, tmpSwPort = PortList; 
	      //tmpSwPort != NULL, i <= num_port; 
	      tmpSwPort != NULL && i <= (int)num_port; 
	      tmpSwPort = tmpSwPort->nextport, i++ ) {
		if ( tmpSwPort->port && (tmpSwPort->port == port_) )
			break;
        }
	newRecord->portNum = (u_int8_t)tmpSwPort->portNum;
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

int sw::delSwTbl(mapTbl *lastEntry, mapTbl *delEntry, swTbl *targetTbl) {

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


int sw::getSrcDstMac(u_char *src, u_char *dst, ePacket_ *pkt) {

	Packet	*pkt_ = (Packet *)pkt->DataInfo_;
	assert(pkt_);

	/* decapsulate ether header */
	struct ether_header *eh = (struct ether_header *)pkt_->pkt_get();

	/* get ether src and ether dst */
	bcopy(eh->ether_shost, src, 6);
	bcopy(eh->ether_dhost, dst, 6);

        return 1;
}


int sw::dumpSwTable() {
	
	struct ExportStr	*ExpStr;
	u_int32_t		row,column;
	char			tmpBuf[30];


	ExpStr = new ExportStr(2);

	/* dump normal switch table */
        for ( mapTbl *mt = swTable->head; mt; mt = mt->next ) {
		row = ExpStr->Add_row();
		column = 1;

                macaddr_to_str(mt->mac, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%d",mt->portNum);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\n");
        }

	/* dump permanent switch table */
	for ( mapTbl *mt = pmntSwTable->head; mt; mt = mt->next )
	{
		row = ExpStr->Add_row();
		column = 1;

                macaddr_to_str(mt->mac, tmpBuf);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\t");

		sprintf(tmpBuf,"%d",mt->portNum);
		ExpStr->Insert_cell(row, column++, tmpBuf, "\n");
        }

	EXPORT_GET_SUCCESS(ExpStr);
        return 1;
}


/* main behaviors of switch are in send function */
int sw::send(ePacket_ *pkt, MBinder *frm) {

        u_char          srcMac[6], dstMac[6];
	MBinder		*frmport;
	int		i;
	SwPort		*tmpSwPort;
	ePacket_	*pkt_;

	frmport = 0;
	getSrcDstMac(srcMac, dstMac, pkt);

	for( i = 1, tmpSwPort = PortList; 
	     //tmpSwPort != NULL, i <= num_port; 
	     tmpSwPort != NULL && i <= (int)num_port; 
	     tmpSwPort = tmpSwPort->nextport, i++ ) {

		if (frm->myModule() == tmpSwPort->port->bindModule()) {
			frmport = tmpSwPort->port;
			break;
		}
	}

	/* 
	 *  Specially implemented for AP module. If dst is ETHER_MULTICAST1
	 *  (multicast address defined in ethernet.h), just learn it but
	 *  doesn't broadcast it.
	 */
	if ( !bcmp(dstMac, ETHER_MULTICAST1, 6) ) {
		Packet *pkt_ = (Packet *)pkt->DataInfo_;

		if( pkt_->pkt_getinfo("AddEn") )
			addSwTbl(srcMac, frmport, pmntSwTable);
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

	if( SWITCH_MODE && !strcmp(SWITCH_MODE,"RunLearningBridgeProtocol") )
		addSwTbl(srcMac, frmport, swTable);

        if ( MBinder *port_ = findport(dstMac, pmntSwTable) ) {
                put(pkt, port_);
		return 1;
	}
        else if ( MBinder *port_ = findport(dstMac, swTable) ) {
		if(port_ != frmport) {	
			for( i = 1, tmpSwPort = PortList;
			     //tmpSwPort != NULL, i <= num_port;
			     tmpSwPort != NULL && i <= (int)num_port;
			     tmpSwPort = tmpSwPort->nextport, i++ ) 
			{
			   if( tmpSwPort->port && (port_ == tmpSwPort->port) ) 
			   {
				if(!strcmp(SpanningTreeProtocol,"off") ||
				   tmpSwPort->PortState!=0) 
				{
					pkt_ = pkt_copy(pkt);	
                			put(pkt_, port_);
				}
				break;
			   }
			}
			freePacket(pkt);
			return 1;
		}
		else {
			freePacket(pkt);
			return 1;
		}
        } else {
		if( (SWITCH_MODE && !strcmp(SWITCH_MODE,"KnowInAdvance")) ) {
			if ( bcmp(dstMac, ETHER_BROADCAST, 6) ) {
				freePacket(pkt);
				return 1;		
			}
		}

		/* not found. we should broadcast it. */
		for( i = 1, tmpSwPort = PortList; 
		     //tmpSwPort != NULL, i <= num_port; 
		     tmpSwPort != NULL && i <= (int)num_port; 
		     tmpSwPort = tmpSwPort->nextport, i++ ) 
		{
			if( tmpSwPort->port && (frmport != tmpSwPort->port) ) 
			{
                            	if(!strcmp(SpanningTreeProtocol,"off") || 
                              	tmpSwPort->PortState!=0) /* Not close by Spanning Tree */
                        	{
				 	pkt_ = pkt_copy(pkt);
				 	put(pkt_, tmpSwPort->port);
                               
                         	}
			}		
		}

		freePacket(pkt);
		return 1;
        }
		
        return 1;
}


int sw::recv(ePacket_ *pkt) {
	return 1;
}

int sw::get(ePacket_ *pkt, MBinder *frm) {

	int 			i;
	SwPort          	*tmpSwPort;
	MBinder         	*frmport = 0;
	int             	frmNum=0;
	Packet			*p;
	struct ether_header	*eh;

	//for(i=1,tmpSwPort=PortList;tmpSwPort!= NULL,i<=num_port;
	for(i=1,tmpSwPort=PortList;tmpSwPort!= NULL && i<=(int)num_port;
	   tmpSwPort=tmpSwPort->nextport,i++)
	{	
	    	if (frm->myModule() == tmpSwPort->port->bindModule())
	    	{
	            	frmport = tmpSwPort->port;
	                frmNum=tmpSwPort->portNum;
	                break;
	    	}
	}

      	GET_PKT(p, pkt);
        eh = (ether_header *)p->pkt_get();

        if(!bcmp(eh->ether_dhost,SPAN_MULTICAST,6))
        {    
              	p->pkt_seek(sizeof(struct ether_header));

              	if(p->pkt_getlen()==4)
               		GetSpanTreeChange(p,frmport,tmpSwPort,frmNum);
              	else
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
           		freePacket(pkt);
           		return 0;
          	}

	 	Packet *pkt_ = (Packet *)pkt->DataInfo_;
	 	pkt_->pkt_setflow(PF_SEND);
	 	return send(pkt, frm);
        }
        else 
        {
            	freePacket(pkt);
            	return 0;
         }



}


int sw::parseLine(char *line, char *mac, int *portNum) {

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

int sw::init() {

	int 		i;
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
	 *	KnowInAdvance:
	 *		Capacity
	 *		1.No learning bridge.
	 *		2.No switch table flush.
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
	if( (SWITCH_MODE && !strcmp(SWITCH_MODE,"KnowInAdvance")) ) {

		char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(fileName)+1);
		sprintf(FILEPATH,"%s%s",GetConfigFileDir() ,fileName);

		ifstream tblFile(FILEPATH);
		if ( !tblFile ) {
			printf("Warning:\n");
			printf("Can't read in specified switch table <%s>.\n"
				,FILEPATH);
			printf("Automatically turn swMode to learning bridge ");
			printf("mode with flush time interval 6ms.\n");

			strcpy(SWITCH_MODE, "RunLearningBridgeProtocol");
			flushInterval = 6000;
			MILLI_TO_TICK(flushInterval_,(u_int64_t)flushInterval);			
		}
		else {
			char	line[128];
			char	mac[20] = {0};
			u_char	mac_[6] = {0};
			int	portNum[1] = {0};
			int	lineAttr;
			int	i;
			SwPort	*tmpSwPort;

			for ( tblFile.getline(line, 128); strlen(line) > 0;
				tblFile.getline(line, 128) )
			{
				if ( (lineAttr = parseLine(line, mac, portNum)) ) {
					if ( lineAttr == -1 ) continue;
					str_to_macaddr(mac, mac_);

					for( i = 1, tmpSwPort = PortList; 
			     		     //tmpSwPort != NULL, i <= num_port; 
			     		     tmpSwPort != NULL && i <= (int)num_port; 
			     		     tmpSwPort = tmpSwPort->nextport, i++ ) {
						if( tmpSwPort->portNum == (u_int8_t)*portNum )
							break;
					}
					addSwTbl(mac_, tmpSwPort->port, swTable);
				}
			}
			tblFile.close();
		}
		free(FILEPATH);
	}

	if( SWITCH_MODE && !strcmp(SWITCH_MODE,"RunLearningBridgeProtocol") ) {
		/* set timer function to regularly check switch table */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(sw, flushSwTbl);
		flushTimer.setCallOutObj(this, type);
		flushTimer.start(flushInterval_, 0);
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

		for( i = 1, tmpSwPort = PortList;
		     //tmpSwPort != NULL, i <= num_port;
		     tmpSwPort != NULL && i <= (int)num_port;
		     tmpSwPort = tmpSwPort->nextport, i++ ) {
			tmpSwPort->PortState = 0;
			tmpSwPort->SwSignal = 0;
			tmpSwPort->RootID = 0;
			tmpSwPort->Cost = 0;
			tmpSwPort->Bridge = 0;
			tmpSwPort->Age = 0;
		}

		/* Initial the Spanning Tree Record */
		SpanTreeTbl0.RootID = get_nid();
		SpanTreeTbl0.Cost = 0;
		SpanTreeTbl0.Bridge = get_nid();
		SpanTreeTbl0.Age = 0;
		SpanTreeTbl0.RootPort = 0;
	
		BASE_OBJTYPE(type);
	 	/* Set Spanning tree timer = Hello Time */
		type = POINTER_TO_MEMBER(sw, SendSpanTreePacket);
		SpanTimer.setCallOutObj(this,  type);
		SpanTimer.start(GetNodeCurrentTime(get_nid())+100,HelloTimeTick);
	
		/* Set Spanning tree time out function */
		type = POINTER_TO_MEMBER(sw, CheckTimeOut);
		TimeOutTimer.setCallOutObj(this,type);
		TimeOutTimer.start(GetNodeCurrentTime(get_nid())+
					SpanTimeOutTick, SpanTimeOutTick);
	 }

	 return(1);  

}

int sw::flushSwTbl() {

	/* take away entry which expires */
	u_int64_t currentTime = GetCurrentTime();
        for ( mapTbl *mt = swTable->head, *mtLast = 0, *mtNext = 0; mt; ) {
		if ( currentTime >= mt->timestamp ) {
			mtNext = mt->next;
			delSwTbl(mtLast, mt, swTable);
			mt = mtNext;
		} else {
			mtLast = mt;
			mt = mt->next;
		}
        }

	/* set timer function to regularly check switch table */
        BASE_OBJTYPE(type);
        type = POINTER_TO_MEMBER(sw, flushSwTbl);
        flushTimer.setCallOutObj(this, type);
	flushTimer.start(flushInterval_, 0);

	return 1;
}

  
int sw::command(int argc, const char *argv[]) {

    NslObject* obj = NULL;

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
                dumpSwTable();
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
        tmpSwPort->portNum = (u_int8_t)portNum;
        tmpSwPort->port = tmpMBinder;
        tmpSwPort->nextport = PortList;

        PortList = tmpSwPort;
    }
    else if (!strcmp(argv[1], "sendtarget"))
        sendtarget_->bind_to(obj);
    else if(!strcmp(argv[1], "recvtarget"))
        recvtarget_->bind_to(obj);
    else {
        printf("sw::command(): Invalid command: %s %s %s %s.\n",argv[0],argv[1],argv[2],argv[3]);
        return(-1);
    }

    return(1);
}


int sw::SpanTreeChange() {

	int			j;
	Packet			*pkt_;
	struct TopChangeINFO	*topchangeinfo;
	struct ether_header	*eh;
	Event_			*ep,*ep_cpy;
	SwPort			*tmpSwPort;

	STABLE = '0';
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

	if( SpanTreeTbl0.RootPort != 0 ) {
		for( j = 1, tmpSwPort = PortList;
		     //tmpSwPort != NULL, j <= num_port;
		     tmpSwPort != NULL && j <= (int)num_port;
		     tmpSwPort = tmpSwPort->nextport, j++ ) {
			if( tmpSwPort->portNum == SpanTreeTbl0.RootPort )                                                
				break;
   	         }

		ep_cpy = pkt_copy(ep);
		put(ep_cpy, tmpSwPort->port);
       
	}

        freePacket(ep);

	return 1;
}

int sw::SendSpanTreePacket(Event_ *eventp){

	Packet			*pkt_;
        SpanINFO		*spaninfo;
        struct ether_header	*eh;
        Event_			*ep;
        Event_			*ep2;
        int			*OpenPort;
        int			i;
	ePacket_		*pkt_2;
	SwPort			*tmpSwPort;


	/* For the Topology Change */
	if( TCNFlag == '1' ) {
		if( SpanTreeTbl0.RootPort != 0 )
			SpanTreeChange();
		TCAFlag = '1';
	}
	else
		TCAFlag = '0';

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


	for( i = 1, tmpSwPort = PortList;
		     //tmpSwPort != NULL, i <= num_port;
		     tmpSwPort != NULL && i <= (int)num_port;
		     tmpSwPort = tmpSwPort->nextport, i++ ) {

		if( SpanTreeTbl0.RootID == (int)get_nid() ) {                           
			pkt_2 = pkt_copy(ep);
			put(pkt_2, tmpSwPort->port);

			if( tmpSwPort->PortState ==0 ) {
				BASE_OBJTYPE(type);                  
				type = POINTER_TO_MEMBER(sw, OpenSpanTreePort);

				ep2 = createEvent();
				OpenPort = (int *)malloc(sizeof(int));
				*OpenPort = tmpSwPort->portNum;
				setObjEvent(ep2, GetNodeCurrentTime(get_nid())
					+ForwardDelayTick,0,this,type,OpenPort);
			}
		}
		else if( tmpSwPort->RootID == 0 &&
			 tmpSwPort->Cost == 0 &&
			 tmpSwPort->Bridge == 0 ) {
				pkt_2 = pkt_copy(ep);
				put(pkt_2, tmpSwPort->port);               
                  	
				if( tmpSwPort->PortState == 0 ) {
					BASE_OBJTYPE(type);
					type = POINTER_TO_MEMBER(sw,
							OpenSpanTreePort);
					ep2 = createEvent();
					OpenPort = (int *)malloc(sizeof(int));
					*OpenPort = tmpSwPort->portNum;
                      		
					setObjEvent(ep2,GetNodeCurrentTime(get_nid())
						+ForwardDelayTick,0,this,type,OpenPort);
				}
		}
             	else if( SpanTreeTbl0.RootPort != tmpSwPort->portNum ) {
              
			if( (SpanTreeTbl0.RootID < tmpSwPort->RootID ) ||
			    ( (SpanTreeTbl0.RootID == tmpSwPort->RootID ) 
			      && (SpanTreeTbl0.Cost < tmpSwPort->Cost) ) ||
			    ( (SpanTreeTbl0.RootID == tmpSwPort->RootID )
			      && (SpanTreeTbl0.Cost == tmpSwPort->Cost)
			      && ((int)get_nid() < tmpSwPort->Bridge) ) ) {

				pkt_2 = pkt_copy(ep);
				put(pkt_2, tmpSwPort->port);

                      		if( tmpSwPort->PortState == 0 ) {
					BASE_OBJTYPE(type);
					type = POINTER_TO_MEMBER(sw, OpenSpanTreePort);
					ep2 = createEvent();
					OpenPort = (int *)malloc(sizeof(int));
					*OpenPort = tmpSwPort->portNum;
					setObjEvent(ep2, GetNodeCurrentTime(get_nid())
						+ ForwardDelayTick, 0, this, type, OpenPort);
				}

			}else if( tmpSwPort->PortState == 1 ) {
					tmpSwPort->PortState = 0;
					TCNFlag = '1';
					SpanTreeChange();
			}

             	}else if(SpanTreeTbl0.RootPort == tmpSwPort->portNum) {
                 			
			if( tmpSwPort->PortState == 0 ) {
				BASE_OBJTYPE(type);
				type = POINTER_TO_MEMBER(sw, OpenSpanTreePort);

				ep2 = createEvent();
				OpenPort = (int *)malloc(sizeof(int));
				*OpenPort = tmpSwPort->portNum;
				setObjEvent(ep2, GetNodeCurrentTime(get_nid())
					+ForwardDelayTick, 0, this, type, OpenPort);
			}
		}
	}

	if( SpanTreeTbl0.RootPort == 0 ) {
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


int sw::GetSpanTreeChange(Packet *pkt_, MBinder *frm,
				SwPort *ThisPort, int frmNum) {
	
	struct TopChangeINFO	*topchangeinfo;

	topchangeinfo = (struct TopChangeINFO *)pkt_->pkt_get();
	TCNFlag = '1';
	
	if( SpanTreeTbl0.RootPort == 0 ) {
		flushSwTbl();
		FlushSwTblFlag = '1';
		TCNFlag = '1';
	}
  		
	return 0;
}

int sw::GetSpanTreePacket(Packet *pkt_, MBinder *frm, 
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

	if( SpanTreeTbl0.RootPort == ThisPort->portNum ) {
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
 	    	
		if(ThisPort->portNum == SpanTreeTbl0.RootPort) {
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
			
			SpanTreeTbl0.RootPort = frmportNum;
			SpanTreeTbl0.RootID = ThisPort->RootID;
			SpanTreeTbl0.Cost = ThisPort->Cost+1;
			SpanTreeTbl0.Bridge = ThisPort->Bridge;
		}
	}

	return 0;
}

int sw::OpenSpanTreePort(Event_ *ep) {

	int		*OpenPort;
	int 		NoDataPort = 0;
	int 		HighOrderPort = 0;
	int		i;
	SwPort		*tmpSwPort;
	MBinder		*frmport;

	OpenPort = (int *)ep->DataInfo_;
	frmport = 0;
	
	for( i=1, tmpSwPort = PortList;
	     //tmpSwPort != NULL, i <= num_port;
	     tmpSwPort != NULL && i <= (int)num_port;
	     tmpSwPort = tmpSwPort->nextport, i++ ) {
		if( tmpSwPort->portNum == *OpenPort )
			break;
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

 	
	if( SpanTreeTbl0.RootPort == *OpenPort ||
	    SpanTreeTbl0.RootID == (int)get_nid() ||
	    NoDataPort == 1 || 
	    HighOrderPort == 1 ) {

		if( tmpSwPort->PortState == 0 ) {
			tmpSwPort->PortState = 1;
			TCNFlag = '1';
			SpanTreeChange();
		}
	}
 		
	freeEvent(ep);
	return 1;
}

int sw::CheckTimeOut() {

	int		RootPortDeath=0,i;
	SwPort		*tmpSwPort; 
    	

	if( STABLEage < MaxAge )
		STABLEage++;

	if( (STABLE == '0') && (STABLEage >= MaxAge) )
		STABLE = '1';

    	
	for( i = 1, tmpSwPort = PortList;
	     //tmpSwPort != NULL, i <= num_port;
	     tmpSwPort != NULL && i <= (int)num_port;
	     tmpSwPort = tmpSwPort->nextport, i++ ) {

		if( tmpSwPort->Age < MaxAge ) {
			tmpSwPort->Age++;

			if( tmpSwPort->Age >= MaxAge ) {
				if( tmpSwPort->PortState == 0 ) {
					tmpSwPort->PortState = 1;
					TCNFlag = '1';
					SpanTreeChange();
				}
  	     			
				tmpSwPort->RootID = 0;
				tmpSwPort->Cost = 0;
				tmpSwPort->Bridge = 0;
				tmpSwPort->Age = MaxAge;
				tmpSwPort->SwSignal = 0; 

				if( SpanTreeTbl0.RootPort == tmpSwPort->portNum )
					RootPortDeath = 1;
			}
		} 
	}

	if( RootPortDeath == 1 ) {
		SpanTreeTbl0.RootID = get_nid();
		SpanTreeTbl0.Cost = 0;
		SpanTreeTbl0.Bridge = get_nid();
		SpanTreeTbl0.RootPort = 0;
	}          

	return 1;
}
