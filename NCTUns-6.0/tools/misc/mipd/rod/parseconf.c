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

int checktag(char *p, int *mask);

int 
parseConfig(int argc, char *argv[], int *port_ro_divert, BLLIST *mn_bindlist, LLIST *if_list)
{
	FILE *fp;
	int  confFile = 0;
	long fpos = 0;
	int  tagmask  = 0;
	char tag[30]; /* IFIP / MN */
	char token1[30];


	/* if(argc == 3){
		if(!strcmp(argv[1], "-f"))
			confFile = 1;
		else{
			printCmdUsage();
			exit(1);
		}
	}else{
	} */

	if(argc == 2){
		confFile = 1;
	}else{
		printCmdUsage();
		exit(1);
	} 
	/* if((!strcmp(argv[1], "-f")) && (argc == 3)){
		confFile = 1;
	}else{
		printCmdUsage();
		exit(1);
	} */
	
	if(confFile){
		if((fp = fopen(argv[1], "r")) == NULL){
			fprintf(stderr, "[X] Open agent config file fail.\n");
			exit(1);
		}

		tagmask = T_IFIP | T_RO_DPORT | T_RO;

		/* printf("tagmask: %d\n", tagmask); */

		while(fscanf(fp, "%s", tag) != EOF){
/* #undef DEBUG_PARSE */
#ifdef DEBUG_PARSE
			printf("tag: %s ", tag); 
#endif
			switch (checktag(tag, &tagmask)){
				/* case T_MN:
					fpos = ftell(fp);
					while(fscanf(fp, "%s%s", token1, token2) != EOF){
#ifdef DEBUG_PARSE
						printf("token1: %s, token2: %s,", token1, token2);
#endif
						if(isTag(token1)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
						addBEntry(mn_bindlist, token1, token2);
					}
					break; */
				case T_IFIP:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token1) != EOF){
#ifdef DEBUG_PARSE
						printf("token: %s ", token1); 
#endif
						if(isTag(token1)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
						addEntry(if_list, token1);
					}
					break;
				case T_RO:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token1) != EOF){
						if(isTag(token1)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSE
						printf("RO: %s ", token1); 
#endif
					}
					break;
				case T_RO_DPORT:
					fpos = ftell(fp);
					while(fscanf(fp, "%s", token1) != EOF){
						if(isTag(token1)){
							fseek(fp, fpos, SEEK_SET);
							break;
						}
						fpos = ftell(fp);
#ifdef DEBUG_PARSE
						printf("ROD,DIVERT_PORT: %s ", token1); 
#endif

						*port_ro_divert = atoi(token1);
					}
					break;
				case T_ERR:
					printf("T_ERR--");
					printConfFileUsage();
					exit(1);
			}
		}
#ifdef DEBUG_PARSE
printf("rod: after parsing, if_list entry(%d)\n", if_list->no);
#endif

		/* check whether all necessary field is equipped. */
		if(tagmask != 0){
			printf("tagmask = %d, != 0\n", tagmask);
			printConfFileUsage();
			exit(1);
		}

		if(fclose(fp) == EOF){
			fprintf(stderr, "[X] Close agent config file fail.\n");
			/* Though fail to close, but we continue to execute. */
		}

	}else{  /* from command line argument. */
	}

	/* fprintf(stderr, "\nifList(%p): ", if_list);
	printList(if_list);
	fprintf(stderr, "\nMNIPList(%p): ", mn_bindlist);
	printBList(mn_bindlist); */

	return 0;
}

/* determine which kind of tag, then clear the corresponding mask bit to
 * represent job of this tag is done before.*/
int
checktag(char *p, int *mask)
{
	if(!strcmp(p, "interfaces")){
		if(*mask & T_IFIP){
			*mask ^= T_IFIP;
			return T_IFIP;
		}
		else 
			return T_ERR;

	}else if(!strcmp(p, "RO_DIVERT_PORT")){
		if(*mask & T_RO_DPORT){
			*mask ^= T_RO_DPORT;
			return T_RO_DPORT;
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

	/* }else if(!strcmp(p, "MN")){
		return T_MN; */

	}else{
		return T_ERR;
	}
}

int
isTag(char *p)
{
	/* if(!strcmp(p, "interfaces") || !strcmp(p, "RO_DIVERT_PORT") || !strcmp(p, "MN")) */
	if(!strcmp(p, "interfaces") || !strcmp(p, "RO_DIVERT_PORT") 
			            || !strcmp(p, "RO"))
		return 1;
	else
		return 0;
}

int 
printCmdUsage()
{
	printf("[MobileIP] rod Usage:\n");
	printf("\trod confFile\n");

	return 0;
}

int 
printConfFileUsage()
{
	printf("MobileIP: rod config file format:\n");
	printf("\tinterfaces ifip\n");
	printf("\tRO_DIVERT_PORT portNo\n");
	printf("\tRO (ENABLE or DISABLE)\n\n");
	printf("Exmaple: The corresponding host with IP 1.0.6.2 enables the RO, the DivertPort is 8500.\n");
	printf("\tinterfaces 1.0.6.2\n");
	printf("\tRO_DIVERT_PORT 8500\n");
	printf("\tRO ENABLE\n");

	return 0;
}
