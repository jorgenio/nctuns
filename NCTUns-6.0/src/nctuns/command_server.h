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

#ifndef __NCTUNS_COMMAND_SERVER_H__
#define __NCTUNS_COMMAND_SERVER_H__

#include <config.h>
#include <command_format.h>
#include <misc/log/logpack.h>
#include "nid_information.h"

#define CHECK_IPC_COMMAND_INTERVAL	100 	// ms
#define CHECK_IPC_MESSAGE_TIMEOUT	40000	// us, real time (40ms)
#define CHECK_IPC_MESSAGE_MAX_TIMES	5

class NslObject;

extern NslObject		**nodelist;

void scheduleCheckIPCCommandFds();

struct CmdServRegInfo {
	int fd;
	u_int32_t nid;
	u_int16_t port;
	pid_t	pid;
	//NslObject *wme_pointer;
	NslObject *wme_obj;
	bool enable;
	struct CmdServRegInfo *next;
};

class cmd_server {
private:
	int mudpsock;
	int msock;	// master IPC server TCP socket
	fd_set rfds;	// read file socket descriptors for agent clients
	fd_set afds;	// active read file socket descriptors for agent clients 
	int nfds;

	nid_information *nid_info[MAX_NUM_NODE + 1];

	TRIGGER_MAP m_mapTriggerMap;
	struct CmdServRegInfo *cmd_serv_reg_info_first, *cmd_serv_reg_info_end;
public:
	cmd_server();
	~cmd_server();

	/* for command execution */
	/* 2007/2/27 added by automatic vehicle group */
	int	ReadSignalFile();
	int 	initAgentClient(char * msg, int fd);
	int 	associateAgentClient(int fd);
	int	sendBackACKToAgentClient(int fd, int result);
	int 	communicateAgentClient(int fd, char* msg, char* longMsg);
	int 	registerApproachingNotification(int nid, int registrationID,
		int timeIntervalInMilliseconds, int anid, int gid, double 
		withinrange);
	int	cancelApproachingNotification(int nid, int registrationID);
	int	suspendApproachingNotification(int nid, int registrationID,
		int timeIntervalInMilliseconds);
	int	resumeApproachingNotification(int nid, int registrationID);
	int	checkIPCmessages();
	int	checkIPCmessagesNodeID(u_int32_t nid, int agent_socket_fd, int pid);
	int	changeNodeSpeedFromGUI(int nid, double speed);
	int	changeNodeMovingDirectionFromGUI(int nid, double angle);
	int	checkRegisteredRequests();
	void	closeSpecifiedClientPID(int pid);
	void	closeAllClientSocket();
	void	moduleCreateRegInfo(u_int32_t nid, u_int16_t port/*, NslObject *wme_pointer*/);
	void	agentEnableRegInfo(int fd, u_int32_t nid, pid_t pid);
	int 	recvWSMtoAgent(u_int32_t nid, struct WSM_Header wsm_header, char *wsm_data);
	int 	sendWSMtoModule(u_int32_t nid, char *mesg);
/*
 * For Large Scale Simulation
 */
	void	sendResultToNode(char* msg, int result);
	int	communicateLargeScaleCar(char* msg, char* longMsg);
	int	initLSCar(char* msg);
};                                                                                                                             
#endif
