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
   This signal agent can change the traffic light on the intersection road.
   Green	= 300
   Yellow	= 400
   Red		= 500
 */

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <fcntl.h>
#include <math.h>
#include "tactic_api.h"
#include "sock_skel.h"

#define	SHOW_LIGHT	0

int mynid, n, myTCPsockfd, groupID=0;
int socketfd2, numNodeInThisGroup;
double curX, curY, curZ, nextX, nextY, nextZ, newMovingDirectionInDegree;
double timeDiff=100000.0, lastTime=0.0;
struct timeval now;
struct agentClientGetSignalsInTheSameGroupReply sameG;

int main(int argc, char *argv[]) {
	int tempC = 0;
	mynid = getMyNodeID();	  
	// SignalAgent is in group 2
	if(argv[1] == NULL)
	{
		printf("Need groupID. Try to use \"SiganlAgent -n\" // n is groupID\n");
		groupID = -1;
	}
	for(int i=1; i<((int )strlen(argv[1])); i++)
	{
		groupID = groupID*10 + (argv[1][i] - '0');
	}
	myTCPsockfd = createTCPSocketForCommunicationWithSimulationEngine(
			mynid, -1, 1, -1, -1, -1, -1, -1, -1, 0, socketfd2, PROCESS_TYPE_AGENT, 1);
	if(groupID == -1)
		n = getSignalGroupID(myTCPsockfd, mynid, groupID, 1);
	if(n < 0){
		printf("Error in SignalAgent1:: Get signal GID fail!! \n");
		stopSimulation(myTCPsockfd, mynid);
	}
	printf("SignalAgent[%u]: groupID is %d, sockfd %d\n", mynid, groupID, myTCPsockfd);
	n = getSignalsInTheSameGroup(myTCPsockfd, mynid, &sameG, groupID, 1);
	if(n < 0){
		printf("Error in SignalAgent1:: Get signal in the same group fail!! \n");
		stopSimulation(myTCPsockfd, mynid);
	}
	for(int j=0; j<sameG.numOfSigs; j++)
		if(SHOW_LIGHT)
			printf("light %d, direction %f, sigGID %d\n", 
					sameG.light[j], sameG.facingDirection[j], sameG.sigGID);
	/* Initialize the signal light */
	for(int i=0; i<(sameG.numOfSigs); i++){
		if((sameG.facingDirection[i] == sameG.facingDirection[0]) || 
				(sameG.facingDirection[i] == fmod(sameG.facingDirection[0]+180,360))){
			setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, GREEN, 1);
			sameG.light[i] = GREEN;
		}
		else{
			setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, RED, 1);
			sameG.light[i] = RED;
		}
	}
	fflush(stdout);
	usleepAndReleaseCPU(myTCPsockfd, mynid, 1, 1);
	//If we want to support the yellow light, the combination could be list as four cases.
	//  	light	A	B
	//  	case1	r	g	maintain 30 secs
	//  	case2	r	y	maintain 10 secs
	//  	case3	g	r	maintain 30 secs
	//  	case4	y	r	maintain 10 secs
	while (1) {
		int sleepTime;
		for(int j=0; j<sameG.numOfSigs; j++)
			if(SHOW_LIGHT)
				printf("Signal [%d]: light %d\n", mynid, sameG.light[j]);
		if((tempC%4)==0){
			for(int i=0; i<sameG.numOfSigs; i++){
				if((sameG.facingDirection[i] == sameG.facingDirection[0]) || 
						(sameG.facingDirection[i] == fmod(sameG.facingDirection[0]+180,360))){
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, GREEN, 1);
					sameG.light[i] = GREEN;
				}
				else{
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, RED, 1);
					sameG.light[i] = RED;
				}
			}
			sleepTime=30000000;
		}
		if((tempC%4)==1){
			for(int i=0; i<sameG.numOfSigs; i++){
				if((sameG.facingDirection[i] == sameG.facingDirection[0]) || 
						(sameG.facingDirection[i] == fmod(sameG.facingDirection[0]+180,360))){
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, YELLOW, 1);
					sameG.light[i] = YELLOW;
				}
				else{
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, RED, 1);
					sameG.light[i] = RED;
				}
			}
			sleepTime=10000000;
		}
		else if((tempC%4)==2){
			for(int i=0; i<sameG.numOfSigs; i++){
				if((sameG.facingDirection[i] == sameG.facingDirection[0]) || 
						(sameG.facingDirection[i] == fmod(sameG.facingDirection[0]+180,360))){
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, RED, 1);
					sameG.light[i] = RED;
				}
				else{
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, GREEN, 1);
					sameG.light[i] = GREEN;
				}
			}
			sleepTime=30000000;
		}
		else if((tempC%4)==3){
			for(int i=0; i<sameG.numOfSigs; i++){
				if((sameG.facingDirection[i] == sameG.facingDirection[0]) || 
						(sameG.facingDirection[i] == fmod(sameG.facingDirection[0]+180,360))){
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, RED, 1);
					sameG.light[i] = RED;
				}
				else{
					setSignalLight(myTCPsockfd, mynid, sameG.sigGID, i, YELLOW, 1);
					sameG.light[i] = YELLOW;
				}
			}
			sleepTime=10000000;
		}
		tempC++;
		usleepAndReleaseCPU(myTCPsockfd, mynid, sleepTime, 1);
		/* Change the traffic light per 20 seconds. */
	}
}
