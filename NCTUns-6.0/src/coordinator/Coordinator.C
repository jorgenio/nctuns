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
#include "Coordinator.H"

#include <sys/signal.h>
#include <sys/utsname.h>

/*--------------------------------------------------------------------------*/

/*
 *   Main function of the Coordinator
 */

int
main (int argc, char *argv[]) {
	
	Coordinator coordinator;
/*	
	if (argc < 2){
		cout << endl << "Usage:   Coordinator [Dispatcher IP] [Dispatcher Port]" << endl;
		cout << "Example: Coordinator 140.113.214.89  9810" << endl << endl;
		exit(1);
	}
	else{	
		coordinator.initialize(string(argv[1]), atoi(argv[2]));	
	}		
*/
	coordinator.initialize();	
	
	coordinator.run();
	
	exit(0);
	

}


	
/*--------------------------------------------------------------------------*/

void
Coordinator::initialize(){
	
	setEnv();

		
	if (connectDispatcher() <0){
		cout << endl << "Cannot Connect to Dispatcher!" << endl;
		cout << "Please check your configuration file at: " << coordinator_cfg << endl << endl;
		exit(1);
	}
	
   /* Fix Broken Pipe Problem Dec 18, 2002 by oli with new IOStream.C*/
	signal(SIGPIPE, SIG_IGN);
	
	socketManager = new SocketManager(dispatcher_fd);

	mediator = new Mediator(socketManager);
	
}


/*--------------------------------------------------------------------------*/

void 
Coordinator::run(){
	
	int count = 0;

	for ( ; ; ){
			//cout << endl;
			socketManager->buildSelectList();			
			socketManager->Select();
			//cout << "<Count= " << ++count << ">\n" << flush;	
	}

}



/*--------------------------------------------------------------------------*/

int 
Coordinator::connectDispatcher(){
	
	Socket dispatcherSock(dispatcherIP, dispatcherPort);
	dispatcher_fd = dispatcherSock.getSocketFD();
	return dispatcherSock.Connect();
}


/*--------------------------------------------------------------------------*/

void 
Coordinator::setEnv(){
	
	struct utsname uname_pointer;
	int uid;

	/*
	   Only NCTUns kernel can run dispatcher. Therefore we check whether 
	   release version of uname include 'nctuns'
	   2006/03/28 by ystseng
	*/
	uname(&uname_pointer);
	if (strstr(uname_pointer.release, "nctuns") == NULL) {
		cerr << endl << "Sorry!! This kernel is not nctuns kernel. Please select nctuns kernel to boot." << endl << endl;
		exit(1);
	}

	uid = getuid();
	if (uid != 0){
		cerr << endl << "Sorry!! coordinator needs root's privilege to operate properly." << endl;
		cerr << "Please switch to root to run coordinator" << endl << endl;
		exit(1);
	}

	const char* nctuns_home  = getenv("NCTUNSHOME");
	//const char* nctuns_tools = getenv("NCTUNS_TOOLS");
	//const char* nctuns_bin	 = getenv("NCTUNS_BIN");
	if (nctuns_home == NULL){
		cerr << "Environment \"NCTUNSHOME\" not set"  << endl << endl;
		exit(1);
	}
	string tools = string(nctuns_home) + "/tools/";
	string bin   = string(nctuns_home) + "/bin/";
	
	setenv("NCTUNS_TOOLS", tools.c_str(), 1);
	setenv("NCTUNS_BIN", bin.c_str(), 1);
	
	cout << getenv("NCTUNS_BIN") << endl;
	/*
	if (nctuns_tools == NULL){
		cerr << "Environment \"NCTUNS_TOOLS\" not set"  << endl << endl;
		exit(1);
	}
	if (nctuns_bin == NULL){
		cerr << "Environment \"NCTUNS_BIN\" not set"  << endl << endl;
		exit(1);
	}
	*/
	coordinator_cfg = string(nctuns_home) + "/etc/coordinator.cfg";
	readConfig(coordinator_cfg);
	
	return;
}


/*--------------------------------------------------------------------------*/
		
	/* 
	 * Read configuration file for the Dispatcher
	 * 
	 * @param filename	filename of the configuration file
	 */
	void Coordinator::readConfig (string filename) {
		
		/* Read the configuration file */	
		ifstream inFile(filename.c_str());
		if(!inFile){
			cerr << "Unable to open configuration file at: " << filename << "system exit\n";
			exit(1);
		}
	
		char line[256];

		while (inFile.getline(line, 256)){
			/* if the line is not comment, marked by "#" */
			if(strlen(line)>0 && line[0] != '#'){
				/* configuration file format is "name   port"  */
				string name = strtok(line, " \t");
				if (name == "DISPATCHER_IP"){
					dispatcherIP = strtok(NULL, " \t");
				} //if
				else{
					int port;
					sscanf(strtok(NULL, " \t"), "%d", &port);
					if (name == "DISPATCHER_PORT")
						dispatcherPort = port;			
			/*
					else if (name == "CLIENT_COMMAND")
						clientCommand = port;	
					else if (name == "CLIENT_DATA")
						clientData = port;
					else if (name == "TIME_PORT")
						timePort = port;
			*/
					// @@@@@ print out a message of the parameter read					
				}//else
			
			} //if
		} //while
		inFile.close();
	}//readConfig

/*--------------------------------------------------------------------------*/

