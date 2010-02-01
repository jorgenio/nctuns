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

#ifndef __NCTUNS_dispatcher_h__
#define __NCTUNS_dispatcher_h__

#include <stdio.h>
#include <mylist.h>
#include <exportStr.h>
#include <object.h>


/*========================================================================
   Define Export Table
  ========================================================================*/
struct exp_info {
	SLIST_ENTRY(exp_info)	nextinfo;
	NslObject		*module; 	/* pointer to module */
	const char		*mname;		/* Module name */
	const char		*vname;		/* variable name */
	u_char			flags;		/* R/W, see below */ 
};   
  
struct batch {
	char			*cmd;
	char			fname[100]; 
}; 

class Dispatcher; 
struct op {
	const char		*opname;
	int			(Dispatcher::*meth_)(const char *); 
}; 

/* define expState */
#define GET_SET_INIT		0x00
#define SET_SUCCESS		0x01
#define GET_SUCCESS		0x02


/* define expFlag */
#define EXP_FLAG_INIT		0x00
#define EXP_TO_SYS_ON		0x01
#define EXP_TO_GUI_ON		0x02
#define EXP_TO_TCSH_ON		0x03

/*========================================================================
   Define Dispatcher
  ========================================================================*/
                                                                                                                                                   
class Dispatcher {

 private :
 
	SLIST_HEAD(, exp_info)	exportTbl_[MAX_NUM_NODE + 1];
	char 			m_pszFrom[80];
	int 			m_shellid;

	struct ExportStr	*expStr_;
	char			expState;
	char			expFlag;
	FILE			*OutputFile;
	
	int 			append(char *cm);
	struct exp_info		*Tbl_lookup(u_int32_t, u_int32_t, const char *, const char *);
	char			gdbmesg[50];

 public :  

	Dispatcher();
	~Dispatcher(); 

	int		reg_export(NslObject *modu, const char *name, u_char flags); 
	int		init(); 
	void		get_NodeExportTbl(u_int32_t, ExportStr *);
	int		dispatching(const char *cm);
	int		GetRequest(const char *cm);
	int		SetRequest(const char *cm);
	int		Tcpdump(const char *cm);
	int		InfoRequest(const char *cm);
	int		GetAllRequest(const char *cm); 
	int		LayerQuery(const char *cm);
	int		ListCmd(const char *cm);
	int		GetIF(const char *cm);
	int 		Pause(const char *cm);
	int 		Continue(const char *cm);
	int 		Stop(const char *cm);
	
	void		SetSuccess();
	void		GetSuccess();
	void		GetExpStr(struct ExportStr *ExpStr);
	void		Set_expFlag(char state) { expFlag = state; }
	void		SetOutputFile(FILE *file) { OutputFile = file; }		

	int 		GdbMessage(int nid , const char *timing);
}; 

/* ========================================================================
	Other Functions
   ======================================================================== */
void NCTUns_exit();

#endif /* __NCTUNS_dispatcher_h__ */

