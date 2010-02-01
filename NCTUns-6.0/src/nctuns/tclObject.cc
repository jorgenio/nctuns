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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <tclBinder.h>
#include <tclObject.h>
#include <nodetype.h>
#include <object.h>
#include <regcom.h>

TclObject		cmd_Interp_ ; 
GroupTable		grp_table;
u_int32_t		cur_dn_nid = 0;

extern	RegTable	RegTable_;
extern  typeTable	*typeTable_; 
u_int32_t 		__TotalNodes__ = 0;


cmdTable cmdTable_[] =
{
 {"Create",    &TclObject::cmdCreate      },
 {"EndCreate", &TclObject::cmdEndCreate   },
 {"Set",       &TclObject::cmdSet         },
 {"Module",    &TclObject::cmdModule	  },
 {"Bind",      &TclObject::cmdBind	  },
 {"Define",    &TclObject::cmdDefine	  },
 {"EndDefine", &TclObject::cmdEndDefine	  },
 {"Connect",   &TclObject::cmdConnect	  },
 {"Help",      &TclObject::cmdHelp        },
 {"List",      &TclObject::cmdList        },
 {"Run",       &TclObject::cmdRun	  },
 {"Debug",     &TclObject::cmdDebug	  },
 {"Group",     &TclObject::cmdGroup	  },
 { NULL,       NULL			  }
};  



TclObject::TclObject() {

	tclTable_.slh_first = NULL;  
	initTable_ = NULL;
	curNode = curPort = curMode = 0;
	pathstack = ptop = NULL;
}

TclObject::~TclObject() {

}


/*
 * The Command Dispatch function uses the Register Table, RegTable_,
 * to search the component name and dispatch command information
 * to corresponding object.
 */
int 
TclObject::Command_Dispatch(ClientData client_data,
	Tcl_Interp *interp,
	int argc, char *argv[])
{
	cmdTable		*ptr;


	for(ptr = cmdTable_; ptr->cmd_name_;  ptr++) {
		if (!strcmp(argv[0], ptr->cmd_name_)) {
			/* if the command found, then call
			 * out to the relative member function
			 */
			if ((this->*(ptr->method_))(argc, argv) < 0)
				return(TCL_ERROR); 
		}
	}  
	return(TCL_OK);
}

int TclObject::cmdGroup(int argc, char *argv[]) {

    printf("TclObject::cmdGroup(): argc = %d print argv now ... \n", argc);

    for (int i=0 ; i<argc ; ++i ) {

        printf("argv[%d]: %s  \n" , i , argv[i] );

    }    

    printf("---- end printing argv----- \n");


    /* command format checking */
    if ( strcmp(argv[1], "SN" ) != 0 ) {

        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[1] is not \"SN\" \n");
        exit(1);

    } 

    if ( strcmp(argv[1], "SN" ) != 0 ) {

        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[1] is not \"SN\" \n");
        exit(1);

    }

    if ( strcmp(argv[4], "as" ) != 0 ) {

        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[4] is not \"as\" \n");
        exit(1);

    }

    if ( strcmp(argv[6], "with" ) != 0 ) {

        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[6] is not \"with\" \n");
        exit(1);

    }

    if ( strcmp(argv[7], "name" ) != 0 ) {

        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[7] is not \"name\" \n");
        exit(1);

    }

    if ( strcmp(argv[8], "=" ) != 0 ) {

        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[7] is not \"=\" \n");
        exit(1);

    }

    if ( strcmp(argv[12], "DN" ) != 0 ) {
        printf("TclObject::cmdGroup(): Group command format is incorrect. Argv[3] is not \"DN\" \n");
        exit(1);        
    }

    char* module_type_name = argv[2];

    u_int32_t snid = strtoul( argv[3] , NULL ,10);

    char* node_type_str = argv[5];

    char* node_name = argv[9];

    u_int32_t dn_num = strtoul( argv[11] , NULL ,10);


    /* Get the identifier for the node type */
    curType =  typeTable_->toType(node_type_str);

    /* Adjust the node type identifier due to the consideration of internal design.*/
    if (curType == 0) {
        printf("No %s type defined\n", node_type_str);
        return(-1);
    }
    if (curType == typeTable_->toType("EXTHOST")) {
        curType =  typeTable_->toType("HOST");
    }
    if (curType == typeTable_->toType("EXTROUTER")) {
        curType =  typeTable_->toType("HOST");
    }
    if (curType == typeTable_->toType("VIRROUTER")) {
	curType =  typeTable_->toType("HOST");
    }
    if (curType == typeTable_->toType("WAN")) {
        curType =  typeTable_->toType("SWITCH");
    }

    GT_Entry* tmp_gte_p = grp_table.get_entry(snid , SEARCH_BY_SNID );

    if (tmp_gte_p) {

        printf("TclObject::cmdGroup(): duplicated supernode's NID = %d \n", snid);
        exit(1);


    }

    tmp_gte_p = grp_table.get_entry(snid , SEARCH_BY_DNID );

    if (tmp_gte_p) {

        printf("TclObject::cmdGroup(): Tree-like supernode's configuration is not allowed. snid = %d \n", snid);
        exit(1);


    }

    for ( unsigned int i=0 ; i<dn_num ; ++i) {

        u_int32_t dnid = strtoul( argv[13+i] , NULL ,10);

        tmp_gte_p = grp_table.get_entry(dnid , SEARCH_BY_DNID );

        if (tmp_gte_p) {

            printf("TclObject::cmdGroup(): Device Node [%d] is assigned to multiple supernodes \n",dnid);
            exit(1);


        }

    }


    /* Create a new node module for the supernode */
    //obj = newObject(char* module, u_int32_t type, u_int32_t id, char *name, struct plist* pl);

    /* pathstack is supposed to be NULL since we don't define any ports here.*/
    NslObject* sn_obj = newObject(module_type_name, curType, snid , node_name, pathstack); 
    if (!sn_obj) return(-1);

    __TotalNodes__++;

    grp_table.add( snid , sn_obj, dn_num , &argv[13] );

    return TCL_OK;
}

int 
TclObject::cmdRun(int argc, char *argv[]) {

	extern int Start_Simulation	__P((int, char **));
	extern NslObject                **nodelist;
	const char			*argvs[] = { "MobilityEvent", 0 };  
	u_int32_t		i;


	/* for debug message */
        //show_table();

	/* call last node's MobilityEvent() method to
	 * generate events to update node's location
	 */
	for(i = 1; i <= MAX_NUM_NODE; i++) {
		if (nodelist[i] != NULL)
			nodelist[i]->init();
	}

	for(i = 1; i <= MAX_NUM_NODE; i++) {
		if (nodelist[i] != NULL) {
                        nodelist[i]->command(1, argvs);
                        break;
                }
	}

	return	Start_Simulation(argc, argv);
}


int 
TclObject::cmdCreate(int argc, char *argv[]) {

	NslObject		*obj;

	/*
	 * Create Syntax:
	 *
	 *	Create Node ID as TYPE with name = node_name
	 */
	if ( (argc!=9)||strcmp(argv[3], "as")||strcmp(argv[5], "with")||
	     strcmp(argv[6], "name")|| strcmp(argv[7], "=") )
	{
	     printf("Usage: Create module NodeID as TYPE with name = node_name\n\n");
	     return(-1);
	} 

	/* node id = 0 is not allowed */
	if (atoi(argv[2]) == 0) {
		printf("zero Node ID is not allowed!\n");
		return(-1);
	}

	/* type resolution */
	curType =  typeTable_->toType(argv[4]);
	if (curType == 0) {
		printf("No %s type defined\n", argv[4]);
		return(-1);
	}
        if (curType == typeTable_->toType("EXTHOST")) {
          // Internally an emulation external host functions exactly
          // the same as a host. This is why we internally create and use
          // a host for an external host, which does not exist in the
          // simulation engine.
	  curType =  typeTable_->toType("HOST");
        }
        if (curType == typeTable_->toType("EXTROUTER")) {
          // Internally an emulation external router functions exactly
          // the same as a host. This is why we internally create and use
          // a host for an external host, which does not exist in the
          // simulation engine.
	  curType =  typeTable_->toType("HOST");
        }
        if (curType == typeTable_->toType("VIRROUTER")) {
          // Internally an emulation external router functions exactly
          // the same as a host. This is why we internally create and use
          // a host for an external host, which does not exist in the
          // simulation engine.
          curType =  typeTable_->toType("HOST");
        }

        if (curType == typeTable_->toType("WAN")) {
          // Internally a WAN packet delay/loss/reordering box functions 
          // exactly the same as a two-port switch. This is why we internally 
          // create and use a switch for a WAN box, which does not exist in the
          // simulation engine.
	  curType =  typeTable_->toType("SWITCH");
        }


	/* 
	 * Create a new instance ,
	 * the port ID of Module always be zero.
	 */
	curNode = atoi(argv[2]);
	curPort = 0;

	/* added by jclin on 01/03/2006: 
	 * if the CREATE block is a device node belonging to a supernode,
	 * the node structure instance should not be allocated.
	 */

	GT_Entry* tmp_gte_p = grp_table.get_entry(curNode, SEARCH_BY_DNID);

	if ( tmp_gte_p ) {

		cur_dn_nid = curNode;
		curNode    = tmp_gte_p->snid;            
		sptr_      = tmp_gte_p->sn_obj;

	}
	else {

		obj = newObject(argv[1], curType, curNode, argv[8], pathstack); 
		if (!obj) return(-1);
		sptr_ = obj;
		//printf("%s %d %d %d\n", argv[1], curType, curNode, curPort);

		__TotalNodes__++;

	}

	return(1);    
}


int 
TclObject::cmdEndCreate(int argc, char *argv[]) {

	struct plist* tpl;

	curPort = 0;
	curNode = 0;
	curType = 0;
	curMode = 0;
        cur_dn_nid = 0;
	sptr_   = 0; 
	ptop = NULL;

	tpl = pathstack;
	while(pathstack != NULL){
		if(!pathstack->next)
			free(tpl);
		else{
			tpl = tpl->next;
			free(pathstack);
			pathstack = tpl;
		}
	}
	return(1); 
}




int 
TclObject::cmdHelp(int argc, char *argv[]) {

	return(1);
}


int 
TclObject::cmdList(int argc, char *argv[]) {

	return(1);
}


int 
TclObject::cmdSet(int argc, char *argv[]) {

	NslObject		*obj;
	int			i;
	char			flag; /* for "." searching */   
	char			cmd1[100], cmd2[100] ;
 	const char		*margv[] = {"Set", NULL, "=", NULL, NULL};  

	if ((argc != 4)||strcmp(argv[2], "=")) {
                printf("Syntax: Set variable = value\n\n");
                return(-1);
        }

	/* for binded global variables */
	if (curNode == 0) {
		if(setVarValue((NslObject *)0, argv[1], argv[3]) < 0) {
			printf("No %s this variable!\n\n", argv[1]);
			return(-1);
		}
		return(1);
	} 

	/* search for "." */
	for(i=0, flag=-1 ; argv[1][i]!='\0'; i++) {
		if(argv[1][i] == '.') {
			flag = i;
			break;
		}       
	}           

	switch(flag) {

		case -1:/* in this level */
			/* we should search Binding Table to find
			 * the corresponding variable we want to set.
			 * if return -1, means that no desired variable
			 * found and we should pass this set command 
			 * to command() method.
			 */
			if(setVarValue((NslObject *)0, argv[1], argv[3]) < 0) 
				printf("%s not found.\n\n", argv[1]);
			return(-1);

		case 0: /* error syntax */
			printf("syntax error!\n\n");
			return(-1);  

		default:/* next level */
			memcpy(cmd1, argv[1], flag);
			cmd1[(int)flag] = '\0';   
			strcpy(cmd2, argv[1]+flag+1);
	
			/*
			 * if began 'guitag_' of cmd2, then skip it. this tag
			 * just only for gui
			 */
			if (!strncmp(cmd2, "guitag_", 7))
				return (1);

			margv[1] = cmd2;  
			margv[3] = argv[3];
			/* find instance */
			obj = table_lookup(curNode, cmd1);
			if (!obj) {
				printf("%s not found.\n\n", cmd1);
				return(-1);
			}

			if(setVarValue(obj, margv[1], margv[3]) < 0) 
				obj->command(4, margv);
			return(1);   
	}   
}


int 
TclObject::cmdModule(int argc, char *argv[]) {

	NslObject		*obj;
	const char		*node_cmd[] = 
			 	{ "Construct", NULL, NULL, NULL };

	/* syntax check */
	if ((argc != 4)) {
		printf("Syntax: Module module : name\n\n");
		return(-1);
	}

  	/* Create a new module Instance */

        /*
	 * C.C.Lin: I decided not to change the names of supernode's modules in
	 * order to prevent the operations related to modules' names from running
	 * incorrectly. The module name translation is now performed by the
	 * grp_table->translate_sn_module_name_to_dn_module_name() function.
         */ 

	obj = newObject(argv[1], curType, curNode, argv[3], pathstack); 

	/* 
	 * Passing Module-Instance to its Node Module.
	 * The Node module will construct a node topology
	 * by using these information.
	 */
	if (obj != NULL) {
		assert(sptr_); /* should not happen */
		node_cmd[1] = argv[1];
		node_cmd[2] = argv[3];
		sptr_->command(3, node_cmd);
	}
	return(0);
}


int 
TclObject::cmdBind(int argc, char *argv[]) {

	NslObject		*obj1, *obj2;
	char			cmd[51], cmd1[51];
	const char		*margv1[] = {"Set", "sendtarget", "=", NULL, NULL};
	const char		*margv2[] = {"Set", "recvtarget", "=", NULL, NULL};
	const char		*varname = NULL;
	char			*ptr = NULL;

	/*
	 * If curNode == 0 means that now the parsing is 
	 * not in node-configuration section. 
	 */
	if (curNode == 0) {
		printf("Can not use Bind command!\n\n");
		return(-1);  
	}

	if (argc != 3) {
		printf("Syntax: Bind module_name module_name\n\n");
		return(-1);
	}

	/*
	 * Bind command:
	 *
	 *	- Bind module1 module2
	 *	- equivalent ==>
	 *		Set module1.sendtarget = module2
	 *		Set module2.recvtarget = module1 
	 *
	 *	- Bind module1.port module2
	 *	- equivalent ==>
	 *		Set module1.port = module2
	 *		Set module2.recvtarget = module1
	 */
	strncpy(cmd1, argv[1], 50);
	varname = margv1[1];
	if ((ptr = strchr(argv[1], '.'))) {
		int len = ptr - argv[1];
		strncpy(cmd1, argv[1], len);
		cmd1[len] = '\0';
		margv1[1] = ptr + 1;
	}

	obj1 = table_lookup(curNode, cmd1);
	if (!obj1) {
		printf("%s not found.\n\n", cmd1);
		return(-1);
	}  

	sprintf(cmd, "%d.%s", curNode, argv[2]);
	if (setVarValue(obj1, margv1[1], cmd) < 0) {
		margv1[3] = argv[2];  
		obj1->command(4, margv1);
		margv1[1] = varname;
	}

	obj2 = table_lookup(curNode, argv[2]);
	if (!obj2) {
		printf("%s not found.\n\n", argv[2]);
		return(-1);
	}   

	sprintf(cmd, "%d.%s", curNode, cmd1);
	if (setVarValue(obj2, margv2[1], cmd) < 0) {
		margv2[3] = cmd1;
		obj2->command(4, margv2);
	}

	return(1);    
}  


int 
TclObject::cmdConnect(int argc, char *argv[]) {

	int			i, flag;
	NslObject		*obj; 
	char			cmd1[100], cmd2[100]; 
	const char		*Usage = 
	"Usage: Connect [WIRELESS/WIRE] module1 module2 ..\n"
	"       Connect [SAT] module1 module2\n\n";

	if( !strcmp(argv[1], "WIRE") ) {
		if(argc < 4) {
			printf("%s", Usage);
			return(-1);
		}
	}
	else if( !strcmp(argv[1], "WIRELESS") ) {
		if(argc < 3) {
			printf("%s", Usage);
			return(-1);
		}
	} 
	else if( !strcmp(argv[1], "SAT") ) {
		if(argc != 4) {
			printf("%s", Usage);
			return(-1);
		}
	}

	/* search for "." */
 	for(i=0, flag=0 ; argv[2][i]!='\0'; i++) {
 		if(argv[2][i] == '.') {
 			flag = i;
 			break;
 		}
	}

	switch(flag) {
		case 0: 
			printf("%s", Usage);
			return(-1);
		default:
			strncpy(cmd1, argv[2], flag);
			cmd1[flag] = '\0';
            		strcpy(cmd2, argv[2]+flag+1);
			obj = table_lookup(atoi(cmd1), cmd2);
			if(!obj) {
				printf("%s not found.\n\n", cmd2);
				return(-1);
			}
			return(obj->command(argc, const_cast<const char **>(argv))); 
	}
}

/* modified by jackyu */
int 
TclObject::cmdDefine(int argc, char *argv[]) {

	if ((argc!=3) || strcmp(argv[1], "port")) {
		printf("Usage: Define port ##%s %s %s\n", argv[0], argv[1], argv[2]);
		return(-1);
	}

	curMode++;	

        GT_Entry* tmp_gte_p = grp_table.get_entry(cur_dn_nid, SEARCH_BY_DNID);

	if ( tmp_gte_p ) {
		curNode = tmp_gte_p->snid;

		DN_Entry* dne_p = tmp_gte_p->get_dn_entry(cur_dn_nid);

		if (!dne_p) {
			printf("TclObject::cmdDefine(): the dn_entry is found.\n");
			exit(1);
		}

		u_int32_t ori_portid = (u_int32_t)atoi(argv[2]);
		++tmp_gte_p->cur_portid;            
		curPort = tmp_gte_p->cur_portid;

		dne_p->add_pi_entry(ori_portid, curPort);

		sptr_   = tmp_gte_p->sn_obj;
	}
	else {
		curPort = (u_int32_t)atoi(argv[2]);
	}

	if(pathstack == NULL){
		pathstack = (plist *)malloc(sizeof(struct plist));
		pathstack->next = NULL;
		ptop = pathstack;
		pathstack->pid = curPort;
	}
	else{
		ptop->next = (plist *)malloc(sizeof(struct plist));
		ptop = ptop->next;
		ptop->next = NULL;
		ptop->pid = curPort;
	}
	return(1);
}


int 
TclObject::cmdEndDefine(int argc, char *argv[]) {

	struct plist* tmp = pathstack;

	assert(pathstack&&ptop);

	//add by H&Z
	//sptr_->Decrease_PortLevel();

	curMode--;
	if(ptop == pathstack){
		curPort = 0;
		ptop = NULL;
		free(pathstack);
		pathstack = NULL;
	}
	else{
		while(tmp->next != ptop)
			tmp = tmp->next;
		free(ptop);
		ptop = tmp;
		ptop->next = NULL;
		curPort = ptop->pid;
	}
	return(1);
}


int 
TclObject::cmdDebug(int argc, char *argv[]) {

	NslObject		*obj;


	if (argc != 3) {
		printf("Syntax: Debug Node_ID Module_name\n\n");
		return(-1);
	}

       bool uncanonical_module_name_flag = 0;

     /* The "." sign is not allowed as part of a module name. */
     if ( strstr(argv[2],".") ) {
     //printf("argv3=%s\n", argv[3]);
          uncanonical_module_name_flag = 1;
     }
    /* The RegTable->lookup_Instance() with two parameter version
     * should use the canonical name of a module. If an illegal
     * module name is found, we should return immediately.
     * A canonical module name should start with
     * a prefix "NODE." As such, for any module names
     * without this prefix, this function simply returns
     * the original string as its output because the
     * name translation process may fail with incorrect
     * input names.
     */
     if (strncasecmp("NODE", argv[2], 4))
         uncanonical_module_name_flag = 1;

    /* A canonical module name should start with
     * a prefix "NODE." As such, for any module names
     * without this prefix, this function simply returns
     * the original string as its output because the
     * name translation process may fail with incorrect
     * input names.
     */

    /* Connectivity */
    if (!uncanonical_module_name_flag) {

        obj = table_lookup(atoi(argv[1]), argv[2]);  
        if (!obj) {
            printf("TclObject::cmdDebug(): Warning: Node[%s] does not have the instance of the %s module.\n\n",
                    argv[1], argv[2]);
            return(-1);
        }
    }
	obj->Debugger();
	return(1);    
}


NslObject * 
TclObject::table_lookup(u_int32_t id, const char *name) {

	struct tclTable			*tTbl;
	struct nslInst			*inst;

    assert(id);
    assert(name);

	GT_Entry* tmp_gte_p = grp_table.get_entry(id, SEARCH_BY_DNID);

	if ( tmp_gte_p ) 
		id = tmp_gte_p->snid;            

    u_int32_t reserved_dn_module_name_len = strlen(name)+100;
    char* dn_module_name = new char[reserved_dn_module_name_len];
    memset(dn_module_name, 0, reserved_dn_module_name_len);
    grp_table.translate_sn_module_name_to_dn_module_name(name, dn_module_name,reserved_dn_module_name_len);

	SLIST_FOREACH(tTbl, &(tclTable_), nextTbl_) {
	    if (tTbl->nodeID_ == id) {
		    SLIST_FOREACH(inst, &(tTbl->hdInst_), nextInst_) {
			    if (!strcmp(inst->obj_->get_name(), dn_module_name)) {
                      delete dn_module_name;
                      dn_module_name = 0;
                      return(inst->obj_);
                }
		    }
	    }
	}
    delete dn_module_name;
    dn_module_name = 0;
    //assert(0); /* for strict checking mode */
    return(NULL); /* for loose checking mode */
}


NslObject * 
TclObject::newObject(char *module, u_int32_t type, u_int32_t id, const char *name, struct plist* pl)
{

	NslObject			*obj;
	struct tclTable		 	*tTbl, *tb;
	struct nslInst			*inst, *is;

	obj = RegTable_.newObject(module, id, pl, type, name);

	/* It is very ugly here. if obj is NULL then it means
  	 * that the component doesn't exist; if obj is 1 then
  	 * it means that the instance already exists.
 	 */
	if (obj == NULL) {
		printf("No %s this Module\n", module);
		return(0);
	}
	if (obj == (NslObject *)1) {
		printf("Instance %s of node %d already exists!\n", name, id);
		return(0);
	}

	/* if create successfully, we should add this instance
  	 * into tclTable.
         */
	SLIST_FOREACH(tTbl, &(tclTable_), nextTbl_) {
	    if (tTbl->nodeID_ == obj->get_nid()) {
		/* exist such node, we should
		 * try to check duplicate node
		 */
		SLIST_FOREACH(inst, &(tTbl->hdInst_), nextInst_) {
			if (!strcmp(inst->obj_->get_name(), obj->get_name()))
				/* duplicate Instance is found!
				 * It is impossible to occur 
				 * because in module creating-time,
				 * this phenomenon will be checked.
				 */
				 assert(0);
		}
		/* Otherwise, we found node but
		 * no duplicate Instance of node
		 * be found.
		 */
		break; 
	    }
	}

	/* No node we want is found, so
	 * add new one.
	 */
	if (tTbl == NULL) {
		tTbl = (struct tclTable *)
			malloc(sizeof(struct tclTable));
		assert(tTbl);
		tTbl->nodeID_   = id;
		tTbl->nodeType_ = type;
		tTbl->hdInst_.slh_first = 0;
		tTbl->nextTbl_.sle_next = 0;
		SLIST_AINSERT(tb, &tclTable_, tTbl, nextTbl_, nodeID_);
	}

	/* Add a new Instance */
        inst = (struct nslInst *)malloc(sizeof(struct nslInst));
        assert(inst);
	inst->obj_    = obj;
	inst->tclTbl_ = tTbl;
	inst->nextInst_.sle_next = 0;
	SLIST_INSERT_TAIL(is, &(tTbl->hdInst_), inst, nextInst_);

	return(obj);  
}

int TclObject::Debugger()
{
	printf("DEBUG: TclObject\n\n");  
	return(1);
}

PortInfoEntry::PortInfoEntry()
{

	ori_pid = 0;
	new_pid = 0;
	next_pie_p.sle_next = NULL;

}

DeviceNodeEntry::DeviceNodeEntry()
{

	nid = 0;
	cur_p = NULL;
	pi_list.slh_first = NULL;
	next_dne_p.sle_next = NULL;

}

int DeviceNodeEntry::add_pi_entry(u_int32_t ori_pid, u_int32_t new_pid)
{

	PI_Entry* pie_p = NULL;

	pie_p = get_pi_entry_by_ori_pid(ori_pid);

	if (pie_p) {

		printf("DeviceNodeEntry::add_pi_entry(): duplicated entry is found. Old pid is found.\n");
		exit(1);

	}

	pie_p = get_pi_entry_by_new_pid(new_pid);

	if (pie_p) {

		printf("DeviceNodeEntry::add_pi_entry(): duplicated entry is found. New pid is found.\n");
		exit(1);

	}

	pie_p = new PI_Entry;
	pie_p->ori_pid    = ori_pid;
	pie_p->new_pid    = new_pid;
	pie_p->next_pie_p.sle_next = NULL;

	PI_Entry* tmp_pie_p = NULL;
	SLIST_INSERT_TAIL(tmp_pie_p , &pi_list, pie_p , next_pie_p)

	return 1;
}

PI_Entry* DeviceNodeEntry::get_pi_entry_by_ori_pid(u_int32_t ori_pid)
{

	PI_Entry* pie_p = NULL;

	SLIST_FOREACH( pie_p , &(this->pi_list), next_pie_p) {

		if ( pie_p->ori_pid == ori_pid )
			return pie_p;

        }

	return NULL;

}

PI_Entry* DeviceNodeEntry::get_pi_entry_by_new_pid(u_int32_t new_pid)
{

	PI_Entry* pie_p = NULL;

	SLIST_FOREACH( pie_p , &(this->pi_list), next_pie_p) {

		if ( pie_p->new_pid == new_pid )
			return pie_p;
	}

	return NULL;
}

GroupTableEntry::GroupTableEntry()
{

	snid                = 0;    
	sn_obj              = NULL;
	cur_portid          = 0;
	cur_p               = NULL;
	dn_list.slh_first   = NULL;
	next_gte_p.sle_next = NULL;

} 


DN_Entry* GroupTableEntry::get_dn_entry(u_int32_t dn_nid) {

    DN_Entry* dne_p = NULL;

	SLIST_FOREACH( dne_p , &(this->dn_list), next_dne_p) {
		if ( dne_p->nid == dn_nid )
			return dne_p;

	}

	return NULL;

}

DN_Entry* GroupTableEntry::get_dn_entry_by_its_sn_linkid(u_int32_t sn_linkid) {

    DN_Entry* dne_p = NULL;

    u_int32_t iteration_cnt = 0;
    SLIST_FOREACH( dne_p , &(this->dn_list), next_dne_p) {

        assert(dne_p);
        ++iteration_cnt;
        if (iteration_cnt >= sn_linkid)
            return dne_p;

    }
    return NULL;
}

GroupTable::GroupTable()
{

	entry_num      = 0;
	head.slh_first = NULL;
	cur_p = NULL;

}

int GroupTable::add(u_int32_t snid , NslObject* sn_obj , u_int32_t dn_num , char** dn_ptr )
{

	if (!dn_ptr) {

		printf("GroupTable::add(): dn_ptr is null.\n");
		exit(1);

	}

	GT_Entry* new_gt_entry = new GT_Entry;

	new_gt_entry->snid   = snid;
	new_gt_entry->sn_obj = sn_obj;

	for ( u_int32_t i=0 ; i<dn_num ;++i) {

		u_int32_t dnid = strtoul(dn_ptr[i],NULL,10);

		DN_Entry* new_dn_entry = new DN_Entry;

		new_dn_entry->nid = dnid;			  

		if ( !(new_gt_entry->dn_list.slh_first) ) {
			SLIST_INSERT_HEAD( &new_gt_entry->dn_list , new_dn_entry , next_dne_p );
			new_gt_entry->cur_p = new_dn_entry;
		}
		else {
			SLIST_INSERT_AFTER( new_gt_entry->cur_p, new_dn_entry , next_dne_p);
			new_gt_entry->cur_p = new_dn_entry;
		}	

	}

	if ( !(head.slh_first) ) {
		SLIST_INSERT_HEAD( &head , new_gt_entry , next_gte_p );
		this->cur_p = new_gt_entry;
	}
	else {
		SLIST_INSERT_AFTER( cur_p, new_gt_entry , next_gte_p);
		this->cur_p = new_gt_entry;
	}

	++entry_num;
	return 1;

}

GT_Entry*  GroupTable::get_entry(u_int32_t nid , int mode)
{

	GT_Entry* gte_p;
	SLIST_FOREACH( gte_p , &head, next_gte_p) {

		if ( mode == SEARCH_BY_SNID ) {

			if ( gte_p->snid == nid )
				return gte_p;

		}
		else {

			DN_Entry* dne_p;
			SLIST_FOREACH( dne_p , &gte_p->dn_list, next_dne_p) {

				if ( dne_p->nid == nid )
					return gte_p;

			}

		}

	}

	return NULL;
}

PI_Entry* GroupTable::get_port(u_int32_t nid, u_int32_t old_pid)
{

	GT_Entry* gte_p = get_entry(nid, SEARCH_BY_DNID);

	if (!gte_p) {

		printf("GroupTable::port_mapping(): cannot find the correspondent gt_entry.\n");
		exit(1);

	}

	DN_Entry* dne_p = gte_p->get_dn_entry(nid);

	if ( !dne_p ) {

		printf("GroupTable::port_mapping(): cannot find the correspondent dn_entry.\n");
		exit(1);

	} 

	PI_Entry* pie_p = NULL;

	SLIST_FOREACH( pie_p , &dne_p->pi_list, next_pie_p) {

		if ( pie_p->ori_pid == old_pid )
			return pie_p;

	}

	return NULL;
}

/* CCLin:
 * This function is required for a simulation case with the presence of multi-interface nodes,
 * when a module issues an instance-lookup request to the TclObject or the Module Registration 
 * Table.
 */

int GroupTable::translate_sn_module_name_to_dn_module_name(const char* sn_module_name, char* dn_module_name, u_int32_t dn_mname_len) {

    assert(sn_module_name);
    assert(dn_module_name);

    strcpy(dn_module_name, sn_module_name);
    char* module_name_buf = dn_module_name;

    const char* delimiter="_";
    char* token_pool[20];

    for (int i=0; i<20 ;++i)
        token_pool[i] = 0;

     /* Parse the given module name to obtain the information required for
      * name translation. The followings are two module name examples:
      *  module name = Node2_Wphy_LINK_1 
      *  module name = Node2_CM_LINK_1
      *  a weird case "Link Module": Node2_LINK_1
      *  another weird case: Node2_phy_80211a_LINK_1
      */
    bool link_module_found_flag = 0;
    int token_cnt = 0;
    for (int i=0; i<20  ;++i) {

        char* token = strsep(&module_name_buf ,delimiter);
        token_pool[i] = token;

        if (!token) break;

        ++token_cnt;
    }

    bool node_str_found_flag = 0;
    int link_str_index = 0;
    for (int i=0; i<token_cnt ;++i) {

        if (!strncasecmp("NODE", token_pool[i], 4))
            node_str_found_flag = 1;

        if (!strncmp("LINK",token_pool[i],4)) {
            link_str_index = i;
            break;
        }

    }

    /* A canonical module name should start with
     * a prefix "NODE." As such, for any module names
     * without this prefix, this function simply returns
     * the original string as its output because the
     * name translation process may fail with incorrect
     * input names.
     */
    if (!node_str_found_flag) {

        strcpy(dn_module_name, sn_module_name);
        return 1; //a switch node cannot be a supernode.
    }

    /* obtain the node ID indicated by this module name */
    u_int32_t module_nid = 0;
    sscanf(token_pool[0], "Node%u", &module_nid);

    char module_class_name[100];
    char link_str[100];
    u_int32_t link_id = strtol(token_pool[token_cnt-1], 0, 10);
    if (link_id == 0) {

	/* In a simulated node, switch or switch-like modules are 
         * located over protocol ports. Thus, since a multi-interface
         * node is implemented as a layer-3 device, we do not process
         * the name translation for switch-like modules in the current
         * implementation.
         */
        strcpy(dn_module_name, sn_module_name);
        return 1; //a switch node cannot be a supernode.

    }

    memset(module_class_name,0,100);
    memset(link_str,0,100);

    assert(link_str_index);
    assert(link_str_index<20);

    /* process the LINK module case */
    if (link_str_index == 1 ) {
        link_module_found_flag = 1;
        strcpy(module_class_name,token_pool[1]);
        link_id = strtol(token_pool[2], 0, 10);
    }
    else if (link_str_index > 2) {

      /* process the cases where an underscore sign "_" exists
       * in a module name, such the phy_80211a module.
       */
        strcpy(module_class_name,token_pool[1]);
        for (int i=2; i<link_str_index ;++i) {

            strncat(module_class_name, "_", 1);
            strcat(module_class_name, token_pool[i]); 
 
        }
        
        strcpy(link_str, token_pool[link_str_index]);
        link_id = strtol(token_pool[link_str_index+1], 0, 10);

    }
    else {

        /* process normal module name cases */
        strcpy(module_class_name,token_pool[1]);
        strcpy(link_str, token_pool[2]);
        link_id = strtol(token_pool[3], 0, 10);

    }

    GT_Entry* gte_p = get_entry(module_nid, SEARCH_BY_SNID);

	if (!gte_p) {
        /* In this case, the given sn_module_name has already been
         * the original module name that is named for the device node.
         */
        strcpy(dn_module_name, sn_module_name);
        return 1;
    }
    else {

        DN_Entry* dne_p = gte_p->get_dn_entry_by_its_sn_linkid(link_id);
        assert(dne_p);

        u_int32_t dn_nid = dne_p->nid;
        assert(dn_nid);

        PI_Entry* pie_p = dne_p->get_pi_entry_by_new_pid(link_id);
        assert(pie_p);
        u_int32_t dn_port_id = pie_p->ori_pid;
        assert(dn_port_id);

        memset(dn_module_name, 0, dn_mname_len);
        if (!link_module_found_flag)
            sprintf(dn_module_name, "Node%u_%s_%s_%u", dn_nid, module_class_name, link_str, dn_port_id);
        else
            sprintf(dn_module_name, "Node%u_%s_%u", dn_nid, module_class_name, dn_port_id);

        //printf("Translate: %s --> %s \n", sn_module_name, dn_module_name);
        return 1;
    }

    assert(0);
}

/**** for debug ****/

int TclObject::show_table()
{

	struct tclTable* tTbl = NULL;
	printf("\n******* Show TclTable Content ********\n");

	SLIST_FOREACH(tTbl, &(tclTable_), nextTbl_) {

		printf("nodeID: %d nodeType = %d ************** \n", tTbl->nodeID_ , tTbl->nodeType_);
		printf("Its Module List: \n");

		nslInst* inst_p = NULL;
		SLIST_FOREACH(inst_p , &(tTbl->hdInst_) , nextInst_ ) {

			NslObject* obj_p = inst_p->obj_;

			printf(" module name = %s port = %d \n" , obj_p->get_name(), obj_p->get_port());

		}


	}

	printf("\n******* End Show TclTable Content ********\n");

	return 1;
}
