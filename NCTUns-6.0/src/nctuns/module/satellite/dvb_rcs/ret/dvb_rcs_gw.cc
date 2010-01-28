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
#include <satellite/dvb_rcs/common/dvbrcs_api.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include <satellite/dvb_rcs/common/fec/crc.h>
#include <con_list.h>
#include <nctuns-dep/link.h>

#include "dvb_rcs.h"
#include "dvb_rcs_gw.h"
#include "rcs_mac.h"
#include "rcs_atm.h"
#include <if_tun.h>
#include <sys/ioctl.h>
extern RegTable RegTable_;

MODULE_GENERATOR(Dvb_rcs_gw);

/*
 * Constructor
 */
Dvb_rcs_gw::Dvb_rcs_gw(u_int32_t type, u_int32_t id, struct plist *pl,
		 const char *name)
:Dvb_rcs(type, id, pl, name)
{
	/*
	 * Parameters for the error model.
	 */
	vBind("GSAnteLength", &_budget.es_dia);
	vBind("GSAnteEfficiency", &_budget.es_e);
	vBind("RainFade", &_budget.rain_fade);
	vBind("rainfade_option", &_budget.rainfade_option);

	/*
	 * Parameters for the rain fade.
	 */
	vBind("Antenna_angle", &_info.ele_angle);
	vBind("taur", &_info.taur );
	vBind("RainHeight", &_info.Hrp );
	vBind("EarthStationHeight", &_info.Hs );
	vBind("Latitude", &_info.latitude);
	vBind("Rainrate", &_info.rainfall_rate);

	_sp_nid = 0;
	_sp_name = (char *)malloc(sizeof(char) * 100);
	_ncc_nid = 0;
	_ncc_name = (char *)malloc(sizeof(char) * 100);

	/*
	 * turn off link fail flag
	 */
	LinkFailFlag = 0;

	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);

}

Dvb_rcs_gw::~Dvb_rcs_gw()
{
	free(_sp_name);
	free(_ncc_name);
}


/*
 * module initialize
 */
int Dvb_rcs_gw::init()
{
	/*
	 * check link fail flag, and to set up timer to start/stop to send
	 */
	char		line[128];

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

			typeStart = POINTER_TO_MEMBER(Dvb_rcs_gw, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(Dvb_rcs_gw, TurnOffLinkFailFlag);

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

	char file_path[200];
	FILE *fd;
	char buf[101];

	/*
	 * get upper layer object pointer
	 */
	sprintf(file_path, "%s%s%d", GetScriptName(), ".dvbrcs.gw.to.", get_nid());

	if ((fd=fopen(file_path, "r")) == NULL) {
		printf("Error: can't open file %s\n", file_path);
		exit(0);
	}

	while( !feof(fd) ) {
		buf[0] = '\0'; fgets(buf, 100, fd);
		if ((buf[0]=='\0')||(buf[0]=='#'))
			continue;

		if( sscanf(buf, "NCC.%d.%s", &_ncc_nid, _ncc_name) != 2 && 
		    sscanf(buf, "SP.%d.%s", &_sp_nid, _sp_name)	!= 2 ) {
			printf("Warning: format error -> %s\n", buf);
			exit(0);
		}
	}
	fclose(fd);

	return (Dvb_rcs::init());
}


/*
 * NCC or SP node should not send packet to return link, free it
 */
int Dvb_rcs_gw::send(ePacket_ * event)
{
	dvb_freePacket(event);
	return (1);
}


/*
 * Dvb_rcs_gw module must do below mechanism:
 * - de-randomization
 * - CRC-16
 * - coding (RS/CC or TC)
 */
int Dvb_rcs_gw::recv(ePacket_ * event)
{
	Dvb_pkt *pkt;
	NslObject *obj;

	assert(pkt = (Dvb_pkt *) event->DataInfo_);

	/*
	 * if link fail flag is on, then stop to recv
	 */
	if ( LinkFailFlag > 0 ) {
		dvb_freePacket(event);
		return(1);
	}

	/*
	 * if this packet type is DVB_S2, then drop it. because it don't
	 * process dvb_s2 packet, and it must at Gateway
	 */
	if (pkt->pkt_gettype() == PKT_DVB_S2) {
		dvb_freePacket(event);
		return (1);
	}
	Dvb_rcs::recv_handler(event);

	/*
	 * jump send to upper layer, upper layer may be at other node
	 */
	if (_sp_name && (obj = RegTable_.lookup_Instance(_sp_nid, _sp_name))) {
		obj->recv(dvb_pkt_copy(event));
	}

	if (_ncc_name && (obj = RegTable_.lookup_Instance(_ncc_nid, _ncc_name))) {
		obj->recv(dvb_pkt_copy(event));
	}

	dvb_freePacket(event);
	return (1);
}

void Dvb_rcs_gw::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void Dvb_rcs_gw::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}

