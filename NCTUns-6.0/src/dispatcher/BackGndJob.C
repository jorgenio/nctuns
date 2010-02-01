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

#include <sys/stat.h>
#include <pwd.h>
#include <dirent.h>

#include "Global.H"
#include "Mediator.H"
#include "JobNode.H"
#include "ServerNode.H"

int 	getFiles(char* filePath, int fd);
string	expand_home (string dir);

int 
Mediator::backGndJob(vector<char*> tokens, int fd){
	
	string username = tokens[1];
	string password = tokens[2];
	string jobName  = tokens[3];
	int	   maxTime  = atoi(tokens[4]);
	string client_ip = string(sockManager->getPeerIP(fd));


	try {
		
	if (!verifyPass((char*)username.c_str(), (char*)password.c_str())){
		cout << "login_fail|" << username << "|" << password << "\n";
		string ans = "login_fail|\n";
		ioStream->writen(fd, ans.c_str(), ans.length());
		return -1;
	}
	else if (username == "guest" && jobQueue.getWaitJobNumber("guest", client_ip) > 5){
		cout << "Quota_Exceed|\n";
		string ans = "Quota_Exceed|\n";
		ioStream->writen(fd, ans.c_str(), ans.length());
		return -2;
	}
	else{

		unsigned long jobID = gnSessionID();
		char jobDir[256];
		sprintf(jobDir, "~%s/.nctuns/job-queue/%lu-%s/", username.c_str(), jobID, jobName.c_str());
		char mkdir_cmd[256];
		sprintf(mkdir_cmd, "mkdir -p %s", jobDir);

		char reply[128];		

		if(system(mkdir_cmd)<0) {
			cout << "Make Directory Error!\n";
			strcpy(reply, "Submit Fail!\n");
			ioStream->writen(fd, reply, strlen(reply));
		}
		else{
			time_t now;
			time(&now);
			string s_time = string(ctime(&now));
			string submit_time = s_time.substr(0, s_time.length() -1);
			JobNode jNode(jobID, username, jobName, jobDir, maxTime, submit_time, client_ip);
			jobQueue.addJob(jNode);
			//cout << jNode.toString() << endl;

			sprintf(reply, "OK|%lu\n", jobID);
			ioStream->writen(fd, reply, strlen(reply));

			getFiles(jobDir, fd);
			cout << "Back ground job submitted, JobID: " << jobID <<endl;
			
			int serverIndex = serverList->anyFreeHost();
			if(serverIndex >= 0){
				ServerNode* node = serverList->getHost(serverIndex);
				serverList->setStatus(serverIndex, BUSY);
				dispatchJob(node->opened_fd, node->clientCmdPort, node->clientDataPort);
			} // if			
			return 1;
		} // else
	}//else

	}//try
	catch (IOStream::IOException){
		cerr << "Socket IO Error!" << endl << flush;
		return -1;
	}
}

int 
getFiles(char* filePath, int fd){
	
	try{
		
	IOStream* ioStream;
	char message[256];
	ioStream->readLine(fd, message, 256);
	//cout << "message: " << message;
	strtok(message, "|\n");
	const char* filename = strtok(NULL, "|\n");
	long  fileSize = atol(strtok(NULL, "|\n"));
		
	string file_dir = expand_home(string(filePath));
	string _outfile = file_dir + string(filename);
	const char *outfile = _outfile.c_str();
	const char* reply = "please\n";
	ioStream -> writen(fd, reply, strlen(reply));	
	cout << "[Saving file...] " << outfile << endl << flush; 
	int 	data_fd = fd;
	long 	buf_size = 256;
	char 	buf[buf_size];
	long 	toRead = fileSize;	
	int 	readBuf = min(toRead, buf_size);
	FILE* out = fopen(outfile, "w");
	
	while ((ioStream->readn(data_fd, buf, readBuf) >0)&& (toRead >0)){
		fwrite (buf, 1, readBuf, out);
		toRead = toRead - readBuf;
		readBuf = min(toRead, buf_size);
		memset(buf, 0, buf_size);
	}
	fclose(out);
	reply="File get\n";
	ioStream -> writen(fd, reply, strlen(reply));
	}
	catch (IOStream::IOException){
		cerr << "IO Exception while getting file" << endl << flush;
		return -1;
	}
	
	cout << "[Got it!]\n" << flush;
	return 1;
		
}


/*------------------------------------------------------------------------*/

/**
 * A utility function that expand ~ to absolute directory
 */
string expand_home (string dir){
	int		pos = dir.find_first_of('/');
	string 	user = dir.substr(1, pos-1);
	string 	subdir = dir.substr(pos, string::npos);
	struct 	passwd* pw = getpwnam(user.c_str());
	return 	string(pw->pw_dir) + subdir;
}

/*------------------------------------------------------------------------*/

/**
 * A utility function that compute the file size of a file
 */
int 
Mediator::
get_file_size(const char *path){
  struct stat file_stats;
  if(stat(path,&file_stats))
    return -1;
  return file_stats.st_size;
}

/*------------------------------------------------------------------------*/



int  
Mediator::
dispatchJob(int coor_fd, int cmd_port, int data_port){

	JobNode jobNode;
	if (jobQueue.getJob(jobNode) < 0){
		return -1;
	}
	//cout << jobNode.toString() << endl;
	string full_path = expand_home(jobNode.directory);
	DIR* dp = opendir(full_path.c_str());
	struct dirent* dirEnt;
	string file_name;
	while ((dirEnt = readdir(dp)) != NULL){
		file_name = string(dirEnt->d_name);
		if ((file_name!=".") && (file_name!="..")){
			full_path += file_name ;
			break;
		}
	} //while
	
	char ignore[128]; // to store replies that we can ignore
	
	//cout << full_path << endl;
	
  try{
	string username = jobNode.username;
	string serv_req = "setupEnv|" + username + "\n";
	ioStream->writen(coor_fd, serv_req.c_str(), serv_req.length());
	ioStream->readLine(coor_fd, ignore, 128);
	
	int file_size = get_file_size(full_path.c_str());
	//cout << "size:" << file_size << endl;
	
	ostringstream s_out;
	s_out << "BG_putFile|" << file_name << "|" << file_size << "\n";
	string put_cmd = s_out.str();
	ioStream->writen(coor_fd, put_cmd.c_str(), put_cmd.length());
	ioStream->readLine(coor_fd, ignore, 128);
	
	FILE* fileFD = fopen(full_path.c_str(), "r");
	long toSend = file_size;
	long send_buf;
	long buf_size = 1024;
	char buf[buf_size];
	send_buf = min(toSend, buf_size);
	while((fread(buf, 1, send_buf, fileFD) >0) && (toSend >0)){
		ioStream->writen(coor_fd, buf, send_buf);
		toSend = toSend-send_buf;
		send_buf = min(toSend, buf_size);
	}
	fclose(fileFD);	

	int temp = ioStream->readLine(coor_fd, ignore, 64);
	if (strcmp(ignore, "File get\n") != 0){
		cout << "ERROR!" <<temp<<"|"<<ignore<< endl << flush;
		return -1;
	}

	cout << "[File Sent!]\n" << endl << flush;
	ostringstream s_out2;
	s_out2 << "BG_start|" << username << "|" << jobNode.jobID << "\n";
	string start_cmd = s_out2.str();
	ioStream->writen(coor_fd, start_cmd.c_str(), start_cmd.length());
	time_t now;
	time(&now);
	string s_time = string(ctime(&now));
	string start_time = s_time.substr(0, s_time.length() -1);
	jobQueue.setStatus(jobNode.jobID, "RUNNING");
	jobQueue.setStartTime(jobNode.jobID, start_time);
	string coor_ip = string(sockManager->getPeerIP(coor_fd));
	jobQueue.set_coor(jobNode.jobID, coor_ip, cmd_port, data_port);
  }
  catch (IOStream::IOException){
  	cerr << "IOException! Can't dispatch Background Job!!\n" << flush;
	return -1;
  }
}

int
Mediator::
BG_jobFinished(char* jobID){
	unsigned long job_id;
	sscanf(strtok(jobID, "|\n"), "%lu", &job_id);
	if(jobQueue.hasJob(job_id)){
		jobQueue.setStatus(job_id, "FINISHED");	
		jobQueue.setProgress(job_id, 100);
		time_t now;
		time(&now);
		string s_time = string(ctime(&now));
		string end_time = s_time.substr(0, s_time.length() -1);	
		jobQueue.setEndTime(job_id, end_time);
	}
	else
		return -1;
	
}

unsigned long long
Mediator::
getTick(string coor_ip){
	ServerNode *sNode = serverList->getHost(coor_ip);
	int coor_fd = sNode->opened_fd;
	const char *message = "getTick\n";
  try{
	ioStream->writen(coor_fd, message, strlen(message));
	char	reply[256];
	if (ioStream->readLine(coor_fd, reply, 256) > 0){
		strtok(reply, "|\n");
		return strtoull(strtok(NULL, "|\n"), NULL, 10);
	}
	else
		return 0;
  }
  catch (IOStream::IOException){
  	cerr << "IOException! Can't get tick!!\n" << flush;
	return 0;
  }
}

int
Mediator::  
getJobList(vector<char*> tokens, int fd){
	string info = "";
	jobQueue.getJobInfo(tokens, info, this);
	if (info.length() > 2){
		info += "\n";
	}
	else{
		info = "EMPTY\n";
	}
  try{
	ioStream->writen(fd, info.c_str(), info.length());
  }
  catch (IOStream::IOException){
  	cerr << "IOException! Get Job List!!\n" << flush;
	return -1;
  
  }
	
}


int	 
Mediator::
BG_Control (vector<char*> tokens, int fd){
	if (strcmp(tokens[1], "RETRIEVE") == 0){
		retrieve_job(tokens[2], fd);
	}
	else if (strcmp(tokens[1], "RECONNECT") == 0){
		reconnect_job(tokens[2], fd);
	}
	else if (strcmp(tokens[1], "DELETE_JOB") == 0){
		delete_job(tokens[2]);
	}
	else if (strcmp(tokens[1], "ABORT_JOB") == 0){
		abort_job(tokens[2]);
	}
	else if (strcmp(tokens[1], "STOP_JOB") == 0){
		stop_job(tokens[2]);
	}
	//cout << tokens[1] << endl;
}

int
Mediator::
retrieve_job(char* jobID, int gui_fd){
	unsigned long job_id; 
	sscanf(jobID, "%lu" , &job_id);
	JobNode* j_node = jobQueue.getJob(job_id);
	ServerNode* s_node = serverList->getHost(j_node->coor_ip); 
	ostringstream s_out;
	s_out << "COOR_INFO|" << j_node->coor_ip << "|" <<
s_node->clientCmdPort << "|" << s_node->clientDataPort << "\n";
	string message = s_out.str();
  try{
	ioStream->writen(gui_fd, message.c_str(), message.length());
  }
  catch (IOStream::IOException){
  	cerr << "Background Retrieve/Reconnect Job Error!!\n" << flush;
	return -1;
  }
}

int
Mediator::
reconnect_job(char* jobID, int gui_fd){
	return retrieve_job(jobID, gui_fd);
}


int 
Mediator::
delete_job(char* jobID){
	unsigned long job_id; 
	sscanf(jobID, "%lu" , &job_id);
	jobQueue.removeJob(job_id);
}

int
Mediator::
abort_job(char* jobID){
	unsigned long job_id; 
	sscanf(jobID, "%lu" , &job_id);
	JobNode* j_node = jobQueue.getJob(job_id);
	if (j_node->status == "RUNNING"){
		ServerNode* s_node = serverList->getHost(j_node->coor_ip);
		int coor_fd = s_node->opened_fd;
		string command = "JOB_ABORT|" + string(jobID) + "\n";
		try{
			ioStream->writen(coor_fd, command.c_str(), command.length());
		}
		catch(IOStream::IOException){
			cerr << "IOException! Background Job Abort!!\n" << flush;
			return -1;
		}
	}
	delete_job(jobID);
}

int
Mediator::
stop_job(char* jobID){

	unsigned long job_id; 
	sscanf(jobID, "%lu" , &job_id);
	JobNode* j_node = jobQueue.getJob(job_id);
	if (j_node->status == "RUNNING"){
		ServerNode* s_node = serverList->getHost(j_node->coor_ip);
		int coor_fd = s_node->opened_fd;
		string command = "JOB_STOP|" + string(jobID) + "\n";
		try{
			ioStream->writen(coor_fd, command.c_str(), command.length());
		}
		catch(IOStream::IOException){
			cerr << "IOException! Background Job Stop!!\n" << flush;
			return -1;
		}
	}
	jobQueue.setStatus(job_id, "FINISHED");
}

