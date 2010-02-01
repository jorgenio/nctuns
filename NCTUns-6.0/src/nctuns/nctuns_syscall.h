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

#ifndef __NCTUNS_nctuns_syscall_h__
#define __NCTUNS_nctuns_syscall_h__

#include <unistd.h>

struct divert_rule;

/*
 * this struct which be used by syscall_NCTUNS_misc, and action equal R2VPORT
 * or V2RPORT
 */
struct syscall_port_info {
	/*
	 * this proto field must fill protocol number of <netinet/in.h>
	 */
	int		proto;	/* protocol number */
	unsigned long	ip;	/* interface ip */
	unsigned short	rport;	/* real port */
	unsigned short	vport;	/* virtual port */
};

/*--------------------------------------------------------------------------
 * Define System call number for nctuns
 *--------------------------------------------------------------------------*/       
#define __NR_syscall_number_base			333
#define __NR_syscall_NCTUNS_divert			__NR_syscall_number_base + 0
#define	__NR_syscall_NCTUNS_clearStateAndReinitialize	__NR_syscall_number_base + 1
#define	__NR_syscall_NCTUNS_misc			__NR_syscall_number_base + 2
#define	__NR_syscall_NCTUNS_callout_chk			__NR_syscall_number_base + 3
#define	__NR_syscall_NCTUNS_mapTable			__NR_syscall_number_base + 4
#define	__NR_syscall_NCTUNS_cancel_socknodeID		__NR_syscall_number_base + 5
#define	__NR_syscall_NCTUNS_usleep_After_Msg_Send	__NR_syscall_number_base + 6

/*--------------------------------------------------------------------------
 * Define misc of system call action enumeration number
 *--------------------------------------------------------------------------*/       
enum syscall_NSC_misc_enum {
	syscall_NSC_misc_TEST = 0x01,
	syscall_NSC_misc_GETNIDINFO,
	syscall_NSC_misc_REGPID,
	syscall_NSC_misc_SET_ENDTIME,
	syscall_NSC_misc_NIDTOTID,
	syscall_NSC_misc_NIDNUM,
	syscall_NSC_misc_R2VPORT,
	syscall_NSC_misc_V2RPORT,
	syscall_NSC_misc_TICKTONANO,
	syscall_NSC_misc_SET_TUN,
	syscall_NSC_misc_GET_TUN,
	syscall_NSC_misc_PIDTONID
};

/*--------------------------------------------------------------------------
 * Define Macros
 *--------------------------------------------------------------------------*/       
/* system call wrapping */
static inline int syscall_NCTUNS_divert(int action, int fd, unsigned long hook, struct divert_rule *dr, int addr_len) {
	return syscall(__NR_syscall_NCTUNS_divert, action, fd, hook, dr, addr_len);
};

static inline int syscall_NCTUNS_clearStateAndReinitialize(int nctuns_pid) {
	return syscall(__NR_syscall_NCTUNS_clearStateAndReinitialize, nctuns_pid);
};

static inline int syscall_NCTUNS_misc(int action, unsigned long value1, unsigned long value2, unsigned long value3) {
	return syscall(__NR_syscall_NCTUNS_misc, action, value1, value2, value3);
};

static inline int syscall_NCTUNS_callout_chk() {
	return syscall(__NR_syscall_NCTUNS_callout_chk);
}

static inline int syscall_NCTUNS_mapTable(int action, unsigned long nid, unsigned long tid, unsigned char *mac, unsigned short s_port) {
	return syscall(__NR_syscall_NCTUNS_mapTable, action, nid, tid, mac, s_port);
};

static inline int syscall_NCTUNS_cancel_socknodeID(int fd) {
	return syscall(__NR_syscall_NCTUNS_cancel_socknodeID, fd);
};

static inline int syscall_NCTUNS_usleep_After_Msg_Send(unsigned long usecond, int fd, char *buf, size_t count, int pre_schedule_select) {
	return syscall(__NR_syscall_NCTUNS_usleep_After_Msg_Send, usecond, fd, buf, count, pre_schedule_select);
};

#endif /* __NCTUNS_nctuns_syscall_h__ */
