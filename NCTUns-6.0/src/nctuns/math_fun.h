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

#ifndef __NCTUNS_MATH_FUN_H__
#define __NCTUNS_MATH_FUN_H__
                                                                                                                             
#include <math.h>
#define PI                      3.14159267
                                                                                                                             
/*
 * Define the number of solutions of simultaneous equitions
 */                                                                                                                              
#define NONE_SOLUTION                                         0x00
#define UNIQUE_SOLUTION                                       0x01
#define INFINITE_SOLUTION                                     0x02


/* 
 * Check whether the two points (x1, y1) and (x2, y2) are on different
 * sides of the aX + bY + c = 0 line.
 */
static inline int DiffSides(double x1, double y1, double x2, double y2, 
double a, double b, double c) {
double product;

        product = (a * x1 + b * y1 + c ) * (a * x2 + b * y2 + c);
        if (product > 0 ) return(0);
        else return(1);
}


/* 
 * Check whether the point (x, y) is in the rectangle whose diagonal
 * corner points are (x1, y1) and (x2, y2).
 */
static inline int NodeInLineBlock(double x , double y, double x1, double y1, 
double x2, double y2) {
	if  ((x1 <= x && x<= x2 ) || (x2 <= x && x <= x1)) {
		if ((y1 <= y && y <= y2) || (y2 <= y && y <= y1)) {
			return(1);	
		}	
	}
	return(0);
}

/* 
 * Given the provided angle, return the corresponding direction vector.
 * Note that the moving angle is the direction of node movement
 * viewed on the GUI screen coordinate system rather than in the normal
 * coordinate system. Here we consider only eight different angles,
 * which are east, north-east, north, north-west, west, south-west,
 * south, and south-east.
 */
static inline void Get_DirVec(double & vec_x, double & vec_y, double angle) {
	if (angle == 0) {
                vec_x = 1;
                vec_y = 0;
        } else if (angle == 90) {
                vec_x = 0;
                vec_y = -1;
        } else if (angle == 180) {
                vec_x = -1;
                vec_y = 0;
        } else if (angle == 270) {
                vec_x = 0;
                vec_y = 1;
        } else if (angle > 0 && angle < 90) { /* 4th */
                vec_x = 1;
                vec_y = -1;
        } else if (angle > 90 && angle < 180) { /* 3th */
                vec_x = -1;
                vec_y = -1;
        } else if (angle > 180 && angle < 270) { /* 2th */
                vec_x = -1;
                vec_y = 1;
        } else if (angle > 270 && angle < 360) { /* 1th */
                vec_x = 1;
                vec_y = 1;
        }
}


/* 
 * Check whether the direction from (x0, y0) to  (x1, y1) is the same as the
 * direction specified by angle. Here we consider only eight different angles,
 * which are east, north-east, north, north-west, west, south-west, south, 
 * and south-east.
 */
static inline int Same_Direction(double x0, double y0, double x1, double y1,
double angle) {
        double vec_x, vec_y;

        Get_DirVec(vec_x, vec_y, angle);
        if (((vec_x * (x1 - x0)) >= 0) && ((vec_y *(y1 - y0)) >= 0 ))
                return 1;
        else
                return 0;
}


/* 
 * Compute the aX + bY + C = 0 line equation formed by (x0, y0) and
 * angle. Note that the moving angle provided here is the direction
 * VIEWED on the GUI screen coordinate system.
 */
static inline int PolarEquation(double &a, double &b, double &c, double x0, 
double y0, double angle) {
        double vec_x, vec_y;

        Get_DirVec(vec_x, vec_y, angle);

        if ((angle == 90) || (angle == 270)) {
                a = 1;
                b = 0;
                c = -1 * x0; 
        } 
        else if ((angle == 0) || (angle  == 180)) {
                a = 0;
                b = 1;
                c = -1 * y0;
        } else {
                b = -1;
                a = fabs( (tan(angle / 180 * PI))) * vec_x * vec_y;
                c = y0 - x0 * a;
        }
        return (1);
}


/* 
 * Compute the aX + bY + C = 0 line equation formed by (x0, y0) and
 * (x1, y1). Note that this line equation is the same in both the normal 
 * coordinate system and the GUI screeen coordinate system. 
 */
static inline int TwoPointEquation(double &a, double &b, double &c, double x0, 
double y0, double x1, double y1) {

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


/* 
 * Check whether the two lines a1X + b1Y + c1 = 0 and a2X + b2Y + C2  = 0
 * intercept. If yes, return the intercept point (&x, &y). In addition, 
 * return whether the number of solution is one (unique), infinite, or none.
 */
static inline char secondorder_SimultaneousEqution(double a1, double b1, double c1,
double a2, double b2, double c2, double &x, double &y) {

double delta, delta_x , delta_y;

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


/*
 * Line 1 : a x + b y + c = 0 , node( x0 , y0)
 * The projection of node(x0,y0) on line L1 is (x1,y1)	
 */
static inline void NodeProjectionOnLine(double a, double b, double c, double x0, 
double y0, double &x1, double &y1) {

	y1 = (a*a*y0 - a*b*x0 - b*c) / (a*a+b*b);
	x1 = (b*b*x0 - a*b*y0 - a*c) / (a*a+b*b);	
}


/*
 * Line Segemnt L1 : (x1,y1),(x2,y2)
 * This function checks if the projection (prox, proy) of node (x0, y0) 
 * on Line Segment L1 is actually on the Line L1
 */
static inline int NodeProjection_InLineSegment(double x0, double y0, double x1, 
double y1, double x2, double y2, double &prox, double &proy) {

	double a, b, c;

	TwoPointEquation(a, b, c, x1, y1, x2, y2);
	NodeProjectionOnLine(a, b, c, x0, y0, prox, proy);
	if (NodeInLineBlock(prox, proy, x1, y1, x2, y2)) {
		return(1);
	}
	return(0);
}

/*
 * Line 1 : a1 * x + b1 * y + c1 = 0, Line2 : a2 * x + b2 *y + c2 = 0
 * (x0,y0) is one end point of Line1 and (x1, y1) and (x2, y2) are the 
 * end points of Line2
 */

static inline int solution_InLineSegment(double a1, double b1, double c1, double a2, 
double b2, double c2, double &x, double &y, double x0, double y0, 
double angle, double x1, double y1, double x2, double y2) {

	if (secondorder_SimultaneousEqution(a1, b1, c1, a2, b2, c2, x, y) != 
	  NONE_SOLUTION){
		if (Same_Direction(x0, y0, x, y, angle)) {
			if (NodeInLineBlock(x, y, x1, y1, x2, y2))
				return(1);
		}		
	}
	return (0);
}


static inline int solution_InLineSegment(double a1, double b1, double c1, double a2, 
double b2, double c2, double &x, double &y, double x0, double y0, double x1,
double y1, double x2, double y2, double  x3, double y3) {

	if (secondorder_SimultaneousEqution(a1, b1, c1, a2, b2, c2, x, y) != 
	  NONE_SOLUTION){
		if (NodeInLineBlock(x, y, x0, y0, x1, y1)) {
			if (NodeInLineBlock(x, y, x2, y2, x3, y3))
				return(1);
		}		
	}
	return (0);
}

static inline double Distance_BetweenTwoNode(double x1, double y1, double x2, double y2, 
double inaccuracy = 0.1) {

	if (fabs(x1 - x2) < inaccuracy)
		x1 = x2;
	if (fabs(y1 - y2) < inaccuracy)
		y1 = y2;
	float distance =  sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1-y2)) ;
	
	return(distance);
}


static inline double Distance_NodeToLine(double a, double b, double c, double x, 
double y) {

        double distance;
        distance = fabs(( a*x + b*y + c)) / sqrt(a*a + b*b);
        return distance;
}


static inline void NearParrallelMoveEqu(double x, double y, double a, double b, 
double c, double dis, double &c1) {

	if ((a * x + b *y + c) > 0)
		c1 = c - sqrt(a*a + b*b) * dis ;
	else
		c1 = sqrt(a*a + b*b) * dis + c;

}


static inline void MidParrallelMoveEqu(double x, double y, double a, double b, 
double c, double dis, double &c1) {

	if ((a * x + b *y + c) > 0) {
		c1 = c - sqrt(a*a + b*b) * dis;
		c1 = (c1 + c)/2;
	}
	else {
		c1 = sqrt(a*a + b*b) * dis + c;
		c1 = (c1 + c)/2;
	}
}


static inline void VerticalBorderLine(double a, double b, double c, double x1, 
double y1, double x2, double y2, double &a1, double &b1, double &c1, 
double &c2) {
	
	a1 = b;
	b1 = -a;

	c1 = -1 * a1 * x1 - b1 * y1;
	c2 = -1 * a1 * x2 - b1 * y2;
}


/*
 * This function uses the mathmatical inner product technique to caculate angle 
 */

static inline float InnerProduct(double x1, double y1, double x2, double y2) {

	double a = (x1*x2 + y1*y2)/(sqrt(x1*x1+y1*y1)*sqrt(x2*x2+y2*y2));

	return(a);
}

/*
 *  Given two nodes (x1,y1) and (x2,y2), calculate the absloute angle   
 *  from the first node to the second node.
 */
static inline double CalcuAngle(double x1, double y1, double x2, double y2) {

	// Be careful! The GUI coordinate system differs from the normal one. 
        // On the GUI coordinate system, if the angle is A, then the
        // corresponding angle on the normal system is (360 - A).
        // The angle returned here is the moving angle viewed in the GUI
        // coordinate system rather than in the normal coordinate system.

	double dx, dy;
        double radians;
        double angle;
                                                                                
	dx = x2 - x1;
	dy = y2 - y1;

	if (dx >= 0 && dy <= 0) {		// angle should be in I
        	radians = atan2(-1*dy, dx);
        	angle = radians * 180 / PI ;
		angle = angle * 1000;
		angle = roundl(angle);
		angle = angle / 1000;
		angle = fmod(angle,360);
	} else if (dx <= 0 && dy <= 0) {	// angle should be in II
        	radians = atan2(-1*dy, -1*dx);
        	angle = radians * 180 / PI ;
		angle = angle * 1000;
		angle = roundl(angle);
		angle = angle / 1000;
		angle = fmod(angle,360);
		angle = 180 - angle;
	} else if (dx <= 0 && dy >= 0) {	// angle should be in III
        	radians = atan2(dy, -1*dx);
        	angle = radians * 180 / PI ;
		angle = angle * 1000;
		angle = roundl(angle);
		angle = angle / 1000;
		angle = fmod(angle,360);
		angle = 180 + angle;
	} else if (dx >= 0 && dy >= 0) {	// angle should be in IV
        	radians = atan2(dy, dx);
        	angle = radians * 180 / PI ;
		angle = angle * 1000;
		angle = roundl(angle);
		angle = angle / 1000;
		angle = fmod(angle,360);
		angle = 360 -  angle;
	}
        return angle;
}

/*
 * This function checks if point (x0, y0) is located in the rectangle area 
 * (x1, y1), (x2, y2), (x3, y3), (x4, y4) or not. 
 */
static inline int CheckNodeInArea(double x0, double y0, double x1, double y1, 
double x2, double y2, double x3, double y3, double x4, double y4) {

	double angle;
	double sum = 0;

	angle = acos(InnerProduct(y2-y0,x2-x0,y1-y0,x1-x0));
	sum += angle;
	angle = acos(InnerProduct(y3-y0,x3-x0,y2-y0,x2-x0));
	sum += angle;
	angle = acos(InnerProduct(y4-y0,x4-x0,y3-y0,x3-x0));
	sum += angle;
	angle = acos(InnerProduct(y1-y0,x1-x0,y4-y0,x4-x0));
	sum += angle;

	if (fabs(sum - 2*PI) <= 0.00001)
		return(1);
	else
		return(0);	
}

/*
 * The following functions are writted by automatic vehicle group. at 2007/03/01
 */
static inline double CheckNodeInAreaAngle(double x0, double y0, double x1, double y1, 
double x2, double y2, double x3, double y3, double x4, double y4) {

	double angle;
	double sum = 0;

	angle = acos(InnerProduct(y2-y0,x2-x0,y1-y0,x1-x0));
	sum += angle;
	angle = acos(InnerProduct(y3-y0,x3-x0,y2-y0,x2-x0));
	sum += angle;
	angle = acos(InnerProduct(y4-y0,x4-x0,y3-y0,x3-x0));
	sum += angle;
	angle = acos(InnerProduct(y1-y0,x1-x0,y4-y0,x4-x0));
	sum += angle;
	sum=fabs((sum - 2*PI));
	return sum;
	/*
	if (fabs(sum - 2*PI) <= 0.00001)
		return(1);
	else
		return(0);	
		*/
}

static inline int SolutionOfQadraticEquationX(double a, double b, double c, double &x1, double &x2)
{
	double delta=b*b-4*a*c;
	if(delta<0)
		return 0;
	else if(delta==0)
	{
		x1=-1*b/2/a;
		x2=x1;
		//y1=a*x1*x1+b*x1+c;
		//y2=y1;
		return 1;
	}
	else
	{
		x1=(-1*b+sqrt(delta))/2/a;
		//y1=a*x1*x1+b*x1+c;
		x2=(-1*b-sqrt(delta))/2/a;
		//y2=a*x2*x2+b*x2+c;
		return 2;
		
	}
}
/*This function solves the equation x=ay^2 + bY +c
   */

static inline int SolutionOfQadraticEquationY(double a, double b, double c, double &y1, double &y2)
{
	double delta=b*b-4*a*c;
	if(delta<0)
		return 0;
	else if(delta==0)
	{
		y1=-1*b/2/a;
		y2=y1;
		//x1=a*y1*y1+b*y1+c;
		//x2=x1;
		return 1;
	}
	else
	{
		y1=(-1*b+sqrt(delta))/2/a;
		//x1=a*y1*y1+b*y1+c;
		y2=(-1*b-sqrt(delta))/2/a;
		//x2=a*y2*y2+b*y2+c;
		//printf("second root x2, y2: %f, %f\n",x2, y2);
		return 2;
		
	}
}

static inline double distance_BetweenTwoParallelLines(double a, double b, double c1, double c2)
{
	return fabs(c1-c2)/(sqrt(a*a+b*b));
}

#endif
