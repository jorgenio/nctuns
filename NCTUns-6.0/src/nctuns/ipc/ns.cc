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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <string.h>
#include <fcntl.h>

#include <IPC/ns.h>
#include <IPC/IOStream.H>
#include <dispatcher.h>

using namespace std;

extern Dispatcher *dispatcher_;

int unixSocket = -1;
IOStream* ioStream;

int InitSock(void){

    struct sockaddr_un servaddr;
    if((unixSocket = socket(AF_UNIX, SOCK_STREAM, 0))<0) {
    	perror("Can't create socket");
		return 0;
    }
	
    servaddr.sun_family = AF_UNIX;
    strcpy(servaddr.sun_path, DomainSockPath);
	
    if(connect(unixSocket, (struct sockaddr *)&servaddr, SUN_LEN(&servaddr))<0) {
		perror("Connect to Coordinator Failed...\n");
		exit(1);
    }
	
	ioStream = new IOStream();
	
	cout << "Successfully Connected to Coordinator!! Socket fd:" << unixSocket << endl << flush;
	return 1;
}

int StartTrafficGenerator(char *trafficgen){ //123 rtp -p 6000

	char command[512];
	sprintf(command, "startTafficeGenerator|%s", trafficgen);
	ioStream->writen(unixSocket, command, strlen(command));
	return 1;
}

int StopTrafficGenerator(char *trafficgen){
	
	char command[512];
	sprintf(command, "stopTraffic|%s", trafficgen);
	ioStream->writen(unixSocket, command, strlen(command));
	return 1;
}


int SimulationDown(void){
	cout << "DONE!" << endl;
	char command[512];
	strcpy(command, "simulationDone\n");
	ioStream->writen(unixSocket, command, strlen(command));
	//sendtoGUI(command,sizeof(command));
	close(unixSocket);
	return 1;
}


int sendtoGUI(const char *data, int size){
	char command[512];
	sprintf(command, "sendToGUI|%d\n", size);
	ioStream->writen(unixSocket, command, strlen(command));
	ioStream->writen(unixSocket, data, size);

	return 1;
}


int set_nonblock_flag (int desc, int value);

int	recvfromGUI(char *&data){
	char msg[2050];
	set_nonblock_flag(unixSocket, 1);
	
	if(ioStream->readLine(unixSocket, msg, 2048)>0){
		data = (char*)malloc(strlen(msg) + 1);

		strncpy(data, msg, strlen(msg)-1);	//Get rid off '\n'
		data[strlen(msg)-1] = '\0';		//Null terminated
		set_nonblock_flag(unixSocket, 0);
		cout << "ipc_in_SE: " << data << endl;
		return 1;
	}
	else{
		set_nonblock_flag(unixSocket, 0);
		return -1;
	}
	
}


void sendTime(u_int64_t currenttime) {
	char command[512];
	sprintf(command, "sendTime|%llu\n", currenttime);
	ioStream->writen(unixSocket, command, strlen(command));
	
}

void closeSock(void){
	
	if(unixSocket >0)
		close(unixSocket);
}

/* Don't know what is it, Unchanged, oli*/
int errexit(const char *format, ...){
	
	cout << "Error Exit Called!!!" << endl << flush;
	/*
    va_list args;

    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    */
    exit(1);
}


int set_nonblock_flag (int desc, int value)
{
  int oldflags = fcntl (desc, F_GETFL, 0);
  /* If reading the flags failed, return error indication now. */
  if (oldflags == -1)
    return -1;
  /* Set just the flag we want to set. */
  if (value != 0)
    oldflags |= O_NONBLOCK;
  else
    oldflags &= ~O_NONBLOCK;
  /* Store modified flag word in the descriptor. */
  return fcntl (desc, F_SETFL, oldflags);
}


int IOStream::readLine(int fd, void *buffer, int maxLen){
	
	int		n, rc;
	char	c, *ptr;
	
	ptr = (char*)buffer;
	
	for (n=1; n<maxLen; n++){
		again:
			if ((rc = read(fd, &c, 1)) == 1 ){
				*ptr++ = c; 
				if (c == '\n'){
					break;
				}
			}//if
			else if (rc == 0){
				if (n == 1) return 0;
				else 		break;				
			}// else if
			else{
				if (errno == EINTR)
					goto again;
				return -1;
			}//else
	}//for
	*ptr = 0;
	return n;
}


int IOStream::writen(int fd, const void *vptr, int n){

	int nleft;
	int nwritten;
	const char *ptr;
	
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0){
		if ((nwritten = write(fd, ptr, nleft))<=0){
			if(errno == EINTR)
				nwritten = 0;
			else
				return -1;			
		}//if
		nleft -= nwritten;
		ptr	+= nwritten;
	}//while
	
	return n;	
}

int sendWarningtoGUI(struct runtimeMsgInfo data)
{
	char command[512];

	if(data.type == RTMSG_WARNING)
		dispatcher_->Pause(NULL);
	else if(data.type == RTMSG_FATAL_ERROR)
		dispatcher_->Stop(NULL);

	sprintf(command, "sendWarningtoGUI|%d\n", sizeof(data));
	ioStream->writen(unixSocket, command, strlen(command));
	ioStream->writen(unixSocket, (char*)&data, sizeof(data));

	return 1;
}
