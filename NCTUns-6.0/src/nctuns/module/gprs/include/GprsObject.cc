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
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <netdb.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <object.h>
#include <packet.h>
#include <mbinder.h>
#include "GprsObject.h"
#include "bss_message.h"



GprsObject::GprsObject(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
	:NslObject(type, id, pl, name), manageque(NULL)
{
}

GprsObject::~GprsObject()
{
}

int GprsObject::init()
{
	return NslObject::init();
}

void GprsObject::pushManage()
{
	ManageQue* q = manageque;
	while(q){
		/* added by cclin */
		ManageQue* tmp_manageque = manageque;
		manageque = (ManageQue*)manageque->next;
		
		free(tmp_manageque);

		NslObject::put(q->pkt,sendtarget_);
		q = manageque;
	}
}

int GprsObject::sendManage(ePacket_ *pkt)
{
	ePacket_ *retpkt;

	SET_MLEVEL_3(pkt);
	sendtarget_->set_upcall(this,(int (NslObject::*)(MBinder *))&GprsObject::pushManage);

	if ((retpkt = put1(pkt,sendtarget_))){

		/* cclin try */
		destroy_gprs_pkt(retpkt);
		return 1;

		if (!manageque){
			manageque = (ManageQue*)malloc(sizeof(ManageQue));
			manageque->pkt = retpkt;
			manageque->next = NULL;
		}else{
			ManageQue* q = manageque;
			while(q->next)q = (ManageQue*)q->next;
			q->next = (ManageQue*)malloc(sizeof(ManageQue));
			q = (ManageQue*)q->next;
			q->pkt = retpkt;
			q->next = NULL;
		}
	}
	return(1);
}

int GprsObject::send(ePacket_ *pkt)
{
	return NslObject::send(pkt);
}

int GprsObject::recv(ePacket_ *pkt)
{
	return NslObject::recv(pkt);
}

int GprsObject::get(ePacket_ *pkt, MBinder *frm) 
{

	bss_message	*p = NULL;
	Packet* pk = (Packet*)pkt->DataInfo_;
	p = (bss_message*)pkt->DataInfo_;
	//if(!p->user_data)printf("GprsDEBUG::%s\n",p->user_data);
	//Packet *pk = (Packet*)pkt->DataInfo_;
	//if ( pk->pkt_getflags()&PF_SEND ) {
	if ( pk->pkt_getflags() & PF_SEND ) {
		/*
		 * If the s_flowctl is disabled, then it is
		 * the module writer's duty to do flow
		 * control. Of course, the module writer may
		 * accept packet form other module without 
		 * doing any flow control. The flow control may 
		 * be made in send() member function.
		 */
		if (s_flowctl == DISABLED) {
			/* flow control is disabled */
			return(send(pkt));
		}

		/*
		 * If the s_flowctl is enabled, then the module
		 * writer still have a chance to do flow control.
		 * This can be made in send() member function by
		 * module writer.
		 *
		 * Note XXX:
		 *	If the flow control is enabled, the queue
		 *	that SE will check is the sendtarget_!!!!
		 */
		if (!sendtarget_->qfull()) 
			/* flow control is enabled */
			return( send(pkt) );
		else return(-1); 
	}

	//if ( pk->pkt_getflags()&PF_RECV ) {
	if (pk->pkt_getflags() & PF_RECV) {
		/*
                 * If the s_flowctl is disabled, then it is
		 * the module writer's duty to do flow
		 * control. Of course, the module writer may
		 * accept packet form other module without 
		 * doing any flow control. The flow control may 
		 * be made in recv() member function.
		 */
		if (r_flowctl == DISABLED) {
			/* flow control is disabled */
			return(recv(pkt));
		}

		/*
                 * If the s_flowctl is enabled, then the module
                 * writer still have a chance to do flow control.
                 * This can be made in recv() member function by
                 * module writer.
                 *
                 * Note XXX:
                 *      If the flow control is enabled, the queue
                 *      that SE will check is the recvtarget_!!!!
                 */
		if (!recvtarget_->qfull())
			/* flow control is enabled */
			return( recv(pkt) );
		else return(-1);  
	}
	assert(0); 
}


int GprsObject::put(ePacket_ *pkt, MBinder *mo) 
{

	bss_message	*p = NULL;
	Packet* pk = (Packet*)pkt->DataInfo_;
	p = (bss_message*)pkt->DataInfo_;
	//Packet *pk = (Packet*)pkt->DataInfo_;
	
	//if(!p->user_data)printf("GprsDEBUG::%s\n",p->user_data);
	if (pk->pkt_getflags() & PF_SEND) {
		/*
		 * Call next module's get() method to try to
		 * push packet to it. If get() return < 0,
		 * we should queue this packet and the 
		 * MBP will try to push it again.
		 */
		if (mo->bindModule() == NULL) {
			printf("%s: next module is not assigned...\n", 
				get_name());
			printf("\nAbort......\n\n");
			exit(-1);
		}
		if((mo->bindModule())->get(pkt, mo) < 0) {
			/* next module is busy */
			if (mo->enqueue(pkt) != NULL) {
				printf("Module(%s): Queue is full while lower module is busy!\nAbort Simulation.\n\n", get_name());
				exit(-1);
			}	
		} else {
			/* the packet is successfully passed to next
			 * module, so we must have a chance to make 
			 * an upcall in the same module.
			 */
			mo->try_upcall();
		}
		return(1);
	} 

	if (pk->pkt_getflags() & PF_RECV) {
		/*
                 * Call next module's get() method to try to
                 * push packet to it. If get() return < 0,
                 * we should queue this packet and the 
                 * MBP will try to push it again.
                 */
		if (mo->bindModule() == NULL) {
			printf("%s: upper module is not assigned...\n",
				get_name());
			printf("\nAbort......\n");
			exit(-1);
		}
		if((mo->bindModule())->get(pkt, mo) < 0) {
			/* upper module is busy */
			if (mo->enqueue(pkt) != NULL) {
				printf("Module(%s): Queue is full while upper module is busy!\nAbort Simulation.\n\n", get_name());
				exit(-1);
			}	
		} else {
			/* the packet is succssfully passed to next
 			 * module, so we must have a chance to make
 			 * an upcall in the same module.
 			 */
			mo->try_upcall();
		}
		return(1); 
	}
	//return NslObject::put(pkt,mo);
	assert(0); 
}


ePacket_ * GprsObject::put1(ePacket_ *pkt, MBinder *mo) 
{

        bss_message	*p;
	p = (bss_message*)pkt->DataInfo_;
	
	if (p->flag & PF_SEND) {
		/*
                 * Call next module's get() method to try to
                 * push packet to it. If get() return < 0,
                 * we should queue this packet and the 
                 * MBP will try to push it again.
                 */
		if (mo->bindModule() == NULL) {
                        printf("%s: next module is not assigned...\n",
                                get_name());
                        printf("\nAbort......\n\n");
                        exit(-1);
                }
		if((mo->bindModule())->get(pkt, mo) < 0) {
			/*
			 * If the enqueue() return a non-NULL value,
			 * it means that the MBQ is full and a packet was
			 * dequeued from the MBQ. (The MBQ will dequeue a 
			 * packet with lowe priority.)
			 */
			return(mo->enqueue(pkt));
		} else {
                       /* the packet is successfully passed to next
                        * module, so we must have a chance to make 
                        * an upcall in the same module.
                        */
                       mo->try_upcall();
                }
		return(NULL);
	}

	if (p->flag & PF_RECV) {
		/*
                 * Call next module's get() method to try to
                 * push packet to it. If get() return < 0,
                 * we should queue this packet and the 
                 * MBP will try to push it again.
                 */
		if (mo->bindModule() == NULL) {
                        printf("%s: upper module is not assigned...\n",
                                get_name());
                        printf("\nAbort......\n");
                        exit(-1);
                }
		if((mo->bindModule())->get(pkt, mo) < 0) {
			/*
                         * If the enqueue() return a non-NULL value,
                         * it means that the MBQ is full and a packet was
                         * dequeued from the MBQ. (The MBQ will dequeue a 
                         * packet with lowe priority.)
                         */
			return(mo->enqueue(pkt));

		} else {
                        /* the packet is succssfully passed to next
                         * module, so we must have a chance to make
                         * an upcall in the same module.
                         */
                        mo->try_upcall();
                }
		return(NULL);
	}
	assert(0);
}

