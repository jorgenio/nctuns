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

#include <arpa/inet.h>
#include <assert.h> 
#include <errno.h>
#include <math.h>
#include <netdb.h>
#include <netinet/tcp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <object.h>
#include <nctuns-dep/node.h>			// for Node class
#include <sock_skel.h>				// for socket connection

#include <agent.h>				// for agent and agent group
#include <config.h> 				// for MAX_NODES
#include <dispatcher.h>
#include <gbind.h>
#include <command_server.h>
#include <auto_vehicle_signal.h>
#include <sock_skel.h>
#include <scheduler.h>
#include <nctuns_api.h>
#include <math_fun.h>
#include <IPC/ns.h>
/* The following cases are added by automatic vehicle group, 2006.9.27 */
#include <vector>
#include <80211p/wme/wme.h>

using namespace std;
TrafficLightCenter signalList;
vector<TrafficGroupInfo> TrafficGroupInfoList;

extern scheduler 		*scheduler_;
extern GROUPAGENT_VECTOR        g_vGroupAgent;
extern CAgent*                  g_pAgentList[MAX_NUM_NODE + 1];
extern unsigned long            g_nNumAgents;

extern logpack *logpack_;	//for GUI
int maxSockFd = 0;

/* for group */
#define MAX_TEAM 10
#define MAX_MEMBER 10
extern int Team_lst[MAX_TEAM][MAX_MEMBER];

cmd_server *cmd_server_ = NULL;

int checkIPCmessageAndRegisteredRequests(Event *ep) {
	cmd_server_->checkIPCmessages();
	cmd_server_->checkRegisteredRequests();
	scheduleCheckIPCCommandFds();
	freeEvent(ep);
	return(1);
}

void scheduleCheckIPCCommandFds() {
	u_int64_t       timeTick ;
	Event_ *ep;

	ep = createEvent();
	MILLI_TO_TICK(timeTick, CHECK_IPC_COMMAND_INTERVAL);
	timeTick += GetCurrentTime();
	setFuncEvent(ep, timeTick, 0, checkIPCmessageAndRegisteredRequests, 
			NULL);
}

cmd_server::cmd_server() {
	/*
	 * initial all data array
	 */
	for(int i = 0; i< (MAX_NUM_NODE + 1) ; i++)
		nid_info[i] = NULL;

	GROUP_INIT();

	mudpsock = passiveUDP(SE_IPC_SERVER_UDP_PORT_FOR_AGENT_NOTIFICATION);
	msock = passiveTCP(SE_IPC_SERVER_PORT_FOR_AGENT, MAX_NUM_NODE);	
	if (msock < 0) {
		printf("Creating the IPC server TCP port failed\n");
		exit(0);
	}
	printf("Creating the IPC server TCP port succeeded, port num %s\n", SE_IPC_SERVER_PORT_FOR_AGENT);

	FD_ZERO(&afds);
	FD_SET(msock, &afds);
	nfds = msock + 1;
	m_mapTriggerMap.clear();
	g_nNumAgents = 0;
	cmd_serv_reg_info_first = cmd_serv_reg_info_end = NULL;
}

cmd_server::~cmd_server() {
	struct CmdServRegInfo *tmp;
	tmp = cmd_serv_reg_info_first;
	while(tmp != NULL)
	{
		cmd_serv_reg_info_first = cmd_serv_reg_info_first->next;
		delete tmp;
		tmp = cmd_serv_reg_info_first;
	}
}

int cmd_server::ReadSignalFile()
{
	int i;
	FILE *fdp;
	char FILEPATH[200];

	TrafficGroupInfoList.clear();
	sprintf(FILEPATH, "%s.sig", GetScriptName());
	if ((fdp = fopen(FILEPATH, "r")) == NULL) {
		printf("Warning: can't open file %s\n", FILEPATH);
	}
	else {
		i = read_signals(fdp,  signalList, TrafficGroupInfoList);
		if (i < 0)
			printf("Error: read_signals fail !!\n");
		else
			printf("Open signal file %s success\n", FILEPATH);
	}
	fclose(fdp);
	return (1);
}

/*
 * This function must be executed ahead of all other agent-related code 
 * because the corresponding agent class must be created first.
 */

int cmd_server::initAgentClient(char *msg, int fd) {
	struct agentClientInfo *p;
	int pid, nid = -1;
	int humanControlled;
	nid_information *tmp_nid_info;

	//printf("Enter cmd_server::initAgentClient()\n");
	p = (struct agentClientInfo *) msg;

	nid = p->nid;
	pid = p->pid;
	humanControlled = p->humanControlled;

	//printf("cmd_server::initAgentClient() nid %d pid %d humanControlled %d\n", nid, pid, humanControlled);

	tmp_nid_info = new nid_information();
	tmp_nid_info->init(pid, fd, p->socket_fd, p->udpportnum, p->process_type);
	if(nid_info[nid] == NULL)
		nid_info[nid] = tmp_nid_info;
	else
		nid_info[nid]->next = tmp_nid_info;

	// generate the corresponding agent class	
	g_pAgentList[nid] = new CAgent(nid);

	// Set whether or not this mobile node is manually controlled by a 
	// human or by an agent program.
	//setAgentHumanOrProgramControlled(nid, humanControlled);

	GROUP_FIND(GROUP_ALL)->AddAgent(g_pAgentList[nid]);
	if (p->group1 == 1) {
		GROUP_FIND(GROUP_1)->AddAgent(g_pAgentList[nid]);
	} else if (p->group2 == 1) {
		GROUP_FIND(GROUP_2)->AddAgent(g_pAgentList[nid]);
	} else if (p->group3 == 1) {
		GROUP_FIND(GROUP_3)->AddAgent(g_pAgentList[nid]);
	} else if (p->group4 == 1) {
		GROUP_FIND(GROUP_4)->AddAgent(g_pAgentList[nid]);
	} else if (p->group5 == 1) {
		GROUP_FIND(GROUP_5)->AddAgent(g_pAgentList[nid]);
	} else if (p->group6 == 1) {
		GROUP_FIND(GROUP_6)->AddAgent(g_pAgentList[nid]);
	} else if (p->group7 == 1) {
		GROUP_FIND(GROUP_7)->AddAgent(g_pAgentList[nid]);
	} else if (p->group8 == 1) {
		GROUP_FIND(GROUP_8)->AddAgent(g_pAgentList[nid]);
	}
	g_nNumAgents++;

	return nid;
}

int cmd_server::initLSCar(char *msg)
{
	struct LSCarInfo *p;
	int nid = -1;
	nid_information *tmp_nid_info;

	p = (struct LSCarInfo *) msg;

	nid = p->nid;
	//printf("cmd_server::%s() nid %d\n", __func__, nid);

	tmp_nid_info = new nid_information();
	tmp_nid_info->init(-1, -1, -1, -1, PROCESS_TYPE_AGENT);
	if(nid_info[nid] == NULL)
		nid_info[nid] = tmp_nid_info;
	else
		nid_info[nid]->next = tmp_nid_info;

	// generate the corresponding agent class
	g_pAgentList[nid] = new CAgent(nid);

	// Set whether or not this mobile node is manually controlled by a
	// human or by an agent program.
	//setAgentHumanOrProgramControlled(nid, humanControlled);

	GROUP_FIND(GROUP_ALL)->AddAgent(g_pAgentList[nid]);
	if (p->group1 == 1) {
		GROUP_FIND(GROUP_1)->AddAgent(g_pAgentList[nid]);
	} else if (p->group2 == 1) {
		GROUP_FIND(GROUP_2)->AddAgent(g_pAgentList[nid]);
	} else if (p->group3 == 1) {
		GROUP_FIND(GROUP_3)->AddAgent(g_pAgentList[nid]);
	} else if (p->group4 == 1) {
		GROUP_FIND(GROUP_4)->AddAgent(g_pAgentList[nid]);
	} else if (p->group5 == 1) {
		GROUP_FIND(GROUP_5)->AddAgent(g_pAgentList[nid]);
	} else if (p->group6 == 1) {
		GROUP_FIND(GROUP_6)->AddAgent(g_pAgentList[nid]);
	} else if (p->group7 == 1) {
		GROUP_FIND(GROUP_7)->AddAgent(g_pAgentList[nid]);
	} else if (p->group8 == 1) {
		GROUP_FIND(GROUP_8)->AddAgent(g_pAgentList[nid]);
	}
	g_nNumAgents++;
	return(nid);
}

int cmd_server::associateAgentClient(int fd) {
	char                    *msg;
	int                     n;
	int                     nid;
	struct GeneralACKBetweenAgentClientAndSimulationEngine ack;

	//printf("Enter cmd_server::associateAgentClient()\n");
	msg = (char*) malloc(sizeof(char) * MAX_RECV_BYTES);
	n = readn(fd, msg, sizeof (struct agentClientInfo));
	nid = initAgentClient(msg, fd);
	ack.type = GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE; 
	ack.moreMsgFollowing = 0;
	n = writen(fd, (char *) &ack, 
			sizeof(GeneralACKBetweenAgentClientAndSimulationEngine));
	free(msg);
	if (((struct agentClientInfo *) msg)->moreMsgFollowing > 0) {

		char* msg1 = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
		char* longMsg = (char *) malloc(IP_PACKET_MAX_LEN);
		communicateAgentClient(fd,msg1,longMsg);
		free(msg1);
		free(longMsg);
	}
	return(0);
}

int cmd_server::sendBackACKToAgentClient(int fd, int result) {
	struct GeneralACKBetweenAgentClientAndSimulationEngine ack;

	ack.type = GENERAL_ACK_BETWEEN_AGENT_CLIENT_AND_SIMULATION_ENGINE;
	ack.result = result; // 0: success, -1: failure
	ack.moreMsgFollowing = 0;

	writen(fd, (char *) &ack, sizeof(struct 
				GeneralACKBetweenAgentClientAndSimulationEngine));
	return(1);
}

int cmd_server::communicateLargeScaleCar(char* msg, char* longMsg) {

	int	nid;
	struct typeChecker q;

	if (0) printf("Enter cmd_server::communicateLargeScaleCar()\n");

	memcpy(&q, msg, sizeof(struct typeChecker));

	switch (q.type) {

		case AGENT_CLIENT_STOP_SIMULATION:
			nid = ((struct agentClientStopSimulation*) msg)->nid;
#if IPC
			SimulationDown();
#endif
			closeAllClientSocket();
			exit(0);
			break;
	}
	return 1;
}

int cmd_server::communicateAgentClient(int fd, char* msg, char* longMsg) {

	struct typeChecker p;
	int     j, n, nid, anid, gid, numNode, moreMsgFollowing, result, r,
		registrationID, timeInterval, timeIntervalInMilliseconds;
	int	sigGID, light, signalIndex, numOfSignals;
	double 	x, y, z, speed, angle, withinRangeinMeter, distance;
	double	acceleration, maxSpeed, CurrentPOS_x, CurrentPOS_y;
	GROUPMEMBER_VECTOR* gv;
	struct nodePosition *npp;
	u_int64_t time_tick;
	int TeamID, TeamLeader, Preceding_Nid, Following_Nid, Turning_block, Turning_times, EnableLaneChange; //add by icchen
	double Turning_direction;

	if (0) printf("Enter cmd_server::communicateAgentClient()\n");

recheck:

	if (0) printf("Entering recheck \n");
	SEC_TO_TICK(time_tick, 1);

	if ((scheduler_->maxsimtime() - GetCurrentTime()) < time_tick) {
		// The simulation is almost finished. Now we do a graceful
		// shutdown to avoid deadlock between agent programs and this
		// IPC command server.
		printf("Preprare a graceful shutdown\n");
		return(0);
	}
	if ((n = recv(fd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK)) == 0) {
		close(fd);
		FD_CLR(fd, &afds);
	}

	switch (p.type) {

		case AGENT_CLIENT_SET_CURRENT_WAYPOINT: 

			n = readn(fd, msg, sizeof(struct agentClientSetCurrentWayPoint));
			moreMsgFollowing = ((struct agentClientSetCurrentWayPoint *) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetCurrentWayPoint *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_CURRENT_WAYPOINT\n", nid);
			x = ((struct agentClientSetCurrentWayPoint *) msg)->x;
			y = ((struct agentClientSetCurrentWayPoint *) msg)->y;
			z = ((struct agentClientSetCurrentWayPoint *) msg)->z;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {

				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->setNodeCurrentWaypoint(x, y, z);
					sendBackACKToAgentClient(fd, 0);
				} 
				else sendBackACKToAgentClient(fd, -1);

			} 
			else sendBackACKToAgentClient(fd, -1);

			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_GET_CURRENT_WAYPOINT: 

			n = readn(fd, msg, sizeof(struct agentClientGetCurrentWayPoint));
			moreMsgFollowing = ((struct agentClientGetCurrentWayPoint *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetCurrentWayPoint *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_GET_CURRENT_WAYPOINT:\n", nid);
			((struct agentClientGetCurrentWayPointReply *) msg)->type = AGENT_CLIENT_GET_CURRENT_WAYPOINT_REPLY;
			x = -1;	y = -1; z = -1; result = -1;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNodeCurrentWaypoint(x, y, z);
					result = 0;
				} 
			} 
			((struct agentClientGetCurrentWayPointReply *) msg)->result = 
				result;
			((struct agentClientGetCurrentWayPointReply *) msg)->x = x;
			((struct agentClientGetCurrentWayPointReply *) msg)->y = y;
			((struct agentClientGetCurrentWayPointReply *) msg)->z = z;
			writen(fd, msg, sizeof(struct agentClientGetCurrentWayPointReply));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_SET_NEXT_WAYPOINT: 
			n = readn(fd, msg, sizeof(struct agentClientSetNextWayPoint));
			moreMsgFollowing = ((struct agentClientSetNextWayPoint *) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetNextWayPoint *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_NEXT_WAYPOINT::\n", nid);
			x = ((struct agentClientSetNextWayPoint *) msg)->x;
			y = ((struct agentClientSetNextWayPoint *) msg)->y;
			z = ((struct agentClientSetNextWayPoint *) msg)->z;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->setNodeNextWaypoint(x, y, z);
					sendBackACKToAgentClient(fd, 0);
				} else sendBackACKToAgentClient(fd, -1);
			} else sendBackACKToAgentClient(fd, -1);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION:

			n = readn(fd, msg, sizeof(struct agentClientGetCurrentMovingDirection));
			moreMsgFollowing = ((struct agentClientGetCurrentMovingDirection *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetCurrentMovingDirection *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION\n", nid);
			angle = -1;
			result = -1;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNodeAngle(angle);
					result = 0;
				}
			}
			((struct agentClientGetCurrentMovingDirectionReply *) msg)->type =
				AGENT_CLIENT_GET_CURRENT_MOVING_DIRECTION_REPLY;
			((struct agentClientGetCurrentMovingDirectionReply *) msg)->result = result;
			((struct agentClientGetCurrentMovingDirectionReply *) msg)->angle = angle;
			writen(fd, msg, sizeof(struct agentClientGetCurrentMovingDirectionReply));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_SET_CURRENT_AND_NEXT_WAYPOINT: 

			n = readn(fd, msg, sizeof(struct agentClientSetCurrentAndNextWayPoint));
			moreMsgFollowing = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_CURRENT_AND_NEXT_WAYPOINT\n", nid);
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					x = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->curx;
					y = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->cury;
					z = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->curz;
					((Node*) nodelist[nid])->setNodeCurrentWaypoint(x, y, z);
					x = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nextx;
					y = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nexty;
					z = ((struct agentClientSetCurrentAndNextWayPoint *) msg)->nextz;
					((Node*) nodelist[nid])->setNodeNextWaypoint(x, y, z);
					sendBackACKToAgentClient(fd, 0);
				} 
				else sendBackACKToAgentClient(fd, -1);
			} 
			else sendBackACKToAgentClient(fd, -1);

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;
		case AGENT_CLIENT_SET_CURRENT_MOVING_SPEED: 

			n = readn(fd, msg, sizeof(struct agentClientSetCurrentMovingSpeed));
			moreMsgFollowing = ((struct agentClientSetCurrentMovingSpeed *) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetCurrentMovingSpeed *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_CURRENT_MOVING_SPEED\n", nid);
			speed = ((struct agentClientSetCurrentMovingSpeed *) msg)->speed;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->setNodeSpeed(speed);
					sendBackACKToAgentClient(fd, 0);
				} 
				else sendBackACKToAgentClient(fd, -1);
			} 
			else sendBackACKToAgentClient(fd, -1);

			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_GET_CURRENT_MOVING_SPEED: 

			n = readn(fd, msg, sizeof(struct agentClientGetCurrentMovingSpeed));
			moreMsgFollowing = ((struct agentClientGetCurrentMovingSpeed *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetCurrentMovingSpeed *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_GET_CURRENT_MOVING_SPEED\n", nid);
			speed = -1; 
			result = -1;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNodeSpeed(speed);
					result = 0;
				}
			}
			((struct agentClientGetCurrentMovingSpeedReply *) msg)->type = AGENT_CLIENT_GET_CURRENT_MOVING_SPEED_REPLY;
			((struct agentClientGetCurrentMovingSpeedReply *) msg)->result = result;
			((struct agentClientGetCurrentMovingSpeedReply *) msg)->speed = speed;
			writen(fd, msg, sizeof(struct agentClientGetCurrentMovingSpeedReply));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_SET_NEXT_WAYPOINT_AND_MOVING_SPEED:

			n = readn(fd, msg, sizeof(struct
						agentClientSetNextWayPointAndMovingSpeed));

			moreMsgFollowing = ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_NEXT_WAYPOINT_AND_MOVING_SPEED\n", nid);
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					x = ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nextx;
					y = ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nexty;
					z = ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->nextz;
					((Node*) nodelist[nid])->setNodeNextWaypoint(x, y, z);
					speed = ((struct agentClientSetNextWayPointAndMovingSpeed *) msg)->speed;
					((Node*) nodelist[nid])->setNodeSpeed(speed);
					sendBackACKToAgentClient(fd, 0);
				} else sendBackACKToAgentClient(fd, -1);
			} 
			else sendBackACKToAgentClient(fd, -1);

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;
		case AGENT_CLIENT_GET_CURRENT_POSITION:

			n = readn(fd, msg, sizeof(struct agentClientGetCurrentPosition));
			moreMsgFollowing = ((struct agentClientGetCurrentPosition *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetCurrentPosition *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_GET_CURRENT_POSITION\n", nid);
			x = -1; y = -1; z = -1;
			result = -1;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNodePosition(x, y, z);
					result = 0;
				}
			}
			((struct agentClientGetCurrentPositionReply *) msg)->type = AGENT_CLIENT_GET_CURRENT_POSITION_REPLY;
			((struct agentClientGetCurrentPositionReply *) msg)->result = result;
			((struct agentClientGetCurrentPositionReply *) msg)->x = x;
			((struct agentClientGetCurrentPositionReply *) msg)->y = y;
			((struct agentClientGetCurrentPositionReply *) msg)->z = z;
			n = writen(fd, msg, sizeof(struct agentClientGetCurrentPositionReply));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE: 

			n = readn(fd, msg, sizeof(struct agentClientGetCurrentPositionOfAGroupOfNode));
			moreMsgFollowing = ((struct agentClientGetCurrentPositionOfAGroupOfNode *) msg)->moreMsgFollowing; 
			gid = ((struct agentClientGetCurrentPositionOfAGroupOfNode *) msg)->gid;
			switch (gid) {
				case 1:
					gv = GROUP_FIND(GROUP_1)->GetMember();
					break;
				case 2:
					gv = GROUP_FIND(GROUP_2)->GetMember();
					break;
				case 3:
					gv = GROUP_FIND(GROUP_3)->GetMember();
					break;
				case 4:
					gv = GROUP_FIND(GROUP_4)->GetMember();
					break;
				case 5:
					gv = GROUP_FIND(GROUP_5)->GetMember();
					break;
				case 6:
					gv = GROUP_FIND(GROUP_6)->GetMember();
					break;
				case 7:
					gv = GROUP_FIND(GROUP_7)->GetMember();
					break;
				case 8:
					gv = GROUP_FIND(GROUP_8)->GetMember();
					break;
				default:
					gv = NULL;
			}
			((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)->type = 
				AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_REPLY;
			if (gv == NULL) { // Arguments nid or gid is wrong 
				result = -1;
				numNode = -1;
				((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)->result = result;
				((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)->numNode = numNode;
				writen(fd, longMsg, sizeof(struct agentClientGetCurrentPositionOfAGroupOfNodeReply));
			} 
			else {

				result = 0;
				numNode = static_cast<int>(gv->size());
				if (((unsigned int) numNode) > ((IP_PACKET_MAX_LEN/sizeof(struct nodePosition)) - 1)) {
					numNode = (IP_PACKET_MAX_LEN/sizeof(struct nodePosition)) - 1;
				}

				((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)->result = result;
				((struct agentClientGetCurrentPositionOfAGroupOfNodeReply *) longMsg)->numNode = numNode;

				npp = (struct nodePosition *) 
					((char *) longMsg + sizeof (struct agentClientGetCurrentPositionOfAGroupOfNodeReply));
				for (j = 0; j < numNode; j++){
					nid = ((*gv)[j])->GetID();
					if(nid_info[nid]->is_agent_up() == 1){	
						((Node*) nodelist[nid])->getNodePosition(x, y, z);
						npp->nid = nid;
						npp->x = x;
						npp->y = y;
						npp->z = z;
						npp++;
					}
					else{
						printf("[Command Server] Warning: agent %d is_up == 0\n", nid);
						npp->nid = nid;
						npp->x = 0;
						npp->y = 0;
						npp->z = 0;
						npp++;
					}
				}
				writen(fd, longMsg, 
						sizeof(struct agentClientGetCurrentPositionOfAGroupOfNodeReply) + numNode * 
						sizeof(struct nodePosition));
			}
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;
		case AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_IN_A_DISTANCE: 

			n = readn(fd, msg, sizeof(struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance));
			moreMsgFollowing = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->moreMsgFollowing; 
			gid = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->gid;
			distance = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->distance;
			CurrentPOS_x = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->CurrentPOS_x;
			CurrentPOS_y = ((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistance *) msg)->CurrentPOS_y;
			switch (gid) {
				case 1:
					gv = GROUP_FIND(GROUP_1)->GetMember();
					break;
				case 2:
					gv = GROUP_FIND(GROUP_2)->GetMember();
					break;
				case 3:
					gv = GROUP_FIND(GROUP_3)->GetMember();
					break;
				case 4:
					gv = GROUP_FIND(GROUP_4)->GetMember();
					break;
				case 5:
					gv = GROUP_FIND(GROUP_5)->GetMember();
					break;
				case 6:
					gv = GROUP_FIND(GROUP_6)->GetMember();
					break;
				case 7:
					gv = GROUP_FIND(GROUP_7)->GetMember();
					break;
				case 8:
					gv = GROUP_FIND(GROUP_8)->GetMember();
					break;
				default:
					gv = NULL;
			}
			((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)->type = 
				AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_GROUP_OF_NODE_IN_A_DISTANCE_REPLY;
			if (gv == NULL) { // Arguments nid or gid is wrong 
				result = -1;
				numNode = -1;
				((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)->result = result;
				((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)->numNode = numNode;
				writen(fd, longMsg, sizeof(struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply));
			} 
			else {

				result = 0;
				numNode = static_cast<int>(gv->size());
				if (((unsigned int) numNode) > ((IP_PACKET_MAX_LEN/sizeof(struct nodePosition)) - 1)) {
					numNode = (IP_PACKET_MAX_LEN/sizeof(struct nodePosition)) - 1;
				}

				((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)->result = result;
				((struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply *) longMsg)->numNode = numNode;

				npp = (struct nodePosition *) 
					((char *) longMsg + sizeof (struct agentClientGetCurrentPositionOfAGroupOfNodeInADistanceReply));
				for (j = 0; j < numNode; j++){
					nid = ((*gv)[j])->GetID();
					if(nid_info[nid]->is_agent_up() == 1){					
						((Node*) nodelist[nid])->getNodePosition(x, y, z);
						if(Distance_BetweenTwoNode(x, y, CurrentPOS_x, CurrentPOS_y)< distance);
						{
							npp->nid = nid;
							npp->x = x;
							npp->y = y;
							npp->z = z;
							npp++;
						}
					}
					else{
						printf("[Command Server] Warning: agent %d is_up == 0\n", nid);
						npp->nid = nid;
						npp->x = 0;
						npp->y = 0;
						npp->z = 0;
						npp++;		
					}
				}
				writen(fd, longMsg, 
						sizeof(struct agentClientGetCurrentPositionOfAGroupOfNodeReply) + numNode * 
						sizeof(struct nodePosition));
			}
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;
		case AGENT_CLIENT_REQUEST_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME: 
			n = readn(fd, msg, sizeof(struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe));
			moreMsgFollowing = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->moreMsgFollowing;
			nid = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_REQUEST_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME\n", nid);

			registrationID = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->registrationID;
			timeIntervalInMilliseconds = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->timeIntervalInMilliseconds;
			anid = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->anid;
			gid = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->gid;
			withinRangeinMeter = ((struct agentClientRequestNotificationWhenAnotherNodeHasApproachedMe *) msg)->withinRangeinMeter;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					registerApproachingNotification(nid, registrationID, timeIntervalInMilliseconds, anid, gid, 
							withinRangeinMeter);
					sendBackACKToAgentClient(fd, 0);
				} 
				else sendBackACKToAgentClient(fd, -1);
			} 
			else sendBackACKToAgentClient(fd, -1);

			if (moreMsgFollowing != 0) {
				goto recheck;
			}

			break;

		case AGENT_CLIENT_CANCEL_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME: 

			n = readn(fd, msg, sizeof(struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe));
			moreMsgFollowing = ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->moreMsgFollowing;
			nid = ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_CANCEL_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME\n", nid);
			registrationID = ((struct agentClientCancelNotificationWhenAnotherNodeHasApproachedMe *) msg)->registrationID;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					cancelApproachingNotification(nid, registrationID);
					sendBackACKToAgentClient(fd, 0);
				} 
				else sendBackACKToAgentClient(fd, -1);
			} 
			else sendBackACKToAgentClient(fd, -1);

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_SUSPEND_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME: 

			n = readn(fd, msg, sizeof(struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe));

			moreMsgFollowing = ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->moreMsgFollowing;

			nid = ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SUSPEND_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME\n", nid);
			registrationID = ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->registrationID;

			timeInterval = ((struct agentClientSuspendNotificationWhenAnotherNodeHasApproachedMe *) msg)->timeInterval;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					suspendApproachingNotification(nid, registrationID, timeInterval);
					sendBackACKToAgentClient(fd, 0);
				} else sendBackACKToAgentClient(fd, -1);
			} else sendBackACKToAgentClient(fd, -1);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_RESUME_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME:

			n = readn(fd, msg, sizeof(struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe));
			moreMsgFollowing = ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) 
					msg)->moreMsgFollowing;
			nid = ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_RESUME_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME\n", nid);
			registrationID = ((struct agentClientResumeNotificationWhenAnotherNodeHasApproachedMe *) msg)->registrationID;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					resumeApproachingNotification(nid, registrationID);
					sendBackACKToAgentClient(fd, 0);
				} else sendBackACKToAgentClient(fd, -1);
			} else sendBackACKToAgentClient(fd, -1);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_DESTROY_MOBILE_NODE: 
			n = readn(fd, msg, sizeof(struct agentClientDestroyMobileNode));
			moreMsgFollowing = ((struct agentClientDestroyMobileNode *) msg)->moreMsgFollowing;
			nid = ((struct agentClientDestroyMobileNode*) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_DESTROY_MOBILE_NODE\n", nid);
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				nid_info[nid]->set_up(fd, 0);
				sendBackACKToAgentClient(fd, 0);
			} 
			else sendBackACKToAgentClient(fd, -1);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_STOP_SIMULATION:
			n = readn(fd, msg, sizeof(struct agentClientStopSimulation));
			nid = ((struct agentClientStopSimulation*) msg)->nid;
#if IPC
			SimulationDown();
#endif
			closeAllClientSocket();
			exit(0);
			break;

		case AGENT_CLIENT_RELEASE_CPU:

			n = readn(fd, msg, sizeof(struct agentClientReleaseCPU));
			nid = ((struct agentClientReleaseCPU *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_RELEASE_CPU\n", nid);
			usleep((10000/g_nNumAgents));
			scheduler_->chkt0e();
			break;

			/* The following cases are added by student Ming-Sheng Hsu , 2006.9.27 */
		case AGENT_CLIENT_GET_SIGNAL_GROUPID:
			n = readn(fd, msg, sizeof(struct agentClientGetSignalGroupID));
			moreMsgFollowing = ((struct agentClientGetSignalGroupID *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetSignalGroupID *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_GET_SIGNAL_GROUPID\n", nid);
			r = getAnUnusedSignalGID(TrafficGroupInfoList, sigGID, numOfSignals);
			if(r > 0)
				result = 0; // success
			else{
				printf("nid %d receive AGENT_CLIENT_GET_SIGNAL_GROUPID fail !!\n", nid);
				result = -1; // fail
			}
			((struct agentClientGetSignalGroupIDReply *) msg)->type =
				AGENT_CLIENT_GET_SIGNAL_GROUPID_REPLY;
			((struct agentClientGetSignalGroupIDReply *) msg)->result = result;
			((struct agentClientGetSignalGroupIDReply *) msg)->sigGID = sigGID;
			((struct agentClientGetSignalGroupIDReply *) msg)->numOfSigs = numOfSignals;
			writen(fd, msg, sizeof(struct agentClientGetSignalGroupIDReply));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_SET_CURRENT_SPEED_ACCELERATION:
			n = readn(fd, msg, sizeof(struct agentClientSetCurrentSpeedAcceleration));
			moreMsgFollowing = ((struct agentClientSetCurrentSpeedAcceleration *) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetCurrentSpeedAcceleration*) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_CURRENT_SPEED_ACCELERATION\n", nid);
			acceleration = ((struct agentClientSetCurrentSpeedAcceleration *) msg)->acceleration;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->setNodeSpeedAcceleration(acceleration);
					sendBackACKToAgentClient(fd, 0); 
				} else {
					sendBackACKToAgentClient(fd, -1);
				}
			} else sendBackACKToAgentClient(fd, -1);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_SET_CURRENT_MOVING_DIRECTION:
			n = readn(fd, msg, sizeof(struct agentClientSetCurrentMovingDirection));
			moreMsgFollowing = ((struct agentClientSetCurrentMovingDirection*) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetCurrentMovingDirection*) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_SET_CURRENT_MOVING_DIRECTION\n", nid);
			angle = ((struct agentClientSetCurrentMovingDirection *) msg)->angle;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->setNodeAngle(angle);
					sendBackACKToAgentClient(fd, 0);
				} else sendBackACKToAgentClient(fd, -1);
			} else sendBackACKToAgentClient(fd, -1);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_NODE_MAX_SPEED:
			n = readn(fd, msg, sizeof(struct agentClientGetNodeMaxSpeed));
			moreMsgFollowing = ((struct agentClientGetNodeMaxSpeed *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetNodeMaxSpeed *) msg)->nid;
			//printf("nid %d receive AGENT_CLIENT_GET_NODE_MAX_SPEED\n", nid);
			maxSpeed = 0.0;
			result = -1;
			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNodeMaxSpeed(maxSpeed);
					result = 0;
				}
			}
			((struct agentClientGetNodeMaxSpeedReply *) msg)->type =
				AGENT_CLIENT_GET_NODE_MAX_SPEED_REPLY;
			((struct agentClientGetNodeMaxSpeedReply *) msg)->result = result;
			((struct agentClientGetNodeMaxSpeedReply *) msg)->maxSpeed = maxSpeed;
			writen(fd, msg, sizeof(struct agentClientGetNodeMaxSpeedReply));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_SET_SIGNAL_LIGHT:
			n = readn(fd, msg, sizeof(struct agentClientSetSignalLight));
			moreMsgFollowing = ((struct agentClientSetSignalLight*) msg)->moreMsgFollowing;
			nid = ((struct agentClientSetSignalLight*) msg)->nid;
			sigGID = ((struct agentClientSetSignalLight*) msg)->sigGID;
			signalIndex = ((struct agentClientSetSignalLight*) msg)->signalIndex;
			light = ((struct agentClientSetSignalLight*) msg)->light;
			n = setSignalLight(signalList, sigGID, signalIndex, light);
			if(n == 1)
			{
				logpack_->logLight(nid, (signalList.trafficLight[sigGID]).sigGID, light, signalList.trafficLight[sigGID].x[signalIndex], signalList.trafficLight[sigGID].y[signalIndex], GetCurrentTime());
				sendBackACKToAgentClient(fd, 0); 
			}
			else
				sendBackACKToAgentClient(fd, -1); 
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP:
			agentClientGetSignalsInTheSameGroupReply tempG;

			n = readn(fd, msg, sizeof(struct agentClientGetSignalsInTheSameGroup));	
			moreMsgFollowing = ((struct  agentClientGetSignalsInTheSameGroup *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetSignalsInTheSameGroup *) msg)->nid;
			sigGID = ((struct agentClientGetSignalsInTheSameGroup *) msg)-> sigGID;
			//printf("nid %d receive AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP\n", nid);

			result = getSignalsInTheSameGroup(signalList, sigGID, &tempG);

			((struct agentClientGetSignalsInTheSameGroupReply *) msg)->type =
				AGENT_CLIENT_GET_SIGNALS_IN_THE_SAME_GROUP_REPLY;
			((struct agentClientGetSignalsInTheSameGroupReply *) msg)->result = result;
			((struct agentClientGetSignalsInTheSameGroupReply *) msg)->numOfSigs = tempG.numOfSigs;
			if(result == 1)
			{
				int i = 0;

				((struct agentClientGetSignalsInTheSameGroupReply *) msg)->sigGID = tempG.sigGID;
				((struct agentClientGetSignalsInTheSameGroupReply *) msg)->signalType
					= tempG.signalType;

				for(i = 0; i < tempG.numOfSigs; i++){
					((struct agentClientGetSignalsInTheSameGroupReply *) msg)->light[i]
						= tempG.light[i];
					((struct agentClientGetSignalsInTheSameGroupReply *) msg)->x[i]
						= tempG.x[i];
					((struct agentClientGetSignalsInTheSameGroupReply *) msg)->y[i]
						= tempG.y[i];
					((struct agentClientGetSignalsInTheSameGroupReply *) msg)->facingDirection[i]
						= tempG.facingDirection[i];
				}
			}
			else
			{
				printf("Engine: getSignalsInTheSameGroup failed !! \n");
			}
			writen(fd, msg, sizeof(struct agentClientGetSignalsInTheSameGroupReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_SIGNAL_LIGHT:
			n = readn(fd, msg, sizeof(struct agentClientGetSignalLight));	
			moreMsgFollowing = ((struct agentClientGetSignalLight *) msg)->moreMsgFollowing;
			nid = ((struct agentClientGetSignalLight *) msg)->nid;
			signalIndex = -1;

			result = getTheNearestPrecedingTrafficLight(signalList,
					((struct agentClientGetSignalLight *) msg)-> x,
					((struct agentClientGetSignalLight *) msg)-> y,
					((struct agentClientGetSignalLight *) msg)-> myDirection,
					((struct agentClientGetSignalLight *) msg)-> distance,
					((struct agentClientGetSignalLight *) msg)-> sigGID,
					light, x, y, signalIndex
					);

			((struct agentClientGetSignalLightReply *) msg)->type =
				AGENT_CLIENT_GET_SIGNAL_LIGHT_REPLY;

			((struct  agentClientGetSignalLightReply *) msg)->result = result;
			((struct agentClientGetSignalLightReply *) msg)-> light = light;
			((struct agentClientGetSignalLightReply *) msg)-> sig_x = x;
			((struct agentClientGetSignalLightReply *) msg)-> sig_y = y;
			((struct agentClientGetSignalLightReply *) msg)-> signalIndex = signalIndex;

			writen(fd, msg, sizeof(struct agentClientGetSignalLightReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case WAVE_SHORT_MESSAGE_PROTOCOL:		//recv WSM from user process
			//recv heaer
			n = readn(fd, msg, sizeof(struct WaveShortMessageProtocol));
			moreMsgFollowing = ((struct WaveShortMessageProtocol*) msg)->moreMsgFollowing;
			nid = ((struct WaveShortMessageProtocol*) msg)->nid;
			//recv WSM_DATA
			n += readn(fd, &msg[n], ((struct WaveShortMessageProtocol*) msg)->wsm_header.wsm_len);
			//printf("se recv <%d> bytes message from nid=%d\n", n, ((struct WaveShortMessageProtocol*) msg)->nid);
			sendWSMtoModule(((struct WaveShortMessageProtocol*) msg)->nid, msg);	//12-> module need WSM_Header in struct WaveShortMessageProtocol but not need other in WaveShortMessageProtocol
			//send data to module
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case WAVE_AGENT_ENABLE_REG_INFO:
			n = readn(fd, msg, sizeof(struct WaveAgentEnableRegInfo));
			moreMsgFollowing = ((struct WaveAgentEnableRegInfo*) msg)->moreMsgFollowing;
			nid = ((struct WaveAgentEnableRegInfo*) msg)->nid;
			agentEnableRegInfo(fd, ((struct WaveAgentEnableRegInfo*) msg)->nid, ((struct WaveAgentEnableRegInfo*) msg)->pid);
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case GET_SE_TIME:

			n = readn(fd, msg, sizeof(struct GetSETime));

			nid = ((struct GetSETime*) msg)->nid;
			moreMsgFollowing = ((struct GetSETime*) msg)->moreMsgFollowing;
			((struct GetSETime*) msg)->time =  GetCurrentTime();
			writen(fd, msg, sizeof(struct GetSETime));
			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_TEAM_INFO:

			n=read(fd,msg,sizeof(struct agentClientGetTeamInfo));
			nid = ((struct agentClientGetTeamInfo*)msg)->nid;
			moreMsgFollowing = ((struct agentClientGetTeamInfo*) msg)->moreMsgFollowing;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getTeamInfo(TeamID,TeamLeader);
					result = 0;
				}
			}
			((struct agentClientGetTeamInfoReply *) msg)->type = AGENT_CLIENT_GET_TEAM_INFO_REPLY;
			((struct agentClientGetTeamInfoReply *) msg)->result = result;
			((struct agentClientGetTeamInfoReply *) msg)->TeamID = TeamID;
			((struct agentClientGetTeamInfoReply *) msg)->TeamLeader = TeamLeader;
			writen(fd, msg, sizeof(struct agentClientGetTeamInfoReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER:

			n=read(fd,msg,sizeof(struct agentClientGetNearestPrecedingTeamMember));
			nid = ((struct agentClientGetNearestPrecedingTeamMember*)msg)->nid;
			moreMsgFollowing =
				((struct agentClientGetNearestPrecedingTeamMember*) msg)->moreMsgFollowing;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNearestPrecedingTeamMember(Preceding_Nid);
					result = 0;
				}
			}
			((struct agentClientGetNearestPrecedingTeamMemberReply *) msg)->type =
				AGENT_CLIENT_GET_NEAREST_PRECEDING_TEAM_MEMBER_REPLY;
			((struct agentClientGetNearestPrecedingTeamMemberReply *) msg)->result =
				result;
			((struct agentClientGetNearestPrecedingTeamMemberReply *) msg)->Preceding_Nid = Preceding_Nid;
			writen(fd, msg, sizeof(struct agentClientGetNearestPrecedingTeamMemberReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER:

			n=read(fd,msg,sizeof(struct agentClientGetNearestFollowingTeamMember));
			nid = ((struct agentClientGetNearestFollowingTeamMember*)msg)->nid;
			moreMsgFollowing =
				((struct agentClientGetNearestFollowingTeamMember*) msg)->moreMsgFollowing;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getNearestFollowingTeamMember(Following_Nid);
					result = 0;
				}
			}
			((struct agentClientGetNearestFollowingTeamMemberReply *) msg)->type =
				AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER_REPLY;
			((struct agentClientGetNearestFollowingTeamMemberReply *) msg)->result =
				result;
			((struct agentClientGetNearestFollowingTeamMemberReply *) msg)->Following_Nid = Following_Nid;
			printf("AGENT_CLIENT_GET_NEAREST_FOLLOWING_TEAM_MEMBER in command serv\n");
			writen(fd, msg, sizeof(struct agentClientGetNearestFollowingTeamMemberReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_TEAM_TURNING_INFO:
			n = read(fd,msg,sizeof(struct agentClientGetTeamTurningInfo));
			nid = ((struct agentClientGetTeamTurningInfo*)msg)->nid;
			moreMsgFollowing =
				((struct agentClientGetTeamTurningInfo*) msg)->moreMsgFollowing;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getTeamTurningInfo(Turning_direction, Turning_block, Turning_times);
					result = 0;
				}
			}
			((struct agentClientGetTeamTurningInfoReply *) msg)->type =
				AGENT_CLIENT_GET_TEAM_TURNING_INFO_REPLY;
			((struct agentClientGetTeamTurningInfoReply *) msg)->result =
				result;
			((struct agentClientGetTeamTurningInfoReply *) msg)->TurningDirection =
				Turning_direction;
			((struct agentClientGetTeamTurningInfoReply *) msg)->TurningBlockNumber =
				Turning_block;
			((struct agentClientGetTeamTurningInfoReply *) msg)->TurningTimes =
				Turning_times;
			writen(fd, msg, sizeof(struct agentClientGetTeamTurningInfoReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_SET_TEAM_TURNING_INFO:
			n=read(fd,msg,sizeof(struct agentClientSetTeamTurningInfo));
			nid = ((struct agentClientSetTeamTurningInfo*)msg)->nid;
			Turning_direction = ((struct agentClientSetTeamTurningInfo*)msg)->TurningDirection;
			Turning_block = ((struct agentClientSetTeamTurningInfo*)msg)->TurningBlockNumber;
			Turning_times = ((struct agentClientSetTeamTurningInfo*)msg)->TurningTimes;
			moreMsgFollowing =
				((struct agentClientSetTeamTurningInfo*) msg)->moreMsgFollowing;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->setTeamTurningInfo(Turning_direction, Turning_block, Turning_times);
					result = 0;
				}
			}
			((struct agentClientSetTeamTurningInfoReply *) msg)->type =
				AGENT_CLIENT_SET_TEAM_TURNING_INFO_REPLY;
			((struct agentClientSetTeamTurningInfoReply *) msg)->result =
				result;
			writen(fd, msg, sizeof(struct agentClientSetTeamTurningInfoReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_LANE_CHANGE_INFO:
			n=read(fd,msg,sizeof(struct agentClientGetLaneChangeInfo));
			nid = ((struct agentClientGetLaneChangeInfo*)msg)->nid;
			moreMsgFollowing =
				((struct agentClientGetLaneChangeInfo*) msg)->moreMsgFollowing;

			if ((nid > 0) && (nid <= MAX_NUM_NODE)) {
				if (nid_info[nid]->is_agent_up() == 1) {
					((Node*) nodelist[nid])->getLaneChangeInfo(EnableLaneChange);
					result = 0;
				}
			}
			((struct agentClientGetLaneChangeInfoReply *) msg)->type =
				AGENT_CLIENT_GET_LANE_CHANGE_INFO_REPLY;
			((struct agentClientGetLaneChangeInfoReply *) msg)->result =
				result;
			((struct agentClientGetLaneChangeInfoReply *) msg)->EnableLaneChange =
				EnableLaneChange;

			writen(fd, msg, sizeof(struct agentClientGetLaneChangeInfoReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE:
			n = readn(fd, msg, sizeof(struct agentClientGetCurrentPositionOfATeamOfNode));
			moreMsgFollowing = ((struct agentClientGetCurrentPositionOfATeamOfNode *) msg)->moreMsgFollowing;
			int tid;
			tid = ((struct agentClientGetCurrentPositionOfATeamOfNode *) msg)->tid;

			result = 0;
			numNode = Team_lst[tid][0];
			((struct agentClientGetCurrentPositionOfATeamOfNodeReply *) longMsg)->type =
				AGENT_CLIENT_GET_CURRENT_POSITION_OF_A_TEAM_OF_NODE_REPLY;
			((struct agentClientGetCurrentPositionOfATeamOfNodeReply *) longMsg)->result = result;
			((struct agentClientGetCurrentPositionOfATeamOfNodeReply *) longMsg)->numNode = numNode;

			npp = (struct nodePosition *)((char *) longMsg + sizeof (struct agentClientGetCurrentPositionOfATeamOfNodeReply));

			for (j = 1; j <= numNode; j++)
			{
				nid = Team_lst[tid][j];
				((Node*) nodelist[nid])->getNodePosition(x, y, z);
                                npp->nid = nid;
				npp->x = x;
				npp->y = y;
				npp->z = z;
				npp++;
			}

			int wn;
			wn = writen(fd, longMsg,
				sizeof(struct agentClientGetCurrentPositionOfATeamOfNodeReply) + numNode*sizeof(struct nodePosition));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;

		case AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR:
			n=read(fd,msg,sizeof(struct agentClientSwitchOrderInTeamListWithPrecedingCar));
			nid = ((struct agentClientSwitchOrderInTeamListWithPrecedingCar*)msg)->nid;
			moreMsgFollowing =
				((struct agentClientSwitchOrderInTeamListWithPrecedingCar*) msg)->moreMsgFollowing;

			result = ((Node*)nodelist[nid])->SwitchOrderWithPrecedingCarInTeam();
			((struct agentClientSetTeamTurningInfoReply *) msg)->type =
				AGENT_CLIENT_SWITCH_ORDER_IN_TEAMLIST_WITH_PRECEDING_CAR_REPLY;
			((struct agentClientSetTeamTurningInfoReply *) msg)->result =
				result;
			writen(fd, msg, sizeof(struct agentClientSwitchOrderInTeamListWithPrecedingCarReply));

			if (moreMsgFollowing != 0) {
				goto recheck;
			}
			break;
	}
	return(1);
}

int cmd_server::registerApproachingNotification(int nid, int registrationID,
		int timeIntervalInMilliseconds, int anid, int gid, double withinRangeinMeter) {
	struct registeredApproachingCheckingRecord *p;
	u_int64_t       timeTick;

	printf("Enter cmd_server::registerApproachingNotification()\n");
	p = (struct registeredApproachingCheckingRecord *) 
		malloc(sizeof(struct registeredApproachingCheckingRecord));
	p->triggerType = TRIGGER_TYPE_NODE_APPROACHING;
	p->nid = nid;
	p->registrationID = registrationID;
	p->anid = anid;
	p->gid = gid;
	p->withinRangeinMeter = withinRangeinMeter;
	p->timeIntervalInMilliseconds = timeIntervalInMilliseconds;
	if (timeIntervalInMilliseconds > 0) {
		p->suspendedflag = 1;
		MILLI_TO_TICK(timeTick, timeIntervalInMilliseconds);
		p->resumeTime = GetCurrentTime() + timeTick;
	} else { // As soon as possible
		p->suspendedflag = 0;
		p->resumeTime = 0;
	}	
	m_mapTriggerMap.insert(TRIGGER_MAP::value_type(GetCurrentTime(), p));
	return(1);
}


int cmd_server::cancelApproachingNotification(int nid, int registrationID) {

	TRIGGER_MAP::iterator it = m_mapTriggerMap.begin();
	while (it != m_mapTriggerMap.end()) {
		if ((it->second->nid == nid) && (it->second->registrationID == registrationID)) {
			delete(it->second);
			m_mapTriggerMap.erase(it);
			return(1);
		} else ++it;
	}
	return(0);
}

int cmd_server::suspendApproachingNotification(int nid, int registrationID, 
		int timeIntervalInMilliseconds) {
	struct registeredApproachingCheckingRecord *p; 
	u_int64_t       timeTick;

	TRIGGER_MAP::iterator it = m_mapTriggerMap.begin();
	while (it != m_mapTriggerMap.end()) {
		if ((it->second->nid == nid) && (it->second->registrationID == registrationID)) {
			p = it->second;
			if (timeIntervalInMilliseconds == 0) { // 0: means forever
				p->suspendedflag = 1;
				p->resumeTime = 0;
				return(1);
			} else if (timeIntervalInMilliseconds > 0) {
				p->suspendedflag = 1;
				MILLI_TO_TICK(timeTick, timeIntervalInMilliseconds);
				p->resumeTime = GetCurrentTime() + timeTick;
				return(1);
			} else return(0);
		} else ++it;
	}
	return(0);
}

int cmd_server::resumeApproachingNotification(int nid, int registrationID) {
	struct registeredApproachingCheckingRecord *p; 

	TRIGGER_MAP::iterator it = m_mapTriggerMap.begin();
	while (it != m_mapTriggerMap.end()) {
		if ((it->second->nid == nid) && (it->second->registrationID == registrationID)) {
			p = it->second;
			p->suspendedflag = 0;
			p->resumeTime = 0;
			return(1);
		} else ++it;
	}
	return(0);
}


int cmd_server::checkIPCmessages() {
	struct  timeval T;
	struct sockaddr_in fsin;
	socklen_t alen = sizeof(fsin);
	int sock, i, on = 1;

	T.tv_sec = 0;
	T.tv_usec = 0;
	rfds = afds;
	if (select(nfds, &rfds, (fd_set *) 0, (fd_set *) 0, 
				(struct timeval *) &T) <= 0) {
		return(0);
	} else {
		if (FD_ISSET(msock, &rfds)) {
			alen = sizeof(fsin);
			sock = accept(msock, (struct sockaddr *) &fsin, &alen);
			//printf("accept return socket fd %d\n", sock);
			if (sock < 0) {
				printf("Error: Accept() sock fd  = %d\n", sock);
				exit(1);
			}
			if (sock > maxSockFd) maxSockFd = sock;
			nfds = maxSockFd + 1;

			FD_SET(sock, &afds);
			if (setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &on, sizeof(on))
					!= 0 ) {
				printf("set TCP_NODELAY failed\n");
				exit(1);
			}
			associateAgentClient(sock);
		}
		for (i = 3; i <= maxSockFd; i++) {
			if ((i != msock) && FD_ISSET(i, &rfds)) {
				for (int j = 1; j <= MAX_NUM_NODE; j++) {
					if(nid_info[j]->find_fd(i) == 1) {
						nid_info[j]->set_mesg(i, GetCurrentTime() + 1);
						break;
					}
				}
				char *msg,*longMsg;
				msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
				longMsg = (char *) malloc(IP_PACKET_MAX_LEN);
				communicateAgentClient(i, msg, longMsg);
				free(msg);
				free(longMsg);

			}
		} // for
	} // else
	return(1);
}

int cmd_server::checkIPCmessagesNodeID(u_int32_t nid, int agent_socket_fd, int pid) {
	struct  timeval T;
	int i, fd;
	int timeout;
	fd_set rfds_n;

	if ((fd = nid_info[nid]->agent_fd_pid_to_my_fd(agent_socket_fd, pid)) == 0)
		return (0);

	if(nid_info[nid]->get_mesg(fd) == GetCurrentTime())
		return (0);

	timeout = CHECK_IPC_MESSAGE_TIMEOUT;
	for (i = 0; i < CHECK_IPC_MESSAGE_MAX_TIMES; ++i) {
		FD_ZERO(&rfds_n);
		FD_SET(fd, &rfds_n);

		T.tv_sec = 0;
		T.tv_usec = timeout;

		switch (select(fd + 1, &rfds_n, (fd_set *) 0, (fd_set *) 0, (struct timeval *) &T)) {
			case -1:
				/* occur error */
				perror("checkIPCmessagesNodeID");
				return(0);
			case 0:
				/* timeout */
				timeout *= 2;
				printf("[CMD_SERVER] handle Node %u message timeout, next timeout is %d us, %llu, %llu\n", nid, timeout, nid_info[nid]->get_mesg(fd), GetCurrentTime());
				continue;
			default:

				char* msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
				char* longMsg = (char *) malloc(IP_PACKET_MAX_LEN);
				nid_info[nid]->set_mesg(fd, GetCurrentTime());
				communicateAgentClient(fd,msg,longMsg);
				free(msg);
				free(longMsg);
				return(1);
		};
	}
	return(0);
}

int cmd_server::changeNodeSpeedFromGUI(int nid, double speed) {

	if (nid > 0) {
		((Node*)nodelist[nid])->setNodeSpeed(speed);
		return(1);
	} else return(-1);
}		

int cmd_server::changeNodeMovingDirectionFromGUI(int nid, double angle) {

	if (nid > 0) {
		((Node*)nodelist[nid])->setNodeAngle(angle);
		return(1);
	} else return(-1);
}

int cmd_server::checkRegisteredRequests() {
	int numNode, timeIntervalInMilliseconds, anid, j;
	struct registeredApproachingCheckingRecord *p;
	double ax, ay, az, mx, my, mz, dx, dy, dz, dis, speed, angle;
	char *msg;
	GROUPMEMBER_VECTOR *gv;
	u_int64_t       timeTick;
	struct sockaddr_in agent_addr;
	struct hostent  *phe;   /* pointer to host information entry    */
	int n, i, *udpports, size;

	timeIntervalInMilliseconds = 0;
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	TRIGGER_MAP::iterator it = m_mapTriggerMap.begin();
	while (it != m_mapTriggerMap.end()) {
		p = it->second;
		if ((p->suspendedflag == 1) && (p->resumeTime > 0) && (GetCurrentTime() > p->resumeTime)) {
			// resume this request because the specified time interval has elapsed
			p->suspendedflag = 0;
			p->resumeTime = 0;
		}
		((Node*) nodelist[p->nid])->getNodePosition(mx, my, mz);
		if (p->suspendedflag != 1) { // Checking conditions

			if (p->anid > 0) {
				((Node*) nodelist[p->anid])->getNodePosition(ax, ay, az);
				dx = ax - mx;
				dy = ay - my;
				dz = az - mz;
				dis = sqrt(dx*dx + dy*dy + dz*dz);
				if (dis < p->withinRangeinMeter) {
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->type = 
						SIMULATION_ENGINE_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME;
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->anid = p->anid;
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->x = ax;
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->y = ay;
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->z = az;
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->distance = dis;
					((Node*) nodelist[p->anid])->getNodeSpeed(speed);
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->speed = speed;
					((Node*)nodelist[p->anid])->getNodeAngle(angle);
					((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->angle = angle;

					bzero(( char *) &agent_addr, sizeof( agent_addr ));
					phe = gethostbyname("127.0.0.1");
					memcpy(&agent_addr.sin_addr, phe->h_addr, phe->h_length);
					agent_addr.sin_family      = AF_INET;

					udpports = nid_info[p->anid]->get_all_udpportnum(size);
					for(i=0 ; i<size ; i++)
					{
						agent_addr.sin_port = htons(udpports[i]);
						n = sendto(mudpsock, msg, sizeof(struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe), 0, (struct sockaddr *)&agent_addr, sizeof(agent_addr));
						if (n < 0) {
							printf("sendto() failed\n");
							exit(0);
						}
					}
				}
			} else {
				if (p->gid > 0) {
					switch (p->gid) {
						case 1:
							gv = GROUP_FIND(GROUP_1)->GetMember();
							break;
						case 2:
							gv = GROUP_FIND(GROUP_2)->GetMember();
							break;
						case 3:
							gv = GROUP_FIND(GROUP_3)->GetMember();
							break;
						case 4:
							gv = GROUP_FIND(GROUP_4)->GetMember();
							break;
						case 5:
							gv = GROUP_FIND(GROUP_5)->GetMember();
							break;
						case 6:
							gv = GROUP_FIND(GROUP_6)->GetMember();
							break;
						case 7:
							gv = GROUP_FIND(GROUP_7)->GetMember();
							break;
						case 8:
							gv = GROUP_FIND(GROUP_8)->GetMember();
							break;
						default:
							gv = NULL;
					} // switch
					((Node*) nodelist[p->nid])->getNodePosition(mx, my, mz);
					if (gv != NULL) numNode = static_cast<int>(gv->size());
					else numNode = 0;
					for (j = 0; j < numNode; j++) {
						anid = ((*gv)[j])->GetID();
						if (anid == p->nid) continue;
						((Node*) nodelist[anid])->getNodePosition(ax, ay, az);
						dx = ax - mx;
						dy = ay - my;
						dz = az - mz;
						dis = sqrt(dx*dx + dy*dy + dz*dz);
						if (dis < p->withinRangeinMeter) {
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->type =
								SIMULATION_ENGINE_NOTIFICATION_WHEN_ANOTHER_NODE_HAS_APPROACHED_ME;
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->anid = anid;
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->x = ax;
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->y = ay;
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->z = az;
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->distance = dis;
							((Node*) nodelist[anid])->getNodeSpeed(speed);
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->speed = speed;
							((Node*)nodelist[anid])->getNodeAngle(angle);
							((struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe *) msg)->angle = angle;
							printf("Send out a notification, nid %d anid %d ax %lf ay %lf az %lf aspeed %lf aangle %lf distance %lf\n", p->nid, anid, ax, ay ,az, speed, angle, dis);

							bzero(( char *) &agent_addr, sizeof( agent_addr ));
							phe = gethostbyname("127.0.0.1");
							memcpy(&agent_addr.sin_addr, phe->h_addr, phe->h_length);
							agent_addr.sin_family      = AF_INET;

							udpports = nid_info[anid]->get_all_udpportnum(size);
							for(i=0 ; i<size ; i++)
							{
								agent_addr.sin_port = htons(udpports[i]);
								n = sendto(mudpsock, msg, sizeof(struct SimulationEngineNotificationWhenAnotherNodeHasApproachedMe), 0, (struct sockaddr *)&agent_addr, sizeof(agent_addr));
								if (n < 0) {
									printf("sendto() failed\n");
									exit(0);
								}
							}
						} // if (dis < p->withinRangeinMeter)
					} // for
				} // if (p->gid > 0)
			} // else
		} //  if (p->suspendedflag != 1)
		if ((p->suspendedflag == 0) && (p->timeIntervalInMilliseconds > 0)) {
			// Re-install the periodic checking operation
			p->suspendedflag = 1;
			MILLI_TO_TICK(timeTick, timeIntervalInMilliseconds);
			p->resumeTime = GetCurrentTime() + timeTick;
		}
		++it;
	}
	free(msg);
	return(0);
}

void cmd_server::closeSpecifiedClientPID(int pid) {
	int i, j;
	int *fds, size;

	for (i = 1; i <= MAX_NUM_NODE; i++) {
		if(nid_info[i]->find_pid(pid) > 0) {
			printf("[closeAllClientSocket]:: kill process of pid %d\n", pid);
			fds = nid_info[i]->pid_to_my_fd(pid, size);
			for( j=0 ; j<size ; j++)
			{
				close(fds[j]);
				FD_CLR(fds[j], &afds);
			}
			nid_info[i]->clear_pid_fd(pid);
			break;
		}
	}
}

void cmd_server::closeAllClientSocket() {
	int i;

	for (i = 1; i <= MAX_NUM_NODE; i++) {
		nid_info[i]->clear_all();
	}
	close(msock);
}

/*
 * Each WSM module will add itself into cmd_serv_reg_info list during initiation
 */
void cmd_server::moduleCreateRegInfo(u_int32_t nid, u_int16_t port/*, void *wme_pointer*/) {
	struct CmdServRegInfo *tmp;

	tmp = new (struct CmdServRegInfo);
	tmp->nid = nid;
	tmp->port = port;
	tmp->wme_obj = InstanceLookup(nid, port, "WME");
	tmp->enable = false;
	tmp->next = NULL;

	if(cmd_serv_reg_info_first == NULL) {
		cmd_serv_reg_info_first = tmp;
		cmd_serv_reg_info_end = tmp;
	}
	else
	{
		cmd_serv_reg_info_end->next = tmp;
		cmd_serv_reg_info_end = cmd_serv_reg_info_end->next;
	}
}

/*
 * Mapping each application process (WSM) into WSM module
 */
void cmd_server::agentEnableRegInfo(int fd, u_int32_t nid, pid_t pid) {
	struct CmdServRegInfo *tmp;
	tmp = cmd_serv_reg_info_first;
	while(tmp != NULL)
	{
		if(tmp->nid == nid)
		{
			tmp->enable = true;
			tmp->fd = fd;
			tmp->pid = pid;
			break;
		}
		tmp = tmp->next;
	}
	if(tmp == NULL)
		printf("[Command server] : Agent(%d) Enable Reg Info failed\n", nid);
}
int cmd_server::recvWSMtoAgent(u_int32_t nid, struct WSM_Header wsm_header, char *wsm_data) {

	struct CmdServRegInfo *tmp;
	int n;
	char *mesg;
	struct WaveShortMessageProtocol wsmp;

	wsmp.type = WAVE_SHORT_MESSAGE_PROTOCOL;
	wsmp.moreMsgFollowing = 0;
	wsmp.nid = nid;
	memcpy(&(wsmp.wsm_header), &wsm_header, sizeof(struct WSM_Header));

	mesg = (char*) malloc(sizeof(char) * MAX_RECV_BYTES);
	memcpy(mesg, &wsmp, sizeof(struct WaveShortMessageProtocol));
	memcpy( (mesg + sizeof(struct WaveShortMessageProtocol)), wsm_data, wsm_header.wsm_len);
	tmp = cmd_serv_reg_info_first;
	while(tmp != NULL)
	{
		if(tmp->nid == nid)
		{
			if(tmp->enable != true) {
				printf("[Command server] : (%d) Reg Info does not enable\n", nid);
				break;
			}
			n = writen(tmp->fd, mesg, sizeof(struct WaveShortMessageProtocol) + wsm_header.wsm_len);
			/* send signal to inform WSM process to recv data */
			if(kill(tmp->pid, SIGUSR1) != 0)
				printf("Command Server : send Signal to Agent(%d) failed\n", nid);
			break;
		}
		tmp = tmp->next;
	}
	if(tmp == NULL)
		printf("Command server : Agent Reg Info does not exist\n");
	free(mesg);
	return -1;
}
int cmd_server::sendWSMtoModule(u_int32_t nid, char *mesg) {
	struct CmdServRegInfo *tmp;
	tmp = cmd_serv_reg_info_first;
	while(tmp != NULL)
	{
		if(tmp->nid == nid)
		{
			if(tmp->enable != true) {
				printf("Command server : Agent Reg Info does not enable\n");
				break;
			}
			((WME *)(tmp->wme_obj))->send_wsm(((struct WaveShortMessageProtocol *)mesg)->wsm_header, mesg + sizeof(struct WaveShortMessageProtocol));
			return 1;
		}
		tmp = tmp->next;
	}
	if(tmp == NULL)
		printf("Command server : Agent Reg Info does not exist\n");
	return -1;
}
