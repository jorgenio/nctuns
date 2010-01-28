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

#include <fcntl.h>
#include <math.h>
#include <stdlib.h>
#include <limits.h>
#include <assert.h>
#include <packet.h>
#include <nctuns_api.h>
#include <sys/stat.h>
#include <phy/awphy.h>
#include <phy/cm.h>
#include <unistd.h>
#include <con_list.h>
#include <wphyInfo.h>
#include <misc/obs/obstacle.h>

#include <mbinder.h>
#include <regcom.h>

#ifdef LINUX
#include <sys/ioctl.h>
#include <if_tun.h>
#endif

#define DEBUG_ANTENNA 0

/* advanced wphy module */
MODULE_GENERATOR(awphy);

extern SLIST_HEAD(headOfLink, con_list)          headOfWireless_;
extern RegTable					 RegTable_;

awphy::awphy(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	/* disable all flow control */
	r_flowctl = DISABLED;
	s_flowctl = DISABLED;

	/* assign propagation model and
	 * modulation model
	 */

	/* bind variable */
	vBind("Bw", &bw_Mbps);
	vBind("freq", &freq_);
	vBind("channel", &channel_);
	vBind("TransPower",&TransPower);
	vBind("Noise", &Noise);
	vBind("CSThresh",&CSThresh_);
	vBind("Gain", &Gain);

	vBind("BeamWidth", &beamwidth);
	vBind("PointingDirection", &pointingDirection);
	vBind("AngularSpeed", &angularSpeed);

	vBind("log", &_log);
	vBind("logInterval", &_logInterval);
	vBind("inOpt", &_inOpt);
	vBind("outOpt", &_outOpt);
	vBind("inoutOpt", &_inoutOpt);

	vBind("logInFileName", &logInFileName);
	vBind("logOutFileName", &logOutFileName);
	vBind("logInOutFileName", &logInOutFileName);

	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);

	/* register variable */
	REG_VAR("CSThresh", &CSThresh_);
        REG_VAR("BW", &bw_);
	REG_VAR("CHANNEL", &channel_);
	REG_VAR("CPThresh", &CPThresh_); // used in mac-802_11-dcf.cc

	REG_VAR("FREQ", &freq_);
	REG_VAR("pointingDirection", &pointingDirection);
	REG_VAR("angularSpeed", &angularSpeed);
	REG_VAR("beamwidth", &beamwidth);

 	/* initial variable */
	CSThresh_ = -87.570857;		// Carrier Sense threshold (dbm)
	freq_ = 2400;		// freqeuency (MHz)
	channel_ = 1;		// channel number 
	TransPower = 15.0;	// dbm
	bw_Mbps = 11;
	BER = 0.0;
	Noise =  10.0;
	Gain = 1.0;
	CPThresh_ = 10.0;

	// Omnidirectional by default
	beamwidth = 360;
	pointingDirection = 0;
	angularSpeed = 0;

	LinkFailFlag = 0;

	inKb = outKb = inoutKb = 0;
	_in = _out = _inout = 0;

	_log = 0;
	_logInterval = 1;
	_inOpt = 0;
	_outOpt = 0;
	_inoutOpt = 0;

	logInFileName = NULL;
	logOutFileName = NULL;
	logInOutFileName = NULL;	
}

awphy::~awphy() {

}

int awphy::init() {
	NslObject::init();

	/* set bandwidth */
	bw_ = bw_Mbps * 1000000.0;
	TransPower = pow(10, TransPower / 10) * 1e-3; // watt

	char	*FILEPATH;
	if ( (_log)&&(!strcasecmp(_log, "on")) ) {
		if ( (_inOpt)&&(!strcasecmp(_inOpt, "on")) ) {
			_in = 1;
			FILEPATH = (char *)malloc(strlen(GetConfigFileDir())
					+ strlen(logInFileName) + 1);
			sprintf(FILEPATH,"%s%s", GetConfigFileDir(), 
					logInFileName);			
			logInFile = fopen(FILEPATH,"w+");
			assert(logInFile);
			free(FILEPATH);		
		}

		if ( (_outOpt)&&(!strcasecmp(_outOpt, "on")) ) {
			_out = 1;
			FILEPATH = (char *)malloc(strlen(GetConfigFileDir())
					+ strlen(logOutFileName) + 1);
			sprintf(FILEPATH,"%s%s", GetConfigFileDir(), 
					logOutFileName);			
			logOutFile = fopen(FILEPATH,"w+");
			assert(logOutFile);
			free(FILEPATH);
		}

		if ( (_inoutOpt)&&(!strcasecmp(_inoutOpt, "on")) ) {
			_inout = 1;
			FILEPATH = (char *)malloc(strlen(GetConfigFileDir())
					+ strlen(logInOutFileName) + 1);
			sprintf(FILEPATH,"%s%s", GetConfigFileDir(), 
					logInOutFileName);
			logInOutFile = fopen(FILEPATH,"w+");
			assert(logInOutFile);
			free(FILEPATH);
		}
		
		/* convert log interval to tick */
		MILLI_TO_TICK(_logIntervalTick, _logInterval);

		/* set timer to log information periodically */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(awphy, log);
		logTimer.setCallOutObj(this, type);
		logTimer.start(_logIntervalTick, _logIntervalTick);
	}

	char		line[128];
	if( _linkfail && !strcmp(_linkfail, "on") ) {

		tunfd_ = GET_REG_VAR1(get_portls(), "TUNFD", int *);
		/*
		 * if tunfd_ equal to NULL, that means
		 * this is a layer-1 or layer-2 device.
		 * NO TYNFD is supposed to be got.
		 */

		FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
						strlen(linkfailFileName) + 1);
		sprintf(FILEPATH,"%s%s", GetConfigFileDir(), linkfailFileName);

		linkfailFile = fopen(FILEPATH,"r");

		if( linkfailFile == NULL ) {
			printf("Warning : Can't read file %s\n", FILEPATH);
		}
		else {
			double		StartTime, StopTime;
			Event_		*start_ep;
			Event_		*stop_ep;
			u_int64_t	StartTimeTick, StopTimeTick;
			BASE_OBJTYPE(typeStart);
			BASE_OBJTYPE(typeStop);

			typeStart = POINTER_TO_MEMBER(awphy, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(awphy, TurnOffLinkFailFlag);

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

		free(FILEPATH);
	}

	return(1);
}

int awphy::send(ePacket_ *pkt)
{
	struct wphyInfo		*wphyinfo;
	struct con_list		*wi;
	double			T_locX, T_locY, T_locZ;
	double			currAzimuthAngle = 0;
	double			recvPower = 0.0;
	double			*rxCSThresh_; // carrier sense threshold of receiver
	cm			*cmobj;
	Packet			*p; 
	ePacket_		*ep;

	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	if ( LinkFailFlag > 0 ) {
		freePacket(pkt);
		return(1);
	}

	wphyinfo = (struct wphyInfo *) malloc(sizeof(struct wphyInfo));
	assert(wphyinfo);

	/* add up the outgoing packet's length (throughput) */
	outKb += (double)p->pkt_getlen() / 1000.0;
	inoutKb += (double)p->pkt_getlen() / 1000.0;

	/* get my location */
	assert(GetNodeLoc(get_nid(), T_locX, T_locY, T_locZ) > 0);

	/* hwchu */
	if (beamwidth != 360) {
		currAzimuthAngle = getAntennaDirection(pointingDirection, angularSpeed);

#if DEBUG_ANTENNA == 1
		printf("(%d) At time %lld: degree = %lf\n",
			get_nid(), GetCurrentTime(), currAzimuthAngle);
#endif
	}

	/* fill advanced wphy information
	 * eg, sender location, frequency ...... etc,.
	 */
	wphyinfo->bw_	   = bw_;
	wphyinfo->BER	   = BER;
	wphyinfo->channel_ = channel_; 
	wphyinfo->TxPr_    = TransPower;  // watt
	wphyinfo->RxPr_    = 0.0;
	wphyinfo->nid      = get_nid();  
	wphyinfo->pid      = get_port();  
  	wphyinfo->srcX_    = T_locX;
  	wphyinfo->srcY_    = T_locY;
  	wphyinfo->srcZ_    = T_locZ; 
	wphyinfo->Pr_	   = 0.0;
	wphyinfo->currAzimuthAngle_ = currAzimuthAngle;

	/* attach wphyinfo to packet */
	p->pkt_addinfo("WPHY", (char *)wphyinfo, sizeof(struct wphyInfo));
	free(wphyinfo);
	
	/* 
	 * Index of using pre-computed received power.
	 * We'll examine this index in cm module
	 */
	int *uPrecompute = new(int);
	*uPrecompute = 1;
	p->pkt_addinfo("uPre", (char *)uPrecompute, sizeof(int));

	p->pkt_setflow(PF_SEND);
	SLIST_FOREACH(wi, &headOfWireless_, nextLoc)
	{
		struct wphyInfo		*tmpWphyinfo;
		char cm_obj_name[100];
		/* if myself, just skip it */
		if( get_nid() == wi->obj->get_nid() &&
		    get_port() == wi->obj->get_port() )
		{
			continue;
		}

		/* Get receiver's CSThresh and use it to compare the pre-computed power */
		rxCSThresh_ = (double *)get_regvar(wi->obj->get_nid(), 
						wi->obj->get_port(), 
						"CSThresh"); 
		if(rxCSThresh_ == NULL){
			/* The peer is not using awphy module. */
			continue;
		}

		/* call receiver's computePr function in channel model module */
		sprintf(cm_obj_name, "Node%d_CM_LINK_%d", wi->obj->get_nid(), wi->obj->get_port());
		cmobj = (cm*)RegTable_.lookup_Instance(wi->obj->get_nid(), cm_obj_name);
		if( !cmobj ){
			// The peer is not using the cm module.
			//printf("Node %d:: No CM this Instance!!\n\n", wi->obj->get_nid());
			continue;
		}

		/* 
		 * Pre-compute the received power when receiver receives this packet:
		 *   An optimization to reduce the number 
		 *   of unnecessary packet duplication and transmission 
		 */
		tmpWphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
		recvPower = cmobj->computePr(tmpWphyinfo);
//#define AWPHY_DEBUG
#ifdef AWPHY_DEBUG
		printf("[AWPHY %s]: rxNid %d, recvPower %f\n", __func__, wi->obj->get_nid(), recvPower);
#endif

		if(recvPower > *rxCSThresh_){
			/* duplicate packet */
			ep = pkt_copy(pkt);

			/* send this packet to the down layer */

			ep->timeStamp_ = 0;
			ep->perio_ = 0;
			/* set call out object */
			ep->calloutObj_ = wi->obj;
			ep->memfun_ = NULL;
			ep->func_ = NULL;
			NslObject::send(ep);
		} else {
			/*
			 * The predicted receive power is lower than the threshold,
			 * it is unnecessary to send this packet.
			 */
		}
	}

	/* Note: XXX
	 * 	Why should I do that???!!!!
	 *	If there are n mobile nodes, we should duplicate
	 * this packet (n-1) times. One of them is sender and we
	 * don't want to send a copy to it cause it is no need.
	 * After duplicating (n-1) packets, there is one more 
	 * packet, which is the original packet from upper module.
	 * We should free it!!!!!
	 *	If the upper module (maybe MAC module) want to
	 * temporarily hold out-going packet for retransmiting,
	 * it is the upper module's duty to duplicate another
	 * copy.
	 */
	freePacket(pkt); 
	return(1); 
}

int awphy::recv(ePacket_ *pkt)
{
	Packet			*p; 
	double			Pr; 
	struct wphyInfo		*wphyinfo;
 
	assert(pkt&&(p=(Packet *)pkt->DataInfo_));

	if ( LinkFailFlag > 0 ) {
		freePacket(pkt);
		return(1);
	}

	/* get awphy-information of incoming packet */
	wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
	if (!wphyinfo) {
		/* 
		 * if the awphy information of incoming packet
		 * is not "WPHY", we should drop it, and return
		 * 1 to inform lower module to successfully 
		 * accept packet.
		 */
		freePacket(pkt);
 		return(1); 
	}
 	
	/* 
	 * Check channel, if the channel of incoming packet is
	 * not equal to channel of receiver, we should drop it
	 * to simulate the phy can't listen this signal.
	 */
	if (wphyinfo->channel_ != channel_) {
		freePacket(pkt);
		return(1);
	}

	Pr = wphyinfo->Pr_;	/* hwchu: pre-computed in channel model (cm) module */

	/* add up the incoming packets' length (throughput) */
	inKb += (double)p->pkt_getlen() / 1000.0;
	inoutKb += (double)p->pkt_getlen() / 1000.0;

	/* set Modulation args */
	errorModel = Modulation::ModForBandwidth(wphyinfo->bw_/1000000);
	assert(errorModel);
	errorModel->Noise = Noise;
	
	/* packet error Model */
	if (errorModel->BitError(Pr,p->pkt_getlen() * 8) == 1){
		p->pkt_err_ = 1;  
	}
	delete errorModel;

	/*
	 * Bring receving power calculated in 
	 * AWPHY module to MAC module.
	 */
 	wphyinfo->RxPr_ = pow(10, Pr/10) * 1e-3;

	/* 
	 * Record the received signal strength indicator (RSSI)
	 * of the incoming packet.
	 */
        p->pkt_addinfo("RSSI", (char *)&(wphyinfo->RxPr_), 8);

	/* push this packet to upper module */
  	return(put(pkt, recvtarget_));

}
int awphy::command(int argc, const char *argv[]) {
	return(NslObject::command(argc, argv));
}

int awphy::log() {
	double	intervalTmp;

	inKb /= (_logInterval / 1000.0);
	outKb /= (_logInterval / 1000.0);
	inoutKb /= (_logInterval / 1000.0);

	intervalTmp = GetCurrentTime() * TICK / 1000000000.0;

	if ( _in ) {
		fprintf(logInFile, "%.3f\t%.3f\n", intervalTmp, inKb);
		inKb = 0;
	}

	if ( _out ) {
		fprintf(logOutFile, "%.3f\t%.3f\n", intervalTmp, outKb);
		outKb = 0;
	}

	if ( _inout ) {
		fprintf(logInOutFile, "%.3f\t%.3f\n", intervalTmp, inoutKb);
		inoutKb = 0;
	}

	return(1);
}

void awphy::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void awphy::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}
  
