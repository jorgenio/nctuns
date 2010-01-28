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
#include <satellite/dvb_rcs/ret/dvb_rcs.h>
#include <satellite/dvb_rcs/ret/rcs_mac.h>
#include <satellite/dvb_rcs/ret/rcs_atm.h>
#include <satellite/dvb_rcs/common/fec/crc.h>
#include <con_list.h>
#include <nctuns-dep/link.h>

#include <wimax/phy/fec/rs/rs_code.h>
#include <wimax/phy/fec/cc/conv_code.h>

extern SLIST_HEAD(headOfLink, con_list) headOfSat_;
extern char *DVBChannelCoding;

/*
 * create Convolutional code and Turbo code punctured table
 */

     struct Dvb_rcs::cc_puncture Dvb_rcs::_cc_punctured_table[] = {
	     {1.0 / 2.0, 01, 01, 1, {NULL}},
	     {2.0 / 3.0, 02, 03, 2, {NULL}},
	     {3.0 / 4.0, 05, 06, 3, {NULL}},
	     {5.0 / 6.0, 025, 032, 5, {NULL}},
	     {7.0 / 8.0, 0105, 0172, 7, {NULL}},
	     /*
	      * below table will be used by Turbo code
	      */
	     {1.0 / 3.0, 0, 0, 0, {NULL}},
	     {2.0 / 5.0, 0, 0, 0, {NULL}},
	     {4.0 / 5.0, 0, 0, 0, {NULL}},
	     {6.0 / 7.0, 0, 0, 0, {NULL}},
	     {0, 0, 0, 0, {NULL}}
     };

/*
 * Constructor
 */
Dvb_rcs::Dvb_rcs(u_int32_t type, u_int32_t id, struct plist *pl,
		 const char *name)
:NslObject(type, id, pl, name)
{
	_rs = new ReedSolomonCode();
	_ptrlog = 0;
}


/*
 * Destructor
 */
Dvb_rcs::~Dvb_rcs()
{
	for (int i = 0; _cc_punctured_table[i].rate != 0; i++) {
		if (_cc_punctured_table[i].coding.CC)
			delete _cc_punctured_table[i].coding.CC;
	}
	delete _rs;
}


/*
 * module initialize
 */
int Dvb_rcs::init()
{
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


	assert(cl);
	_link_obj = cl->obj;

	return (NslObject::init());
}


/*
 * Dvb_rcs module must do below mechanism:
 * - randomization
 * - CRC-16
 * - coding (RS/CC or TC)
 * - Modulation (BER)
 */
int Dvb_rcs::send(ePacket_ * event)
{
	struct con_list *cl;
	ePacket_ *ep;
	Dvb_pkt *pkt;
	int sendcount;

	assert(pkt = (Dvb_pkt *) event->DataInfo_);

	u_int64_t       tx_time;

        if (DVBChannelCoding && !strcasecmp(DVBChannelCoding, "on")) {
                _encode_atm_burst(pkt);
        }

	tx_time = _tx_delay(pkt->pkt_getlen(), 55);

	/*
	 * Add link_budget and log_info to SAT for cal BER and log
	 */
	_info.freq = _budget.freq;
	pkt->pkt_getretinfo()->lbudget = _budget;
	pkt->pkt_getretinfo()->linfo= _info;
	pkt->pkt_getretinfo()->loginfo.phy_src = get_nid();
	pkt->pkt_getretinfo()->loginfo.tx_time = tx_time;

	pkt->pkt_setflag(FLOW_RECV);
	pkt->pkt_settype(PKT_DVBRCS);

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(NslObject, get);


	/*
	 * for each connect Link module in linked list of my_link->hos_
	 */
	SLIST_FOREACH(cl, &(((Link *) _link_obj)->hos_), nextLoc) {
		u_int64_t ticks =
			_prop_delay_time(cl->obj) + GetCurrentTime();

		/* log "StartTX" event and "SuccessTX" event*/
		if(_ptrlog == 1){

			struct logEvent* logep;
			dvbrcs_log* ssdvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
			dvbrcs_log* sedvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));

			LOG_SAT_DVBRCS(ssdvbrcs_log, GetCurrentTime(), GetCurrentTime(),
					get_type(), get_nid(), StartTX, get_nid(),
					cl->obj->get_nid(), 0,pkt->pkt_gettype(), pkt->pkt_getlen(), 1, DROP_NONE);
			INSERT_TO_HEAP(logep, ssdvbrcs_log->PROTO, ssdvbrcs_log->Time+START, ssdvbrcs_log);

			LOG_SAT_DVBRCS(sedvbrcs_log, GetCurrentTime()+tx_time , GetCurrentTime(),
					get_type(), get_nid(), SuccessTX, get_nid(),
					cl->obj->get_nid(), 0,pkt->pkt_gettype(), pkt->pkt_getlen(), 1, DROP_NONE);
			INSERT_TO_HEAP(logep, sedvbrcs_log->PROTO, sedvbrcs_log->Time+ENDING, sedvbrcs_log);
		}

		/*
		 * copy a packet to every one 
		 */
		ep = dvb_pkt_copy(event);
		setObjEvent(ep, ticks, 0, cl->obj, type, ep->DataInfo_);

		sendcount++;
	}

	dvb_freePacket(event);
	return (1);
}


/*
 * Dvb_rcs module must do below mechanism:
 * - de-randomization
 * - CRC-16
 * - coding (RS/CC or TC)
 */
int Dvb_rcs::recv(ePacket_ * event)
{
	dvb_freePacket(event);
	return (1);
}

ePacket_ *Dvb_rcs::recv_handler(ePacket_ *event)
{
	Dvb_pkt *pkt;

	assert(pkt = (Dvb_pkt *) event->DataInfo_);
	assert(pkt->pkt_gettype() == PKT_DVBRCS);

	/*
	 * calculate ber
	 */

	if (DVBChannelCoding && !strcasecmp(DVBChannelCoding, "on")) {
		SatErrorModel error_obj;
		/*
		 * Apply the error model if the error simulation is enabled.
		 * This function gets the BER via a certain error model
		 * then chooses some bits from the frame to flip.
		 * frame: the pointer to the frame to apply the error model.
		 * len: the length of the frame.
		 */

		/* 
		 * set up budget parameters
		 */
		_info.freq = _budget.freq;
		_budget.rx_bw   = 55;
		_budget.rx_snt  = 700;
		_budget.tx_pwr= pkt->pkt_getretinfo()->lbudget.tx_pwr;
		_budget.freq= pkt->pkt_getretinfo()->lbudget.freq;
		_budget.st_e = pkt->pkt_getretinfo()->lbudget.st_e;
		_budget.st_dia = pkt->pkt_getretinfo()->lbudget.st_dia;

		/*
		 * Check the rainfade is 'Default' or 'User_define',
		 * then cal BER
		 */
		if(!strcmp(_budget.rainfade_option, "Default"))
			_bit_err_rate = error_obj.ber(UPLINK, _budget);
		else
			_bit_err_rate = error_obj.ber(UPLINK, _budget, _info);

		calculate_bit_error(pkt->pkt_getdata(), pkt->pkt_getretinfo()->encode_data_len, _bit_err_rate);
	}

	/* log "StartRX" event and "SuccessRX" event*/
	if(_ptrlog == 1){
		struct logEvent*        logep;
		uint64_t	rx_time;
		dvbrcs_log* rsdvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
		dvbrcs_log* redvbrcs_log = (dvbrcs_log*)malloc(sizeof(dvbrcs_log));
		rx_time = _tx_delay(pkt->pkt_getlen(), 55);

		LOG_SAT_DVBRCS(rsdvbrcs_log, GetCurrentTime(), GetCurrentTime(),
				get_type(), get_nid(), StartRX,pkt->pkt_getretinfo()->loginfo.phy_src ,
				get_nid(), 0,pkt->pkt_gettype(), pkt->pkt_getlen(), 1, DROP_NONE);
		INSERT_TO_HEAP(logep, rsdvbrcs_log->PROTO, rsdvbrcs_log->Time+START, rsdvbrcs_log);

		LOG_SAT_DVBRCS(redvbrcs_log, GetCurrentTime()+rx_time , GetCurrentTime(),
				get_type(), get_nid(), SuccessRX,pkt->pkt_getretinfo()->loginfo.phy_src ,
				get_nid(), 0,pkt->pkt_gettype(), pkt->pkt_getlen(), 1, DROP_NONE);
		INSERT_TO_HEAP(logep, redvbrcs_log->PROTO, redvbrcs_log->Time+ENDING, redvbrcs_log);
	}

	/*
	 * The pkt will send to NCC or SP,
	 * then reset the phy_src to GW node id.
	 */
	pkt->pkt_getretinfo()->loginfo.phy_src = get_nid();

	if (_sp_name || _ncc_name) {
		/*
		 * Apply the error model if the error simulation is enabled.
		 */
		if (DVBChannelCoding && !strcasecmp(DVBChannelCoding, "on")) {
			_decode_atm_burst(pkt);
		}
	}

	return event;
}

/*
 * CSC burst, must do ranomization, CRC16, CC
 * CSC burst, must do ranomization, CRC16, RS, CC
 * ATM TRF burst, must do ranomization, RS, CC
 */
void Dvb_rcs::_encode_atm_burst(Dvb_pkt * pkt)
{
	char *rand_output, *inner_output;
	int pkt_len_outer, pkt_len_inner, len;
	struct slot_info *timeslot;
	ConvoCode *CC;

	assert(pkt);
	timeslot = &pkt->pkt_getretinfo()->timeslot;

	/*
	 * outer_coding == X1b => enable RS
	 * outer_coding == 1Xb => enable CRC
	 */
	if (timeslot->outer_coding & 0x01) {
		pkt_len_outer = pkt->pkt_getlen() + RS_PARITY_LENGTH;
	}

	/*
	 * do CRC16
	 */
	if (timeslot->outer_coding & 0x02) {
		pkt_len_outer = pkt->pkt_getlen();
		assert(0);
	}

	/*
	 * calcuate packet encode length for CC or TC
	 */
	pkt_len_inner =
		(int)(pkt_len_outer /
		      _cc_punctured_table[timeslot->inner_code_puncturing].
		      rate);

	/*
	 * memory allocate encode buffer
	 */
	rand_output = new char[pkt_len_outer];
	memset(rand_output, 0, sizeof (char) * pkt_len_outer);

	inner_output = new char[pkt_len_inner + 1];
	memset(inner_output, 0, sizeof (char) * (pkt_len_inner + 1));

	/*
	 * do randomization
	 */
	char *payload;

	payload = ((struct atm_burst *)pkt->pkt_getdata())->payload;

	_randomization((char *)pkt->pkt_getdata(),
		       rand_output + RS_PARITY_LENGTH,
		       pkt->pkt_getlen() * 8);

	/*
	 * do Reed-Solomon encode
	 */
	if (timeslot->outer_coding & 0x01) {
		_rs->encode(rand_output + RS_PARITY_LENGTH,
			    rand_output, pkt->pkt_getlen(),
			    RS_PARITY_LENGTH);
	}

	/*
	 * inner_code_type == 0 => CC
	 * inner_code_type == 1 => TC
	 */
	if (timeslot->inner_code_type == 0) {
		/*
		 * do Convolution encode
		 */
		_create_cc_table(timeslot->inner_code_puncturing);
		CC = _cc_punctured_table[timeslot->inner_code_puncturing].
			coding.CC;

		len = CC->encode(rand_output, inner_output,
				 pkt_len_outer * 8);
	}
	else {
		/*
		 * do Turbo encode
		 */
		assert(0);
	}
	pkt->pkt_getretinfo()->encode_data_len = len;
	delete rand_output;

	/*
	 * replace payload data from atm_burst to encode atm_burst
	 */
	pkt->pkt_attach(inner_output, pkt_len_inner);
}


/*
 * decode atm burst, must do de-randomization, RS/CC or TC
 */
void Dvb_rcs::_decode_atm_burst(Dvb_pkt * pkt)
{
	struct atm_burst *burst;
	char *rand_output, *inner_output, *outer_output;
	int pkt_len_inner, pkt_len_outer, pkt_len, bit_len;
	struct slot_info *timeslot;
	ConvoCode *CC;

	timeslot = &pkt->pkt_getretinfo()->timeslot;

	bit_len = pkt->pkt_getretinfo()->encode_data_len;
	pkt_len_inner =
		(int)(bit_len *
		      _cc_punctured_table[timeslot->inner_code_puncturing].
		      rate) / 8;

	/*
	 * memory allocate decode for inner buffer
	 */
	inner_output = new char[pkt_len_inner + 1];
	memset(inner_output, 0, sizeof (char) * (pkt_len_inner + 1));

	/*
	 * inner_code_type == 0 => CC
	 * inner_code_type == 1 => TC
	 */
	if (timeslot->inner_code_type == 0) {
		/*
		 * do Convolution encode
		 */
		_create_cc_table(timeslot->inner_code_puncturing);
		CC = _cc_punctured_table[timeslot->inner_code_puncturing].
			coding.CC;

		pkt_len_outer =
			CC->decode((char *)pkt->pkt_getdata(),
				   inner_output, bit_len) / 8;

		pkt_len = pkt_len_outer - RS_PARITY_LENGTH;

		/*
		 * memory allocate decode for outer buffer
		 */
		outer_output = new char[pkt_len_outer];
		memset(outer_output, 0, sizeof (char) * pkt_len_outer);
	}
	else {
		/*
		 * do Turbo decode
		 */
		assert(0);
	}

	/*
	 * do Reed-Solomon decode
	 */
	if (timeslot->outer_coding & 0x01) {
		_rs->decode(inner_output, outer_output, pkt_len_outer,
			    RS_PARITY_LENGTH);
		delete inner_output;
	}
	else {
		outer_output = inner_output;
	}

	/*
	 * do CRC16
	 */
	if (timeslot->outer_coding & 0x02) {
		pkt_len_outer = pkt->pkt_getlen();
		assert(0);
	}

	/*
	 * do de-randomization, just randomization again
	 */
	rand_output = new char[pkt_len];

	memset(rand_output, 0, sizeof (rand_output));

	_randomization(outer_output, rand_output, pkt_len * 8);
	delete outer_output;

	/*
	 * create struct atm_burst and calcuate number of atm cell in atm burst
	 */
	burst = new struct atm_burst;

	memcpy(burst->payload, rand_output, pkt_len);
	delete rand_output;

	burst->cell_cnt = pkt_len / ATM_CELL_LEN;

	/*
	 * replace payload data from atm_burst to encode atm_burst
	 */
	pkt->pkt_attach(burst, pkt_len);
}


/*
 * randomization function
 */
int Dvb_rcs::_randomization(char *input, char *output, int inputLen)
{
	unsigned int seed = RANDOMIZER_SEED;
	int in, out, tmp;

	for (int i = 0; i < inputLen; i++) {
		// Extract 1-bit input
		in = ((input[i / 8] << (i % 8)) & 0x80) >> 7;

		tmp = (seed & 0x1) ^ ((seed & 0x2) >> 1);
		out = in ^ tmp;
		seed = (seed >> 1) | (tmp << 14);

		output[i / 8] <<= 1;
		output[i / 8] |= out;
	}

	return inputLen;
}

void Dvb_rcs::_create_cc_table(inner_coding_puncture type)
{
	if (_cc_punctured_table[type].coding.CC == NULL) {
		ConvoCode *CC = new ConvoCode(CC_X_GEN_POLY, CC_Y_GEN_POLY,
					      CC_GEN_LEN,
					      _cc_punctured_table[type].PX,
					      _cc_punctured_table[type].PY,
					      _cc_punctured_table[type].
					      PLen);

		_cc_punctured_table[type].coding.CC = CC;

		/*
		 * create encode/decode lookup table
		 */
		CC->setEncoderLookupTable(CC->genEncoderLookupTable());
		CC->setDecoderLookupTable(CC->genDecoderLookupTable());
	}
}

/*
 * calcaute propagation delay time
 */
u_int64_t Dvb_rcs::_prop_delay_time(NslObject * obj)
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

/*
 * calculate tx_dealy time
 */
u_int64_t Dvb_rcs::_tx_delay(int len, int bw)
{
        u_int64_t       tx_delay_in_tick;
        u_int64_t       tx_delay_in_ns;
        tx_delay_in_ns = (len*8/((u_int64_t)bw))*1000;
        NANO_TO_TICK(tx_delay_in_tick, tx_delay_in_ns);
        return tx_delay_in_tick;
}

int
Dvb_rcs::_parse_nodeid_cfg(char *filename, list<dvbrcs_node_id> &global_system_node_id)
{
        global_system_node_id.clear();
        FILE    *nodeid_cfg;
        if (!(nodeid_cfg = fopen(filename, "r"))) {
                printf("[RCST_CTL] Warning: Cannot open file %s", filename);
                assert(0);
        }   
        else {

                char    line[200];
                char    buf[200];
                while (fgets(line, 200, nodeid_cfg)) {
                        dvbrcs_node_id  one_dvbrcs_node_id;
                        if (sscanf(line, " DVB-RCS:%s", buf)) {
                                char*   tok;
                                tok = strtok(buf, "_");
                                assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.sat_nid)));
                                tok = strtok(NULL, "_");
                                assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.feeder_nid)));
                                tok = strtok(NULL, "_");
                                assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.gw_nid)));
                                tok = strtok(NULL, "_");
                                assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.sp_nid)));
                                tok = strtok(NULL, "_");
                                assert(sscanf(tok, "%u", &(one_dvbrcs_node_id.ncc_nid)));

                                while ((tok = strtok(NULL, "_"))) {
                                        struct rcst_node_id     one_rcst_node_id;
                                        assert(sscanf(tok, "%u", &one_rcst_node_id.rcst_nid));
                                        one_dvbrcs_node_id.rcst_nid_list.push_back(one_rcst_node_id);
                                }
                        }
                        else if (sscanf(line, " #%s", buf))
                                continue;

                        global_system_node_id.push_back(one_dvbrcs_node_id);
                }
		fclose(nodeid_cfg);
                return 1;
        }
}

