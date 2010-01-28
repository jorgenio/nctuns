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

#include "obstacle.h"

extern int GetNodeLoc(u_int32_t, double &, double &, double &);
struct obstacle		*Obs_head = NULL;
struct obstacle		*Obs_tail = NULL;
u_int32_t		Num_obstacle = 0;

void Insert_obstacles(FILE *obs_fptr){
	char			line[128];
	double			x1, y1, x2, y2, width;
	int			blockView, blockMovement, blockWirelessSignal;
	double			attenuation;
	double			dx, dy, len;
	struct obstacle		*tmpObs;

	while( !feof(obs_fptr) ) {
		line[0] = '\0';
		fgets(line, 127, obs_fptr);
		if ((line[0]=='\0')||(line[0]=='#'))
			continue;

		if (9 == sscanf(line, "%lf,%lf,%lf,%lf,%lf,%d,%d,%d,%lf", 
			&x1, &y1, &x2, &y2, &width,
			&blockView, &blockMovement, &blockWirelessSignal,
			&attenuation) )
		{
			/*
			printf("Obstacle: %lf, %lf, %lf, %lf, %lf, %d, %d, %d, %lf\n",
				x1, y1, x2, y2, width,
				blockView, blockMovement, blockWirelessSignal,
				attenuation);
			*/

			tmpObs = (struct obstacle *)malloc(sizeof(struct obstacle));
			tmpObs->blockView = blockView;
			tmpObs->blockMovement = blockMovement;
			tmpObs->blockWirelessSignal = blockWirelessSignal;
			tmpObs->attenuation = attenuation;

			dx = x1 - x2;
			dy = y1 - y2;
			len = sqrt(dx*dx + dy*dy);

			tmpObs->x1 = x1 - (width / 2) * dy / len;
			tmpObs->y1 = y1 + (width / 2) * dx / len;
			tmpObs->x2 = x2 - (width / 2) * dy / len;
			tmpObs->y2 = y2 + (width / 2) * dx / len;
			tmpObs->x3 = x2 + (width / 2) * dy / len;
			tmpObs->y3 = y2 - (width / 2) * dx / len;
			tmpObs->x4 = x1 + (width / 2) * dy / len;
			tmpObs->y4 = y1 - (width / 2) * dx / len;
			tmpObs->next = NULL;

			/*
			printf("%lf %lf %lf %lf %lf %lf %lf %lf\n",
				tmpObs->x1,
				tmpObs->y1,
				tmpObs->x2,
				tmpObs->y2,
				tmpObs->x3,
				tmpObs->y3,
				tmpObs->x4,
				tmpObs->y4);
			*/

			if( Num_obstacle == 0 ) {
				Obs_head = tmpObs;
				Obs_tail = tmpObs;
			}
			else {
				Obs_tail->next = tmpObs;
				Obs_tail = tmpObs;
			}
			Num_obstacle++;
		}
	}

	return;
}


int TwoPointEquation(double &a, double &b, double &c, double x0, double y0, 
double x1, double y1) {

/* Compute the aX + bY + C = 0 line equation formed by (x0, y0) and
   (x1, y1). 
 */
        if (y1 == y0) {
                a = 0;
                b = 1;
                c = -1 * y0;
                if (x1 == x0)
                        a = b = c = 0;
        }
        else if (x1 == x0) {
                a = 1;
                b = 0;
                c = -1 * x0;
        } else {
                b = -1;
                a = (y1 - y0) / (x1 - x0);
                c = y0 - x0 * a;
        }
        return (1);
}

#define NONE_SOLUTION 0
#define UNIQUE_SOLUTION 1
#define INFINITE_SOLUTION 2

char secondorder_SimultaneousEqution(double a1, double b1, double c1,
double a2, double b2, double c2, double &x, double &y) {
/* Check whether the two lines a1X + b1Y + c1 = 0 and a2X + b2Y + C2  = 0
   intercept. If yes, return the intercept point (&x, &y). In addition, 
   return whether the number of solution is one (unique), infinite, or none.
 */
double delta , delta_x , delta_y;

        delta = b2 * a1 - b1 * a2;
        delta_x = c2 * b1 - c1 * b2;
        delta_y = a2 * c1 - a1 * c2;

        if (delta != 0) {
                 x  = delta_x / delta;
                 y  = delta_y / delta;
                return (UNIQUE_SOLUTION);
        }
        else if (delta_x == 0 && delta_y == 0)
                return (INFINITE_SOLUTION);
        else
                return (NONE_SOLUTION);
}

double Check_obstacles(u_int32_t Src_NID, u_int32_t Dst_NID) {
	u_int32_t		i;
	struct obstacle		*tmpObs;
	double			Src_x, Src_y, Src_z;
	double			Dst_x, Dst_y, Dst_z;
	double			attenuation = 0;
	double 			a1, b1, c1, a2, b2, c2, ix, iy;
	int 			numSolution1, numSolution2, numSolution3, numSolution4;
	double 			lx, rx, ty, by;
	double			lx2, rx2, ty2, by2;

	if (Num_obstacle == 0) return(0);

	GetNodeLoc(Src_NID, Src_x, Src_y, Src_z);
	GetNodeLoc(Dst_NID, Dst_x, Dst_y, Dst_z);

	if (Src_x > Dst_x) {
		rx2 = Src_x; lx2 = Dst_x;
	} else {
		rx2 = Dst_x; lx2 = Src_x;
	}
	if (Src_y > Dst_y) {
		by2 = Src_y; ty2 = Dst_y;
	} else {
		by2 = Dst_y; ty2 = Src_y;
	}

	for( i = 1, tmpObs = Obs_head; i <= Num_obstacle && tmpObs != NULL; 
				i++, tmpObs = tmpObs->next ) {

		if (!tmpObs->blockWirelessSignal)
		{
			continue;
		}

		/* check intersection */

		TwoPointEquation(a1, b1, c1, Src_x, Src_y, Dst_x, Dst_y);

		TwoPointEquation(a2, b2, c2, tmpObs->x1, tmpObs->y1,
			tmpObs->x2, tmpObs->y2);
		numSolution1 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
		if (numSolution1 == UNIQUE_SOLUTION) {
			if (tmpObs->x1 > tmpObs->x2) {
				rx = tmpObs->x1; lx = tmpObs->x2;
			} else {
				rx = tmpObs->x2; lx = tmpObs->x1;
			}
			if (tmpObs->y1 > tmpObs->y2) {
				by = tmpObs->y1; ty = tmpObs->y2;
			} else {
				by = tmpObs->y2; ty = tmpObs->y1;
			}
			if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by) &&
			    (lx2 <= ix) && (ix <= rx2) && (ty2 <= iy) && (iy <= by2)) {
				numSolution1 = 1;
			} else numSolution1 = 0;
		}

		TwoPointEquation(a2, b2, c2, tmpObs->x2, tmpObs->y2,
			tmpObs->x3, tmpObs->y3);
		numSolution2 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);

		if (numSolution2 == UNIQUE_SOLUTION) {
                        if (tmpObs->x2 > tmpObs->x3) {
                                rx = tmpObs->x2; lx = tmpObs->x3;
                        } else {
                                rx = tmpObs->x3; lx = tmpObs->x2;
                        }
                        if (tmpObs->y2 > tmpObs->y3) {
                                by = tmpObs->y2; ty = tmpObs->y3;
                        } else {
                                by = tmpObs->y3; ty = tmpObs->y2;
                        }
                        if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by) &&
			    (lx2 <= ix) && (ix <= rx2) && (ty2 <= iy) && (iy <= by2)) {
				numSolution2 = 1;
			} else numSolution2 = 0;
		}

		TwoPointEquation(a2, b2, c2, tmpObs->x3, tmpObs->y3,
			tmpObs->x4, tmpObs->y4);
		numSolution3 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);
                if (numSolution3 == UNIQUE_SOLUTION) {
                        if (tmpObs->x3 > tmpObs->x4) {
                                rx = tmpObs->x3; lx = tmpObs->x4;
                        } else {
                                rx = tmpObs->x4; lx = tmpObs->x3;
                        }
                        if (tmpObs->y3 > tmpObs->y4) {
                                by = tmpObs->y3; ty = tmpObs->y4;
                        } else {
                                by = tmpObs->y4; ty = tmpObs->y3;
                        }
                        if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by) &&
			    (lx2 <= ix) && (ix <= rx2) && (ty2 <= iy) && (iy <= by2)) {
				numSolution3 = 1;
			} else numSolution3 = 0;
		}

		TwoPointEquation(a2, b2, c2, tmpObs->x4, tmpObs->y4,
			tmpObs->x1, tmpObs->y1);
		numSolution4 = secondorder_SimultaneousEqution(a1, b1, c1, 
			a2, b2, c2, ix, iy);

                if (numSolution4 == UNIQUE_SOLUTION) {
                        if (tmpObs->x4 > tmpObs->x1) {
                                 rx = tmpObs->x4; lx = tmpObs->x1;
                        } else {                                 
				rx = tmpObs->x1; lx = tmpObs->x4;
                        }                         
			if (tmpObs->y4 > tmpObs->y1) {
                                by = tmpObs->y4; ty = tmpObs->y1;
                        } else {
                                by = tmpObs->y1; ty = tmpObs->y4;
                        }
                        if ((lx <= ix) && (ix <= rx) && (ty <= iy) && (iy <= by) &&
			    (lx2 <= ix) && (ix <= rx2) && (ty2 <= iy) && (iy <= by2)) {
				numSolution4 = 1;
			}
			else numSolution4 = 0;
		}

		if ((numSolution1 + numSolution2 + numSolution3 + numSolution4)
			> 0) {
			attenuation += tmpObs->attenuation;
		}
	}

	return attenuation;
}
