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

#include "SessionNode.H"
#include "SessionList.H"

extern	string session_file;

SessionList::SessionList(){
    load_s_file();
}

int SessionList::addSession(SessionNode node){

	entry.push_back(node.sessionName);	
	sessionMap[node.sessionName] = node;
	update_s_file();
	return 1;
}

int SessionList::removeSession(string sessionName){
	vector<string>::iterator iter = find(entry.begin(), entry.end(), sessionName);
	if (iter != entry.end()){
		entry.erase(iter);
	}	
	sessionMap.erase(sessionName);
	update_s_file();
	return 1;
}


SessionNode* SessionList::getSession(unsigned long sessionID){
	
	for (int i=0; i<entry.size(); i++){
		string key = entry[i];
		SessionNode node = sessionMap[key];
		if (node.sessionID == sessionID)
			return &(sessionMap[key]);	
	}
	return NULL;

}

SessionNode* SessionList::getSession(string sessionName){
	return &(sessionMap[sessionName]);
	
}

string SessionList::toString(){
	string str = "";
	for (int i=0; i<entry.size(); i++){
		string key = entry[i];
		SessionNode node = sessionMap[key];
		cout << node.sessionID << endl << flush;
		if (node.coordinatorIP.length() >5)
			str = str + node.toString() + "\n";
	}
	return str;	
}

void
SessionList::load_s_file(){
	
    FILE *in = fopen(session_file.c_str(), "r");
    if (in != NULL){
	char line[512];
	while(fgets(line, 512, in) != NULL){
	    unsigned long s_id;
	    sscanf(strtok(line, "|\n"), "%lu", &s_id);
	    string s_name  = string(strtok(NULL, "|\n"));
	    string coor_ip = string(strtok(NULL, "|\n"));
	    string cli_ip  = string(strtok(NULL, "|\n"));
	    int    cli_port= atoi(strtok(NULL, "|\n"));
	    bzero(line, 512);
	}
    }

}

void
SessionList::update_s_file(){
//	cout << session_file << endl << flush;
	FILE* out = fopen(session_file.c_str(), "w");
	const char* s_file = toString().c_str();
//	cout << "S_file: " << s_file << flush; 
	fwrite(s_file, 1, strlen(s_file), out);
	fclose(out);
}
