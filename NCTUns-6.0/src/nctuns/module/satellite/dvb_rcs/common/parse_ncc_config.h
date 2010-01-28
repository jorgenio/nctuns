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

#ifndef __NCTUNS_PARSE_NCC_CONFIG__
#define __NCTUNS_PARSE_NCC_CONFIG__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>
#include <list>

#include <satellite/dvb_rcs/common/descriptor/common.h>
#include <satellite/dvb_rcs/common/rcst_info.h>
#include <satellite/dvb_rcs/common/ncc_config.h>

#define	ATM_CELL_SIZE		53
#define	RS_OVERHEAD		16	//in bits.
#define	INVERSE_CC_RATE		2

int parse_ncc_config(char* path, Ncc_config &ncc_config, Rcst_info_list& rcst_infos);

void show_rcst_info_list(Rcst_info_list& rcst_info_list, Ncc_config& ncc_config);
#endif	/*__NCTUNS_PARSE_NCC_CONFIG__*/
