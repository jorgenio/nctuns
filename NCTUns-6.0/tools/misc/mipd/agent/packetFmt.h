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

#ifndef MIP_MNSTATE
#define MIP_MNSTATE
typedef enum{ATHOME, FOREIGN, AWAY, NA} MnState;
#endif
typedef enum{ADVERTISE, REPLY, REPLY_ACK, REG_REQ, REG_REPLY, 
	     RO_UPDATE, RO_WARN, RO_ACK, UNKNOWN} PacketType; 
/* packetType :
 * ADVERTISE --> advertisement packet broadcasted from agent.
   CONFIRM --> confirm packet from Mnd.    
   REGISTER--> registration from FA to HA.
   UNKNOWN --> not mobileIP packet. */

struct mipPkt{            
	u_char         type;       /* format of registration from FA to HA:1 */
                                   /* format of registration reply:3 */
	                           /* format of advertisement to MN:9 */
	                           /* format of solicitation from MN:10 */
	                           /* format of reply from MN to agent:13 */
	                           /* format of reply ack from agent to MN:99 */
#if 0
        struct in_addr agentOuterAddr;                          	
        struct in_addr agentInnerAddr;                          	
	struct in_addr haddr;     /* addr of HA belongs to MN is provided
				      for FA to registrate. */
	struct in_addr coaddr;     /* COA, care-of-addr, i.e. the outer IP
				      of FA */
        struct in_addr mnaddr;
	struct in_addr ro_addr;
        MnState        mnState;
        u_short        lifetime;
#endif
};
 
struct mipAdvertisePkt{            
	u_char         type;

        struct in_addr agentOuterAddr;                          	
        struct in_addr agentInnerAddr;                          	
	
        u_short        lifetime;
};

struct mipReplyPkt{            
	u_char         type;

	struct in_addr mnaddr;
	struct in_addr haddr;
        struct in_addr agentInnerAddr;                          	
	struct in_addr preagent;

        u_short        lifetime;
};

struct mipReplyAckPkt{            
	u_char         type;

        struct in_addr agentOuterAddr;                          	
        struct in_addr agentInnerAddr;                          	

        u_short        lifetime;
};

struct mipRegPkt{            
	u_char         type;
	
	struct in_addr mnaddr;
	struct in_addr coaddr;
	struct in_addr haddr;
        struct in_addr agentInnerAddr; /* for nctuns. */

        u_short        lifetime;
};

struct mipRegReplyPkt{            
	u_char         type;

	struct in_addr mnaddr;
	struct in_addr haddr;
        struct in_addr agentInnerAddr; /* for nctuns. */

        u_short        lifetime;
};

struct mipROupdatePkt{            
	u_char         type;

	struct in_addr mnaddr;
	struct in_addr coaddr;
	struct in_addr haddr;

        u_short        lifetime;
};

struct mipROackPkt{            
	u_char         type;

	struct in_addr mnaddr;
};

struct mipROwarnPkt{            
	u_char         type;

	struct in_addr mnaddr;
	struct in_addr target;
};

/* struct fmtRoPacket{
	u_char         type;

	struct in_addr homeAddr;
	struct in_addr coaddr;
};  */
