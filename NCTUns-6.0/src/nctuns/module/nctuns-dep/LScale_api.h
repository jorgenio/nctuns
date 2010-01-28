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

#ifndef __NCTUNS_LSCALE_API_H_
#define __NCTUNS_LSCALE_API_H_

#include <assert.h>

#include "../misc/obs/obstacle.h"
#include "command_format.h"
#include "command_server.h"

extern cmd_server *cmd_server_;
extern TrafficLightCenter signalList;
extern vector<TrafficGroupInfo> TrafficGroupInfoList;
extern logpack *logpack_;

// function declaration
int BuildTeam();

void setLargeScaleCarInfo(int mynid, int gid1, int gid2, int gid3, int gid4, int gid5, int gid6, int gid7, int gid8) 
{
	struct LSCarInfo *p;
	p = (struct LSCarInfo *) malloc(sizeof(struct LSCarInfo));
	p->type = LargeScaleCar;
	p->nid = mynid;
	p->group1 = gid1;
	p->group2 = gid2;
	p->group3 = gid3;
	p->group4 = gid4;
	p->group5 = gid5;
	p->group6 = gid6;
	p->group7 = gid7;
	p->group8 = gid8;
	assert(cmd_server_);
	cmd_server_->initLSCar((char *)p);
	free(p);
}

/*
 * This API function sends a command to
 * the simulation engine to ask it to stop the whole simulation. 
 * Upon receiving this command, the simulation engine stops the
 * simulation in the same way as it receives a "STOP" command
 * from the GUI. The simulation results are then transferred back
 * to the GUI.
 */
int stopLargeScaleSimulation(int mynid) 
{

	char    *msg;
	if (mynid <= 0) return(-1);
	msg = (char *) malloc(sizeof(char) * MAX_RECV_BYTES);
	((struct agentClientStopSimulation *) msg)->type =AGENT_CLIENT_STOP_SIMULATION;
	((struct agentClientStopSimulation *) msg)->nid = mynid;

	cmd_server_->communicateLargeScaleCar(msg, 0);

	free(msg);
	return(0);

}

/* 
 *
 * Note:
 *    signal group ID is related to Node block ID.
 *    Therefore, we can get current front node block ID by calling getFrontNID (in road.cc)
 *    and then use the node block ID to search the traffic light.
 *
 * Parameters:
 *    my_x, my_y:	My current position
 *    myDirection:	My current moving direction viewed on GUI screen
 *    distance:		My search range in meters
 *    sigGID:		signal group ID which need to search (one signal group controls one intersection)
 *    light:		return signal light which is facing me (the opposite direction of myDirection)
 *    TrafficLightPOS_x, TrafficLightPOS_y: Traffic light position
 *    signalIndex:	return the searched traffic light index 
 *    			(There are 4 traffic light in one intersection (group) by default)
 *    			XXX: signalIndex may be reused in the next time
 */

int getTheNearestPrecedingTrafficLight_LS(double my_x, double my_y, double myDirection, double distance, int sigGID, int &light, double &TrafficLightPOS_x, double &TrafficLightPOS_y, int &signalIndex)
{
	int result = 0; // fail

	if((sigGID < 0) || (signalList.MaxGroupID == -1) || (myDirection < 0))
	{
		return result; // fail
	}

	/* Check if this car passes the traffic light.
	 * If not, no need to search again.
	 * Note:
	 *    signalIndex may be reused in the next time
	 */
	if((sigGID != -1) && (signalIndex != -1) && (light == signalList.trafficLight[sigGID].light[signalIndex]))
	{
		double tempAngle = fmod(atan2(TrafficLightPOS_y - my_y, TrafficLightPOS_x - my_x)/PI*180 + 360, 360);
		tempAngle = fmod(360 - tempAngle, 360);

		double angleDiff = fabs(tempAngle - myDirection);

		if(angleDiff > 180)
			angleDiff = 360 - angleDiff;

		if(angleDiff <= 90)
		{
			/*
			printf("DEBUG: light %d, sigPos (%lf, %lf), sigGID %d, sigIndex %d\n",
					light, TrafficLightPOS_x, TrafficLightPOS_y, sigGID, signalIndex);
			*/
			return 1; // still in front of the previous searched traffic light.
		}
	}

	double miniSigDistance = 999999;

	// search signal group to find out which traffic light is in front of me
	if(sigGID > signalList.MaxGroupID)
	{
		printf("%s Warning: no group ID %d\n", __func__, sigGID);
		return result; // fail
	}

	int numOfSigs = signalList.trafficLight[sigGID].numOfSignals;

	// search each traffic light in signal group
	for(int j = 0; j < numOfSigs; ++j)
	{
		double sigX, sigY, sigDirection, angle;
		double sigDistance;

		sigX = signalList.trafficLight[sigGID].x[j];
		sigY = signalList.trafficLight[sigGID].y[j];
		sigDirection = signalList.trafficLight[sigGID].FacingDirection[j];

		sigDistance = Distance_BetweenTwoNode(my_x, my_y, sigX, sigY);

		/* check if this traffic light is inside my search range */
		if(sigDistance > distance)
		{
			continue;
		}

		angle = fabs(sigDirection - myDirection);
		if(angle >= 180)
			angle = 360 - angle;

		/* check if this traffic light is facing to me */
		if(fabs(180 - angle) > 15)
		{
			continue;
		}

		// mathematical angle
		angle = atan2(sigY - my_y, sigX - my_x)/PI*180;

		// translate to GUI viewed direction 
		angle = fmod(360 - fmod(angle + 360, 360), 360);

		double difference = fabs(angle - myDirection);

		if(difference > 180)
			difference = 360 - difference;

		if((difference < 90) && (sigDistance < miniSigDistance))
		{
			/* This traffic light is in front of me 
			 * and also the closest to me.
			 */
			light             = signalList.trafficLight[sigGID].light[j];
			TrafficLightPOS_x = signalList.trafficLight[sigGID].x[j];
			TrafficLightPOS_y = signalList.trafficLight[sigGID].y[j];
			signalIndex       = j;

			miniSigDistance = sigDistance;

			result = 1; // success
		}
	}

	return result;
}

/*
 * This API is used to get an unused signal group ID.
 */
int getSignalGroupID_LS(int &sigGID)
{
	int result = 0;
	int numOfSignals = 0;

	result = getAnUnusedSignalGID(TrafficGroupInfoList, sigGID, numOfSignals);
	if(result > 0)
		return 1; // Success
	else
		return 0; // Fail
}

/*
 * This API is used to get the signal lights in the specified signal group ID.
 *
 * Note:
 *    The signal group ID represents the traffic lights in one intersection
 */
int getSignalsInTheSameGroup_LS(agentClientGetSignalsInTheSameGroupReply *tempG, int sigGID) 
{
	int result = 0; // fail

	result = getSignalsInTheSameGroup(signalList, sigGID, tempG);

	return result;
}

/*
 * This API is used to set the signal light
 */
int setSignalLight_LS(int mynid, int sigGID, int signalIndex, int light)
{
	int n = 0; // fail

	n = setSignalLight(signalList, sigGID, signalIndex, light);
	if(n == 1)
	{
		// log the traffic light for playback in GUI
		logpack_->logLight(mynid, sigGID, light, 
				signalList.trafficLight[sigGID].x[signalIndex], 
				signalList.trafficLight[sigGID].y[signalIndex], 
				GetCurrentTime());
	}
	else
	{
		printf("%s failed\n", __func__);
	}
	return n;
}

#endif
