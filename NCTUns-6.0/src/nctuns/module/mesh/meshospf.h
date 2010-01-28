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

#ifndef __NCTUNS_MeshOSPF_h__
#define __NCTUNS_MeshOSPF_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <timer.h>
#include <string>

#include "meshlib/ospf.h"

#define MAGICN 0x1F

extern int use_etxlib_flag;

using namespace std;

class MeshOSPF : public NslObject {

    private:
	ospf_d_etx*             ospf_etx;
        ospf_d_ori*             ospf_ori;
	keytype			ospf_id;
	struct mesh_if		*ifaces[32];
	int			ifaces_num;
	timerObj 		heloTimer;
	struct mesh_rtb 	route_table;
	int 			helocount;
        int                     use_etx_flag;
        char*			tmp_etx_flag_str;

     public:
 	MeshOSPF(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~MeshOSPF();   

	int 			init(); 
	int			recv(ePacket_ *pkt);
	int 			command(int argc, const char *argv[]);	
        int                     get(ePacket_ *pkt, MBinder *frm);
        int                     send(ePacket_ *pkt, MBinder *frm);
	int 			getSrcDstMac(keytype *src, keytype *dst, ePacket_ *pkt);
	int			helo();
        int 			determine_fixed_lan_port(MBinder* frma);

}; 
 

#endif /* __NCTUNS_MeshOSPF_h__ */
