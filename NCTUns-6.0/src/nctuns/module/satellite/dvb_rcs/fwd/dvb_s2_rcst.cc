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
#include <timer.h>
#include <nctuns_api.h>

#include "dvb_s2_rcst.h"
#include "../common/dvb_pkt.h"
#include "../common/dvbrcs_api.h"
#include "../common/fec/bch.h"
#include <if_tun.h>
#include <sys/ioctl.h>


MODULE_GENERATOR(Dvb_s2_rcst);

extern char *DVBChannelCoding;

/*
 * Member functions definition of class Dvb_s2_rcst.
 */

/*
 * Constructor
 */
Dvb_s2_rcst::Dvb_s2_rcst(uint32_t type, uint32_t id, struct plist *pl, const char *name)
: Dvb_s2(type, id, pl, name)
, _state(DVB_S2_IDLE)
, _recv_buffer(NULL)
, _ldpc_iteration_threshold(0)
, _byte_recv(0)
, _prev_tick(0)
{
	/*
	 * DVB-S.2 RCST module parameters.
	 */
	vBind("LDPCIterationThreshold", (int*)&_ldpc_iteration_threshold);
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
	/*
	 * Initial timers.
	 */
	_rx_timer = new timerObj();

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
Dvb_s2_rcst::~Dvb_s2_rcst()
{
	for (int i = 0 ; i < _number_of_input_stream ; i++)
		delete _incomplete_up_buf[i];

	delete[] _incomplete_up_buf;
	delete _rx_timer;
}


/*
 * Initialize the DVB-S.2 RCST system.
 * The function will be called by tclObject.
 */
int
Dvb_s2_rcst::init()
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

			typeStart = POINTER_TO_MEMBER(Dvb_s2_rcst, TurnOnLinkFailFlag);
			typeStop  = POINTER_TO_MEMBER(Dvb_s2_rcst, TurnOffLinkFailFlag);

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
	 * Initialize incomplete user packet buffers.
	 */
	_incomplete_up_buf = new Stream_buffer*[_number_of_input_stream];
	for (int i = 0 ; i < _number_of_input_stream ; i++)
		_incomplete_up_buf[i] = new Stream_buffer(MPEG2_TS_LEN);

	/*
	 * Initial timers.
	 */
	_rx_timer->init();
	_rx_timer->setCallOutObj(this,
		(int (NslObject::*)(Event_*))&Dvb_s2_rcst::_rx_handler);

	/*
	 * _ldpc_iteration_threshold can't be zero.
	 */
	if (!_ldpc_iteration_threshold) {
		fprintf(stderr, "\n%s: Error: The LDPC decoder iteration "
				"threshold should be non-zero.\n", __func__);
		exit(EXIT_FAILURE);
	}

	/* 
	 * set up Uplink budget parameters
	 */
	SatErrorModel error_obj;
	return Dvb_s2::init();
}


/*
 * Receive the packet from the satellite. The function is only called
 * in the RCST node.
 */
int
Dvb_s2_rcst::recv(ePacket_* event)
{
	class Dvb_pkt*	packet;

	assert(event && (packet = (Dvb_pkt*)event->DataInfo_));

	/*
	 * if link fail flag is on, then stop to recv
	 */
	if ( LinkFailFlag > 0 ) {
		dvb_freePacket(event);
		return(1);
	}

	/*
	 * if packet type is not DVBRCS, drop it
	 */
	if (packet->pkt_gettype() == PKT_DVBRCS) {
		dvb_freePacket(event);
		return 1;
	}

	assert(packet->pkt_gettype() == PKT_DVB_S2);
	assert(packet->pkt_getflag() == FLOW_RECV);
	/*
	 * Release the event.
	 */
	event->DataInfo_ = NULL;
	dvb_freePacket(event);

	uint64_t	rx_time;
	switch (_state) {
	case DVB_S2_IDLE:
		_state = DVB_S2_RECV;
		/*
		 * Get the modulation type from the physical frame preamble.
		 */
		_dvb_s2_pls->copy_info_from(
				packet->pkt_getfwdinfo()->dvb_s2_pls);
		/*
		 * Get the symbol rate from the Dvb_pkt
		 * to simulate receiving delay.
		 */
		_symbol_rate = packet->pkt_getfwdinfo()->symbol_rate;
		/*
		 * Simulate the receiving delay.
		 */
		rx_time = _rx_delay(
				_dvb_s2_pls->modulation_type(),
				packet->pkt_getlen());
		//rx_time = 5uLL; /* 500 ns */
		_rx_timer->start(rx_time, 0);
		/*
		 * Buff the receiving packet.
		 */
		_recv_buffer = packet;
		break;

	case DVB_S2_RECV:
		/*
		 * Impossible in present design.
		 */
		assert(0);
		break;
	};

	return 1;
}


/*
 * The RCST should not send any packet via forward channel.
 */
int
Dvb_s2_rcst::send(ePacket_* event)
{
	assert(0);
}


/*
 * This function is called by _rx_timer when previous receiving
 * is completed.
 */
int
Dvb_s2_rcst::_rx_handler(Event_*)
{
	assert(_recv_buffer);

	void*		physical_layer_frame;
	unsigned int	fec_frame_size;

	switch (_state) {
	case DVB_S2_IDLE:
		/*
		 * Impossible in present design.
		 */
		assert(0);
		break;

	case DVB_S2_RECV:
		_state = DVB_S2_IDLE;
		/*
		 * Preamble processing. Load physical layer signalling for
		 * user packet receiving processing.
		 */
		_dvb_s2_pls->copy_info_from(
				_recv_buffer->pkt_getfwdinfo()->dvb_s2_pls);
		/*
		 * Get the FEC frame size for the error model.
		 * The size is in bit.
		 */
		fec_frame_size = _recv_buffer->pkt_getlen() * 8;
		/*
		 * Detach the physical layer frame from the receiving buffer.
		 */
		physical_layer_frame = _recv_buffer->pkt_detach();
		/*
		 * Skip processing if dummy frame is received. The user packet
		 * of the dummy frame should be NULL.
		 */
		if (_dvb_s2_pls->modcod() == Dvb_s2_pls::MODCOD_DUMMY_PLFRAME) {
			assert(physical_layer_frame == NULL);
			break;
		}
		/*
		 * Set _coding_type via the received physical layer signalling,
		 * this variable will be used in _fec_decoding(),
		 * _bb_scrambler() and _demultiplexer() .
		 */
		_coding_type = _dvb_s2_pls->coding_type();
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
			_budget.rx_bw	= 55;
			_budget.rx_snt	= 700;
			_budget.tx_pwr = _recv_buffer->pkt_getfwdinfo()->lbudget.tx_pwr;
			_budget.freq = _recv_buffer->pkt_getfwdinfo()->lbudget.freq;
			_budget.st_e = _recv_buffer->pkt_getfwdinfo()->lbudget.st_e;
			_budget.st_dia = _recv_buffer->pkt_getfwdinfo()->lbudget.st_dia;

			/*
			 * Check the rainfade is 'Default' or 'User_define',
			 * then cal BER
			 */
			if(!strcmp(_budget.rainfade_option, "Default"))
				_bit_err_rate = error_obj.ber(UPLINK, _budget);
			else
				_bit_err_rate = error_obj.ber(UPLINK, _budget, _info);


			/*
			 * Apply the error model if the error simulation is enabled.
			*
			 * This function gets the BER via a certain error model
			 * then chooses some bits from the frame to flip.
			 * frame: the pointer to the frame to apply the error model.
			 * len: the length of the frame.
			 */
			/*int num = */calculate_bit_error(physical_layer_frame, fec_frame_size, _bit_err_rate);

			/*
			 * Apply FEC decoding if FEC system is enabled.
			 */
			_fec_decoding(physical_layer_frame);
		}

		_bb_scrambler(
			physical_layer_frame,
			_fec_parameter_table[_coding_type][BCH_UNCODED] / 8
			);
		/*
		 * De-multiplexer the base-band frame which is the
		 * decoded physical layer frame.
		 */
		_demultiplexer(physical_layer_frame);
		delete _recv_buffer;
		_recv_buffer = NULL;
		delete (int8_t*)physical_layer_frame;
		break;
	};

	return 0;
}


/*
 * Transport stream de-multiplexer. It de-multiplexes the base band frame
 * to one or more transport stream.
 * - base_band_frame: the pointer to the base band frame to be de-multiplexed.
 */
void
Dvb_s2_rcst::_demultiplexer(void* base_band_frame)
{
	uint8_t*	data_field;
	uint16_t	data_field_len;
	uint16_t	i;
	uint8_t		id;
	void*		user_packet;

	_base_band_header->apply_base_band_header(base_band_frame);
	/*
	 * Apply CRC-8 error detection to the first 9 bytes of base-band header.
	 * If the CRC-8 check failed, drop the received base-band frame.
	 */
	if (_base_band_header->crc_8() != _crc_8(
				_base_band_header,
				_base_band_header->header_size() - 1,
				DVB_CRC8_GEN_POLY)) {
		/*
		 * Skip processing the base-band-frame with error.
		 */
#if 0
		printf("\e[1;36m[DROP] %s: base-band header CRC-8 "
				"check failed, drop the base-band frame.\e[m\n",
				__func__);
#endif
		return;
	}
	/*
	 * Get the stream identification, which is corresponding to
	 * the input interface in the feeder, and the data field via
	 * the received base-band header.
	 */
	id = _base_band_header->input_stream_id();
	/*
	 * The specific interface should not exceed
	 * the number of input streams.
	 */
	assert(id < _number_of_input_stream);
	data_field = (uint8_t*)_base_band_header->data_field(base_band_frame);
	/*
	 * Check if length of the incompleted user packet in the receiving
	 * buffer match the first user packet received.
	 */
	if (_incomplete_up_buf[id]->content_len()
	+ _base_band_header->syncd() / 8 == MPEG2_TS_LEN - 1) {
		/*
		 * Recompose incomplete user packet.
		 */
		user_packet = new uint8_t[MPEG2_TS_LEN];
		_incomplete_up_buf[id]->put(
			data_field,
			_base_band_header->syncd() / 8 + 1
			);
		/*
		 * The first byte of the user packet is reserved for sync-byte
		 * of transport stream, so we put CRC-8 of the user packet at
		 * this byte to let _recv_user_packet() check it.
		 */
		_incomplete_up_buf[id]->get(
				(uint8_t*)user_packet + 1, MPEG2_TS_LEN - 1);
		_incomplete_up_buf[id]->get(user_packet, 1);

		assert(!_incomplete_up_buf[id]->content_len());

		_recv_user_packet(id, user_packet);
	}
	/*
	 * Process the user packet after the first one.
	 */
	i = _base_band_header->syncd() / 8 + 1;
	data_field_len = _base_band_header->data_field_length() / 8;
	for (  ; i + MPEG2_TS_LEN <= data_field_len ; i += MPEG2_TS_LEN ) {
		user_packet = new uint8_t[MPEG2_TS_LEN];
		/*
		 * The first byte of the user packet is reserved for sync-byte
		 * of transport stream, so we put CRC-8 of the user packet at
		 * this byte to let _recv_user_packet() check it.
		 */
		memcpy(&((uint8_t*)user_packet)[1],
				&data_field[i], MPEG2_TS_LEN - 1);
		((uint8_t*)user_packet)[0] = data_field[i + MPEG2_TS_LEN - 1];
		//memcpy(user_packet, &data_field[i], 1);

		_recv_user_packet(id, user_packet);
	}
	/*
	 * Put incompleted-received user packet into the buffer.
	 */
	_incomplete_up_buf[id]->put(&data_field[i], data_field_len - i);
}


/*
 * Sub-procedure of de-multiplexer. It generates packet to be received by
 * upper layer.
 * - interface: the interface to receive the user packet.
 * - user_packet: the pointer to the user packet.
 */
void
Dvb_s2_rcst::_recv_user_packet(uint8_t interface, void* user_packet)
{
	/*
	 * If CRC-8 check fails, drop the user packet.
	 */
	if (((uint8_t*)user_packet)[0]
	!= _crc_8(
		(uint8_t*)user_packet + 1,
		MPEG2_TS_LEN - 1,
		DVB_CRC8_GEN_POLY)
	) {
#if 0
		printf("\e[1;36m[DROP] %s: drop user packet due to "
				"CRC-8 check error.\e[m\n", __func__);
#endif
		delete (uint8_t*)user_packet;
		return;
	}

	/*
	 * Add sync-byte to the first byte of the user packet.
	 */
	((uint8_t*)user_packet)[0] = SYNC_MPEG2_TS;

	/*
	 * If input stream synchroniser or null packet deletion functionalities
	 * is added in the future, the additional processing should be performed
	 * at here.
	 */

	/*
	 * Generate the packet.
	 */
	class Dvb_pkt*	packet;
	struct event*	event;

	packet = new Dvb_pkt();
	packet->pkt_attach(user_packet, MPEG2_TS_LEN);
	packet->pkt_setflag(FLOW_RECV);
	packet->pkt_settype(PKT_MPEG2_TS);
	packet->pkt_getfwdinfo()->interface = interface;

	event = new struct event;
	event->DataInfo_ = packet;

	/*
	 * Accounting receving rate every second.
	 */
	_byte_recv += 188;
	if (GetCurrentTime() - _prev_tick > 10000000) {
		_prev_tick = GetCurrentTime();
		_byte_recv = 0;
	}

	assert(NslObject::recv(event));
}


/*
 * The procedure performs forward error correction decoding, which includes
 * outer coding (BCH), inner coding (LDPC) and bit de-interleaving.
 * - physical_layer_frame: the pointer to the frame to be decoded.
 */
int
Dvb_s2_rcst::_fec_decoding(void* physical_layer_frame)
{
	/*
	 * Apply proper bit de-interleaving.
	 */
	switch (_dvb_s2_pls->modulation_type()) {
	case Dvb_s2_pls::MOD_QPSK:
		break;
	case Dvb_s2_pls::MOD_8PSK:
		_bit_deinterleaver(
			physical_layer_frame,
			_dvb_s2_pls->fec_frame_size()
				/ DVB_8PSK_INTERLEAVER_COL,
			DVB_8PSK_INTERLEAVER_COL
			);
		break;
	case Dvb_s2_pls::MOD_16APSK:
		_bit_deinterleaver(
			physical_layer_frame,
			_dvb_s2_pls->fec_frame_size()
				/ DVB_16PSK_INTERLEAVER_COL,
			DVB_16PSK_INTERLEAVER_COL
			);
		break;
	case Dvb_s2_pls::MOD_32APSK:
		_bit_deinterleaver(
			physical_layer_frame,
			_dvb_s2_pls->fec_frame_size()
				/ DVB_32PSK_INTERLEAVER_COL,
			DVB_32PSK_INTERLEAVER_COL
			);
		break;
	default:
		assert(0);
	}
	/*
	 * Apply inner decoding.
	 */
	if (_ldpc_decoder(physical_layer_frame) < 0)
	{
		/*printf ("\e[31m%s: LDPC decoding may be fail.\e[m\n",
				get_name())*/;/* drop? */
	}
	/*
	 * Apply outer decoding.
	 */
	if (_bch_decoder(physical_layer_frame) < 0)
	{
#if 0
		printf ("\e[31m%s: BCH decoding failed.\e[m\n",
				get_name());/* drop? */
#endif
	}

	return 0;
}


/*
 * Sub-procedure of FEC decoding. BCH decoding invoker.
 * bch_block: the pointer to the block to be decoded.
 */
int
Dvb_s2_rcst::_bch_decoder(void* bch_block)
{
	enum _dvb_bch_mode	bch_mode;
	bch_mode = _bch_mode_table[_coding_type];
	return _bch[bch_mode]->decoder(
		bch_block, _fec_parameter_table[_coding_type][BCH_CODED]);
}


/*
 * Sub-procedure of FEC decoding. LDPC decoding invoker.
 * ldpc_block: the pointer to the block to be decoded.
 */
int
Dvb_s2_rcst::_ldpc_decoder(void* ldpc_block)
{
	switch (_dvb_s2_pls->fec_frame_size()) {
	case Dvb_s2_pls::FECFRAME_NORMAL:
		return _ldpc[_coding_type]->decoder(
			ldpc_block, DVB_FECFRAME_SIZE_NORMAL,
			_ldpc_iteration_threshold);

	case Dvb_s2_pls::FECFRAME_SHORT:
		return _ldpc[_coding_type]->decoder(
			ldpc_block, DVB_FECFRAME_SIZE_SHORT,
			_ldpc_iteration_threshold);

	default:
		assert(0);
	};
}

void Dvb_s2_rcst::TurnOnLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_DOWN;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag++;
}

void Dvb_s2_rcst::TurnOffLinkFailFlag(Event_ *ep){

	if (tunfd_ != NULL) {
		unsigned long flag = TUN_UP;
		ioctl(*tunfd_, TUNSETUD, (void *)&flag);
	}

	LinkFailFlag--;
}

