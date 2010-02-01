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

#ifndef __NCTUNS_auto_vehicle_signal_h__
#define __NCTUNS_auto_vehicle_signal_h__

#include <stdlib.h>
#include <vector>

//Define the 3 kinds of traffic lights.
#define 	GREEN 			300
#define 	YELLOW			400
#define 	RED			500

//Define all the kinds of traffic signals 
#define 	TRAFFIC_LIGHT 		100
#define 	SPEED_LIMITATION	101

/* Define the maximum number of traffic signal controllers */
#define		MAX_NUM_SIG 		4096

using namespace std;

class TrafficLight {
	public:
	int		type;
	int		sigGID;			// group ID
	int		light[4];
	int		numOfSignals;		// number of signal lights in this traffic light controller
	double		x[4];
	double 		y[4];
	double		FacingDirection[4];
};

class TrafficLightCenter {
public:
	TrafficLight	trafficLight[MAX_NUM_SIG];
	int 		MaxGroupID;
};

class TrafficGroupInfo {
	public:
	int sigGID;
	int numOfSigs;
};

int read_signals(FILE *, TrafficLightCenter &, vector<TrafficGroupInfo> &);

int setSignalLight(TrafficLightCenter &, int, int, int);

int getSignalsInTheSameGroup(TrafficLightCenter, int, agentClientGetSignalsInTheSameGroupReply *);

int getAnUnusedSignalGID(vector<TrafficGroupInfo> &, int &, int &);

int getTheNearestPrecedingTrafficLight(TrafficLightCenter, double, double, double, double, int, int &, double &, double &, int &);

#endif
