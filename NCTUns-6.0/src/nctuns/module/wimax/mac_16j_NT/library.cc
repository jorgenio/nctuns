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

#include "library.h"

/* ========================================================================= */
uint32_t OFDMA_crc_table_NT[256] = {
	0x00000000L, 0xb71dc104L, 0x6e3b8209L, 0xd926430dL, 0xdc760413L,
	0x6b6bc517L, 0xb24d861aL, 0x0550471eL, 0xb8ed0826L, 0x0ff0c922L,
	0xd6d68a2fL, 0x61cb4b2bL, 0x649b0c35L, 0xd386cd31L, 0x0aa08e3cL,
	0xbdbd4f38L, 0x70db114cL, 0xc7c6d048L, 0x1ee09345L, 0xa9fd5241L,
	0xacad155fL, 0x1bb0d45bL, 0xc2969756L, 0x758b5652L, 0xc836196aL,
	0x7f2bd86eL, 0xa60d9b63L, 0x11105a67L, 0x14401d79L, 0xa35ddc7dL,
	0x7a7b9f70L, 0xcd665e74L, 0xe0b62398L, 0x57abe29cL, 0x8e8da191L,
	0x39906095L, 0x3cc0278bL, 0x8bdde68fL, 0x52fba582L, 0xe5e66486L,
	0x585b2bbeL, 0xef46eabaL, 0x3660a9b7L, 0x817d68b3L, 0x842d2fadL,
	0x3330eea9L, 0xea16ada4L, 0x5d0b6ca0L, 0x906d32d4L, 0x2770f3d0L,
	0xfe56b0ddL, 0x494b71d9L, 0x4c1b36c7L, 0xfb06f7c3L, 0x2220b4ceL,
	0x953d75caL, 0x28803af2L, 0x9f9dfbf6L, 0x46bbb8fbL, 0xf1a679ffL,
	0xf4f63ee1L, 0x43ebffe5L, 0x9acdbce8L, 0x2dd07decL, 0x77708634L,
	0xc06d4730L, 0x194b043dL, 0xae56c539L, 0xab068227L, 0x1c1b4323L,
	0xc53d002eL, 0x7220c12aL, 0xcf9d8e12L, 0x78804f16L, 0xa1a60c1bL,
	0x16bbcd1fL, 0x13eb8a01L, 0xa4f64b05L, 0x7dd00808L, 0xcacdc90cL,
	0x07ab9778L, 0xb0b6567cL, 0x69901571L, 0xde8dd475L, 0xdbdd936bL,
	0x6cc0526fL, 0xb5e61162L, 0x02fbd066L, 0xbf469f5eL, 0x085b5e5aL,
	0xd17d1d57L, 0x6660dc53L, 0x63309b4dL, 0xd42d5a49L, 0x0d0b1944L,
	0xba16d840L, 0x97c6a5acL, 0x20db64a8L, 0xf9fd27a5L, 0x4ee0e6a1L,
	0x4bb0a1bfL, 0xfcad60bbL, 0x258b23b6L, 0x9296e2b2L, 0x2f2bad8aL,
	0x98366c8eL, 0x41102f83L, 0xf60dee87L, 0xf35da999L, 0x4440689dL,
	0x9d662b90L, 0x2a7bea94L, 0xe71db4e0L, 0x500075e4L, 0x892636e9L,
	0x3e3bf7edL, 0x3b6bb0f3L, 0x8c7671f7L, 0x555032faL, 0xe24df3feL,
	0x5ff0bcc6L, 0xe8ed7dc2L, 0x31cb3ecfL, 0x86d6ffcbL, 0x8386b8d5L,
	0x349b79d1L, 0xedbd3adcL, 0x5aa0fbd8L, 0xeee00c69L, 0x59fdcd6dL,
	0x80db8e60L, 0x37c64f64L, 0x3296087aL, 0x858bc97eL, 0x5cad8a73L,
	0xebb04b77L, 0x560d044fL, 0xe110c54bL, 0x38368646L, 0x8f2b4742L,
	0x8a7b005cL, 0x3d66c158L, 0xe4408255L, 0x535d4351L, 0x9e3b1d25L,
	0x2926dc21L, 0xf0009f2cL, 0x471d5e28L, 0x424d1936L, 0xf550d832L,
	0x2c769b3fL, 0x9b6b5a3bL, 0x26d61503L, 0x91cbd407L, 0x48ed970aL,
	0xfff0560eL, 0xfaa01110L, 0x4dbdd014L, 0x949b9319L, 0x2386521dL,
	0x0e562ff1L, 0xb94beef5L, 0x606dadf8L, 0xd7706cfcL, 0xd2202be2L,
	0x653deae6L, 0xbc1ba9ebL, 0x0b0668efL, 0xb6bb27d7L, 0x01a6e6d3L,
	0xd880a5deL, 0x6f9d64daL, 0x6acd23c4L, 0xddd0e2c0L, 0x04f6a1cdL,
	0xb3eb60c9L, 0x7e8d3ebdL, 0xc990ffb9L, 0x10b6bcb4L, 0xa7ab7db0L,
	0xa2fb3aaeL, 0x15e6fbaaL, 0xccc0b8a7L, 0x7bdd79a3L, 0xc660369bL,
	0x717df79fL, 0xa85bb492L, 0x1f467596L, 0x1a163288L, 0xad0bf38cL,
	0x742db081L, 0xc3307185L, 0x99908a5dL, 0x2e8d4b59L, 0xf7ab0854L,
	0x40b6c950L, 0x45e68e4eL, 0xf2fb4f4aL, 0x2bdd0c47L, 0x9cc0cd43L,
	0x217d827bL, 0x9660437fL, 0x4f460072L, 0xf85bc176L, 0xfd0b8668L,
	0x4a16476cL, 0x93300461L, 0x242dc565L, 0xe94b9b11L, 0x5e565a15L,
	0x87701918L, 0x306dd81cL, 0x353d9f02L, 0x82205e06L, 0x5b061d0bL,
	0xec1bdc0fL, 0x51a69337L, 0xe6bb5233L, 0x3f9d113eL, 0x8880d03aL,
	0x8dd09724L, 0x3acd5620L, 0xe3eb152dL, 0x54f6d429L, 0x7926a9c5L,
	0xce3b68c1L, 0x171d2bccL, 0xa000eac8L, 0xa550add6L, 0x124d6cd2L,
	0xcb6b2fdfL, 0x7c76eedbL, 0xc1cba1e3L, 0x76d660e7L, 0xaff023eaL,
	0x18ede2eeL, 0x1dbda5f0L, 0xaaa064f4L, 0x738627f9L, 0xc49be6fdL,
	0x09fdb889L, 0xbee0798dL, 0x67c63a80L, 0xd0dbfb84L, 0xd58bbc9aL,
	0x62967d9eL, 0xbbb03e93L, 0x0cadff97L, 0xb110b0afL, 0x060d71abL,
	0xdf2b32a6L, 0x6836f3a2L, 0x6d66b4bcL, 0xda7b75b8L, 0x035d36b5L,
	0xb440f7b1L
};
/* ========================================================================= */

uint32_t crc32_16j_NT(uint32_t crc, char *buf, int len)
{
	int a = 0;

	if (buf == NULL)
		return 0L;

	memcpy(&crc, buf, 4);
	crc = 0xffffffffL ^ crc;

	while (len != 0)
	{
		a = (crc) & 0xff;
		buf++;
		len--;
		crc = crc >> 8;

		if (len >= 4)
			crc += (buf[3] << 24);

		crc = crc ^ OFDMA_crc_table_NT[a];
	}
	return crc ^ 0xffffffffL;
}

uint8_t hcs_16j_NT(char *buf, int len)
{
	// Generator: D^8 + D^2 + D + 1

	char crc = 0x00;
	int i;
	char b;

	for (i = 0; i < len; i++) {
		crc ^= buf[i];

		for (b = 0; b < 8; ++b) {
			if (crc & 0x80)
				crc = (crc << 1) ^ 0x07;
			else
				crc = (crc << 1);
		}
	}
	return crc;
}

/* Ranging codes: PRBS generator (Spec 16e. 8.4.7.3)*/
void gen_ranging_code_NT(uint8_t perm_base, uint8_t code[][18], int code_num)
{
	int seed = ((perm_base & 0x7F) << 8) | 0x00D4;
	uint8_t buf[18] = "";
	int index = 0;
	int tmp = 0;

	for(index = 0;index < code_num;index++)
	{
		memset(buf, 0, 18);

		for(int i = 0;i < 144;i++)
		{
			tmp = (seed & 0x1) ^ ((seed & 0x100) >> 8) ^ ((seed & 0x800) >> 11) ^ ((seed & 0x4000) >> 14);

			buf[i / 8] <<= 1;
			buf[i / 8] |= tmp;

			seed = (seed >> 1) | (tmp << 14);
		}

		memcpy(code[index], buf, 18);
	}
}
