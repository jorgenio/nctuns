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
     ^                                 MNODE(2)            ^
     ^                                                     ^
     ^                                                     ^
     ^                        MNOD(1)                      ^
     ^                                                     ^
     ^                                                     ^
     ^                                                     ^
     ^                                  MNOD(3)            ^
     ^                                                     ^
     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                                                                
  Multiple mobile nodes are located within a closed field whose
borders are occuiped by obstacles. Each of them move linearly
at a constant speed toward a moving direction until it is about
to collide with one obstacle or another mobile node. At that time,
it changes its current moving direction and and moves toward a new
direction. When two mobile nodes collide, they exchange their moving
directions to simulate the "bouncing" effect. These mobile nodes continue 
to move until the simulated time has elapsed. In this case, all mobile 
nodes belong to group 1.

*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include "tactic_api.h"

u_int32_t mynid, n, myTCPsockfd;
double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double curspeed, Mdistance, obsAngle, aangle;
double ax, ay;
int socketfd2, numNodeInThisGroup;
struct nodePosition *NPArray, *tmpNPArray;
int i, downCount = -1, downCount2 = -1;
double minDist = 99999999;
u_int32_t ignoreID;

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
     usleepAndReleaseCPU(myTCPsockfd, mynid, 2000000, 1);

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

       if (downCount >= 0) goto skip;
       n = getCurrentPositionOfAGroupOfNode(myTCPsockfd, mynid, 1, &NPArray, 
	  numNodeInThisGroup, 1);
       tmpNPArray = NPArray;
       for (i=0; i<numNodeInThisGroup; i++) {
	  ax = NPArray->x;
	  ay = NPArray->y;
	  if (NPArray->nid == mynid) {
		// A mobile node should not check the withinRange with itself.
		NPArray++;
		continue;
	  }
	  if ((NPArray->nid == ignoreID) && (downCount2 > 0)) {
		// After just changing the moving direction,
		// a mobile node should temporarily ignore the withinRange 
		// alerts caused by the node that just collided with itself. 
	        // Withdoing so, we will see that the two mobile nodes keep
		// exchanging their moving directions until they eventually
		// move out of the withinRange (20 m) distance from each other.
		// 
		// The ignoring period is downCount2 = 10 checking intervals
		// long.
		downCount2--;
		NPArray++;
		continue;
	  }
	  Mdistance = twoPointsDistance(curX, curY, ax, ay);
	  if ((Mdistance < 20) && (Mdistance < minDist)) {

	     // Find the nearest mobile node that has approached me.
	     // To allow the two colliding mobile nodes to have time to
	     // exchange their moving direction, a mobile node should not
	     // change its moving angle to the moving direction of the other
	     // mobile node immediately. Otherwise, due to the detection time
	     // difference, the second mobile node will get the new moving
	     // direction of the first mobile node (which is the same as its
	     // current moving direction) resulting that the two mobile nodes
	     // move along the same moving direction of the second mobile node. 
	     // 
	     // For this reason, the actual moving direction change is delayed
	     // and performed downCount = 10 checking intervals later.
	     //
	     // Even with this careful design, sometimes the above problem
	     // still happens. This is because in some rare cases, 
	     // due to the detection time differences between the two mobile 
	     // nodes and their relative high speed movements, the first 
	     // mobile node may detect the second mobile node but the second 
	     // mobile node may not detect the first mobile node. This 
	     // phenomenon is normal and can be explained.

	     minDist = Mdistance;
	     n = getCurrentMovingDirectionViewedOnGUIScreen(myTCPsockfd, 
	       NPArray->nid, aangle, 1);
	     ignoreID = NPArray->nid;
	     downCount = 10;
	  }
	  NPArray++;
       }
       free((char *) tmpNPArray);
skip:
       downCount--;
       if (downCount == 0) {
	     downCount2 = 10;
	     newMovingDirectionInDegree = aangle;
	     getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(
		curX, curY, curZ, newMovingDirectionInDegree, nextX, nextY, 
		nextZ, Mdistance, obsAngle);
	     n = setNextWaypointAndMovingSpeed(myTCPsockfd, mynid, nextX, 
		nextY, nextZ, 10, 1);
       }
       usleepAndReleaseCPU(myTCPsockfd, mynid, 100000, 1);
    }
}


