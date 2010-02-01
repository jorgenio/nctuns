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

#ifndef __JOBQUEUE
#define __JOBQUEUE

#include <vector>
#include <map>
#include <string>
using namespace std;

#include "JobNode.H"

class Mediator;
	
class JobQueue{

	public:
		void 	addJob(JobNode node);
		int 	getJob(JobNode& node);
		int		removeJob(unsigned long id);
		void 	setStatus(unsigned long id, string status);
		void	setStartTime(unsigned long id, string start_time);
		void	setEndTime(unsigned long id, string end_time);
		void	setProgress(unsigned long id, int progress);
		void	set_coor(unsigned long id, string ip, int c_port, int d_port);
		bool 	isEmpty();
		bool	hasJob(unsigned long id);
		int 	getJobInfo(vector<char*> ids, string& list, const Mediator *mediator);
		int		getWaitJobNumber(string username, string client_ip);
		string 	toString();
		
		JobNode* getJob(unsigned long id);
		
	protected:
		int 	getProgress(JobNode *jNode, Mediator *meditor);

	private:
		vector<JobNode> 			jobQueue;
		map<unsigned long, JobNode> jobMap;
		

};


#endif

