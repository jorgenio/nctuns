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

#ifndef __NCTUNS_WIMAX_MAC_ADDRESS_H__
#define __NCTUNS_WIMAX_MAC_ADDRESS_H__


#include <cstddef>
#include <stdint.h>


class Mac_address {

private:
    enum {
        ADDRESS_LEN = 6,
        ADDRESS_STR_LEN = ADDRESS_LEN * 3,
    };

private:

protected:
    uint8_t _buf[ADDRESS_LEN];
    char    _str[ADDRESS_STR_LEN];

public:
	Mac_address(uint8_t* = NULL);
	virtual ~Mac_address();

    char* str();
    uint64_t scalar();
    void copy_from(uint8_t*, size_t);
    void copy_to(uint8_t*, size_t);

    inline uint8_t* buf() { return _buf; }

    inline bool operator ==(Mac_address& mac)
    { return scalar() == mac.scalar(); }
    inline bool operator !=(Mac_address& mac)
    { return scalar() != mac.scalar(); }
};

#endif /* __NCTUNS_WIMAX_MAC_ADDRESS_H__ */
