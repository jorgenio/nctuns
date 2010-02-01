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

#include <sys/types.h>

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <event.h>
#include <nctuns_api.h>
#include <nctuns_syscall.h>
#include <nctuns_divert.h>
#include <sysrt.h>
#include <object.h>
#include <dispatcher.h>
#include <maptable.h>
#if IPC
#include <IPC/ns.h>
#endif
#include <list>
#include <algorithm>

#include <config.h>
#include <gbind.h>

using namespace std;

Dispatcher *dispatcher_ = new Dispatcher;
const char *GET[] = {"Get", 0};
const char *SET[] = {"Set", 0, 0};

extern int errno;

struct op 	OPCODE[] = 
{
	{"Get", &Dispatcher::GetRequest},
	{"Set", &Dispatcher::SetRequest},
	{"GetAll", &Dispatcher::GetAllRequest},  
	{"ExportTable", &Dispatcher::InfoRequest},
	{"List", &Dispatcher::ListCmd},
	{"LayerQuery",	&Dispatcher::LayerQuery},
	{"GetIF", &Dispatcher::GetIF},
	{"Tcpdump", &Dispatcher::Tcpdump},
	{"Pause", &Dispatcher::Pause},
	{"Continue", &Dispatcher::Continue},
	{"Stop", &Dispatcher::Stop},
	{"Abort", &Dispatcher::Stop},
	{ NULL, NULL}
}; 

// use X macro to define command-retval mapping

#define CMD_TABLE \
X(Get, "Data") \
X(SetOK, "OK") \
X(SetFail, "Fail") 

#define X(a, b) a,
enum CMD
{
    CMD_TABLE
};
#undef X

#define X(a, b) b,
const char *g_RETVAL[] =
{
    CMD_TABLE
};
#undef X

// end of X macro definition


extern sysrt	*sysrt_;
 
extern list<int> g_tglist;
extern list<int> g_tglist;
extern list<tcpdumpInfo> g_tdInfolist;	// tcpdumpInfo list

int processBatch(Event_ *ep) {

	double			time; 
	struct batch		*bch;
	FILE			*OutputFile;
	char			*cmd;
 
	bch = (struct batch *)ep->DataInfo_;
	if( bch == NULL || bch->fname == NULL || bch->cmd == NULL ) {
		freeEvent(ep);
		return(-1);
	}

	char *FILEPATH = (char *)malloc(strlen(GetConfigFileDir())+
				strlen(bch->fname) + 1);
	sprintf(FILEPATH,"%s%s", GetConfigFileDir(), bch->fname);

	if ((OutputFile = fopen(FILEPATH, "a+")) != NULL) {
		TICK_TO_SEC(time, GetCurrentTime());
		fprintf(OutputFile,"\n\n================================\n");
		fprintf(OutputFile, "System commend : %s\n", bch->cmd);
		fprintf(OutputFile, "Trigger Time: %.3f\n", time);

		cmd = (char *)malloc(strlen(bch->cmd) + 15);
		sprintf(cmd, "%s%s", "From SYS : ", bch->cmd);		

		dispatcher_->SetOutputFile(OutputFile);
		dispatcher_->dispatching(cmd);
	}
	else {
		printf("Warning: can't open file %s\n",FILEPATH);
	}

	free(FILEPATH);
	free(cmd);
	fclose(OutputFile);
	OutputFile = NULL;

	
 	free(bch->cmd); 
	bch->cmd = NULL;  
	free(bch); 
	ep->DataInfo_ = NULL;  
	freeEvent(ep); 
	return(1);  
}


Dispatcher::Dispatcher() {

	int			i;
	
	/* Initialize export Table */
	for(i = 0; i < MAX_NUM_NODE; i++)
		SLIST_INIT(&exportTbl_[i]);

	m_pszFrom[0] = 0;
	m_shellid = -1;

	expStr_ = NULL;
	expState = GET_SET_INIT;
	OutputFile = NULL;
	expFlag = EXP_FLAG_INIT;
}


Dispatcher::~Dispatcher() {

}



int
Dispatcher::init() {

	FILE			*fd;
	u_int32_t		time_sec; 
	u_int64_t		time_tick; 
 	char			buf[135];
	const char		*sep = "\t";
 	char			*tr;
	Event_			*ep; 
	struct batch		*bch; 


	/* 
	 * Open batch file, if fault return.
	 */
	char *FILEPATH = (char *)malloc(strlen(GetScriptName())+5);
	strcpy(FILEPATH, GetScriptName());
	sprintf(FILEPATH, "%s%s", FILEPATH, ".sct");

	if ((fd=fopen(FILEPATH, "r")) == NULL) {
 		printf("Warning: can't open file %s\n", FILEPATH);
		return(-1);
	}
	free(FILEPATH);

	/*  
	 * If it is success to open bat file then
	 * for every entry in bat file, create an 
	 * event.
	 */
	while(!feof(fd)) {
		buf[0] = '\0'; fgets(buf, 100, fd);
		if ((buf[0]=='\0')||(buf[0]=='#'))
                        continue;
		buf[strlen(buf)-1] = '\0';  

		/* 
		 * The format of every entry is as follows: 
		 *   - {Trigger Time}+ \t + {Command} + \t + {Output filename}
		 *   - The unit of trigger time is second.    
		 */
		bch = (struct batch *)malloc(sizeof(struct batch));
		assert(bch);
 
		tr = strtok(buf, sep);	// time
		sscanf(tr, "%u", &time_sec);
		SEC_TO_TICK(time_tick, time_sec);  

		tr = strtok(NULL, sep);	// command
		bch->cmd = (char *)malloc(strlen(tr)+1);
		assert(bch->cmd);
		strcpy(bch->cmd, tr);      

		tr = strtok(NULL, sep); // output file name
		strcpy(bch->fname, tr);

 		ep = createEvent();
		setFuncEvent(ep, time_tick, 0, processBatch, (void *)bch); 
	}
	fclose(fd); 

	return 0;
}



int
Dispatcher::reg_export(NslObject *modu, const char *name, u_char flags) {

	struct exp_info		*e, *te ;
	const char		*module_name;


	module_name = getModuleName(modu);
	assert(module_name);
	
	/* 
	 * Check duplicate register 
	 *
	 *	Index-Key : {nodeID, portID, moduleName, varName}
	 */
	SLIST_FOREACH(e, &(exportTbl_[modu->get_nid()]), nextinfo) {
 	    if ( (modu->get_port()==e->module->get_port()) &&
	         (!strcmp(module_name, e->mname)) &&
		 (!strcmp(name, e->vname)) ) {
		/* duplicate found! */
		return(-1);
	    }
	}

	e = (struct exp_info *)
		malloc(sizeof(struct exp_info));
	assert(e);
	e->module = modu;
	e->mname  = module_name;
	e->vname  = name;
	e->flags  = flags;
	e->nextinfo.sle_next = 0;
	SLIST_INSERT_TAIL(te, &(exportTbl_[modu->get_nid()]), e, nextinfo);
	
	return(1); 
}


void
Dispatcher::get_NodeExportTbl(u_int32_t node, ExportStr *ExpStr) {

 	struct exp_info		*einfo; 
	char			tmpBuf[100]; 
	const char		*attr;


	if (exportTbl_[node].slh_first == 0)
		return;

	/* print title */
	sprintf(tmpBuf, "Export-Table of node %u:\n", node);
	ExpStr->Insert_comment(tmpBuf);
	sprintf(tmpBuf, "Port-ID\tModule-Name\tVar-Name\t Attr\n");
	ExpStr->Insert_comment(tmpBuf);
	
	/* Get one node's Export-Table */
	SLIST_FOREACH(einfo, &(exportTbl_[node]), nextinfo) {
		switch(einfo->flags) {

			case E_RONLY: 
				attr = " Get Only"; 
				break;
			case E_WONLY: 
				attr = " Set Only"; 
				break;
			case E_RONLY|E_WONLY: 
				attr = " Get/Set"; 
				break;
			default : 
				attr = NULL;
		}

		sprintf(tmpBuf, "  %4u\t%-13s\t%-13s\t%s\n", 
			einfo->module->get_port(), einfo->mname, 
			einfo->vname, attr);
		ExpStr->Insert_comment(tmpBuf);
	}
	ExpStr->Insert_comment("\n\n");
};


int
Dispatcher::InfoRequest(const char *cm) {
	int			i;
	struct ExportStr	*ExpStr;

	ExpStr = new ExportStr(1);

	for(i = 1; i < MAX_NUM_NODE; i++) {
		get_NodeExportTbl(i, ExpStr);
	}

	if( expFlag == EXP_TO_SYS_ON ) {
		if( OutputFile != NULL)
			fprintf(OutputFile, ExpStr->ExportStr_to_FILE());
	}

	ExpStr->~ExportStr();

	return(1); 
}



int
Dispatcher::GetAllRequest(const char *cm) {

	struct exp_info		*einfo;
	u_int32_t		nid;
	char			mname[50]; 
	char			vname[50];
	char			tmpBuf[200];
	struct ExportStr	*ExpStr;

	sscanf(cm, "%s %s", mname, vname);

	ExpStr = new ExportStr(1);

	for(nid = 1; nid < MAX_NUM_NODE; nid++) {
		
		if (exportTbl_[nid].slh_first == NULL)
			continue;
			
		SLIST_FOREACH(einfo, &(exportTbl_[nid]), nextinfo) {
		  if ((einfo->flags&E_RONLY) &&
		    (!strcmp(einfo->mname, mname)) &&
		      (!strcmp(einfo->vname, vname))) { 
			GET[1] = einfo->vname;
 
                      	sprintf(tmpBuf, "Get %u %u %s %s:\n", 
		       		(einfo->module)->get_nid(), 
				(einfo->module)->get_port(),mname, vname);
			ExpStr->Insert_comment(tmpBuf);
			
			expStr_ = NULL;
			expState = GET_SET_INIT;
			(einfo->module)->command(2, GET);  
			if( expState == GET_SUCCESS && expStr_ != NULL ) {
				ExpStr->Insert_comment(
					expStr_->ExportStr_to_FILE());
				expStr_->~ExportStr();
			}
		  }
		}
	}    


	if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
		char *outstr = (char*)malloc(
			ExpStr->Get_ExportFILE_Strlen()+9);
		sprintf(outstr, "%s%s", "SUCCESS:",ExpStr->ExportStr_to_FILE());

		sendtoGUI(outstr, strlen(outstr));
		free(outstr);
#endif
	}
	else if( expFlag == EXP_TO_SYS_ON ) {
		if( OutputFile != NULL )
			fprintf(OutputFile, ExpStr->ExportStr_to_FILE());
	}	

	ExpStr->~ExportStr();

	return 1; 
}

  
int
Dispatcher::GetRequest(const char *cm)
{
	u_int32_t		nodeid, portid; 
	char			varname[80]; 
	char			mname[80];
	struct exp_info		*einfo; 

 	sscanf(cm, "%d %d %s %s", &nodeid, &portid, mname, varname);

	/* 
	 * Give a chance to get Kernel-Routing Table, if
	 * the request is a routing-table request.
	 */

	if ( !strcmp(mname, "SYSRT") &&
	     !strcmp(varname, "System-Routing-Table") ) {
		expStr_ = NULL;
		expState = GET_SET_INIT;
		sysrt_->get_rt(nodeid);

		if( expState == GET_SUCCESS && expStr_ != NULL ) {
			if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
				expStr_->Insert_header("SUCCESS:");
				sendtoGUI(expStr_->ExportStr_to_GUI(),
					expStr_->Get_ExportGUI_Strlen());
#endif
			}
			else if( expFlag == EXP_TO_SYS_ON ) {
				if( OutputFile != NULL )
					fprintf(OutputFile,
						expStr_->ExportStr_to_FILE());
			}
			expStr_->~ExportStr();
		}
		else {
			if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
				sendtoGUI("FAIL:", 6);
#endif
			}
			else if( expFlag == EXP_TO_SYS_ON ) {
				if( OutputFile != NULL)
					fprintf(OutputFile, "FAIL:");
			}
		} 
		
		return 1;
	}

 	einfo = Tbl_lookup(nodeid, portid, mname, varname);

	if (einfo&&(einfo->flags&E_RONLY)) {
		GET[1] = einfo->vname;  

		expStr_ = NULL;
		expState = GET_SET_INIT;
		einfo->module->command(2, GET);

		if( expState == GET_SUCCESS && expStr_ != NULL ) {
			if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
				expStr_->Insert_header("SUCCESS:");
				sendtoGUI(expStr_->ExportStr_to_GUI(),
					expStr_->Get_ExportGUI_Strlen());
#endif
			}
			else if( expFlag == EXP_TO_SYS_ON ) {
				if( OutputFile != NULL )
					fprintf(OutputFile, 
						expStr_->ExportStr_to_FILE());
			}
			expStr_->~ExportStr();
		}
		else {
			if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
				sendtoGUI("FAIL:", 6);
#endif
			}
			else if( expFlag == EXP_TO_SYS_ON ) {
				if( OutputFile != NULL)
					fprintf(OutputFile, "FAIL:");
			}
		}		
	}
	else {
		if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
			sendtoGUI("FAIL:", 6);
#endif
		}
		else if( expFlag == EXP_TO_SYS_ON ) {
			if( OutputFile != NULL )
				fprintf(OutputFile, "FAIL:");
		}
	}

	return 1;   
}

  
int
Dispatcher::SetRequest(const char *cm) 
{
	u_int32_t nodeid, portid;
 	char varname[80];
	char varvalue[80] = "1";	
	char mname[80];
	struct exp_info	*einfo;

	sscanf(cm, "%d %d %s %s %s", &nodeid, &portid, mname, varname, varvalue);

	einfo = Tbl_lookup(nodeid, portid, mname, varname);
	if (einfo&&(einfo->flags&E_WONLY)) {
		SET[1] = einfo->vname;
		SET[2] = varvalue;   
	
		expState = GET_SET_INIT;
		einfo->module->command(3, SET);

		if( expState == SET_SUCCESS ) {
			if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
				sendtoGUI("SUCCESS:", 9);
#endif
			}
			else if( expFlag == EXP_TO_SYS_ON ) {
				if( OutputFile != NULL)
					fprintf(OutputFile, "SUCCESS:");
			}
		}
		else {
			if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
				sendtoGUI("FAIL:", 6);
#endif
			}
			else if( expFlag == EXP_TO_SYS_ON ) {
				if( OutputFile != NULL)
					fprintf(OutputFile, "SUCCESS:");
			}	
		}
	}
	else {
		if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
			sendtoGUI("FAIL:", 6);
#endif
		}
		else if( expFlag == EXP_TO_SYS_ON ) {
			if( OutputFile != NULL )
				fprintf(OutputFile, "SUCCESS:");
		}	
	}

	return 1;
}

int
Dispatcher::Tcpdump(const char *cm) 
{
	u_int32_t nodeid, portid;
	char turnFlag[10];
	char mname[80];
 	char varname[80];
	struct exp_info	*einfo;

	sscanf(cm, "%d %d %s %s %s", &nodeid, &portid, mname, varname, turnFlag);

	einfo = Tbl_lookup(nodeid, portid, mname, varname);

	if( !einfo )
		einfo = Tbl_lookup(nodeid, portid, "WTCPDUMP", varname);

	if (einfo&&(einfo->flags&E_WONLY)) {
		SET[1] = einfo->vname;
		SET[2] = turnFlag;   
	
		expState = GET_SET_INIT;
		einfo->module->command(3, SET);
	}

	return 1;
}


int 
Dispatcher::LayerQuery(const char *cm)
{
	int 		nodeid;
	u_char 		uLayer;
#if IPC
	char		tmpBuf[50];
#endif

	sscanf(cm, "%d", &nodeid);
	uLayer = getNodeLayer(nodeid);

	if (uLayer != 0) {
		if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
			sprintf(tmpBuf, "SUCCESS: %d",(int)uLayer);
			sendtoGUI(tmpBuf, strlen(tmpBuf));
#endif
		}
		else if( expFlag == EXP_TO_SYS_ON ) {
			if( OutputFile != NULL )
				fprintf(OutputFile, "SUCCESS: %d", 
							(int)uLayer);
		}
	}
	else {
		if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
			sendtoGUI("FAIL:", 6);
#endif
		}
		else if( expFlag == EXP_TO_SYS_ON )
			if( OutputFile != NULL )
				fprintf(OutputFile, "FAIL:"); 
	}
	return 1;
}


int
Dispatcher::ListCmd(const char *cm) 
{
	char	tmpBuf[400];

	sprintf(tmpBuf, "SUCCESS:%s\n%s\n%s\n%s\n%s\n%s\n%s\n%s\n",
	"- {Set} {nodeID} {portID} {module_name} {varialbe_name} {value}",
	"- {Get} {nodeID} {portID} {module_name} {variable_name}",
	"- {List}",
	"- {GetIF} {nodeID}",
	"- {LayerQuery} {nodeID}",
	"- {GetAll} {module_name} {variable_name}",
	"- {ExportTable}",
	"- {Tcpdump} {nodeID} {portID} {module_name} {variable_name} {turnFlag}");

	if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
		sendtoGUI(tmpBuf, strlen(tmpBuf));
#endif
	}
	else if( expFlag == EXP_TO_SYS_ON )
		if( OutputFile != NULL )
			fprintf(OutputFile, tmpBuf); 

	return 1;
}


int
Dispatcher::GetIF(const char *cm)
{
	int 			nodeid;
	int			ifcnt;
	struct maptable 	*pMaptable;
	struct if_info		*ifinfo;
	struct ExportStr	*ExpStr;
	char 			tmpBuf[150];
	char 			IP_[20]; 
	char			Netmask_[20]; 
	char			Mac_[30];
	u_int32_t		row,column;

	sscanf(cm, "%d", &nodeid);
	pMaptable = mtbl_getnidinfo(nodeid);
	ifcnt = pMaptable->portnum;
	
	ExpStr = new ExportStr(4);
	sprintf(tmpBuf,"Interfaces of Node %d:\n",nodeid);
	ExpStr->Insert_comment(tmpBuf);
	sprintf(tmpBuf,"Iface\tIP\t\tNetmask\t\t\tMAC\n");
	ExpStr->Insert_comment(tmpBuf);


	SLIST_FOREACH(ifinfo, &(pMaptable->ifinfo), nextif) {

		ipv4addr_to_str(*(ifinfo->ip), IP_);
		ipv4addr_to_str(*(ifinfo->netmask), Netmask_);
		macaddr_to_str(ifinfo->mac, Mac_);
	
		row = ExpStr->Add_row();
		column = 1;
		ExpStr->Insert_cell(row, column++, ifinfo->name, "\t");
		ExpStr->Insert_cell(row, column++, IP_, "\t\t");
		ExpStr->Insert_cell(row, column++, Netmask_, "\t\t");
		ExpStr->Insert_cell(row, column++, Mac_, "\n");
	}

	if( expFlag == EXP_TO_GUI_ON ) {
#if IPC
		char *outstr = (char*)malloc(
			ExpStr->Get_ExportFILE_Strlen()+9);
		sprintf(outstr, "%s%s", "SUCCESS:",ExpStr->ExportStr_to_FILE());
		sendtoGUI(outstr, strlen(outstr));
		free(outstr);
#endif
	}
	else if( expFlag == EXP_TO_SYS_ON ) {
		if( OutputFile != NULL )
			fprintf(OutputFile, ExpStr->ExportStr_to_FILE());
	}

	ExpStr->~ExportStr();

	return 1;
}

struct exp_info *
Dispatcher::Tbl_lookup(u_int32_t nodeid, u_int32_t portid, 
	const char *mname, const char *vname) 
{
	struct exp_info		*einfo;


	/* Index-Key: {nodeID, portID, moduleName, varName} */
	SLIST_FOREACH(einfo, &(exportTbl_[nodeid]), nextinfo) {
		if ( (einfo->module->get_port()==portid) &&
		     (!strcmp(einfo->mname, mname)) &&
		     (!strcmp(einfo->vname, vname)) )  {
			/* desired info is found. */
			return(einfo);
		}
	}
	return(0);     
}

/*
 * Command Format:
 *
 *  - Format:
 *	- [From] [SYS|GUI|TCSH] [:] {opcode}
 *  - Supported OP Code:
 *	- {Set} {nodeID} {portID} {module_name} {variable_name} {value}
 *	- {Get} {nodeID} {portID} {module_name} {variable_name}
 *	- {List}
 *	- {GetIF} {nodeID}
 *	- {LayerQuery} {nodeID}
 *	- {GetAll} {module_name} {variable_name}
 *	- {ExportTable}
 *	- {Tcpdump} {nodeID} {portID} {module_name} {variable_name} {turnFlag}
 */
int 
Dispatcher::dispatching(const char *cm) {
	char head[80], from[80], opcode[80];
	char c;
	const char *pch;

	if( 4 != sscanf(cm, "%s %s %c %s", head, from, &c, opcode) )
		return 0;

	if (strcmp(head, "From")) {
		printf("Error: Dispatcher: incorrect command %s\n",cm);
		return -1;
	}

	/* set export state */
	strcpy(m_pszFrom, from); 
	if (!strcmp(m_pszFrom, "TCSH"))
		Set_expFlag(EXP_TO_TCSH_ON);
	else if (!(strcmp(m_pszFrom, "GUI")))
		Set_expFlag(EXP_TO_GUI_ON);
	else if (!(strcmp(m_pszFrom, "SYS")))
		Set_expFlag(EXP_TO_SYS_ON);


	/* table lookup */
	for (int i = 0; OPCODE[i].opname; i++) {
		if (!strcmp(opcode, OPCODE[i].opname)) {
			pch = strstr(cm, opcode);
//			pch += strlen(opcode);

			(this->*(OPCODE[i].meth_))(pch + strlen(opcode));

			Set_expFlag(EXP_FLAG_INIT);
			return (1);
		}  
	}

	Set_expFlag(EXP_FLAG_INIT);
	printf("Error: Dispatcher: Unknown opcode!\n");
	return (-1);  
}

void
Dispatcher::SetSuccess() {

	expState = SET_SUCCESS;
}

void
Dispatcher::GetSuccess() {

	expState = GET_SUCCESS;
}

void
Dispatcher::GetExpStr(struct ExportStr *ExpStr) {

	expStr_ = ExpStr;
}

//
// Pause all traffic-generator processes
// and return an ACK(by sendtoGUI).
//
int
Dispatcher::Pause(const char *cm)
{
	list<int>::iterator itr;
	int pid;
#if IPC
	//static const char *ack = "Pause OK";
	const char *const ack = "Pause OK";
#endif
	list<tcpdumpInfo>::iterator tditr, tditrEnd;
	char dis_buf[256];
	void (*sigchld)(int);

	sigchld = signal(SIGCHLD,SIG_IGN);

	tditrEnd = g_tdInfolist.end();
	for (tditr = g_tdInfolist.begin(); tditr != tditrEnd; tditr++) {
		// turn off the tcpdump-module's flag
		sprintf(dis_buf, "From TCSH : Tcpdump %d %d TCPDUMP DumpFlag off\n", ((tcpdumpInfo)*tditr).nid, ((tcpdumpInfo)*tditr).portid);
		dispatcher_->dispatching(dis_buf);
	}


	for (itr = g_tglist.begin(); itr != g_tglist.end(); itr++) {
		pid = (int)(*itr);

		printf("1. current ticks = %llu, pause process of pid %d\n", GetCurrentTime(), pid);
		fflush(stdout);

		if (kill(-pid, SIGSTOP) <0)perror("kill():");
	}
	signal(SIGCHLD, sigchld);

#if IPC
	sendtoGUI(const_cast<char *>(ack), strlen(ack));
#endif

	return 1;
}

int
Dispatcher::GdbMessage(int nid , const char *timing)
{
	list<int>::iterator itr;
	int pid;
#if IPC
	memset(gdbmesg , 0 , sizeof(gdbmesg));
	sprintf(gdbmesg , "[%d] %s Forking OK" , nid , timing);
#endif

	list<tcpdumpInfo>::iterator tditr, tditrEnd;
	char dis_buf[256];
	void (*sigchld)(int);

	sigchld = signal(SIGCHLD,SIG_IGN);

	tditrEnd = g_tdInfolist.end();
	for (tditr = g_tdInfolist.begin(); tditr != tditrEnd; tditr++) {
		// turn off the tcpdump-module's flag
		sprintf(dis_buf, "From TCSH : Tcpdump %d %d TCPDUMP DumpFlag off\n", ((tcpdumpInfo)*tditr).nid, ((tcpdumpInfo)*tditr).portid);
		dispatcher_->dispatching(dis_buf);
	}

	for (itr = g_tglist.begin(); itr != g_tglist.end(); itr++) {
		pid = (int)(*itr);

		printf("1. current ticks = %llu, pause process of pid %d\n", GetCurrentTime(), pid);
		fflush(stdout);

		if (kill(-pid, SIGSTOP) <0)perror("kill():");
	}
	signal(SIGCHLD, sigchld);

#if IPC
	sendtoGUI(const_cast<char *>(gdbmesg), strlen(gdbmesg));
#endif
	return 1;
}

//
// Wakeup all traffic-generator processes
// and retun an ACK(by sendtoGUI).
//
int
Dispatcher::Continue(const char *cm)
{
	list<int>::iterator itr;
	int pid;
#if IPC
	static const char *const ack = "Continue OK";
#endif
	list<tcpdumpInfo>::iterator tditr, tditrEnd;
	char dis_buf[256];

	tditrEnd = g_tdInfolist.end();
	for (tditr = g_tdInfolist.begin(); tditr != tditrEnd; tditr++) {
		// turn on the tcpdump-module's flag
		sprintf(dis_buf, "From TCSH : Tcpdump %d %d TCPDUMP DumpFlag on\n", ((tcpdumpInfo)*tditr).nid, ((tcpdumpInfo)*tditr).portid);
		dispatcher_->dispatching(dis_buf);
	}

	for (itr = g_tglist.begin(); itr != g_tglist.end(); itr++) {
		pid = (int)(*itr);

		printf("current ticks = %llu, wakeup process of pid %d\n", GetCurrentTime(), pid);
		fflush(stdout);

		kill(-pid, SIGCONT);
	}

#if IPC
	sendtoGUI(const_cast<char *>(ack), strlen(ack));
#endif

	return 1;
}

//
// Kill all traffic-generator processes
// and return an ACK(by sendtoGUI).
//
int
Dispatcher::Stop(const char *cm)
{
#if IPC
	static const char *const ack = "Stop OK";
#endif

	printf("=>Dispatcher::Stop()\n");

	/* Do something which should be executed when SE exits */
	NCTUns_exit();

/*	for (list<int>::iterator itr = g_tglist.begin(); itr != g_tglist.end(); itr++) {
		printf("current ticks = %llu, kill process of pid %d\n", GetCurrentTime(), (int)(*itr));
		fflush(stdout);
		kill(-(int)(*itr), SIGKILL);
	}
*/
	/* Flush divert rules of emulation */
//	syscall_NCTUNS_divert(syscall_NSC_divert_FLUSH, 0, 0, 0, 0);

#if IPC
	printf("Dispatcher::Stop(): call sendtoGUI\n");
	sendtoGUI(const_cast<char *>(ack), strlen(ack));
#endif

	//
	// Note: XXX
	//       i assume the coordinator will kill me when he receives
	//       the ACK. So i don't have to remove events which will
	//       kill these traffic-generator processes.
	//

	return 1;
}

