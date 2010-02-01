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

#include "Mediator.H"
#include "JobNode.H"
#include "JobQueue.H"

/* Add a job to the job queue*/
void 
JobQueue::
addJob(JobNode node){
	jobQueue.push_back(node);
	jobMap[node.jobID] = node;	
}

/* Get a job to dispatch from the job queue*/
int
JobQueue::
getJob(JobNode& node){
	if(jobQueue.empty())
		return -1;
	
	vector<JobNode>::iterator iter = jobQueue.begin();
	node = *iter;
	jobQueue.erase(iter);
	return 1;
}

JobNode*
JobQueue::
getJob(unsigned long id){
	return &(jobMap[id]);
}

int
JobQueue::
removeJob(unsigned long id){
	JobNode node = jobMap[id];
	
	vector<JobNode>::iterator iter = find(jobQueue.begin(), jobQueue.end(), node);
	/*
	if (iter!=jobQueue.end())
		jobQueue.erase(iter);
	*/
	map<unsigned long, JobNode>::iterator it = jobMap.find(id);
	if (it!=jobMap.end())
		jobMap.erase(it);
}

/* Is job queue empty?*/
bool 
JobQueue::
isEmpty(){
	//cout << "empty" << jobQueue.size() << endl;
	return jobQueue.empty();	
}

int
JobQueue::
getProgress(JobNode *jNode, Mediator *mediator){
	unsigned long long curr_tick = mediator->getTick(jNode->coor_ip);
	jNode->progress = (curr_tick / ((jNode->maxTime)*100000) );
	//jNode->progress = (int)progress;
	
}


int
JobQueue::
getJobInfo(vector<char*> ids, string& list, const Mediator *mediator){
	vector<char*>::iterator iter = ids.begin()+1; // get rid of opcode
	while( iter != ids.end() ){
		unsigned long job_id;
		sscanf((*iter), "%lu", &job_id);
		if (hasJob(job_id)){
			JobNode* node = &(jobMap[job_id]);
			if ((node->status) == "RUNNING"){
				getProgress(node, (Mediator *) mediator);
			}
			list = list + node->toString() + "\t";
		}			
		++iter;
	}
	//cout << "list: " << list << endl;
}

void 	
JobQueue::
setStatus(unsigned long id, string _status){
	JobNode* node_ptr = &jobMap[id];
	node_ptr->status = _status;
}

void	
JobQueue::
setStartTime(unsigned long id, string start_time){
	JobNode* node_ptr = &jobMap[id];
	node_ptr->startTime = start_time;		
}

void	
JobQueue::
setEndTime(unsigned long id, string end_time){
	JobNode* node_ptr = &jobMap[id];
	node_ptr->endTime = end_time;	
}


void	
JobQueue::
setProgress(unsigned long id, int _progress){
	JobNode* node_ptr = &jobMap[id];
	node_ptr->progress = _progress;	
}

void	
JobQueue::
set_coor(unsigned long id, string ip, int c_port, int d_port){
	JobNode* node_ptr = &jobMap[id];
	node_ptr->coor_ip = ip;	
	
}

bool	
JobQueue::
hasJob(unsigned long id){
	map<unsigned long, JobNode>::iterator iter = jobMap.find(id);
	if (iter != jobMap.end())
		return true;
	else
		return false;
}

int
JobQueue::
getWaitJobNumber(string username, string cli_ip){
	int count = 0;
	cout << "size=" << jobQueue.size();
	vector<JobNode>::iterator iter = jobQueue.begin();
	while(iter != jobQueue.end()){
		JobNode j_node = *iter;
		cout << "username= " << j_node.username << "|" << username << "|" << endl;
		cout << "ip= " << j_node.client_ip <<"|"<< cli_ip << "|" << endl;
		if (j_node.username == username && j_node.client_ip == cli_ip){
			++count;
		}
		++iter;
	}
	return count;

}


/* Print out the job queue*/
string
JobQueue::
toString(){
	string str;
	map<unsigned long, JobNode>::iterator iter = jobMap.begin();
	while(iter != jobMap.end()){
		
		str = str + (*iter).second.toString() + "\n";
		++iter;
	}
	return str;
}



