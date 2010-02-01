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

#include <iostream>
using namespace std;

#include "SessionList.H"
#include "SessionNode.H"
			 
int 
SessionList::addSession(unsigned long id, SessionNode node){
	
	sessionMap.insert(map<unsigned long, SessionNode>::value_type(id, node));
	return 0;	
}

int 
SessionList::removeSession(unsigned long id){
	sessionMap.erase(id);	
}

bool 
SessionList::getSessionStatus(unsigned long id){
	SessionNode node = sessionMap[id];
	return node.status;
}

void 
SessionList::setSessionStatus(unsigned long id, bool status){
	SessionNode old_node = sessionMap[id];
	SessionNode new_node(old_node.s_id, old_node.file_dir);
	new_node.status = status;
	removeSession(id);
	addSession(new_node.s_id, new_node);
}

SessionNode 
SessionList::getSessionNode(unsigned long id){
	return sessionMap[id];
}
