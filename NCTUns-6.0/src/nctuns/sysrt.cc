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
#include <assert.h>
#include <nctuns_api.h>
#include <sysrt.h>
#include <exportStr.h>

sysrt            *sysrt_ = NULL;

sysrt::sysrt() {

	char buf[500];
	char path[300], *ptr;

#ifndef LINUX
	sprintf(buf, "%s/route flush", getenv("NCTUNS_TOOLS"));
        if (system(buf) == -1) {
		printf("Error: can not execute %s", buf);
                exit(-1);
	}
#endif

#define PRIO_PATH	"/sbin:/usr/sbin"
	if (!(ptr = getenv("PATH"))) {
		printf("Error: cannot get environment for PATH!\n");
		exit(-1);
	}
	sprintf(path, "%s:%s", PRIO_PATH, ptr);
	if (setenv("PATH", path, 1) == -1) {
		perror("setenv");
		exit(-1);
	}


#ifdef LINUX
#define SRT_EXT_LINUX	".srt-l"
	system("/sbin/sysctl -w net.ipv4.ip_forward=1");
	sprintf(buf, "%s %s%s", getenv("SHELL"), GetScriptName(), SRT_EXT_LINUX);
#else
#define SRT_EXT_BSD	".srt-f"
	sprintf(buf, "%s %s%s", getenv("SHELL"), GetScriptName(), SRT_EXT_BSD);
#endif
	if (system(buf) == -1) {
		printf("Error: set routing entries error!\n"); 
		exit(-1);
	}
}


sysrt::~sysrt() {

}




int sysrt::get_rt(u_int32_t nodeid) {

	int			i; 
	u_long			nip, tid, tmpid; 
	FILE			*fd;
 	char			buf[100], gt[20], dst[20];
	char			flg[10], nxtif[10]; 
#ifdef LINUX
	char 			mask[20], mss[10], win[10], irtt[10];
#else
	char 			ref[10], use[10];
#endif
 	u_char			*pr; 
	struct ExportStr	*ExpStr;
	u_int32_t		row = 0;
	u_int32_t		column = 0;
	u_char			duplicate;

#ifdef LINUX
	if (system("netstat -rn | grep tun > temp.rt") == -1) {
#else
	if (system("netstat -rn -f inet | grep tun > temp.rt") == -1) {
#endif
		printf("Error: can not read system routing table!\n"); 
 		exit(-1);
	}

	if ((fd=fopen("temp.rt", "r")) == NULL) {
		printf("Error: can not read system routing table!\n");
		exit(-1); 
	}   

	/* make the node's IP format to 1.0.X.X */
	pr = (u_char *)&nip;
	pr[0] = 1; pr[1] = 0;      

#ifdef LINUX
	ExpStr = new ExportStr(7);	
#else
	ExpStr = new ExportStr(7);	
#endif
	sprintf(buf,"Routing Table of node %u :\n", nodeid);
	ExpStr->Insert_comment(buf);
#ifdef LINUX
	sprintf(buf,"\nDistination\t\tGateway\t\tGenmask\t\tFlags\tMSS\tWindow\tirtt\tIface\n");
#else
	sprintf(buf,"\nDst\t\tGateway\t\tFlags\tRefs\tUse\tNetif\tExpire\n");
#endif
	ExpStr->Insert_comment(buf);

	while (!feof(fd)) {
		buf[0] = '\0'; 
		fgets(buf, 100, fd);
		if (buf[0] == '\0') 
			continue;

#ifdef LINUX
		if( 8 != sscanf(buf, "%s %s %s %s %s %s %s %s", 
				dst, gt, mask, flg, mss, win, irtt, nxtif)) 
#else
		if( 6 != sscanf(buf, "%s %s %s %s %s %s", 
				dst, gt, flg, ref, use, nxtif)) 
#endif
			 continue;

		sscanf(dst, "%u.%u%s", (unsigned int *)pr+2, (unsigned int *)pr+3, buf);
#ifdef LINUX
		if ( pr[2]==1 && pr[3]==0 )
			continue;
#endif

		/* If nip doesn't belong to desired node, we should
		 * ignore this entry or we should modify it to fit
		 * in with real routing entry.
		 */
		if (nodeid != (tmpid=ipv4addr_to_nodeid(nip)))
			continue; 

		/* modify destination address to 1.0.x.x */
		sprintf(dst, "1.0%s", buf);

		/* filter the duplicate entry */
		duplicate = 0;
		for(i = 1; (u_int32_t)i <= row; i++) {
			if( ExpStr->Get_cell(i,1) != NULL ) {
				if( !strncmp(dst, ExpStr->Get_cell(i,1), 
							strlen(dst)) )
					duplicate = 1;
			}
		}
		if( duplicate == 1 )
			continue;

		/* change output interface to fxpXX */
		nxtif[0] = nxtif[1] = nxtif[2] = ' ';
    		sscanf(nxtif, "%u", (unsigned int *)&tid); 
#ifdef LINUX
		sprintf(nxtif, "eth%u", getportbytunid(tid)); 
#else
		sprintf(nxtif, "fxp%u", getportbytunid(tid)); 
#endif

		/* If the gateway is tunXX, modify it 
		 * to link#XX.
		 */
		if (gt[0] == 't'){
			strcpy(buf, getifnamebytunid(tid));
			buf[0] = buf[1] = buf[2] = ' ';     
			sprintf(gt, "link#%u", atoi(buf+3)+1);  
		}
#ifdef LINUX
		else {
			char tmp[10];
			sscanf(gt, "%u.%u.%s", (unsigned int *)tmp, (unsigned int *)tmp+1, buf);
			sprintf(gt, "1.0.%s", buf);
		}
#endif

		/* form a new entry */
		row = ExpStr->Add_row();
		column = 1;
		ExpStr->Insert_cell(row, column++, dst, "\t");
		ExpStr->Insert_cell(row, column++, gt, "\t\t");
#ifdef LINUX
		ExpStr->Insert_cell(row, column++, mask, "\t\t");
#endif
		ExpStr->Insert_cell(row, column++, flg, "\t");
#ifdef LINUX
		ExpStr->Insert_cell(row, column++, mss, "\t");
		ExpStr->Insert_cell(row, column++, win, "\t");
		//ExpStr->Insert_cell(row, column++, irtt, "\t");
#else
		ExpStr->Insert_cell(row, column++, ref, "\t");
		ExpStr->Insert_cell(row, column++, use, "\t");
#endif
		ExpStr->Insert_cell(row, column++, nxtif, "\n");
	}

	fclose(fd);
	system("rm temp.rt"); 
	EXPORT_GET_SUCCESS(ExpStr);
	return(1); 
}

