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

#ifdef DEBUG_MOBILEIP
 /* #define DEBUG_PRINTLIST */
 #define DEBUG_REBINDUPDATE
 #define DEBUG_VLIST_DEL
 #define DEBUG_BLIST_DEL
 /* #define DEBUG_LISTENSOCKET */
 #define DEBUG_RECEIVE_REPLY
 #define DEBUG_MN_AT_HOME
 #define DEBUG_MN_AWAY
 #define DEBUG_REG_REQ
 #define DEBUG_REG_REPLY
 #define DEBUG_RO_UPDATE
 #define DEBUG_RO_ACK
 /* #define DEBUG_DIVERTSOCKET */
 #define DEBUG_RAWSOCKET_ON_HA_SYM
 /* #define DEBUG_RAWSOCKET_ON_HA
 #define DEBUG_MTU
 #define DEBUG_ROSOCKET */
 #define DEBUG_RO_SMOOTH_HANDOFF
 #define DEBUG_RO_SPECIALTUN_SYM
 #define DEBUG_RO_SYM
 /* #define DEBUG_RO_RAWSOCKET
 #define DEBUG_IPINUDPSOCKET
 #define DEBUG_RAWSOCKET_ON_FA */
 #define DEBUG_RAWSOCKET_ON_FA_SYM
 #define DEBUG_ISCHILDMND
 #define DEBUG_CHANGE_COA_OF_MN
 /* #define DEBUG_QUERY_MN_LIST */
 #define DEBUG_IS_IN_VLIST
 #define DEBUG_UPDATE_BLIST
 #define DEBUG_ACK_TO_MND
 /* #define DEBUG_ADD_BCAST_ROUTE */
 #define DEBUG_REG_TO_HA
 #define DEBUG_REGREPLY_TO_FA
 #define DEBUG_BU_SYM
 #define DEBUG_REBU_SYM
 #define DEBUG_BA_SYM
 #define DEBUG_BW_SYM
 #define DEBUG_SH_SYM
 #define DEBUG_NOBUFS_SYM
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>

int               ip2ssdd(struct in_addr, struct in_addr, char *);
struct in_addr *  ssdd2dst(struct in_addr *ssdd);
struct in_addr *  ssdd2src(struct in_addr *ssdd);
char *            addr2str(struct in_addr *addr);
int               cpaddr(struct in_addr *, struct in_addr *);
struct in_addr *  dstFrmPkt(char *);
struct in_addr *  srcFrmPkt(char *);
