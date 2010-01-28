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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>
#include <fcntl.h>
#include <assert.h>
#include <iostream>
#include <udp.h>
#include <ip.h>
#include <ethernet.h>
#include <packet.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <nctuns_syscall.h>
#include <tcpdump/tcpdump.h>
#include <errno.h>

#include <tun_mmap.h>

extern RegTable			RegTable_;

MODULE_GENERATOR(tcpdump);


tcpdump::tcpdump(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
 : NslObject(type, id, pl, name)
{
	dump_flag = 0;	
	cachesize = 0;
}

tcpdump::~tcpdump() {

	close(dumptunfd);
}


void tcpdump::modifypkt(Packet *pkt) {

	char			*ptr;

	assert(pkt);
        agptr = (char *)pkt->pkt_aggregate();

        ptr = (char *)(agptr + pkt->pkt_getlen());
        assert(agptr);
        assert(ptr);

	/* bring the information in the additional 2 bytes to the packet */
	if (pktflow == SEND_FLOW) {
		/* tunID or choose a tunnel for tcpdump if the module
                 * didn't attach an interface.
                 */
		ptr[0] = (u_char)(*tunID); 
		ptr[1] = 0;
	}
	else {
		ptr[0] = 0;
		/* tunID or choose a tunnel for tcpdump if the module
                 * didn't attach an interface.
                 */
		ptr[1] = (u_char)(*tunID);
	}
}



void tcpdump::pktdump(Packet *pkt) {
	
/*
	char                    *testp;
        u_short                 portc, vportc, temp;
	int                     iphl,i;
	char                    mark = 0;
*/
	
	/*add by jackyu, for port fix*/
/*
	testp = pkt->pkt_sget();
	if(testp){
		iphl = (int)(testp[0] & 0x0f) * 4;
		bcopy(testp+iphl, &portc, sizeof(u_short));
		temp = portc;
	
		for(i=0; i<cachesize; i++){
			if(portc == portcache[i].rport){
				vportc = portcache[i].vport;
				mark = 1;
				break;
			}
		}
		
		if(mark == 0){
			cachesize = (cachesize+1)%100;
#ifdef LINUX
			syscall_NCTUNS_misc(syscall_NSC_misc_R2VPORT, (u_short)ntohs(portc), 0, (unsigned long)&vportc);
#else
			syscall(290, 0x07, portc, 0, &vportc);
#endif
			portcache[cachesize-1].rport = temp;
			portcache[cachesize-1].vport = vportc;
		}
		vportc = htons(vportc);
		bcopy(&vportc, testp+iphl, sizeof(u_short));
	}						
*/
	
	/* bring the information in the additional 2 bytes to the packet */
	if (write(dumptunfd, agptr, pkt->pkt_getlen() + 2) < 0)
		printf("dump packet error\n");
	
/*
	if(testp)
		bcopy(&temp, testp+iphl, sizeof(u_short));
*/
}



int tcpdump::recv(ePacket_ *pkt) {

	Packet                  *p;

        assert(pkt&&pkt->DataInfo_);


	if( dump_flag == 1 ) {
		p = (Packet *)pkt->DataInfo_;
		pktflow = RECV_FLOW;
		modifypkt(p);
		pktdump(p);
	}

        return(put(pkt, recvtarget_));
}


int tcpdump::send(ePacket_ *pkt) {

	Packet			*p;


        assert(pkt&&pkt->DataInfo_);

	if( dump_flag == 1 ) {
		p = (Packet *)pkt->DataInfo_;
		pktflow = SEND_FLOW;
        	modifypkt(p);
        	pktdump(p);
	}

        return(put(pkt, sendtarget_));
}



int tcpdump::init() {

        NslObject::init();	
	EXPORT("DumpFlag", E_WONLY);

	tunID = GET_REG_VAR1(get_portls(), "TUNID", u_long *);

 	if (dumptunfd < 0) {
		dumptunfd = tun_alloc("tun0");
 		if (dumptunfd < 0) {
 			printf("Error: tcpdump %d: open tunnel tun0 error\n",
					get_nid());
			exit(-1);
		}
        }

        return(1);
}



int tcpdump::command(int argc, const char *argv[]) {

	if (!strcmp(argv[0], "Set")&&(argc==3)) {
	  if (!strcmp(argv[1], "DumpFlag")) {
	    if (!strcmp(argv[2], "on")) {
		dump_flag = 1;
	    }
	    else {
		dump_flag = 0;
	    }

	    EXPORT_SET_SUCCESS();
	    return 1 ;
	  }
        }
	
        return(NslObject::command(argc, argv));
}
