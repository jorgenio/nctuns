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

#ifndef	__NCTUNS_OBS_H_
#define	__NCTUNS_OBS_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>

struct obstacle {
        double                  x1,y1;
        double                  x2,y2;
	double			x3,y3;
	double			x4,y4;
	char 			type;
	int			blockView;
	int			blockMovement;
	int			blockWirelessSignal;
	double			attenuation;
        struct obstacle         *next;
};    
struct road: public obstacle
{	
	int		SerialNumber;
	int		NumberOfDirections;
	int		NumberOfSearchedDirections;
	double		*direction;
	int		LeftSeparationType;
	int	 	RightSeparationType;
};
//Use this data to enhance the performance
struct StepRecord
{
	struct road*	CurrentRoad;
	struct road*	ExpectNextRoad;
	double 		x;
	double 		y;
	double		SharedVertex[4];
	double 		direction;
	StepRecord(){
		CurrentRoad=NULL;
		ExpectNextRoad=NULL;
	}
	
};
extern struct obstacle         *Obs_head ;
extern struct obstacle         *Obs_tail ;
extern u_int32_t               Num_obstacle;

extern void Insert_obstacles(FILE *obs_fptr);    
extern double Check_obstacles(u_int32_t Src_NID, u_int32_t Dst_NID);
#endif
