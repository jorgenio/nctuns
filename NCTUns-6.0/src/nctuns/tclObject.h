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

#ifndef	__NCTUNS_tclObject_h__
#define __NCTUNS_tclObject_h__

#include <tcl.h>
#include <mylist.h>

#define  SEARCH_BY_SNID 1
#define  SEARCH_BY_DNID 0

#define MAX_STACK		10

class NslObject;
class TclBinder;
class TclObject;

/* Added by C.C. Lin on 01/02/2006 .. Happy New Year!! 
 * GroupMappingTable relevant classes.
 */

typedef class PortInfoEntry {
 
    public:

	u_int32_t ori_pid;
	u_int32_t new_pid;

	SLIST_ENTRY(PortInfoEntry) next_pie_p;

	PortInfoEntry();


} PI_Entry; 

typedef class DeviceNodeEntry {

    public:

	u_int32_t nid;
	SLIST_HEAD( , PortInfoEntry ) pi_list;
	PortInfoEntry*                cur_p;

	SLIST_ENTRY(DeviceNodeEntry)  next_dne_p;


	DeviceNodeEntry();
	int add_pi_entry(u_int32_t ori_pid, u_int32_t new_pid);
	PI_Entry* get_pi_entry_by_ori_pid(u_int32_t ori_pid);
	PI_Entry* get_pi_entry_by_new_pid(u_int32_t new_pid);

} DN_Entry;

typedef class GroupTableEntry {

    public:
   
	u_int32_t  snid;
	NslObject* sn_obj;
	u_int32_t  cur_portid;

	SLIST_HEAD( , DeviceNodeEntry ) dn_list;
	DeviceNodeEntry*                cur_p;

	SLIST_ENTRY(GroupTableEntry) next_gte_p;    

	    GroupTableEntry();
        DN_Entry* get_dn_entry(u_int32_t dn_nid);
        DN_Entry* get_dn_entry_by_its_sn_linkid(u_int32_t sn_linkid);

} GT_Entry;
  
class GroupTable {

    private:
 
	u_int32_t   entry_num;
	SLIST_HEAD( , GroupTableEntry) head;
	GroupTableEntry*               cur_p;
    
    public:
    GroupTable();
    int        add(u_int32_t snid , NslObject* sn_obj, u_int32_t dn_num , char** dn_ptr );
    GT_Entry*  get_entry(u_int32_t nid , int mode); /* mode == 1 , nid stands for snid. Otherwise, nid stands for dnid */
    PI_Entry*  get_port(u_int32_t nid, u_int32_t old_pid);
    int        translate_sn_module_name_to_dn_module_name(const char* sn_module_name, char* dn_module_name, u_int32_t dn_mname_len);
};



/*======================================================================
   Define Command Structure
  ======================================================================*/
/* specify our tcl command */
typedef struct cmdTable {
	const char		*cmd_name_; 
	int			(TclObject::*method_)(int ,char **); 
} cmdTable;   
  

struct tclTable;
struct nslInst {
	NslObject		*obj_; 
	struct tclTable		*tclTbl_;

	SLIST_ENTRY(nslInst)	nextInst_;
}; 
                               
struct tclTable {
	u_int32_t		nodeID_; 
	u_int32_t		nodeType_;

	SLIST_HEAD(, nslInst)	hdInst_;
	SLIST_ENTRY(tclTable)	nextTbl_;
}; 


/* 
 * initial Table is used to init user-specify functions.
 * when the simulatior start, the initial-table will be 
 * used to initial specified functions/objects.
 */
typedef struct initTable {
	NslObject		*obj_;
	int			(NslObject::*method_)();    
	int			(*func_)();
	struct initTable	*next_;  
} initTable;
            
                                                                                                                                                     
/*======================================================================
   Define TclObject Class
  ======================================================================*/                                                                                                                                              
class TclObject : public TclBinder {

 private:

	u_int32_t		curNode;
	u_int32_t		curPort;
	u_int32_t		curType;
	u_int32_t		curMode;
 	SLIST_HEAD(, tclTable)	tclTable_;
 	/* add by jackyu */
 	struct plist		*pathstack;
 	struct plist 		*ptop;
 	
 	NslObject		*sptr_;
 public:
 
	initTable		*initTable_; 
	 
 	TclObject(); 
 	~TclObject();  

	int			Command_Dispatch(ClientData,
				Tcl_Interp *, int, char **);
	int			cmdRun(int argc, char *argv[]);
	int 			cmdCreate(int argc, char *argv[]);
	int			cmdEndCreate(int argc, char *argv[]); 
	int			cmdHelp(int argc, char *argv[]);
	int			cmdList(int argc, char *argv[]);
	int 			cmdSet(int argc, char *argv[]);    
	int			cmdModule(int argc, char *argv[]); 
	int			cmdBind(int argc, char *argv[]); 
	int			cmdDefine(int argc, char *argv[]);
	int			cmdEndDefine(int argc, char *argv[]);  
	int			cmdDebug(int argc, char *argv[]); 
	int			cmdConnect(int argc, char *argv[]); 
	int			cmdGroup(int argc, char *argv[]);
	NslObject		*table_lookup(u_int32_t id, const char *name); 
	NslObject		*newObject(char *, u_int32_t, u_int32_t, const char *, struct plist *); 
	int			show_table();
	int			Debugger(); 
}; 



#endif /* __NCTUNS_tclObject_h__ */
