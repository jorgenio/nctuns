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

#include <wimax/phy/fec/rs/rs_code.h>
#include <wimax/phy/fec/cc/conv_code.h>

#include "dvb_rcs.h"
#include "dvb_rcs_rcst.h"
#include "rcs_mac.h"
#include "rcs_atm.h"
#include <if_tun.h>
#include <sys/ioctl.h>

MODULE_GENERATOR(Dvb_rcs_rcst);

/*
 * Constructor
 */
Dvb_rcs_rcst::Dvb_rcs_rcst(u_int32_t type, u_int32_t id, struct plist *pl,
		 const char *name)
:Dvb_rcs(type, id, pl, name)
{
	/*
	 * Parameters for error model
	 */
	vBind("Power", &_budget.tx_pwr);
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

	/*
	 * turn off link fail flag
	 */
	LinkFailFlag = 0;

	vBind("linkfail", &_linkfail);
	vBind("linkfailFileName", &linkfailFileName);
}


/*
 * module initialize
 */
int Dvb_rcs_rcst::init()
{
        char            line[200];
        char            freqfilepath[200];

        /* get satellite_node_id */
        char                    filename[1000];
        list<dvbrcs_node_id>    node_id_cfg;
        uint32_t                sat_nid;
        sprintf(filename, "%s.dvbrcs.nodeid", GetScriptName());
        if (_parse_nodeid_cfg(filename, node_id_cfg))
        {
                bool            nid_found = false;

                for (list<dvbrcs_node_id>::iterator it = node_id_cfg.begin();
                     it != node_id_cfg.end() && !nid_found; it++)
                {
                        list<rcst_node_id>      &p_list = it->rcst_nid_list;
                        for(list<rcst_node_id>::iterator it_rcst_nid = p_list.begin(); 
                            it_rcst_nid != p_list.end(); it_rcst_nid++)
                        {
                                if(it_rcst_nid->rcst_nid == get_nid())
                                {
                                        sat_nid = it->sat_nid;
                                        nid_found = true;
                                }
                        }
                }

                if (!nid_found)
                {
                        printf("Node ID config file error??\n");
                        assert(0);
                }

        }
        else
        {
                assert(0);
        }

        _ncc_config = GET_REG_VAR1(get_portls(), "NCC_CONFIG", Ncc_config *); 
        assert(_ncc_config);
       _my_superframe_id = GET_REG_VAR1(get_portls(), "MY_SUPERFRAME_ID", uint8_t *);

        /* open xxx.dvbrcs.freq file to read return down link freq */
        sprintf(freqfilepath,"%s.dvbrcs.freq", GetScriptName());
        if( (freqFile = fopen(freqfilepath,"r")) )
        {
                uint32_t        satnodeid;
                double          ReturnUpCentreFreq;
                uint8_t         superframe_id;

                while (fgets(line, 200, freqFile)) {
                        if ((sscanf(line, " SatNodeId: %u", &satnodeid)) && satnodeid == sat_nid) {
                                while (fgets(line, 200, freqFile)) {
                                        if(!(strncmp(line, "#Return up link", 15))){
                                                while (fgets(line, 200, freqFile)) {
                                                        sscanf(line, "%hhu_%lf",&superframe_id, &ReturnUpCentreFreq);
                                                        if(superframe_id == *_my_superframe_id)
                                                        {
                                                                _budget.freq = ReturnUpCentreFreq;
                                                                break;
                                                        }
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

			typeStart = POINTER_TO_MEMBER(Dvb_rcs_rcst, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(Dvb_rcs_rcst, TurnOffLinkFailFlag);

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
	return (Dvb_rcs::init());
}


/*
 * Dvb_rcs_rcst module must do below mechanism:
 * - randomization
 * - CRC-16
 * - coding (RS/CC or TC)
 * - Modulation (BER)
 */
int Dvb_rcs_rcst::send(ePacket_ * event)
{
	/*
	 * if link fail flag is on, then stop to send
	 */
	if ( LinkFailFlag > 0 ) {
		dvb_freePacket(event);
		return(1);
	}

	return Dvb_rcs::send(event);
}


/*
 * Dvb_rcs_rcst module must do below mechanism:
 * - de-randomization
 * - CRC-16
 * - coding (RS/CC or TC)
 */
int Dvb_rcs_rcst::recv(ePacket_ * event)
{
	Dvb_pkt *pkt;

	assert(pkt = (Dvb_pkt *) event->DataInfo_);

	/*
	 * if this node is RCST, then by pass other packet unless packet type
	 * is DVBRCS then drop it
	 */
	if (pkt->pkt_gettype() == PKT_DVBRCS) {
		dvb_freePacket(event);
		return (1);
	}

	/*
	 * if this node is RCST, then just recv it by next layer
	 */
	return NslObject::recv(event);
}

void Dvb_rcs_rcst::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void Dvb_rcs_rcst::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}

