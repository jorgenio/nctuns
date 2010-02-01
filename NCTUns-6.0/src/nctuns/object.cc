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
#include <assert.h>

#include <mbinder.h>
#include <nctuns_api.h>
#include <nodetype.h>
#include <object.h>
#include <packet.h>
#include <regcom.h>

extern RegTable RegTable_;
extern typeTable *typeTable_ ;

	NslObject::NslObject(u_int32_t type, u_int32_t id, struct plist *MPlist, const char *name) 
: nodeID_(id), nodeType_(type)
{

	struct plist* tmp;
	struct plist* tmp1;

	portid_ = 0;
	assert(name);
	name_ = strdup(name);

	/* set up Binder list*/
	BinderList = NULL;
	PortNum = 0;
	MPlist_ = 0;

	/* allocate output buffer space */
	recvtarget_ = new MBinder(this);
	/* now sendtarget_ becomes the "default binder" */
	sendtarget_ = new MBinder(this);
	assert(recvtarget_&&sendtarget_);

	/* initial flow contrl flag */
	s_flowctl = ENABLED;
	r_flowctl = ENABLED;
	pdepth = 0;

	/* set up MPlist */
	tmp1 = MPlist;
	while(tmp1){
		if(!MPlist_){
			MPlist_ = (struct plist *)malloc(sizeof(struct plist));
			MPlist_->pid = tmp1->pid;
			MPlist_->next = NULL;
			tmp = MPlist_;
		}
		else{
			tmp->next = (struct plist *)malloc(sizeof(struct plist));
			tmp = tmp->next;
			tmp->pid = tmp1->pid;
			tmp->next = NULL;
		}
		tmp1 = tmp1->next;
		pdepth++;
	}
	if(MPlist_){
		portid_ = tmp->pid;
	}
}

	NslObject::NslObject()
: name_(NULL), nodeID_(0), nodeType_(0), portid_(0), recvtarget_(NULL), sendtarget_(NULL)
{
}


NslObject::~NslObject()
{
	if (sendtarget_)
		delete sendtarget_;

	if (recvtarget_)
		delete recvtarget_;

	if (name_)
		delete [] name_; 
}


int NslObject::command(int argc, const char *argv[]) {

	NslObject* obj = NULL;

	if (argc == 0 ) return(-1);

	/* check syntax */
	if (argc!=4 || strcmp(argv[0], "Set")) {
		printf("Syntax error : %s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
		return(-1);	
	}

    bool uncanonical_module_name_flag = 0;

    /* The "." sign is not allowed as part of a module name. */
    if ( strstr(argv[3],".") ) {
        uncanonical_module_name_flag = 1;
    }
        /* The RegTable->lookup_Instance() with two parameter version
        * should use the canonical name of a module. If an illegal
        * module name is found, we should return immediately.
    * A canonical module name should start with
    * a prefix "NODE." As such, for any module names
    * without this prefix, this function simply returns
    * the original string as its output because the
    * name translation process may fail with incorrect
    * input names.
    */
    if (strncasecmp("NODE", argv[3], 4))
        uncanonical_module_name_flag = 1;

    /* A canonical module name should start with
    * a prefix "NODE." As such, for any module names
    * without this prefix, this function simply returns
    * the original string as its output because the
    * name translation process may fail with incorrect
    * input names.
    */
    
	/* Connectivity */
    if (!uncanonical_module_name_flag) {
    
        obj = RegTable_.lookup_Instance(nodeID_, argv[3]);
        if (!obj) {
          printf("Instance error : %s %s %s %s\n",argv[0],argv[1],argv[2],argv[3]);
          printf("No %s this Instance!\n\n", argv[3]);
          return(-1);
        }

   }

	/* new added part, support for tree structure */
	MBinder         *tmpMBinder;
	struct MBlist   *tmpPort = BinderList;
	u_int32_t       portNum;

	if (!strncmp(argv[1], "port", 4)){
		sscanf(argv[1], "port%d", &portNum);
		PortNum++;

		tmpMBinder = new MBinder(this);
		assert(tmpMBinder);
		tmpMBinder->bind_to(obj);

		if(!BinderList){
			BinderList = (struct MBlist *)malloc(sizeof(struct MBlist));
			tmpPort = BinderList;
			BinderList->next = NULL;

			tmpPort->portnum = (u_int8_t)portNum;
			tmpPort->sendt = tmpMBinder;
		}
		else{	
			tmpPort = BinderList;
			while(tmpPort->next != NULL)
				tmpPort = tmpPort->next;

			tmpPort->next = (struct MBlist *)malloc(sizeof(struct MBlist));
			tmpPort = tmpPort->next;
			tmpPort->portnum = (u_int8_t)portNum;
			tmpPort->sendt = tmpMBinder;
			tmpPort->next = NULL;					
		}
		return(1);
	}
	if (!strcmp(argv[1], "sendtarget"))
		sendtarget_->bind_to(obj);
	else if (!strcmp(argv[1], "recvtarget"))
		recvtarget_->bind_to(obj);
	else {
		printf("NslObject::command(): Invalid pre-configuration command: ");
		printf("%s %s %s %s\n", argv[0], argv[1], argv[2], argv[3]);
		return(-1);
	}
	return(1);
}


int NslObject::init() {

	/* check dynamic link */

	if(BinderList)
		sendtarget_ = BinderList->sendt;
	if(!sendtarget_->bindModule()) return(-1); 
	//if(!recvtarget_->bindModule()) return(-1);

	return(1); 
}


inline int NslObject::recv(ePacket_ *pkt) {

	assert(pkt);
	assert(pkt->DataInfo_);

	return(put(pkt, recvtarget_));  
}



inline int NslObject::send(ePacket_ *pkt) {

	assert(pkt);
	assert(pkt->DataInfo_);

	return(put(pkt, sendtarget_)); 
}



int 
NslObject::get(ePacket_ *pkt, MBinder *frm) {

	Packet			*p;

	p = (Packet *)pkt->DataInfo_;
	if (p->pkt_getflags()&PF_SEND) {
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

	if (p->pkt_getflags()&PF_RECV) {
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
NslObject::put(ePacket_ *pkt, MBinder *mo) {

	Packet			*p;

	p = (Packet *)pkt->DataInfo_;
	if (p->pkt_getflags()&PF_SEND) {
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

	if (p->pkt_getflags()&PF_RECV) {
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


ePacket_ * NslObject::put1(ePacket_ *pkt, MBinder *mo) {

	Packet                  *p;


	p = (Packet *)pkt->DataInfo_;
	if (p->pkt_getflags()&PF_SEND) {
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

	if (p->pkt_getflags()&PF_RECV) {
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



int NslObject::Debugger() {

	printf("Instance name: %s\n", name_);
	printf("Node ID: %d,  Node type: %s\n", 
			nodeID_, typeTable_->toName(nodeType_));
	printf("variables: \n");

	return(1); 
}


