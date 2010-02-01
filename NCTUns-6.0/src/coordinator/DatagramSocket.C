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

#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>

using namespace std;

#include "DatagramSocket.H"

DatagramSocket::DatagramSocket(int port){

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(port);
	if ( bind(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr))< 0){
		cout << "Bind Error! " << strerror(errno) << endl << flush;
		exit(1);
	}
}

DatagramSocket::DatagramSocket(string remoteIP, int remotePort){

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(remotePort);
	inet_pton(AF_INET, remoteIP.c_str(), &servaddr.sin_addr);
	
}


int DatagramSocket::sendPacket(void *buff, int length){
	return sendto(sockfd, buff, length, 0, (struct sockaddr*)&servaddr, sizeof(servaddr));
}

int DatagramSocket::receivePacket(void* buff, int length){
	socklen_t len;
	return recvfrom(sockfd, buff, length, 0, &cliaddr, &len);

}

int DatagramSocket::getSocketFD(){
	return sockfd;	
}
	

	
