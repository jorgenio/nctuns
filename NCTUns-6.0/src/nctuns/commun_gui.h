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

#ifndef __NCTUNS_COMMUN_GUI_H__
#define __NCTUNS_COMMUN_GUI_H__

#include <unistd.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <netdb.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <nctuns_api.h>
#include <command_format.h>
#include <misc/log/logpack.h>
#include <sock_skel.h>

#define BUFSIZE                 2048
#define NLISTEN                 MAX_NODES      /* max number of connections */
#define isvalidsock(s)  ( ( s ) >= 0 )

//#define SEND_PTR_LOC_TO_GUI_INTERVAL	100	// ms	
#define SEND_PTR_LOC_TO_GUI_INTERVAL	300	// ms	
//#define LOG_NODE_LOCATION_INTERVAL	300	// ms
#define LOG_NODE_LOCATION_INTERVAL	600	// ms

int  sendPtrAndLocToGUI(Event_ *ep);
void scheduleNextSendToGUI();
int  logNodeLocation(Event_ *ep);
void scheduleLogNodeLocation();

class commun_gui {
private:
	int sendto_gui_sockfd; // the socket used to communicate with GUI
        int nfds;       // number of descriptors to check in select()
	fd_set rfds;    // socket descriptors checked in select()
        fd_set afds;    // active socket descriptors copied into select()
	int shouldPoll_IPC_CommandsFromGUI;
public:
	commun_gui();
	~commun_gui();

	int polling();
	int communicateWithGUI(int fd);
	int initialize();
	void receiveFromGUIchangeNodeSpeed(int fd, char *msg);
	void receiveFromGUIchangeNodeMovingDirection(int fd, char *msg);
	int sendLocToGUI(int);
	int sendPtrToGUI(int);
	// add by AD
	int sendLightToGUI(int);
}; 
#endif
