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

#ifndef __NCTUNS_DSR_OPTION_H
#define __NCTUNS_DSR_OPTION_H

#include <cassert>

struct RREQ {
    u_int8_t	option_type;
    u_int8_t	opt_data_len;
    
    u_int16_t	identification;
    u_int32_t	destination;
};

struct RREP {
    u_int8_t	option_type;
    u_int8_t	opt_data_len;
    
    u_int8_t	l:1,
    		reserved:7;
};

struct RERR {
    u_int8_t	option_type;
    u_int8_t	opt_data_len;
    
    u_int8_t	error_type;
    u_int8_t	reservd:4,
                salvage:4;
    u_long	source;
    u_long	destination;
    u_long	info;
};    

struct ROUTE {
    u_int8_t	option_type;
    u_int8_t	opt_data_len;
    
    u_int8_t	f:1,
    		l:1,
    		segs_left:6;
    u_int8_t	reservd:4,
    		salvage:4;
};
    
#endif // __NCTUNS_DSR_OPTION_H
