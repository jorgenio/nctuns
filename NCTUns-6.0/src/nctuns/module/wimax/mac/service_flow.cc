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
#include <vector>
#include <netinet/ip.h>

#include "service_flow.h"

#define VERBOSE_LEVEL   MSG_INFO
#include "verbose.h"

using namespace std;

SfTableReader::SfTableReader(FILE * fp, ProvisionedSfTable * sftable)
{
	char buffer[128], *strlist[10];
	int state, argc;
	ServiceFlow *newsf;

	while (fgets(buffer, 128, fp)) {
		argc = parseLine(strlist, 10, buffer, " \t=\n");

		if (argc == 0)
			continue;

		else if (strcmp(strlist[0], "Entry") == 0) {
			state = 1;	// new entry
			newsf = new ServiceFlow();
		} else if (strcmp(strlist[0], "Service-Flow-ID") == 0
			   && argc == 2) {
			newsf->sfindex = atoi(strlist[1]);
		} else if (strcmp(strlist[0], "SS-MAC") == 0 && argc == 2) {
			newsf->SetMac(strlist[1]);
			printf("[DEBUG] %s %s\n", newsf->ssmac,
			       strlist[1]);
		} else if (strcmp(strlist[0], "Direction") == 0
			   && argc == 2) {
			newsf->direction = strlist[1][0];
		} else if (strcmp(strlist[0], "QoS-Index") == 0
			   && argc == 2) {
			newsf->qosindex = atoi(strlist[1]);
		} else if (strcmp(strlist[0], "Service-Flow-State") == 0
			   && argc == 2) {
/*
			Not Implemented
*/
		} else if (argc == 2 && strcmp(strlist[0], "End") == 0
			   && strcmp(strlist[1], "Entry") == 0) {
			if (state) {
				printf("Add New Entry\n");
				sftable->AddItem(newsf);
				newsf = NULL;
			} else {
				printf
				    ("%s#%d: Error: missing Entry in config\n",
				     __FILE__, __LINE__);
			}
			state = 0;
		} else if (argc == 2 && strcmp(strlist[0], "End") == 0
			   && strcmp(strlist[1], "Table") == 0) {
			break;
		}
	}
}

SfClassTableReader::SfClassTableReader(FILE * fp,
				       ServiceClassTable * sctable)
{
	char buffer[128], *strlist[10];
	int state, argc;
	ServiceClass *newsc;

	while (fgets(buffer, 128, fp)) {
		argc = parseLine(strlist, 10, buffer, " \t=\n");

		if (argc == 0)
			continue;

		else if (strcmp(strlist[0], "Entry") == 0) {
			state = 1;	// new entry
			newsc = new ServiceClass();
		}
        else if (strcmp(strlist[0], "QoS-Index") == 0
			   && argc == 2) {
			newsc->QoSIndex = atoi(strlist[1]);
		}
        else if (strcmp(strlist[0], "Class-Name") == 0
			   && argc == 2) {
			strncpy(newsc->ClassName, strlist[1], 16);
		}
        else if (strcmp(strlist[0], "Priority") == 0
			   && argc == 2) {
			newsc->Priority = atoi(strlist[1]);
		}
        else if (strcmp(strlist[0], "Max-Sustained-Rate") == 0
			   && argc == 2) {

            u_int32_t sustained_rate_in_kbps = atoi(strlist[1]);
            newsc->MaxSustainedRate =
                static_cast<u_int32_t> (sustained_rate_in_kbps*1024);
		}
        else if (strcmp(strlist[0], "Max-Traffic-Burst") == 0
			   && argc == 2) {
		}
        else if (argc == 2 && strcmp(strlist[0], "End") == 0
			   && strcmp(strlist[1], "Entry") == 0) {
			
            if (state) {
				printf("Add New Entry\n");
				sctable->AddItem(newsc);
				newsc = NULL;
			}
            else {
				printf
				    ("%s#%d: Error: missing Entry in config\n",
				     __FILE__, __LINE__);
			}
			state = 0;
		}
        else if (argc == 2 && strcmp(strlist[0], "End") == 0
			   && strcmp(strlist[1], "Table") == 0) {
			break;
		}
	}
}

ClassifierRuleTableReader::ClassifierRuleTableReader(
        FILE* fp, ClassifierRuleTable* crtable)
{
	_fp = fp;
	while (hasNext()) {
		crtable->AddItem(getNext());
	}
}

ClassifierRuleTableReader::ClassifierRuleTableReader(FILE * fp)
{
	_fp = fp;
	_entity = NULL;
}

bool ClassifierRuleTableReader::hasNext()
{
	char buffer[128], *strlist[10];
	int state, argc;

	if (!_fp)
		return false;

	while (fgets(buffer, 128, _fp)) {
		argc = parseLine(strlist, 10, buffer, " \t=\n");

		if (argc == 0)
			continue;

		else if (strcmp(strlist[0], "Entry") == 0) {
			state = 1;	// new entry
			_entity = new ClassifierRule();
		} else if (strcmp(strlist[0], "Service-flow-ID") == 0
			   && argc == 2) {
			_entity->Sfid = atoi(strlist[1]);
		} else if (strcmp(strlist[0], "Rule-Index") == 0
			   && argc == 2) {
			_entity->RuleIndex = atoi(strlist[1]);
		} else if (strcmp(strlist[0], "Priority") == 0
			   && argc == 2) {
			_entity->f.Priority = atoi(strlist[1]);
		} else if (strcmp(strlist[0], "IP-ToS-Low") == 0
			   && argc == 2) {
		} else if (strcmp(strlist[0], "Inet-Dst-Addr") == 0
			   && argc == 2) {
			_entity->f.InetDstAddr.s_addr =
			    inet_addr(strlist[1]);
		} else if (strcmp(strlist[0], "Inet-Dst-Mask") == 0
			   && argc == 2) {
			_entity->f.InetDstMask.s_addr =
			    inet_addr(strlist[1]);
		} else if (argc == 2 && strcmp(strlist[0], "End") == 0
			   && strcmp(strlist[1], "Entry") == 0) {
			if (state) {
				return true;
			} else {
				printf
				    ("%s#%d: Error: missing Entry in config\n",
				     __FILE__, __LINE__);
			}
			state = 0;
		} else if (argc == 2 && strcmp(strlist[0], "End") == 0
			   && strcmp(strlist[1], "Table") == 0) {
			break;
		}
	}
	_fp = NULL;
	return false;
}

ClassifierRule::ClassifierRule()
{
    memset(&f, 0, sizeof(f));
}

ClassifierRule::~ClassifierRule()
{
}

ClassifierRule *ClassifierRuleTableReader::getNext()
{
	return _entity;
}

int ClassifierRule::Match(struct ip *iphdr)
{

	if ((f.InetDstAddr.s_addr & f.InetDstMask.s_addr) ==
	    (iphdr->ip_dst.s_addr & f.InetDstMask.s_addr))
		return 1;
	return 0;

}

int ClassifierRule::MatchDst(u_int32_t gw)
{
	if ((f.InetDstAddr.s_addr & f.InetDstMask.s_addr) ==
	    (gw & f.InetDstMask.s_addr))
		return 1;
	return 0;
}

ClassifierRuleTable::ClassifierRuleTable()
{
}

ClassifierRuleTable::~ClassifierRuleTable()
{
	for (vector<ClassifierRule*>::iterator it = List.begin();
            it != List.end(); it++)
			delete *it;
    List.clear();
}

ClassifierRule*
ClassifierRuleTable::Find(ether_header* etherhdr, ip* iphdr)
{
	vector < ClassifierRule * >::iterator iter;

	for (iter = List.begin(); iter != List.end(); iter++) {
		if ((*iter)->Match(iphdr)) {
			return *iter;
		}
	}
	return NULL;
}

ClassifierRule*
ClassifierRuleTable::Find(ip* iphdr)
{
	vector < ClassifierRule * >::iterator iter;

	for (iter = List.begin(); iter != List.end(); iter++) {
		if ((*iter)->Match(iphdr)) {
			return *iter;
		}
	}
	return NULL;
}

ClassifierRule*
ClassifierRuleTable::Find(uint32_t gw)
{
	vector < ClassifierRule * >::iterator iter;
    
    if (List.empty()) {
    
        FATAL("There is no classification rule.\n");
    
    }
    else {
    
        DEBUG("CrTable size = %u.\n", List.size());
    
    }

    fflush(stdout);
	for (iter = List.begin(); iter != List.end(); iter++) {
		if ((*iter)->MatchDst(gw)) {
			return *iter;
		}
	}
	return NULL;
}

void
ClassifierRuleTable::Dump()
{
    std::vector<ClassifierRule*>::iterator iter;
    for (iter = List.begin(); iter != List.end(); iter++) {
        printf("-------------------\n");
        (*iter)->Dump();
    }
}
