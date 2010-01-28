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
	The API functions provided in this file can be called by a 
	user-level agent program to support dynamic tactic mobile
	ad hoc network researches.
 
        A programmer can include this file into his (her) agent program 
	so that his (her) agent program can call these API functions 
	to achieve a certin goal. 

	Some functions are IPC commands exchanged between the calling
	agent program and the simulation engine while some others
	are not.
*/

#ifndef _NCTUNS_TACTIC_API_H
#define _NCTUNS_TACTIC_API_H

#include <vector>
#include "command_format.h"
#include "auto_vehicle_signal.h"

#define	gridWidthMeter		5	// the default value
#define CAN_NOT_CORRECT		-1
#define CORRECT_LATER		0
#define CORRECT_SUCCESS		1
#define CAR_AGENT_GROUP_NUMBER  1
#define SIG_AGENT_GROUP_NUMBER  2
#define RSU_AGENT_GROUP_NUMBER  3
extern double 			maxXMeter, maxYMeter, maxZMeter;
extern int 			maxXGrid, maxYGrid, maxZGrid;

/* Helper function */
char* GetWorkingDir();
                                                                                                                            
/* Helper function */
char* agentGetScriptName();

/* Helper function */
void read_obstacles(FILE *obs_fptr);

/*
 * The following functions are wrote by automatic vehicle group. at 2007/03/01
 */
char *getProfileName(int gid);

int getProFileData(int gid, double &MaxSpeed, double &MaxAcceleration, 
		double &MinAcceleration, int IsAgentClient);

int createTCPSocketForCommunicationWithSimulationEngine(int mynid, int gid1, 
	int gid2, int gid3, int gid4, int gid5, int gid6, int gid7, int gid8,
	int humanControlled, int &UDPsocketFd2, int proc_type, int moreMsgFollowing);

unsigned long getMyNodeID();

void constructGridMapofTheWholeField();

int getInitialNodePosition(u_int32_t mynid, double &initX, double &initY, double &initZ);

void getNearestCollisionPointAlongTheMovingDirectionViewedOnGUIScreen(double 
	curX, double curY, double curZ, double 
	movingDirectionInDegreeViewedOnGUIScreen, double &collisionX, double 
	&collisionY, double &collisionZ, double &distance, double &obstacleAngle);

void getFarthestVisablePointAlongTheMovingDirectionViewedOnGUIScreen(double
	curX, double curY, double curZ, double 
	movingDirectionInDegreeViewedOnGUIScreen, double &visualX, double &visualY, 
	double &visualZ, double &distance);

double twoPointsDistance(double x1, double y1, double x2, double y2);

/* Helper function */
unsigned char getGridElement(int xGrid, int yGrid);

int inObstacleGridLocation(double x, double y, double z);

int setCurrentWaypoint(int myTCPsockfd, u_int32_t mynid, double x, double y, 
	double z, int moreMsgFollowing);

int getCurrentWaypoint(int myTCPsockfd, int mynid, double &x, double &y, 
	double &z, int moreMsgFollowing);

int setNextWaypoint(int myTCPsockfd, int mynid, double x, double y, 
	double z, int moreMsgFollowing);

int getCurrentMovingDirectionViewedOnGUIScreen(int myTCPsockfd, int mynid,
	double &angle, int moreMsgFollowing);

int setCurrentAndNextWaypoint(int myTCPsockfd, int mynid, double curX, 
	double curY, double curZ, double nextX, double nextY, double nextZ, 
	int moreMsgFollowing);

int setCurrentMovingSpeed(int myTCPsockfd, int mynid, double curSpeed, 
	int moreMsgFollowing);

int getCurrentMovingSpeed(int myTCPsockfd, int mynid, double &curSpeed,
	int moreMsgFollowing);

int setNextWaypointAndMovingSpeed(int myTCPsockfd, int mynid, double nextX, 
	double nextY, double nextZ, double curSpeed, int moreMsgFollowing);

int getCurrentPosition(int myTCPsockfd, int mynid, double &curX, double &curY, 
	double &curZ, int moreMsgFollowing);

int getCurrentPositionOfAGroupOfNode(int myTCPsockfd, int mynid, int gid, 
	struct nodePosition **groupNodePositionArray, int &numNodeInThisArray, 
	int moreMsgFollowing);

void getVisableMobileNodesFromThePosition(double x, double y, double z, 
	struct nodePosition *groupNodePositionArray, int numNodeInTheArray, struct 
	nodePosition **visableNodePositionArray, int &numNodeVisable);

void getSurroundingViews(double x, double y, double z, double 
	**viewDistance360Degree);

int requestNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd, 
	int mynid, int registrationID, int timeIntervalInMilliseconds, 
	int anotherNodeID, int gid, double withinRangeinMeter, int moreMsgFollowing);

int cancelNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd,
	int mynid, int registrationID, int moreMsgFollowing);

int suspendNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd,
	int mynid, int registrationID, int timeIntervalInMilliseconds,
	int moreMsgFollowing);

int resumeNotificationWhenAnotherNodeHasApproachedMe(int myTCPsockfd,
	int mynid, int registrationID, int moreMsgFollowing);

int destroyMobileNode(int myTCPsockfd, int targetNid, int moreMsgFollowing);

int stopSimulation(int myTCPsockfd, int mynid);

void usleepAndReleaseCPU(int myTCPsockfd, int mynid, int usecond, int pre_schedule_select);

int selectAndReleaseCPU(int myTCPsockfd, int mynid, int nfds, 
		fd_set *inp, fd_set *outp, fd_set *exp, timeval *tvp);

int getApproachingNotificationInformation(int myTCPsockfd, int &anid, 
	double &ax, double &ay, double &az, double &aspeed, double &aangle, 
	double &distance);

/* the following functions are wrote by automatic vehicle group. at 2007/03/01 */
void constructRoadMapofTheWholeField(int IsAgentClient);

int setCurrentSpeedAcceleration(int myTCPsockfd, int mynid, double 
	speedAcceleration, int moreMsgFollowing);

int setCurrentMovingDirectionViewedOnGUIScreen(int myTCPsockfd, int mynid,
double angle, int moreMsgFollowing);

/* the following APIs are added by mshsu. at 2006.07.24 */
int getSignalGroupID(int myTCPsockfd, int mynid, int &sigGID, int moreMsgFollowing);

int setSignalLight(int myTCPsockfd, int mynid, int sigGID, int signalIndex, int light, int moreMsgFollowing);

int getSignalsInTheSameGroup(int myTCPsockfd, int mynid, 
		agentClientGetSignalsInTheSameGroupReply *tempG, 
		int sigGID, int moreMsgFollowing);

int getNearestNodePositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(int myTCPsockfd, int mynid, 
	double curx, double cury, double curz, double direction, double range, double tDistance, int gid, 
	double &x, double &y, double &z, double &tDirection, int &id, int moreMsgFollowing);

int getNodeMaxSpeed(int myTCPsockfd, int mynid, double &maxSpeed, int moreMsgFollowing);

/* The following are add by student welljeng on 2006, 07, 23 */
int getCurrentPositionOfAGroupOfNodeInADistance(int myTCPsockfd, int mynid, int gid, 
		struct nodePosition **groupNodePositionArray, int &numNodeInThisArray, 
		double CurrentPOS_x, double CurrentPOS_y, double distance, int moreMsgFollowing);

int getTheNearestTrafficLightInfrontOfMe(int myTCPsockfd, int mynid, double my_x, double my_y, 
		double myDirection, double distance, int sigGID, 
		int &light, double &TrafficLightPOS_x, double &TrafficLightPOS_y, int &signalIndex,
		int moreMsgFollowing);

/* Added By Icchen 08.12.16 */
int SetGroupID(int myTCPsockfd, int mynid, int Group_ID, int moreMsgFollowing);

int getLaneChangeInfo(int myTCPsockfd, int mynid, int &EnableLaneChange, int moreMsgFollowing);

int getTeamInfo(int myTCPsockfd, int mynid, int &TeamID, int &TeamLeader, int moreMsgFollowing);

int getTeamTurningInfo(int myTCPsockfd, int mynid, double &TurningDirection, 
		int &TurningBlockNumber, int &TurningTimes, int moreMsgFollowing);

int getNearestFollowingTeamMember(int myTCPsockfd, int mynid, int &Following_Nid, int moreMsgFollowing);

int getNearestPrecedingTeamMember(int myTCPsockfd, int mynid, int &Preceding_Nid, int moreMsgFollowing);

int setTeamTurningInfo(int myTCPsockfd, int mynid, double TurningDirection, 
		int TurningBlockNumber, int TurningTimes, int moreMsgFollowing);

int SwitchOrderWithPrecedingTeamMember(int myTCPsockfd, int mynid, int moreMsgFollowing);

int getNearestTeamMemberPositionAlongTheSpecifiedDirectionAndRangeViewedOnGUIScreen(int myTCPsockfd, int mynid,
        double curx, double cury, double curz, double direction, double range, double tDistance, int tid, 
	double &x, double &y, double &z, double &tDirection, int &id, int moreMsgFollowing);

#endif
