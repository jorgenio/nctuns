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

#include <stdio.h>
#include <string.h>
#include <nodetype.h>


typeTable       *typeTable_ = new typeTable;
tTable	  	typeTable::ttable_[MAX_NODE_TYPE];

u_int32_t 	typeTable::idx_ = 0;
u_char	  	typeTable::flag_ = 0;    
    

typeTable::typeTable() {

	if (!flag_) {
		/* add default node type */
		Reg_NodeType("EXTHOST", 3);  /* emulation, external host */
		Reg_NodeType("EXTMOBILE", 3);  /* emulation, external mobile */
		Reg_NodeType("EXTMOBILE_INFRA", 3);  /* emulation, external mobile infra */
		Reg_NodeType("EXTROUTER", 3);  /* emulation, external router */
		Reg_NodeType("VIRROUTER", 3);
		Reg_NodeType("HOST", 3); 
		Reg_NodeType("WAN", 2); /* WAN packet delay/reordering/loss box */
		Reg_NodeType("SWITCH", 2); 
		Reg_NodeType("HUB", 1); 
		Reg_NodeType("AP", 2); 
		Reg_NodeType("QoS_AP", 2); 
		//Reg_NodeType("WIRE", 1);
		//Reg_NodeType("WIRELESS", 1);  
		Reg_NodeType("ROUTER", 3);
		Reg_NodeType("QoS_MOBILE_INFRA", 3);
		Reg_NodeType("MOBILE", 3);
		Reg_NodeType("MOBILE_INFRA", 3);
		Reg_NodeType("OPT_SWITCH", 2);
		Reg_NodeType("OBS_OPT_SWITCH", 2);
		Reg_NodeType("QBROUTER", 3);
		Reg_NodeType("QIROUTER", 3);
 		Reg_NodeType("GSWITCH", 2);
		Reg_NodeType("BS",2);
		//Reg_NodeType("MS",3);
		Reg_NodeType("PHONE",3);
		Reg_NodeType("SGSN",3);
		Reg_NodeType("GGSN",3);

#ifdef CONFIG_MESH
	        /* for Wireless Mesh Network Nodes */
                Reg_NodeType("MESH_OSPF_AP", 2);
                Reg_NodeType("MESHSWITCH", 2);
                Reg_NodeType("MESH_STP_AP", 2);
#endif	/* CONFIG_MESH */

#ifdef CONFIG_WIMAX
		Reg_NodeType("WIMAX_PMP_BS", 2);
		Reg_NodeType("WIMAX_PMP_SS", 2);
		Reg_NodeType("WIMAX_MESH_BS", 3);
		Reg_NodeType("WIMAX_MESH_SS", 3);
		//Reg_NodeType("WIMAX_MESH_GATEWAY_SS", 3);
		/* for Mobile Wimax */
		Reg_NodeType("MobileWIMAX_PMPBS", 2);
		Reg_NodeType("MobileWIMAX_PMPMS", 2);

		/*for MobileRelay Wimax-transparent mode*/
		Reg_NodeType("MobileRelayWIMAX_PMPBS", 2);
		Reg_NodeType("MobileRelayWIMAX_PMPMS", 2);
		Reg_NodeType("MobileRelayWIMAX_PMPRS", 2);

		Reg_NodeType("MR_WIMAX_NT_PMPBS", 2);
		Reg_NodeType("MR_WIMAX_NT_PMPRS", 2);
		Reg_NodeType("MR_WIMAX_NT_PMPMS", 2);

#endif	/* CONFIG_WIMAX */

		/* for Satellite DVB-RCS Network Nodes */
#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
                Reg_NodeType("DVB_RCS_SP", 3);
                Reg_NodeType("DVB_RCS_NCC", 2);
                Reg_NodeType("DVB_RCS_RCST", 3);
                Reg_NodeType("DVB_RCS_GATEWAY", 1);
                Reg_NodeType("DVB_RCS_FEEDER", 1);
                Reg_NodeType("DVB_RCS_SAT", 1);
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */

                /* C.C. Lin: added for generic supernode type */
                Reg_NodeType("SUPERNODE", 3);
	
		/* ITS nodes */
                Reg_NodeType("CAR_INFRA", 3);
                Reg_NodeType("CAR_ADHOC", 3);
                Reg_NodeType("CAR_GPRS_PHONE", 3);
                Reg_NodeType("CAR_RCST", 3);
		Reg_NodeType("CAR_LS", 3); // Large-Scale Car
		Reg_NodeType("WAVE_OBU", 3);
		Reg_NodeType("WAVE_RSU", 3);
	}
}


typeTable::~typeTable() {

}


int typeTable::Reg_NodeType(const char *name, u_char layer) {

	u_int32_t		i;

	/* search for duplicated type name */
	for(i=0; i<idx_; i++) 
		if ( !strcmp(ttable_[i].name_, name) )
			return(-1);

	ttable_[idx_].layer_ = layer;  
	strcpy(ttable_[idx_++].name_, name);
	return(1); 
}


/*
 * return 0, meaning not found, otherwise
 * successfully found.
 */
u_int32_t typeTable::toType(const char *name) {

	u_int32_t		i;

	for(i=0; i<idx_; i++) {
		if( !strcmp(ttable_[i].name_, name) ) {
			return(i+1);   
		}
	}      
	return(0); 
}

const char * typeTable::toName(u_int32_t type) {

	return(ttable_[type-1].name_); 
}


u_char typeTable::NameToLayer(const char *name) {

	u_int32_t		i;

	for(i=0; i<idx_; i++) {
		if ( !strcmp(ttable_[i].name_, name) )
			return(ttable_[i].layer_);
	}
	return(0);
}


u_char typeTable::TypeToLayer(u_int32_t type) {

	return(ttable_[type-1].layer_); 
}
          
  
 
void typeTable::display() {

	u_int32_t		i;

	printf("Listing Node Type ......\n\n"); 
	for(i=0; i<idx_; i++) {
		printf("  %s layer: %d \n", ttable_[i].name_, 
					    ttable_[i].layer_); 
	}      
}

         
