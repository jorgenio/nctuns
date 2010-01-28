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

#ifndef __NCTUNS_dvb_s2_pls_h__
#define __NCTUNS_dvb_s2_pls_h__


/*
 * Physical layer signalling, NCC must signal the DVB-S.2 system with
 * a set of these enumerations. It will just be used at forward link.
 */
class Dvb_s2_pls {
public:
	/*
	 * Modulation/Coding type.
	 */
	enum modcod_type {
		MODCOD_DUMMY_PLFRAME	= 0x00,	/* Dummy physical layer frame */
		MODCOD_QPSK_1_4		= 0x01,	/* QPSK 1/4 */
		MODCOD_QPSK_1_3		= 0x02,	/* QPSK 1/3 */
		MODCOD_QPSK_2_5		= 0x03,	/* QPSK 2/5 */
		MODCOD_QPSK_1_2		= 0x04,	/* QPSK 1/2 */
		MODCOD_QPSK_3_5		= 0x05,	/* QPSK 3/5 */
		MODCOD_QPSK_2_3		= 0x06,	/* QPSK 2/3 */
		MODCOD_QPSK_3_4		= 0x07,	/* QPSK 3/4 */
		MODCOD_QPSK_4_5		= 0x08,	/* QPSK 4/5 */
		MODCOD_QPSK_5_6		= 0x09,	/* QPSK 5/6 */
		MODCOD_QPSK_8_9		= 0x0a,	/* QPSK 8/9 */
		MODCOD_QPSK_9_10	= 0x0b,	/* QPSK 9/10 */
		MODCOD_8PSK_3_5		= 0x0c,	/* 8PSK 3/5 */
		MODCOD_8PSK_2_3		= 0x0d,	/* 8PSK 2/3 */
		MODCOD_8PSK_3_4		= 0x0e,	/* 8PSK 3/4 */
		MODCOD_8PSK_5_6		= 0x0f,	/* 8PSK 5/6 */
		MODCOD_8PSK_8_9		= 0x10,	/* 8PSK 8/9 */
		MODCOD_8PSK_9_10	= 0x11,	/* 8PSK 9/10 */
		MODCOD_16APSK_2_3	= 0x12,	/* 16APSK 2/3 */
		MODCOD_16APSK_3_4	= 0x13,	/* 16APSK 3/4 */
		MODCOD_16APSK_4_5	= 0x14,	/* 16APSK 4/5 */
		MODCOD_16APSK_5_6	= 0x15,	/* 16APSK 5/6 */
		MODCOD_16APSK_8_9	= 0x16,	/* 16APSK 8/9 */
		MODCOD_16APSK_9_10	= 0x17,	/* 16APSK 9/10 */
		MODCOD_32APSK_3_4	= 0x18,	/* 32APSK 3/4 */
		MODCOD_32APSK_4_5	= 0x19,	/* 32APSK 4/5 */
		MODCOD_32APSK_5_6	= 0x1a,	/* 32APSK 5/6 */
		MODCOD_32APSK_8_9	= 0x1b,	/* 32APSK 8/9 */
		MODCOD_32APSK_9_10	= 0x1c,	/* 32APSK 9/10 */
		MODCOD_RESERVED			/* reserved */
	};
	/*
	 * FEC frame size.
	 */
	enum fec_frame_size {
		FECFRAME_NORMAL	= 0x00,
		FECFRAME_SHORT	= 0x01
	};
	/*
	 * Pilot configuration.
	 */
	enum pilot_conf {
		PILOT_OFF	= 0x00,
		PILOT_ON	= 0x01,
	};
	/*
	 * Modulation types.
	 */
	enum modulation_type {
		MOD_DUMMY_PLFRAME,
		MOD_QPSK,
		MOD_8PSK,
		MOD_16APSK,
		MOD_32APSK,
	};
	/*
	 * Coding types.
	 */
	enum coding_type {
		COD_NORMAL_1_4 = 0,	/* Normal FEC frame 1/4 */
		COD_NORMAL_1_3,		/* Normal FEC frame 1/3 */
		COD_NORMAL_2_5,		/* Normal FEC frame 2/5 */
		COD_NORMAL_1_2,		/* Normal FEC frame 1/2 */
		COD_NORMAL_3_5,		/* Normal FEC frame 3/5 */
		COD_NORMAL_2_3,		/* Normal FEC frame 2/3 */
		COD_NORMAL_3_4,		/* Normal FEC frame 3/4 */
		COD_NORMAL_4_5,		/* Normal FEC frame 4/5 */
		COD_NORMAL_5_6,		/* Normal FEC frame 5/6 */
		COD_NORMAL_8_9,		/* Normal FEC frame 8/9 */
		COD_NORMAL_9_10,	/* Normal FEC frame 9/10 */
		COD_SHORT_1_4,		/* Short FEC frame 1/4 */
		COD_SHORT_1_3,		/* Short FEC frame 1/3 */
		COD_SHORT_2_5,		/* Short FEC frame 2/5 */
		COD_SHORT_1_2,		/* Short FEC frame 1/2 */
		COD_SHORT_3_5,		/* Short FEC frame 3/5 */
		COD_SHORT_2_3,		/* Short FEC frame 2/3 */
		COD_SHORT_3_4,		/* Short FEC frame 3/4 */
		COD_SHORT_4_5,		/* Short FEC frame 4/5 */
		COD_SHORT_5_6,		/* Short FEC frame 5/6 */
		COD_SHORT_8_9		/* Short FEC frame 8/9 */
	};

	/*
	 * Structures definitation.
	 */
	struct info {
		enum modcod_type	_modcod		:5; /* modulation/coding type */
		enum fec_frame_size	_fec_frame_size	:1; /* FEC frame size */
		enum pilot_conf		_pilot_conf	:1; /* Polot configuration */
	};

private:
	struct info	_info;

public:
	Dvb_s2_pls();
	Dvb_s2_pls(class Dvb_s2_pls&);
	~Dvb_s2_pls();
	/*
	 * Set/Get modulation and coding type.
	 */
	inline void set_modcod(enum modcod_type modcod) {
		_info._modcod = modcod;
	}
	inline enum modcod_type modcod() {
		return _info._modcod;
	}
	/*
	 * Set/Get FEC frame size.
	 */
	inline void set_fec_frame_size(enum fec_frame_size size) {
		_info._fec_frame_size = size;
	}
	inline enum fec_frame_size fec_frame_size() {
		return _info._fec_frame_size;
	}
	/*
	 * Set/Get pilot configuration.
	 */
	inline void set_pilot_conf(enum pilot_conf conf) {
		_info._pilot_conf = conf;
	}
	inline enum pilot_conf pilot_conf() {
		return _info._pilot_conf;
	}
	/*
	 * Get modulation type.
	 */
	enum modulation_type modulation_type();
	/*
	 * Get coding type.
	 */
	enum coding_type coding_type();
	/*
	 * Copy _info.
	 */
	void copy_info_to(struct info&);
	void copy_info_from(struct info&);
};

#endif	/* __NCTUNS_dvb_s2_pls_h__ */
