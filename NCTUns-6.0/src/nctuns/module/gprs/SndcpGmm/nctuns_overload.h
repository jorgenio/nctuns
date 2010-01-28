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

int 
GprsGmm::get(ePacket_ *pkt, MBinder *frm) {

	bss_message	*p;
	p = (bss_message*)pkt->DataInfo_;
	
	if ( p->flag == PF_SEND ) {
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

	if (p->flag == PF_RECV) {
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



int 
GprsGmm::put(ePacket_ *pkt, MBinder *mo) {

	bss_message	*p;
	p = (bss_message*)pkt->DataInfo_;
	
	if (p->flag == PF_SEND) {
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

	if (p->flag == PF_RECV) {
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
	assert(0); 
}


ePacket_ * GprsGmm::put1(ePacket_ *pkt, MBinder *mo) {

        bss_message	*p;
	p = (bss_message*)pkt->DataInfo_;
	
	if (p->flag == PF_SEND) {
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

	if (p->flag == PF_RECV) {
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
