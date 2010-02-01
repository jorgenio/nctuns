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
#include <stdlib.h>
#include <unistd.h>
#include <regcom.h>
#include <config.h>

#include <string.h>
#include <assert.h>

#include "nctuns.h"
#include "command_server.h"
#include "module/phy/power/channel_Model.h"

extern cmd_server *cmd_server_;

int main(int argc, char *argv[]) {

	extern int Start_Simulator	__P((int, char **));
	int 				opt;
	int				longindex;

	/*
	 * parse all options and arguments from command line
	 */

	while((opt=getopt_long(argc, argv, OPT_STR_FOR_SHORT_OPT_CHAR, LongOpts, &longindex)) != -1){
		switch(opt) {
			case RETURN_VALUE_FOR_OPT_HELP:
				printf("\nSupport command formats:\n");
				printf("./nctuns -h|--help\n");
				printf("./nctuns -v|--version\n");
				printf("./nctuns -m|--query_cm_list output_file\n");
				printf("./nctuns -s|--query_cs_dist\n");
				printf("./nctuns input_tcl_file\n");
				exit(0);
			case RETURN_VALUE_FOR_OPT_VERSION:
				printf("\nVersion:\n"
					"\tNCTUns Kernel: %s\n"
					"\tNCTUns Engine: %s-%s %s\n",
					CONFIG_KERNELVERSION,
					CONFIG_VERSION,
					CONFIG_LOCALVERSION,
					NCTUNS_VERSION);
				exit(0);
			case RETURN_VALUE_FOR_OPT_QUERY_CM_LIST:
				Dump_CM_List(optarg);
				exit(0);
			case RETURN_VALUE_FOR_OPT_QUERY_CS_DIST:
				Get_CM_Property();
				exit(0);
			default:
				printf("Wrong command format: use \"./nctuns -h\" for help\n");
				exit(0);
		}
	}


	cmd_server_ = new cmd_server();
	assert(cmd_server_);

	/*
	 * Module Registry :
	 *
	 *	All of the modules used by the
	 * NCTU Simulator must register before the
	 * Simulator start.
	 */

	/* Group : ARP */
	REG_MODULE("ARP", arp);

	/* Group : HUB */
	REG_MODULE("Hub", hub);

	/* Group : MAC8023 */
	REG_MODULE("MAC8023", mac8023);

	/* Group : MAC80211 */
	REG_MODULE("MAC80211", mac802_11dcf);
	REG_MODULE("80211e", mac802_11e);
	REG_MODULE("MAC80211p", mac_80211p);
	REG_MODULE("MAC80211a", mac802_11a_dcf);

	/* Group : PHY */
	REG_MODULE("Phy", phy);

	/* Group : WPHY */
	REG_MODULE("Wphy", wphy);
	REG_MODULE("AWphy", awphy);
	REG_MODULE("phy_80211p", phy_80211p);
        REG_MODULE("phy_80211a", phy_80211a);

	/* Group : PS&BM */
	REG_MODULE("FIFO", fifo);
#ifdef	CONFIG_GPRS
	REG_MODULE("GPRS_FIFO", GPRSfifo);
#endif
	REG_MODULE("RED", RED);
	REG_MODULE("DRR", drr);
	REG_MODULE("WAN", WAN);
	REG_MODULE("DS_I", ds_i);
	REG_MODULE("DS_TC", TrafCond);
	REG_MODULE("DS_REMARKER", Remarker);

	/* Group : MROUTED */
	REG_MODULE("GOD", GodRouted);

	/* Group : SW */
	REG_MODULE("Switch", sw);

	/* Group : MNode */
	REG_MODULE("MNode",MoblNode);
	REG_MODULE("QoSMN",qosMN);

	/* Group : AP */
	REG_MODULE("AP",AP);
	REG_MODULE("QoSAP",qosAP);

	/* Group : OTHERS */
	REG_MODULE("TCPDUMP",tcpdump);
	REG_MODULE("WTCPDUMP",wtcpdump);

	/* Group : NCTUNS_dep */
	REG_MODULE("Interface", interface);
        REG_MODULE("FakeIf", FakeIf);
	REG_MODULE("MultiIf", MultiIf);
	REG_MODULE("PseudoIf", PseudoIf);

	REG_MODULE("Link", Link);
	REG_MODULE("Node", Node);


	/* Group: MROUTED */
	REG_MODULE("DSDV",DSDVd);
	REG_MODULE("DSR",DSRAgent);
	REG_MODULE("ADV",ADVd);
	REG_MODULE("AODV",AODV);

	/* Group : Channel Model (CM) */
	REG_MODULE("CM", cm);

#ifdef	CONFIG_OPTICAL
	REG_MODULE("OPT_PHY", ophy);
	REG_MODULE("OPT_PORT", op);
	REG_MODULE("OPT_SW", osw);
	REG_MODULE("OPT_WA", wa);
	REG_MODULE("OPT_RWA", rwa);
	REG_MODULE("OPT_MA", opmanage);
	REG_MODULE("OPT_OBWA",obwa);
	REG_MODULE("OPT_OBSW",obsw);
	REG_MODULE("OPT_FIFO",ofifo);
#endif	/* CONFIG_OPTICAL */

#ifdef	CONFIG_GPRS
	/* Group: GPRS */
        REG_MODULE("GPRSMsMac",GprsMsMac);
        REG_MODULE("BTSMAC",GprsBtsMac);
        REG_MODULE("GprsSw",GprsSw);
        REG_MODULE("LLC",Llc);
        REG_MODULE("MSRLC",MsRlc);
        REG_MODULE("BTSRLC",BtsRlc);
        REG_MODULE("NS",NS);
        REG_MODULE("SGSNBssgp",SgsnBSSGP);
        REG_MODULE("BSSBssgp",BssBSSGP);
        REG_MODULE("SNDCPGmm",SndcpGmm);
        REG_MODULE("GPRSGmm",GprsGmm);
        REG_MODULE("SGSNGtp",SgsnGtp);
        REG_MODULE("GGSNGtp",GgsnGtp);
        REG_MODULE("BSSRelay",BSSRelay);
        REG_MODULE("RadioLink",radiolink);
#endif	/* CONFIG_GPRS */

#ifdef CONFIG_MESH
        REG_MODULE("MeshSW", MeshSW);
        REG_MODULE("MeshOSPF", MeshOSPF);
        REG_MODULE("MeshARP" , MeshArp);
        REG_MODULE("Bridge", Bridge);
#endif	/* CONFIG_MESH */

#ifdef CONFIG_WIMAX
	REG_MODULE("MAC802_16_PMPBS", mac802_16_PMPBS);
	REG_MODULE("MAC802_16_PMPSS", mac802_16_PMPSS);
	REG_MODULE("MAC802_16_MeshSS", mac802_16_MeshSS);
	REG_MODULE("MAC802_16_MeshBS", mac802_16_MeshBS);
	REG_MODULE("OFDM_PMPBS", OFDM_PMPBS);
	REG_MODULE("OFDM_PMPSS", OFDM_PMPSS);
	REG_MODULE("OFDM_Mesh", OFDM_Mesh);
	REG_MODULE("Mesh_Route", MeshRoute);
#endif	/* CONFIG_WIMAX */

#if defined(CONFIG_WIMAX) && defined(CONFIG_MobileWIMAX)
	REG_MODULE("MAC802_16E_PMPBS", mac802_16e_PMPBS);
	REG_MODULE("MAC802_16E_PMPMS", mac802_16e_PMPMS);
	REG_MODULE("OFDMA_PMPBS", OFDMA_PMPBS);
	REG_MODULE("OFDMA_PMPMS", OFDMA_PMPMS);
#endif  /* CONFIG_WIMAX && CONFIG_MobileWIMAX */

#if defined(CONFIG_WIMAX) && defined(CONFIG_MobileWIMAX) && defined(CONFIG_MobileRelayWIMAX)
//WiMAX transparent-mode system module register
	REG_MODULE("MAC802_16J_PMPBS", mac802_16j_PMPBS);
	REG_MODULE("MAC802_16J_PMPMS", mac802_16j_PMPMS);
	REG_MODULE("MAC802_16J_PMPRS", mac802_16j_PMPRS);
	REG_MODULE("OFDMA_PMPBS_MR", OFDMA_PMPBS_MR);
        REG_MODULE("OFDMA_PMPMS_MR", OFDMA_PMPMS_MR);
	REG_MODULE("OFDMA_PMPRS_MR", OFDMA_PMPRS_MR);
#endif

#if defined(CONFIG_WIMAX) && defined(CONFIG_MobileWIMAX) && defined(CONFIG_MR_WIMAX_NT)
//WiMAX non-transparent mode system module register
	REG_MODULE("MAC802_16J_NT_PMPBS", mac802_16j_NT_PMPBS);
	REG_MODULE("MAC802_16J_NT_PMPRS", mac802_16j_NT_PMPRS);
	REG_MODULE("CROSS802_16J_NT_PMPRS",cross802_16j_NT_PMPRS);
	REG_MODULE("MAC802_16J_NT_PMPMS", mac802_16j_NT_PMPMS);
	REG_MODULE("OFDMA_PMPBS_NT", OFDMA_PMPBS_NT);
	REG_MODULE("OFDMA_PMPRS_NT", OFDMA_PMPRS_NT);
	REG_MODULE("OFDMA_PMPMS_NT", OFDMA_PMPMS_NT);
#endif

#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
	/* Group: DVB-RCS */
	REG_MODULE("RCS_ATM_RCST", Rcs_atm_rcst);
	REG_MODULE("RCS_ATM_SP", Rcs_atm_sp);
	REG_MODULE("RCS_MAC_RCST", Rcs_mac_rcst);
	REG_MODULE("RCS_MAC_SP", Rcs_mac_spncc);
	REG_MODULE("RCS_MAC_NCC", Rcs_mac_spncc);
	REG_MODULE("DVB_RCS_RCST", Dvb_rcs_rcst);
	REG_MODULE("DVB_RCS_GW", Dvb_rcs_gw);
	REG_MODULE("DVB_S2_FEEDER", Dvb_s2_feeder);
	REG_MODULE("DVB_S2_RCST", Dvb_s2_rcst);
	REG_MODULE("DVBS2_SAT", Dvbs2_sat);
	REG_MODULE("NCC_CTL", Ncc_ctl);
	REG_MODULE("SP_CTL", Sp_ctl);
	REG_MODULE("RCST_CTL", Rcst_ctl);
	REG_MODULE("SECTION", Section);
	REG_MODULE("MPE", Mpe);
	REG_MODULE("MPEG2_TS_SP", Mpeg2_ts_sp);
	REG_MODULE("MPEG2_TS_NCC", Mpeg2_ts_ncc);
	REG_MODULE("MPEG2_TS_RCST", Mpeg2_ts_rcst);
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS && */

	REG_MODULE("WME", WME);

	/*
	 * Start the Simulator
	 */
	Start_Simulator(argc, argv);
	return(0);
}
