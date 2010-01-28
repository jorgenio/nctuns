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
#include <sys/uio.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <assert.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include <regcom.h>
#include <packet.h>
#include <nctuns-dep/multiif.h>
#include <nctuns_api.h>
#include <scheduler.h>
#include <ethernet.h>
#include <mbinder.h>

#include <fcntl.h>
#include <ip.h>

extern RegTable			RegTable_;
extern  scheduler *scheduler_;

MODULE_GENERATOR(MultiIf);

MultiIf::MultiIf(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)  
{
	bzero(upports,sizeof(UpPort[32]));
	up_num = 0;
}  


MultiIf::~MultiIf() {

}


int MultiIf::init() {
	return(1);  
}

int MultiIf::send(ePacket_ *pkt){
	return NslObject::send(pkt);
}
  
int MultiIf::recv(ePacket_ *pkt) {

	Packet			*pkt_; 
	char			*p = 0 ; 
 	u_long			ipDst;
	int 			i;

	assert(pkt&&(pkt_=(Packet *)pkt->DataInfo_));
	p = pkt_->pkt_sget();
	assert(p);

	IP_DST(ipDst, p);
	for ( i = 0; i < up_num; i++){
		if ((upports[i].ip&upports[i].netmask) == (ipDst & upports[i].netmask)){
			return put(pkt,upports[i].target);
		}
	}
	
	return NslObject::recv(pkt); 
}


int MultiIf::command(int argc, const char *argv[]) {
	
	if (strcmp(argv[0],"Set")==0 && strncmp(argv[1],"IP_",3) == 0){
		const char *p;
		char *dup = strdup(argv[1]);
		int index = 0;
		u_long mask;
		
		p = strchr(argv[1],'/');
		index = (int)(p - argv[1]);
		dup[index] = 0;
		//p++;
		mask = atoi(p+1);
		
		upports[up_num].ip = inet_addr(dup+3);
		upports[up_num].netmask = htonl(0xffffffff << (32 - mask));
		upports[up_num].target = new MBinder(this);
		upports[up_num].target->bind_to(RegTable_.lookup_Instance(get_nid(), argv[3]));
		up_num++;
		printf("add %s %s\n", argv[1],argv[3]);
		free(dup);
		return 1;
	}
	return(NslObject::command(argc, argv));   
}

