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
#include <sys/signal.h>
#include <sys/utsname.h>

#include "Dispatcher.H"
#include "ServerSocket.H"
#include "Variables.H"
#include "Mediator.H"


/* The Mediator: brain of dispatcher */
static Mediator*		mediator;	
/* Socket Manager that handles connections */
static SocketManager* 	socketManager;
/* A list that stores available simulation servers*/
static ServerList		serverList;
/* File name of the file that stores session infomation*/
string session_file;
/* Log file that logs access to the dispatcher */
string log_file;
/* Environment Variable: $NCTUNSHOME */
string nctuns_home;

/*--------------------------------------------------------------------------*/

/*
 *   Main function of the Dispatcher
 */
int main (void) {
	
	Dispatcher dispatcher;
	
	dispatcher.initialize();
	
	dispatcher.run();
	
	exit(0);

}

/*--------------------------------------------------------------------------*/

	/*
	 * Run the Dispatcher
	 *
	 */
	void Dispatcher::run(){
		
		for ( ; ; ){
			/* Build select list for the first time */
			socketManager->buildSelectList();
			/* select file descriptor, which has something to read from */
			socketManager->Select();
		}	
	}
	
/*--------------------------------------------------------------------------*/

	/*
	 * Initialize the Dispatcher
	 *
	 */
	void Dispatcher::initialize (){
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

		/* Only root can run dispatcher. Therefore we check whether uid == 0 */
		uid = getuid();
		if (uid != 0){
			cerr << endl << "Sorry!! dispatcher needs root's privilege to operate properly." << endl;
			cerr << "Please switch to root to run dispatcher" << endl << endl;
			exit(1);
		}
		
		/* 
		 * Make sure $NCTUNSHOME is set before we make any further operation
		 * On it
		 */
		const char* nctunsHome = getenv("NCTUNSHOME");
		if (nctunsHome == NULL){
			cerr << "Environment \"NCTUNSHOME\" not set"  << endl << endl;
			exit(1);
		}
		nctuns_home  = string(nctunsHome);
	
		/* 
		 * Set a session_file variable to session file in the file system
		 */
		char *home = getenv("HOME");
		string make = "mkdir -p " + string(home) + "/.nctuns/dispatcher/";
		system(make.c_str());
		session_file = string(home) + "/.nctuns/dispatcher/session_file";
		
		/*
		 * Read configuration file
		 * Store variables in the configuration file to the var object
		 */
		string cfg_file = nctuns_home + "/etc/dispatcher.cfg";
		readConfig(cfg_file);
			
		/*
		 * Initialize log file variable
		 */
		log_file = nctuns_home + "/etc/access.log";
		
		/* 
		 * Ignore Pipe signal to prevent broken pipe
		 * We check epipe error in IOStream.C 
		 */
		signal(SIGPIPE, SIG_IGN);
		
		/*
		 * Initialize server sockets and pass them to the new socket manager
		 */
		vector<int> listenfds = setupNetwork();		
		socketManager  = new SocketManager(listenfds);

		/* Initialize mediator */
		mediator = new Mediator(socketManager, &serverList);		
	}

/*--------------------------------------------------------------------------*/

		
	/* 
	 * Read configuration file for the Dispatcher
	 * 
	 * @param filename	filename of the configuration file
	 */
	void Dispatcher::readConfig (string filename) {
		
		/* Read the configuration file */	
		ifstream inFile(filename.c_str());
		if(!inFile){
			cerr << "Unable to open configuration file, system exit\n";
			cerr << "Please check if you have setenv NCTUNSHOME\n";
			exit(1);
		}
	
		int  line_length = 256;
		char line[line_length]; // Buffer that stroes each line read

		while (inFile.getline(line, line_length)){
			/* if the line is not comment, marked by "#" */
			if(strlen(line)>0 && line[0] != '#'){
				/* configuration file format is "name   port"  */
				string name = strtok(line, " \t");
				int port;
				sscanf(strtok(NULL, " \t"), "%d", &port);				
				/* put into variable table ....see Variable.H*/ 
				var.setVariable(name, port);
				
				// @@@@@ print out a message of the parameter read
				//cout << name << "\t|\t" << var.getVariable(name)  <<endl;
			
			} //if
		} //while
	
		inFile.close();
	}

/*--------------------------------------------------------------------------*/
	
	/*
	 * Initialize network connections for the Dispatcher
	 *
	 */
	vector<int> Dispatcher::setupNetwork(){
		
		int bindCoordinatorPort = var.getVariable(var.coordinatorPort);
		ServerSocket coorSocket(bindCoordinatorPort);
		 
		int bindGUIClientPort 	 = var.getVariable(var.clientPort);
		ServerSocket guiSocket (bindGUIClientPort);
		
		const int length = 2;
		int fds[length] = {coorSocket.getListenFD(), guiSocket.getListenFD()}; 
		vector<int> listenfds (fds, fds+length);
		return listenfds;			
	}

/*--------------------------------------------------------------------------*/

