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

#include "Global.H"
#include "ServerList.H"
#include "ServerNode.H"

vector <string> entry;
map <string, ServerNode> serverMap;
typedef vector<string>::iterator s_iter;

void ServerList::addHost(ServerNode node){
	
	s_iter iter = find(entry.begin(), entry.end(), node.ip);
	if (iter == entry.end())
	    entry.push_back(node.ip);	
	
	serverMap[node.ip]= node;
}

void ServerList::removeHost(string hostIP){
	
	vector<string>::iterator iter = find(entry.begin(), entry.end(), hostIP);
	if (iter != entry.end()){
		entry.erase(iter);
	}
	
	serverMap.erase(hostIP);
}

string ServerList::getHostIP(int fd){

	for (int i=0; i<entry.size(); i++){
		if(serverMap[entry[i]].opened_fd == fd)
			return entry[i];
	}
	return NULL;
}

ServerNode* ServerList::getHost(string hostIP){				
	return &(serverMap[hostIP]);
}


ServerNode* ServerList::getHost(int hostIndex){
	// Array Boundary Check Code Goes Here....
	string key = entry[hostIndex];
	return &(serverMap[key]);
}

ServerNode* ServerList::getHost(string IP, int Port){
	for (int i=0; i<entry.size(); i++) {
	string key = entry[i];
	if(serverMap[key].clientIP == IP) 
		/* marked out by oli Feb 23, 2003; may not need port#*/
		//&& (serverMap[key].clientPort == Port)) 
		return &(serverMap[key]);		
	else 
		return NULL;
	}
	

}


int ServerList::anyFreeHost(){
	for (int i=0; i<entry.size(); i++) {
		string key = entry[i];
		if(serverMap[key].status)
			return i;		
	}
	return -1;
}

int ServerList::findFreeHost(char* IP) {
	string host = IP;
	for (int i=0; i<entry.size(); i++) {
		string key = entry[i];
		if(host == key) {
			if(serverMap[key].status)
				return i;
			else
				return -1;
		}
	}
	return -1;
}

ServerNode*
ServerList::
get_aHost(){
	int num = entry.size();
	if (num	>0)
		return getHost(num-1);
	else
		return NULL;
	
}

void ServerList::setStatus(int hostIndex, bool status){
	
	string key = entry[hostIndex];
	serverMap[key].status = status;
	
	return;
}

void ServerList::setStatus(char* hostIP, bool status){
	
	ServerNode* node = &(serverMap[hostIP]);
	node->status = status;

	return;
}

void ServerList::setStatus(const char* hostIP, bool status){
	
	ServerNode* node = &(serverMap[hostIP]);
	node->status = status;
	
	return;
}


void ServerList::setClient(int serverIndex, string IP, int Port){
	
	string key = entry[serverIndex];
	ServerNode* node = &(serverMap[key]);
	node->clientIP = IP;
	node->clientPort = Port;
	return;
}

string ServerList::toString(){
	string temp = "<ServerList>:";
	for (int i=0; i<entry.size(); i++){
		temp.append(entry[i]);
		temp.append(":");
		if ((serverMap[entry[i]]).status)
			temp.append("FREE");
		else
			temp.append("BUSY");
		temp.append("\t");
	}
	temp.append("\n");
	return temp;
}


