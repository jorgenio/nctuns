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
	This road side unit ( RSU ) will receive message from Agent and forward message to other Agent.
	Agent can be RSUAgent or CarAgent.

	By automatic vehicle group.
	5/31/2007
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

int mynid, n;
int myUDPsockfd;
int agentUDPportNum = 4000;
struct timeval now;
double lastTime = 0.0;
char portNumStr[32];

class msgSequence{
	public:
	u_int32_t nid;
	int seqNum; // sequence number
};
vector<msgSequence> msgSeq;

void reportMyStatusToAGroupOfNode(struct agentClientReportStatus msg)
{
	//double lastTime = (double) now.tv_sec * 1000000 + (double) now.tv_usec;
	char hostname[128];
	int value;
	int len;

	sockaddr_in cli_addr;

//Braod cast start
	value = 1;
	setsockopt( myUDPsockfd , SOL_SOCKET , SO_BROADCAST , &value , sizeof(value) );
	len =  sizeof(struct sockaddr);
	strcpy(hostname, "1.0.1.255");
	memset( &cli_addr, 0, sizeof(cli_addr) );
	cli_addr.sin_family = AF_INET;
	cli_addr.sin_port   = htons(agentUDPportNum);
	cli_addr.sin_addr.s_addr = inet_addr( hostname );
	int n = sendto(myUDPsockfd, &msg, sizeof(struct agentClientReportStatus), 0, (struct sockaddr *) &cli_addr, len);
	if (n < 0) {
		printf("Agent (%d) sendto failed\n", mynid);
	}
	else{
		if(ITS_SHOW_MSG){
			printf("RSU_AGNET: Car Agent (%d) broadcast pkt, Size %d, seqNum %d, packet type==%d\n", mynid, sizeof(struct agentClientReportStatus), msg.seqNum, msg.type);
		}
	}
}

void receiveMsg(){
	int len = sizeof(struct sockaddr);
	int n = 1;
	sockaddr_in cli_addr;
	while( n > 0 ){
		agentClientReportStatus msg;
		n = recvfrom(myUDPsockfd, &msg, sizeof(struct agentClientReportStatus), 0, (struct sockaddr *) &cli_addr, (socklen_t*)&len); 
		if (n > 0) { // Someone has sent me a message, I can do something by this message.
			if ((msg.type != AGENT_CLIENT_REPORT_STATUS) && (msg.type != AGENT_CLIENT_IS_A_BROKEN_CAR)) {
				printf("Agent(%d): no such message type.\n", mynid);
			}
			if(msg.seqNum > 0){
				int nidExists = 0;
				for (int i=0 ; i<(int)(msgSeq.size()); i++)
				{
					if(msg.nid == msgSeq[i].nid)
					{
						nidExists=1;
						if(msg.seqNum > msgSeq[i].seqNum)
						{
							//printf("RSUAgent(%d) receive message of agent %d.\n", mynid, msg.nid);
							msgSeq[i].seqNum = msg.seqNum;
							reportMyStatusToAGroupOfNode(msg);
						}
						else{
							//printf("RSUAgent(%d): Message is already received from agent (%d)_%d\n", mynid, msg.nid, msg.seqNum);
						}
						break;
					}
				}
				if(nidExists == 0)
				{
					if(ITS_SHOW_MSG){
						printf("RSUAgent(%d) receives message of agent %d.\n", mynid, msg.nid);
					}
					reportMyStatusToAGroupOfNode(msg);
					msgSequence temp;
					temp.nid = msg.nid;
					temp.seqNum = msg.seqNum;
					msgSeq.push_back(temp);
				}
			}
		}
	}
}
int main(int argc, char *argv[]) {
     mynid = getMyNodeID(); 
     // CarAgent is in group 3

     sprintf(portNumStr, "%d", agentUDPportNum);
     myUDPsockfd = passiveUDP(portNumStr); 
     printf("Agent(%d) created myUDPsockfd %d\n", mynid, myUDPsockfd);
     if (myUDPsockfd < 0) {
	printf("Agent(%d): Creating myUDPsockfd failed\n", mynid);
	exit(0);
     }
     n = fcntl(myUDPsockfd, F_SETFL, O_NONBLOCK);
     while (1) {
	//gettimeofday(&now, 0);
	receiveMsg();
	usleep(100000);
       /* The above control is exerted every 0.1 second. */
    }
}
