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

#ifndef __NCTUNS_ROAD_H_
#define __NCTUNS_ROAD_H_

#include <stdlib.h>
#include <vector>
#include <queue>
#define RIGHT 	0x12
#define LEFT 	0x13

using namespace std;

enum RoadType{
	ROAD_TYPE_EDGE,
	ROAD_TYPE_NODE
};

enum NodeBlockType{
	NODE_TYPE_I, // Intersection
	NODE_TYPE_T, // T-type intersection
	NODE_TYPE_L, // L-type intersection
	NODE_TYPE_R, // Land-Reduction intersection
	NODE_TYPE_P  // Pseudo-type (Ring) intersection
};

enum ConnectionType {

	CT_ROAD_SEGMENT,       	// connection_type: edge 
	CT_INTERSECTION,    	// connection_type: intersection
	CT_LT_INTERSECTION,	// connection_type: L-type intersection 
	CT_RT_INTERSECTION,	// connection_type: Land-Reduction intersection 
	CT_PT_INTERSECTION,	// connection_type: Pseudo-type intersection 
	CT_TT_INTERSECTION	// connection_type: T-type intersection

};

class RoadBlock{
	public:
		double x[4];
		double y[4];
		double a, b, c; // linear equation coefficients (ax+by+c) of the road block
		double end_x[2], end_y[2];
		double direction;
		RoadType type;
		int RID; 	// Road Block ID
		RoadBlock &operator=(const RoadBlock &);
};

class RoadNode{
	public:
		int NID; // node ID
		NodeBlockType type;
		int NumOfEdges;
		int *EID;
		double *direction;
		bool *HeadOrNot;
		RoadBlock RoadBlockID;
};

class RoadLane{
	public:
		int ExitRoadBlockIndex; // It's an index not ID
		vector<RoadBlock> RoadBlockID;
};

class RoadEdge{
	public:
		int EID;
		int NumOfLane;
		int NumOfNode;
		int *NID;
		int L_Number;
		int R_Number;
		double length;
		RoadLane *lane;
};

class CacheRecord{
	public:
		double x, y, NextDirection;
		int CurrentNodeSerial, CurrentEdgeSerial, CurrentLaneSerial, CurrentBlockSerial, CurrentDirty;
		int NextNodeSerial, NextEdgeSerial, NextLaneSerial, NextBlockSerial, NextDirty;
		int NodeAfterTurn, EdgeAfterTurn, LaneAfterTurn, BlockAfterTurn, AfterDirty;
		CacheRecord();
		void RefreshCurrent(int, int, int, int);
		void RefreshNext(int, int, int, int);
		void RefreshAfter(int, int, int, int);

};

class tracks{
	public:
		int RelatedNodeIndex;
		int CurrentEdgeIndex, CurrentLaneIndex, CurrentBlockIndex;
		int EndEdgeIndex, EndLaneIndex, EndBlockIndex;
		int nodeID;
};

void read_RoadStructure(FILE *);

void dumpRoadStructure();

int getNeighboringLaneInformation(double, double, int, /*int &,*/ double &, /*double &, double &, double &, double &, double &,*/ int, CacheRecord *);

inline int getRoadBlockByIndex (int, int, int, int, RoadBlock &);

int getNextRoadIndex(double, double, double, int &, int &, int &, int &, int, CacheRecord *);

int getNextRoadInformation(double, double, double, int &, double *&, double &, double &, double &, int, CacheRecord *);

int getCurrentRoadInformation(double, double, int &, double *&, double &, double &, double &, double &, double &, int &, int, CacheRecord *);

int getCurrentRoadIndex(double, double, int &,int &, int &, int &, int, CacheRecord *);

int takeATurn(double, double, double, double, int &, double *&, double *&, double &, double &, double *&, int, CacheRecord *);

int selfCorrectness(double, double, double &, double &, int, CacheRecord *);

int CheckCurrentCache(double, double, CacheRecord *);

int CheckPOSInBlock(double, double, int, int, int, int);

void cacheCopy(CacheRecord *, CacheRecord *);

int checkIfOnTheSameLane(double, double, double, double, int, CacheRecord *, CacheRecord *);

int getCurrentNodeExit(double, double, double &, double &, double &, double &, int, CacheRecord *);

void getRoadWidth(double &);

int isTheSameDirection(double, double, double, double, double);

int getFrontNID(double, double, int &, int, CacheRecord *);

#endif

