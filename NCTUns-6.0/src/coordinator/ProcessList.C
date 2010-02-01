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
#include <vector>
#include <algorithm>
#include <signal.h>
#include <sys/wait.h>
using namespace std;

#include "ProcessList.H"


void ProcessList::add(string id, int pid){
	procList.push_back(pid);
	procMap[id] = pid;
}

	
	
void ProcessList::remove(string id){
	int pid = procMap[id];
	/* No this process */
	if (pid == 0)
		return;
	procMap.erase(id);
	vector<int>::iterator iter = find(procList.begin(), procList.end(), pid);
	if (iter != procList.end()){
		procList.erase(iter);
	}
	if (kill (pid, SIGTERM) >=0)
		waitpid(pid, NULL, 0);
}


void ProcessList::pauseAll(){
	
	for (int i=0; i<procList.size(); i++){
		kill (procList[i], SIGSTOP);
	}
	
}

void ProcessList::stopAll(){
	
	for (int i=0; i<procList.size(); i++){
		int pid = procList[i];
		if (kill (pid, SIGTERM) >=0)
		 	waitpid(pid, NULL, 0);
	}
	
}

void ProcessList::resumeAll(){
	
	for (int i=0; i<procList.size(); i++){
		kill (procList[i], SIGCONT);
	}

}

void ProcessList::abort(){
	
	stopAll();	
}
	
void ProcessList::clearAll(){
	
}

string ProcessList::toString(){
	map<string, int>::iterator iter = procMap.begin();
	while (iter != procMap.end()){
		cout << (*iter).first << "|" << (*iter).second << endl;
		++iter;
	}
}


