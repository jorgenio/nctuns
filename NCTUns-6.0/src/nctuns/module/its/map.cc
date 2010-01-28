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

#include "math_fun.h"
#include "AStar.h"
#include "map.h"

#include "tactic_api.h"

PATH_STACK pathstack_;

/* Extern the following obstacle parameters from tactic_api.cc */
extern struct obstacle         *obs_head;
extern struct obstacle         *obs_tail;
extern u_int32_t               Num_obs;

// Initialize the map board 
Map::Map() {

	if ((maxXGrid > ASE_BOARDX) || (maxYGrid > ASE_BOARDY)) {
		printf("Error: maxXGrid %d > ASE_BOARDX %d or maxYGrid %d > ASE_BOARDY %d\n", 
			maxXGrid, ASE_BOARDX, maxYGrid, ASE_BOARDY);
		printf("Enlarge ASE_BOARDX and ASE_BOARDY to fix this error\n");
		exit(0);
	}

	gridTileWidth = gridWidthMeter;

	// Determine whether diagnoal movement on a path is allowed or not.
	m_bAllowDiagonal = false;
	
	for (int i = 0; i < maxXGrid; i++) {
		for (int j = 0; j < maxYGrid; j++) {
			m_iBoard[i][j] = 0;
			m_iPath[i][j] = 0;
		}
	}

  	m_Astar.SetRows(maxXGrid);
  	m_Astar.SetData(reinterpret_cast<void*>(this));
	m_Astar.udValid = Valid;  
	m_Astar.udCost = Cost;
	m_Astar.udHeuristic = Heuristic;
	
}

// Load obstacle information and build the grid map
void Map::LoadMap() {
	
	struct obstacle *tmpObs;
	double leftest_X, rightest_X, topest_Y, bottomest_Y;
	// remember that the (0,0) point is on the top-left corner of the 
	// GUI screen

	u_int32_t i;
	for(i = 1, tmpObs = obs_head; i <= Num_obs && tmpObs != NULL;
		i++, tmpObs = tmpObs->next ) {

		// find the four corner points of the obstacle
		if (tmpObs->x1 < tmpObs->x2) {
			leftest_X = tmpObs->x1;
			rightest_X = tmpObs->x2;
		}
		else {  leftest_X = tmpObs->x2;
			rightest_X = tmpObs->x1;
		}
		if (leftest_X > tmpObs->x3)
			leftest_X = tmpObs->x3;
		if (leftest_X > tmpObs->x4)
			leftest_X = tmpObs->x4;
		if (rightest_X < tmpObs->x3)
			rightest_X = tmpObs->x3;
		if (rightest_X < tmpObs->x4)
			rightest_X = tmpObs->x4;

		if (tmpObs->y1 < tmpObs->y2) {
			bottomest_Y = tmpObs->y1;
			topest_Y = tmpObs->y2;
		}
		else {  bottomest_Y = tmpObs->y2;
			topest_Y = tmpObs->y1;
		}
		if (bottomest_Y > tmpObs->y3)
			bottomest_Y = tmpObs->y3;
		if (bottomest_Y > tmpObs->y4)
			bottomest_Y = tmpObs->y4;
		if (topest_Y < tmpObs->y3)
			topest_Y = tmpObs->y3;
		if (topest_Y < tmpObs->y4)
			topest_Y = tmpObs->y4;
		markObstacleArea(leftest_X, rightest_X, bottomest_Y, topest_Y,
		  tmpObs);
	}

        // Now we automatically put obstacles along the two horizonal borders.
        for (int i = 0; i < maxXGrid; i++) {
	  	m_iBoard[i][0] = OBSTACLE_COST;
	  	m_iBoard[i][maxYGrid-1] = OBSTACLE_COST;
        }
        // and then automatically put obstacles along the two vertical borders.
        for (int i = 0; i < maxYGrid; i++) {
	  	m_iBoard[0][i] = OBSTACLE_COST;
	  	m_iBoard[maxXGrid-1][i] = OBSTACLE_COST;
        }
	// Doing so ensure that the constructed grid map is a closed field.


	// Now we fill in the holes left in previous obstacle marking with 
	// obstacles and mark them with OBSTACLE_COST.
	for (int i = 1; i < (maxXGrid - 1); i++) {
		for (int j = 1; j < (maxYGrid - 1) ; j++) {
			if ((m_iBoard[i-1][j-1] == OBSTACLE_COST  || m_iBoard[i-1][j-1] == CORNER_COST) &&
				(m_iBoard[i-1][j] == OBSTACLE_COST || m_iBoard[i-1][j] == CORNER_COST) &&
				(m_iBoard[i-1][j+1] == OBSTACLE_COST || m_iBoard[i-1][j+1] == CORNER_COST) &&
				(m_iBoard[i][j-1] == OBSTACLE_COST || m_iBoard[i][j-1] == CORNER_COST) &&
				(m_iBoard[i][j+1] == OBSTACLE_COST || m_iBoard[i][j+1] == CORNER_COST) &&
				(m_iBoard[i+1][j-1] == OBSTACLE_COST || m_iBoard[i+1][j-1] == CORNER_COST) &&
				(m_iBoard[i+1][j] == OBSTACLE_COST || m_iBoard[i+1][j] == CORNER_COST) &&
				(m_iBoard[i+1][j+1] == OBSTACLE_COST || m_iBoard[i+1][j+1] == CORNER_COST))
				
				m_iBoard[i][j] = OBSTACLE_COST ;
		}
	}

	// Now we identify and mark the borders of obstacles with BORDER_COST. 
	for (int i = 0; i < (maxXGrid - 1); i++) {
		for (int j = 0; j < (maxYGrid - 1); j++) {
			if ( m_iBoard[i][j] == OBSTACLE_COST || m_iBoard[i][j] == CORNER_COST) {
				if (m_iBoard[i+1][j] != OBSTACLE_COST && m_iBoard[i+1][j] != CORNER_COST)
					m_iBoard[i+1][j] = BORDER_COST;
				if (m_iBoard[i+1][j+1] != OBSTACLE_COST && m_iBoard[i+1][j+1] != CORNER_COST)
					m_iBoard[i+1][j+1] = BORDER_COST;
				if (m_iBoard[i][j+1] != OBSTACLE_COST && m_iBoard[i][j+1] != CORNER_COST)
					m_iBoard[i][j+1] = BORDER_COST;
				if (i != 0)
					if (m_iBoard[i-1][j+1] != OBSTACLE_COST && m_iBoard[i-1][j+1] != CORNER_COST)
						m_iBoard[i-1][j+1] = BORDER_COST;
			}
			else if (m_iBoard[i][j] == BORDER_COST)
				continue;
			else {
				if (m_iBoard[i+1][j] == OBSTACLE_COST || m_iBoard[i+1][j] == CORNER_COST)
					m_iBoard[i][j] = BORDER_COST;
				else if (m_iBoard[i+1][j+1] == OBSTACLE_COST || m_iBoard[i+1][j+1] == CORNER_COST)
					m_iBoard[i][j] = BORDER_COST;
				else if (m_iBoard[i][j+1] == OBSTACLE_COST || m_iBoard[i][j+1] == CORNER_COST)
					m_iBoard[i][j] = BORDER_COST;
				else {
					if (i != 0) {
						if (m_iBoard[i-1][j+1] == OBSTACLE_COST || m_iBoard[i-1][j+1] == CORNER_COST)
							m_iBoard[i][j] = BORDER_COST;
					}
				}
			}
		}
	}

}

/*
 * Roughly find the obstacle area ,now we ave to check it in detai;
 * to find if grid is on the obstacle
 */
void Map::markObstacleArea(double x1, double x2, double y1, double y2 , 
struct obstacle* obs) {

	double x, y;
	double prox, proy;

	// first x then y
	for (int j = grid_n(y1); j <= grid_n(y2); j++) {
		for (int i = grid_n(x1); i <= grid_n(x2); i++) {

			if( (i >= maxXGrid)||(j >= maxYGrid)) {
				printf("Map:: out of range (%d, %d)\n", i, j);
				exit(0);
				assert(1);
			} 
			// get the center location of grid[i][j]
			x = mid_of_grid(i);
			y = mid_of_grid(j);
			// if (x,y) is in the obstacle area, then mark it
			if (CheckNodeInArea(x, y, obs->x1, obs->y1, obs->x2, 
				obs->y2, obs->x3, obs->y3, obs->x4 , obs->y4)) {
				m_iBoard[i][j] = OBSTACLE_COST;
			}
			//or if (x, y) is near an obstacle, then mark it
			else if (NodeProjection_InLineSegment(x, y, obs->x1,obs->y1, obs->x2, obs->y2, prox, proy)) {
				if (Distance_BetweenTwoNode(x, y, prox, proy) <= (sqrt(2) * gridTileWidth)) {
					m_iBoard[i][j] = OBSTACLE_COST;
				}
			}
			else if (NodeProjection_InLineSegment(x, y, obs->x2, obs->y2, obs->x3, obs->y3, prox, proy)) {
				if (Distance_BetweenTwoNode(x,y,prox,proy) <= (sqrt(2) * gridTileWidth)) {
					m_iBoard[i][j] = OBSTACLE_COST;
				}
			}
			else if (NodeProjection_InLineSegment(x, y, obs->x3, obs->y3, obs->x4, obs->y4, prox, proy)) {
				if (Distance_BetweenTwoNode(x, y, prox, proy) <= (sqrt(2) * gridTileWidth)) {
					m_iBoard[i][j] = OBSTACLE_COST;
				}
			}
			else if (NodeProjection_InLineSegment(x, y, obs->x4, obs->y4, obs->x1, obs->y1, prox, proy)) {
				if (Distance_BetweenTwoNode(x, y, prox, proy) <= (sqrt(2) * gridTileWidth)) {
					m_iBoard[i][j] = OBSTACLE_COST;
				}
			}
		}
	}

	// find the four corner points of an obstacle and mark them	
	int ix, iy;
	ix = grid_n(obs->x1);
	iy = grid_n(obs->y1);
	m_iBoard[ix][iy] = CORNER_COST;
	ix = grid_n(obs->x2);
	iy = grid_n(obs->y2);
	m_iBoard[ix][iy] = CORNER_COST;
	ix = grid_n(obs->x3);
	iy = grid_n(obs->y3);
	m_iBoard[ix][iy] = CORNER_COST;
	ix = grid_n(obs->x4);
	iy = grid_n(obs->y4);
	m_iBoard[ix][iy] = CORNER_COST;
}

/*
 * This function checks if the location (x, y) is marked with an obstacle.
 */
bool Map::InObstacleGrid(double x, double y, double z) {
	int ix = grid_n(x);
	int iy = grid_n(y);

	if (ix < 0 || iy < 0 || ix >= maxXGrid || iy >= maxYGrid) {
		return true;
	}
	if (m_iBoard[ix][iy] == OBSTACLE_COST || m_iBoard[ix][iy] == 
	  CORNER_COST || m_iBoard[ix][iy] == BORDER_COST)
		return true;

	return false;
}

/*
 * This function checks if (x, y) is located on the borders of an obstacle.
 */
bool Map::InBorderGrid(double x ,double y ,double z){
	int ix = grid_n(x);
	int iy = grid_n(y);

	if (ix < 0 || iy < 0 || ix >= maxXGrid || iy >= maxYGrid) {
		return true;
	}
	if (m_iBoard[ix][iy] == BORDER_COST)
		return true;

	return false;
}

/* 
 * This function serves as a validity function for the A* search algorithm.
 */ 
int Map::Valid(_asNode *pParent, _asNode *pNode, void *pPointer) {
	int x = pNode->x, y = pNode->y;
	Map *me = reinterpret_cast<Map *>(pPointer);

	if (x < 0 || y < 0 || x >= maxXGrid || y >= maxYGrid) return false;
	if (me->m_iBoard[x][y] == CORNER_COST) return false;
	if (me->m_iBoard[x][y] == OBSTACLE_COST) return false;

	if (!(me->m_bAllowDiagonal)) {
		int px = pParent->x;
		int py = pParent->y;

		if (px - x != 0 && py - y != 0) return false;
	}

	return true;
}

/* 
 * This function serves as a cost function for the A* search algorithm.
 */ 
int Map::Cost(_asNode *pParent, _asNode *pNode, void *pPointer) {
        Map *me = reinterpret_cast<Map*>(pPointer);
	int cost;

	if (pParent->parent) {
		if ((pParent->parent->x != pNode->x) && (pParent->parent->y != pNode->y)) {
			cost = me->m_iBoard[pNode->x][pNode->y] + 2;	
			// impose some penalty for making turns. We prefer a straight movement.
			return cost;
		}
	}
	cost = me->m_iBoard[pNode->x][pNode->y] + 1;		
	// The cost for straight movement is the minimum.
	// Ensure that the cost must always > 1

	return cost;
}

/*
 * The estimated cost to get from (x, y) to (goal_x, goal_y)
 */
int Map::Heuristic(int x, int goal_x, int y, int goal_y) {

	return ((x-goal_x)*(x-goal_x)+(y-goal_y)*(y-goal_y));
}


/*
 * This function finds a path from (startx, starty) to (endx, endy)
 * and returns only the turning waypoints of the path. 
 */

bool Map::FindPathToList(double startx, double starty, double endx, 
double endy, PATH_LIST *pList) {

	int sx, sy, ex, ey;
	int px, py, cx, cy, nx, ny;		
	int vec_pcx, vec_pcy, vec_cnx, vec_cny;
	bool turn_point;
	
	sx = grid_n(startx);
	sy = grid_n(starty);
	ex = grid_n(endx);
	ey = grid_n(endy);
	
	// The first node is the starting node (sx, sy).
	LOCATION loc(mid_of_grid(sx), mid_of_grid(sy));
	pList->push_back(loc);
	m_iPath[sx][sy] = 2; // 2: stand for starting, waypoints, or ending 
			     //    point
	nx = sx; ny = sy;
  	
	if (m_Astar.GeneratePath(sx, sy, ex, ey)) {
		
    		_asNode *dest = m_Astar.GetBestNode();	
		while (dest) {
			pathstack_.push(dest);
			dest = dest->parent;
		}
			
    		while (!pathstack_.empty()) {
			dest = pathstack_.top();
			pathstack_.pop();	
			m_iPath[dest->x][dest->y] = 1; // 1: stand for normal movement
			cx = dest->x;
			cy = dest->y;	
			if (!pathstack_.empty()) {
				_asNode *parent = pathstack_.top();
				px = parent->x;
				py = parent->y;	
		
				if (m_bAllowDiagonal){
					vec_pcx = px - cx;
					vec_pcy = py - cy;
					vec_cnx = cx - nx;
					vec_cny = cy - ny;
					turn_point = ((vec_pcx != vec_cnx) || (vec_pcy != vec_cny));
					// The first starting node should not be counted in.
					if (nx == cx && ny == cy)
						turn_point = false;
				}
				else
					turn_point = ((px != nx) && (py != ny));
					
				if (turn_point) {
					LOCATION loc(mid_of_grid(cx), mid_of_grid(cy));
					pList->push_back(loc);
					m_iPath[cx][cy] = 2; // 2: stand for turning waypoints
				}
			}				
			else {
				/* The end node has been reached. */
				assert(ex == dest->x && ey == dest->y);
				LOCATION loc(mid_of_grid(ex), mid_of_grid(ey));
				pList->push_back(loc);
				pList->push_back(loc);
				m_iPath[ex][ey] = 2;
			}
			nx = cx;
			ny = cy;
    		}
		return (true);
  	} else {
    		if (0) printf("Cannot use A* algorithm to find a path...");
		return (false);
  	}
	return (false);
}


/*
 * This function is a debugging function.
 */
void Map::PrintMap() {

	FILE *fptr[10];
	char *filepath;
	int   total;
	int   row= 100;

	filepath = (char *) malloc(32);

	total = maxXGrid / row;
	if ((maxXGrid % row) != 0)
		total++;
	if (total >= 10)
		assert(0);

	for (int i = 0; i < total; i++) {
		sprintf(filepath, "/tmp/gridMap.sta%d", i);
		printf("filepath %s \n", filepath);
		fptr[i] = fopen(filepath, "w+");
	}
		
	free(filepath);

	for (int j = 0; j <= maxYGrid; j++) {
		for (int i = 0; i <= maxXGrid; i++) {
			if (m_iBoard[i][j] == CORNER_COST)
				fprintf(fptr[i/row],"@");
			else if (m_iPath[i][j] ==1 )
				fprintf(fptr[i/row],"#");
			else if (m_iPath[i][j] ==2 )
				fprintf(fptr[i/row],"*");
			else if (m_iBoard[i][j] == OBSTACLE_COST )
				fprintf(fptr[i/row],".");
			else if (m_iBoard[i][j] == BORDER_COST )
				fprintf(fptr[i/row],"+");
			else
				fprintf(fptr[i/row]," ");
			if ((((i) % row) == 0) || (i == maxXGrid)) {
				if( i!=0 )
					fprintf(fptr[(i-1)/row],"##\n");
			}
		}
	}
	for (int i = 0; i < total; i++)
		fflush(fptr[i]);
}

