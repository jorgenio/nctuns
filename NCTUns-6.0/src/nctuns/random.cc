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

/*
 * Copyright (c) 1997 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the Computer Systems
 *	Engineering Group at Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


/* new random number generator */

#include <stdlib.h>
#include <math.h>
#include <sys/time.h>   // for gettimeofday
#include <unistd.h>
#include <gbind.h>


#include <stdio.h>


#if defined(sun)
extern "C" {			
	int atoi(...);
	int gettimeofday(struct timeval*, struct timezone*);
	   }
#endif



/*
 * Generate a periodic sequence of pseudo-random numbers with
 * a period of 2^31 - 2.  The generator is the "minimal standard"
 * multiplicative linear congruential generator of Park, S.K. and
 * Miller, K.W., "Random Number Generators: Good Ones are Hard to Find,"
 * CACM 31:10, Oct. 88, pp. 1192-1201.
 *
 * The algorithm implemented is:  Sn = (a*s) mod m.
 *   The modulus m can be approximately factored as: m = a*q + r,
 *   where q = m div a and r = m mod a.
 *
 * Then Sn = g(s) + m*d(s)
 *   where g(s) = a(s mod q) - r(s div q)
 *   and d(s) = (s div q) - ((a*s) div m)
 *
 * Observations:
 *   - d(s) is either 0 or 1.
 *   - both terms of g(s) are in 0, 1, 2, . . ., m - 1.
 *   - |g(s)| <= m - 1.
 *   - if g(s) > 0, d(s) = 0, else d(s) = 1.
 *   - s mod q = s - k*q, where k = s div q.
 *
 * Thus Sn = a(s - k*q) - r*k,
 *   if (Sn <= 0), then Sn += m.
 *
 * To test an implementation for A = 16807, M = 2^31-1, you should
 *   get the following sequences for the given starting seeds:
 *
 *   s0, s1,    s2,        s3,          . . . , s10000,     . . . , s551246 
 *    1, 16807, 282475249, 1622650073,  . . . , 1043618065, . . . , 1003 
 *    1973272912, 1207871363, 531082850, 967423018
 *
 * It is important to check for s10000 and s551246 with s0=1, to guard 
 * against overflow.
*/
/*
 * The sparc assembly code [no longer here] is based on Carta, D.G., "Two Fast
 * Implementations of the 'Minimal Standard' Random Number
 * Generator," CACM 33:1, Jan. 90, pp. 87-88.
 *
 * ASSUME that "the product of two [signed 32-bit] integers (a, sn)
 *        will occupy two [32-bit] registers (p, q)."
 * Thus: a*s = (2^31)p + q
 *
 * From the observation that: x = y mod z is but
 *   x = z * the fraction part of (y/z),
 * Let: sn = m * Frac(as/m)
 *
 * For m = 2^31 - 1,
 *   sn = (2^31 - 1) * Frac[as/(2^31 -1)]
 *      = (2^31 - 1) * Frac[as(2^-31 + 2^-2(31) + 2^-3(31) + . . .)]
 *      = (2^31 - 1) * Frac{[(2^31)p + q] [2^-31 + 2^-2(31) + 2^-3(31) + . . 
.]}
 *      = (2^31 - 1) * Frac[p+(p+q)2^-31+(p+q)2^-2(31)+(p+q)3^(-31)+ . . .]
 *
 * if p+q < 2^31:
 *   sn = (2^31 - 1) * Frac[p + a fraction + a fraction + a fraction + . . .]
 *      = (2^31 - 1) * [(p+q)2^-31 + (p+q)2^-2(31) + (p+q)3^(-31) + . . .]
 *      = p + q
 *
 * otherwise:
 *   sn = (2^31 - 1) * Frac[p + 1.frac . . .]
 *      = (2^31 - 1) * (-1 + 1.frac . . .)
 *      = (2^31 - 1) * [-1 + (p+q)2^-31 + (p+q)2^-2(31) + (p+q)3^(-31) + . . .]
 *      = p + q - 2^31 + 1
 */
 

#define A       16807L          /* multiplier, 7**5 */
#define M       2147483647L     /* modulus, 2**31 - 1; both used in random */
#define INVERSE_M ((double)4.656612875e-10)  /* (1.0/(double)M) */

#define RAW_SEED_SOURCE			0x01
#define PREDEF_SEED_SOURCE		0x02
#define HEURISTIC_SEED_SOURCE		0x03


static long seed_ = 1;


long Random()
{
        long L, H;
	L = A * (seed_ & 0xffff);
	H = A * (seed_ >> 16);
	
	seed_ = ((H & 0x7fff) << 16) + L;
	seed_ -= 0x7fffffff;
	seed_ += H >> 15;
	
	if (seed_ <= 0) {
	        seed_ += 0x7fffffff;
	}
	return(seed_);
}

double fRandom()
{
	long i = Random();
	return i * INVERSE_M;
}


void set_seed( long seed ) {
	seed_ = seed;
}


void Randomize(unsigned char source, int seed )
{
struct timeval tv;

        /* The following predefined seeds are evenly spaced around
	 * the 2^31 cycle.  Each is approximately 33,000,000 elements
	 * apart.
	 */
#define N_SEEDS_ 64
        static long predef_seeds[N_SEEDS_] = {  
		1973272912L,  188312339L, 1072664641L,  694388766L,
		2009044369L,  934100682L, 1972392646L, 1936856304L,
		1598189534L, 1822174485L, 1871883252L,  558746720L,
		605846893L, 1384311643L, 2081634991L, 1644999263L,
		773370613L,  358485174L, 1996632795L, 1000004583L,
		1769370802L, 1895218768L,  186872697L, 1859168769L,
		349544396L, 1996610406L,  222735214L, 1334983095L,
		144443207L,  720236707L,  762772169L,  437720306L,
		939612284L,  425414105L, 1998078925L,  981631283L,
		1024155645L,  822780843L,  701857417L,  960703545L,
		2101442385L, 2125204119L, 2041095833L,   89865291L,
		898723423L, 1859531344L,  764283187L, 1349341884L,
		678622600L,  778794064L, 1319566104L, 1277478588L,
		538474442L,  683102175L,  999157082L,  985046914L,
		722594620L, 1695858027L, 1700738670L, 1995749838L,
		1147024708L,  346983590L,  565528207L,  513791680L
	};
	static long heuristic_sequence = 0;

	switch (source) {
	case RAW_SEED_SOURCE:
		// use it as it is
		break;
	case PREDEF_SEED_SOURCE:
		if (seed < 0 || seed >= N_SEEDS_)
			abort();
		seed = predef_seeds[seed];
		break;
	case HEURISTIC_SEED_SOURCE:
		gettimeofday(&tv, 0);
		heuristic_sequence++;   // Always make sure we're different than last time.
		seed = (tv.tv_sec ^ tv.tv_usec ^ (heuristic_sequence << 8)) & 0x7fffffff;
		break;
	};
	// set it
	if (seed < 0)
		seed = -seed;

// Added and changed by Prof. S.Y. Wang on 01/10/2005
// Now that the GUI program now allows the user to specifiy his/her desired
// random number seed, we should use the one (RanSeed, defined in qbind.h) 
// provided by the user.
        if (RanSeed == 0) {
           // The GUI user wants to use a random random number seed.
           struct timeval  Time;
           gettimeofday(&Time,NULL);
           set_seed(Time.tv_usec);
        } else {
          // In the following, we multiply RanSeed by an arbitray number
          // (in this case, 123456789) so that different RanSeeds can lead to
          // different sequences of random numbers. 
          set_seed((long) (123456789 ^ RanSeed) & 0x7fffffff);
        }

	// Toss away the first few values of heuristic seed.
	// In practice this makes sequential heuristic seeds
	// generate different first values.
	//
	if (source == HEURISTIC_SEED_SOURCE) {
		int i;
		for (i = 0; i < 128; i++) {
			Random();
		};
	};
}


