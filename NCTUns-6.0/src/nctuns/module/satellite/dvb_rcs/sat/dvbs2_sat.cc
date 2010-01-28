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
#include <event.h>
#include <object.h>
#include <nctuns_api.h>
#include <math.h>
#include <satellite/dvb_rcs/sat/dvbs2_sat.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/errormodel/saterrormodel.h>
#include <misc/log/logmacro.h>
#include <misc/log/logHeap.h>
#include <con_list.h>
#include <nctuns-dep/link.h>
#include <if_tun.h>
#include <nodetype.h>
#include <sys/ioctl.h>

extern SLIST_HEAD(headOfLink, con_list) headOfSat_;
extern char *DVBChannelCoding;

MODULE_GENERATOR(Dvbs2_sat);

/*
 * Constructor
 */
Dvbs2_sat::Dvbs2_sat(u_int32_t type, u_int32_t id, struct plist *pl,
		     const char *name)
:NslObject(type, id, pl, name)
{
	/*
	 * Parameters for SAT four antennas
	 */
	vBind("tx_fwd_Power", &_tx_fwd_Power);
	vBind("tx_fwd_SatAnteEfficient", &_tx_fwd_SatAnteEfficient);
	vBind("tx_fwd_SatAnteLength", &_tx_fwd_SatAnteLength);

	vBind("tx_ret_Power", &_tx_ret_Power);
	vBind("tx_ret_SatAnteEfficient", &_tx_ret_SatAnteEfficient);
	vBind("tx_ret_SatAnteLength", &_tx_ret_SatAnteLength);

	vBind("rx_fwd_SatAnteLength", &_rx_fwd_SatAnteLength);
	vBind("rx_fwd_SatAnteEfficient", &_rx_fwd_SatAnteEfficient);

	vBind("rx_ret_SatAnteLength", &_rx_ret_SatAnteLength);
	vBind("rx_ret_SatAnteEfficient", &_rx_ret_SatAnteEfficient);

	_ptrlog = 0;

	/*
	 * turn off link fail flag
	 */
	LinkFailFlag = 0;

	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);

}

/*
 * Destructor
 */
Dvbs2_sat::~Dvbs2_sat()
{
}


/*
 * module initialize
 */
int Dvbs2_sat::init()
{
        char            line[200];
        char            freqfilepath[200];

        /* open xxx.dvbrcs.freq file to read return down link freq */
        sprintf(freqfilepath,"%s.dvbrcs.freq", GetScriptName());
        if( (freqFile = fopen(freqfilepath,"r")) )
        {
                uint32_t        satnodeid;
                float           ReturnUpCentreFreq;
                float           ReturnDownCentreFreq;
                float           ForwardDownCentreFreq;

                while (fgets(line, 200, freqFile)) {
                        if ((sscanf(line, " SatNodeId: %u", &satnodeid)) && satnodeid == get_nid()) {
                                while (fgets(line, 200, freqFile)) {
                                        if((sscanf(line, " ForwardDownCentreFreq: %f", &ForwardDownCentreFreq))){
                                                _tx_fwd_Freq = ForwardDownCentreFreq;
                                                fgets(line, 200, freqFile);
                                        }
                                        if((sscanf(line, " ReturnUpCentreFreq: %f", &ReturnUpCentreFreq))){
                                                fgets(line, 200, freqFile);
                                                if((sscanf(line, " ReturnDownCentreFreq: %f", &ReturnDownCentreFreq))){
                                                        _diff_of_Freq = ReturnUpCentreFreq - ReturnDownCentreFreq;
                                                        fgets(line, 200, freqFile);
                                                }
                                        }
                                }
                                break;
                        }
                }
		fclose(freqFile);
        }
        else
        {
                printf("Warning : Can't read file %s\n", freqfilepath);
                assert(0);
        }


	/*
	 * check link fail flag, and to set up timer to start/stop to send
	 */

	if( _linkfail && !strcmp(_linkfail, "on") ) {
	
		tunfd_ = GET_REG_VAR1(get_portls(), "TUNFD", int *);
		/*
		 * if tunfd_ equal to NULL, that means
		 * this is a layer-1 or layer-2 device.
		 * NO TYNFD is supposed to be got.
		 */

		char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(linkfailFileName) + 1);
		sprintf(FILEPATH,"%s%s", GetConfigFileDir(), linkfailFileName);
		printf("FILEPATH: %s\n",FILEPATH);

		linkfailFile = fopen(FILEPATH,"r");
		free(FILEPATH);

		if( linkfailFile == NULL ) {
			printf("Warning : Can't read file %s\n", FILEPATH);
			assert(0);
		}
		else {
			double		StartTime, StopTime;
			Event_		*start_ep;
			Event_		*stop_ep;
			u_int64_t	StartTimeTick, StopTimeTick;
			BASE_OBJTYPE(typeStart);
			BASE_OBJTYPE(typeStop);

			typeStart = POINTER_TO_MEMBER(Dvbs2_sat, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(Dvbs2_sat, TurnOffLinkFailFlag);

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

	struct con_list *cl;

	/*
	 * find my Link module in global linked list headOfSat_
	 */
	SLIST_FOREACH(cl, &headOfSat_, nextLoc) {

		char mark = 0;
		struct plist *clp = cl->obj->get_portls();
		struct plist *obp = get_portls();

		/*
		 * compare ports list whether match, if match then this module is
		 * my Link module
		 */
		while (clp && obp) {
			if (clp->pid != obp->pid) {
				mark = 1;
				break;
			}
			else {
				clp = clp->next;
				obp = obp->next;
			}
		}
		if (clp || obp)
			mark = 1;

		if (cl->obj->get_nid() == get_nid() && !mark)
			break;
	}
	assert(cl);
	linkObj = cl->obj;

	/*
	 * Initial log flag
	 */
	if ( SatLogFlag && !strcasecmp(SatLogFlag, "on") ) {
		if ( !ptrlogFileOpenFlag ) {
			ptrlogFileOpenFlag = true;
			
			char	*ptrFile;
			if( ptrlogFileName ) {
				ptrFile = (char *)malloc(strlen
					(GetConfigFileDir())+strlen
					(ptrlogFileName) + 1);
				sprintf(ptrFile,"%s%s", GetConfigFileDir(),ptrlogFileName);
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
	
			Event_ *heapHandle = createEvent();
			u_int64_t time;
			MILLI_TO_TICK(time, 100);
			u_int64_t chkInt = GetCurrentTime() + time;
			setEventTimeStamp(heapHandle, chkInt, 0);

			int (*__fun)(Event_ *) = 
			(int (*)(Event_ *))&DequeueLogFromHeap;;
			setEventCallOutFunc(heapHandle, __fun, heapHandle);
			scheduleInsertEvent(heapHandle);
		}

		_ptrlog = 1;
	}

	return (NslObject::init());
}


/*
 * recv function, do nothing, just repeat packet to rcst
 */
int Dvbs2_sat::recv(ePacket_ * event)
{
	Dvb_pkt *pkt;
	assert(event && (pkt = (Dvb_pkt *)event->DataInfo_));

	if (DVBChannelCoding && !strcasecmp(DVBChannelCoding, "on")) {
		SatErrorModel error_obj;
		struct link_budget _recv_lbudget;
		struct link_info _recv_linfo;
		int num = 0;

		/* 
		 * Get feeder link_budget & link_info for the use of calculating BER
		 */
		if( pkt->pkt_gettype() == PKT_DVB_S2 ) {
			_recv_linfo = pkt->pkt_getfwdinfo()->linfo;
			_recv_lbudget = pkt->pkt_getfwdinfo()->lbudget;
			_recv_lbudget.st_dia = _rx_fwd_SatAnteLength;
			_recv_lbudget.st_e = _rx_fwd_SatAnteEfficient;
		}
		else if( pkt->pkt_gettype() == PKT_DVBRCS) {
			_recv_linfo = pkt->pkt_getretinfo()->linfo;
			_recv_lbudget = pkt->pkt_getretinfo()->lbudget;
			_recv_lbudget.st_dia = _rx_ret_SatAnteLength;
			_recv_lbudget.st_e = _rx_ret_SatAnteEfficient;
		}

		/*
		 * Check the rainfade is 'Default' or 'User_define',
		 * then cal BER
		 */
		if(!strcmp(_recv_lbudget.rainfade_option, "Default"))
			_bit_err_rate = error_obj.ber(UPLINK, _recv_lbudget);
		else
			_bit_err_rate = error_obj.ber(UPLINK, _recv_lbudget, _recv_linfo);

		/*
		 * call ber() with default rain_attenuation = 10
		 */
		if (pkt->pkt_getlen() > 0)
			num = calculate_bit_error(pkt->pkt_getdata(), pkt->pkt_getlen() * 8, _bit_err_rate);

	}

	/* Log "StartRX" event and "SuccessRX" event*/
	if(_ptrlog == 1){

		struct logEvent*	logep;
		u_int64_t		rx_time;
		int			phy_src;
		int			pktlen;
		dvbrcs_log* rsdvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
		dvbrcs_log* redvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));

		/* Get RET or FWD link info*/
		if( pkt->pkt_gettype() == PKT_DVB_S2 ){
			phy_src = pkt->pkt_getfwdinfo()->loginfo.phy_src;
			rx_time = pkt->pkt_getfwdinfo()->loginfo.tx_time;
			pktlen = pkt->pkt_getlen();
		}
		else if( pkt->pkt_gettype() == PKT_DVBRCS){
			phy_src = pkt->pkt_getretinfo()->loginfo.phy_src;
			rx_time = pkt->pkt_getretinfo()->loginfo.tx_time;
			pktlen = pkt->pkt_getlen();
		}

		LOG_SAT_DVBRCS(rsdvbrcs_log, GetCurrentTime(), GetCurrentTime(),
				get_type(), get_nid(), StartRX,phy_src,
				get_nid(), 0,pkt->pkt_gettype(), pktlen, 1,DROP_NONE);
		INSERT_TO_HEAP(logep, rsdvbrcs_log->PROTO, rsdvbrcs_log->Time+START, rsdvbrcs_log);

		LOG_SAT_DVBRCS(redvbrcs_log, GetCurrentTime()+rx_time , GetCurrentTime(),
				get_type(), get_nid(), SuccessRX,phy_src,
				get_nid(), 0,pkt->pkt_gettype(), pktlen, 1,DROP_NONE);
		INSERT_TO_HEAP(logep, redvbrcs_log->PROTO, redvbrcs_log->Time+ENDING, redvbrcs_log);
	}

	/*
	 * recv a pkt, then send to next module, like a hub 
	 */
	assert(event->DataInfo_);

	return send(event);
}


/*
 * send function, repeat packet to all rcst
 */
int Dvbs2_sat::send(ePacket_ * event)
{
	Dvb_pkt *pkt;
	struct con_list *cl;
	ePacket_ *ep;
	int sendcount = 0;

	assert(event && (pkt = (Dvb_pkt *)event->DataInfo_));
	pkt->pkt_setflag(FLOW_RECV);

	/*
	 * Add link_budget and log_info to RCST or GW for cal BER and log
	 */
	if( pkt->pkt_gettype() == PKT_DVB_S2 ) {
		pkt->pkt_getfwdinfo()->lbudget.tx_pwr = _tx_fwd_Power;
		pkt->pkt_getfwdinfo()->lbudget.freq = _tx_fwd_Freq;
		pkt->pkt_getfwdinfo()->lbudget.st_e = _tx_fwd_SatAnteEfficient;
		pkt->pkt_getfwdinfo()->lbudget.st_dia = _tx_fwd_SatAnteLength;
		pkt->pkt_getfwdinfo()->loginfo.phy_src = get_nid();
	}
	else if( pkt->pkt_gettype() == PKT_DVBRCS ) {
		pkt->pkt_getretinfo()->lbudget.tx_pwr = _tx_ret_Power;
		pkt->pkt_getretinfo()->lbudget.freq = _tx_ret_Freq;
		pkt->pkt_getretinfo()->lbudget.st_e = _tx_ret_SatAnteEfficient;
		pkt->pkt_getretinfo()->lbudget.st_dia = _tx_ret_SatAnteLength;
		pkt->pkt_getretinfo()->loginfo.phy_src = get_nid();
	}
	
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(Dvbs2_sat, get);

	/*
	 * for each connect Link module in linked list of my_link->hos_ 
	 */
	SLIST_FOREACH(cl, &(((Link *) linkObj)->hos_), nextLoc) {
		u_int64_t ticks = _prop_delay_time(cl->obj) + GetCurrentTime();

		/* Avoid sending FwdPkt to GW or RetPkt to RCST */
		typeTable 	*typetable = NULL;
		if( pkt->pkt_gettype() == PKT_DVB_S2 && cl->obj->get_type() != typetable->toType("DVB_RCS_RCST") )
			continue;
		if( pkt->pkt_gettype() == PKT_DVBRCS && cl->obj->get_type() != typetable->toType("DVB_RCS_GATEWAY") )
			continue;

		/* Log "StartTX" event and "SuccessTX" event*/
		if( _ptrlog == 1){

			struct logEvent* logep;
			u_int64_t	tx_time;
			dvbrcs_log* ssdvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
			dvbrcs_log* sedvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));

			/* Get RET or FWD link info*/
			if(pkt->pkt_gettype() == PKT_DVB_S2)
				tx_time = pkt->pkt_getfwdinfo()->loginfo.tx_time;
			else if( pkt->pkt_gettype() == PKT_DVBRCS )
				tx_time = pkt->pkt_getretinfo()->loginfo.tx_time;

			LOG_SAT_DVBRCS(ssdvbrcs_log, GetCurrentTime(), GetCurrentTime(),
					get_type(), get_nid(), StartTX, get_nid(),
					cl->obj->get_nid(), 0,pkt->pkt_gettype(), pkt->pkt_getlen(), 1,DROP_NONE);
			INSERT_TO_HEAP(logep, ssdvbrcs_log->PROTO, ssdvbrcs_log->Time+START, ssdvbrcs_log);

			LOG_SAT_DVBRCS(sedvbrcs_log, GetCurrentTime()+tx_time , GetCurrentTime(),
					get_type(), get_nid(), SuccessTX, get_nid(),
					cl->obj->get_nid(), 0,pkt->pkt_gettype(), pkt->pkt_getlen(), 1,DROP_NONE);
			INSERT_TO_HEAP(logep, sedvbrcs_log->PROTO, sedvbrcs_log->Time+ENDING, sedvbrcs_log);
		}

		/*
		 * copy a packet to every one 
		 */
		ep = dvb_pkt_copy(event);
		setObjEvent(ep, ticks, 0, cl->obj, type,
			    ep->DataInfo_);
		sendcount++;
	}

	dvb_freePacket(event);
	if (sendcount > 0)
		return (1);

	return (-1);
}


/*
 * calcaute propagation delay time
 */
u_int64_t Dvbs2_sat::_prop_delay_time(NslObject * obj)
{
	double R_locX, R_locY, R_locZ;
	double df;
	u_int64_t ticks;

	assert(obj);

	/*
	 * get my location 
	 */
	assert(GetNodeLoc(get_nid(), _loc_x, _loc_y, _loc_z) > 0);

	/*
	 * Simulate progagation delay, get connected node location 
	 */
	assert(GetNodeLoc(obj->get_nid(), R_locX, R_locY, R_locZ) > 0);

	/*
	 * count ProgDelay 
	 */
	df = sqrt((_loc_x - R_locX) * (_loc_x - R_locX) +
		  (_loc_y - R_locY) * (_loc_y - R_locY) +
		  (_loc_z - R_locZ) * (_loc_z - R_locZ));
	df = (df / SPEED_OF_LIGHT) * 1000000;	// us 

	/*
	 * unit transfer us to ticks 
	 */
	MICRO_TO_TICK(ticks, df);

	return ticks;
}

void Dvbs2_sat::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void Dvbs2_sat::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}

