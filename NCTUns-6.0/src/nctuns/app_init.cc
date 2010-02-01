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

#include <stdlib.h>
#include <assert.h>
#include <tcl.h>
#include <tclBinder.h>
#include <tclObject.h>
#include <tun_mmap.h>
#include <app_init.h>
#include <gbind.h>


extern TclObject		cmd_Interp_;
u_int32_t			__errorLine__ = 0;
 
const char *const cmds[] = 
{
 "Run", "List", "Set", "Create", "EndCreate", "Module", "Define",
 "EndDefine", "Bind", "Connect", "Debug", "Group",
 NULL 
};
 
 
int Command_Dispatch(ClientData client_data,
	Tcl_Interp* interp,
	int argc, char *argv[])
{
	int		error;
	int		i;

	__errorLine__ ++;

	/* by pass to tclObject to dispatch command */
	error = cmd_Interp_.Command_Dispatch(client_data, interp, argc, argv);
	if (error == TCL_ERROR) {
		printf("nctuns: parse error --> ");
		for( i=0; i<argc; i++ ) {
			printf("%s ",argv[i]);
		}
		printf("\n");
		exit(-1);
	}
	return TCL_OK; 
}



int Tcl_AppInit(Tcl_Interp *interp) {

	int             	status;
	int			i; 
 

	status = Tcl_Init(interp);
	if (status != TCL_OK)
		return(TCL_ERROR);

	/* Create our own TCL command */
	for(i=0 ; cmds[i]; i++) {
		Tcl_CreateCommand(interp,
				cmds[i],
				(Tcl_CmdProc*)Command_Dispatch,
				(ClientData) NULL,
				(Tcl_CmdDeleteProc*)NULL);
	}      

	return(TCL_OK);
}


int Start_Simulator(int argc, char *argv[]) {

	int			i; 
	extern char		*script_;
	extern char		*FILEDIR_;

  
	if (argv[1]) {
		/* copy script's file name */
		script_ = (char *)malloc(strlen(argv[1])+1);
  		assert(script_);
		strcpy(script_, argv[1]);
		for(i = strlen(script_); i >= 0; i--) {
      			if (script_[i] == '.') {
					script_[i] = '\0';
					break;
			}
 		}

		/* get directory of the script file */
		FILEDIR_ = (char *)malloc(strlen(script_) + 1);
		strcpy(FILEDIR_, script_);

		for(int i = strlen(FILEDIR_); i>=0; i--) {
			/*
			 * eg: demo_case/demo_case.tcl => demo_case/
			 */
			if(FILEDIR_[i] == '/') {
				FILEDIR_[i+1] = '\0';
				break;
			}
		}
	}

	/* mmap to tunnel device */
        tun_mmap();

	/* Global Variable binding Initial */
	gBind_Init();
	
	/*
	 * Start the tcl application
	 */
	Tcl_Main(argc, argv, Tcl_AppInit);
	return(0);  
}
