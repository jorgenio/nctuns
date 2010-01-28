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

#ifndef __NCTUNS_bch_h__
#define __NCTUNS_bch_h__


namespace galois
{
	class GaloisField;
	class GaloisFieldElement;
	class GaloisFieldPolynomial;
}

class Bch {

private:
	bool					_initialized;
	unsigned int				_m;
	unsigned int				_n;
	unsigned int				_t;
	unsigned int				_gen_poly_deg;
	unsigned int*				_gen_poly;
	class galois::GaloisField*		_galois_field;
	class galois::GaloisFieldElement*	_recv_block;
	class galois::GaloisFieldElement*	_syndrome;

	inline bool _bit(void* data, unsigned int i)
	{	return ((char*)data)[i / 8] & (0x80 >> (i % 8));	}
	void _sys_check(unsigned);

public:
	Bch();
	~Bch();
	void init(const unsigned int*, unsigned int,
			const unsigned int*, unsigned int);
	void encoder(void*, unsigned int);
	int decoder(void*, unsigned int);
}; 
  

#endif	/* __NCTUNS_bch_h__ */
