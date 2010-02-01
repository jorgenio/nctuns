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

#ifndef __NCTUNS_nctuns_h__
#define __NCTUNS_nctuns_h__

#include <getopt.h>

#define CHAR_FOR_SHORT_OPT_HELP			'h'
#define CHAR_FOR_SHORT_OPT_VERSION		'v'
#define CHAR_FOR_SHORT_OPT_QUERY_CM_LIST	'm'
#define CHAR_FOR_SHORT_OPT_QUERY_CS_DIST	's'

#define OPT_STR_FOR_SHORT_OPT_CHAR		"hvm:s"

#define STR_FOR_LONG_OPT_HELP			"help"
#define STR_FOR_LONG_OPT_VERSION		"version"
#define STR_FOR_LONG_OPT_QUERY_CM_LIST		"query_cm_list"
#define STR_FOR_LONG_OPT_QUERY_CS_DIST		"query_cs_dist"

#define RETURN_VALUE_FOR_OPT_HELP		(int)CHAR_FOR_SHORT_OPT_HELP
#define RETURN_VALUE_FOR_OPT_VERSION		(int)CHAR_FOR_SHORT_OPT_VERSION
#define RETURN_VALUE_FOR_OPT_QUERY_CM_LIST	(int)CHAR_FOR_SHORT_OPT_QUERY_CM_LIST
#define RETURN_VALUE_FOR_OPT_QUERY_CS_DIST	(int)CHAR_FOR_SHORT_OPT_QUERY_CS_DIST

struct option LongOpts[] = {
	{"help", no_argument, NULL, RETURN_VALUE_FOR_OPT_HELP},
	{"version", no_argument, NULL, RETURN_VALUE_FOR_OPT_VERSION},
	{"query_cm_list", required_argument, NULL, RETURN_VALUE_FOR_OPT_QUERY_CM_LIST},
	{"query_cs_dist", no_argument, NULL, RETURN_VALUE_FOR_OPT_QUERY_CS_DIST}
};

#endif /* __NCTUNS_nctuns_h__ */
