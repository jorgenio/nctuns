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
#include <regcom.h>
#include <object.h>
#include <tclBinder.h>
#include <tclObject.h>

#define  search_by_snid 1
#define  search_by_dnid 0
extern GroupTable grp_table;

/*
 * Global Variable, Register Table
 * 	- RegTable  RegTable_
 */
RegTable		RegTable_;


RegTable::RegTable()
{

	regTable_.slh_first = NULL;  
}

RegTable::~RegTable()
{

	Module_Info             *minfo;
	struct Instance         *inst;

	SLIST_FOREACH(minfo, &regTable_, nextInfo_) 
	{
		SLIST_FOREACH(inst, &(minfo->hdInst_), nextInst_) 
		{
			delete (inst->obj_);
		}
	}

	/* we do not release the regTable_ memeory space
	 * here, becasue this is the only one object in
	 * the simulator and when this method is called,
	 * the whole simulator also stop. So we give this
	 * duty to OS.
	 */
}


/* new type of this function */
NslObject * 
RegTable::newObject(const char *name_, u_int32_t id, struct plist* pl, u_int32_t dtype, const char *objname) 
{
	NslObject		*obj;
 	struct Instance		*inst;
	struct Module_Info	*minfo;

 	/*
	 * Check to see if the module name has been
	 * registered. If no this module be found,
	 * we should return NULL to indicate caller.
	 */
	minfo = get_moduleInfo(name_);
	//SLIST_FOREACH(minfo, &regTable_, nextInfo_) {
	if (minfo) {
		/*
		 * This module has beed registered
		 * and we should try to check duplicate
		 * Module-Instance-Name.
		 */
		SLIST_FOREACH(inst, &(minfo->hdInst_), nextInst_) {
			/*
			 * We use node ID and Instance-Name as a
			 * a unique ID.
			 */
			if ((!strcmp(inst->obj_->get_name(), objname))&&(inst->obj_->get_nid() == id))
				/* 
				 * This Module-Instance already exists,
				 * return 1.
				 */
				 return((NslObject *)1);
		}
		/* Otherwise, no this Module-Instance 
		 * exists, so we should create one 
		 */
		obj = minfo->create_comp(dtype, id, pl, objname);
		assert(obj);

		/* Insert it into register Table */
		inst = (struct Instance *)malloc(sizeof(struct Instance));
	    	assert(inst);
		inst->obj_   = obj;
		inst->mInfo_ = minfo;
		SLIST_INSERT_HEAD(&(minfo->hdInst_), inst, nextInst_);
	    	return(obj);
	}
	//}

	/* otherwise, return NULL to indicate
	 * that there has no such component 
	 */
	return(0); 
}

int
RegTable::Register_Com(const char *name_,
        NslObject *(*maker)(u_int32_t, u_int32_t, struct plist *, const char *))
{
	
        Module_Info             *minfo;
	
        SLIST_FOREACH(minfo, &regTable_, nextInfo_) {
                if (!strcmp(minfo->cname_, name_)) {
                        /*
			 * This Module has aleady beed
			 * registered.
	                 */
	                 return(-1);
	        }
        }

        /* Otherwise, this Module has not been registered.
         * add this Module to regTable
         */
        minfo = (struct  Module_Info *)malloc(sizeof(struct Module_Info));
        assert(minfo);
        /* fill Module_Info data structure */
        minfo->cname_ = name_;
        minfo->create_comp = maker;
        minfo->hdInst_.slh_first = 0;
        SLIST_INSERT_HEAD(&regTable_, minfo, nextInfo_);
        return(1);
}


struct Module_Info *
RegTable::get_moduleInfo(const char *mname)
{

	struct Module_Info		*minfo;

	SLIST_FOREACH(minfo, &regTable_, nextInfo_) {
		if (!strcmp(minfo->cname_, mname))
			return(minfo);
	}
	return(0);
}


struct Instance *
RegTable::get_instanceInfo(const NslObject *obj)
{

	struct Module_Info		*minfo;
	struct Instance			*inst;

	SLIST_FOREACH(minfo, &regTable_, nextInfo_) {
	    SLIST_FOREACH(inst, &(minfo->hdInst_), nextInst_) {
		if (inst->obj_ == obj)
			return(inst);
	    }
	}
	return(NULL);
}



/*
 * Search whole register table to find desired 
 * Instance by NslObject name and node ID.
 *
 * objname (canonical) format: Node[NodeID]_[ModuleName]_LINK_[Port]
 * objname example: Node3_WPHY_LINK_1
 */
NslObject * 
RegTable::lookup_Instance(u_int32_t id, const char *objname)
{

    struct Instance     *inst;
    struct Module_Info  *minfo;


    /* added by c.c. lin:
      * the device node's id has to be replaced with its super node's id.
      */

    assert(id);
    assert(objname);
    GT_Entry* tmp_gte_p = grp_table.get_entry(id, search_by_dnid);

    if ( tmp_gte_p )
        id = tmp_gte_p->snid;

    const char* name = objname;
    u_int32_t reserved_dn_module_name_len = strlen(name)+100;

    char* dn_module_name = new char[reserved_dn_module_name_len];
    memset(dn_module_name, 0, reserved_dn_module_name_len);
    grp_table.translate_sn_module_name_to_dn_module_name(name, dn_module_name,reserved_dn_module_name_len);

    /* In this search function,
     * the canonical name of a module is used as the search index rather than
     * the class name of a module.
     */
    SLIST_FOREACH(minfo, &regTable_, nextInfo_) {
        SLIST_FOREACH(inst, &(minfo->hdInst_), nextInst_) {
            if ((!strcmp(dn_module_name, inst->obj_->get_name()))&&
                (id == inst->obj_->get_nid())) {

                    delete dn_module_name;
                    dn_module_name = 0;
                    return(inst->obj_); //found

            }
        }
    }

    delete dn_module_name;
    dn_module_name = 0;

    return(NULL); 
}


/*
 * Search whole register table to find desired
 * Instance by module name, node ID and port ID.
 */
NslObject *
RegTable::lookup_Instance(u_int32_t NodeID, u_int32_t PortID, const char *ModuleName)
{

        struct Instance         *inst;
        struct Module_Info      *minfo;

    /* added by C.C. Lin:
    * the device node's id has to be replaced with its super node's id.
    */
    assert(NodeID);
    assert(ModuleName);
    GT_Entry* tmp_gte_p = grp_table.get_entry(NodeID, search_by_dnid);

    if ( tmp_gte_p ) {
        PI_Entry* pie_p = grp_table.get_port(NodeID,PortID);
        if (!pie_p) {
            printf("RegTable::lookup_Instance(): port_mapping_info entry is not found. \n");
            exit(1);
        }

        NodeID = tmp_gte_p->snid;

        /* the portID has to be mapped to the one used in the supernode */
        PortID = pie_p->new_pid;
    }

    /* The following codes are removed because in this search function,
     * the class name of a module is used as the search index rather than
     * the canonical name of a module.
     */
    #if 0
    const char* name = ModuleName;
    u_int32_t reserved_dn_module_name_len = strlen(name)+100;
    char* dn_module_name = new char[reserved_dn_module_name_len];
    memset(dn_module_name, 0, reserved_dn_module_name_len);
    grp_table.translate_sn_module_name_to_dn_module_name(name, dn_module_name,reserved_dn_module_name_len);
    #endif
    const char* dn_module_name = ModuleName;

    SLIST_FOREACH(minfo, &regTable_, nextInfo_) {
      if( !strcmp(minfo->cname_, dn_module_name) ) {

          SLIST_FOREACH(inst, &(minfo->hdInst_), nextInst_) {
              if ( PortID == inst->obj_->get_port() &&
                    NodeID == inst->obj_->get_nid() ) {

	          #if 0
                  delete dn_module_name;
                  dn_module_name = 0;
                  #endif
                  return(inst->obj_); //found
      
              }
          }
      }
    }

    #if 0
    delete dn_module_name;
    dn_module_name = 0;
    #endif
    return(NULL);
}
