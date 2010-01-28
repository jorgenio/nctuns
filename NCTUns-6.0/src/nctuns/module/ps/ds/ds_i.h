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

#ifndef __NCTUNS_DS_I_h__
#define __NCTUNS_DS_I_h__

#include <object.h>
#include <packet.h>
#include <timer.h>
#include "dsqueue.h"


class ds_i : public NslObject {

 public:
	ds_i(u_int32_t type, u_int32_t id,struct plist *pl, const char *name);
	~ds_i();
	int				init();
	int				recv(ePacket_ *);
	int				send(ePacket_ *);

	
 protected:
 	int 			intrq(MBinder *);

 private:
	AFQueue			*af1,*af2,*af3,*af4;
	DSQueue			*be,*nc,*ef;
	DSQueue			*cntQue;
	DSQueue			*qArray[7];			/* af1,af2,af3,af4,be,nc,ef*/
	int				qRate[6];

	// variables which can be set from GUI
	char			*ds_domain;			//	ds domain name
	char			*log_qlen; 			//	on/off
	char			*log_qdrop;			//	on/off
	char			*qlen_option;		//	saiple_rate/full_log
	double			qlen_samRate;		//	(second)
	double			qdrop_samRate;		//	(second)
	
	// variables for log files
	timerObj		lenLogTimer;
	timerObj		dropLogTimer;
	bool			qlen_full_log;
	char			*log_qlen_file[7];	/* af1,af2,af3,af4,be,nc,ef */
	char			*log_drop_file[11];	/* af11,af12,af21,af22,af31,af32,af41,af42,be,nc,ef*/
	void			logQueueDrop();
	void			logQueueLength();

	void			DSCPTophb(char* phb,const unsigned char codepoint);
	void			parseDSFile(char* dsFilePath);


	// debug & show message
	static const char*		className;
	void 			showMsg(const int msgType,const char* funcName,const char* message);
	int				debug;
	bool			DDDshowMe;
};


#endif /* __NCTUNS_DS_I_h__ */

