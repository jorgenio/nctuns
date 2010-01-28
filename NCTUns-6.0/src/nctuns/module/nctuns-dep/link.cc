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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

#include <regcom.h>
#include <heap.h>
#include <scheduler.h>
#include <nctuns_api.h>
#include <nctuns-dep/link.h>
#include <packet.h>
#include <mbinder.h>

#include <ethernet.h>

extern RegTable                 RegTable_;
extern scheduler       		*scheduler_; 

SLIST_HEAD(headOfLink, con_list)	headOfWire_;
struct headOfLink			headOfWireless_;
struct headOfLink			headOfGPRSRF_;
struct headOfLink			headOfWiMAX_;
struct headOfLink			headOfMobileWIMAX_;
struct headOfLink                       headOfMobileRelayWIMAX_;
struct headOfLink			headOfMRWIMAXNT_;
struct headOfLink			headOfSat_;
struct headOfLink			headOfWAVE_;

MODULE_GENERATOR(Link);


Link::Link(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
	s_flowctl = DISABLED; 
	r_flowctl = DISABLED;
	
	REG_VAR("ConnNodeID",&ConnNodeID);

	SLIST_INIT(&hos_);
	/*
	 * Note:
	 *	There is no specific sendtarget of Link module for
	 *	wireless node, but this will cause a connectivity
	 *	error in the Node module (node.cc) when initilization,
	 *	therefore, we set the sendtarget module to itself.
	 */
	sendtarget_->bind_to(this);	
}


Link::~Link() {

}


int Link::init() {

	ConnNodeID = getConnectNode(get_nid(), get_portls());
	return(1);  
}

  

int Link::recv(ePacket_ *pkt) {

	assert(pkt);
	return(put(pkt, recvtarget_)); 
}


int Link::send(ePacket_ *pkt) {

	Packet			*p;

	assert(pkt&&(p=(Packet *)pkt->DataInfo_));  
	p->pkt_setflow(PF_RECV); 

	/* 
	 * only wired network's packets would be passed through
	 * this function (only HUB device)
	 */
	if(!strcmp("WIRE", nettype_)||!strcmp("wire", nettype_))
		return(put(pkt, sendtarget_));

	return 1;
}



int DotSeparate(const char *sr, char *cm1, char *cm2) {

	const char *dotPtr;

	if ((dotPtr = strchr(sr, '.'))) {
		int cm1Len;

		cm1Len = dotPtr - sr;
		strncpy(cm1, sr, cm1Len);
		cm1[cm1Len] = '\0';

		strcpy(cm2, dotPtr + 1);

		return (1);
	}
	return (-1);
}


int Link::command(int argc, const char *argv[]) {

	int				i; 
	struct con_list			*wi, *wd;
	char				cm1[100], cm2[100];
	Link				*obj1, *obj2; 


	if (argc < 3) {
		printf("Syntax error!!\n\n");
		return(-1);
	}
  
	/* 
	 * Make sure that the instance specified by users
	 * exists.
	 */
	for (i = 2; (!strcmp(argv[0], "Connect")) && i < argc; i++) {     
		/* check format */
		if (DotSeparate(argv[i], cm1, cm2) < 0) {
			printf("Syntax error!!\n\n");
			return(-1);
		}

		/* check to see if exist such instance */
		obj1 = (Link*)RegTable_.lookup_Instance(atoi(cm1), cm2); 
		if( !obj1 ) {
  	                printf("The %s module instance does not exist.\n\n", cm2);
        	        return(-1);
 	        }

		/* set network type */
		obj1->nettype_ = new char[strlen(argv[1])+1];
		assert(obj1->nettype_); 
		strcpy(obj1->nettype_, argv[1]);
	}

	if(!strcmp(argv[1], "WIRE")) {
		if(argc != 4) {
			printf("need 2 arguments!!\n\n");
			return(-1);
		}
     
		/* network type: wire */ 
		obj1 = this;  
		DotSeparate(argv[3], cm1, cm2); 
		obj2 = (Link *)RegTable_.lookup_Instance(atoi(cm1), cm2);

		obj1->sendtarget_->bind_to(obj2);
		obj2->sendtarget_->bind_to(obj1);

		/* insert obj1 and obj2 into wired-list */
		wd = (struct con_list *)malloc(sizeof(struct con_list));
		assert(wd); 
		wd->obj = obj1;
		SLIST_INSERT_HEAD(&headOfWire_, wd, nextLoc);
		wd = (struct con_list *)malloc(sizeof(struct con_list));
		assert(wd);
		wd->obj = obj2;
		SLIST_INSERT_HEAD(&headOfWire_, wd, nextLoc);

		return(1); 
	}     

	if(!strcmp(argv[1], "WIRELESS")) {
		/* otherwise, network type: wireless */
 		for(i=2; i<argc; i++) {
			DotSeparate(argv[i], cm1, cm2); 
			obj1 = (Link *)
				RegTable_.lookup_Instance(atoi(cm1), cm2);

			/* insert obj1 into wireless-list */
			wi = (struct con_list *)
			     malloc(sizeof(struct con_list));
			assert(wi);
			wi->obj = obj1;
			SLIST_INSERT_HEAD(&headOfWireless_, wi, nextLoc);
		}
		return(1); 
	}
	
        if(!strcmp(argv[1], "GPRSRF")) {
            
                //printf("test\n");
                /* otherwise, network type: wireless */
                for(i=2; i<argc; i++) {
                        DotSeparate(argv[i], cm1, cm2);
                        obj1 = (Link *)
                                RegTable_.lookup_Instance(atoi(cm1), cm2);
                                                                                                                                               
                        /* insert obj1 into wireless-list */
                        wi = (struct con_list *)
                             malloc(sizeof(struct con_list));
                        assert(wi);
                        wi->obj = obj1;
                        SLIST_INSERT_HEAD(&headOfGPRSRF_, wi, nextLoc);
                }
                return(1);
        }

	if(!strcmp(argv[1], "WIMAX")) {
		/* otherwise, network type: wireless */
 		for(i=2; i<argc; i++) {
			DotSeparate(argv[i], cm1, cm2); 
			obj1 = (Link *)
				RegTable_.lookup_Instance(atoi(cm1), cm2);

			/* insert obj1 into wireless-list */
			wi = (struct con_list *)
			     malloc(sizeof(struct con_list));
			assert(wi);
			wi->obj = obj1;
			SLIST_INSERT_HEAD(&headOfWiMAX_, wi, nextLoc);
		}
		return(1); 
	}

	if(!strcmp(argv[1], "WAVE")) {
                /* otherwise, network type: wireless */
                for(i=2; i<argc; i++) {
                        DotSeparate(argv[i], cm1, cm2);
                        obj1 = (Link *)
                                RegTable_.lookup_Instance(atoi(cm1), cm2);

                        /* insert obj1 into wireless-list */
                        wi = (struct con_list *)
                             malloc(sizeof(struct con_list));
                        assert(wi);
                        wi->obj = obj1;
                        SLIST_INSERT_HEAD(&headOfWAVE_, wi, nextLoc);
                }
                return(1);
        }

	if(!strcmp(argv[1], "MobileWIMAX")) {
		/* otherwise, network type: wireless */
 		for(i=2; i<argc; i++) {
			DotSeparate(argv[i], cm1, cm2); 
			obj1 = (Link *)
				RegTable_.lookup_Instance(atoi(cm1), cm2);

			/* insert obj1 into wireless-list */
			wi = (struct con_list *)
			     malloc(sizeof(struct con_list));
			assert(wi);
			wi->obj = obj1;
			SLIST_INSERT_HEAD(&headOfMobileWIMAX_, wi, nextLoc);
		}
		return(1); 
	}

	if(!strcmp(argv[1], "MobileRelayWIMAX")) {
                /* otherwise, network type: wireless */
                for(i=2; i<argc; i++) {
                        DotSeparate(argv[i], cm1, cm2);
                        obj1 = (Link *)
                                RegTable_.lookup_Instance(atoi(cm1), cm2);

                        /* insert obj1 into wireless-list */
                        wi = (struct con_list *)
                             malloc(sizeof(struct con_list));
                        assert(wi);
                        wi->obj = obj1;
                        SLIST_INSERT_HEAD(&headOfMobileRelayWIMAX_, wi, nextLoc);
                }
                return(1);
        }

	if(!strcmp(argv[1], "MR_WIMAX_NT")) {
		/* otherwise, network type: wireless */
		for(i=2; i<argc; i++) {
			DotSeparate(argv[i], cm1, cm2);
			obj1 = (Link *)
				RegTable_.lookup_Instance(atoi(cm1), cm2);

			/* insert obj1 into wireless-list */
			wi = (struct con_list *)
				malloc(sizeof(struct con_list));
			assert(wi);
			wi->obj = obj1;
			SLIST_INSERT_HEAD(&headOfMRWIMAXNT_, wi, nextLoc);
		}
		return(1);
	}

	if (!strncmp(argv[1], "SAT", 3)) {
		if (argc != 4) {
			printf("need 2 arguments!!\n\n");
			return (-1);
		}

		obj1 = this;
		DotSeparate(argv[3], cm1, cm2);
		obj2 = (Link *) RegTable_.lookup_Instance(atoi(cm1), cm2);

		/*
		 * insert obj2 (to) linked list for obj1 (from)
		 */
		insertLinkTohead((headOfLink *)&hos_, obj2);

		/*
		 * insert obj1, obj2 to global linked list 
		 */
		insertLinkTohead(&headOfSat_, obj1);
		insertLinkTohead(&headOfSat_, obj2);
		return (1);
	}

	return(NslObject::command(argc, argv));
}

            
u_int32_t
Link::getConnectNode(u_int32_t nid, u_int32_t portid) {

	struct con_list		*cl;


	/* search in wired-list */
	SLIST_FOREACH(cl, &headOfWire_, nextLoc) {
		if ((cl->obj->get_nid() == nid)&&
		    (cl->obj->get_port() == portid))
		    return((cl->obj->sendtarget_->bindModule())->get_nid());
	}
	return(0);
}

u_int32_t
Link::getConnectNode(u_int32_t nid, struct plist *pl){
	
	struct con_list		*cl;
	
	SLIST_FOREACH(cl, &headOfWire_, nextLoc) {
		char		mark = 0;
		struct plist	*clp = cl->obj->get_portls();
		struct plist	*obp = pl;

		while(clp && obp){
			if(clp->pid != obp->pid){
				mark = 1;
				break;
			}
			else{
				clp = clp->next;
				obp = obp->next;
			}
		}

		if(clp || obp)
			mark = 1;
		
                if ((cl->obj->get_nid() == nid)&& !mark){	
			/* simulate propagation delay here */
			return((cl->obj->sendtarget_->bindModule())->get_nid()); 
		}
	}
	return 0;
}

u_int32_t
Link::getNodeIDbymac(u_char *cmac) {

	struct con_list		*cl;
	u_char			*mac;


	/* search in wired-list */
	SLIST_FOREACH(cl, &headOfWire_, nextLoc) {
		mac = (u_char *)get_regvar(cl->obj->get_nid(),
				cl->obj->get_portls(), "MAC");
		if (mac&&!bcmp(cmac, mac, 6))
			return(cl->obj->get_nid());
	}

	/* search in wireless-list */
	SLIST_FOREACH(cl, &headOfWireless_, nextLoc) {
		mac = (u_char *)get_regvar(cl->obj->get_nid(),
				cl->obj->get_portls(), "MAC");
		if (mac&&!bcmp(cmac, mac, 6))
			return(cl->obj->get_nid());
	}
	/* search in WAVE-list */
        SLIST_FOREACH(cl, &headOfWAVE_, nextLoc) {
                mac = (u_char *)get_regvar(cl->obj->get_nid(),
                                cl->obj->get_portls(), "MAC");
                if (mac&&!bcmp(cmac, mac, 6))
                        return(cl->obj->get_nid());
        }
	return(0);
}



int Link::Debugger() {

	struct con_list		*lt;

	printf("GPRSRF:\n");
        SLIST_FOREACH(lt, &headOfGPRSRF_, nextLoc) {
                printf("%s\n", lt->obj->get_name());
        }
	printf("wireless:\n");
	SLIST_FOREACH(lt, &headOfWireless_, nextLoc) {
		printf("%s\n", lt->obj->get_name());
	}
	printf("\nwire:\n");

	SLIST_FOREACH(lt, &headOfWire_, nextLoc) {
		printf("%s\n", lt->obj->get_name());
	}

	printf("WiMAX:\n");
	SLIST_FOREACH(lt, &headOfWiMAX_, nextLoc) {
		printf("%s\n", lt->obj->get_name());
	}

	printf("MobileWIMAX:\n");
	SLIST_FOREACH(lt, &headOfMobileWIMAX_, nextLoc) {
		printf("%s\n", lt->obj->get_name());
	}

	printf("MobileRelayWIMAX:\n");
	SLIST_FOREACH(lt, &headOfMobileRelayWIMAX_, nextLoc) {
		printf("%s\n", lt->obj->get_name());
	}

	printf("MR_WIMAX_NT:  ");
	SLIST_FOREACH(lt, &headOfMRWIMAXNT_, nextLoc) {
		printf("%s\n", lt->obj->get_name());
	}

	printf("\n");

	return(1);   
}


int Link::insertLinkTohead(struct headOfLink *head, NslObject *obj)
{
	struct con_list *cl;

	/*
	 * search obj whether exist in this linked list
	 */
	SLIST_FOREACH(cl, head, nextLoc) {
		if (cl->obj == obj) {
			return (-1);
		}
	}

	/*
	 * insert obj to linked list 
	 */
	cl  = (struct con_list *)malloc(sizeof (struct con_list));
	assert(cl);
	cl->obj = obj;
	SLIST_INSERT_HEAD(head, cl, nextLoc);

	return (0);
}
