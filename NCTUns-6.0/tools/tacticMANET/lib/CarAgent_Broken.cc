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

/*
Scenario Description:
	This CarAgent simulate a car with flat tire. The car can't move but can send BROKEN_CAR message to 
	other cars near it.
	When other CarAgents receive this message, they can use this message to do something they want.
	For example, change direction or slow down its speed.

	By automatic vehicle group
	6/10/2007
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include "math_fun.h"
#include "tactic_api.h"
#include "sock_skel.h"
#include "road.h"

#define ITS_SHOW_MSG	0

int mynid, n, myTCPsockfd;
int socketfd2;
int myUDPsockfd;
int agentUDPportNum = 4000;
int CacheOrNot = 1;

double CurrentPOS_x, CurrentPOS_y, CurrentPOS_z;
double curRandDirection;
double timeDiff = 0.0;
double lastTime = 0.0;

CacheRecord myCache;

struct timeval now;

class msgSequence{
	public:
	u_int32_t nid;
	int seqNum; // sequence number
};

vector<msgSequence> msgSeq;

double randomDirection(double *tempRoadDirection, int tempNumDirection){
	int choice;
	choice=rand()%tempNumDirection;
	return tempRoadDirection[choice];
}
/*
void reportMyStatusToAGroupOfNode(int myUDPsocket, int agetnUDPportNum, int myTCPsockfd, int gid, int moreMsgFollowing)
{
	static int seqNum = 0;
	double lastTime = (double) now.tv_sec * 1000000 + (double) now.tv_usec;
	char hostname[128];
	double tmpCarDistance;
	double effectiveDistance = 500;
	int numNodeInThisGroup = 0, n = 0;

	agentClientReportStatus msg;
	msg.type = AGENT_CLIENT_IS_A_BROKEN_CAR;
	msg.x = curX;
	msg.y = curY;
	msg.nid = mynid;
	msg.moreMsgFollowing = 1;
	msg.acceleration = 0;
	msg.speed = 0;
	msg.direction = curRandDirection;
	msg.seqNum = seqNum;
	msg.timeStamp = lastTime;
	msg.TTL = 3;

	seqNum++;
	nodePosition *NPArray, *tmpNPArray;
	sockaddr_in cli_addr;
	n = getCurrentPositionOfAGroupOfNodeInADistance(myTCPsockfd, mynid, gid, &NPArray, numNodeInThisGroup, curX, curY, effectiveDistance, 1);
	if((NPArray == NULL) || (numNodeInThisGroup == 0) || (n < 0)) {
		printf("CarAgent %d getCurrentPositionOfAGroupOfNodeInADistance from group %d failed.\n", mynid, gid);
		//groupNotFound++;
		return;
	}
	tmpNPArray = NPArray;

	for (int i=0; i<numNodeInThisGroup; i++) {
		if ((int)NPArray->nid == mynid) {
			// A mobile node should not send a message to itself.
			NPArray++;
			continue;
		}

		tmpCarDistance = twoPointsDistance( curX, curY, NPArray->x, NPArray->y);
		if(tmpCarDistance <= effectiveDistance){
			sprintf(hostname, "1.0.1.%d", NPArray->nid);
			bzero( &cli_addr, sizeof(cli_addr) );
			cli_addr.sin_family = AF_INET;
			cli_addr.sin_port   = htons(agentUDPportNum);
			cli_addr.sin_addr.s_addr = inet_addr( hostname );
			n = sendto(myUDPsockfd, &msg, sizeof(struct agentClientReportStatus), 0, 
				(struct sockaddr *) &cli_addr, sizeof(sockaddr));
			if (n < 0) {
				printf("Agent (%d) send to (%d) failed\n", mynid, NPArray->nid);
				//exit(0);
			}
			else{
				if(ITS_SHOW_MSG)
					printf("Node (%d) reports status to (%d) distance %f, pktSize %d, seqNum %d\n", mynid, NPArray->nid, tmpCarDistance, sizeof(struct agentClientReportStatus), msg.seqNum);
			}
		}
		NPArray++;
	}
	free((char *) tmpNPArray);
}
*/

void reportMyStatusToAGroupOfNode(int myUDPsocket, int agetnUDPportNum, int myTCPsockfd, int moreMsgFollowing)
{
	static int seqNum=0;
	double lastTime = (double) now.tv_sec * 1000000 + (double) now.tv_usec;
	char hostname[128];
	int value;
	int len;

	agentClientReportStatus *msg;
	msg = new agentClientReportStatus;
	msg->x = CurrentPOS_x;
	msg->y = CurrentPOS_y;
	msg->type = AGENT_CLIENT_IS_A_BROKEN_CAR;
	msg->nid = mynid;
	msg->moreMsgFollowing = 1;
	msg->acceleration = 0;
	msg->speed = 0;
	msg->direction = curRandDirection;
	msg->seqNum = seqNum;
	msg->timeStamp=lastTime;
	msg->TTL=3;
	seqNum++;
	sockaddr_in cli_addr;

	//Broadcast start

	value = 1;
	setsockopt( myUDPsockfd , SOL_SOCKET , SO_BROADCAST , &value , sizeof(value) );
	len =  sizeof(struct sockaddr);
	strcpy(hostname, "1.0.1.255");
	memset( &cli_addr, 0, sizeof(cli_addr) );
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port   = htons(agentUDPportNum);
	cli_addr.sin_addr.s_addr = inet_addr( hostname );
	int n = sendto(myUDPsockfd, msg, sizeof(struct agentClientReportStatus), 0, (struct sockaddr *) &cli_addr, len);
	if (n < 0) {
		printf("Agent (%d) sendto() failed\n", mynid);
	}
	else{
		//printf("Car Agent (%d) broadcast pkt, Size %d, seqNum %d, packet type==%d\n", mynid, sizeof(struct agentClientReportStatus), msg->seqNum, msg->type);
	}
	//Broadcast end
	free(msg);
}

void receiveMsg()
{
	int len = sizeof(struct sockaddr);
	int n = 1;
	sockaddr_in cli_addr;
	while( n > 0){
		typeChecker p;
		n = recvfrom(myUDPsockfd, (char *) &p, sizeof(struct typeChecker), MSG_PEEK, (struct sockaddr *) &cli_addr, (socklen_t*)&len); 
		if( n == 0){
			printf("Car Agent: %d UDP socket error\n", mynid);
			return;
		}
		if(p.type == RSUAGENT_REPORT_WARNING){
			RSUAgentReportWarning msg;
			n = recvfrom(myUDPsockfd, (char *)&msg, sizeof(struct RSUAgentReportWarning), 0, (struct sockaddr *) &cli_addr, (socklen_t*)&len); 
			printf("Agent %d : Receive Warning from RSU %d\n", mynid, msg.RSUnid);
			if( msg.AccelerationOrDeceleration == -1)
				//WarningDecelerate = -10;
			continue;
		}
		else if(p.type == AGENT_CLIENT_REPORT_STATUS || p.type == AGENT_CLIENT_IS_A_BROKEN_CAR){
			agentClientReportStatus msg;
			n = recvfrom(myUDPsockfd, (char *)&msg, sizeof(struct agentClientReportStatus), 0, (struct sockaddr *) &cli_addr, (socklen_t*)&len); 
			if(msg.seqNum > 0){
				int nidExists = 0;
				for (int i=0 ; i<(int)(msgSeq.size()); i++)
				{
					if(msg.nid == msgSeq[i].nid)
					{
						nidExists=1;
						if(msg.seqNum > msgSeq[i].seqNum)
						{
							msgSeq[i].seqNum = msg.seqNum;
							if(msg.type == AGENT_CLIENT_IS_A_BROKEN_CAR){
								if(ITS_SHOW_MSG)
									printf("CarAgent(%d) received a broken car report from Agent (%d): %d\n", mynid, msg.nid, msg.seqNum);
								break;
							}
							if( ITS_SHOW_MSG)
								printf("CarAgent(%d) received a status report from Agent (%d): %d\n", mynid, msg.nid, msg.seqNum);

						}
						else{ 
							//	printf("CarAgent(%d): Message is already received from agent (%d)_%d\n", mynid, msg.nid, msg.seqNum);
						}
						break;

					}

				}
				if(nidExists == 0)
				{
					msgSequence temp;
					temp.nid = msg.nid;
					temp.seqNum = msg.seqNum;
					msgSeq.push_back(temp);
					if(msg.type == AGENT_CLIENT_IS_A_BROKEN_CAR){
						if( ITS_SHOW_MSG)
							printf("CarAgent(%d) received a broken car report from Agent(%d): %d\n", mynid, msg.nid, msg.seqNum);
					}
					else {
						if( ITS_SHOW_MSG)
							printf("CarAgent(%d) received a status report from Agent(%d): %d\n", mynid, msg.nid, msg.seqNum);
					}
				}
			}
		}
	}
}

int main(int argc, char *argv[]) 
{
     char portNumStr[32];

     mynid = getMyNodeID();	  
     // CarAgent is in group 1
     myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(
       	mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);

     sprintf(portNumStr, "%d", agentUDPportNum);
     myUDPsockfd = passiveUDP(portNumStr); 
     printf("Agent(%d) created myUDPsockfd %d\n", mynid, myUDPsockfd);
     if (myUDPsockfd < 0) {
	printf("Agent(%d): Creating myUDPsockfd failed\n", mynid);
	exit(0);
     }
     n = fcntl(myUDPsockfd, F_SETFL, O_NONBLOCK);

     constructRoadMapofTheWholeField(1);
     n = getInitialNodePosition(mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z);

     double curspeed;
     double curAcceleration = 0;

     /* default parameter */
     curspeed = 0;
     curAcceleration = 1; /* acceleration = 1 m/sec^2 */

     /* manual input parameter */
     if(argc == 3){
	curspeed = atof(argv[1]);
	curAcceleration = atof(argv[2]);
     }

     double c_X, c_Y;

     n = selfCorrectness(CurrentPOS_x, CurrentPOS_y, c_X, c_Y, CacheOrNot, &myCache);
     if(n == 1){
	     n = setCurrentWaypoint(myTCPsockfd, mynid, c_X, c_Y, CurrentPOS_z, 1);
	     printf("agent %d currects his position (%f, %f)\n", mynid, c_X, c_Y);
     }

     n = getCurrentPosition(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);

     int RoadType;
     int numCurDirection;
     double* curRoadDirection = NULL;
     double a, b, c, endPOS_X, endPOS_Y;

     n = getCurrentRoadInformation(CurrentPOS_x, CurrentPOS_y, numCurDirection, curRoadDirection, 
	a, b, c, endPOS_X, endPOS_Y, RoadType, CacheOrNot, &myCache); 
     if(n <= 0){
	     printf("Warning_1_1: Node is not on the lane!! Node %d position(%f, %f)\n", mynid, CurrentPOS_x,CurrentPOS_y);
	     stopSimulation(myTCPsockfd, mynid);
     }

     //srand(time(0));
     curRandDirection = randomDirection(curRoadDirection, numCurDirection);
     n = setCurrentMovingSpeed(myTCPsockfd, mynid, curspeed, 1);
     n = getCurrentPosition(myTCPsockfd, mynid, CurrentPOS_x, CurrentPOS_y, CurrentPOS_z, 1);
     n = setCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, mynid, curRandDirection, 1);
     reportMyStatusToAGroupOfNode(myUDPsockfd, agentUDPportNum, myTCPsockfd, 1);
     usleepAndReleaseCPU(myTCPsockfd, mynid, 1, 1);

     //int unicastPeriod = 0;

     while (1) 
     {
	gettimeofday(&now, 0);

	timeDiff = ((double) now.tv_sec * 1000000 + (double) now.tv_usec) - lastTime;
	lastTime = (double) now.tv_sec * 1000000 + (double) now.tv_usec;
	receiveMsg();
	//if( unicastPeriod == 0 ){
	// wakeup 10 times then unicast once
			reportMyStatusToAGroupOfNode(myUDPsockfd, agentUDPportNum, myTCPsockfd, 1);
	//}
	//unicastPeriod++;
	//unicastPeriod %= 10;
        usleepAndReleaseCPU(myTCPsockfd, mynid, 100000, 1);
       /* The above control is exerted every 0.1 second. */
    }
}
