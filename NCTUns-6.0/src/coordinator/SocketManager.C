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

#include <arpa/inet.h>
#include <netdb.h>
#include "Global.H"

/*--------------------------------------------------------------------------*/

SocketManager::SocketManager(int dispatcher_fd){
	
	memset((char*) &connect_list, 0, sizeof(connect_list));
	highsock = 0;
	
	connect_list[conn_disp_idx] = dispatcher_fd;
	
	setupNetwork();
	for (int i=0; i<4; i++){
		highsock = max (highsock, connect_list[i]);
	}
	highsock++;
}

/*--------------------------------------------------------------------------*/

void 
SocketManager::regist(Mediator* media){
	mediator = media;
}


/*--------------------------------------------------------------------------*/

int 
SocketManager::setupNetwork(){
	
	//Default Ports, domain socket path are defined in Global.H
	command_port = commandPort;
	ServerSocket commandSock(&command_port); 
	connect_list[serv_cmd_idx] = commandSock.getListenFD();
	
	
	data_port = dataPort;
	ServerSocket dataSock(&data_port);
	connect_list[serv_data_idx] = dataSock.getListenFD();
	
	tcsh_port = tcshPort;
	ServerSocket tcshSock(&tcsh_port);
	connect_list[serv_tcsh_idx] = tcshSock.getListenFD();
	
	UnixSocket   unixSocket(domainSocketPath);	
	connect_list[serv_se_idx] = unixSocket.getSockFD(); 
	
	return 0;
	
}

/*--------------------------------------------------------------------------*/

int 
SocketManager::getConnectedFD(int index){
	int fd = connect_list[index];
	if(fd > 0)
		return fd;
	else
		return -1;
}

/*--------------------------------------------------------------------------*/

int  
SocketManager::find_data_fd(char* ip, int data_port){
	for(int i=cli_data_offset; i<max_connection; i++){
		int fd = connect_list[i];
		if( fd != 0){
			char* peer_ip   = getPeerIP(fd);
			int	  peer_port = getPeerPort(fd);
			if((strcmp(peer_ip, ip) ==0) && (peer_port == data_port))
				return fd;
		}
	}
	return -1;
}


/*--------------------------------------------------------------------------*/

int	 
SocketManager::getToDispatcherFD(){
	return connect_list[conn_disp_idx];
}


/*--------------------------------------------------------------------------*/

int  
SocketManager::getUnixSocketFD(){
	int fd = connect_list[conn_se_idx];
	if(fd > 0)
		return fd;
	else
		return -1;
}

/*--------------------------------------------------------------------------*/

string
SocketManager::getCoorInfo(){
	char portsInfo[64];
	sprintf(portsInfo, "%d|%d", command_port, data_port);
	return string(getLocalIP(connect_list[conn_disp_idx]))+"|"+string(portsInfo);
}

/*--------------------------------------------------------------------------*/

void 
SocketManager::Select(){
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

/*--------------------------------------------------------------------------*/

void SocketManager::buildSelectList(){
	
	int listnum;
			
	FD_ZERO(&socks);	
	for (listnum = 0; listnum < max_connection; listnum++) {
		if (connect_list[listnum] != 0) {
			//cout << "(Active:" << listnum << "| fd:" << connectlist[listnum] << ")\t" << flush;
			FD_SET(connect_list[listnum],&socks);
			if (connect_list[listnum] > highsock)
				highsock = connect_list[listnum];
		}//if
	}//for
	
}

/*--------------------------------------------------------------------------*/	
	
void SocketManager::readSocks(){
	
	int toDispatcher  = connect_list[conn_disp_idx];
	int clientCommand = connect_list[serv_cmd_idx];
	int clientData	  = connect_list[serv_data_idx];
	int servSE		  = connect_list[serv_se_idx];
	int servTCSH	  = connect_list[serv_tcsh_idx];
	int toSE		  = connect_list[conn_se_idx];
	int toTCSH		  = connect_list[conn_tcsh_idx];

	if (FD_ISSET(toDispatcher,&socks))
		mediator->handleDispatcher(conn_disp_idx);

	else if (FD_ISSET(clientCommand,&socks)){

		handleNewCommand(clientCommand);
	}// else if
	
	else if (FD_ISSET(clientData,&socks)){
		handleNewData(clientData);		
	}// else if
	
	else if (FD_ISSET(servSE, &socks)){
			cout << "[New Unix Domain Socket]" << endl << flush;
			connect_list[conn_se_idx]  = accept(servSE, NULL, NULL);
	}
	else if (FD_ISSET(toSE, &socks)){
		mediator->handleUnix(conn_se_idx);
	}
	
	else if (FD_ISSET(servTCSH, &socks)){
		cout << "[TCSH CONNECTED!]" << endl;
		connect_list[conn_tcsh_idx] = accept(servTCSH, NULL, NULL);
	}
	
	else if (FD_ISSET(toTCSH, &socks)){
		mediator->handleTCSH(conn_tcsh_idx);
	}

	else{ 
		for (int i=cli_cmd_offset; i < max_connection; i++) {
			if (FD_ISSET(connect_list[i],&socks)){
				if(i < cli_data_offset){
					mediator->handleCommand(i);
				}
				else{	
					mediator->handleData(i);
				}
			}//if			
		}//for
	}//else
		
}	


/*--------------------------------------------------------------------------*/

void 
SocketManager::handleNewCommand(int lis_command){
	
	cout << "--->New Command\t\t";
	
	int connection = accept(lis_command, NULL, NULL);
	
	if (connection < 0) {
		perror("accept command");
		exit(EXIT_FAILURE);
	}
	
	for (int i  = cli_cmd_offset; (i < cli_data_offset) && (connection != -1); i++)
		if (connect_list[i] == 0) {
			cout << "Command accepted: " << connection  << "\t" << i << endl << flush;
			connect_list[i] = connection;
			connection = -1;
		}
	if (connection != -1) {
		/* No room left in the queue! */
		cout << "\nNo room left for new command connection.\n" << flush;
		close(connection);
	}


}


/*--------------------------------------------------------------------------*/

void 
SocketManager::handleNewData(int lis_data){

	cout << "--->New Data\t";
		
	int connection = accept(lis_data, NULL, NULL);
	
	if (connection < 0) {
		perror("accept data");
		exit(EXIT_FAILURE);
	}
	
	for (int i  = cli_data_offset; (i < max_connection) && (connection != -1); i++)
		if (connect_list[i] == 0) {
			cout << "Data accepted: " << connection  << "\t" << i << endl << flush;
			connect_list[i] = connection;
			connection = -1;
		}
	if (connection != -1) {
		/* No room left in the queue! */
		cout << "\nNo room left for new client.\n" << flush;
		close(connection);
	}

}

/*--------------------------------------------------------------------------*/

char* 
SocketManager::getLocalIP(int fd){
	struct sockaddr_in sockaddr;
	socklen_t len = sizeof(sockaddr);

	if(getsockname(fd, (struct sockaddr *)&sockaddr, &len) < 0)
	    perror("getSockName");
	
	return inet_ntoa(sockaddr.sin_addr);
}

/*--------------------------------------------------------------------------*/

char* 
SocketManager::getPeerIP(int fd){
	struct sockaddr_in  peeraddr;
	socklen_t len = sizeof(peeraddr);
	
	if (getpeername(fd, (struct sockaddr *)&peeraddr, &len) < 0 )
		perror("getPeerName");
	
	return inet_ntoa(peeraddr.sin_addr);	
	
}

/*--------------------------------------------------------------------------*/

int 
SocketManager::getLocalPort(int fd){
	struct sockaddr_in  localaddr;
	socklen_t len = sizeof(localaddr);
	
	if (getsockname(fd, (struct sockaddr *)&localaddr, &len) < 0 )
		perror("getSockName");
	
	return ntohs(localaddr.sin_port);	
}


/*--------------------------------------------------------------------------*/

int 
SocketManager::getPeerPort(int fd){
	struct sockaddr_in  peeraddr;
	socklen_t len = sizeof(peeraddr);
	
	if (getpeername(fd, (struct sockaddr *)&peeraddr, &len) < 0 )
		perror("getSockName");
	
	return ntohs(peeraddr.sin_port);	
}

/*--------------------------------------------------------------------------*/

void 
SocketManager::closeByIndex(int index){

	cout << "Connection closed\t" << connect_list[index] << "\t" << index << endl << flush;
	close(connect_list[index]);
	connect_list[index] = 0;	
}

/*--------------------------------------------------------------------------*/

void 
SocketManager::closeByFD(int fd){
	int index = -1;
	for (int i=0; i<max_connection; i++){
		if (connect_list[i] == fd){
			index = i;
			break;
		}
	}
	if (index >=0)
		closeByIndex(index);	
}


/*--------------------------------------------------------------------------*/

string 
SocketManager::toString(){
/*
	cout << connectlist[0] << " | " << connect_list[1] << " | ";
	cout << "high:" << highsock << endl << flush;
*/
}

/*--------------------------------------------------------------------------*/
