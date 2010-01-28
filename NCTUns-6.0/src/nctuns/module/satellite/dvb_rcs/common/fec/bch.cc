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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "bch.h"
#include "GaloisField.h"
#include "GaloisFieldElement.h"
#include "GaloisFieldPolynomial.h"


/*
 * Member functions definition of class Bch.
 */

/*
 * Constructor
 */
Bch::Bch()
: _initialized(false)
, _gen_poly(NULL)
, _galois_field(NULL)
, _recv_block(NULL)
{
}


/*
 * Destructor
 */
Bch::~Bch()
{
	if (_gen_poly)
		delete _gen_poly;
	if (_galois_field)
		delete _galois_field;
	if (_recv_block)
		delete _recv_block;
}


/*
 * Initialize the BCH encoding and decoding system.
 * - prim_poly: the primitive polynomial of the galois field.
 * - m: the degree of the primitive polynomial.
 * - gen_poly: the generator polynomial of the BCH system, the degree of
 *             the generaotr polynomial should be m * t.
 * - t: the error correction capability of the BCH system.
 */
void
Bch::init(const unsigned int* prim_poly, unsigned int m,
		const unsigned int* gen_poly, unsigned int t)
{
	_initialized = true;
	_m = m;
	_n = (unsigned int)pow(2, m) - 1;
	_t = t;
	_gen_poly_deg = m * t;
	_galois_field = new class galois::GaloisField(m, prim_poly);
	_gen_poly = new unsigned int[_gen_poly_deg];
	memcpy (_gen_poly, gen_poly, _gen_poly_deg * sizeof(unsigned int));
	_recv_block = new class galois::GaloisFieldElement[_n];
	/*
	 * _syndrome[0] is redundancy for the purpose of code readability.
	 */
	_syndrome = new class galois::GaloisFieldElement[2 * _t + 1];
}


/*
 * BCH encoder.
 * - block: the pointer to the block to be encoded.
 * - n: size of the block in bit.
 */
void
Bch::encoder(void* block, unsigned int n)
{
	/*
	 * Check if the system is ready for encoding block with length n.
	 */
	_sys_check(n);

	unsigned int	r = _gen_poly_deg;
	unsigned int	k = n - r;
	bool		shift_reg[r];
	int8_t*		remainder;

	memset (shift_reg, 0, sizeof (shift_reg));
	remainder = &((int8_t*)block)[k / 8];
	for (unsigned int i = 0 ; i < k ; i++) {
		/*
		 * Most significant bit of shift register XOR with
		 * i-th bit of the bit string.
		 */
		if (shift_reg[0] ^ _bit(block, i)) {
			/*
			 * Left-shift the shift register and XOR with
			 * the generator polynomial.
			 */
			for (unsigned int l = 0 ; l < r - 1 ; l++)
				shift_reg[l] = shift_reg[l + 1] ^ _gen_poly[l];
			shift_reg[r - 1] = _gen_poly[r - 1];
		} else {
			/*
			 * Left-shift the shift register.
			 */
			for (unsigned int l = 0 ; l < r - 1 ; l++)
				shift_reg[l] = shift_reg[l + 1];
			shift_reg[r - 1] = 0;
		}
	}
	/*
	 * Fill the remainder to generate the codeword.
	 */
	memset (remainder, 0, r / 8);
	for (unsigned int i = 0 ; i < r ; i++) {
		if (shift_reg[i])
			remainder[i / 8] ^= (0x80 >> (i % 8));
	}
}


/*
 * BCH decoder.
 * - block: the pointer to the block to be decoded. MSB of the block is
 *          the coefficient of the highest degree in polynomial form.
 * - n: size of the block in bit.
 * - return value: -1 if decoding failed while 0 if decoding succeeded.
 */
int
Bch::decoder(void* block, unsigned int n)
{
	/*
	 * Check if the system is ready for decoding block with length n.
	 */
	_sys_check(n);

	const class galois::GaloisFieldElement gf_zero(_galois_field, 0);
	const class galois::GaloisFieldElement gf_one(_galois_field, 1);
	const class galois::GaloisFieldElement
		gf_alpha(_galois_field, _galois_field->alpha(1));

	/*
	 * Transfer received block into the representation in the galois field.
	 * Since the representation in the galois field should be in ascending
	 * order, so we fill the block from the highest degree.
	 */
	for (unsigned int i = 0 ; i < n ; i++) {
		if (_bit(block, i))
			_recv_block[n - 1 - i] = gf_one;
		else
			_recv_block[n - 1 - i] = gf_zero;
	}
	/*
	 * Transfer to polynomial form.
	 */
	class galois::GaloisFieldPolynomial
		recv_poly(_galois_field, n - 1, _recv_block);
		//recv_poly(_galois_field, _n - 1, _recv_block);

	/*
	 * Apply the Berlekamp-Massey algorithm.
	 * Step.01: Compute the syndrome sequence _syndrome[1], _syndrome[2],
	 *          ..., _syndrome[_t] for the received word.
	 * Step.02: Initialize the algorithm variables as follows:
	 *          k = 0, Lambda(x) = 1, l = 0, and T(x) = x.
	 * Step.03: Set k = k + 1. Compute the discrepancy delta by
	 *          subtracting the k-th output of the LFSR defined by
	 *          Lambda(x) from the k-th syndrome.
	 *          	delta = _syndrome[k] -
	 *          		Summation {Lambda[i] * _syndrome[k - i]}
	 *          		i = 1 to l
	 * Step.04: If delta = 0, then go to step 8.
	 * Step.05: Modify the connection polynomial:
	 *          	Lambda(x) = Lambda(x) - delta * T(x)
	 * Step.06: If 2L >= k, then go to step 8.
	 * Step.07: Set l = k - l and T(x) = Lambda(x) / delta
	 * Step.08: Set T(x) = x * T(x)
	 * Step.09: If k < 2t, then go to step 3.
	 * Step.10: Determine the roots of Lambda(x). If the roots are distinct
	 *          and lie in the right field, then determine the error
	 *          magnitudes, correct the corresponding locations in the
	 *          received word, and STOP.
	 * Step.11: Declare a decoding failure and STOP.
	 *
	 * Reference:
	 * 	Stephen B. Wicker,
	 *	"Error Control Systems for Digital Communication and Storage", 
	 *	Prentice Hall, 1995.  
	 *	Section 9.2.2 "The Berlekemp-Massey Algorithm"
	 */

	/*
	 * Step.1
	 * _syndrome[i] = _recv_poly(alpha^i)
	 */
	bool	err = false;
	for (unsigned int i = 1 ; i <= 2 * _t ; i++) {
		if ((_syndrome[i] = recv_poly(gf_alpha ^ i)) != gf_zero)
			err = true;
	}
	/*
	 * If all syndrome are zero, decoding is done.
	 */
	if (!err) return 0;

	/*
	 * Step.2
	 * Lambda(x) = 1
	 * T(x) = x
	 */
	unsigned int			k = 0;
	unsigned int			l = 0;
	class galois::GaloisFieldPolynomial	poly_lambda(gf_one);
	class galois::GaloisFieldPolynomial	poly_t(gf_one);
	poly_t <<= 1;

	for (k += 1 ; k < 2 * _t ; k++ ) {
		galois::GaloisFieldElement	delta(_galois_field, 0);
		galois::GaloisFieldPolynomial	poly_lambda_prev;

		poly_lambda_prev = poly_lambda;

		/*
		 * Step.3
		 */
		for (unsigned int i = 1 ; i <= l ; i++) {
			if (poly_lambda_prev.deg () < i)
				break;
			delta += poly_lambda_prev[i] * _syndrome[k - i];
		}
		delta = _syndrome[k] - delta;

		/*
		 * Step.4
		 */
		if (delta != 0) {
			/*
			 * Step.5
			 */
			poly_lambda = poly_lambda_prev - delta * poly_t;
			/*
			 * Step.6
			 */
			if (2 * l < k) {
				/*
				 * Step.7
				 */
				l = k - l;
				poly_t = poly_lambda_prev / delta;
			}
		}
		/*
		 * Step.8
		 * poly_t(x) = x * poly_t(x)
		 */
		poly_t <<= 1;
	}

	/*
	 * If Lambda(x) is a constant or the degree of Lambda(x) is greater
	 * than t, then the decoding fails.
	 */
	if (poly_lambda.deg() < 1 || poly_lambda.deg() > _t) {
		printf ("Decoding failure, degree of Lambda(x) = %d\n",
			poly_lambda.deg());
		return -1;
	}
	/*
	 * Step.10
	 */
	class std::vector<galois::GaloisFieldElement>	roots;
	roots.clear();
	for (int i = 0 ; i < pow(2, _m) ; i++) {
		if (poly_lambda(i) == 0)
			roots.push_back(
			galois::GaloisFieldElement(_galois_field, i));
	}
	/*
	 * Notation: ri = roots[i], j = roots.size() - 1, ai = ri ^ -1
	 * Lambda(x) = (r0 + x)(r1 + x)...(rj + x)
	 *           = (1 + a0 * x)(1 + a1 * x)...(1 + aj * x)
	 * ==> log(a0), log(a1), ..., log(aj) are error positions.
	 */
	unsigned int err_bit;
	for (unsigned int i = 0 ; i < roots.size() ; i++) {
		/*
		 * If the error bit found is larger than the length of the block
		 * , then the decoding fails, otherwise correct the error bit.
		 */
		if ((err_bit = n - 1 - (roots[i] ^ -1).index()) >= n)
			return -1;
		((int8_t*)block)[err_bit / 8] ^= (0x80 >> (err_bit % 8));
	}
	return 0;
}


/*
 * Check if the BCH system is initialized and if the system is capable of
 * the length of the received block.
 * - n: size of the block in bit.
 */
void
Bch::_sys_check(unsigned int n)
{
	if (!_initialized) {
		fprintf (stderr,
		"BCH: The BCH system has not been initialized.\n");
		exit(EXIT_FAILURE);
	}
	if (n > _n) {
		fprintf (stderr, "BCH: The length of received block"
				" can't exceed the size of the field.\n");
		exit(EXIT_FAILURE);
	}
}
