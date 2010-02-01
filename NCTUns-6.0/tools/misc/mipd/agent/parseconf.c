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

#include "parseconf.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef MIP_MNSTATE
#define MIP_MNSTATE
typedef enum{ATHOME, FOREIGN, AWAY, NA} MnState;
#endif

/* #define DEBUG_PARSECONF */

int checktag(char *p, int *mask);

int 
parseConfig(int argc, char *argv[], int *port_divert,
		unsigned int *roEnable, struct sockaddr_in *bAddr, struct in_addr *coa, 
		MNLLIST *mn_ip_list, LLIST *all_if_list, LLIST *inner_if_list)
{
	FILE *fp;
	int  confFile = 0;
	long fpos = 0;
	int  tagmask  = 0;
	char tag[30];
	char token[30];


	/* if((!strcmp(argv[1], "-f")) && (argc == 5)){
		confFile = 1;
	}else if(!strcmp(argv[1], "-ha")){
		*isha = 1;
		confFile = 0;
	}else if(!strcmp(argv[1], "-fa")){
		*isfa = 1;
		confFile = 0;
	}else{
		printCmdUsage();
		exit(1);
	} */

	if(argc == 2){
		confFile = 1;
	}else{
		printCmdUsage();
		exit(1);
	}
	
	if(confFile){
		if((fp = fopen(argv[1], "r")) == NULL){
			fprintf(stderr, "[X] Open agent config file fail.\n");
			exit(1);
		}

		tagmask = gentagmask();
		/* enable RO or not coresponding to the cmdline parameter. */
		/* if(!strcmp(argv[3], "RO")){
			tagmask ^= T_RO;
			*roEnable = 1;
		}else if(!strcmp(argv[3], "NORO")){
			tagmask ^= T_RO;
			*roEnable = 0;
		} */

		/* set the port for divert socket. */
		/* *port_divert = atoi(argv[4]); */

#ifdef DEBUG_PARSECONF
		printf("tagmask: %d\n", tagmask); 
#endif

		while(fscanf(fp, "%s", tag) != EOF){
#ifdef DEBUG_PARSECONF
			printf("tag: %s ", tag); 
#endif
			switch (checktag(tag, &tagmask)){
				case T_RO:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
#ifdef DEBUG_PARSECONF
						printf("token: %s, ", token); 
#endif
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("RO: %s.\n", token);
#endif
						if(!strcmp(token, "DISABLE")){
							*roEnable = 0;
						}else if(!strcmp(token, "ENABLE")){
							*roEnable = 1;
						}
					}
					break;
				case T_ALLIP:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
#ifdef DEBUG_PARSECONF
						printf("token: %s, ", token); 
#endif
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("add %s to all_if_list.\n", token);
#endif
						addEntry(all_if_list, token);
					}
					break;
				case T_INNERIP:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("add %s to inner_if_list.\n", token);
#endif
						addEntry(inner_if_list, token);
					}
					break;
				case T_DPORT:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("DIVERT_PORT: %s.\n", token);
#endif
						*port_divert = atoi(token);
					}
					break;
				case T_COA:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("set %s to care of addr.\n", token);
#endif
						coa->s_addr = inet_addr(token);
					}
					break;
				/* case T_BCAST:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("set %s to broadcast addr.\n", token);
#endif
						bAddr->sin_addr.s_addr = inet_addr(token);
					}
					break; */
				case T_MN:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token) != EOF){
						if(isTag(token)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSECONF
						printf("add %s to mn_ip_list.\n", token);
#endif
						addMNEntry(mn_ip_list, token, "",  NA);
					}
					break;
				case T_ERR:
					printf("T_ERR--");
					printConfFileUsage();
					exit(1);
			}
		}

		/* check whether all necessary field is equipped. */
		if(tagmask != 0){
			printf("tagmask = %d != 0\n", tagmask);
			printConfFileUsage();
			exit(1);
		}

		if(fclose(fp) == EOF){
			fprintf(stderr, "[X] Close agent config file fail.\n");
			/* Though fail to close, but we continue to execute. */
		}

	}else{
	}

	return 0;
}

int
gentagmask()
{
	return (T_ALLIP | T_INNERIP | T_COA | T_RO | T_DPORT | T_MN);
}

/* determine which kind of tag, then clear the corresponding mask bit to
 * represent job of this tag is done before.*/
int
checktag(char *p, int *mask)
{
	if(!strcmp(p, "interfaces")){
		if(*mask & T_ALLIP){
			*mask ^= T_ALLIP;
			return T_ALLIP;
		}
		else 
			return T_ERR;

	}else if(!strcmp(p, "RO")){
		if(*mask & T_RO){
			*mask ^= T_RO;
			return T_RO;
		}
		else 
			return T_ERR;
	}else if(!strcmp(p, "COA")){
		if(*mask & T_COA){
			*mask ^= T_COA;
			return T_COA;
		}
		else 
			return T_ERR;
	}else if(!strcmp(p, "RO_DIVERT_PORT")){
		if(*mask & T_DPORT){
			*mask ^= T_DPORT;
			return T_DPORT;
		}
		else 
			return T_ERR;
	}else if(!strcmp(p, "wirelessInterfaces")){
		if(*mask & T_INNERIP){
			*mask ^= T_INNERIP;
			return T_INNERIP;
		}
		else 
			return T_ERR;
	/* }else if(!strcmp(p, "broadcast")){
		if(*mask & T_BCAST){
			*mask ^= T_BCAST;
			return T_BCAST;
		}
		else 
			return T_ERR; */
	}else if(!strcmp(p, "MN")){
		if(*mask & T_MN){
			*mask ^= T_MN;
			return T_MN;
		}
		else 
			return T_ERR;

	}else{
		return T_ERR;
	}
}

int
isTag(char *p)
{
	if(!strcmp(p, "interfaces") || !strcmp(p, "wirelessInterfaces") 
			        || !strcmp(p, "RO") 
			        || !strcmp(p, "COA") 
			        || !strcmp(p, "RO_DIVERT_PORT") 
			        || !strcmp(p, "broadcast") 
			        || !strcmp(p, "MN"))
		return 1;
	else
		return 0;
}

int 
printCmdUsage()
{
	printf("[MobileIP] agent Usage:\n");
	printf("\tagent confFile\n");
	/* printf("      -ha -o outerIP1 outerIP2 ... -i innerIP1 innerIP2 ...\n");
	printf("      -fa -o outerIP1 outerIP2 ... -i innerIP1 innerIP2 ...\n"); */

	return 0;
}
int 
printConfFileUsage()
{
	printf("MobileIP: agent config file format:\n");
	printf("\tRO (ENABLE or DISABLE)\n");
	printf("\tinterfaces ip1 ip2 ...\n");
	printf("\twirelessInterfaces iip1 iip2 ...\n");
	printf("\tCOA coa_IP\n");
	printf("\tDIVERT_PORT divert_port\n");
	printf("\tMN      mnip1 mnip2 ...  (if HA is specified)\n\n");
	printf("Exmaple: This router(with wired interfaces 1.0.3.1(COA), 1.0.2.1, wireless ones 1.0.4.1) has two MNs(1.0.4.2, 1.0.4.3), and divertPort 8501 is used.\n");
	printf("\tinterfaces 1.0.3.1 1.0.2.1 1.0.4.1\n");
	printf("\twirelessInterfaces 1.0.4.1\n");
	printf("\tCOA 1.0.3.1\n");
	printf("\tDIVERT_PORT 8401\n");
	printf("\tMN      1.0.4.2 1.0.4.3\n");

	return 0;
}
