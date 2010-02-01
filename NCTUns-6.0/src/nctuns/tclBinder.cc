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
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <object.h>
#include <tclBinder.h>
#include <nctuns_api.h>


TclBinder::TclBinder() {

	bindTable_ = 0;  
}

TclBinder::~TclBinder() {

}


bindTable * TclBinder::table_lookup(NslObject *obj) {

	bindTable		*bt;

	
	for(bt=bindTable_; bt; bt=bt->next_) {
		if(bt->obj_ == obj)
			return(bt);   
	}
	return(0);
}

 
bindTable * TclBinder::table_add(NslObject *obj) {

	bindTable		*bt;

 
	/* if not found, add one */
	bt = new bindTable;
	assert(bt);
	
	bt->obj_ = obj;
	bt->blidx_ = 0;    
	bt->next_ = bindTable_;
	bindTable_ = bt;
	
	return(bt); 
}



int TclBinder::b_intfun(void *var, const char *value) {

	*((int *)var) = atoi(value);
	return(1);   
}


int TclBinder::b_doublefun(void *var, const char *value ) {

	double			p;
 
	if (sscanf(value, "%lf", &p)!=1) {
		printf("Error: can not set this variable!\n\n");  
		return(-1);
	} else  *((double *)var) = p; 

	return(1); 
}


int TclBinder::b_boolfun(void *var, const char *value) {
    
	if (!strcasecmp(value, "TRUE")) {
		*((u_char *)var) = 1;
	}
	else if (!strcasecmp(value, "FALSE")) {
		*((u_char *)var) = 0;
	}
	else {
		printf("Error!! should be TRUE or FALSE!\n\n"); 
		return (-1);    
	}
	return (1);   
}


int TclBinder::b_ipfun(void *var,  const char *value) {

	*((u_long *)var) = inet_addr(value);   
	return(1); 
}


int TclBinder::b_strfun(void *var, const char *value ) {

	char			**v;

	v = (char **)var;    
	if(*v != 0)
		free(*v);

	*v = strdup(value);
	assert(*v);

	return(1);  
}


int TclBinder::b_macfun(void *var, const char *value) {

	u_char		*v;
 
	v = (u_char *)var; 
	str_to_macaddr(value, v);
	return(1); 
}


int TclBinder::b_floatfun(void *var, const char *value) {

	float			p;

 	if (sscanf(value, "%f", &p)!=1) {
		printf("Error: can not set this variable!\n\n");
 		return(-1);
	} else  *((float *)var) = p;

	return(1); 
}




int TclBinder::Bind_(NslObject *obj, const char *vname, int *var) {

	bindList			*bl;
 

	bl = list_add(obj, vname);
	if(!bl) return(-1);
   
	bl->var_ = (void *)var;
	bl->func_ = &TclBinder::b_intfun;

	return(1);  
}


int TclBinder::Bind_(NslObject *obj, const char *vname, double *var) {

        bindList                        *bl;


	bl = list_add(obj, vname);
	if(!bl) return(-1);
   
        bl->var_ = (void *)var;
        bl->func_ = &TclBinder::b_doublefun;

	return(1); 
}


int TclBinder::Bind_bool(NslObject *obj, const char *vname, u_char *var) {

 	bindList			*bl;
	

	bl = list_add(obj, vname);
	if(!bl) return(-1);
   
        bl->var_ = (void *)var;
        bl->func_ = &TclBinder::b_boolfun;

	return(1); 
}


int TclBinder::Bind_ip(NslObject *obj, const char *vname, u_long *var) {

 	bindList			*bl;


	bl = list_add(obj, vname);
	if(!bl) return(-1);
   
        bl->var_ = (void *)var;
        bl->func_ = &TclBinder::b_ipfun;

	return(1); 
}  


int TclBinder::Bind_(NslObject *obj, const char *vname, char **var) {

	bindList			*bl;


	bl = list_add(obj, vname);
	if(!bl) return(-1);
 
        bl->var_ = (void *)var;
        bl->func_ = &TclBinder::b_strfun;

	return(1); 
}


int TclBinder::Bind_mac(NslObject *obj, const char *vname, u_char *var) {

 	bindList			*bl;


	bl = list_add(obj, vname);
	if(!bl) return(-1);
   
        bl->var_ = (void *)var;
        bl->func_ = &TclBinder::b_macfun;

	return(1); 
}



int TclBinder::Bind_(NslObject *obj, const char *vname, float *var) {

	bindList			*bl;

	bl = list_add(obj, vname);
	if(!bl) return(-1);

	bl->var_ = (void *)var;
	bl->func_ = &TclBinder::b_floatfun;

	return(1);  
}



bindList * TclBinder::list_add(NslObject *obj, const char *vname) {

	int				i;
 	bindTable			*bt;
	bindList			*bl;


	bt = table_lookup(obj);
	if(!bt) bt = table_add(obj);

	if (bt->blidx_ == MAX_BIND_VARS)
		return(0);

	/* search for duplicated variable name */
 	for(i=0; i<bt->blidx_; i++) {
 		bl = &(bt->varlist_[i]);
 		if(!strcmp(bl->vname_, vname))
 			return(0);
	}

	/* otherwise, no duplicated found */
 	bl = &(bt->varlist_[bt->blidx_++]);
 	assert((bl->vname_ = strdup(vname)));

	return(bl); 
}


int TclBinder::setVarValue(NslObject *obj, const char *vname, const char *value) {

	bindTable		*bt;
	bindList		*bl;
 	int			i; 
	

	bt = table_lookup(obj);
   	if(!bt) return(-1);

	for(i=0; i<bt->blidx_; i++) {
		bl = &(bt->varlist_[i]);
		if(!strcmp(bl->vname_, vname)) {
			return((this->*(bl->func_))(bl->var_, value)); 
        	}
	}      
	return(-1); 
}


 
