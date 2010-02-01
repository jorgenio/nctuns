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

#ifndef	__NCTUNS_config_h__
#define	__NCTUNS_config_h__

#include <sys/types.h>
#include <gbind.h>

/*===================================================================
   Define Macros
  ===================================================================*/
#define ErrorMesg(mesg) {					\
	printf("nctuns: ");					\
	printf("%s\nStop Simulation!", mesg);			\
	exit(-1); 						\
}

#define UsageMesg(mesg) {					\
	printf("%s\n", mesg);					\
} 

/* Time translation */
#define SEC_TO_TICK(tick, time)		tick = (u_int64_t)((time)*(1000000000.0/TICK))
#define MILLI_TO_TICK(tick, time)	tick = (u_int64_t)((time)*(1000000.0/TICK))
#define MICRO_TO_TICK(tick, time)	tick = (u_int64_t)((time)*(1000.0/TICK))
#define NANO_TO_TICK(tick, time)	tick = (u_int64_t)(((time)*1.0)/TICK)
#define TICK_TO_SEC(time, tick)		time = (double)((tick)*TICK/1000000000.0) 
#define TV_TO_TICK(tick, tv)		tick = (u_int64_t)((tv).tv_sec * (1000000000.0 / TICK)) + \
						(u_int64_t)((tv).tv_usec * (1000.0 / TICK))

#define TICK_TO_SEC(time, tick)		time = (double)((tick)*TICK/1000000000.0)
#define TICK_TO_MILLI(time, tick)	time = (double)((tick)*TICK/1000000.0)
#define TICK_TO_MICRO(time, tick)	time = (double)((tick)*TICK/1000.0)
#define TICK_TO_NANO(time, tick)	time = (double)((tick)*TICK/1.0) 


/* implementation to pointer to member */
#define BASE_OBJTYPE(name)		int (NslObject::*name)(Event_ *);


#define POINTER_TO_MEMBER(obj, member)			\
	(int (NslObject::*)(Event_ *))&obj::member

/* for module register */
#define MODULE_GENERATOR(type)					\
NslObject * Create_ ## type ## _Module(u_int32_t a, u_int32_t b, struct plist* d, const char *c) {\
         type *z;\
         z = new type(a, b, d, c);\
         if (!z) ErrorMesg("malloc() failed\n");\
         return((NslObject *)z);\
}

#define REG_MODULE(name, type) \
{\
        extern NslObject * Create_ ## type ## _Module(u_int32_t a, u_int32_t b, struct plist* d, const char *c);\
        extern RegTable         RegTable_;\
	RegTable_.Register_Com(name, Create_ ## type ## _Module);\
}




/* Define system call number */
/*
 * The maximum number of tunnel interfaces to be simulated, can be
 * increased if needed. This number should be consistent with that
 * defined and used by the simulation engine.
 */
#define MAX_NUM_IF_TUN	4096
#define MAX_NUM_TUN	MAX_NUM_IF_TUN + 1

/*
 * The maximum number of nodes to be simulated, can be
 * increased if needed. This number should be consistent with that
 * defined and used by the simulation engine.
 */
#define MAX_NUM_NODE	4096             		


/*
 * Dispatcher flags for flags field.
 */
#define E_RONLY                 0x01
#define E_WONLY                 0x02



/* Define constant for Node Editor */
#define CHECKBOX		0x01
#define RADIOBOX		0x02
#define COMBOBOX		0x03
#define TEXTLINE_INT		0x04
#define TEXTLINE_STR		0x05
#define TEXTLINE_FLT		0x06


#endif	/* __NCTUNS_config_h__ */
