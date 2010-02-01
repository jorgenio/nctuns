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

#include <signal.h>
#include <sys/wait.h>
#include <fstream>
#include "Global.H"
#include "Mediator.H"


void Mediator::sendTime(char* message){
	u_int64_t now = strtoull(message, NULL, 10);
	curr_tick = now;
	if (timeRelay){		
		char sendString[256];
		sprintf(sendString, "sendTime|%llu\n", now);
	
		timeSocket->sendPacket((void *)sendString, strlen(sendString));
		//printf("[Current Time Relaid]\t %lu\n", now);
	}
}
int Mediator::startSimulation(const char* command, bool BG_Job){

	cout << "Start Simulation!" << endl << flush;
	string ori_command = string(command);

	isStop = false;
	isPause = false;
	
	if(!BG_Job){
		ClientNode* actClient = clientList.getActiveClient();
		string clientIP = actClient->client_ip;
		timeSocket = new DatagramSocket(clientIP.c_str(), actClient->time_port);
		infoSocket = new DatagramSocket(clientIP.c_str(), actClient->info_port);
		currentSession = 0;
	}
	
	char* nctuns_bin = getenv("NCTUNS_BIN");
	string nctunsse = string(nctuns_bin) + "/nctunsse";
	DIR* dirp = opendir(currDir.c_str());
	struct dirent *dp;	
	string tgz;
    while ((dp = readdir(dirp)) != NULL){
    	string f_name = string(dp->d_name);
    	if (f_name.rfind(".tgz") == (f_name.length() -4)){
    		tgz = currDir + f_name;
    		break;
    	}
    }
    string _cmd = "tar zvxf " + tgz + " -C "+ currDir;
    system(_cmd.c_str());
    remove(tgz.c_str());
   
	setenv("NCTUNS_WORKDIR", currDir.c_str(), 1);

    string _cmd1 = "echo " + currDir + "> /tmp/.nctuns_Cur_Workdir";
    system(_cmd1.c_str());
	
	string tcl = tgz.replace(tgz.rfind(".tgz"), 4, ".tcl");
	/* Because the string tgz have been replaced to .tcl in the
	 * previous command, we replace .tcl instead of .tgz*/
	
	pid_t childpid = -1;
	if((childpid=fork()) <0){
		perror("Can Not Fork!");
		return 1;
	}
	
	// child process 
	else if (childpid == 0){
		//cout << tcl << "|" << endl;
		//cout << nctunsse <<"|"<< endl;
		if(execl(nctunsse.c_str(), "nctunsse", tcl.c_str(), (char *)0)<0) 
        	perror("exec");
    }
    
    //Parent Process
    else{
    	engine_pid = childpid;
    	//registProcess(ori_command, childpid);
	}
	

}

int Mediator::pauseSimulation(const char* cmd){

	if(!isPause)
	{
		cout << "Pause Simulation!" << endl;
		sendtoSE("From GUI : ", "Pause\n");
		string reply;
		readlnSE(reply);
		cout << "reply form SE: " << reply << endl;
		cout << "About to send signal to SE\n" << endl;
		//getchar();
		kill (engine_pid, SIGSTOP);
		//processList.pauseAll();	
	}
	else
		isPause = false;
}

int Mediator::stopSimulation(const char* cmd)
{
	if(!isStop)
	{
		if(isPause)
		{
			isPause = false;
			cout << "Resume Simulation!" << endl;
			kill (engine_pid, SIGCONT);
			string reply1;
			readlnSE(reply1);
			cout << "reply: form SE: " << reply1 << endl;
			usleep(100000);
		}

		cout << "Stop Simulation!" << endl;
	
		sendtoSE("From GUI : ", "Stop\n");
		string reply;
		readlnSE(reply);
		cout << "reply: form SE: " << reply << endl;
		//getchar();
		kill (engine_pid, SIGKILL);
		waitpid(engine_pid, NULL, 0);
		engine_pid = -1;	
		//processList.stopAll();
		if (currentSession == 0){	
			int command_fd = clientList.getActiveClient()->command_fd;
			char command[128];
			strcpy(command, "simulationDone\n");
			try{
				ioStream->writen(command_fd, command, strlen(command));	
			}
			catch (IOStream::IOException){
				cerr << "IOException! in stop simulation, write to GUI Error!\n" << flush;
				return -1;
			}
		}
	}
	else
		isStop = false;
	//closeClient();
	//becomeFree();
	return 1;
}

int Mediator::resumeSimulation(const char* cmd){
	cout << "Resume Simulation!" << endl;
	kill (engine_pid, SIGCONT);
	sendtoSE("From GUI : ", "Continue\n");
	string reply;
	readlnSE(reply);
	cout << "reply form SE: " << reply << endl;
	//processList.resumeAll();	
}

int Mediator::abortSimulation(){
	
	if (engine_pid > 0 && socketManager->getUnixSocketFD() > 0){
		sendtoSE("From GUI : ", "Abort\n");
		string reply;
		readlnSE(reply);
		cout << "reply form SE: " << reply << endl;
		kill (engine_pid, SIGKILL);
		waitpid(engine_pid, NULL, 0);
		engine_pid = -1;
	}
	
	cleanSpace();
	if (currentSession == 0){
		closeClient();
	}

	//cleanSpace();
	becomeFree();	
}

int Mediator::simulationFinished(){
  try{
	if (currentSession >0){
		sessionList.setSessionStatus(currentSession, sessionList.FINISHED);
		//cout << sessionList.getSessionStatus(currentSession);
		int dispatcherFD = socketManager->getToDispatcherFD();
		char message[128];
		sprintf(message, "jobFinished|%lu\n", currentSession);
		ioStream->writen(dispatcherFD, message, strlen(message));		
		currentSession = 0;
	}
	else{
		int command_fd = clientList.getActiveClient()->command_fd;
		char command[128];
		strcpy(command, "simulationDone\n");
		ioStream->writen(command_fd, command, strlen(command));
	}
  }
  catch(IOStream::IOException){
  	cerr << "IOException!, in function Simulation Finish...\n" << flush;
	return -1;
  }
	//kill (engine_pid, SIGKILL);
	//waitpid(engine_pid, NULL, 0);
	//processList.abort();
	becomeFree();
	return 1;
}

int 
Mediator::BG_start(const char* command, unsigned long sessionID){
	string dir = getCurrDir();
	SessionNode node(sessionID, dir);
	sessionList.addSession(sessionID, node);
	currentSession = sessionID;
	timeRelay = false;
	startSimulation(command, true);
}

/*
void Mediator::trafficControl(char* message){
	//cout << message << endl << flush;
	char oriCmd[128];
	strcpy(oriCmd, message);
	char* token;	
	vector<string> tokens;
	token = strtok(message, "\n\t ");
	while (token != NULL){
		string _token = string(token);
		tokens.push_back(_token);
		token = strtok(NULL, "\n\t ");
	}
	
	int childpid = -1;
	if((childpid=fork()) <0){
		perror("Can Not Fork!");
		return;
	}
	
	// child process 	
	else if (childpid == 0){
		char syscmd[BUFFER_SIZE];
		char trafficCmd[BUFFER_SIZE];
		int node = atoi(tokens[0].c_str());
		
		int pid = getpid();
		syscall(290, 04, pid, node, 0);
		syscall(286, pid);
		
		
		char* toolpath = getenv("NCTUNS_TOOLS");
		sprintf(trafficCmd, "%s/%s", toolpath, tokens[1].c_str());
		//cout << trafficCmd << endl << flush;
		int tokenSize = tokens.size();
		char* argv[tokenSize];
		for (int i=1; i<tokens.size(); i++){
			argv[i-1] = (char*)tokens[i].c_str();
			//cout << "kkk" << argv[i-1] << "\t" ;
		}
		//argv[tokenSize-1] = "-n";
		//argv[tokenSize]   = tokens[0];
		argv[tokenSize-1] = 0; // NULL terminate
        if(execvp(trafficCmd, argv)<0) {
				perror("exec fail");
				return;
		}    
	}// else if
    
    //Parent Process
    else{
    	
    	//if (syscall(286, childpid) <0)
    	//	perror("System Call Error!\n");
    	
    	registProcess(string(oriCmd), childpid);
	}
	
	return;
}

void 
Mediator::stopTraffic(char* message){
	processList.remove(message);
}

void Mediator::registProcess(string id, int childpid){
	processList.add(id, childpid);
	
}
	    
*/



