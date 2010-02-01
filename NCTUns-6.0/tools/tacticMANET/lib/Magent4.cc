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
     ^         ^                                     ^     ^
     ^         ^          ^            ^          ^  ^     ^
     ^         ^          ^   ^^^^^^^  ^          ^        ^
     ^         ^          ^            ^          ^        ^
     ^         ^          ^            ^                   ^
     ^                                 ^                   ^
     ^     ^^^^^^^^^^^^^^^^^^^       ^^^^^^^^^^^           ^
     ^                                            MNODE(1) ^
     ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

  A mobile node is located in a closed maze filled with many 
obstacles. It chooses a random destination place (Dx, Dy) and 
moves toward that place. To get there, it uses the A* path search 
algorithm and the maze map to find a path to that destination. 
The path is composed of a sequence of turning points (X1, Y1), 
(X2, Y2), ..., (Xm == Dx, Ym = Dy). The mobile node removes the 
head point from the path and sets it as the next-point position. 
Then it moves linearly toward its next-point position. 
After reaching its next-point position, it repeats the above 
operation until the path becomes empty, which means that the 
mobile node has reached its destination point. At this time, 
the mobile node chooses a new random position and again uses 
the A* algorithm to construct a path. The above process is 
repeated until the simulation is finished.

*/

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/time.h>
#include "map.h"
#include "math_fun.h"
#include "tactic_api.h"

int mynid, n, myTCPsockfd, socketfd2, count;
double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double curspeed, distance, obsAngle, destX, destY, prevX, prevY;
PATH_LIST *pList, *filteredListP;


void findNewDestination() {

retry:
    destX = fmod((double) random(), maxXMeter);
    destY = fmod((double) random(), maxYMeter);
    if (m_Map->InObstacleGrid(destX, destY) == true)
        goto retry; // Occupied by an obstacle
    if (grid_n(destX) == grid_n(curX) && grid_n(destY) == grid_n(curY))
        goto retry; // Source and Destination fall into the same grid tile
    if (twoPointsDistance(curX, curY, destX, destY) < 200)
        goto retry; // New destination point is too near
    pList->clear();
    if (m_Map->FindPathToList(curX, curY, destX, destY, pList) == false)
        goto retry; // There is no path to reach the new destination point

    filteredListP->clear();
    prevX = curX;
    prevY = curY;

    while (!pList->empty()) {
                nextX = pList->front().first; // get the head point
                nextY = pList->front().second;
                if (twoPointsDistance(prevX, prevY, nextX, nextY) < 15) {
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


int main(int argc, char *argv[]) {

    mynid = getMyNodeID();	   
    myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(
       	mynid, 1, -1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);
    constructGridMapofTheWholeField();
    n = getInitialNodePosition(mynid, curX, curY, curZ);
    pList = new PATH_LIST();
    filteredListP = new PATH_LIST();

    // Choose a good and interesring random destination point 
    findNewDestination();

    nextX = filteredListP->front().first; // get the head point
    nextY = filteredListP->front().second;
    printf("Agent(%d): Set next waypoint to (%lf, %lf)\n", mynid, nextX, nextY);
    filteredListP->pop_front(); // remove the head point

    curspeed = 10; /* moving speed: meter/second */
    n = setNextWaypointAndMovingSpeed(myTCPsockfd, mynid, nextX, nextY, 
	nextZ, curspeed, 1);
    usleepAndReleaseCPU(myTCPsockfd, mynid, 1, 1);

    while (1) {

     n = getCurrentPosition(myTCPsockfd, mynid, curX, curY, curZ, 1);
     if (twoPointsDistance(curX, curY, nextX, nextY) <= 10) {
       /* The node has reached the next specified waypoint. Now, we
	  needs to change its waypoint based on the constructed path.
	*/
	if (filteredListP->empty()) {
	    // This node has reached the destination of the current path.
	    // Now we need to choose a new random destination point for the
	    // node and make the node move toward it as before.

	    // Choose a good and interesring random destination point
	    findNewDestination();

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


