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
     ^         ^^^^^^^^^                                   ^
     ^                                                     ^
     ^                                                     ^
     ^                 ^                                   ^
     ^                 ^      MNODE                        ^
     ^                 ^                                   ^
     ^                 ^                                   ^
     ^                                                     ^
     ^                                                     ^
     ^                                                     ^
     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  A single mobile node is located within a closed area which is surrounded 
by several obstacles. It moves linearly at a constant speed toward a direction 
until it collides with one obstacle.  At that time, it bounces back (i.e., 
changes its moving direction) and moves toward another direction. The mobile 
node continues to move in this way until the simulated time has elapsed.

*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include "tactic_api.h"

int mynid, n, myTCPsockfd;
double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double curspeed, Mdistance, obsAngle;
int socketfd2;

int main(int argc, char *argv[]) {

    mynid = getMyNodeID();
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
    usleepAndReleaseCPU(myTCPsockfd, mynid, 1, 1);

    while (1) {
     n = getCurrentPosition(myTCPsockfd, mynid, curX, curY, curZ, 1);
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
     } else {
       /* The node has not reached its next-point position. So,
	  we do nothing here to keep it moving along the current moving
	  direction.
	*/
     }
     usleepAndReleaseCPU(myTCPsockfd, mynid, 200000, 1);

   }
}


