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

#ifndef	__NCTUNS_regcom_h__
#define __NCTUNS_regcom_h__

#include <sys/types.h>
#include <mylist.h>

class NslObject;

/*=======================================================================
   Define Component Register Table
  =======================================================================*/
struct Instance {
	NslObject		*obj_;		/* pointer to Instance */
	struct Module_Info	*mInfo_;	/* pointer to module info */
	SLIST_ENTRY(Instance)	nextInst_;	/* next Instance */
};

struct Module_Info {
	const char		*cname_;
	NslObject		*(*create_comp)(u_int32_t, u_int32_t, struct plist*, const char *);
	SLIST_HEAD(, Instance)	hdInst_;	/* pointer to Instance */
	SLIST_ENTRY(Module_Info) nextInfo_;
};


/*=======================================================================
   Define RegTable Class
  =======================================================================*/                                                                                                                                              
class RegTable {
 private:

 	SLIST_HEAD(, Module_Info) regTable_;
 public:  
	
	RegTable();
	~RegTable();
	
	//new added
	NslObject	*newObject(const char *, u_int32_t, struct plist*,
			u_int32_t, const char *);
	
	//new added
	int             Register_Com(const char *, NslObject *(*maker)(u_int32_t,
                        u_int32_t, struct plist *, const char *));
	
	NslObject       	*lookup_Instance(u_int32_t, const char *);
	NslObject               *lookup_Instance(u_int32_t, u_int32_t, const char *);
	struct Module_Info 	*get_moduleInfo(const char *mname);
	struct Instance		*get_instanceInfo(const NslObject *obj);

};


#endif	/* __NCTUNS_regcom_h__ */
