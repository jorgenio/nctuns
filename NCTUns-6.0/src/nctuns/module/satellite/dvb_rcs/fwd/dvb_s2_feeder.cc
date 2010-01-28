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

#include <if_tun.h>
#include <sys/ioctl.h>
#include <timer.h>
#include <nctuns_api.h>
#include <nctuns-dep/link.h>

#include "dvb_s2_feeder.h"
#include "../common/dvb_pkt.h"
#include "../common/dvbrcs_api.h"
#include "../common/fec/bch.h"
#include "../rcst/rcst_ctl.h"

MODULE_GENERATOR(Dvb_s2_feeder);

extern SLIST_HEAD(headOfSat, con_list) headOfSat_;
extern char *DVBChannelCoding;

/*
 * Member functions definition of class Dvb_s2_feeder.
 */

/*
 * Constructor
 */
Dvb_s2_feeder::Dvb_s2_feeder(uint32_t type, uint32_t id, struct plist *pl, const char *name)
: Dvb_s2(type, id, pl, name)
, _next_input_stream(0)
{
	/*
	 * DVB-S.2 feeder module parameters.
	 */
	vBind("BufferSize", (int*)&_buffer_size);
	vBind("SymbolRate", &_symbol_rate); /* Mega symbols per second */

	/*
	 * Parameters for the error model.
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
	 * Initial physical layer signalling.
	 */
	_dvb_s2_pls_from_ncc = new Dvb_s2_pls();

	/*
	 * Initial timers.
	 */
	_tx_timer = new timerObj();

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
Dvb_s2_feeder::~Dvb_s2_feeder()
{
	for (int i = 0 ; i < _number_of_input_stream ; i++)
		delete _input_buffer[i];
	delete[] _input_buffer;

	delete _tx_timer;
	delete _dvb_s2_pls_from_ncc;
}


/*
 * Initialize the DVB-S.2 feeder system.
 * The function will be called by tclObject.
 */
int
Dvb_s2_feeder::init()
{
	_dvb_s2_pls_from_ncc->set_modcod(Dvb_s2_pls::MODCOD_32APSK_9_10);
	_dvb_s2_pls_from_ncc->set_fec_frame_size(Dvb_s2_pls::FECFRAME_NORMAL);
	_dvb_s2_pls_from_ncc->set_pilot_conf(Dvb_s2_pls::PILOT_OFF);

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
                        if (it->feeder_nid==get_nid())
                        {
                                sat_nid = it->sat_nid;
                                nid_found = true;
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

        /* open xxx.dvbrcs.freq file to read return down link freq */
        sprintf(freqfilepath,"%s.dvbrcs.freq", GetScriptName());
        if( (freqFile = fopen(freqfilepath,"r")) )
        {
                uint32_t        satnodeid;
                float           ForwardUpCentreFreq;

                while (fgets(line, 200, freqFile)) {
                        if ((sscanf(line, " SatNodeId: %u", &satnodeid)) && satnodeid == sat_nid) {
                                while (fgets(line, 200, freqFile)) {
                                        if((sscanf(line, " ForwardUpCentreFreq: %f", &ForwardUpCentreFreq))){
                                                _budget.freq = ForwardUpCentreFreq;
                                                break;
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

			typeStart = POINTER_TO_MEMBER(Dvb_s2_feeder, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(Dvb_s2_feeder, TurnOffLinkFailFlag);

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

	/*
	 * Initialize input buffers.
	 */
	_input_buffer = new Stream_buffer*[_number_of_input_stream];
	for (int i = 0 ; i < _number_of_input_stream ; i++)
		_input_buffer[i] = new Stream_buffer(_buffer_size);

	/*
	 * Initial timers.
	 */
	_tx_timer->init();
	_tx_timer->setCallOutObj(this,
		(int (NslObject::*)(Event_*))&Dvb_s2_feeder::_tx_handler);
	_tx_timer->start(GetCurrentTime(), 0);

	/*
	 * Initial link.
	 */
	_init_link();

	return Dvb_s2::init();
}


/*
 * The feeder should not receive any packet via forward channel.
 */
int
Dvb_s2_feeder::recv(ePacket_* event)
{
	assert(0);
}


/*
 * Process the packet and send to the satellite. The function is called by
 * the transport stream layer above, only used in the feeder.
 * - event: the pointer to the user packet.
 * - return value: -1 if the input stream buffer of the specific interface
 *                 is full while 1 if the user packet is stored in the buffer
 *                 successfully.
 */
int
Dvb_s2_feeder::send(ePacket_* event)
{
	Dvb_pkt*	packet;
	void*		user_packet;
	unsigned int	user_packet_len;
	uint8_t		send_interface;

	/*
	 * Get user_packet from Dvb_pkt.
	 */
	assert(event && (packet = (Dvb_pkt*)event->DataInfo_)
		&& (user_packet = packet->pkt_getdata()));
	/*
	 * if link fail flag is on, then stop to send
	 */
	if ( LinkFailFlag > 0 ) {
		dvb_freePacket(event);
		return(1);
	}

	/*
	 * Get user packet length and interface to send.
	 */
	user_packet_len = packet->pkt_getlen();
	send_interface = packet->pkt_getfwdinfo()->interface;
	/*
	 * Only support MPEG2 transport stream packet in current version.
	 */
	assert(((int8_t*)user_packet)[0] == SYNC_MPEG2_TS);
	assert(packet->pkt_gettype() == PKT_MPEG2_TS);
	/*
	 * Packet length should be 188 bytes.
	 */
	assert(user_packet_len == MPEG2_TS_LEN);
	/*
	 * The specific interface should not exceed
	 * the number of input streams.
	 */
	assert(send_interface < _number_of_input_stream);
	/*
	 * Call mode adaptation to handle the user packet.
	 */
	if (_mode_adaptation(send_interface, user_packet, user_packet_len) < 0) {
		/*
		 * The buffer of the specific interface is full.
		 */
		return -1;
	} else {
		//printf("\e[32mSend store packet into buffer of interface[%u]\e[m\n", send_interface);
		/*
		 * Free the event and the handled user packet.
		 */
		dvb_freePacket(event);
		return 1;
	}
}


/*
 * Initialize link information.
 */
void
Dvb_s2_feeder::_init_link()
{
	/*
	 * find my SatLink module in global linked list headOfSat_ 
	 */
	struct con_list* cl;
	SLIST_FOREACH(cl, &headOfSat_, nextLoc) {
		char mark = 0;
		struct plist *clp = cl->obj->get_portls();
		struct plist *obp = get_portls();
		/*
		 * compare ports list whether match, if match then this module is
		 * my SatLink module
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
	_link_obj = (class Link*)cl->obj;
}


/*
 * This function is called by _tx_timer when previous transmission
 * is completed. It calls _merger_slicer to check if there is any
 * data in one of input stream buffers. If nothing in buffers, it
 * would send dummy frame.
 */
int
Dvb_s2_feeder::_tx_handler(Event_* event)
{
	_mpeg2_ts_sp->send_traffic_ts();
	/*
	 * Set physical layer signalling assigned by the NCC.
	 */
	*_dvb_s2_pls = *_dvb_s2_pls_from_ncc;
	/*
	 * Set _coding_type to current coding type, this will be used in
	 * _merger_slicer(), _stream_adaptation() and _fec_encoding().
	 */
	_coding_type = _dvb_s2_pls->coding_type();
	/*
	 * Get the frame from the buffer and prepare to send it.
	 */
	void*		base_band_frame;
	unsigned int	data_field_len;

	if ((base_band_frame = _merger_slicer(data_field_len))) {
		_stream_adaptation(base_band_frame, data_field_len);

		/*
		 * Apply FEC encoding if FEC system is enabled.
		 */
		if (DVBChannelCoding && !strcasecmp(DVBChannelCoding, "on"))
			_fec_encoding(base_band_frame);
	} else {
		/*
		 * No frame is ready to send. The dummy frame
		 * should be transmitted, so that the physical
		 * layer signalling should be specified properly
		 * for the receiver.
		 */
		_dvb_s2_pls->set_modcod(Dvb_s2_pls::MODCOD_DUMMY_PLFRAME);
	}


	/*
	 * Simulate transmission delay.
	 */
	uint64_t	tx_time;
	unsigned int	fec_frame_size;
	/*
	 * Set the FEC frame size.
	 */
	if (_dvb_s2_pls->modcod() == Dvb_s2_pls::MODCOD_DUMMY_PLFRAME)
		fec_frame_size = 0;
	else {
		switch (_dvb_s2_pls->fec_frame_size()) {
		case Dvb_s2_pls::FECFRAME_NORMAL:
			fec_frame_size = DVB_FECFRAME_SIZE_NORMAL;
			break;

		case Dvb_s2_pls::FECFRAME_SHORT:
			fec_frame_size = DVB_FECFRAME_SIZE_SHORT;
			break;

		default:
			assert(0);
		}
	}
	/*
	 * Compute the transmission delay.
	 */
	tx_time = _tx_delay(_dvb_s2_pls->modulation_type(), fec_frame_size);
	/*
	 * Add 1 tick to prevent that the start tick of sending is
	 * the same as the end tick of receiving.
	 */
	_tx_timer->start(tx_time + 1, 0);

	/*
	 * Send to linked satellite.
	 */
	class Dvb_pkt*	packet;

	packet = new class Dvb_pkt();
	/*
	 * If the frame is the dummy frame, data_field_len should be zero,
	 * therefore the pkt_attach will only allocate the _pbuf for the
	 * packet type setting, so that the pkt_settype should be invoked
	 * AFTER pkt_attach.
	 */
	if(!data_field_len)
		assert(_dvb_s2_pls->modcod()
		== Dvb_s2_pls::MODCOD_DUMMY_PLFRAME);

	/*
	 * We should attach base_band_frame with length of fec_frame_size,
	 * since the base_band_frame is pre-allocated with length of
	 * fec_frame_size by _merger_slicer().
	 */
	packet->pkt_attach(base_band_frame, fec_frame_size / 8);
	packet->pkt_setflag(FLOW_RECV);
	packet->pkt_settype(PKT_DVB_S2);
	/*
	 * Put the symbol rate of the feeder in the Dvb_pkt for
	 * the receiver to simulate the receiving time.
	 */
	packet->pkt_getfwdinfo()->symbol_rate = _symbol_rate;
	/*
	 * Update the physical layer signalling to the lastest version.
	 * for the receiver to process the packet.
	 */
	_dvb_s2_pls->copy_info_to(packet->pkt_getfwdinfo()->dvb_s2_pls);

	event = new ePacket_;
	event->DataInfo_ = packet;

	/*
	 * Add link_info link_budget & log_info to packet,
	 * for the use of sat calculating rainfade. BER & log
	 */
	_info.freq = _budget.freq;
	packet->pkt_getfwdinfo()->linfo = _info;
	packet->pkt_getfwdinfo()->lbudget = _budget;
	packet->pkt_getfwdinfo()->loginfo.phy_src = get_nid();
	packet->pkt_getfwdinfo()->loginfo.tx_time = tx_time;


	/*
	 * for each connect SatLink module in linked list of my_link->hos_
	 */
	struct con_list*	cl;
	uint64_t		ticks;
	BASE_OBJTYPE(type);
	ePacket_*		ep;
	SLIST_FOREACH(cl, &_link_obj->hos_, nextLoc) {
		ticks =	_prop_delay(cl->obj) + GetCurrentTime();
		/*
		 * copy a packet to every one 
		 */
		type = POINTER_TO_MEMBER(NslObject, get);
		ep = dvb_pkt_copy(event);

		 /* Log "StartTX" event and "SuccessTX" event*/
		if(_ptrlog == 1){

			struct logEvent* logep;
			dvbrcs_log*	ssdvbrcs_log;
			ssdvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
			dvbrcs_log* sedvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
		
			LOG_SAT_DVBRCS(ssdvbrcs_log, GetCurrentTime(), GetCurrentTime(),
					get_type(), get_nid(), StartTX, get_nid(), 
					cl->obj->get_nid(), 0,packet->pkt_gettype(), packet->pkt_getlen(), 1, DROP_NONE);
			INSERT_TO_HEAP(logep, ssdvbrcs_log->PROTO, ssdvbrcs_log->Time+START, ssdvbrcs_log);

			LOG_SAT_DVBRCS(sedvbrcs_log, GetCurrentTime()+tx_time , GetCurrentTime(),
					get_type(), get_nid(), SuccessTX, get_nid(),
					cl->obj->get_nid(), 0,packet->pkt_gettype(), packet->pkt_getlen(), 1, DROP_NONE);
			INSERT_TO_HEAP(logep, sedvbrcs_log->PROTO, sedvbrcs_log->Time+ENDING, sedvbrcs_log);
		}

		setObjEvent(ep, ticks, 0, cl->obj, type, ep->DataInfo_);
	};
	dvb_freePacket(event);
	return 0;
}


/*
 * Calcaute the propagation delay.
 * - obj: destination object.
 * - return value: the propagation delay in tick.
 */
uint64_t
Dvb_s2_feeder::_prop_delay(class NslObject* obj)
{
	double		loc_x;
	double		loc_y;
	double		loc_z;
	double		r_loc_x;
	double		r_loc_y;
	double		r_loc_z;
	double		df;
	uint64_t	prop_delay;

	char		dummy[1000];
	assert(obj);
	/*
	 * Get my location. 
	 */
	assert(GetNodeLoc(get_nid(), loc_x, loc_y, loc_z) > 0);
	/*
	 * Simulate the progagation delay, get connected node location.
	 */
	assert(GetNodeLoc(obj->get_nid(), r_loc_x, r_loc_y, r_loc_z) > 0);
	/*
	 * Calculate the propagation delay.
	 */
	df = sqrt((loc_x - r_loc_x) * (loc_x - r_loc_x) +
		  (loc_y - r_loc_y) * (loc_y - r_loc_y) +
		  (loc_z - r_loc_z) * (loc_z - r_loc_z));
	df = (df / SPEED_OF_LIGHT) * 1000000;	/* us */
	/*
	 * Unit transfer: us to ticks
	 */
	MICRO_TO_TICK(prop_delay, df);

	sprintf(dummy, "node %u to %u delay = %llu\n",
		get_nid(), obj->get_nid(), prop_delay);

	return prop_delay;
}


/*
 * Store the user packet in the input stream buffer of the specific interface.
 * - user_packet: pointer to the user packet.
 * - interface: the interface to put the user packet.
 * - return value: -1 if the input stream buffer is full while 0 if the
 *                 user packet is stored in the buffer successfully.
 */
int
Dvb_s2_feeder::_mode_adaptation(uint8_t interface,
	void* user_packet, unsigned int user_packet_len)
{
	uint8_t	crc_8;
	int8_t*	user_packet_without_sync;

	/*
	 * The input stream buffer is no space for the user packet.
	 */
	if (_input_buffer[interface]->free() < user_packet_len)
		return -1;

	/*
	 * These two functions do nothing in current design.
	 */
	_input_stream_synchroniser();

	_null_packet_deletion();

	/*
	 * Generate CRC-8 from the user packet without the sync-byte.
	 */
	user_packet_without_sync = (int8_t*)user_packet + 1;
	crc_8 = _crc_8(user_packet_without_sync, MPEG2_TS_LEN - 1, DVB_CRC8_GEN_POLY);

	/*
	 * Put the user packet without the sync byte into the buffer of the
	 * specific interface.
	 */
	_input_buffer[interface]->put(user_packet_without_sync, MPEG2_TS_LEN - 1);
	/*
	 * Put the computed CRC-8 into the sync-byte of the following user packet.
	 */
	_input_buffer[interface]->put(&crc_8, 1);
	return 0;
}


/*
 * Sub-procedure of mode adaptation.
 * Not implement yet.
 */
int
Dvb_s2_feeder::_input_stream_synchroniser()
{
	return 0;
}


/*
 * Sub-procedure of mode adaptation.
 * Not implement yet.
 */
int
Dvb_s2_feeder::_null_packet_deletion()
{
	return 0;
}


/*
 * Sub-procedure of mode adaptation. It gets data of proper length from one of
 * input buffers and generates a packet then fills proper base-band header and
 * the data gotten from the buffer. If there is no data saved in input buffers,
 * this function does nothing.
 * - data_field_len: the pointer for this function to return the length of the
 *                   data field gotten from the input stream buffer. (excluding
 *                   the base-band header)
 * - interface: the interface where the data is gotten. If no data is gotten,
 *              , e.g. data_field_len is zero, this variable is meaningless.
 * - return value: NULL if the there is no data in the buffers while the pointer
 *                 to the base-band frame if there are available data in one of
 *                 the input stream buffers.
 */
void*
Dvb_s2_feeder::_merger_slicer(unsigned int& data_field_len)
{
	/*
	 * Check each input stream buffer whether there is data waiting to send.
	 */
	uint8_t	interface;
	interface = _next_input_stream;
	for (int i = 0 ; i < _number_of_input_stream ; i++) {
		if ((data_field_len = _input_buffer[interface]->content_len()))
			break;
		interface = (interface + 1) % _number_of_input_stream;
	}
	if (!data_field_len)
		return NULL;
	/*
	 * Start from next one when select input streams.
	 */
	_next_input_stream = (interface + 1) % _number_of_input_stream;

	/*
	 * SYNCD field computing (The SYNCD value is in bit).
	 */
	uint16_t	syncd;

	syncd = ((data_field_len - 1) % MPEG2_TS_LEN) * 8;

	/*
	 * Base-band frame generation.
	 */
	int8_t*		base_band_frame;

	/*
	 * We pre-allocate the space which is sufficieny for the size
	 * required when performing FEC encoding.
	 */
	switch (_dvb_s2_pls->fec_frame_size()) {
	case Dvb_s2_pls::FECFRAME_NORMAL:
		base_band_frame = new int8_t[DVB_FECFRAME_SIZE_NORMAL / 8];
		break;
	case Dvb_s2_pls::FECFRAME_SHORT:
		base_band_frame = new int8_t[DVB_FECFRAME_SIZE_SHORT / 8];
		break;
	default:
		assert(0);
	};

	/*
	 * Fill the data field of base-band frame. The max length of the data
	 * field should be the length of BCH uncoded block.
	 */
	//printf("\e[1;5;32m%s:_merger_slicer: input buffer get.\e[m\n", get_name());
	data_field_len = _input_buffer[interface]->get(
		_base_band_header->data_field(base_band_frame),
		_fec_parameter_table[_coding_type][BCH_UNCODED] / 8 -
		_base_band_header->header_size()
		);
	/*
	 * Set up base-band header.
	 */
	_base_band_header->set_input_stream_id(interface);
	_base_band_header->set_data_field_length(data_field_len * 8);
	_base_band_header->set_syncd(syncd);
	_base_band_header->set_crc_8(
		_crc_8(
			_base_band_header,
			_base_band_header->header_size() - 1,
			DVB_CRC8_GEN_POLY
			)
		);
	/*
	 * Fill the header of base-band frame.
	 */
	_base_band_header->fill_base_band_header(base_band_frame);

	return base_band_frame;
}

uint32_t 
Dvb_s2_feeder::remain_len()
{
	uint32_t fetch = _fec_parameter_table[_coding_type][BCH_UNCODED] / 8 - 10;
	uint32_t content = _input_buffer[0]->content_len();

	return fetch > content ? fetch - content : 0;
}



/*
 * This function will be called by _tx_handler when the physical layer
 * finishes last transmit and find there is data waiting to be sent in
 * one of the input stream buffer. After padding and scrambling, it will
 * call _fec_encoding to encode the frame to send.
 * base_band_frame: the pointer to the frame to be sent.
 * data_field_len: the length of data field gotten from the buffer.
 */
void
Dvb_s2_feeder::_stream_adaptation(void* base_band_frame, unsigned int data_field_len)
{
	_padder(
		base_band_frame,
		_base_band_header->header_size() + data_field_len,
		_fec_parameter_table[_coding_type][BCH_UNCODED] / 8
		);

	_bb_scrambler(
		base_band_frame,
		_fec_parameter_table[_coding_type][BCH_UNCODED] / 8
		);
}


/*
 * Sub-procedure of stream adaptation. It pads zeroes after the relevant
 * content of the frame.
 * frame: the pointer to the frame to be padded.
 * content_len: length of the relevant content in the base-band frame.
 * frame_len: length of the frame.
 */
void
Dvb_s2_feeder::_padder(void* frame, unsigned int content_len, unsigned int frame_len)
{
	memset((int8_t*)frame + content_len, 0, frame_len - content_len);
}


/*
 * The procedure performs forward error correction encoding, which includes
 * outer coding (BCH), inner coding (LDPC) and bit interleaving.
 * - base_band_frame: the pointer to the base band frame to be encoded.
 */
void
Dvb_s2_feeder::_fec_encoding(void* base_band_frame)
{
	/*
	 * Apply inner encoding.
	 */
	_bch_encoder(base_band_frame);
	/*
	 * Apply outer encoding.
	 */
	_ldpc_encoder(base_band_frame);
	/*
	 * Apply proper bit interleaving.
	 */
	switch (_dvb_s2_pls->modulation_type()) {
	case Dvb_s2_pls::MOD_QPSK:
		break;
	case Dvb_s2_pls::MOD_8PSK:
		_bit_interleaver(
			base_band_frame,
			_dvb_s2_pls->fec_frame_size()
				/ DVB_8PSK_INTERLEAVER_COL,
			DVB_8PSK_INTERLEAVER_COL
			);
		break;
	case Dvb_s2_pls::MOD_16APSK:
		_bit_interleaver(
			base_band_frame,
			_dvb_s2_pls->fec_frame_size()
				/ DVB_16PSK_INTERLEAVER_COL,
			DVB_16PSK_INTERLEAVER_COL
			);
		break;
	case Dvb_s2_pls::MOD_32APSK:
		_bit_interleaver(
			base_band_frame,
			_dvb_s2_pls->fec_frame_size()
				/ DVB_32PSK_INTERLEAVER_COL,
			DVB_32PSK_INTERLEAVER_COL
			);
		break;
	default:
		assert(0);
	}
}


/*
 * Sub-procedure of FEC encoding. BCH encoding invoker, check if BCH system
 * of current mode is initialized. If not, initialize the system and call
 * the decoder.
 * bch_block: the pointer to the block to be encoded.
 */
void
Dvb_s2_feeder::_bch_encoder(void* bch_block)
{
	enum _dvb_bch_mode	bch_mode;
	switch (_dvb_s2_pls->fec_frame_size()) {
	case Dvb_s2_pls::FECFRAME_NORMAL:
		switch (_fec_parameter_table[_coding_type][BCH_T_ERR]) {
		case 12:
			bch_mode = DVB_BCH_GF16_T12;
			if (_bch[DVB_BCH_GF16_T12])
				break;
			_bch[DVB_BCH_GF16_T12] = new class Bch;
			_bch[DVB_BCH_GF16_T12]->init(
				_bch_prim_poly_gf16, 16,
				_bch_gen_poly_gf16_t12, 12);
			break;

		case 10:
			bch_mode = DVB_BCH_GF16_T10;
			if (_bch[DVB_BCH_GF16_T10])
				break;
			_bch[DVB_BCH_GF16_T10] = new class Bch;
			_bch[DVB_BCH_GF16_T10]->init(
				_bch_prim_poly_gf16, 16,
				_bch_gen_poly_gf16_t10, 10);
			break;

		case 8:
			bch_mode = DVB_BCH_GF16_T8;
			if (_bch[DVB_BCH_GF16_T8])
				break;
			_bch[DVB_BCH_GF16_T8] = new class Bch;
			_bch[DVB_BCH_GF16_T8]->init(
				_bch_prim_poly_gf16, 16,
				_bch_gen_poly_gf16_t8, 8);
			break;

		default:
			assert(0);
		};
		break;

	case Dvb_s2_pls::FECFRAME_SHORT:
		switch (_fec_parameter_table[_coding_type][BCH_T_ERR]) {
		case 12:
			bch_mode = DVB_BCH_GF14_T12;
			if (_bch[DVB_BCH_GF14_T12])
				break;
			_bch[DVB_BCH_GF14_T12] = new class Bch;
			_bch[DVB_BCH_GF14_T12]->init(
				_bch_prim_poly_gf14, 14,
				_bch_gen_poly_gf14_t12, 12);
			break;

		default:
			assert(0);
		};
		break;

	default:
		assert(0);
	}
	_bch[bch_mode]->encoder(
		bch_block, _fec_parameter_table[_coding_type][BCH_CODED]);
}


/*
 * Sub-procedure of FEC encoding. LDPC encoding invoker, check if LDPC system
 * of current mode is initialized. If not, initialize the system and call
 * the decoder.
 * ldpc_block: the pointer to the block to be encoded.
 */
void
Dvb_s2_feeder::_ldpc_encoder(void* ldpc_block)
{
	if (!_ldpc[_coding_type]) {
		_ldpc[_coding_type] = new class Ldpc;
		_ldpc[_coding_type]->init(_coding_type);
	}
	switch (_dvb_s2_pls->fec_frame_size()) {
	case Dvb_s2_pls::FECFRAME_NORMAL:
		_ldpc[_coding_type]->encoder(
			ldpc_block, DVB_FECFRAME_SIZE_NORMAL);
		break;

	case Dvb_s2_pls::FECFRAME_SHORT:
		_ldpc[_coding_type]->encoder(
			ldpc_block, DVB_FECFRAME_SIZE_SHORT);
		break;

	default:
		assert(0);
	};
}

void Dvb_s2_feeder::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void Dvb_s2_feeder::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}

