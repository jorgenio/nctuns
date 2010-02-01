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

#ifndef  __Variables
#define  __Variables

#include <string>
#include <map>

using namespace std;


class Variables {

	
	public:
	
		/*
		 * If you put a variable into the map, you can keep the name as the key here,
		 * and therefore, you can remember it later on without keeping the information
		 * in the main program.
		 */
		string coordinatorPort;
		string clientPort;
		
		
/*--------------------------------------------------------------------------*/	

		/* Constructor of the class*/
		Variables(){
			coordinatorPort = "COORDINATOR_PORT";
			clientPort 		= "CLIENT_PORT";			
		};
				
		/* 
		 * Set variable value of the map
		 * 
		 * @param key	the key to put value to the map
		 * @param value the value to put
		 */		
		void setVariable(string key, int value){
		    var_map.insert(map<string, int>::value_type(key, value));	
		};
		 
		/* 
		 * Get varialbe out from the map
		 *
		 * @param key	the key to get value from the map
		 * @return		the value associate to the key
		 */ 
		int  getVariable(string key){
			return (var_map[key]);
		};
		
		/*		
		void dumpToFile(){
			
			TO DO: save the variable table into the file system
			
			1. open a file (ex. Dispatcher.conf.curr)
			2. toString()
			3. write into the file
			
		};
		*/
		
		/*
		void toString(){
			
			TO DO: print out the contents in the variable table.
			
		};	
		*/
	
/*--------------------------------------------------------------------------*/


	private:
		
		map <string, int> var_map ;
	
};

#endif
