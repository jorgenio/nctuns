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

#include <assert.h>
#include "dvb_s2_pls.h"


/*
 * Member functions definition of class Physical_layer_signalling.
 */

/*
 * Constructors
 */
Dvb_s2_pls::Dvb_s2_pls()
{
	_info._modcod = MODCOD_QPSK_1_4;
	_info._fec_frame_size = FECFRAME_NORMAL;
	_info._pilot_conf = PILOT_OFF;
}

Dvb_s2_pls::Dvb_s2_pls(class Dvb_s2_pls& dvb_s2_pls)
{
	_info._modcod = dvb_s2_pls._info._modcod;
	_info._fec_frame_size = dvb_s2_pls._info._fec_frame_size;
	_info._pilot_conf = dvb_s2_pls._info._pilot_conf;
}


/*
 * Destructor
 */
Dvb_s2_pls::~Dvb_s2_pls()
{
}


/*
 * Get modulation mode.
 */
enum Dvb_s2_pls::modulation_type
Dvb_s2_pls::modulation_type() {
	switch (_info._modcod) {
	case MODCOD_DUMMY_PLFRAME:
		return MOD_DUMMY_PLFRAME;

	case MODCOD_QPSK_1_4:
	case MODCOD_QPSK_1_3:
	case MODCOD_QPSK_2_5:
	case MODCOD_QPSK_1_2:
	case MODCOD_QPSK_3_5:
	case MODCOD_QPSK_2_3:
	case MODCOD_QPSK_3_4:
	case MODCOD_QPSK_4_5:
	case MODCOD_QPSK_5_6:
	case MODCOD_QPSK_8_9:
	case MODCOD_QPSK_9_10:
		return MOD_QPSK;

	case MODCOD_8PSK_3_5:
	case MODCOD_8PSK_2_3:
	case MODCOD_8PSK_3_4:
	case MODCOD_8PSK_5_6:
	case MODCOD_8PSK_8_9:
	case MODCOD_8PSK_9_10:
		return MOD_8PSK;

	case MODCOD_16APSK_2_3:
	case MODCOD_16APSK_3_4:
	case MODCOD_16APSK_4_5:
	case MODCOD_16APSK_5_6:
	case MODCOD_16APSK_8_9:
	case MODCOD_16APSK_9_10:
		return MOD_16APSK;

	case MODCOD_32APSK_3_4:
	case MODCOD_32APSK_4_5:
	case MODCOD_32APSK_5_6:
	case MODCOD_32APSK_8_9:
	case MODCOD_32APSK_9_10:
		return MOD_32APSK;

	default:
		assert(0);
	}
}


/*
 * Get coding type.
 */
enum Dvb_s2_pls::coding_type
Dvb_s2_pls::coding_type()
{
	switch (_info._fec_frame_size) {
	case FECFRAME_NORMAL:
		switch (_info._modcod) {
		case MODCOD_QPSK_1_4:
			return COD_NORMAL_1_4;

		case MODCOD_QPSK_1_3:
			return COD_NORMAL_1_3;

		case MODCOD_QPSK_2_5:
			return COD_NORMAL_2_5;

		case MODCOD_QPSK_1_2:
			return COD_NORMAL_1_2;

		case MODCOD_QPSK_3_5:
		case MODCOD_8PSK_3_5:
			return COD_NORMAL_3_5;

		case MODCOD_QPSK_2_3:
		case MODCOD_8PSK_2_3:
		case MODCOD_16APSK_2_3:
			return COD_NORMAL_2_3;

		case MODCOD_QPSK_3_4:
		case MODCOD_8PSK_3_4:
		case MODCOD_16APSK_3_4:
		case MODCOD_32APSK_3_4:
			return COD_NORMAL_3_4;

		case MODCOD_QPSK_4_5:
		case MODCOD_16APSK_4_5:
		case MODCOD_32APSK_4_5:
			return COD_NORMAL_4_5;

		case MODCOD_QPSK_5_6:
		case MODCOD_8PSK_5_6:
		case MODCOD_16APSK_5_6:
		case MODCOD_32APSK_5_6:
			return COD_NORMAL_5_6;

		case MODCOD_QPSK_8_9:
		case MODCOD_8PSK_8_9:
		case MODCOD_16APSK_8_9:
		case MODCOD_32APSK_8_9:
			return COD_NORMAL_8_9;

		case MODCOD_QPSK_9_10:
		case MODCOD_8PSK_9_10:
		case MODCOD_16APSK_9_10:
		case MODCOD_32APSK_9_10:
			return COD_NORMAL_9_10;

		case MODCOD_DUMMY_PLFRAME:
		default:
			assert(0);
		}

	case FECFRAME_SHORT:
		switch (_info._modcod) {
		case MODCOD_QPSK_1_4:
			return COD_SHORT_1_4;

		case MODCOD_QPSK_1_3:
			return COD_SHORT_1_3;

		case MODCOD_QPSK_2_5:
			return COD_SHORT_2_5;

		case MODCOD_QPSK_1_2:
			return COD_SHORT_1_2;

		case MODCOD_QPSK_3_5:
		case MODCOD_8PSK_3_5:
			return COD_SHORT_3_5;

		case MODCOD_QPSK_2_3:
		case MODCOD_8PSK_2_3:
		case MODCOD_16APSK_2_3:
			return COD_SHORT_2_3;

		case MODCOD_QPSK_3_4:
		case MODCOD_8PSK_3_4:
		case MODCOD_16APSK_3_4:
		case MODCOD_32APSK_3_4:
			return COD_SHORT_3_4;

		case MODCOD_QPSK_4_5:
		case MODCOD_16APSK_4_5:
		case MODCOD_32APSK_4_5:
			return COD_SHORT_4_5;

		case MODCOD_QPSK_5_6:
		case MODCOD_8PSK_5_6:
		case MODCOD_16APSK_5_6:
		case MODCOD_32APSK_5_6:
			return COD_SHORT_5_6;

		case MODCOD_QPSK_8_9:
		case MODCOD_8PSK_8_9:
		case MODCOD_16APSK_8_9:
		case MODCOD_32APSK_8_9:
			return COD_SHORT_8_9;

		case MODCOD_DUMMY_PLFRAME:
		case MODCOD_QPSK_9_10:
		case MODCOD_8PSK_9_10:
		case MODCOD_16APSK_9_10:
		case MODCOD_32APSK_9_10:
		default:
			assert(0);
		}

	default:
		assert(0);
	}
}


/*
 * Copy _info of this object to specific spapce.
 */
void
Dvb_s2_pls::copy_info_to(struct info& info)
{
	info._modcod = _info._modcod;
	info._fec_frame_size = _info._fec_frame_size;
	info._pilot_conf = _info._pilot_conf;
}


/*
 * Replace _info of this object from specific spapce.
 */
void
Dvb_s2_pls::copy_info_from(struct info& info)
{
	_info._modcod = info._modcod;
	_info._fec_frame_size = info._fec_frame_size;
	_info._pilot_conf = info._pilot_conf;
}
