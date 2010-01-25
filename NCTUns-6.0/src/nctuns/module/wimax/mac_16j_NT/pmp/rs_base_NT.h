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

#ifndef __NCTUNS_80216J_NT_RS_BASE_H__
#define __NCTUNS_80216J_NT_RS_BASE_H__

#include "../mac802_16j_NT.h"

using namespace MR_Mac80216j_NT;

namespace MR_RSbase_NT {
	class mac802_16RSbase : public mac802_16j_NT {
		public:
			uint16_t    BasicCID;
			uint16_t    PriCID;
			uint16_t    SecCID;
			uint8_t     AttrARQ;

		public:
			explicit mac802_16RSbase();
			explicit mac802_16RSbase(uint32_t, uint32_t, plist*, const char*);
	};
}

#endif /* __NCTUNS_80216J_NT_RS_BASE_H__ */
