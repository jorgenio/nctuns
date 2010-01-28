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
#include <assert.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <opt/ophy/ophy.h>
#include <string>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <con_list.h>
#include <phyInfo.h>
#include <maptable.h>
#include <timer.h>
#include <opt/Lheader.h>
#include <nctuns-dep/link.h>
#include <packet.h>
#include <mbinder.h>

extern RegTable                 	RegTable_;
extern SLIST_HEAD(headOfLink, con_list)        headOfWire_;

MODULE_GENERATOR(ophy);

ophy::ophy(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	r_flowctl = DISABLED;
	s_flowctl = DISABLED;  
	
	bw_Mbps = 10;
	PropDelay_micro = 0;
	PropDelay_tick = 10; // 10 ticks by default
	BER = 0.0;
	LinkFailFlag = 0;
	txState = rxState = false;
	ptr_log = 0;
	
	/* bind variable */
	vBind("Bw", &bw_Mbps); 
	vBind("BER", &BER); 
	vBind("PropDelay", &PropDelay_micro); 

	/* log the performance */
	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);
	
	vBind("log", &_log);
        vBind("logInterval", &logInterval);
        vBind("NumUniInPkt", &LogUniIn);
        vBind("NumUniOutPkt", &LogUniOut);
        vBind("NumUniInOutPkt", &LogUniInOut);
        vBind("NumCollision", &LogColl);
        vBind("NumDrop", &LogDrop);
        vBind("InThrput", &LogInThrput);
        vBind("OutThrput", &LogOutThrput);
        vBind("InOutThrput", &LogInOutThrput);
        vBind("UniInLogFile", &UniInLogFileName);
        vBind("UniOutLogFile", &UniOutLogFileName);
        vBind("UniInOutLogFile", &UniInOutLogFileName);
        vBind("CollLogFile", &CollLogFileName);
        vBind("DropLogFile", &DropLogFileName);
        vBind("InThrputLogFile", &InThrputLogFileName);
        vBind("OutThrputLogFile", &OutThrputLogFileName);
        vBind("InOutThrputLogFile", &InOutThrputLogFileName);
	
	/* register variable */
	REG_VAR("BW", &bw_);
	REG_VAR("PropDelay", &PropDelay_micro);
	REG_VAR("LINK_F", &LinkFailFlag);

	_log = NULL;
        NumUniIn = NumUniOut = NumUniInOut = 0;
        NumColl = NumDrop = 0;
        InThrput = OutThrput = InOutThrput = 0.0;
        logInterval = 1;
        LogUniIn = LogUniOut = LogUniInOut = 0;
        LogColl = LogDrop = 0;
        LogInThrput = LogOutThrput = LogInOutThrput = 0;
        UniInLogFileName = UniOutLogFileName = UniInOutLogFileName = NULL;
        CollLogFileName = DropLogFileName = NULL;
        InThrputLogFileName = OutThrputLogFileName = InOutThrputLogFileName = NULL;
}

ophy::~ophy() {
}

void ophy::RxHandler(){
	Packet *p;
	struct phyInfo          *phyinfo;

	assert(rxBuf&&(p=(Packet *)rxBuf->DataInfo_));
	if ( LinkFailFlag > 0 ) {
		rxState = false;
		freePacket(rxBuf);
		return;
	}
	
	/* see packet has bit error or not */
	phyinfo = (struct phyInfo *)p->pkt_getinfo("PHY");
	assert(phyinfo);
	if( BitError(phyinfo->BER, p->pkt_getlen()) == 1 ){
		p->pkt_err_ = 1;
		if ( ptr_log )
			relog(rxBuf, DropRX, DROP_BER);

		if( _log && !strcasecmp(_log, "on") )
			if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
                                NumDrop++;	
	}
	else{
		if ( ptr_log )
			relog(rxBuf, SuccessRX, DROP_NONE);

		if(_log && !strcasecmp(_log, "on")){
			if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) )
	                        NumUniIn++;
	                if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
	                        NumUniInOut++;
		        if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) )
	                        InThrput += (double)p->pkt_getlen() / 1000.0;
	                if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
	                        InOutThrput += (double)p->pkt_getlen() / 1000.0;
		}
	}
	
	//modify at final, do not store & forward
	if(is_switch){
		char	*obschk = p->pkt_getinfo("OBS");
		if(obschk && (obschk[0] == 1)){
			//store & forward
			put(rxBuf, recvtarget_);
		}
		else{
			//not store & forward
			freePacket(rxBuf);
		}
	}
	else{
		put(rxBuf, recvtarget_);
	}
		
	rxState = false;
}

int ophy::recv(ePacket_ *pkt) {	
	u_int64_t               txtime;
	Packet			*p = (Packet *)pkt->DataInfo_;
	char			*obschk;
	
	obschk = p->pkt_getinfo("OBS");
	
	//simulate transmission
	if(!rxState){
		rxState = true;
		BASE_OBJTYPE(type);
		txtime = scheduleTime(pkt);
		type = POINTER_TO_MEMBER(ophy, RxHandler);
		rxTimer.setCallOutObj(this, type);
		rxTimer.start(txtime, 0);
		
		if ( ptr_log )
			rslog(pkt);
		
		//modify at final, do not store & forward
		if(is_switch){
			if(obschk && (obschk[0] == 1)){
				//store & forward
				rxBuf = pkt;
				return 1;
			}
			else if(obschk && (obschk[0] == 3)){
				//store & forward
				rxBuf = pkt;
				return 1;
			}
			else{
				if ( LinkFailFlag <= 0 ) {
					//not store & forward
					rxBuf = pkt_copy(pkt);
					return put(pkt, recvtarget_);
				}
				else
					rxBuf = pkt_copy(pkt);
					return put(pkt, recvtarget_); //modify by chenyuan 
					//return 1;
			}
		}
		else{
			rxBuf = pkt;
			return 1;
		}
	}

	return -1;
}

void ophy::TxHandler(){
	
	txState = false;
	Packet *p = (Packet *)txBuf->DataInfo_;

	
	if ( ptr_log ) {
		selog(txBuf, SuccessTX, DROP_NONE, 0);
	}

	if(_log && !strcasecmp(_log, "on")){
		if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) )
                	NumUniOut++;
                if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
                        NumUniInOut++;
                if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
                        OutThrput += (double)p->pkt_getlen() / 1000.0;
                if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
                        InOutThrput += (double)p->pkt_getlen() / 1000.0;
	}
	//this line can't put in the if(ptr_log)
	freePacket(txBuf);
}


int ophy::send(ePacket_ *pkt) {

	Packet			*p;
	struct con_list         *cl;
	struct phyInfo		*phyinfo;
	u_int64_t		txtime;

	p=(Packet *)pkt->DataInfo_;
	
	//simulate transmission
	if(!txState){
                txState = true;
                BASE_OBJTYPE(type);
                txtime = scheduleTime(pkt);
                type = POINTER_TO_MEMBER(ophy, TxHandler);
                txTimer.setCallOutObj(this, type);
                txTimer.start(txtime, 0);
                
                if ( ptr_log ) {
			sslog(pkt, 0, txtime);
		}

		txBuf = pkt_copy(pkt);
        }
	//log contention drop here
	else{
		//do drop(collision) log here
		if(_log && !strcmp(_log, "on"))
			LogColl++;
		/*if(ptr_log){
			sslog(pkt, 0, txtime);
			selog(pkt, DropTX, DROP_COLL, 0);
		}*/
		//freePacket(pkt);
		return -1;
	}
	
	assert(pkt&&(p=(Packet *)pkt->DataInfo_));
		
	if ( LinkFailFlag > 0 ) {
		freePacket(pkt);
		return(1);
	}

	p->pkt_setflow(PF_RECV);
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(ophy, get);

	phyinfo = (struct phyInfo *)malloc(sizeof(struct phyInfo));
	assert(phyinfo);
	phyinfo->TxBW = bw_;
	phyinfo->BER = BER;

	p->pkt_addinfo("PHY", (const char *)phyinfo, sizeof(struct phyInfo));
	free(phyinfo);
        SLIST_FOREACH(cl, &headOfWire_, nextLoc) {
		char		mark = 0;
		struct plist	*clp = cl->obj->get_portls();
		struct plist	*obp = get_portls();

		while(clp && obp){
			if(clp->pid != obp->pid){
				mark = 1;
				break;
			}
			else{
				clp = clp->next;
				obp = obp->next;
			}
		}

		if(clp || obp)
			mark = 1;
		
                if ((cl->obj->get_nid() == get_nid())&& !mark){	
			/* simulate propagation delay here */
			setObjEvent(pkt, GetCurrentTime() + PropDelay_tick, 0, 
				cl->obj->sendtarget_->bindModule(), type, 
				pkt->DataInfo_);
			return(1); 
		}
	}
	return -1;
}

u_int64_t ophy::scheduleTime(ePacket_ *pkt) {

	double                  time, dt;
	u_int64_t               retime;
        Packet                  *p;

        p = (Packet *)pkt->DataInfo_;
        time = ((p->pkt_getlen()*8.0)*(1000000/(bw_))) * (1000.0/TICK);
        retime = (u_int64_t)time;
        dt = (time - retime) * 10.0;
        if(dt >= 5)     
		retime++;
	 return(retime);
}


int ophy::init() {

	NslObject::init();
	const char *name = getTypeName(this);

	/* set random seed */

	/* set bandwidth */
	bw_ = bw_Mbps * 1000000.0;
	
/*
	if ((strcmp(name, "Switch") == 0) ||
	    (strcmp(name, "OptSwitch") == 0) ||
	    (strcmp(name, "ObsOptSwitch") == 0)) {
*/
	if ((strcmp(name, "SWITCH") == 0) ||
	   (strcmp(name, "OPT_SWITCH") == 0) ||
	   (strcmp(name, "OBS_OPT_SWITCH") == 0)) {
		is_switch = true;
	} else {
		is_switch = false;
	}

/*
	if ((get_type() == 4 )){
		is_switch = true;
	} 
	else if ((get_type() == 12 )){
		is_switch = true;
	} 
	else if ((get_type() == 13)){
		is_switch = true;
	} 
	else{
		is_switch = false;
	}
*/

	/* set propagation delay */
	if(PropDelay_micro != 0)
		 MICRO_TO_TICK(PropDelay_tick, PropDelay_micro);
		 
	ConnNodeID_ = sendtarget_->bindModule()->sendtarget_->bindModule()->get_nid();

	char		line[128];
	
	// set up link failure
	if( _linkfail && !strcmp(_linkfail, "on") ) {
		char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(linkfailFileName)+1);
		sprintf(FILEPATH,"%s%s", GetConfigFileDir(), linkfailFileName);

		linkfailFile = fopen(FILEPATH,"r");

		if( linkfailFile == NULL ) {
			printf("Warning : Can't read file %s\n", FILEPATH);
			free(FILEPATH); //modify 04/20 by chenyuan
		}
		else {
			free(FILEPATH);
			double		StartTime, StopTime;
			Event_		*start_ep;
			Event_		*stop_ep;
			u_int64_t	StartTimeTick, StopTimeTick;
			BASE_OBJTYPE(typeStart);
			BASE_OBJTYPE(typeStop);

			typeStart = POINTER_TO_MEMBER(ophy, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(ophy, TurnOffLinkFailFlag);

			while( !feof(linkfailFile) ) {
				line[0] = '\0';
				fgets(line, 127, linkfailFile);
				if ((line[0]=='\0')||(line[0]=='#'))
					continue;
				if ( 2 == sscanf(line, "%lf %lf",
					&StartTime, &StopTime) ) {

					if( StartTime >= StopTime )
						continue;
					/* handle start evnet */
					SEC_TO_TICK(StartTimeTick, StartTime);
					start_ep =  createEvent();
					setObjEvent(start_ep,
						    StartTimeTick,
						    0,this,typeStart,
						    (void *)NULL);

					/* handle stop event */
					SEC_TO_TICK(StopTimeTick, StopTime);
					stop_ep =  createEvent();
					setObjEvent(stop_ep,
						    StopTimeTick,
						    0,this,typeStop,
						    (void *)NULL);
				}
			}
			fclose(linkfailFile);
		}
	}
	
	//set up packet trace
	if ( OphyLogFlag && !strcasecmp(OphyLogFlag, "on") ) {
		if ( !ptrlogFileOpenFlag ) {
			ptrlogFileOpenFlag = true;

 			char	*ptrFile;
			if( ptrlogFileName ) {
				ptrFile = (char *)malloc(strlen(GetConfigFileDir()) + strlen(ptrlogFileName)+1);
				sprintf(ptrFile,"%s%s", GetConfigFileDir(), ptrlogFileName);
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}
			else {
				ptrFile = (char *)malloc(strlen(GetScriptName())+5);
				sprintf(ptrFile, "%s.ptr", GetScriptName());
				fptr = fopen(ptrFile, "w+");
				free(ptrFile);
			}

			if( fptr == NULL ) {
				printf("Error : Can't create file %s\n",ptrFile);
				exit(-1);
			}
	
			/* triggler the heapHandle event */
			Event_ *heapHandle = createEvent();
			u_int64_t chkInt;
			MILLI_TO_TICK(chkInt, 100);
			chkInt += GetCurrentTime();
			setEventTimeStamp(heapHandle, chkInt, 0);

			/* heapHandle is handled by the function
			 * in logHeap.cc : DequeueLogFromHeap
			 */
			int (*__fun)(Event_ *) = (int (*)(Event_ *))&DequeueLogFromHeap;
			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}

		ptr_log = 1;

		/* every a interval, flush the logchktbl */
		u_int64_t life;
		MILLI_TO_TICK(life,100);
		int execint = life / 6;

		logTbl = new logChkTbl(life);
			
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(ophy, logFlush);
		ptrlogTimer.setCallOutObj(this, type); 
		ptrlogTimer.start(execint, execint);
	}
		
	/* Optional Log*/
	if ( _log && (!strcasecmp(_log, "on")) ) {
		char    *FILEPATH;
                // * open log files 
                if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(UniInLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), UniInLogFileName);
                        UniInLogFILE = fopen(FILEPATH,"w+");
                        assert(UniInLogFILE);
                        free(FILEPATH);
                }
		
                if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(UniOutLogFileName)+1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(), UniOutLogFileName);
                        UniOutLogFILE = fopen(FILEPATH,"w+");
                        assert(UniOutLogFILE);
                        free(FILEPATH);
                }
                
		if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(UniInOutLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), UniInOutLogFileName);
                        UniInOutLogFILE = fopen(FILEPATH,"w+");
                        assert(UniInOutLogFILE);
                        free(FILEPATH);
                }
		
		if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(CollLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), CollLogFileName);
                        CollLogFILE = fopen(FILEPATH,"w+");
                        assert(CollLogFILE);
                        free(FILEPATH);
                }

                if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(DropLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), DropLogFileName);
                        DropLogFILE = fopen(FILEPATH,"w+");
                        assert(DropLogFILE);
                        free(FILEPATH);
                }

                if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(InThrputLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), InThrputLogFileName);
                        InThrputLogFILE = fopen(FILEPATH,"w+");
                        assert(InThrputLogFILE);
                        free(FILEPATH);
                }

		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(OutThrputLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), OutThrputLogFileName);
                        OutThrputLogFILE = fopen(FILEPATH,"w+");
                        assert(OutThrputLogFILE);
                        free(FILEPATH);
                }

                if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) ) {
                        FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + strlen(InOutThrputLogFileName)+1);
                        sprintf(FILEPATH, "%s%s", GetConfigFileDir(), InOutThrputLogFileName);
                        InOutThrputLogFILE = fopen(FILEPATH,"w+");
                        assert(InOutThrputLogFILE);
                        free(FILEPATH);
                }
		
                // * convert log interval to tick */
		SEC_TO_TICK(logIntervalTick, logInterval);
                //* set timer to log information periodically */
                BASE_OBJTYPE(type);
                type = POINTER_TO_MEMBER(ophy, log);
                _logTimer.setCallOutObj(this, type);
                _logTimer.start(logIntervalTick, logIntervalTick);
        }
	return 1;  
}

int ophy::log(){	
        double          cur_time;
	
        cur_time = (double)(GetCurrentTime() * TICK) / 1000000000.0;
        if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) ) {
                fprintf(UniInLogFILE, "%.3f\t%llu\n", cur_time, NumUniIn);
                fflush(UniInLogFILE);
                NumUniIn = 0;
        }

        if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) ) {
                fprintf(UniOutLogFILE, "%.3f\t%llu\n", cur_time, NumUniOut);
                fflush(UniOutLogFILE);
                NumUniOut = 0;
        }
	
        if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) ) {
                fprintf(UniInOutLogFILE, "%.3f\t%llu\n", cur_time, NumUniInOut);
                fflush(UniInOutLogFILE);
                NumUniInOut = 0;
        }

	if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
                fprintf(CollLogFILE, "%.3f\t%llu\n", cur_time, NumColl);
                fflush(CollLogFILE);
                NumColl = 0;
        }

        if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
                fprintf(DropLogFILE, "%.3f\t%llu\n", cur_time, NumDrop);
                fflush(DropLogFILE);
                NumDrop = 0;
        }

	InThrput /= logInterval;
        OutThrput /= logInterval;
        InOutThrput /= logInterval;
        if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) ) {
                fprintf(InThrputLogFILE, "%.3f\t%.3f\n", cur_time, InThrput);
                fflush(InThrputLogFILE);
                InThrput = 0;
        }

        if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) ) {
                fprintf(OutThrputLogFILE, "%.3f\t%.3f\n", cur_time, OutThrput);
                fflush(OutThrputLogFILE);
                OutThrput = 0;
        }

	if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) ) {
                fprintf(InOutThrputLogFILE, "%.3f\t%.3f\n", cur_time, InOutThrput);
                fflush(InOutThrputLogFILE);
                InOutThrput = 0;
        }
	return 1;
}

int ophy::BitError(double BER_, int plen) {
	double	x,PER;

	if (BER_ == 0.0)
		return(0);	// no bit error
	else {

		PER = plen * 8 * BER_; /* PER = 1-(1-BER)^n */
		if(PER >= 1.0)
			return(1);

		PER *= 1e9;

		x = random() % (1000000000);
  		if (x < PER)
			return(1);	// bit error
		else 
			return(0);	// no bit error  
	}

}

void ophy::TurnOnLinkFailFlag(Event_ *ep){
	ePacket_	*pkt;
	Packet		*p;
	struct plist	*pl = get_portls();
	struct plist	*l = pl;
	char		LS[3];

	LinkFailFlag ++;
	//send out packet to upper
	while(l->next->next != NULL)
		l = l->next;
	pkt = createPacket();
	p = (Packet *)pkt->DataInfo_;
	p->pkt_setflow(PF_RECV);
	LS[0] = 0;
	LS[1] = l->pid;
	LS[2] = l->next->pid;
	p->pkt_addinfo("LS", LS, 3);
	NslObject::recv(pkt);
}

void ophy::TurnOffLinkFailFlag(Event_ *ep){	
	ePacket_	*pkt;
	Packet		*p;
	struct plist	*pl = get_portls();
	struct plist	*l = pl;
	char		LS[3];

	LinkFailFlag --;
	//send out packet to upper
	while(l->next->next != NULL)
		l = l->next;
	pkt = createPacket();
	p = (Packet *)pkt->DataInfo_;
	p->pkt_setflow(PF_RECV);
	LS[0] = 1;
	LS[1] = l->pid;
	LS[2] = l->next->pid;
	p->pkt_addinfo("LS", LS, 3);
	NslObject::recv(pkt);
}

int ophy::logFlush()
{
	logTbl->flush();
	return 1;
}

int ophy::sslog(ePacket_ *pkt, int rtxCnt, u_int64_t txtime)
{
	/* insert this record to logchktbl */
        chkEntry *thisRcd = logTbl->insertRecord(pkt);

        Packet *pkt_;
        GET_PKT(pkt_, pkt);

	//char *__log, *__txtime;
	/* try to get log information: logchktbl pointer
	 * , if no, insert it. If having, get it for using.
	 */
	pkt_->pkt_addinfo("LOG", (char *)&thisRcd, 4);

	/* the same action with txtime */
	pkt_->pkt_addinfo("TXTIM", (char *)&txtime, 8);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

	/* prepare logEvent */
        logEvent *sslogEvent = new logEvent[1];
        sslogEvent->PROTO = PROTO_OPHY;
        sslogEvent->Time = GetCurrentTime();
	sslogEvent->Time += START;

	/* OPT Log */
        struct ophy_log *ssophylog = new struct ophy_log[1];
        ssophylog->PROTO = PROTO_OPHY;
        ssophylog->Time = ssophylog->sTime = ssTime = GetCurrentTime();
        ssophylog->NodeType = get_type();
        ssophylog->NodeID = get_nid();
        ssophylog->Event = StartTX;
        if ( __ip ) {
                ssophylog->IP_Src = mtbl_iptonid(ipSrc);
                ssophylog->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                ssophylog->IP_Src = 0;
                ssophylog->IP_Dst = 0;
        }
		
	ssophylog->PHY_Src = get_nid();
	ssophylog->PHY_Dst = ConnNodeID_;
	ssophylog->RetryCount = rtxCnt;
        ssophylog->FrameID = pkt_->pkt_getpid();
        ssophylog->FrameType = ptype(pkt_);
        ssophylog->FrameLen = pkt_->pkt_getlen();
        ssophylog->DropReason = DROP_NONE;
	ssophylog->Channel = get_port();

        sslogEvent->log = (char *)ssophylog;
        logQ.push(sslogEvent);
	return 1;
}

int ophy::selog(ePacket_ *pkt, u_char etype, u_char dreason, int rtxCnt){
	/* ending this record in logchktbl */
        //chkEntry *thisRcd = logTbl->endingRecord(pkt, etype, dreason, rtxCnt);
        logTbl->endingRecord(pkt, etype, dreason, rtxCnt);

        Packet *pkt_;
        GET_PKT(pkt_, pkt);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

	/* prepare logEvent */
        logEvent *selogEvent = new logEvent[1];
        selogEvent->PROTO = PROTO_OPHY;
        selogEvent->Time = GetCurrentTime();
	selogEvent->Time += ENDING;

	/* prepare ophy_log */
        struct ophy_log *seophylog = new struct ophy_log[1];
        seophylog->PROTO = PROTO_OPHY;
	seophylog->sTime = ssTime;
        seophylog->Time = GetCurrentTime();
        seophylog->NodeType = get_type();
        seophylog->NodeID = get_nid();
        seophylog->Event = etype;
        if ( __ip ) {
                seophylog->IP_Src = mtbl_iptonid(ipSrc);
                seophylog->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                seophylog->IP_Src = 0;
                seophylog->IP_Dst = 0;
        }

	seophylog->PHY_Src = get_nid();
	seophylog->PHY_Dst = ConnNodeID_;
        seophylog->RetryCount = rtxCnt;
        seophylog->FrameID = pkt_->pkt_getpid();
        seophylog->FrameType = ptype(pkt_);
        seophylog->FrameLen = pkt_->pkt_getlen();
        seophylog->DropReason = dreason;
	seophylog->Channel = get_port();

        selogEvent->log = (char *)seophylog;
        logQ.push(selogEvent);
	return 1;
}

int ophy::rslog(ePacket_ *pkt)
{
        Packet *pkt_;
        GET_PKT(pkt_, pkt);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

        logEvent *rslogEvent = new logEvent[1];
        rslogEvent->PROTO = PROTO_OPHY;
        rslogEvent->Time = GetCurrentTime();
	rslogEvent->Time += START;

        struct ophy_log *rsophylog = new struct ophy_log[1];
        rsophylog->PROTO = PROTO_OPHY;
        rsophylog->Time = rsophylog->sTime = rsTime = GetCurrentTime();
        rsophylog->NodeType = get_type();
        rsophylog->NodeID = get_nid();
        rsophylog->Event = StartRX;
        if ( __ip ) {
                rsophylog->IP_Src = mtbl_iptonid(ipSrc);
                rsophylog->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                rsophylog->IP_Src = 0;
                rsophylog->IP_Dst = 0;
        }

	rsophylog->PHY_Src = ConnNodeID_;
	rsophylog->PHY_Dst = get_nid();
        rsophylog->RetryCount = 0;
        rsophylog->FrameID = pkt_->pkt_getpid();
        rsophylog->FrameType = ptype(pkt_);
        rsophylog->FrameLen = pkt_->pkt_getlen();
        rsophylog->DropReason = DROP_NONE;
	rsophylog->Channel = get_port();

        rslogEvent->log = (char *)rsophylog;
        logQ.push(rslogEvent);
	return 1;
}

int ophy::relog(ePacket_ *pkt, u_char etype, u_char dreason){
        Packet *pkt_;
        GET_PKT(pkt_, pkt);

        char *__ip = pkt_->pkt_sget();
        u_long ipSrc, ipDst;
        if ( __ip ) {
                IP_SRC(ipSrc, __ip);
                IP_DST(ipDst, __ip);
        }
        else {
                ipSrc = ipDst = 0;
        }

        logEvent *relogEvent = new logEvent[1];
        relogEvent->PROTO = PROTO_OPHY;
        relogEvent->Time = GetCurrentTime();
	relogEvent->Time += ENDING;

        struct ophy_log *reophylog = new struct ophy_log[1];
        reophylog->PROTO = PROTO_OPHY;
	reophylog->sTime = rsTime;
        reophylog->Time = GetCurrentTime();
        reophylog->NodeType = get_type();
        reophylog->NodeID = get_nid();
        reophylog->Event = etype;
        if ( __ip ) {
                reophylog->IP_Src = mtbl_iptonid(ipSrc);
                reophylog->IP_Dst = mtbl_iptonid(ipDst);
        }
        else {
                reophylog->IP_Src = 0;
                reophylog->IP_Dst = 0;
        }

	reophylog->PHY_Src = ConnNodeID_;
	reophylog->PHY_Dst = get_nid();
        reophylog->RetryCount = 0;
        reophylog->FrameID = pkt_->pkt_getpid();
        reophylog->FrameType = ptype(pkt_);
        reophylog->FrameLen = pkt_->pkt_getlen();
        reophylog->DropReason = dreason;
	reophylog->Channel = get_port();

        relogEvent->log = (char *)reophylog;
        logQ.push(relogEvent);
	return 1;
}

int ophy::ptype(Packet* p){
	char* typeOBS; 
	struct lheader *lh;

	typeOBS = p->pkt_getinfo("OBS");
	lh      = (struct lheader *)p->pkt_get();
	
	if(typeOBS && (typeOBS[0] == 1))
		return FRAMETYPE_OBS_CTRL_NORMAL;
	else if(typeOBS && (typeOBS[0] == 2))
		return FRAMETYPE_OBS_DATA;
	else if(typeOBS && (typeOBS[0] == 3))
		return FRAMETYPE_OBS_CTRL_SWITCH;
	else if((lh->option & 0xf0) == 0)
		return FRAMETYPE_OPTICAL_DATA;
	else
		return FRAMETYPE_OPTICAL_LP;
}

