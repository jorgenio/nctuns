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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#include "ServerSocket.H"


#include <iostream>
using namespace std;


ServerSocket::ServerSocket(int bindPort){
		
	qlen = 1024;	
	port = bindPort;	

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	int reuse_addr = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
		
	memset (&servaddr, 0, sizeof(servaddr));		
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port); 
	if ( bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))< 0){
		cout << "Bind Error! " << strerror(errno) << endl << flush;
		exit(1);
	}
	
	listen(listenfd, qlen);
	
	cout << "ServerSocket listen to port:" << port << endl << flush;
	
}


int ServerSocket::Accept(){

	len = sizeof(cliaddr);
	int connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &len);
	return  connfd;
}


void ServerSocket::Close(){
	if (listenfd >0)
		close(listenfd);
	return;
	
}

int ServerSocket::getListenFD(){
	return listenfd;	
}

