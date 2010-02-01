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

#ifndef	__COOR_MEDIATOR
#define	__COOR_MEDIATOR

#include <vector>
#include <string>

using namespace std;

#include "ClientList.H"
#include "ProcessList.H"
#include "SessionList.H"
#include "SessionNode.H"

#define RTMSG_INFORMATION	0x01
#define RTMSG_WARNING		0x02
#define RTMSG_FATAL_ERROR	0x03

struct runtimeMsgInfo {
	u_int32_t	type;
	double		time;
	int		nodeID;
	char		module[20];
	char		message[128];
};

class IOStream;
class SocketManager;
class DatagramSocket;

class Mediator {
								
	public:
	
		Mediator(SocketManager* manager);
			
		void handleDispatcher(int index);
		void handleCommand(int index);
		void handleData(int index);
		void handleUnix(int index);
		void handleTCSH(int index);
		
		string getCurrDir();

	protected:
	
		void registerToDispatcher();
		void setupEnv(vector<char*> tokens, int fd);
		int  initClient(char* ip, int cmd_fd, int cmd_port, int data_port, int time_port, int infor_port);
		int  putfile(char* filename, long fileSize, int cmd_fd, int data_fd);
		int  startSimulation(const char* cmd, bool bkJob);
		int  pauseSimulation(const char* cmd);
		int  stopSimulation(const char* cmd);
		int  resumeSimulation(const char* cmd);
		int  abortSimulation();
		int  simulationFinished();
		
		void sendTime(char* message);	
	
	/* 
	 * Since Simulation Engine used discrete event method,
	 * traffic generators have been forked by engine instead of coordinator
	 */
	/* 	
		void trafficControl(char* message);
		void stopTraffic(char* message);
		void registProcess(string id, int childpid);
	*/
		void becomeFree();
		void closeClient();
		void cleanSpace();
		
		void disconnect(char* sessionID, int fd);
		void reconnect(char* msg, int commandFD);

		void sendtoSE(const char* header, const char* command);
		void readlnSE(string& message);
		void sendtoGUI(char* message);
		void sendWarningtoGUI(char* message);
		void commandConsole(char* message, int fd);
		void initRemote(char* data_port, int cmd_fd);		
		
		void getResults();
		
		//void getFileList();
		int BG_put_file(char* file_name, long file_size, int fd);
		int BG_start(const char* command, unsigned long sessionID);
		void getFile(char* filename);
		void getTick(int fd);
		
		
	private:

		IOStream* 	ioStream;
		SocketManager* 	socketManager;	
		DatagramSocket* timeSocket;
		DatagramSocket*	infoSocket;
	
		ProcessList 	processList;
		SessionList 	sessionList;
		ClientList	clientList;
	
		pid_t		engine_pid;
		string 		currDir;
		u_int64_t	curr_tick;
		bool		timeRelay;
		unsigned long   currentSession;
		bool		isChild;
		bool		isPause;
		bool		isStop;
		/*
		 * Function to support emulation
		 * Set up ip firewall rules
		 */
		int 		ipfw(string ifw_file);

};
#endif
