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
#include "Mediator.H"
#include "ZombieHandleThread.H"

void Mediator::disconnect(char* ID, int dispatcher_fd){
	
	unsigned long sessionID;
	sscanf(ID, "%lu", &sessionID);
	string dir = getCurrDir();
	SessionNode node(sessionID, dir);
	sessionList.addSession(sessionID, node);
	currentSession = sessionID;
	timeRelay = false;
	const char* reply = "disconnect_success!\n";
	try{
		ioStream->writen(dispatcher_fd, reply, strlen(reply));
		cout << "[Disconnected!] Session ID: " << sessionID <<
			 "  Status: " << sessionList.getSessionStatus(sessionID) << endl;
	}
	catch (IOStream::IOException){
		cerr << "IOException in Disconnect!\n" << flush;
		return;
	}
	return;
}

void Mediator::reconnect(char* msg, int fd){
//	cout << msg << flush;

	unsigned long sessionID;
	int	data_port;
	int	time_port;
	int	info_port;

	sscanf(msg, "%lu|%d|%d|%d", &sessionID, &data_port, &time_port, &info_port);
	char* cmd_ip = socketManager->getPeerIP(fd);
	int	  cmd_port = socketManager->getPeerPort(fd);
	//cout << cmd_ip << " " << data_port << endl;
	int   data_fd  = socketManager->find_data_fd(cmd_ip, data_port);
	//cout << "data:" << data_fd << endl;

	string c_ip = string(cmd_ip);
	ClientNode new_node(c_ip, cmd_port, fd, data_fd, time_port, info_port);
	//clientList.addNode(new_node);

	// Simulation is running	
//	cout << "p"<<sessionList.getSessionStatus(sessionID) << endl;
	if (sessionList.getSessionStatus(sessionID)){
		clientList.setActiveClient(new_node);
		timeSocket = new DatagramSocket((new_node.client_ip).c_str(), new_node.time_port);
		infoSocket = new DatagramSocket((new_node.client_ip).c_str(), new_node.time_port);
		timeRelay = true;
		//cout << "Session: " << currentSession << endl;
		currentSession = 0;
		infoSocket = new DatagramSocket((new_node.client_ip).c_str(), new_node.info_port);
	}
	
	// Simulation is finished
	else{
	    
	   	int filedes[2];
		pipe(filedes);
	    
		pid_t childpid = fork();
		
		if (childpid < 0){
		    cerr << "Can not fork!\n";
		    return;
		}
		//Child
		else if (childpid == 0){
			isChild = true;
			char c_temp[64];
			char reply[128];
			ioStream->readLine(filedes[0], c_temp, 64);
			cout << "Child says:" << c_temp << endl << flush;
			clientList.setActiveClient(new_node);
			SessionNode node = sessionList.getSessionNode(sessionID);
			currDir = node.file_dir;
			strcpy(reply, "simulationDone\n");
			try{
				ioStream->writen(fd, reply, strlen(reply));
				cout << "Job Finished!" << endl;
				ioStream->readLine(fd, reply, 128);
			}
			catch(IOStream::IOException){
				cerr << "IOException! Reconnect, write to GUI Error!\n" << flush;
				_exit(7);
			}
			getResults();
			//_exit(7);
		}
		//Parent
		else {
	    	ZombieHandleThread z_thread(childpid);
			socketManager->closeByFD(fd);
			socketManager->closeByFD(data_fd);
	//		cout << "Parent" << endl << flush;
			const char *p_temp = "Here goes child\n";
			ioStream->writen(filedes[1], p_temp, strlen(p_temp));
			
		}

	}
	sessionList.removeSession(sessionID);
	
}

	
