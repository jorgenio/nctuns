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

#include <nctuns_api.h>

#include "dvb_s2.h"
#include "../rcst/rcst_ctl.h"


MODULE_GENERATOR(Dvb_s2);

extern char *DVBChannelCoding;

/*
 * Member functions definition of class Dvb_s2.
 */

/*
 * Constructor
 */
Dvb_s2::Dvb_s2(uint32_t type, uint32_t id, struct plist *pl, const char *name)
: NslObject(type, id, pl, name)
, _coding_type(Dvb_s2_pls::COD_NORMAL_1_4)
{
	/*
	 * DVB-S.2 module parameters.
	 */
	vBind("InterfaceNumber", (int*)&_number_of_input_stream);
	vBind("SymbolRate", &_symbol_rate); /* Mega symbols per second */

	/*
	 * Initial physical layer signalling.
	 */
	_dvb_s2_pls = new Dvb_s2_pls();

	/*
	 * The object is used to form the base-band header.
	 */
	_base_band_header = new Base_band_header();
}


/*
 * Destructor
 */
Dvb_s2::~Dvb_s2()
{
	delete _base_band_header;
	delete _dvb_s2_pls;
}


/*
 * Calcaute the transmission delay.
 * - mod_type: the modulation type in current configuration.
 * - len: the size of the packet to transmit. (in bit)
 * - return value: the transmission delay in tick.
 */
uint64_t
Dvb_s2::_tx_delay(enum Dvb_s2_pls::modulation_type mod_type, unsigned int len)
{
	/*
	 * Calculate number of symbols to transmit.
	 */
	unsigned int	number_of_symbol;
	unsigned int	number_of_slot;

	switch (_dvb_s2_pls->modulation_type()) {
	case Dvb_s2_pls::MOD_DUMMY_PLFRAME:
		number_of_symbol =
			DVB_PL_DUMMY_FRAME_SLOT * DVB_PL_SYMBOL_PER_SLOT;
		break;

	case Dvb_s2_pls::MOD_QPSK:
		number_of_symbol = len / 2;
		number_of_slot = number_of_symbol / DVB_PL_SYMBOL_PER_SLOT;
		break;

	case Dvb_s2_pls::MOD_8PSK:
		number_of_symbol = len / 3;
		number_of_slot = number_of_symbol / DVB_PL_SYMBOL_PER_SLOT;
		break;

	case Dvb_s2_pls::MOD_16APSK:
		number_of_symbol = len / 4;
		number_of_slot = number_of_symbol / DVB_PL_SYMBOL_PER_SLOT;
		break;

	case Dvb_s2_pls::MOD_32APSK:
		number_of_symbol = len / 5;
		number_of_slot = number_of_symbol / DVB_PL_SYMBOL_PER_SLOT;
		break;

	default:
		assert(0);
	};
	/*
	 * Pilot block insertion if the frame is not a dummy frame and
	 * the physical frame configured with pilot insertion.
	 */
	if (_dvb_s2_pls->modulation_type() != Dvb_s2_pls::MOD_DUMMY_PLFRAME
	&& _dvb_s2_pls->pilot_conf() == Dvb_s2_pls::PILOT_ON)
		number_of_symbol +=
			((number_of_slot - 1) / DVB_PL_SLOT_PER_PILOT_INSERT)
			* DVB_PL_SYMBOL_PER_PILOT_BLOCK;
	/*
	 * At last, add the number of symbol of the physical layer header.
	 */
	number_of_symbol += DVB_PL_PLHEADER_SYMBOL;

	/*
	 * Calculate the transmission delay.
	 */
	double		symbol_per_sec;
	double		tx_delay_in_sec;

	symbol_per_sec = _symbol_rate * 1000000;
	tx_delay_in_sec = (double)number_of_symbol / symbol_per_sec;

	/*
	 * Round down if the difference < 0.5, else round up.
	 */
	uint64_t	tx_delay;
	double		tx_delay_in_ns;
	double		tx_delay_in_tick;
	double		difference;

	tx_delay_in_ns = 1000000000 * tx_delay_in_sec;
	tx_delay_in_tick = tx_delay_in_ns / TICK;
	NANO_TO_TICK(tx_delay, tx_delay_in_ns);
	difference = (tx_delay_in_tick - tx_delay);
	if (difference >= 0.5)
		tx_delay++;
	return tx_delay;
}


/*
 * Perform a 8-bit CRC encoding on usr packet excluding first byte,
 * which is TS SYNC field, with the generator polynomial.
 * - user_packet: pointer to the user packet.
 * - len: length of the user packet.
 * - gen_poly: generator polynomial.
 * - return value: computed CRC-8 of this user packet.
 */
uint8_t
Dvb_s2::_crc_8(void* user_packet, unsigned int len, uint8_t gen_poly)
{
	uint8_t	shift_reg;
	int8_t*	bit_string;

	shift_reg = 0;
	bit_string = (int8_t*)user_packet;
	for (unsigned int i = 0 ; i < len ; i++) {
		for (int j = 0 ; j < 8 ; j++) {
			/*
			 * Most significant bit of shift register XOR with
			 * (8 * i + j)-th bit of the bit string.
			 */
			if ((shift_reg & 0x80) ^
				((bit_string[i] & (0x80 >> j)) << j)) {
				shift_reg = (shift_reg << 1) ^ gen_poly;
			} else {
				shift_reg = shift_reg << 1;
			}
		}
	}

	return shift_reg;
}


/*
 * The function generates scrambling sequence by the feed-back shift register.
 * The polynomial for the pseudo random binary sequence generator is:
 * 	x^15 + x^14 + 1
 * Initial sequence is:
 * 	0b100101010000000 = 0x4a80
 * frame: the pointer to the frame to be scrambled.
 * frame_len: length of the frame.
 */
void
Dvb_s2::_bb_scrambler(void* frame, unsigned int frame_len)
{
	int8_t*		byte;
	uint16_t	prbs_reg;

	/*
	 * Initialize pseudo random binary sequence register, which is
	 * a 15-bit shift register.
	 */
	prbs_reg = 0x4a80;

	/*
	 * Scramble the frame.
	 */
	byte = (int8_t*)frame;
	for (unsigned int i = 0 ; i < frame_len ; i++) {
		for (int j = 0 ; j < 8 ; j++) {
			/*
			 * XOR 14-th and 15-th bit of PRBS register.
			 */
			if (((prbs_reg & 0x0002) >> 1) ^ (prbs_reg & 0x0001)) {
				/*
				 * Set the MSB of the PRBS to 1 and
				 * scramble the (8 * i + j)-th bit.
				 */
				prbs_reg |= 0x8000;
				byte[i] ^= (0x80 >> j);
			} else {
				/*
				 * Set the MSB of the PRBS to 0.
				 */
				prbs_reg &= 0x7fff;
			}
			/*
			 * Shift right the PRBS register.
			 */
			prbs_reg >>= 1;
		}
	}
}


/*
 * Sub-procedure of FEC encoding. Data block is written into the interleaver
 * column-wise, and serially read out row-wise.
 * block: the pointer to the frame to be interleaved.
 * m: number of row
 * n: number of column
 */
void
Dvb_s2::_bit_interleaver(void* block, unsigned int m, unsigned int n)
{
	char	interleaved_frame[m * n / 8];

	memset(interleaved_frame, 0, sizeof(interleaved_frame));
	for (unsigned int i = 0 ; i < m ; i++) {
		for (unsigned int j = 0 ; j < n ; j++) {
			if (((int8_t*)block)[(m * j + i) / 8]
				& (0x80 >> ((m * j + i) % 8)))
				interleaved_frame[(n * i + j) / 8] ^=
					(0x80 >> ((n * i + j) % 8));
		}
	}
	memcpy(block, interleaved_frame, sizeof(interleaved_frame));
}


/*
 * Sub-procedure of FEC decoding. Data block is written into the de-interleaver
 * row-wise, and serially read out column-wise.
 * block: the pointer to the frame to be de-interleaved.
 * m: number of row
 * n: number of column
 */
void
Dvb_s2::_bit_deinterleaver(void* block, unsigned int m, unsigned int n)
{
	_bit_interleaver(block, n, m);
}


/*
 * Member functions definition of class Stream_buffer.
 */

/*
 * Constructor
 */
Dvb_s2::Stream_buffer::Stream_buffer(uint32_t size)
: _front(0)
, _rear(0)
, _size(size)
, _free(size)
{
	_data = new uint8_t[size];
}


/*
 * Destructor
 */
Dvb_s2::Stream_buffer::~Stream_buffer()
{
	delete[] _data;
}


/*
 * Get data from the buffer. If the specific length is larger than total
 * data in the buffer, all data in the buffer will be detached.
 * - data: pointer to an available space for data with specific data length.
 * - len: length of the input data.
 * - return value: number of byte get from the buffer.
 */
unsigned int
Dvb_s2::Stream_buffer::get(void* data, unsigned int len)
{
	//printf("\e[1;34mget start: front = %u, len = %u\n", _front, len);
	if (len > content_len())
		len = content_len();

	unsigned int cont_data_len;

	/*
	 * Compute the length of data stored after _front.
	 */
	cont_data_len = _size - _front;
	if (len <= cont_data_len)
		memcpy(data, _data + _front, len);
	else {
		memcpy(data, &_data[_front], cont_data_len);
		memcpy(&((uint8_t*)data)[cont_data_len],
			_data,
			len - cont_data_len);
	}
	_front = (_front + len) % _size;
	_free += len;

	return len;
}


/*
 * Put data into the buffer.
 * - data: pointer to the input data.
 * - len: length of the input data.
 * - return value: -1 if there is no free space for the input data
 *                 while 0 if the data is successfully put in the buffer.
 */
int
Dvb_s2::Stream_buffer::put(void* data, unsigned int len)
{
	if (len > _free)
		return -1;

	unsigned int cont_free_len;

	/*
	 * Compute the free buffer space after _rear.
	 */
	cont_free_len = _size - _rear;
	if (len <= cont_free_len)
		memcpy(_data + _rear, data, len);
	else {
		memcpy(&_data[_rear], data, cont_free_len);
		memcpy(_data,
			&((uint8_t*)data)[cont_free_len],
			len - cont_free_len);
	}
	_rear = (_rear + len) % _size;
	_free -= len;

	return 0;
}


/*
 * Member functions definition of class Base_band_header.
 */

/*
 * Constructor
 */
Dvb_s2::Base_band_header::Base_band_header()
: _ts_gs_field(IF_TYPE_TS)
, _sis_mis_field(IS_TYPE_MULTIPLE)
, _ccm_acm_field(CM_TYPE_ACM)
, _input_stream_sync_indicator(ISSYI_NOT_ACT)
, _null_packet_deletion(NPD_NOT_ACT)
, _roll_off(RO_RESERVED)
, _user_packet_length(MPEG2_TS_LEN * 8)
, _sync(SYNC_MPEG2_TS)
{
}

int
Dvb_s2::_parse_nodeid_cfg(char *filename, list<dvbrcs_node_id> &global_system_node_id)
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

