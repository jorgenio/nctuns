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

#ifndef		__MEDIATOR
#define		__MEDIATOR

#include <vector>
using namespace std;

#include "ServerList.H"
#include "SessionList.H"
#include "JobQueue.H"

class IOStream;
class SocketManager;

class Mediator{

	IOStream* 	ioStream;
	SocketManager* 	sockManager;	
	ServerList*	serverList;
	
	public:
	
		Mediator(SocketManager* manager, ServerList* serverList);
			
		void handleServer(int fdIndex);
		void handleClient(int fdIndex);
		unsigned long long getTick(string coor_ip);

	protected:
	
		void do_Server  (char* message, int fd);
		void do_Client  (char* message, int fd);
		void registServer(vector<char*> message, int fd);
		int  setupSimEnv(int index, char* username, int clientFD);	
		int  verifyPass (char* username, char* passwd);
		void setStatus  (const char* status, int fd);
		void servRequest(vector<char*> tokens, int fd);
		int  disconnect (vector<char*> tokens, int fd);
		int  reconnect  (vector<char*> tokens, int fd);
		int  backGndJob (vector<char*> tokens, int fd);
		int  getJobList (vector<char*> tokens, int fd);
		int	 BG_Control (vector<char*> tokens, int fd);
		int  getAppManual(vector<char*> tokens, int fd);
		int	 remoteManagement(vector<char*>tokens, int fd);
		int  dispatchJob(int fd, int c_port, int d_port);
		int	 BG_jobFinished(char* job_id);
		
		int  retrieve_job (char *job_id, int gui_fd);
		int  reconnect_job(char *job_id, int gui_fd);
		int  delete_job(char *job_id);
		int	 abort_job (char *job_id);
		int	 stop_job  (char *job_id);
		
		unsigned long gnSessionID();
		int get_file_size(const char *path);
		int sent_a_file(int fd, FILE *f_ptr, long file_size);

		void log_ip(const char *username, char *email, int fd);

	private:
		SessionList	sessionList;
		JobQueue	jobQueue;
		
};

#endif

