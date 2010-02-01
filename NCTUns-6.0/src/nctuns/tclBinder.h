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

#ifndef	__NCTUNS_tclBinder_h__
#define __NCTUNS_tclBinder_h__

#include <sys/types.h>

class TclBinder;
class NslObject;
 
typedef struct bindLisst {
	char			*vname_; 
	void			*var_;
	int			(TclBinder::*func_)(void *, const char *); 
} bindList;


#define MAX_BIND_VARS		50
  
typedef struct bindTable {
	struct bindTable	*next_;
	NslObject		*obj_;
	bindList		varlist_[MAX_BIND_VARS];
	int			blidx_; 	/* index of bind list */
} bindTable; 


class TclBinder {

 private: 
 
 	bindTable	*bindTable_; 
 
 	int		b_intfun(void *var, const char *value);
 	int		b_doublefun(void *var, const char *value);
 	int		b_boolfun(void *var, const char *value);
 	int		b_ipfun(void *var, const char *value);      
	int		b_strfun(void *var, const char *value); 
	int  		b_macfun(void *var, const char *value);
	int		b_floatfun(void *var, const char *value); 
 	
 	bindTable	*table_lookup(NslObject *obj); 
 	bindTable	*table_add(NslObject *obj); 
 	bindList	*list_add(NslObject *obj, const char *vname); 
 public:
 
 	TclBinder();
 	~TclBinder();  
 	
 	int		Bind_(NslObject *obj, const char *vname, int *var);
 	int		Bind_(NslObject *obj, const char *vname, double *var);
	int		Bind_(NslObject *obj, const char *vname, float *var); 
 	int		Bind_bool(NslObject *obj, const char *vname, u_char *var);
 	int		Bind_ip(NslObject *obj, const char *vname, u_long *var);      
	int		Bind_(NslObject *obj, const char *vname, char **var); 
	int     	Bind_mac(NslObject *obj, const char *vname, u_char *var);

	int		setVarValue(NslObject *obj, const char *vname, const char *value); 
};
 


#endif /* __NCTUNS_tclBinder_h__ */
