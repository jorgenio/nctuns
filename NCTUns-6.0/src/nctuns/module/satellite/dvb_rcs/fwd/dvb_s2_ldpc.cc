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
#include "dvb_s2.h"


/*
 * Member functions definition of class Ldpc.
 */

/*
 * Constructor
 */
Dvb_s2::Ldpc::Ldpc()
: _initialized(false)
, _cnode_from(NULL)
, _vnode_from(NULL)
{
}


/*
 * Destructor
 */
Dvb_s2::Ldpc::~Ldpc()
{
	if (_cnode_from)
		delete _cnode_from;
	if (_vnode_from)
		delete _vnode_from;
}


/*
 * Initialize the LDPC encoding and decoding system.
 * - type: LDPC coding type.
 */
void
Dvb_s2::Ldpc::init(enum Dvb_s2_pls::coding_type type)
{
	_initialized = true;
	_k = _fec_parameter_table[type][LDPC_UNCODED];
	if (type <= Dvb_s2_pls::COD_NORMAL_9_10)
		_n = DVB_FECFRAME_SIZE_NORMAL;
	else
		_n = DVB_FECFRAME_SIZE_SHORT;
	_p = _n - _k;
	_q = _fec_parameter_table[type][LDPC_Q_VALUE];
	_vnode_deg = _fec_parameter_table[type][LDPC_VNODE_DEG];
	_cnode_deg = _fec_parameter_table[type][LDPC_CNODE_DEG];
	_acc_per_group = _fec_parameter_table[type][LDPC_VNODE_DEG];
	/*
	 * Set the address table of parity bit accumulators.
	 */
	_acc_addr = (const int*)_acc_addr_table[type];
	if (_cnode_from)
		delete _cnode_from;
	/*
	 * _n v-nodes, each v-node at most connects with _vnode_deg c-nodes.
	 */
	_cnode_from = new int32_t[_n * _vnode_deg];
	if (_vnode_from)
		delete _vnode_from;
	/*
	 * _p c-nodes, each c-node at most connects with _cnode_deg v-nodes.
	 */
	_vnode_from = new int32_t[_p * _cnode_deg];

	_init_decoder();
}

void
Dvb_s2::Ldpc::_init_decoder()
{
	memset (_cnode_from, -1, _n * _vnode_deg * sizeof(int32_t));
	memset (_vnode_from, -1, _p * _cnode_deg * sizeof(int32_t));
	/*
	 * Add the mapping information of information bits in v-nodes.
	 */
	for (unsigned int g = 0 ; g < _k / LDPC_BIT_PER_GROUP ; g++) {
		for (unsigned int m = 0 ; m < LDPC_BIT_PER_GROUP ; m++) {
			unsigned int	vnode_no;
			unsigned int 	i;
			const int*	x;
			vnode_no = LDPC_BIT_PER_GROUP * g + m;
			i = vnode_no * _vnode_deg;
			x = &_acc_addr[(_acc_per_group + 1) * g];
			for (  ; *x >= 0 ; x++, i++) {
				assert(i < (vnode_no + 1) * _vnode_deg);
				int32_t		cnode_no;
				unsigned int	j;
				/*
				 * Add the mapping information:
				 *	cnode_no is connected with vnode_no
				 */
				cnode_no = (*x + m * _q) % _p;
				_cnode_from[i] = cnode_no;
				/*
				 * Add the mapping information:
				 *	vnode_no is connected with cnode_no
				 */
				j = cnode_no * _cnode_deg;
				for ( ; _vnode_from[j] >= 0 ; j++);
				assert(j < (cnode_no + 1) * _cnode_deg);
				_vnode_from[j] = vnode_no;
			}
		}
	}
	/*
	 * Add the mapping information of parity bits in v-nodes.
	 */
	unsigned int	vnode_no;
	unsigned int	cnode_no;
	unsigned int	i;
	unsigned int	j;
	for (vnode_no = _k ; vnode_no < _n ; vnode_no++) {
		/*
		 * Add the mapping information:
		 *	cnode_no is connected with vnode_no
		 */
		cnode_no = vnode_no - _k;
		i = vnode_no * _vnode_deg;
		for ( ; _cnode_from[i] >= 0 ; i++);
		assert(i < (vnode_no + 1) * _vnode_deg);
		_cnode_from[i++] = cnode_no;
		if (cnode_no != _p - 1)
			_cnode_from[i] = cnode_no + 1;
		/*
		 * Add the mapping information:
		 *	vnode_no is connected with cnode_no
		 */
		j = cnode_no * _cnode_deg;
		for ( ; _vnode_from[j] >= 0 ; j++);
		assert(j < (cnode_no + 1) * _cnode_deg);
		if (vnode_no != _k)
			_vnode_from[j++] = vnode_no - 1;
		_vnode_from[j] = vnode_no;
	}
}



/*
 * LDPC encoder.
 * - block: the pointer to the block to be encoded.
 * - n: size of the block in bit.
 */
void
Dvb_s2::Ldpc::encoder(void* block, unsigned int n)
{
	_sys_check(n);

	int8_t*		parity;

	parity = &((int8_t*)block)[_k / 8];

	/*
	 * Initialize parity bits.
	 */
	memset(parity, 0, _p / 8);
	/*
	 * For each information bit which is `1', generate parity bits.
	 */
	for (unsigned int vnode_no = 0 ; vnode_no < _k ; vnode_no++) {
		if (_bit(block, vnode_no)) {
			/*
			 * Accumulate all cnodes (parity bits) corresponding
			 * to the information bit which is `1'.
			 */
			for (unsigned int i = vnode_no * _vnode_deg
			; i < (vnode_no + 1) * _vnode_deg ; i++) {
				if (_cnode_from[i] < 0)
					break;
				_xor_bit(parity, _cnode_from[i]);
			}
		}
	}
	/*
	 * p[i] = p[i] XOR p[i-1], i = 1, 2, ..., n - k - 1
	 */
	bool previous_bit;
	/* 
	 * previous_bit is initialized as p[0].
	 */
	previous_bit = _bit(parity, 0);
	for (unsigned int i = 1 ; i < _p ; i++) {
		if (previous_bit)
			_xor_bit(parity, i);
		previous_bit = _bit(parity, i);
	}
}



/*
 * LDPC decoder.
 * - block: the pointer to the block to be decoded.
 * - n: size of the block in bit.
 * - iteration_threshold: The maximum iterations of the decoding procedure.
 * - return value: -1 if errors are not completely corrected while
 *                 0 if all errors are compleletly corrected.
 */
int
Dvb_s2::Ldpc::decoder(void* block, unsigned int n,
		unsigned int iteration_threshold)
{
	_sys_check(n);

	/*
	 * Skip decoding if there is no error.
	 */
	unsigned int	parity_check_err;
	if (!(parity_check_err = parity_check(block, n)))
		return 0;
	/*
	 * Transfer channel message to pre-defined probability form
	 * for soft-decision decoding algorithm.
	 */
	float	channel_msg[_n];
	for (unsigned int i = 0 ; i < _n ; i++) {
		if (_bit(block, i))
			channel_msg[i] = LDPC_SOFT_DECISION_ONE;
		else
			channel_msg[i] = 1 - LDPC_SOFT_DECISION_ONE;
	}

	unsigned int	prev_parity_check_err;
	unsigned int	fail_count;
	unsigned int	i;
	fail_count = 0;
	for (i = 0 ; i < iteration_threshold ; i++) {
		_decoding_iteration(channel_msg);
		prev_parity_check_err = parity_check_err;
		/*
		 * We abort following decoding iterations if the
		 * last iteration does not correct any error.
		 */
		if ((parity_check_err = _parity_check(channel_msg)) == 0)
			break;
	}
	if (i == iteration_threshold)
		parity_check_err = _parity_check(channel_msg);

	memset(block, 0, n / 8);
	for (unsigned int i = 0 ; i < _n ; i++) {
		if (channel_msg[i] >= 0.5)
			_xor_bit(block, i);
	}
	if (parity_check_err)
		return -1;
	return 0;
}


/*
 * LDPC decoding iteration.
 * - channel_msg: probability of that the bit received from channel is `1'.
 */
void
Dvb_s2::Ldpc::_decoding_iteration(float channel_msg[])
{
	float	cnode_msg[_n * _vnode_deg];
	_decoding_phase1(channel_msg, cnode_msg);
	_decoding_phase2(channel_msg, cnode_msg);
}


/*
 * LDPC decoding iteration phase 1 -- Update Rji
 * - channel_msg: probability of that the bit received from channel is `1'.
 * - cnode_msg: probability, that the bit of specific v-node is `0',
 *              passed by the corresponding c-nodes.
 */
void
Dvb_s2::Ldpc::_decoding_phase1(float channel_msg[], float cnode_msg[])
{
	for (unsigned int cnode_no = 0 ; cnode_no < _p ; cnode_no++) {
		/*
		 * Probability of that there are even 1's.
		 */
		float	product = 1;
		int	vnode_no;
		for (unsigned int c = cnode_no * _cnode_deg
		; c < (cnode_no + 1) * _cnode_deg ; c++) {
			if ((vnode_no = _vnode_from[c]) < 0)
				break;
			product *= 1 - 2 * channel_msg[vnode_no];
		}
		for (unsigned int c = cnode_no * _cnode_deg
		; c < (cnode_no + 1) * _cnode_deg ; c++) {
			if ((vnode_no = _vnode_from[c]) < 0)
				break;
			unsigned int v = vnode_no * _vnode_deg;
			for ( ; _cnode_from[v] != (int)cnode_no ; v++);

			cnode_msg[v] = product /
				(1 - 2 * channel_msg[vnode_no]);
			/*
			 * cnode_msg[i] <=> Rji(0)
			 */
			cnode_msg[v] = 0.5 + 0.5 * cnode_msg[v];
		}
	}
}


/*
 * LDPC decoding iteration phase 2 -- Update Qij
 * - channel_msg: probability of that the bit received from channel is `1'.
 * - cnode_msg: probability, that the bit of specific v-node is `0',
 *              passed by the corresponding c-nodes.
 */
void
Dvb_s2::Ldpc::_decoding_phase2(float channel_msg[], float cnode_msg[])
{
	/*
	 * channel_msg[j] <=> Qij
	 */
	for (unsigned int vnode_no = 0 ; vnode_no < _n ; vnode_no++) {
		/*
		 * q0 <=> Qij(0)
		 * q1 <=> Qij(1)
		 */
		float	q0 = 1 - channel_msg[vnode_no];
		float	q1 = channel_msg[vnode_no];
		for (unsigned int v = vnode_no * _vnode_deg
		; v < (vnode_no + 1) * _vnode_deg ; v++) {
			if (_cnode_from[v] < 0)
				break;
			q0 *= cnode_msg[v];
			q1 *= 1 - cnode_msg[v];
		}
		channel_msg[vnode_no] = q1 / (q0 + q1);
		if (channel_msg[vnode_no] == 0)
			channel_msg[vnode_no] = 1 - LDPC_SOFT_DECISION_ONE;
		else if (channel_msg[vnode_no] == 1)
			channel_msg[vnode_no] = LDPC_SOFT_DECISION_ONE;
	}
}


/*
 * LDPC parity check
 * - channel_msg: probability of that the bit received from channel is `1'.
 * - return value: number of parity check errors.
 */
unsigned int
Dvb_s2::Ldpc::_parity_check(float channel_msg[])
{
	unsigned int	parity_check[_p];
	unsigned int	parity_check_err;

	memset(parity_check, 0, _p * sizeof(unsigned int));
	for (unsigned int vnode_no = 0 ; vnode_no < _n ; vnode_no++) {
		if (channel_msg[vnode_no] >= 0.5) {
			for (unsigned int v = vnode_no * _vnode_deg
			; v < (vnode_no + 1) * _vnode_deg ; v++) {
				if (_cnode_from[v] < 0)
					break;
				parity_check[_cnode_from[v]]++;
			}
		}
	}
	parity_check_err = 0;
	for(unsigned  int i = 0 ; i < _p ; i++)
		parity_check_err += (parity_check[i] % 2);
	return parity_check_err;
}


/*
 * LDPC parity check
 * - block: the pointer to the block to be applied parity check.
 * - n: size of the block in bit.
 * - return value: number of parity check errors.
 */
unsigned int
Dvb_s2::Ldpc::parity_check(void* block, unsigned int n)
{
	_sys_check(n);
	unsigned int	parity_check[_p];
	unsigned int	parity_check_err;

	memset(parity_check, 0, _p * sizeof(unsigned int));
	for (unsigned int vnode_no = 0 ; vnode_no < _n ; vnode_no++) {
		if (_bit(block, vnode_no)) {
			for (unsigned int v = vnode_no * _vnode_deg
			; v < (vnode_no + 1) * _vnode_deg ; v++) {
				if (_cnode_from[v] < 0)
					break;
				parity_check[_cnode_from[v]]++;
			}
		}
	}
	parity_check_err = 0;
	for(unsigned  int i = 0 ; i < _p ; i++)
		parity_check_err += (parity_check[i] % 2);
	return parity_check_err;
}


/*
 * Check if the LDPC system is initialized and if the system is capable of
 * the length of the received block.
 * - n: size of the block in bit.
 */
void
Dvb_s2::Ldpc::_sys_check(unsigned int n)
{
	if (!_initialized) {
		fprintf (stderr,
		"LDPC: The LDPC system has not been initialized.\n");
		exit(EXIT_FAILURE);
	}
	if (n != _n) {
		fprintf (stderr, "LDPC: The length of received block"
				"is not identical to the LDPC system.\n");
		exit(EXIT_FAILURE);
	}
}
