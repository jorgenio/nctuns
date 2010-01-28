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

#ifndef __NCTUNS_dvb_s2_h__
#define __NCTUNS_dvb_s2_h__


#include <assert.h>
#include <list>
#include <stdint.h>
#include <object.h>

#include "../common/dvb_s2_pls.h"

/*
 * Max data field length.
 */
#define DVB_MAX_DFL 58112

/*
 * CRC-8 with generator polymonial:
 * 	g(x) = x^8 + x^7 + x^6 + x^4 + x^2 + 1
 * Normal original representation:
 * 	0b11010101 = 0xd5
 */
#define DVB_CRC8_GEN_POLY 0xd5

/*
 * FEC coding system parameters.
 */
/*
 * FEC frame size.
 */
#define DVB_FECFRAME_SIZE_NORMAL	64800
#define DVB_FECFRAME_SIZE_SHORT		16200
/*
 * Number of FEC coding schemes.
 */
#define DVB_BCH_MODE	4
#define DVB_LDPC_MODE	21
/*
 * Number of columns used in the FEC interleaver.
 */
#define DVB_8PSK_INTERLEAVER_COL	3
#define DVB_16PSK_INTERLEAVER_COL	4
#define DVB_32PSK_INTERLEAVER_COL	5

/*
 * Physical layer system parameters.
 */
/*
 * Physucal layer header is modulated into 90 `(Pi/2)-BPSK' symbols.
 */
#define DVB_PL_PLHEADER_SYMBOL		90
/*
 * Each slot in the physical frame is composed of 90 symbols.
 */
#define DVB_PL_SYMBOL_PER_SLOT		90
/*
 * Pilot block is inserted into the physical frame every 16 slots.
 */
#define DVB_PL_SLOT_PER_PILOT_INSERT	16
/*
 * Each pilot block in the physical frame is composed of 36 pilot symbols.
 */
#define DVB_PL_SYMBOL_PER_PILOT_BLOCK	36
/*
 * Dummy physical layer frame is composed of
 * physical layer header and 36 slots of un-modulated carriers.
 */
#define DVB_PL_DUMMY_FRAME_SLOT		36
#define DVB_PL_NORMFRM_QPSK_SLOT	360
#define DVB_PL_NORMFRM_8PSK_SLOT	240
#define DVB_PL_NORMFRM_16APSK_SLOT	180
#define DVB_PL_NORMFRM_32APSK_SLOT	144
#define DVB_PL_SHRTFRM_QPSK_SLOT	90
#define DVB_PL_SHRTFRM_8PSK_SLOT	60
#define DVB_PL_SHRTFRM_16APSK_SLOT	45
#define DVB_PL_SHRTFRM_32APSK_SLOT	36

/*
 * Length of MPEG2 transport stream packet.
 */
#define MPEG2_TS_LEN 188

struct dvbrcs_node_id;

class Dvb_s2 : public NslObject {

protected:
	/*
	 * Enumberations.
	 */
	/*
	 * Coding type parameters entries.
	 */
	enum _coding_param_entry {
		BCH_UNCODED = 0x00,	/* BCH uncoded block length. */
		BCH_CODED,		/* BCH coded block length. */
		LDPC_UNCODED =		/* LDPC uncoded block length. */
		BCH_CODED,
		BCH_T_ERR,		/* BCH t-error bit correction. */
		LDPC_Q_VALUE,		/* LDPC q-value. */
		LDPC_VNODE_DEG,		/* LDPC max v-node degree. */
		LDPC_CNODE_DEG,		/* LDPC max c-node degree. */
		FEC_PARAM_COLS,		/* LDPC max c-node degree. */
	};

	/*
	 * DVB BCH modes
	 */
	enum _dvb_bch_mode {
		/* BCH with GF(2^16) */
		DVB_BCH_GF16_T12 = 0x00,	/* 12-error correction mode. */
		DVB_BCH_GF16_T10,		/* 10-error correction mode. */
		DVB_BCH_GF16_T8,		/* 8-error correction mode. */
		/* BCH with GF(2^14) */
		DVB_BCH_GF14_T12,		/* 12-error correction mode. */
	};

	/*
	 * MATYPE.
	 */
	/*
	 * TS/GS field.
	 */
	enum _if_type {
		IF_TYPE_TS		= 0x00,	/* Transport stream */
		IF_TYPE_GS		= 0x01,	/* Generic stream */
		IF_TYPE_ACM		= 0x02,	/* Adaptive coding and modulation */
		IF_TYPE_RESERVED	= 0x03	/* Reserved */
	};
	/*
	 * SIS/MIS field.
	 */
	enum _is_type {
		IS_TYPE_MULTIPLE	= 0x00,	/* Multiple input streams */
		IS_TYPE_SINGLE		= 0x01,	/* Sigle input stream */
	};
	/*
	 * CCM/ACM field.
	 */
	enum _cm_type {
		CM_TYPE_ACM	= 0x00,	/* Adaptative Coding and Modulation */
		CM_TYPE_CCM	= 0x01,	/* Constant Coding and Modulation */
	};
	/*
	 * Input stream synchronization indicator field.
	 */
	enum _issyi_mode {
		ISSYI_NOT_ACT	= 0x00,	/* ISSYI is not active */
		ISSYI_ACTIVE	= 0x01,	/* ISSYI is active */
	};
	/*
	 * Null packet deletion.
	 */
	enum _npd_mode {
		NPD_NOT_ACT	= 0x00,	/* NPD is not active */
		NPD_ACTIVE	= 0x01,	/* NPD is active */
	};
	/*
	 * Roll-off factor.
	 */
	enum _ro_factor {
		RO_35		= 0x00,	/* Roll-off factor = 0.35 */
		RO_25		= 0x01,	/* Roll-off factor = 0.25 */
		RO_20		= 0x02,	/* Roll-off factor = 0.20 */
		RO_RESERVED	= 0x03,	/* Reserved */
	};

	/*
	 * Copy of user packet sync-byte.
	 */
	enum _sync_type {
		SYNC_GEN_PACKED	= 0x00,	/* Generic packetized stream */
		SYNC_MPEG2_TS	= 0x47,	/* MPEG2 transport stream packets */
		SYNC_GEN_CONT,		/* Generic continuous stream */
	};


	/*
	 * Classes declarations.
	 */
	class Stream_buffer;
	class Base_band_header;
	class Ldpc;

	/*
	 * Private variables.
	 */
	enum Dvb_s2_pls::coding_type		_coding_type;
	class Dvb_s2_pls*			_dvb_s2_pls;
	class Base_band_header*			_base_band_header;
	uint8_t					_number_of_input_stream;
	double					_symbol_rate;

	/*
	 * FEC encoders and decoders.
	 */
	static const unsigned int		_fec_parameter_table[][FEC_PARAM_COLS];
	static const unsigned int		_bch_prim_poly_gf16[16 + 1];
	static const unsigned int		_bch_prim_poly_gf14[14 + 1];
	static const unsigned int		_bch_gen_poly_gf16_t12[16 * 12];
	static const unsigned int		_bch_gen_poly_gf16_t10[16 * 10];
	static const unsigned int		_bch_gen_poly_gf16_t8[16 * 8];
	static const unsigned int		_bch_gen_poly_gf14_t12[14 * 12];
	static class Bch*			_bch[DVB_BCH_MODE];
	static class Ldpc*			_ldpc[DVB_LDPC_MODE];
	static enum _dvb_bch_mode		_bch_mode_table[];

	/*
	 * for log
	 */
	u_char			_ptrlog;

	uint64_t _tx_delay(enum Dvb_s2_pls::modulation_type mod_type, unsigned int);
	inline uint64_t _rx_delay(
		enum Dvb_s2_pls::modulation_type mod_type, unsigned int len)
	{	return _tx_delay(mod_type, len);	}

	uint8_t _crc_8(void*, unsigned int, uint8_t);
	void _bb_scrambler(void*, unsigned int);

	void _bit_interleaver(void*, unsigned int, unsigned int);
	void _bit_deinterleaver(void*, unsigned int, unsigned int);
	int  _parse_nodeid_cfg(char *path, std::list<dvbrcs_node_id> &node_id);

	/*
	 * for turn on/off link fail flag
	 */
	uint32_t	LinkFailFlag;
	char            *_linkfail;
	char            *linkfailFileName;
	FILE            *linkfailFile;
	int		*tunfd_;

public:
	/*
	 * Constructor and Destructor.
	 */
 	Dvb_s2(uint32_t type, uint32_t id, struct plist* pl, const char *name);
 	~Dvb_s2();   
	inline double get_symbol_rate() {
		return _symbol_rate;
	}
}; 


/*
 * Classes definitions.
 */
/*
 * Input stream buffer, a circular buffer.
 */
class Dvb_s2::Stream_buffer {
	uint8_t*	_data;
	uint32_t	_front;
	uint32_t	_rear;
	uint32_t	_size;
	uint32_t	_free;
public:
	Stream_buffer(uint32_t);
	~Stream_buffer();
	unsigned int get(void*, unsigned int);
	int put(void*, unsigned int);
	inline uint32_t content_len() {
		return _size - _free;
	}
	inline uint32_t free() {
		return _free;
	}
};


/*
 * Base-band header.
 */
class Dvb_s2::Base_band_header {
private:
	/*
	 * Mode adaptation. (2 bytes)
	 */
	uint16_t	_ts_gs_field			:2;
	uint16_t	_sis_mis_field			:1;
	uint16_t	_ccm_acm_field			:1;
	uint16_t	_input_stream_sync_indicator	:1;
	uint16_t	_null_packet_deletion		:1;
	uint16_t	_roll_off			:2;
	uint16_t	_input_stream_id		:8;
	/*
	 * Remaining fields. (8 bytes)
	 */
	uint16_t	_user_packet_length;	/* in bit */
	uint16_t	_data_field_length;	/* in bit */
	uint8_t		_sync;
	uint16_t	_syncd;			/* in bit */
	uint8_t		_crc_8;

public:
	Base_band_header();
	/*
	 * Mode adaptation.
	 */
	inline void set_ts_gs_field(enum _if_type ts_gs_field) {
		_ts_gs_field = ts_gs_field;
	}
	inline void set_sis_mis_field(enum _is_type sis_mis_field) {
		_sis_mis_field = sis_mis_field;
	}
	inline void set_ccm_acm_field(enum _cm_type ccm_acm_field) {
		_ccm_acm_field = ccm_acm_field;
	}
	inline void set_input_stream_sync_indicator
		(enum _issyi_mode input_stream_sync_indicator) {
		_input_stream_sync_indicator =
			input_stream_sync_indicator;
	}
	inline void set_null_packet_deletion
		(enum _npd_mode null_packet_deletion) {
		_null_packet_deletion = null_packet_deletion;
	}
	inline void set_roll_off(enum _ro_factor roll_off) {
		_roll_off = roll_off;
	}
	inline void set_input_stream_id(uint8_t input_stream_id) {
		_input_stream_id = input_stream_id;
	}
	inline uint8_t input_stream_id() {
		return _input_stream_id;
	}
	/*
	 * Remaining fields.
	 */
	inline void set_user_packet_length(uint16_t length) {
		_user_packet_length = length;
	}
	inline void set_data_field_length(uint16_t length) {
		if (length > DVB_MAX_DFL)
			assert(0);
		_data_field_length = length;
	}
	inline uint16_t data_field_length() {
		return _data_field_length;
	}
	inline void set_sync(enum _sync_type sync) {
		_sync = sync;
	}
	inline void set_syncd(uint16_t syncd) {
		_syncd = syncd;
	}
	inline uint16_t syncd() {
		return _syncd;
	}
	inline void set_crc_8(uint8_t crc_8) {
		_crc_8 = crc_8;
	}
	inline uint8_t crc_8() {
		return _crc_8;
	}
	/*
	 * Fill the specific space with the base-band header.
	 * hdr: pointer to the space to be filled base-band header.
	 */
	inline void fill_base_band_header(void* frame_header) {
		memcpy(frame_header, this, sizeof(*this));
	}
	/*
	 * Apply the specific base-band header to this object.
	 * hdr: pointer to the specific base-band header.
	 */
	inline void apply_base_band_header(void* frame_header) {
		memcpy(this, frame_header, sizeof(*this));
	}
	/*
	 * Get the length of the base-band header.
	 */
	inline unsigned int header_size() {
		return sizeof(*this);
	}
	/*
	 * Get the data field of the given base-band frame.
	 */
	inline void* data_field(void* base_band_frame) {
		return &((int8_t*)base_band_frame)[header_size()];
	}
} __attribute__((packed));

  
/*
 * The LDPC encoder and decoder.
 */
#define LDPC_BIT_PER_GROUP	360
#define LDPC_SOFT_DECISION_ONE	(0.9)
class Dvb_s2::Ldpc {

private:
	static const void*	_acc_addr_table[];
	static const int	_acc_addr_table_normal_1_4[][13];
	static const int	_acc_addr_table_normal_1_3[][13];
	static const int	_acc_addr_table_normal_2_5[][13];
	static const int	_acc_addr_table_normal_1_2[][9];
	static const int	_acc_addr_table_normal_3_5[][13];
	static const int	_acc_addr_table_normal_2_3[][14];
	static const int	_acc_addr_table_normal_3_4[][13];
	static const int	_acc_addr_table_normal_4_5[][12];
	static const int	_acc_addr_table_normal_5_6[][14];
	static const int	_acc_addr_table_normal_8_9[][5];
	static const int	_acc_addr_table_normal_9_10[][5];
	static const int	_acc_addr_table_short_1_4[][13];
	static const int	_acc_addr_table_short_1_3[][13];
	static const int	_acc_addr_table_short_2_5[][13];
	static const int	_acc_addr_table_short_1_2[][9];
	static const int	_acc_addr_table_short_3_5[][13];
	static const int	_acc_addr_table_short_2_3[][14];
	static const int	_acc_addr_table_short_3_4[][13];
	static const int	_acc_addr_table_short_4_5[][4];
	static const int	_acc_addr_table_short_5_6[][14];
	static const int	_acc_addr_table_short_8_9[][5];
	bool			_initialized;
	unsigned int		_k;
	unsigned int		_n;
	unsigned int		_p;
	unsigned int		_q;
	unsigned int		_vnode_deg;
	unsigned int		_cnode_deg;
	unsigned int		_acc_per_group;
	const int*		_acc_addr;
	int32_t*		_cnode_from;
	int32_t*		_vnode_from;

	inline bool _bit(void* data, unsigned int i)
	{	return ((char*)data)[i / 8] & (0x80 >> (i % 8));	}
	inline bool _xor_bit(void* data, unsigned int i)
	{	return ((char*)data)[i / 8] ^= (0x80 >> (i % 8));	}
	inline void _init_decoder();
	void _decoding_iteration(float[]);
	inline void _decoding_phase1(float[], float[]);
	inline void _decoding_phase2(float[], float[]);
	inline unsigned int _parity_check(float[]);
	inline void _sys_check(unsigned);

public:
	Ldpc();
	~Ldpc();
	void init(enum Dvb_s2_pls::coding_type);
	void encoder(void*, unsigned int);
	int decoder(void*, unsigned int, unsigned int);
	unsigned int parity_check(void*, unsigned int);
}; 


#endif	/* __NCTUNS_dvb_s2_h__ */
