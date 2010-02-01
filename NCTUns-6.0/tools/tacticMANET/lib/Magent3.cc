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

     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
     ^                                                     ^
     ^                                                     ^
     ^               MNODE(4)          MNODE(2)            ^
     ^                                                     ^
     ^                                                     ^
     ^                        MNODE(1)                     ^
     ^                                                     ^
     ^                                                     ^
     ^                                                     ^
     ^               MNODE(5)           MNODE(3)           ^
     ^                                                     ^
     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                                                                
Multiple mobile nodes are located within a closed field. They form a group 
and move in the same direction. Initially, they all move in a randomly chosen 
moving direction. When any mobile node is about to collide with an obstacle, 
it changes its moving direction and sends a message to all other mobile nodes 
to notify them of this new moving direction. Upon receiving the message, each 
mobile node sets its moving direction to the one just received and keeps 
moving.  The whole process repeats until the simulated time has elapsed.

In this case, all mobile nodes belong to group 1. Initially, all mobile nodes 
are placed within the wireless transmission range of each other so that they 
can receive messages from each other.  The routing protocol used among all 
mobile nodes can be AODV.

*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include "tactic_api.h"
#include "sock_skel.h"

u_int32_t mynid;
int n, myTCPsockfd;
double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double curspeed, Mdistance, obsAngle, aangle;
double ax, ay, az, dx, dy, dz;
int socketfd2, numNodeInThisGroup;
struct nodePosition *NPArray, *tmpNPArray;
int i, downCount = -1, downCount2 = -1;
double minDist = 99999999;
int ignoreID, myUDPsockfd;
char portNumStr[32];
struct GUIChangeNodeMovingDirection msg;
struct sockaddr_in cli_addr;
int len = sizeof(struct sockaddr);
int agentUDPportNum = 4000;
char hostname[128];

int main(int argc, char *argv[]) {

     mynid = getMyNodeID();	   

     sprintf(portNumStr, "%d", agentUDPportNum);
     myUDPsockfd = passiveUDP(portNumStr); 
     printf("Agent(%d) created myUDPsockfd %d\n", mynid, myUDPsockfd);
     if (myUDPsockfd < 0) {
	printf("Agent(%d): Creating myUDPsockfd failed\n", mynid);
	exit(0);
     }
     n = fcntl(myUDPsockfd, F_SETFL, O_NONBLOCK);

     myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(
       	mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);
     constructGridMapofTheWholeField();
     n = getInitialNodePosition(mynid, curX, curY, curZ);
     newMovingDirectionInDegree = random() % 360;
     getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(
	curX, curY, curZ, newMovingDirectionInDegree, nextX, nextY, 
	nextZ, Mdistance, obsAngle);
     curspeed = 10; /* moving speed: meter/second */
     n = setNextWaypointAndMovingSpeed(myTCPsockfd, mynid, nextX, nextY, 
	nextZ, curspeed, 1);
     usleepAndReleaseCPU(myTCPsockfd, mynid, 2000000, 1);

     while (1) {

       n = getCurrentPosition(myTCPsockfd, mynid, curX, curY, curZ, 1);

       // Check whether some agent has sent me a message
       len = sizeof(struct sockaddr);
       n = recvfrom(myUDPsockfd, &msg, sizeof(struct 
	 GUIChangeNodeMovingDirection), 0, (struct sockaddr *) &cli_addr, 
	 (socklen_t*)&len); 

       if (n > 0) { // Some one has sent me a message asking me to change
		    // my moving direction.
	 if (msg.type != GUI_CHANGE_NODE_MOVING_DIRECTION) {
	   printf("Agent(%d): msg.type != GUI_CHANGE_NODE_MOVING_DIRECTION\n", 
	     mynid);
	   exit(0);
	 }
	 printf("Agent(%d) received a new angle %lf degree from Agent(%d)\n",
		mynid, msg.angle, msg.nid);
	 newMovingDirectionInDegree = msg.angle;
         getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(
           curX, curY, curZ, newMovingDirectionInDegree, nextX, nextY,
           nextZ, Mdistance, obsAngle);
         n = setNextWaypoint(myTCPsockfd, mynid, nextX, nextY, nextZ, 1);
	 goto skip;
       } else { // No message has arrived so we do nothing special here.
       }

       if (twoPointsDistance(curX, curY, nextX, nextY) <= 10) {
         /* The node has reached the next specified waypoint. Now, we
            needs to change its moving direction.
          */
         newMovingDirectionInDegree = (2 * obsAngle - 
	   newMovingDirectionInDegree);
         if (newMovingDirectionInDegree < 0) newMovingDirectionInDegree += 360;
         else {
           newMovingDirectionInDegree = fmod(newMovingDirectionInDegree, 360);
         }
         getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(
           curX, curY, curZ, newMovingDirectionInDegree, nextX, nextY,
           nextZ, Mdistance, obsAngle);
         n = setNextWaypoint(myTCPsockfd, mynid, nextX, nextY, nextZ, 1);

         // We also send a message to every other agent that is current up
	 // to ask it to move in my new moving direction.
         n = getCurrentPositionOfAGroupOfNode(myTCPsockfd, mynid, 1, &NPArray,
           numNodeInThisGroup, 1);
         tmpNPArray = NPArray;

	 msg.type = GUI_CHANGE_NODE_MOVING_DIRECTION;
	 msg.nid = mynid;
	 msg.angle = newMovingDirectionInDegree;

         for (i=0; i<numNodeInThisGroup; i++) {

           if (NPArray->nid == mynid) {
                // A mobile node should not send a message to itself.
                NPArray++;
                continue;
           }

	   sprintf(hostname, "1.0.1.%d", NPArray->nid);
           memset( &cli_addr, 0, sizeof(cli_addr) );
           cli_addr.sin_family = AF_INET;
           cli_addr.sin_port   = htons(agentUDPportNum);
           cli_addr.sin_addr.s_addr = inet_addr( hostname );
                                                                                
	   len = sizeof(struct sockaddr);
	   n = sendto(myUDPsockfd, &msg, sizeof(struct 
	     GUIChangeNodeMovingDirection), 0, (struct sockaddr *) &cli_addr, 
	     len);
	   if (n < 0) {
	     printf("Agent(%d) Sendto() failed\n", mynid);
	     exit(0);
	   }
           NPArray++;

         }
         free((char *) tmpNPArray);

       } else {
           /* The node has not reached its next-point position. So,
              we do nothing here to keep it moving along the current moving
              direction.
            */
       }

skip:
       usleepAndReleaseCPU(myTCPsockfd, mynid, 100000, 1);
    }
}


