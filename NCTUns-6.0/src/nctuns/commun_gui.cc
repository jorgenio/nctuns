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

#include <agent.h>
#include <command_server.h>
#include <commun_gui.h>
#include <config.h>
#include <module/nctuns-dep/node.h>
#include <gbind.h>

commun_gui *commun_gui_  = new commun_gui();
extern struct PtrPacket *ptrpacket_;
extern struct LocPacket *locpacket_;
extern int PtrArray_last;
extern int LocArray_last;
extern struct Light *light_;
extern int LightArray_last;
extern cmd_server * cmd_server_;
extern logpack *logpack_;

int sendPtrAndLocToGUI(Event_ *ep) {

	if (PtrArray_last > -1)
		commun_gui_->sendPtrToGUI(PtrArray_last+1);
	if (LocArray_last > -1)
		commun_gui_->sendLocToGUI(LocArray_last+1);
	freeEvent(ep);
	scheduleNextSendToGUI();
	return(1);
}

void scheduleNextSendToGUI() {
	u_int64_t       timeTick ;
	Event_ *ep;

	ep = createEvent();
	MILLI_TO_TICK(timeTick, SEND_PTR_LOC_TO_GUI_INTERVAL);
	timeTick = GetCurrentTime() + timeTick;
	setFuncEvent(ep, timeTick , 0, sendPtrAndLocToGUI, NULL);
}


int logNodeLocation(Event_ *ep) {
	int nid;
	double x,y,z;
	double speed;

	GROUPMEMBER_VECTOR* gv = GROUP_FIND(GROUP_ALL)->GetMember();
	for (int j = 0; j < static_cast<int>(gv->size()); j++) {
		nid = ((*gv)[j])->GetID();

		// No need to log traffic light's location
		int nodeType;
		((Node*) nodelist[nid])->getNodeType(nodeType);
		if(nodeType == TRAFFIC_LIGHT_CONTROLLER)
			continue;

		GetNodeLoc(nid, x, y, z);
		GetNodeSpeed(nid, speed);
		logpack_->logNodeLocation(nid, x, y, z, GetCurrentTime(), 0, speed);
	}	
	freeEvent(ep);
	scheduleLogNodeLocation();
	return(1);
}

void scheduleLogNodeLocation(){
	u_int64_t       timeTick;
	Event_ *ep;

	ep = createEvent();
	MILLI_TO_TICK(timeTick, LOG_NODE_LOCATION_INTERVAL);
	timeTick = GetCurrentTime() + timeTick;
	setFuncEvent(ep, timeTick , 0, logNodeLocation , NULL);
}

commun_gui::commun_gui() {

	FD_ZERO(&afds);	
	sendto_gui_sockfd = connectUDP(GUI_ADDRESS, GUI_UDP_PORT);
	nfds = sendto_gui_sockfd+1;	
	FD_SET(sendto_gui_sockfd, &afds);
	shouldPoll_IPC_CommandsFromGUI = 0;
}

int commun_gui::polling() {
	struct  timeval T;
	int	someDataArrived;
	int	ret;

	if (shouldPoll_IPC_CommandsFromGUI == 0) return(1);

	do {
		T.tv_sec = 0;
		T.tv_usec = 1;
		someDataArrived = 0;
		rfds = afds;

		if ((ret = select(nfds, &rfds, (fd_set *) 0, (fd_set *) 0,
						(struct timeval *)&T) < 0)) {
			/* Something is wrong, however, we just skip it. */
			perror("select from GUI");
		} else if (ret == 0) {
			break;
		} else {
			if (FD_ISSET(sendto_gui_sockfd, &rfds)) {
				someDataArrived = 1;
				communicateWithGUI(sendto_gui_sockfd);
			}
		}
	} while (someDataArrived == 1);
	return (1);
}


int commun_gui::communicateWithGUI(int fd) {
	char                    *msg;
	int                     recv_bytes;
	struct typeChecker      *p;

	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	recv_bytes = read(fd, (void *) msg, MAX_RECV_BYTES);

	if (recv_bytes < 0) {
		printf("[commun_gui::communicateWithGUI()]: recv_bytes < 0\n");
		exit(1);
	} else if (recv_bytes == 0) {
		printf("[commun_gui::communicateWithGUI()]: recv_bytes == 0\n");
		/* This situation should not occur, however, we just skip it. */
	}
	p = (struct typeChecker *) msg;
	if (p->type == GUI_CHANGE_NODE_SPEED) {
		printf("commun_gui::receive GUI_CHANGE_NODE_SPEED \n");
		receiveFromGUIchangeNodeSpeed(fd, msg);
	} else if (p->type == GUI_CHANGE_NODE_MOVING_DIRECTION) {
		printf("commun_gui::receive GUI_CHANGE_NODE_MOVING_DIRECTION\n");
		receiveFromGUIchangeNodeMovingDirection(fd, msg);
	} else {
		printf("[commun_gui::communicateWithGUI()]: unsupported type %d\n", p->type);
	}
	free(msg);
	return(1);
}

int commun_gui::initialize() {

#if IPC
	if ( (dynamicLocLogFlag && !strcasecmp( dynamicLocLogFlag, "on")) ||
			(dynamicPtrLogFlag && !strcasecmp( dynamicPtrLogFlag, "on")) ){

		scheduleLogNodeLocation();
		scheduleNextSendToGUI();
		/*
		 * disabled by cclin since we turn off the human interaction
		 * functionality in NCTUns4.
		 * shouldPoll_IPC_CommandsFromGUI = 1;
		 */
		shouldPoll_IPC_CommandsFromGUI = 0;

	} else shouldPoll_IPC_CommandsFromGUI = 0;
#else
	/*
	 * Log the node's location when manually start the simulation.
	 */
	scheduleLogNodeLocation();
#endif
	return(1);
}

void commun_gui::receiveFromGUIchangeNodeSpeed(int fd, char *msg) {
	struct GUIChangeNodeSpeed *p;
	int nid;
	double speed;	

	p = (struct GUIChangeNodeSpeed *) msg;
	nid = p->nid;
	speed = p->speed;

	printf("Received a command from GUI: change node(%d)'s speed to %lf \n",
			p->nid, p->speed);
	/* Set the specified node as the target node, whose movement is
	   manipulated by a human.
	   */
	setAgentHumanOrProgramControlled(nid, 1);
	cmd_server_->changeNodeSpeedFromGUI(nid, speed);
}

void commun_gui::receiveFromGUIchangeNodeMovingDirection(int fd, char *msg) {
	struct GUIChangeNodeMovingDirection *p;
	int nid;
	double angle;

	p = (struct GUIChangeNodeMovingDirection *) msg;
	nid = p->nid;
	angle = p->angle;
	printf("Received a command from GUI: change node(%d)'s moving direction to %lf \n" ,p->nid, p->angle);

	/* Set the specified node as the target node, whose movement is
	   manipulated by a human.
	   */
	setAgentHumanOrProgramControlled(nid, 1);
	cmd_server_->changeNodeMovingDirectionFromGUI(nid, angle);
}

int commun_gui::sendLocToGUI(int num) {
	int length;
	int send_bytes;

	if ((LocArray_last+1) < num ){
		num = LocArray_last;
	}
	length = sizeof(struct NodeLocationHeader) + 
		sizeof(struct NodeLocation) * num;
	locpacket_->type = SE_SEND_LOC_TO_GUI;
	send_bytes = write(sendto_gui_sockfd ,(void *)locpacket_, length);
	if(send_bytes < 0) {
		printf("ERROR(%d): commun_gui::sendLocToGUI()\n", errno);
		exit(1);
	}
	LocArray_last -= num;	
	locpacket_->num -= num;
	return(num);
}

int commun_gui::sendPtrToGUI(int num) {
	int length;
	int send_bytes;

	if ((PtrArray_last+1) < num ){
		num = PtrArray_last;
	}
	length = sizeof(struct PTR_Header) + num * sizeof(struct LogObject); 
	ptrpacket_->type = SE_SEND_PTR_TO_GUI;
	send_bytes = write(sendto_gui_sockfd, (void *) ptrpacket_, length);
	if(send_bytes < 0) {
		printf("ERROR(%d): commun_gui::sendPtrToGUI\n", errno);
		exit(1);
	}
	PtrArray_last -= num;
	ptrpacket_->num-= num;
	return(num);
}

int commun_gui::sendLightToGUI(int num) {
	int length;
	int send_bytes;

	if ((LightArray_last+1) < num ){
		num = LightArray_last;
	}
	length = sizeof(struct LightInfoHeader) +
		sizeof(struct LightInfo) * num;
	light_->type = SE_SEND_LIGHT_TO_GUI;
	send_bytes = write(sendto_gui_sockfd ,(void *)light_, length);
	if(send_bytes < 0) {
		printf("ERROR(%d): commun_gui::sendLightToGUI()\n", errno);
		exit(1);
	}
	LightArray_last -= num;
	light_->num -= num;
	return(num);
}

