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

#include "Global.H"
#include "SocketManager.H"
#include "Mediator.H"

#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>


SocketManager::SocketManager(vector<int> &listenfds){
	
	memset((char*) &connectlist, 0, sizeof(connectlist));
	highsock = 0;

	for (int i=0; i<listenfds.size(); i++){
		connectlist[i] = listenfds[i];
		if (listenfds[i] > highsock)
			highsock = listenfds[i];
	}
	listenServer = connectlist[0];
	listenClient = connectlist[1];
}


void SocketManager::regist(Mediator* media){
	mediator = media;
};

void SocketManager::Select(){

	int readsocks = select(highsock+1, &socks, NULL, NULL, NULL);		

	if (readsocks < 0) {
		perror("select");
		exit(EXIT_FAILURE);
	}
	if (readsocks == 0) {
		cout << "." << flush;
	} 
	else {
		readSocks();
	}
}


void SocketManager::buildSelectList(){
	
	int listnum;
			
	FD_ZERO(&socks);	
	FD_SET(listenClient,&socks);
	FD_SET(listenServer,&socks);
		
	for (listnum = 0; listnum < MAX_CONNECTION; listnum++) {
		if (connectlist[listnum] != 0) {
			cout << "(Active:" << listnum <<"| fd:" << connectlist[listnum] << ")   " << flush;
			FD_SET(connectlist[listnum],&socks);
			if (connectlist[listnum] > highsock)
				highsock = connectlist[listnum];
		}//if
	}//for
	
	cout << endl << flush;
}

void SocketManager::readSocks(){

	//cout << "READSOCKS..." << endl << flush;
	
	if (FD_ISSET(listenClient,&socks))
		handleNewClient();
	else if (FD_ISSET(listenServer,&socks))
		handleNewServer();
	else{ 
		for (int i=0; i < MAX_CONNECTION; i++) {
			if (FD_ISSET(connectlist[i],&socks)){
				if(i < CLIENT_OFFSET){
					mediator->handleServer(i);
				}
				else{	
					mediator->handleClient(i);
				}
			}//if			
		}//for
	}//else
		
	//@@@@@sleep	
	//sleep (2);
}	

void SocketManager::handleNewServer(){
	
	cout << "--->New Server" << endl << flush;
	
	int connection = accept(listenServer, NULL, NULL);
	
	if (connection < 0) {
		perror("accept server");
		exit(EXIT_FAILURE);
	}
	
	for (int i  = 0; (i < CLIENT_OFFSET) && (connection != -1); i++)
		if (connectlist[i] == 0) {
			cout << "Server accepted: " << connection  << "\t" << i << endl << flush;
			connectlist[i] = connection;
			connection = -1;
		}
	if (connection != -1) {
		/* No room left in the queue! */
		cout << "\nNo room left for new simulation server.\n" << flush;
		close(connection);
	}


}

void SocketManager::handleNewClient(){

	cout << "--->New Client" << endl << flush;
	
	int connection = accept(listenClient, NULL, NULL);
	
	if (connection < 0) {
		perror("accept client");
		exit(EXIT_FAILURE);
	}
	
	for (int i  = CLIENT_OFFSET; (i < MAX_CONNECTION) && (connection != -1); i++)
		if (connectlist[i] == 0) {
			cout << "Client accepted: " << connection  << "\t" << i << endl << flush;
			connectlist[i] = connection;
			connection = -1;
		}
	if (connection != -1) {
		/* No room left in the queue! */
		cout << "\nNo room left for new client.\n" << flush;
		close(connection);
	}

}

int SocketManager::getConnectedFD(int index){
	return connectlist[index];	
}


char* SocketManager::getLocalIP(int fd){
	struct sockaddr_in  localaddr;
	socklen_t len = sizeof(localaddr);
	
	if (getsockname(fd, (struct sockaddr *)&localaddr, &len) < 0 )
		perror("getSockName");
	
	return inet_ntoa(localaddr.sin_addr);	
}


char* SocketManager::getPeerIP(int fd){
    	char *peer_IP;
	struct sockaddr_in  peeraddr;
	socklen_t len = sizeof(peeraddr);
	
	if (getpeername(fd, (struct sockaddr *)&peeraddr, &len) < 0 )
		perror("getPeerName");
	peer_IP = inet_ntoa(peeraddr.sin_addr);
	if(strcmp(peer_IP , "127.0.0.1") == 0)
	{
	    	struct hostent *peer_info;
		peer_info = gethostbyname("localhost");
		peer_IP = inet_ntoa(*((struct in_addr*)peer_info->h_addr));
	}    
	return peer_IP;	
	
}

int  SocketManager::getPeerPort(int fd){
	struct sockaddr_in  peeraddr;
	socklen_t len = sizeof(peeraddr);
	
	if (getpeername(fd, (struct sockaddr *)&peeraddr, &len) < 0 )
		perror("getPeerPort");
	
	return ntohs(peeraddr.sin_port);
	
}


void SocketManager::closeConnection(int index){

	cout << "Connection closed\t" << connectlist[index] << "\t" << index << endl << flush;
	close(connectlist[index]);
	connectlist[index] = 0;	
}


void SocketManager::toString(){

	cout << connectlist[0] << " | " << connectlist[1] << " | ";
	cout << "high:" << highsock << endl << flush;
}
