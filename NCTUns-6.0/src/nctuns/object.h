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

#ifndef	__NCTUNS_nslobject_h__
#define __NCTUNS_nslobject_h__

#include <stdio.h>
#include <event.h>

class MBinder;

struct plist {
	u_int8_t		pid;
	struct plist            *next;
};

struct MBlist {
	u_int8_t	portnum;
	MBinder		*sendt;
	struct MBlist   *next;
};
/*=========================================================================
   Define Macros
  =========================================================================*/
#define DISABLED		0x00
#define ENABLED			0x01


/*=========================================================================
   Define Class ProtoType
  =========================================================================*/

class NslObject {

 private: 

	char		*name_;		/* Instance name */
	const u_int32_t	nodeID_;	/* Node Id */
	const u_int32_t	nodeType_;	/* Node type, eg: SWITCH, HOST.. */
 	u_int32_t	portid_;  	/* port Id */
	struct plist    *MPlist_;
	
 public :
	/* add for new structure engine*/
        u_int32_t       pdepth;
        struct MBlist   *BinderList;
        u_int8_t       PortNum;
					
   	
	u_char          s_flowctl;      /* flow control for sending pkt */
	u_char          r_flowctl;      /* flow control for receiving pkt */
	        
	MBinder		*recvtarget_;	/* to upper component */
	MBinder		*sendtarget_;	/* to lower component */

	  	
	NslObject(u_int32_t, u_int32_t, struct plist*, const char *);
	NslObject();
	virtual 		~NslObject();   
	virtual int		init();
	virtual int		recv(ePacket_ *); 
	virtual int		send(ePacket_ *); 
	virtual int		get(ePacket_ *, MBinder *);
	virtual int		put(ePacket_ *, MBinder *);
	virtual ePacket_	*put1(ePacket_ *, MBinder *);
	virtual int		command(int argc, const char *argv[]); 
	virtual int		Debugger();
    

	inline  void	set_port(u_int32_t portid) { 
			portid_ = portid; 
		};   
	inline u_int32_t get_port() const { 
			return(portid_); 
		}; 
	inline struct plist* get_portls() const {
			return(MPlist_);
		};
	inline const char * get_name() const {
			return(name_);
		}
	inline u_int32_t get_nid() const {
			return(nodeID_);
		}
	inline u_int32_t get_type() const {
			return(nodeType_);
		}
};

/* Added by CCLin to uniformly format
 * debugging messages.
 */

#define NSLOBJ_DEBUG_STRING_HEAD() do {				\
	double sec;						\
	TICK_TO_SEC(sec, GetCurrentTime());			\
	printf("%11.7f: [%03d]%s::%s: ",			\
			sec, get_nid(),				\
			getModuleName(this), __FUNCTION__);	\
} while(0)

#endif	/* __NCTUNS_object_h__ */
