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

#include "AStar.h"

CPathFinder::CPathFinder() 
{
	m_pOpen = m_pClosed = NULL;
	m_pStack = NULL;
	m_pBest = NULL;
	m_pTravelClosed = NULL;
	
	udCost = NULL;
	udValid = NULL;
}

CPathFinder::~CPathFinder() 
{
	ClearNodes();
}

void CPathFinder::ClearNodes() 
{
	_asNode *temp = NULL;

	if (m_pOpen) {
		while (m_pOpen) {
			temp = m_pOpen->next;
			delete m_pOpen;
			m_pOpen = temp;
		}
	}

	if (m_pClosed) {
		while (m_pClosed) {
			temp = m_pClosed->next;
			delete m_pClosed;
			m_pClosed = temp;
		}
	}
}

bool CPathFinder::GeneratePath(int sx, int sy, int dx, int dy) {


	ClearNodes();
	
  	m_iSX = sx; m_iSY = sy; m_iDX = dx; m_iDY = dy;
  	m_iDNum = Coord2Num(dx, dy);

  	_asNode *temp = new _asNode(sx, sy);
  	_asNode *best;

  	temp->g = 0;
    	temp->h = udFunc(udHeuristic, sx, m_iDX, sy, m_iDY);
  	temp->f = temp->g + temp->h;
  	temp->number = Coord2Num(sx, sy);

  	m_pOpen = temp;
  	while (true) {
    		if (!(best = GetBest())) return false;
    		if (best->number == m_iDNum) break;
    		CreateChildren(best);
  	};
  	m_pBest = best;
  
  	return true;
}

_asNode *CPathFinder::GetBest() 
{
	if (!m_pOpen) return NULL;

	_asNode *temp = m_pOpen, *temp2 = m_pClosed;
	m_pOpen = temp->next;

	m_pClosed = temp;
	m_pClosed->next = temp2;
	return temp;
}

void CPathFinder::CreateChildren(_asNode *node) {
  	int x = node->x, y = node->y;

	_asNode	temp;
  	for (int i = -1; i < 2; i++) {
    		for (int j = -1; j < 2; j++) {
			temp.x = x + i;
			temp.y = y + j;
			if ((i == 0 && j == 0) || 
				(!udFunc(udValid, node, &temp, m_pCBData))) continue;
          		LinkChild(node, &temp);
    		}
  	}
}


void CPathFinder::LinkChild(_asNode *node, _asNode *temp) {
	int x = temp->x;
	int y = temp->y;
  	int g = node->g + udFunc(udCost, node, temp, m_pCBData);
  	int num = Coord2Num(x, y);

  	_asNode *check = NULL;

  	if ((check = CheckList(m_pOpen, num))) {
    		node->children[node->numchildren++] = check;
		
    		// A better route found, so we update the node and variables 
		// accordingly.
    		if (g < check->g) {
      			check->parent = node;
      			check->g = g;
      			check->f = g + check->h;
    		}
  	}
  	else if ((check = CheckList(m_pClosed, num))) {
    		node->children[node->numchildren++] = check;

    		if (g < check->g) {
      			check->parent = node;
      			check->g = g;
      			check->f = g + check->h;

      			UpdateParents(check);
    		}
  	}
  	else {
    		_asNode *newnode = new _asNode(x, y);
    		newnode->parent = node;
		newnode->g = g;
		newnode->h = udFunc(udHeuristic, x, m_iDX, y, m_iDY);
    		newnode->f = newnode->g + newnode->h;
    		newnode->number = Coord2Num(x, y);

    		AddToOpen(newnode);
    		node->children[node->numchildren++] = newnode;
  	}
}


_asNode *CPathFinder::CheckList(_asNode *node, int num) 
{
	while (node) {
		if (node->number == num) return node;
		node = node->next;
	}

	return NULL;
}

void CPathFinder::UpdateParents(_asNode *node) {
  	int g = node->g, c = node->numchildren;

  	_asNode *kid = NULL;
  	for (int i = 0; i < c; i++) {
    		kid = node->children[i];
    		if ((g + udFunc(udCost, node, kid, m_pCBData) ) < kid->g) {
      			kid->g = g + 1;
      			kid->f = kid->g + kid->h;
      			kid->parent = node;
      			Push(kid);
    		}
  	}

  	_asNode *parent;
  	while (m_pStack) {
    		parent = Pop();
    		c = parent->numchildren;
    		for (int i = 0; i < c; i++) {
      			kid = parent->children[i];
			
      			if ((parent->g + udFunc(udCost, parent, kid, m_pCBData)) 
			  < kid->g) {
        			kid->g = parent->g + udFunc(udCost, parent, kid,  
				  m_pCBData);
        			kid->f = kid->g + kid->h;
        			kid->parent = parent;
        			Push(kid);
      			}
    		}
  	}
}


void CPathFinder::AddToOpen(_asNode *addnode) 
{
	_asNode *node = m_pOpen;
	_asNode *prev = NULL;

	if (!m_pOpen) {
		m_pOpen = addnode;
		m_pOpen->next = NULL;
		return;
	}

	while(node) {
		if (addnode->f > node->f) {
			prev = node;
			node = node->next;
		} else {
			if (prev) {
				prev->next = addnode;
				addnode->next = node;
			} else {
				_asNode *temp = m_pOpen;
				m_pOpen = addnode;
				m_pOpen->next = temp;
			}
			return;
		}
	}
	prev->next = addnode;
}

void CPathFinder::Push(_asNode *node) {

	if (!m_pStack) {
		m_pStack = new _asStack;
		m_pStack->data = node;
		m_pStack->next = NULL;
	} else {
		_asStack *temp = new _asStack;

		temp->data = node;
		temp->next = m_pStack;
		m_pStack = temp;
	}
}

_asNode *CPathFinder::Pop() 
{
	_asNode *data = m_pStack->data;
	_asStack *temp = m_pStack;

	m_pStack = temp->next;
	
	delete temp;

	return data;
}

int CPathFinder::udFunc(_hFunc func, int sx, int dx, int sy, int dy)
{
	if (func) return func(sx, dx, sy, dy);

	return 1;
}

int CPathFinder::udFunc(_asFunc func, _asNode *param1, _asNode *param2, void *cb)
{
	if (func) return func(param1, param2, cb);

	return 1;
}

