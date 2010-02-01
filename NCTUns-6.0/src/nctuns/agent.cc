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

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <unistd.h>
#include <math.h>

#include <agent.h>
#include <config.h>                             // for MAX_NODES
#include <command_server.h>

unsigned long           g_nNumAgents = 0; 

GroupAgent		g_vGroupAgent[GROUP_TYPE_END - GROUP_TYPE_START];

CAgent*                 g_pAgentList[MAX_NUM_NODE + 1];

/*
 * Add all group types which are defined in group_type_define.h to
 * vector g_vGroupAgent.
 * This function will be called only once in the constructor of cmd_server().
 */
void GROUP_INIT() {
	// we add GROUP_TYPE_START to let vector can be searched
	for (EnumGroupType i = GROUP_TYPE_START; i < GROUP_TYPE_END; i = static_cast<EnumGroupType>(int(i) + 1)) {
		g_vGroupAgent[i].SetGroupType(i);
	}
}

GroupAgent * GROUP_FIND(EnumGroupType type) {
        // Becuse we add GroupAgent into vector sequentially, they are 
	// arranged in the EnumGroupType order
        assert (g_vGroupAgent[type].GetGroupType() == type);
        return (&g_vGroupAgent[type]);
}

CAgent* GetAgent(int nid) {
	return(g_pAgentList[nid]);
}
	
const u_char getAgentHumanOrProgramControlled(int nid) {
int control;

	control = g_pAgentList[nid]->GetControl();
	return(control);
}

int setAgentHumanOrProgramControlled(int nid, u_char control) {
int j, id;

        g_pAgentList[nid]->SetControl(control);
	// A human can control only one agent at a time.
	// Therefore, we need to find the node that is currently controlled by
	// the human and set it "program-controlled."
	if (control == 1) {
	  GROUPMEMBER_VECTOR* gv = GROUP_FIND(GROUP_ALL)->GetMember();
          for (j = 0; j < static_cast<int>(gv->size()); j++) {
            id = ((*gv)[j])->GetID();
	    if (id == nid) continue;
	    if (g_pAgentList[id]->GetControl() == 1) {
	      g_pAgentList[id]->SetControl(0);
	    }
	  }
	}
	return(1);
}

GroupAgent::GroupAgent()
{
}

GroupAgent::~GroupAgent()
{
}
