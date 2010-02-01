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

#include "mac_address.h"

using namespace mobileRelayMacAddress;

Mac_address::Mac_address(uint8_t* addr)
{
	if (addr)
		memcpy(_buf, addr, ADDRESS_LEN);
	else
		memset(_buf, 0, ADDRESS_LEN);

	memset(_str, 0, ADDRESS_STR_LEN);
}

Mac_address::~Mac_address()
{
	;
}

char *Mac_address::str()
{
	sprintf(_str, "%02x:%02x:%02x:%02x:%02x:%02x", _buf[0], _buf[1], _buf[2], _buf[3], _buf[4], _buf[5]);

	return _str;
}

uint64_t Mac_address::scalar()
{
	return ((uint64_t)_buf[0] << 40) + ((uint64_t)_buf[1] << 32) + ((uint64_t)_buf[2] << 24) +
		((uint64_t)_buf[3] << 16) + ((uint64_t)_buf[4] <<  8) + ((uint64_t)_buf[5] <<  0);
}

void Mac_address::copy_from(uint8_t* src_addr, size_t len)
{
	if (len > ADDRESS_LEN)
		len = ADDRESS_LEN;

	memcpy(_buf, src_addr, len);
}

void Mac_address::copy_to(uint8_t* dst_addr, size_t len)
{
	if (len > ADDRESS_LEN)
		len = ADDRESS_LEN;

	memcpy(dst_addr, _buf, len);
}
