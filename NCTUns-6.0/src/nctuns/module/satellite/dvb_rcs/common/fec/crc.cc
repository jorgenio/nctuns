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

#include <sys/types.h>
#include "crc.h"

/*
 * Sub-procedure of mode adaptation. Perform a CRC encoding. This function
 * can support CRC-8, CRC-16, CRC-32.
 * - user_packet: pointer to the user packet.
 * - len: length of the user packet.
 * - crc_len: length of CRC encoding field. (should be one of 8, 16, 32)
 * - gen_poly: generator polynomial.
 * - init_val: initial value of shift_reg. (should be 1 or 0)
 * - return value: computed CRC of this user packet.
 */
u_int32_t Crc::crc(void *user_packet, u_int32_t len, u_int32_t crc_len,
	       u_int32_t gen_poly, int init_value)
{
	u_int32_t shift_reg;
	u_int8_t *bit_string;
	u_int32_t i;
	int msb_mask, j;

	if (len <= 0 ) return 0;

	/*
	 * if init_value equal one, then fill '1' in all bits
	 * else fill zero in all bits of initial shift_reg
	 */
	if (init_value == 1)
		shift_reg = 0xFFFFFFFF;
	else
		shift_reg = 0x00;

	/*
	 * compute the shift_reg MSB mask
	 */
	msb_mask = 1 << (crc_len - 1);

	bit_string = (u_int8_t *) user_packet;
	for (i = 0; i < len; i++) {
		for (j = 0; j < 8; j++) {
			/*
			 * Most significant bit of shift register XOR with
			 * (8 * i + j)-th bit of the bit string.
			 */
			if ((shift_reg & msb_mask) ^
			    ((bit_string[i] & (0x80 >> j)) <<
			     (crc_len - 1 - (7 - j)))) {
				shift_reg = (shift_reg << 1) ^ gen_poly;
			}
			else {
				shift_reg = shift_reg << 1;
			}
		}
	}

	return shift_reg;
}
