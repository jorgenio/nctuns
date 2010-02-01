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
     ^         ^                                     ^     ^
     ^         ^     ^^^^^^^^^^^^^^^^^^^^^^^^^^      ^     ^
     ^         ^  MNODE(1)                           ^     ^
     ^         ^          ^            ^          ^  ^     ^
     ^         ^          ^   ^^^^^^^  ^          ^        ^
     ^         ^          ^            ^          ^        ^
     ^         ^          ^            ^                   ^
     ^                                 ^                   ^
     ^     ^^^^^^^^^^^^^^^^^^^       ^^^^^^^^^^^           ^
     ^                                            MNODE(2) ^
     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
                                                                                                                            
  Two mobile nodes are located in a maze filled with many obstacles.
One mobile node (here MNODE(2)) is the target node while the other node
is the chasing node. During the simulation, each mobile node knows the
position of the other node at all time (assuming that this location
information is provided by a third body through a satellite). With this
information on hand, the target node tries to run away from the chasing node
in the opposite direction of the chasing node. During the simulation, the
target node detects the position of the chasing node and recalculates a
new escaping path when the distance between the two nodes falls within a
threshold.
                                                                                
  The behavior of the chasing node is described as follows. Initially, it
uses the position of the chasing node as the destination point to construct
an A* path to chase the target node. Then it moves along the path to try to
capture the target node. During the chasing, it detects the position of the
target node and recalculates a new A* path to chase the target node once
every a few seconds.
                                                                                
In this case, the target node belongs to group 1 while the chasing node
belongs to group 2. The moving speed of the target node is set to a value
higher than that set to the chasing node. The chasing process continues
until the target node is captured (collided with) by the chasing node.


*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "map.h"
#include "math_fun.h"
#include "tactic_api.h"

int dangerNid, i, mynid, n, myTCPsockfd, socketfd2, count, numNodeInThisGroup;
double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double curspeed, minDist, obsAngle, destX, destY, prevX, prevY;
double targetX, targetY, dangerX, dangerY, pl;
PATH_LIST *pList, *filteredListP, *chaseP, *filteredToChaseP;
struct nodePosition *NPArray;
int recalculateCount;

void filterPathList(double startx, double starty, PATH_LIST *pList, 
PATH_LIST *filteredListP) {

double nextX, nextY, prevX, prevY;

    prevX = startx;
    prevY = starty;
    while (!pList->empty()) {
                nextX = pList->front().first; // get the head point
                nextY = pList->front().second;
                if (twoPointsDistance(prevX, prevY, nextX, nextY) < 10) {
                        // Filter out consecutive waypoint points that are too
                        // close to each other
                        pList->pop_front();
                        continue;
                } else {
                        LOCATION loc(nextX, nextY);
                        filteredListP->push_back(loc);
                        pList->pop_front();
                        prevX = nextX;
                        prevY = nextY;
                }
    }

}


void findNewDestination() {

int count1 = 0, count2 = 0;

retry:
                                                                                
    if (count1 > 3 || count2 > 10) {
        stopSimulation(myTCPsockfd, mynid);
	sleep(10);
    }
    count2++;
    destX = fmod((double) random(), maxXMeter);
    destY = fmod((double) random(), maxYMeter);
    if (m_Map->InObstacleGrid(destX, destY) == true)
        goto retry; // Occupied by an obstacle
    if (grid_n(destX) == grid_n(curX) && grid_n(destY) == grid_n(curY))
        goto retry; // Source and Destination fall into the same grid tile
    if (twoPointsDistance(curX, curY, destX, destY) < 100)
        goto retry; // New destination point is too near
    pList->clear();
    if (m_Map->FindPathToList(curX, curY, destX, destY, pList) == false) {
        count1++;
        goto retry; // There is no path to reach the new destination point
    }
    filteredListP->clear();
    filterPathList(curX, curY, pList, filteredListP);
}


int main(int argc, char *argv[]) {

    mynid = getMyNodeID();	   
    myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(
       	mynid, -1, 1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);
    constructGridMapofTheWholeField();
    n = getInitialNodePosition(mynid, curX, curY, curZ);
    pList = new PATH_LIST();
    filteredListP = new PATH_LIST();
    chaseP = new PATH_LIST();

    // Choose a random destination point 
    findNewDestination();

    nextX = filteredListP->front().first; // get the head point
    nextY = filteredListP->front().second;
    filteredListP->pop_front(); // remove the head point

    curspeed = 10; /* moving speed: meter/second */
    n = setNextWaypointAndMovingSpeed(myTCPsockfd, mynid, nextX, nextY, 
	nextZ, curspeed, 1);
    usleepAndReleaseCPU(myTCPsockfd, mynid, 1, 1);

    recalculateCount = 5 * 10; // every 10 seconds

    while (1) {
      n = getCurrentPosition(myTCPsockfd, mynid, curX, curY, curZ, 1);
      n = getCurrentPositionOfAGroupOfNode(myTCPsockfd, mynid, 1, &NPArray,
        numNodeInThisGroup, 1);
                                                                                
      targetX = NPArray->x;
      targetY = NPArray->y;
      free((char *) NPArray);

      if (m_Map->InObstacleGrid(targetX, targetY) == true)
      {
	for (int k=1; k<10; k++)
	{
		double off = 10.0*k;
		if (m_Map->InObstacleGrid(targetX+off, targetY+off) == false)
		{
			targetX += off;
			targetY += off;
		} else if (m_Map->InObstacleGrid(targetX-off, targetY+off) == false)
		{
			targetX -= off;
			targetY += off;
		} else if (m_Map->InObstacleGrid(targetX+off, targetY-off) == false)
		{
			targetX += off;
			targetY -= off;
		} else if (m_Map->InObstacleGrid(targetX-off, targetY-off) == false)
		{
			targetX -= off;
			targetY -= off;
		}
		else
		{
			continue;
		}
		break;
	}
      }
      if (m_Map->InObstacleGrid(curX, curY) == true)
      {
        //printf("(1) Curr is in obstacle grid: (%lf, %lf)\n", curX, curY);
      }
	
      if (twoPointsDistance(curX, curY, targetX, targetY) <= 15) {
	printf("Agent(%d) has captured the target node.\n", mynid);
	stopSimulation(myTCPsockfd, mynid);	
	sleep(10);
      } 

      recalculateCount--;

      if (recalculateCount == 0) {
        recalculateCount = 50;

	chaseP->clear();
	m_Map->FindPathToList(curX, curY, targetX, targetY, chaseP);
	
	filteredListP->clear();
//	delete filteredListP;	filteredListP = new PATH_LIST();
	filterPathList(curX, curY, chaseP, filteredListP);

	nextX = filteredListP->front().first; // get the head point
	nextY = filteredListP->front().second;
	filteredListP->pop_front(); // remove the head point
	n = setNextWaypoint(myTCPsockfd, mynid, nextX, nextY, nextZ, 1);
        usleepAndReleaseCPU(myTCPsockfd, mynid, 200000, 1);
	continue;
     }

     if (twoPointsDistance(curX, curY, nextX, nextY) <= 10) {
       /* The node has reached the next specified waypoint. Now, we
	  needs to change its waypoint based on the constructed path.
	*/
	if (filteredListP->empty()) {
	    // This node has reached the destination of the current path.
	    // Now we need to find a new path to chase the target node. 

          n = getCurrentPositionOfAGroupOfNode(myTCPsockfd, mynid, 1, &NPArray,
            numNodeInThisGroup, 1);
                                                                                
          targetX = NPArray->x;
          targetY = NPArray->y;
          free((char *) NPArray);

          chaseP->clear();
          m_Map->FindPathToList(curX, curY, targetX, targetY, chaseP);
                                                                                
          filteredListP->clear();
          filterPathList(curX, curY, chaseP, filteredListP);

  	} else {
	    // There is still a waypoint in the current constructed path,
	    // so we need not construct a new path.
	}
	nextX = filteredListP->front().first; // get the head point
	nextY = filteredListP->front().second;
	filteredListP->pop_front(); // remove the head point
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


