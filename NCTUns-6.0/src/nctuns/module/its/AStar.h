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

#ifndef __NCTUNS_ASTAR__
#define __NCTUNS_ASTAR__

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>

#include <sys/socket.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <assert.h>
#include <math.h>
#include "obstacle.h"

class _asNode {
public:
        int             f, g, h;                  // Fitness, goal, heuristic.
        int             x, y;                    // Coordinate position
        int             number;                 // y * maxXGrid + x
        int             numchildren;
        _asNode         *parent;
        _asNode         *next;                  // For open and closed lists
        _asNode         *tnext;                 // For traveled closed lists
        _asNode 	*children[8]; 		// Assuming square tiles
        void           	*dataptr;               // Associated data

        _asNode(int a = -1,int b = -1) : x(a), y(b), number(0), numchildren(0) {
                parent = next = NULL; dataptr = NULL;
                memset(children, 0, sizeof(children));
        }
};

// Stack used to store path points.
struct _asStack {
        _asNode  *data;
        _asStack *next;
};


typedef int (*_asFunc)(_asNode *, _asNode *, void *);
typedef int (*_hFunc)(int, int, int, int); 

class CPathFinder {
public:

        CPathFinder();
        ~CPathFinder();

        _asFunc udCost;      // the user-defined cost function
        _asFunc udValid;     // the user-defined validity function
        _hFunc  udHeuristic;

	void *m_pCBData;     // the data passed back to the callback functions

        bool GeneratePath(int, int, int, int);
        void SetData(void *sd) { m_pCBData = sd; }
        void SetRows(int r) { m_iRows = r;  }

        _asNode *GetBestNode() { return m_pBest; }

protected:
        int      m_iRows;    // maxXGrid: used to calculate node->number
        int      m_iSX, m_iSY, m_iDX, m_iDY, m_iDNum;

        _asNode *m_pOpen;    // The open list
        _asNode *m_pClosed;  // The closed list
        _asNode *m_pBest;    // The best node
        _asStack*m_pStack;   // The stack used to store path points
	_asNode *m_pTravelClosed;

        // Functions
        void AddToOpen(_asNode *);
        void ClearNodes();
        void CreateChildren(_asNode *);
        void LinkChild(_asNode *, _asNode*);
        void UpdateParents(_asNode *);

        // Stack Functions
        void Push(_asNode *);
        _asNode *Pop();

        _asNode *CheckList(_asNode *, int);
        _asNode *GetBest();

        // Inline functions
        inline int Coord2Num(int x, int y) { return y * m_iRows + x; }
        inline int udFunc(_asFunc, _asNode *, _asNode *, void *);
	inline int udFunc(_hFunc, int, int, int, int);
};

#endif 

