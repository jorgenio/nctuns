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

#include <heap.h>
#include <scheduler.h>
#include <regcom.h>
#include <packet.h>
#include <nctuns-dep/interface.h>
#include <nctuns_api.h>
#include <ethernet.h>

#include <fcntl.h>
#include <ip.h>

extern RegTable			RegTable_;
extern scheduler *scheduler_;

MODULE_GENERATOR(interface);

interface::interface(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
		: NslObject(type, id, pl, name)  
{

	/* register to poller : 
	 * When the object's tunnel has packet
	 * to send, the event scheduler will
	 * call out to the interface and it's
	 * signal() member. In the member
	 * signal() must read out the packet.
	 */
	REG_POLLER(interface, signal, tunfd_, tid_);

	/* turn off flow control */
	r_flowctl = s_flowctl = DISABLED;

	/* bind variable */
	vBind_ip("ip", &ip_);
	vBind_ip("netmask", &netmask_);

	/* register variable */
        REG_VAR("IP", &ip_);
        REG_VAR("NETMASK", &netmask_);
        REG_VAR("TUNFD", &tunfd_);
	REG_VAR("TUNID", &tid_);
}  


interface::~interface() {

	close(tunfd_);
}


int interface::init() {

	/* get register variable */
	mac_ = GET_REG_VAR(get_port(), "MAC", u_char *);  

	/* set maptable */
	assert(set_tuninfo(get_nid(), get_port(), 
	       tid_, &ip_, &netmask_, mac_) == 0);
	return(1);  
}


  
int interface::recv(ePacket_ *pkt) {

	Packet			*pkt_;
	char			*p = 0 ; 
 	int			n;

	assert(pkt&&(pkt_=(Packet *)pkt->DataInfo_));
	p = pkt_->pkt_sget();
	
	/* write this packet to tunnel interface, here we
	 * simulate that a packet is received.
	 *
	 * Note: XXX 
	 *
	 *	We must write this packet with offsetting the length
	 * of ether_header to tunnel interface.
	 */
	if (pkt_->pkt_getlen() == 0) {
		freePacket(pkt);
		return(1);
	}
	n = write(tunfd_, p, pkt_->pkt_getlen()); 

	// after writing the packet, we need to inform scheduler to check tunnel0
	scheduler_->chkt0e();
   
	if(n != pkt_->pkt_getlen()) {
		printf("ERROR: write packet to tunnel!\n");
		printf("errno = %d\n",errno);
		exit(-1); 
	}
  
	/* release the packet space */
  	freePacket(pkt); 
	return(1); 
}


int interface::send(ePacket_ *pkt) {

	assert(pkt);
	return(put(pkt, sendtarget_)); 
}


/*
 * This member function should be called by event
 * scheduler. Whenever this member function is called,
 * it represents that this interface has packet to 
 * be sent.
 */
#include <com_HEADER/tcp.h>
#include <arpa/inet.h>
int interface::signal(Event_ *ep) {

	Packet			*pkt;
	char			*buf, *p;
	u_long			gw; 
	int			n;

	assert(ep&&(pkt=new Packet));
	/* malloc shared pbuf */
	buf = pkt->pkt_sattach(MAXPACKETSIZE); 

	/*
	 * Why from buf-sizeof(struct tether_header) !!??
	 *     When we use pkt_sattach() method to malloc
	 *     memory space, the space always be allocated
	 *     (len+PLEN) bytes. And when we read data from
	 *     tunnel device, the data is a ether-frame. The
	 *     header of this ether-frame is only used to 	
	 *     do routing or arp-request, so we don't take 
	 *     the ether header into consideration. 
	 */
	n = read(tunfd_, buf-sizeof(struct ether_header), MAXPACKETSIZE);

	if (n >= 0) {
		pkt->pkt_sprepend(buf, n-sizeof(struct ether_header));
		pkt->pkt_setflow(PF_SEND); 
		
		/*
		 * Because the kernel routing information is
		 * hid in the ether header of read-out data 
		 * from tunnel, I should to find this information
		 * and set it in Packet object.
		 */
		p = buf-sizeof(struct ether_header);
		(void)memcpy((char *)&gw, p, sizeof(u_long)); 
  		pkt->rt_setgw(gw); 
	} 
	else {
		if (errno == EWOULDBLOCK) {
			pkt->release(); 
			return(0);
		}
		assert(0);   
	}

	/* attach packet to event */
	SET_EVENT_CALLOUTOBJ(ep, NULL, NULL, (void *)pkt);

	/* call get() member function 
	 * to follow normal process
	 */
	assert(get(ep, 0) > 0); 

	return 1;
}


int interface::Debugger(Event *e) {

	u_char			*p;

 
	printf("DEBUG: interface object\n");
	NslObject::Debugger();
	printf("   tun fd: %d\n", tunfd_);
  
	p = (u_char *)&ip_;  
	printf("   ip = %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

	p = (u_char *)&netmask_;
	printf("   netmask = %d.%d.%d.%d\n", p[0], p[1], p[2], p[3]);

	return(1);
}

  
              
