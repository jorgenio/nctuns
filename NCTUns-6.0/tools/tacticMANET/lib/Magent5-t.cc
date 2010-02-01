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
     ^         ^  MNODE(2)                           ^     ^
     ^         ^          ^            ^          ^  ^     ^
     ^         ^          ^   ^^^^^^^  ^          ^        ^
     ^         ^          ^            ^          ^        ^
     ^         ^          ^            ^                   ^
     ^                                 ^                   ^
     ^     ^^^^^^^^^^^^^^^^^^^       ^^^^^^^^^^^           ^
     ^                                            MNODE(1) ^
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
an A* path to chase the target node. Then it moves along the path to tryy to 
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

double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double curspeed, Mdistance, minDist, obsAngle, destX, destY, prevX, prevY;
double chaseX, chaseY, dangerX, dangerY, pl;
double tlx, tly, blx, bly, trx, tryy, brx, bry;
struct nodePosition *NPArray, *tmpNPArray;
int dangerNid, i, mynid, n, myTCPsockfd, socketfd2, count, numNodeInThisGroup;
int delayCount = 0;
PATH_LIST *pList, *filteredListP, *tochaseP, *filteredToChaseP;



void filterPathList(double startx, double starty, PATH_LIST *pList, 
PATH_LIST *filteredListP) {

double nextX, nextY, prevX, prevY;

    prevX = startx;
    prevY = starty;
    while (!pList->empty()) {
	nextX = pList->front().first; // get the head point
	nextY = pList->front().second;
	if ( pList->size()>5 && twoPointsDistance(prevX, prevY, nextX, nextY) < 10) {
		// Filter out consecutive waypoint points that are too
		// close to each other
	} else {
		LOCATION loc(nextX, nextY);
		filteredListP->push_back(loc);
		prevX = nextX;
		prevY = nextY;
	}
	pList->pop_front();
    }

}


void findNewDestination(double hintx, double hinty) {

int count1 = 0, count2 = 0;

    if (m_Map->InObstacleGrid(curX, curY) == true)
    {
//    	printf("%s #%d InObstacleGrid (%lf, %lf)\n", __FILE__, __LINE__, curX, curY);
        if (m_Map->InObstacleGrid(curX+10.0, curY+10.0) == false)
	{
		curX += 10.0;
		curY += 10.0;
	} else if (m_Map->InObstacleGrid(curX-10.0, curY+10.0) == false)
	{
		curX -= 10.0;
		curY += 10.0;
	} else if (m_Map->InObstacleGrid(curX+10.0, curY-10.0) == false)
	{
		curX += 10.0;
		curY -= 10.0;
	} else if (m_Map->InObstacleGrid(curX-10.0, curY-10.0) == false)
	{
		curX -= 10.0;
		curY -= 10.0;
	}
//        printf("(3) Now Target is in obstacle grid: (%lf, %lf)\n", curX, curY);
    }

retryy:

    if (count1 > 2 || count2 > 20) {
	printf("Agent(%d) findNewDestination() failed. curX %lf curY %lf\n", mynid, curX, curY);
	stopSimulation(myTCPsockfd, mynid);
	sleep(10);
    }

	if (count1 == 0 && count2 == 0) {
		if (hintx > 0) {
		    destX = hintx;
		} else {
		    destX = fmod((double) random(), maxXMeter);
		}
		if (hinty > 0) {
		    destY = hinty;
		} else {
		    destY = fmod((double) random(), maxYMeter);
		}
        } else {
		destX = fmod((double) random(), maxXMeter);
		destY = fmod((double) random(), maxYMeter);
	}
    count2++;
    if (m_Map->InObstacleGrid(destX, destY) == true)
        goto retryy; // Occupied by an obstacle
    if (grid_n(destX) == grid_n(curX) && grid_n(destY) == grid_n(curY))
        goto retryy; // Source and Destination fall into the same grid tile
    if (twoPointsDistance(curX, curY, destX, destY) < 200)
        goto retryy; // New destination point is too near
    pList->clear();
    if (m_Map->FindPathToList(curX, curY, destX, destY, pList) == false) {
        count1++;
        goto retryy; // There is no path to reach the new destination point
    }
    filteredListP->clear();
//    delete filteredListP;
//    filteredListP = new PATH_LIST;
    filterPathList(curX, curY, pList, filteredListP);
}


double pathLength(PATH_LIST *filteredListp) {

double dist, totalDist, lprevX, lprevY, lnextX, lnextY;
PathListItor pl;

    if (filteredListp->empty()) {
	return(0);
    }

    pl = filteredListp->begin();
    lprevX = pl->first; 
    lprevY = pl->second; 

    totalDist = 0;
    while (pl != filteredListp->end()) {

	count++;
	lnextX = pl->first; // get the head point
	lnextY = pl->second;
	dist = twoPointsDistance(lprevX, lprevY, lnextX, lnextY); 
	totalDist += dist;
	lprevX = lnextX;
	lprevY = lnextY;

	pl++;
    }
    return(totalDist);
}


double minPointToPathDistance(double dangerX, double dangerY, PATH_LIST 
*filteredListp) {

double minDist, dist, d1, d2, x, y, prevX, prevY, prox, proy;
int n;
PathListItor pl;

    prevX = filteredListp->front().first; // get the head point
    prevY = filteredListp->front().second;
//    filteredListp->pop_front();

    pl = filteredListp->begin();
    pl++;
    minDist = 9999999; // infinity
    while (pl != filteredListp->end()) {
	x = pl->first; // get the head point
	y = pl->second;

	n = NodeProjection_InLineSegment(dangerX, dangerY, prevX, prevY, x, y, 
	  prox, proy);
	if (n == 1) {
	  dist = twoPointsDistance(dangerX, dangerY, prox, proy);
	} else {
	  d1 = twoPointsDistance(dangerX, dangerY, prevX, prevY);
	  d2 = twoPointsDistance(dangerX, dangerY, x, y);
	  if (d1 < d2) dist = d1;
	  else dist = d2;
	}
	if (dist < minDist) minDist = dist;
	prevX = x;
	prevY = y;

	pl++;
    }
    return(minDist);
}


void findFourCornerPoints() {

	tlx = 0;
	tly = 0;
	while (m_Map->InObstacleGrid(tlx, tly) == true) {
		tlx += gridWidthMeter;
		tly += gridWidthMeter;
	}

	trx = maxXMeter;
	tryy = 0;
	while (m_Map->InObstacleGrid(trx, tryy) == true) {
		trx -= gridWidthMeter;
		tryy += gridWidthMeter;
	}

	blx = 0;
	bly = maxYMeter;
	while (m_Map->InObstacleGrid(blx, bly) == true) {
		blx += gridWidthMeter;
		bly -= gridWidthMeter;
	}

	brx = maxXMeter;
	bry = maxYMeter;
	while (m_Map->InObstacleGrid(brx, bry) == true) {
		brx -= gridWidthMeter;
		bry -= gridWidthMeter;
	}

}


int main(int argc, char *argv[]) {

    mynid = getMyNodeID();	   
    myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(
       	mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);
    constructGridMapofTheWholeField();
    findFourCornerPoints();

    n = getInitialNodePosition(mynid, curX, curY, curZ);
    pList = new PATH_LIST();
    filteredListP = new PATH_LIST();
    tochaseP = new PATH_LIST();
    filteredToChaseP = new PATH_LIST();

    // Choose a random destination point 
    findNewDestination(0, 0);

    nextX = filteredListP->front().first; // get the head point
    nextY = filteredListP->front().second;
    filteredListP->pop_front(); // remove the head point

    curspeed = 15; /* moving speed: meter/second */
    n = setNextWaypointAndMovingSpeed(myTCPsockfd, mynid, nextX, nextY, 
	nextZ, curspeed, 1);
    usleepAndReleaseCPU(myTCPsockfd, mynid, 1, 1);

    while (1) {
     n = getCurrentPosition(myTCPsockfd, mynid, curX, curY, curZ, 1);
     delayCount--;
     if (delayCount > 0) goto skipApproachChecking;

     n = getCurrentPositionOfAGroupOfNode(myTCPsockfd, mynid, 2, &NPArray,
       numNodeInThisGroup, 1);
     tmpNPArray = NPArray;

     minDist = 9999999; // infinity
     dangerNid = -1; // Assuming none

     for (i = 0; i < numNodeInThisGroup; i++) {

       chaseX = NPArray->x;
       chaseY = NPArray->y;

      if (m_Map->InObstacleGrid(chaseX, chaseY) == true)
      {
//        printf("(3) Target is in obstacle grid: (%lf, %lf)\n", chaseX, chaseY);

        if (m_Map->InObstacleGrid(chaseX+10.0, chaseY+10.0) == false)
	{
		chaseX += 10.0;
		chaseY += 10.0;
	} else if (m_Map->InObstacleGrid(chaseX-10.0, chaseY+10.0) == false)
	{
		chaseX -= 10.0;
		chaseY += 10.0;
	} else if (m_Map->InObstacleGrid(chaseX+10.0, chaseY-10.0) == false)
	{
		chaseX += 10.0;
		chaseY -= 10.0;
	} else if (m_Map->InObstacleGrid(chaseX-10.0, chaseY-10.0) == false)
	{
		chaseX -= 10.0;
		chaseY -= 10.0;
	}
//        printf("(3) Now Target is in obstacle grid: (%lf, %lf)\n", chaseX, chaseY);
      }
      if (m_Map->InObstacleGrid(curX, curY) == true)
      {
        //printf("(4) Curr is in obstacle grid: (%lf, %lf)\n", curX, curY);
      }

       Mdistance = twoPointsDistance(curX, curY, chaseX, chaseY);
       if (Mdistance > 200) {
	// The physical distance between the two nodes is > 100 
        // No immediate threat at the present time
         NPArray++;
         continue;
       } else if (Mdistance < minDist) {
	 tochaseP->clear();
	 m_Map->FindPathToList(curX, curY, chaseX, chaseY, tochaseP);
	 filteredToChaseP->clear();
	 filterPathList(curX, curY, tochaseP, filteredToChaseP);
         pl = pathLength(filteredToChaseP);
	 if (pl > 200) {
	   // The length of the path connecting the two nodes is > 100
	   // No immediate threat at the present time
           NPArray++;
           continue;
	 } else if (pl < minDist) {
	   // Identify the most dangerous (nearest) chasing node
	   minDist = pl;
	   dangerX = chaseX;
	   dangerY = chaseY;
	   dangerNid = NPArray->nid;
	 }
       }
       NPArray++;

     }
     free((char *) tmpNPArray);

     if (dangerNid > 0) {
	// There exists a dangerous chasing node.
	
	// Check the directions to the four corners and see which way is
	// good enough.
	Mdistance = 0;
	if (Mdistance < 100) {
	  if (twoPointsDistance(tlx, tly, curX, curY) > 210 ) {
	    findNewDestination(tlx, tly);
	    Mdistance = minPointToPathDistance(dangerX, dangerY, filteredListP);
	  } 
   	}
	if (Mdistance < 100) {
	  if (twoPointsDistance(trx, tryy, curX, curY) > 210 ) {
	    findNewDestination(trx, tryy);
	    Mdistance = minPointToPathDistance(dangerX, dangerY, filteredListP);
	  } 
	}
	if (Mdistance < 100) {
	  if (twoPointsDistance(brx, bry, curX, curY) > 210 ) {
	    findNewDestination(brx, bry);
	    Mdistance = minPointToPathDistance(dangerX, dangerY, filteredListP);
	  } 
	}
	if (Mdistance < 100) {
	  if (twoPointsDistance(blx, bly, curX, curY) > 210 ) {
	    findNewDestination(blx, bly);
	    Mdistance = minPointToPathDistance(dangerX, dangerY, filteredListP);
	  } 
	}

	delayCount = 60; // Allow the new escaping plan some time to take effect
	nextX = filteredListP->front().first; // get the head point
	nextY = filteredListP->front().second;
	filteredListP->pop_front(); // remove the head point
	n = setNextWaypoint(myTCPsockfd, mynid, nextX, nextY, nextZ, 1);
        usleepAndReleaseCPU(myTCPsockfd, mynid, 200000, 1);
	continue;
     }

skipApproachChecking:

     if (twoPointsDistance(curX, curY, nextX, nextY) <= 10) {
       /* The node has reached the next specified waypoint. Now, we
	  needs to change its waypoint based on the constructed path.
	*/
	if (filteredListP->empty()) {
	    // This node has reached the destination of the current path.
	    // Now we need to choose a new random destination point for the
	    // node and make the node move toward it as before.

	    // Choose a random destination point
	    findNewDestination(0, 0);
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


