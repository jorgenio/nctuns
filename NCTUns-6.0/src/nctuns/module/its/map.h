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

#ifndef __NCTUNS_MAP_H__
#define __NCTUNS_MAP_H__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/param.h>
#include <assert.h>
#include <math.h>
#include <list>
#include <stack>
#include "obstacle.h"
#include "AStar.h"

#define ASE_BOARDX                                      1300
#define ASE_BOARDY                                      1300

#define BORDER_COST  					1000
#define OBSTACLE_COST 					100000
#define CORNER_COST 					200000

typedef std::pair<double,double> LOCATION;
typedef std::list<LOCATION> PATH_LIST;
typedef PATH_LIST::iterator  PathListItor;
typedef std::stack<_asNode *> PATH_STACK;

class Map {
        int             m_iBoard[ASE_BOARDX][ASE_BOARDY];
        int             m_iPath[ASE_BOARDX][ASE_BOARDY];
        CPathFinder     m_Astar;
        bool            m_bAllowDiagonal;
	double  	gridTileWidth;
public:
        Map();
        ~Map();

	double  GetGridTileWidth() const {return gridTileWidth;}
	void markObstacleArea(double leftx, double rightx, double bottomy, double 
	  topy, struct obstacle *obs);
	bool InObstacleGrid(double x ,double y, double z = 0);
	bool InBorderGrid(double x ,double y, double z = 0);
        void LoadMap();
        void PrintMap();
	void Find(int startx ,int starty, int endx, int endy);
	bool FindPath(double startx, double starty, double endx, double endy);
	bool FindPathToList(double startx, double starty, double endx, double 
	  endy, PATH_LIST *pList);
        static int Valid(_asNode *, _asNode*, void *);
        static int Cost(_asNode*, _asNode*, void *);
	static int Heuristic(int,int,int,int);

};

extern int grid_n(double location);
extern double mid_of_grid(int grid_num);
extern Map *m_Map;

inline int grid_n(double location) {             
       // translate from a node's x (or y) coordinate to its grid's number on
       // the X (or Y) axis
       double gridTileWidth = m_Map->GetGridTileWidth();	

       int num = (int) (location/gridTileWidth);
       if (num == 0)
		return 0;
       if ((fmod(location, gridTileWidth)) != 0)
                num++;
       return (num-1);
}

inline double mid_of_grid(int grid_num) {
       // return the middle location of a grid tile element 
       double gridTileWidth = m_Map->GetGridTileWidth();	

       double location  = (grid_num ) * gridTileWidth + (gridTileWidth/2);
       return location;
}

#endif
