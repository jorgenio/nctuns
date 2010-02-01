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

class MSH_CSCH:public ManagementMessage {
	u_char type;
	u_char ConfigSeqNumber:3;
	u_char GrantFlag:1;
	u_char ScheduleFlag:1;
	u_char configFlag:1;
	u_char reserve:2;
	u_char NFlowEntry:8;
/*
	Flow Entries orz
*/


	 MSH_NENT(u_char pType);
	 MSH_NENT(u_char * pBuffer, int pLength);
	~MSH_NENT();
	int copyTo(u_char * dstbuf);
};

class MSH_CSCF:public ManagementMessage {
	u_char type;
	u_char ConfigSeqNumber:4;
	u_char NChannel:4;
	u_char Channelindex[8];
	u_char Nnode:8;
/*
	Tree representation list
 */
	 MSH_NENT(u_char pType);
	 MSH_NENT(u_char * pBuffer, int pLength);
	~MSH_NENT();
	int copyTo(u_char * dstbuf);
};
