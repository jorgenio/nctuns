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
#include <phy/cm.h>
#include <unistd.h>
#include <con_list.h>
#include <wphyInfo.h>
#include <cmInfo.h>
#include <misc/obs/obstacle.h>
#include <module/gprs/include/bss_message.h>

#include <scheduler.h>
#include <regcom.h>

#ifdef LINUX
#include <sys/ioctl.h>
#include <if_tun.h>
#endif

//#define DEBUG_ANTENNA 0
//#define CM_DEBUG 0

extern scheduler *scheduler_;
extern RegTable	 RegTable_;

/*
 * mshsu: 
 *    This is a channel model module. 
 *    It can provide many propagation channel models 
 *    to simulate the wireless propagation power loss 
 *    (path loss / shadowing / fading).
 */

/* channel model module */
MODULE_GENERATOR(cm);

cm::cm(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)
{
	/* disable all flow control */
	r_flowctl = DISABLED;
	s_flowctl = DISABLED;

	/* assign propagation model and
	 * modulation model
	 */

	/* bind variable */
	vBind("propChannelMode", &propChannelMode);
	vBind("PLModel", &PLModel);
	vBind("fadingModel", &fadingModel);
	vBind("empiricalModel", &empiricalModel);

	vBind("FadingVar",&FadingVar);
	vBind("RiceanK",&RiceanK);		// Ricean factor K (db)
	vBind("avgHB", &averageHB);		// average heights of surrounding buildings
	vBind("avgDist", &averageDist);		// average separation distance between the rows of buildings
	vBind("StreetWidth", &streetWidth);
	vBind("PLExp", &PLExp_);		// path-loss exponent
	vBind("std_db", &std_db_);		// shadowing standard deviation (dB)
	vBind("dist0", &dist0_);		// close-in reference distance (m)
	vBind("SystemLoss", &L_);		// system loss factor
	vBind("antennaHeight", &antennaHeight);	// antenna height (m)
	
	vBind("AGPOpt", &AGPOpt);		// user defined antenna gain pattern option (enable/disable)
	vBind("AGPFileName", &AGPFileName);	// user defined antenna gain pattern file name

	/* register variable */
	REG_VAR("PropModel", &PropModel);
	REG_VAR("FadingVar", &FadingVar);
	REG_VAR("antennaHeight", &antennaHeight);

 	/* initial variable */
	AGPOpt = 0;		// default: disable
	AGPFileName = NULL;

	propChannelMode = NULL;
	PLModel = NULL;
	fadingModel = NULL;
	empiricalModel = NULL;

	fadingOpt = 0;

	PropModel = 1;			// index of the propagation model
	FadingVar = 10.0;
	RiceanK = 10;
 	L_ = 1.0;			// system loss

	averageHB   = 10;  // meters
	averageDist = 80; // meters
	streetWidth = 30; // meters

	PLExp_	= 2.0;
	std_db_ = 4.0;
	dist0_	= 1.0;
	antennaHeight = 1.5;

	uGPRS_ = 0;
}

cm::~cm() {

}

int cm::init() {

	NslObject::init();

	freq = (double *)get_regvar(get_nid(),
			get_port(), 
			"FREQ");
	pointingDirection = (double *)get_regvar(get_nid(),
			get_port(), 
			"pointingDirection");
	angularSpeed = (double *)get_regvar(get_nid(),
			get_port(), 
			"angularSpeed");
	beamwidth = (int *)get_regvar(get_nid(), get_port(), "beamwidth");
	if( freq == NULL || pointingDirection == NULL || angularSpeed == NULL || beamwidth == NULL)
	{
		/* not register the antenna variables. */
		printf("[%s] node %d Warning:: not register antenna variables\n", __func__, get_nid());
		/* using default value */
		freq_ = 2400;
		pointingDirection_ = 0;
		angularSpeed_ = 0;
		beamwidth_ = 360;
	}
	else{
		freq_ = *freq;
		pointingDirection_ = *pointingDirection;
		angularSpeed_ = *angularSpeed;
		beamwidth_ = *beamwidth;
	}
	uGPRS = (int *)get_regvar(get_nid(), get_port(), "uGPRS");
	if( uGPRS == NULL )
	{
		/* upper layer is not the gprs radiolink module. */
		uGPRS_ = 0;
	}
	else {
		uGPRS_ = *uGPRS;
	}

	/* load user defined antenna gain pattern */
	if(AGPOpt != 0){
		parseAGPFile(AGPFileName, gainUser);
	}

	/* get the index of the propagation channel model */
	if(propChannelMode){ 
		if(!strcasecmp(propChannelMode, "theoretical")){
			if(PLModel != NULL){
				if(!strcasecmp(PLModel, "Free_Space")){
					PropModel = FREE_SPACE;
				}
				else if(!strcasecmp(PLModel, "Two_Ray_Ground")){
					PropModel = TWO_RAY_GROUND;
				}
				else if(!strcasecmp(PLModel, "Free_Space_and_Shadowing")){
					PropModel = FREE_SPACE_AND_SHADOWING;
				}
				else{
					printf("[%s] Warning:: No such channel model\n", __func__);
					PropModel = TWO_RAY_GROUND;
				}
			}
			else{
				printf("[%s] Warning:: Can't get path loss option\n", __func__);
				PropModel = TWO_RAY_GROUND;
			}
			if(fadingModel != NULL){
				if(!strcasecmp(fadingModel, "None")){
					/* not apply fading model */
					fadingOpt = 0;
				}
				else if(!strcasecmp(fadingModel, "Rayleigh")){
					fadingOpt = 1;
				}
				else if(!strcasecmp(fadingModel, "Ricean")){
					fadingOpt = 2;
				}
				else{
					printf("[%s] Warning:: No such fading model\n", __func__);
					fadingOpt = 0;
				}
			}
			else{
				fadingOpt = 0;
			}
		}
		else if(!strcasecmp(propChannelMode, "empirical")){
			if(empiricalModel){
				if(!strcasecmp(empiricalModel, "LEE_Microcell")){
					PropModel = 3;
				}
				else if(!strcasecmp(empiricalModel, "Okumura_Hata_VHF_UHF")){
					PropModel = 4;
				}
				else if(!strcasecmp(empiricalModel, "Okumura_Hata_large_urban")){
					PropModel = 5;
				}
				else if(!strcasecmp(empiricalModel, "Okumura_Hata_medium_urban")){
					PropModel = 6;
				}
				else if(!strcasecmp(empiricalModel, "Okumura_Hata_suburban")){
					PropModel = 7;
				}
				else if(!strcasecmp(empiricalModel, "Okumura_Hata_open_areas")){
					PropModel = 8;
				}
				else if(!strcasecmp(empiricalModel, "COST_231_Hata")){
					PropModel = 9;
				}
				else if(!strcasecmp(empiricalModel, "Suburban_1_9GHz_TA")){
					PropModel = 10;
				}
				else if(!strcasecmp(empiricalModel, "Suburban_1_9GHz_TB")){
					PropModel = 11;
				}
				else if(!strcasecmp(empiricalModel, "Suburban_1_9GHz_TC")){
					PropModel = 12;
				}
				else if(!strcasecmp(empiricalModel, "M2M_UMTS_LOS")){
					PropModel = 13;
				}
				else if(!strcasecmp(empiricalModel, "M2M_UMTS_NLOS")){
					PropModel = 14;
				}
				else if(!strcasecmp(empiricalModel, "ECC33")){
					PropModel = 15;
				}
				else if(!strcasecmp(empiricalModel, "ECC33_dversion")){
					PropModel = 16;
				}
				else if(!strcasecmp(empiricalModel, "AdHoc_LOS")){
					PropModel = 17;
				}
				else if(!strcasecmp(empiricalModel, "HarXiaMicrocell_Low_Rise_NLOS")){
					PropModel = 18;
				}
				else if(!strcasecmp(empiricalModel, "HarXiaMicrocell_High_Rise_Lateral")){
					PropModel = 19;
				}
				else if(!strcasecmp(empiricalModel, "HarXiaMicrocell_High_Rise_ST")){
					PropModel = 20;
				}
				else if(!strcasecmp(empiricalModel, "HarXiaMicrocell_LOS")){
					PropModel = 21;
				}
				else if(!strcasecmp(empiricalModel, "HowardXia_UniformRoofTop")){
					PropModel = 22;
				}
				else if(!strcasecmp(empiricalModel, "HowardXia_BSAntennaAboveRoofTopLevel")){
					PropModel = 23;
				}
				else if(!strcasecmp(empiricalModel, "HowardXia_BSAntennaBelowRoofTopLevel")){
					PropModel = 24;
				}
				else if(!strcasecmp(empiricalModel, "Indoor_2_4G_LOS_80211_a_b")){
					PropModel = 25;
				}
				else if(!strcasecmp(empiricalModel, "Indoor_5_3G_LOS_80211_a_b")){
					PropModel = 26;
				}
				else if(!strcasecmp(empiricalModel, "Indoor_2_4G_NLOS_80211_a_b")){
					PropModel = 27;
				}
				else if(!strcasecmp(empiricalModel, "Indoor_5_3G_NLOS_80211_a_b")){
					PropModel = 28;
				}
				else if(!strcasecmp(empiricalModel, "Bertoni_Walfisch_Urban")){
					PropModel = 29;
				}
				else if(!strcasecmp(empiricalModel, "Egli_Urban_Rural")){
					PropModel = 30;
				}
				else if(!strcasecmp(empiricalModel, "COST_231_Hata_3GPP_TR_25966_V8_Urban")){
					PropModel = 31;
				}
				else if(!strcasecmp(empiricalModel, "COST_231_Hata_3GPP_TR_25966_V8_Suburban")){
					PropModel = 32;
				}
			}
			else{
				printf("[%s]:: Using two-ray-ground channel model \n", __func__);
				PropModel = TWO_RAY_GROUND;
			}
		}
		else{
			printf("[%s]:: Using two-ray-ground channel model\n", __func__);
			PropModel = TWO_RAY_GROUND;
		}
	}
	else{
		printf("[%s]:: Using two-ray-ground channel model\n", __func__);
		PropModel = TWO_RAY_GROUND;
	}

	char	*FILEPATH;
	if ( ObstacleFlag && !strcasecmp(ObstacleFlag, "on") ) {
		if ( !obstacleFileOpenFlag ) {
			obstacleFileOpenFlag = true;

			FILEPATH = (char *)malloc(strlen(GetScriptName()) + 5);
			sprintf(FILEPATH, "%s.obs", GetScriptName());
			obstacleFile = fopen(FILEPATH, "r");

			if( obstacleFile == NULL ) {
				printf("Warning : Can't read file %s\n", FILEPATH);
			}
			else {
				Insert_obstacles(obstacleFile);
			}

			free(FILEPATH);
		}
	}

	return(1);
}

int cm::send(ePacket_ *pkt) {

	double			df;
	double			T_locX;
	double			T_locY;
	double			T_locZ;
	double			R_locX;
	double			R_locY;
	double			R_locZ;
	u_int64_t		ticks;
	struct wphyInfo		*wphyinfo;

	if(uGPRS_ == 1){
		/*
		 * GPRS uses different packet structure
		 */
		bss_message*        bs;
		assert(pkt&&(bs=(bss_message *)pkt->DataInfo_));
		/* get information of incoming packet */
		wphyinfo = bs->wphyinfo;
		bs->flag = PF_RECV;
	}
	else{
		Packet		*p;
		assert(pkt&&(p=(Packet *)pkt->DataInfo_));
		/* get information of incoming packet */
		wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
		p->pkt_setflow(PF_RECV);
	}

	if (!wphyinfo) {
		/* 
		 * If the wireless-phy information of incoming packet
		 * is not "WPHY", we should drop it, and return
		 * 1 to inform lower module to successfully 
		 * accept packet.
		 */
		freePacket(pkt);
		return(1);
	}

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(cm, get);

	T_locX = wphyinfo->srcX_;
	T_locY = wphyinfo->srcY_;
	T_locZ = wphyinfo->srcZ_;

	GetNodeLoc(pkt->calloutObj_->get_nid(), R_locX, R_locY, R_locZ);

	/* simulate propagation delay */
	df = sqrt((T_locX-R_locX)*(T_locX-R_locX)+
			(T_locY-R_locY)*(T_locY-R_locY)+
			(T_locZ-R_locZ)*(T_locZ-R_locZ));
	df = (df / VELOCITY_OF_LIGHT) * 1000000; // us 
	MICRO_TO_TICK(ticks, df);
	ticks += GetCurrentTime();  

	pkt->timeStamp_ = ticks;
	pkt->perio_ = 0;
	pkt->memfun_ = type;
	pkt->func_ = NULL;

	SCHEDULER_INSERT_EVENT(scheduler_, pkt);

	return(1); 
}

int cm::recv(ePacket_ *pkt) {

	int *uPrecompute;
	struct wphyInfo *wphyinfo;

	if(uGPRS_ == 1){
		/*
		 * GPRS uses different packet structure
		 */
		bss_message*        bs;
		assert(pkt&&(bs=(bss_message *)pkt->DataInfo_));
		uPrecompute = NULL;
		wphyinfo = bs->wphyinfo;
	}
	else{
		Packet		*p;
		assert(pkt&&(p=(Packet *)pkt->DataInfo_));
		/* get information of incoming packet */
		uPrecompute = (int *)p->pkt_getinfo("uPre");
		wphyinfo = (struct wphyInfo *)p->pkt_getinfo("WPHY");
	}
	if(uPrecompute == NULL){
		/* 
		 * If the upper-layer module (wireless phy module) of the sending node
		 * doesn't pre-compute the receive power before sending the packet,
		 * we need to compute receive power here
		 */
		computePr(wphyinfo);
		return(put(pkt, recvtarget_));
	}

	/* push this packet to upper module */
	return(put(pkt, recvtarget_));
}

/* compute the power */
double cm::computePr(struct wphyInfo *wphyinfo) {

	assert(wphyinfo);

	u_int32_t               txNid;	/* sender node id */
	u_int32_t               txPid;	/* sender port id */
	struct cmInfo		*cminfo; // channel model information
	double			T_locX, T_locY, T_locZ; // sender location
	double			R_locX, R_locY, R_locZ; // receiver location
	double			t1, t2, t3, r1, r2, r3;
	double			Pr;
	double			*pr;
	double			txCurrAzimuthAngle = 0;
	double			rxCurrAzimuthAngle = 0;
	double			txGain;
	double			rxGain;
	double			*txAntennaHeight;
	double			*Gt, *Gr;
	int			*txBeamWidth, txBeamWidth_;

	txNid		= wphyinfo->nid;
	txPid		= wphyinfo->pid;
	T_locX		= wphyinfo->srcX_;
	T_locY		= wphyinfo->srcY_;
	T_locZ		= wphyinfo->srcZ_;
	txCurrAzimuthAngle	= wphyinfo->currAzimuthAngle_;

	pr = &(wphyinfo)->Pr_;

	/* propagation model (Hz) */
  	propModel = new channelModel(freq_ * 1e6);
	assert(propModel);

	/* get my location and antenna location */
	GetNodeLoc(get_nid(), R_locX, R_locY, R_locZ);
	GetNodeAntenna(get_nid(), r1, r2, r3);
	GetNodeAntenna(txNid, t1, t2, t3);

	/* the offset of the antenna buttom to the node */
	txAntennaHeight = (double *)get_regvar(txNid, txPid, "antennaHeight");

	Gt = (double *)get_regvar(txNid, txPid, "Gain");
	Gr = (double *)get_regvar(get_nid(), get_port(), "Gain");

	if(Gt == NULL || Gr == NULL)
	{
		/*
		 * If the wireless phy layer doesn't regist the Gain value,
		 * we will calculate its Gain here.
		 */
		char	cm_obj_name[100];	// tx cm object name
		cm	*txcmobj;		// tx cm object

		txBeamWidth = (int *)get_regvar(txNid, txPid, "beamwidth");

		if(txBeamWidth == NULL)
		{
			/* not using the channel module. */
			printf("[CM %s] Warning: node %d does not register beamwidth\n",
					__func__, get_nid());
			txBeamWidth_ = 360;
		}
		else{
			txBeamWidth_ = *txBeamWidth;
		}
		rxCurrAzimuthAngle = getAntennaDirection(pointingDirection_, angularSpeed_);

		// get tx node's cm object
		sprintf(cm_obj_name, "Node%d_CM_LINK_%d", txNid, txPid);
		txcmobj = (cm*)RegTable_.lookup_Instance(txNid, cm_obj_name);
		if(txcmobj == NULL){
			delete propModel;
			return 0;
		}

		// get antenna gain
		txGain = getAntennaGain(txNid, get_nid(),
				txBeamWidth_, txCurrAzimuthAngle, txcmobj->AGPOpt, txcmobj->gainUser);

		rxGain = getAntennaGain(get_nid(), wphyinfo->nid,
				beamwidth_, rxCurrAzimuthAngle, AGPOpt, gainUser);
	} else {
		/*
		 * If network uses its own Gain values.
		 */
		txGain = *Gt;
		rxGain = *Gr;
	}

	cminfo = (struct cmInfo *) malloc(sizeof(struct cmInfo));
	assert(cminfo);
	cminfo->PropModel = PropModel;		// index of the propagation channel model
	cminfo->fadingOpt_ = fadingOpt;		// index of the fading model
	cminfo->txNid = txNid;			// tx node id
	cminfo->rxNid = get_nid();		// my node id (rx node id)
	cminfo->fv = FadingVar;			// used in Rayleigh fading
	cminfo->RiceanK = RiceanK; 		// db
	double tX = T_locX+t1; 
	double tY = T_locY+t2;	
	double tZ = T_locZ + *txAntennaHeight;
	double rX = R_locX+r1; 
	double rY = R_locY+r2;	
	double rZ = R_locZ + antennaHeight;
	cminfo->txAntennaHeight = tZ;
	cminfo->rxAntennaHeight = rZ;
	cminfo->nodeDist = sqrt((rX-tX)*(rX-tX) + (rY-tY)*(rY-tY) + (rZ-tZ)*(rZ-tZ));
	cminfo->L = L_;				// system loss
	cminfo->Pt = wphyinfo->TxPr_; 		// tx power (watt)
	cminfo->Gt = txGain;			// tx gain (dbi)
	cminfo->Gr = rxGain;			// rx gain (dbi)
	cminfo->hbd = averageHB; 		// average heights of surrounding buildings (meter)
	cminfo->w = streetWidth;		// street width (meter)
	cminfo->d = averageDist; 		// average separation distance between the rows of buildings (meter)
	cminfo->pathlossExp_ = PLExp_;		// path loss exponent
	cminfo->std_db_ = std_db_;		// standard deviation
	cminfo->dist0_ = dist0_;		// close-in distance (meter)

#if DEBUG_ANTENNA
	printf("[CM %s]node #%d: rxGain = %lf, txGain = %lf (%d, %lf)\n",
			__func__, get_nid(), rxGain, txGain,
			txBeamWidth_, txCurrAzimuthAngle);
#endif

	if((txNid == get_nid()) || (cminfo->nodeDist == 0)){
		/*
		 * If we find that the transmitter and receiver are in the same node or the distance
		 * between two nodes is 0, we return 0 and let the upper layer (wireless phy) 
		 * decide whether to drop this packet or not.
		 *
		 * It will happen in the multi-interface node. If the multi-interface node cotains 
		 * both infrastructure mode and ad hoc mode of mobile node interfaces, 
		 * these two kinds of interfaces will send packets to each other. 
		 * (Because the wireless connects these two kinds of mobile node interfaces together)
		 */
#ifdef CM_DEBUG
		printf("[CM %s]node %d: txNid = %d, txPort = %d, rxNid = %d, rxPort = %d\n",
				__func__, get_nid(), txNid, txPid, get_nid(), get_port());
#endif
		*pr = 0;

		free(cminfo);
		delete propModel;
		return 0;
	}
	/* channel model power function */
	Pr = propModel->Pr(cminfo);
#ifdef CM_DEBUG
	printf("[CM %s]: txNid %d, power %f, dist %f\n", __func__, txNid, Pr, cminfo->nodeDist);
#endif

	/* check attenuation caused by obstacles. */
	if ( ObstacleFlag && !strcasecmp(ObstacleFlag, "on") )
	{
		Pr -= Check_obstacles(txNid, get_nid());
	}

	*pr = Pr;

	free(cminfo);
	delete propModel;

	/* this packet can be received */
	return Pr;
}

int cm::command(int argc, const char *argv[]) {
	return(NslObject::command(argc, argv));
}

/*---------------------------------------------------------------------------
 * getAntennaGain()
 *
 * Look up antenna gain in a pre-computed table.
 *
 * Arguments:
 *	srcNodeID		source node ID
 *	dstNodeID		destination node ID
 *	beamwidth		source antenna beamwidth
 *      pointingDirection	source antenna direction
 *      AGPOpt_			antenna gain pattern option, 1 = enable, 0 = disable
 *
 * Returns:
 *	antenna gain of the source node
 *
 * Side effects:
 *      no side effect.
 *---------------------------------------------------------------------------*/
double 
cm::getAntennaGain(u_int32_t srcNodeID, u_int32_t dstNodeID,
		int beamwidth, double pointingDirection, int AGPOpt_, double gainUser_[360])
{
	double	T_locX, T_locY, T_locZ;
	double	R_locX, R_locY, R_locZ;
	double	relX, relY, relZ;	// Relative position
	double	rxAngle;
	double	angle;
	double	gain;

	if((AGPOpt_ == 0) && (beamwidth == 360)){
		return 1.0;
	}

	GetNodeLoc(srcNodeID, T_locX, T_locY, T_locZ);
	GetNodeLoc(dstNodeID, R_locX, R_locY, R_locZ);

	relX = R_locX - T_locX;
	relY = T_locY - R_locY;
	relZ = R_locZ - T_locZ;
	rxAngle = pos2angle(relX, relY);

	angle = (pointingDirection >= rxAngle) ?
		pointingDirection - rxAngle : rxAngle - pointingDirection;

	gain = Get_Antenna_Gain((int)angle, beamwidth, AGPOpt_, gainUser_);

	//printf("#%d rxAngle = %lf, angle = %lf, gain = %lf\n", dstNodeID, rxAngle, angle, gain);
	
	return gain;
}

