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

#include <time.h>
#include <pwd.h>

#include "Global.H"
#include "IOStream.H"
#include "RemoteHandler.H"
#include "ZombieHandleThread.H"
#include <nctuns_syscall.h>
#include <nctuns_divert.h>

	
Mediator::Mediator(SocketManager* manager){
 
	socketManager = manager;
	socketManager->regist(this);
	
	registerToDispatcher();
	
	timeRelay = true;	 //Relay time to client 
	currentSession = 0; //Not a session
	isChild	  = false;
	isPause = false;
	isStop = false;
}


void Mediator::registerToDispatcher(){
	int dispatcher_fd = socketManager->getToDispatcherFD();
	    
	char regMessage[BUFFER_SIZE];
	strcpy(regMessage, "register|");
	strcat(regMessage, (socketManager->getCoorInfo()).c_str());
	strcat(regMessage, "|IDLE\n");
    cout << "[To Dispatcher...]\t" << regMessage << flush;
	char messageIN[BUFFER_SIZE];
	try{
		ioStream->writen(dispatcher_fd, (void*)regMessage, strlen(regMessage));	
		ioStream->readLine(dispatcher_fd, messageIN, BUFFER_SIZE);
	}
	catch(IOStream::IOException){
		cerr << "IOException! in Register to Dispatcher...\n" << flush;
		return;
	}
	cout << "[From Dispatcher...]\t" << messageIN  << flush;
	if (strcmp (messageIN, "OK\n") != 0)
		cout << "Regist Error" << endl << flush;
		
	return;
}

void Mediator::handleDispatcher(int index){
	
	
	int fd = socketManager->getConnectedFD(index);
	char messageIN[BUFFER_SIZE];
	int rd  = ioStream->readLine(fd, messageIN, BUFFER_SIZE);
	if (rd > 0){
		cout << "[From Dispatcher...]\t" << messageIN  << flush;
		vector<char*> tokens ;
		char* token = strtok(messageIN, "|");
		while (token != NULL){
			tokens.push_back(token);
			token = strtok(NULL, "|\n");	
		}
		if (strcmp(tokens[0],"setupEnv") == 0)
			setupEnv(tokens, fd);
		else if (strcmp(tokens[0], "disconnect")==0){
			disconnect(tokens[1], fd);
		}
		else if (strcmp(tokens[0], "BG_putFile")==0){
			BG_put_file(tokens[1], atol(tokens[2]), fd);
		}
		else if (strcmp(tokens[0], "BG_start") == 0){
			BG_start(tokens[1], atol(tokens[2]));
		}
		else if (strcmp(tokens[0], "JOB_STOP") ==0 ){
       	//	processList.stopAll();
 		//	becomeFree(); 
			stopSimulation("dispatcher");
			becomeFree();
			simulationFinished();
		}
		else if (strcmp(tokens[0], "JOB_ABORT") ==0 ){
			abortSimulation();
		}
		else if (strcmp(tokens[0], "getTick\n") == 0){
			getTick(fd);
		}
		else
			cout << "else?" << endl << flush;
	}
	else{
		socketManager->closeByIndex(index);
		cout << "Dispatcher Closed" << endl << flush;
		
	}
}

void Mediator::setupEnv(vector<char*> tokens, int fd){
	timeRelay = true;	 //Relay time to client 
	currentSession = 0; //Not a session
	time_t rawtime;
	time(&rawtime);
	char username[64];
	strcpy (username, tokens[1]);
	struct passwd* pw = getpwnam(username);
	string user_home = string(pw->pw_dir);
	cout << user_home << endl << flush;


	char fullDir[512];
	sprintf (fullDir, "%s/.nctuns/coordinator/workdir/%s-%lu-job/", user_home.c_str(), username, rawtime);

	currDir = fullDir;
	cout << "[Working Directory] "<< currDir <<endl << flush;
	string mkdir_cmd = "mkdir -p " + string(fullDir);
	string chmod_cmd = "chmod 777 " + string(fullDir);
	if (system(mkdir_cmd.c_str()) <0 )
		cout << "Simultion Environment Setup Error!\n" << flush;
	else{
	        system(chmod_cmd.c_str());

		const char* reply = "EnvOK\n";
		try{
			ioStream->writen(fd, reply, strlen(reply)); 
		}
		catch(IOStream::IOException){
			cerr << "IOExcetion! in Setup Env, write to dispatcher Error!\n" << flush;
			return;
		}
	}	
	return;
}

void Mediator::handleCommand(int index){
	
	int fd = socketManager->getConnectedFD(index);
	char messageIN[BUFFER_SIZE];
	int rd = ioStream->readLine(fd, messageIN, BUFFER_SIZE);
	if (rd > 0){
		cout << "[From Command...]\t" << messageIN  << flush;
		char* command = strtok(messageIN, "|");
		if (strcmp(command,"init") ==0){
			char* ip   = strtok(NULL, "|\n");
			int	  cmd_port  = atoi(strtok(NULL, "|\n"));
			int   data_port = atoi(strtok(NULL, "|\n"));
			int   time_port = atoi(strtok(NULL, "|\n"));
			int   info_port = atoi(strtok(NULL, "|\n"));
			initClient(ip, fd, cmd_port, data_port, time_port, info_port);
		}
		else if (strcmp(command,"putfile") ==0){
			long fileSize = 0;
			char* filename = strtok(NULL, "|");
			sscanf(strtok(NULL, "|\n"), "%d", &fileSize);
			int data_fd = clientList.getActiveClient()->data_fd;
			putfile(filename, fileSize, fd, data_fd);
		}
		else if (strcmp(command,"startSimulation") == 0){
			char* command = strtok(NULL, "|\n");
			int timePort;
			sscanf(strtok(NULL, "|\n"), "%d", &timePort);
			startSimulation(command, false);
		}
		else if (strcmp(command,"pauseSimulation") == 0){
			pauseSimulation(strtok(NULL, "|\n"));
		}
		else if (strcmp(command,"stopSimulation") == 0){
			stopSimulation(strtok(NULL, "|\n"));
		}
		else if (strcmp(command,"resumeSimulation") == 0){
			resumeSimulation(strtok(NULL, "|\n"));
		}
		else if (strcmp(command,"abortSimulation") == 0){
			abortSimulation();
		}
		else if (strcmp(command, "getResults\n") == 0){
			getResults();
		}
/*
		else if (strcmp(command, "getFileList\n") == 0){
			getFileList();
		}
*/
		else if (strcmp(command, "getFile") == 0){
			getFile(strtok(NULL, "|\n"));
		}

		else if (strcmp(command, "reconnect") == 0){
			reconnect(strtok(NULL, "\n"), fd);			
		}
		else if (strcmp(command, "sendtoSE") == 0){
			sendtoSE("From GUI : ", strtok(NULL, "|"));
		}
		else if (strcmp(command, "console") == 0){
			commandConsole(strtok(NULL, "\n"), fd);
		}
		else if (strcmp(command, "init_remote") == 0){
			initRemote(strtok(NULL, "\n"), fd);
		}
		else
			cout << "else??\n" << flush;
	}
	else{
		if (currentSession == 0){
			abortSimulation(); 
		}else{
			closeClient();
		}
	}
}

void Mediator::handleData(int index){

	int fd = socketManager->getConnectedFD(index);
	char messageIN[BUFFER_SIZE];
	int rd = ioStream->readLine(fd, messageIN, BUFFER_SIZE);
	if (rd > 0){
		cout << "[From Data...]\t" << messageIN  << flush;
	}
	else 
		socketManager->closeByIndex(index);

}

void Mediator::handleUnix(int index){
	
	int fd = socketManager->getConnectedFD(index);
	char messageIN[BUFFER_SIZE];
	int rd = ioStream->readLine(fd, messageIN, BUFFER_SIZE);
	
	if (rd > 0){
		//cout << "[From S.E ->]\t" << messageIN  << flush;
		char* command = strtok(messageIN, "|");
		if (strcmp(command,"sendTime") == 0){
			sendTime(strtok(NULL, "|\n"));
		}
		else if (strcmp(command,"startTafficeGenerator") ==0){
			//trafficControl(strtok(NULL, "|\n"));
			cout << "[?????]  START traffic " << endl;
		}	
		else if (strcmp(command,"simulationDone\n") == 0){
		    	cout << "Coordinator receives the information from Simulation Engine about the simulation is down." << endl;
			simulationFinished();
		}
		else if (strcmp(command,"stopTraffic") == 0){
			//stopTraffic(strtok(NULL, "|\n"));
			cout << "[?????]  STOP traffic " << endl;
		}
		else if (strcmp(command,"sendToGUI") == 0){
			sendtoGUI(strtok(NULL, "|\n"));
		}
		else if (strcmp(command, "sendWarningtoGUI") == 0) {
			sendWarningtoGUI(strtok(NULL, "|\n"));
		}
		else
			cout << "Unknown Command\t" << command << endl << flush;
	}
	else{
		socketManager->closeByIndex(index);
	}		
}

void
Mediator::handleTCSH(int index){
	int fd = socketManager->getConnectedFD(index);
	char messageIN[BUFFER_SIZE];
	int rd = ioStream->readLine(fd, messageIN, BUFFER_SIZE);
	
	if (rd > 0){
		cout << "[From TCSH ->]\t" << messageIN  << flush;
		sendtoSE("From TCSH : ", messageIN);
	}
	else{
		socketManager->closeByIndex(index);
	}	
}

int  
Mediator::initClient(char* ip, int cmd_fd, int cmd_port, int data_port, int time_port, int info_port){
	int data_fd = socketManager->find_data_fd(ip, data_port);
	string client_ip(ip);
	ClientNode node(client_ip, cmd_port, cmd_fd, data_fd, time_port, info_port);
	clientList.setActiveClient(node);
}

string Mediator::getCurrDir(){
	return currDir;	
}

void Mediator::getTick(int fd){
	char message[128];
	sprintf(message, "getTick|%llu\n", curr_tick);
	try{
		ioStream->writen(fd, message, strlen(message));
	}
	catch(IOStream::IOException){
		cerr << "IOException!, in get tick, write to dispatcher Error!\n" << flush;
		return;
	}
	return;
}

void Mediator::becomeFree(){

	/* Flush divert rules of emulation */
	syscall_NCTUNS_divert(syscall_NSC_divert_FLUSH, 0, 0, 0, 0);
	
	socketManager->closeByFD(socketManager->getUnixSocketFD());
	int dispatcherFD = socketManager->getToDispatcherFD();
	const char* message = "setStatus|FREE\n";
	try{
	    	cout << "Coordinator tries to request Dispatcher to set its status to FREE." << endl;
		ioStream->writen(dispatcherFD, message, strlen(message));
	}
	catch(IOStream::IOException){
		cerr << "IOException! in Become free, write to dispatcher Error!\n" << flush;
		return;
	}
	return;
}


void Mediator::closeClient(){

	ClientNode* actor = clientList.getActiveClient();
	clientList.removeNode(actor->key);
	socketManager->closeByFD(actor->command_fd);
	socketManager->closeByFD(actor->data_fd);
	if (isChild)
		_exit(7);
}

void Mediator::cleanSpace(){
	char command[256];
	strcpy(command, "rm -rf ");
	strcat(command, getCurrDir().c_str());
	cout << command  << endl;
	system(command);
	
}

void 
Mediator::
commandConsole(char* message, int fd){
	char* username = strtok(message, "|");
	char* password = strtok(NULL, "|");
	char* nodeID   = strtok(NULL, "|");
	char* nctuns_home = getenv("NCTUNSHOME");
	string tcsh_str = string(nctuns_home) + "/tools/tsetenv ";
	char* gui_ip = socketManager->getPeerIP(fd);
	string cmd = tcsh_str + username + " " + nodeID + " " + gui_ip;

  try{	
	if (system(cmd.c_str()) >=0){
		ioStream->writen(fd, "OK\n", strlen("OK\n"));
		cout << "console ok" << endl;
	}
	else{
		ioStream->writen(fd, "FAIL\n", strlen("FAIL\n"));
		cout << "console fail" << endl;
	}
  }
  catch (IOStream::IOException){
  	cerr << "IOException! at Command Console!\n" << flush;
	return;
  }
  return;
	
}


void 
Mediator::
initRemote(char* msg, int cmd_fd){

	string username = string(strtok(msg, "|\n"));
	int data_port 	= atoi(strtok(NULL, "|\n"));
	
	char* ip = socketManager->getPeerIP(cmd_fd);
	int	cmd_port = socketManager->getPeerPort(cmd_fd);
	int data_fd = socketManager->find_data_fd(ip, data_port);
	string client_ip(ip);
	ClientNode node(client_ip, cmd_port, cmd_fd, data_fd, 0, 0);

	int filedes[2];
	pipe(filedes);
	
	pid_t 	childpid;
	if ((childpid = fork()) < 0){
		cerr << "Can not fork!\n";
		return;
	}
	else if (childpid == 0){
		char c_temp[64];
		ioStream->readLine(filedes[0], c_temp, 64);
	    	//sleep(1);
		RemoteHandler remote_handler(node, username, socketManager);
		remote_handler.run();
		_exit(7);
	}
	else {
		ZombieHandleThread z_thread(childpid);
		socketManager->closeByFD(cmd_fd);
		socketManager->closeByFD(data_fd);
		const char *p_temp = "Here goes child\n";
		ioStream->writen(filedes[1], p_temp, strlen(p_temp));
	}

}


