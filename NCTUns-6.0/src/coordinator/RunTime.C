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

#include <sys/wait.h>

#include "Global.H"
#include "Mediator.H"


void
Mediator::sendtoSE(const char* header, const char* message){
	
	int se_fd = socketManager->getUnixSocketFD();
	char command[128];
	sprintf(command,"%s%s", header, message);
	cout << "to SE " << command << "|" << endl;
	try{
		ioStream->writen(se_fd, command, strlen(command));
	}
	catch(IOStream::IOException){
		cerr << "IOException! send to SE Error!\n" << flush;
		return;
	}
	return;
}

void
Mediator::readlnSE(string& message){
	
	int se_fd = socketManager->getUnixSocketFD();
	string opcode = "NULL";
	char msg[1024];
  try{
	while (opcode != "sendToGUI"){
		bzero(msg, 1024);
		if(se_fd == -1)
			return;
		ioStream->readLine(se_fd, msg, 1024);
		opcode = string(strtok(msg, "|"));
	}
	int size = atoi(strtok(NULL, "|\n"));
	char data[size];
	ioStream->readn(se_fd, data, size);

	message = string(data);
  }
  catch(IOStream::IOException){
  	cerr << "Read from SE Error!\n" << flush;
	return;
  }
	return;
}

void
Mediator::sendtoGUI(char* message){
	int se_fd = socketManager->getUnixSocketFD();
	ClientNode* node = clientList.getActiveClient();
	int cmd_fd = node->command_fd;
	int size = atoi(strtok(message, "|\n"));
	char command[128];
	char data[size];
	sprintf(command, "sendtoGUI|%d\n", size);
	try{
		ioStream->readn(se_fd, data, size);
		ioStream->writen(cmd_fd, command, strlen(command));
		ioStream->writen(cmd_fd, data, size);	
	}
	catch(IOStream::IOException){
		cerr << "IOException! send to GUI Error!\n" << flush;
		return;
	}
	return;
}

void
Mediator::sendWarningtoGUI(char* message) {
	int se_fd = socketManager->getUnixSocketFD();
	int size = atoi(strtok(message, "|\n"));
	char data[size];

	try {
		ioStream->readn(se_fd, data, size);
	}
	catch(IOStream::IOException) {
		cerr << "IOException! send to GUI Error!\n" << flush;
		return;
	}

	//infoSocket->sendPacket(data, size);

	struct runtimeMsgInfo *info = (struct runtimeMsgInfo *)data;

	if(info->type == RTMSG_WARNING)
	{
		isPause = true;
		kill(engine_pid, SIGSTOP);
	}
	else if(info->type == RTMSG_FATAL_ERROR)
	{
		isStop = true;
		kill(engine_pid, SIGKILL);
		waitpid(engine_pid, NULL, 0);
		engine_pid = -1;

		if (currentSession == 0){
			int command_fd = clientList.getActiveClient()->command_fd;
			char command[128];
			strcpy(command, "simulationDone\n");
			try{
				ioStream->writen(command_fd, command, strlen(command));
			}
			catch (IOStream::IOException){
				cerr << "IOException! in stop simulation, write to GUI Error!\n" << flush;
				return;
			}
		}

	}

	infoSocket->sendPacket(data, size);
	return;
}
