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

#include <pwd.h>

#include <shadow.h>
#include <unistd.h>

#include "Global.H"
#include "Mediator.H"
#include "ServerNode.H"
#include "ServerList.H"
#include "SessionNode.H"
#include "SessionList.H"
#include "JobQueue.H"

extern string log_file;

Mediator::Mediator(SocketManager* manager, ServerList* list){
	sockManager = manager;
	serverList  = list;
	sockManager->regist(this);
}

void Mediator::handleServer(int fdIndex){
	char messageIN[BUFFER_SIZE];
	memset(messageIN, 0, BUFFER_SIZE);
	int fd = sockManager->getConnectedFD(fdIndex);
	cout << "[From Server...] " ;
	int rd = ioStream->readLine(fd, (void*)messageIN, BUFFER_SIZE);
	if (rd < 0 ){
		cout << "Server Connection ERROR !!" << endl << flush;
		sockManager->closeConnection(fdIndex);
	}
	else if ( rd == 0){		
		serverList->removeHost(sockManager->getPeerIP(fd));
		cout << "Server Closed" << endl << serverList->toString() << flush;
		sockManager->closeConnection(fdIndex);	
	}
	else{
		cout << messageIN << flush;
		do_Server(messageIN,fd);		
	}
}


void Mediator::handleClient(int fdIndex){
	char messageIN[BUFFER_SIZE];
	int fd = sockManager->getConnectedFD(fdIndex);
	cout << "[From Client...] ";
	int rd = ioStream->readLine(fd, (void*)messageIN, BUFFER_SIZE);
	if (rd < 0 ){
		cout << "Client Connection ERROR !!" << endl << flush;		
		sockManager->closeConnection(fdIndex);
	}
	else if ( rd == 0){
		cout << "Client Closed" << endl << flush;
		sockManager->closeConnection(fdIndex);
	}
	else{
		cout << messageIN << flush;
		do_Client(messageIN, fd);
	}
	
}


void Mediator::do_Server(char* message, int fd){
	vector<char*> tokens ;
	char* token = strtok(message, "|");
	while (token != NULL){
		tokens.push_back(token);
		token = strtok(NULL, "|");	
	}
	if (strcmp(tokens[0], "register") == 0){	
		registServer(tokens, fd);
	}
	else if (strcmp(tokens[0], "setStatus") == 0){
		setStatus(tokens[1], fd);
	}
	else if (strcmp(tokens[0], "jobFinished") == 0){
		BG_jobFinished(tokens[1]);
	}
	else
		cout <<"ELSE...ignore!" << endl << flush;
	
	return;
	
}

void Mediator::do_Client(char* message, int fd){
	vector<char*> tokens ;
	char* token = strtok(message, "|");
	while (token != NULL){
		tokens.push_back(token);
		token = strtok(NULL, "|\n");	
	}
	if (strcmp(tokens[0], "serv_req") == 0){
		servRequest(tokens, fd);
	}
	else if (strcmp(tokens[0], "disconnect") == 0){
		disconnect(tokens, fd);
	}
	else if (strcmp(tokens[0], "reconnect") == 0){
		reconnect(tokens, fd);	
	}
	else if (strcmp(tokens[0], "backgroundjob") == 0){
		backGndJob(tokens, fd);
	}
	else if (strcmp(tokens[0], "getJobList") == 0){
		getJobList(tokens, fd);
	}
	else if (strcmp(tokens[0], "BG_MANAGE") == 0){
		BG_Control(tokens, fd);
	}
	else if (strcmp(tokens[0], "remoteManagement") == 0){
		remoteManagement(tokens, fd);
	}
	else if (strcmp(tokens[0], "GetAppManual") == 0){
		getAppManual(tokens, fd);	
	}
	else	
		cout <<"ELSE.." << tokens[0] << endl << flush;


	return;	
}


void Mediator::registServer(vector<char*> tokens, int fd){
	ServerNode node;
	node.ip		 = tokens[1];
	node.opened_fd = fd;
	sscanf(tokens[2], "%d" , &node.clientCmdPort);
	sscanf(tokens[3], "%d" , &node.clientDataPort);
	if (strcmp(tokens[4], "IDLE\n") == 0)
		node.status = FREE;
	serverList->addHost(node);
	
	const char* reply = "OK\n";
	try {
		ioStream->writen(fd, reply, strlen(reply));
	}
	catch (IOStream::IOException){
		cerr << "Write to Coordinator Error!\n" << flush;
		return;
	}
	cout << "(Register Complete!)\n" << serverList->toString() << flush;

//  Try to dispatch a back ground job	
	if (dispatchJob(fd, node.clientCmdPort, node.clientDataPort) >=0){
		setStatus("BUSY\n", fd);
	}
}

void Mediator::
log_ip(const char *username, char *email, int fd){
	string log_str = "";
	time_t now;
	time(&now);
	const char *time_str = ctime(&now);
	const char *ip_str 	 = sockManager->getPeerIP(fd);

	if (email != NULL) {
		log_str = string(ip_str) + "\t" + username +"\t" + 
			  email + "\t" + string(time_str);
	}
	else {
		const char * tmp_email = "[no email]";
		log_str = string(ip_str) + "\t" + username +"\t" + 
			  tmp_email + "\t" + string(time_str);
	}	
	FILE *log = fopen(log_file.c_str(), "a");
	if (log != NULL)
		fwrite(log_str.c_str(), 1, log_str.length(), log);
	fclose(log);

}

void Mediator::servRequest(vector<char*> tokens, int fd){
	
	const char* reply; 
	char* username = tokens[1];
	char* password = tokens[2];
	char* servIP = NULL;
	//char* email	   = tokens[3];
	
	if(tokens.size() == 4)
		servIP = tokens[3];

	try{
		if (verifyPass(username, password)){
			//log_ip(username, email, fd);
			int serverIndex = -1;
			if(servIP == NULL)
				serverIndex = serverList->anyFreeHost();
			else
				serverIndex = serverList->findFreeHost(servIP);

			if(serverIndex >= 0){
				int test = setupSimEnv(serverIndex, username, fd);
			}
			else{
				cout << "no_idle_server\n";
				reply = "no_idle_server\n";
				ioStream->writen(fd, reply, strlen(reply));
			}		
		}
		else{
			cout << "login_fail\n";
			reply = "login_fail\n";
			ioStream->writen(fd, reply, strlen(reply));
		}
		return;
	}
	catch(IOStream::IOException){
		cerr << "IOException!, Service Request, write to GUI!\n" << flush;
		return;
	}
}


int check_pass(const char *plainpw, const char *cryptpw){

	//printf("plainpw = %s, cryptpw = [%s]\n", plainpw, cryptpw);
	//printf("crypt(plainpw, cryptpw) = [%s]\n",crypt(plainpw, cryptpw));
	
        return strcmp(crypt(plainpw, cryptpw), cryptpw) == 0;
}

int Mediator::verifyPass (char* username, char* passwd){
	
	struct  passwd 	*ptr;
	char	*cryptpass;

	struct spwd *spw;
	spw = getspnam(username);
	if (spw == NULL) {
		printf("getspnam() return NULL\n");
		return 0;
	}else{
	    int ret = check_pass(passwd, spw->sp_pwdp); 
	    if(!ret)
		    printf("check_pass() return 0\n");
	    return ret;
	}
}

int Mediator::setupSimEnv(int index, char* username, int clientFD){

	string clientIP = sockManager->getPeerIP(clientFD);
	int  clientPort = sockManager->getPeerPort(clientFD);
	
	serverList->setClient(index, clientIP, clientPort);
	serverList->setStatus(index, BUSY);
	
	ServerNode* node = serverList->getHost(index);
	int coordinatorFD = node->opened_fd;
			
	//cout << "fd: " << coordinatorFD << "\t" << node->ip << "\tstatus:"<< node->status<<endl;

	char message[128];
	sprintf(message, "setupEnv|%s\n", username);
	char messageIN[BUFFER_SIZE];
   try{
	ioStream->writen(coordinatorFD, message, strlen(message));
	ioStream->readLine(coordinatorFD, (void*)messageIN, BUFFER_SIZE);
	cout << "[Coordinator reply]..." << messageIN << flush;
	if (strcmp (messageIN, "EnvOK\n") !=0)
		cout << "Setup Environment Error!" << endl << flush;
	else{
		char reply[256];
		sprintf(reply, "OK|%s|%d|%d\n", node->ip.c_str(), node->clientCmdPort, node->clientDataPort);
		//cout << reply << flush; 
		ioStream->writen(clientFD, reply, strlen(reply));
	}
	return 0;
   }
   catch (IOStream::IOException){
   		cerr << "IOException! Set up Simulation Environment!\n" << endl;
		return -1;
   }
}

void Mediator::setStatus(const char* status, int fd){
	string tmpIP = serverList->getHostIP(fd);
	const char* peerIP = tmpIP.c_str();
	//char* peerIP = sockManager->getPeerIP(fd);
	//cout << "ip:"<< peerIP << "\tstatus:" << status << flush;
	if (strcmp(status, "FREE\n") ==0){
		string coor_ip = string(peerIP);
		ServerNode* node_ptr = serverList->getHost(coor_ip);
		if (dispatchJob(fd, node_ptr->clientCmdPort, node_ptr->clientDataPort) <0)
			serverList->setStatus(peerIP, FREE);
	}
	else
		serverList->setStatus(peerIP, BUSY);
}



int Mediator::disconnect(vector<char*> tokens, int fd){
	cout << "[Disconnect!]\t" ;
	string clientIP = sockManager->getPeerIP(fd);
	int clientPort =  sockManager->getPeerPort(fd);
	ServerNode* node = serverList-> getHost(clientIP, clientPort);
	string serverIP = node->ip;
	string sessionName(tokens[1]);
	SessionNode sNode(gnSessionID(), sessionName, serverIP, clientIP, clientPort); 
	
	sessionList.addSession(sNode);

	char message[256];
	char temp[128];
	sprintf(message, "disconnect|%lu\n", sNode.sessionID); 
	
	try{
		ioStream->writen(node->opened_fd, message, strlen(message));
		ioStream->readLine(node->opened_fd, temp, 128);
		ioStream->writen(fd, message, strlen(message));
		return 1;
	}
	catch(IOStream::IOException){
		cerr << "IOException! Disconnect!\n" << flush;
		return -1;
	}
	 	
	//cout << message ;
	
}

int Mediator::reconnect(vector<char*> tokens, int fd){
	//cout << "[Reconnect!...Reply:]\t";
	unsigned long sessionID;
	sscanf(tokens[1], "%lu", &sessionID);
	//cout << sessionList.toString() << flush;
	SessionNode* node = sessionList.getSession(sessionID);

	char reply[256];
	if (node != NULL){
		ServerNode* s_node = serverList->getHost(node->coordinatorIP);
		sprintf(reply, "reconnect|%s|%d|%d\n", 
			   (s_node->ip).c_str(), s_node-> clientCmdPort, s_node->clientDataPort);
		sessionList.removeSession(node->sessionName);
		//cout << sessionList.toString() << flush;
	}
	else
		strcpy(reply, "No_Such_Session\n");	
	//cout << reply;
	try{
		ioStream->writen(fd, reply, strlen(reply));
		return 1;
	}
	catch(IOStream::IOException){
		cerr << "IOException! Reconnect\n" << flush;
		return -1;
	}
}

int	
Mediator::
remoteManagement(vector<char*> tokens, int fd){
	
	ServerNode* node = serverList->get_aHost();
	string 		reply;
	if(node != NULL){
		ostringstream out;
		out << "coorInfo|" << node->ip << "|" << node->clientCmdPort << "|" << node->clientDataPort << "\n";
		reply = out.str();
	}
	else{
		reply = "NO_SERVER\n";
	}

	try{
		ioStream->writen(fd, reply.c_str(), reply.length());
		return 1;
	}
	catch(IOStream::IOException){
		cerr << "IOException! Remote File Management Request\n" << flush;
		return -1;
	}
}

int
Mediator::  
getAppManual(vector<char*> tokens, int fd){
	const char *reply_fail = "FAIL|\n";
	string app_xml = string(getenv("NCTUNSHOME")) + "/etc/app.xml";
	FILE  *rd_file = fopen(app_xml.c_str(), "r");
	try{
		if (rd_file != NULL){
			int file_size = get_file_size(app_xml.c_str());
			ostringstream out;
			out << "OK|" << file_size << "\n";
			string reply_file = out.str();
			ioStream->writen(fd, reply_file.c_str(), reply_file.length());
			sent_a_file(fd, rd_file, file_size);
			fclose(rd_file);
			cout << "Application Manual Sent!" << endl;
			return 1;
		}
		else{
			fclose(rd_file);
			cout << "Can't Open Application Manual!" << endl;
			ioStream->writen(fd, reply_fail, strlen(reply_fail));
			return -1;
		}
	}
	catch (IOStream::IOException){
		cerr << "IOException! Get Application Manual\n" << flush;
		return -1;
	}
	
}

/*------------------------------------------------------------------------*/

/**
 * A utility function that sent a file to remote machine
 */

int
Mediator::
sent_a_file(int fd, FILE *f_ptr, long file_size){
	long toSend = file_size;
	long send_buf;
	long buf_size = 1024;
	char buf[buf_size];
	send_buf = min(toSend, buf_size);
	while((fread(buf, 1, send_buf, f_ptr) >0) && (toSend >0)){
		ioStream->writen(fd, buf, send_buf);
		toSend = toSend-send_buf;
		send_buf = min(toSend, buf_size);
	}
	return 1;

}

/*------------------------------------------------------------------------*/

/*
 * Generate Session ID
 */
unsigned long Mediator::gnSessionID(){
	time_t now;
	return time(&now);	
}

