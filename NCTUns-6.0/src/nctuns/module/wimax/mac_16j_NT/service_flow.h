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

#ifndef __NCTUNS_80216J_NT_SERVICE_FLOW_H__
#define __NCTUNS_80216J_NT_SERVICE_FLOW_H__

#include <vector>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ethernet.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

using namespace std;

namespace MR_ServiceFlow_NT {
	class ServiceFlow {
		public:
			int     sfindex;
			char    ssmac[6];
			char    direction;
			int     qosindex;
			char    sfstate;
			int     cid;

			ServiceFlow()
			{
				memset(this, 0, sizeof(ServiceFlow));
				cid = -1;
			}

			inline void SetMac(char *mac)
			{
				int imac[6];

				sscanf(mac, "%x:%x:%x:%x:%x:%x", imac, imac + 1, imac + 2, imac + 3, imac + 4, imac + 5);
				for (int i = 0; i < 6; i++)
				{
					ssmac[i] = imac[i] & 0xff;
				}
			}

			inline void SetConnectionID(int _cid)
			{
				cid = _cid;
			}

			inline int Match(int sfid)
			{
				return (sfindex == sfid);
			}

			inline int Match(uint8_t mac[6])
			{
				//printf("key mac: %02x:%02x:%02x:%02x:%02x:%02x\n", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
				return (memcmp(mac, ssmac, 6) == 0) ? 1 : 0;
			}

			inline int GetSfindex()
			{
				return sfindex;
			}

			inline int GetQosIndex()
			{
				return qosindex;
			}

			inline void Dump()
			{
				printf("sfindex: %u\n", sfindex);
				printf("ssmac: %02x:%02x:%02x:%02x:%02x:%02x\n", ssmac[0], ssmac[1], ssmac[2], ssmac[3], ssmac[4], ssmac[5]);
				printf("direction: %c\n", direction);
				printf("qosindex: %d\n", qosindex);
				printf("sfstate: %d\n", sfstate);
				printf("ConnectionID: %d\n", cid);
			}
	};

	class ServiceClass {
		public:
			uint32_t   QoSIndex;
			char        ClassName[16];
			char        Priority;
			int         MaxSustainedRate;
			int         MaxTrafficBurst;
			int         MinReservedRate;
			int         ToleratedJitter;
			int         MaxLatency;
			char        FixedorVariableSDU;
			int         SDUSize;
			int         SchedulingType;
			int         FragmentLen;
			int         MinRsvTolerableRate;

			ServiceClass()
			{
				memset(this, 0, sizeof(ServiceClass));
			}

			inline int Match(uint32_t qosIndex)
			{
				return (QoSIndex == qosIndex);
			}

			inline void Dump()
			{
				printf("QoSIndex: %u\n", QoSIndex);
				printf("ClassName: %s\n", ClassName);
				printf("Priority: %d\n", Priority);
				printf("MaxSustainedRate: %d\n", MaxSustainedRate);
				printf("MaxTrafficBurst: %d\n", MaxTrafficBurst);
				printf("MinReservedRate: %d\n", MinReservedRate);
				printf("ToleratedJitter: %d\n", ToleratedJitter);
				printf("MaxLatency: %d\n", MaxLatency);
			}
	};

	class ClassifierRule {
		public:
			int Sfid;
			int RuleIndex;
			int nextHop;

			struct {

				int Priority;
				int IPToSLow;
				int IPToSHigh;
				int IPToSMask;
				int IPProto;
				int InetAddrType;
				int InetSrcAddr;
				int InetSrcMask;
				struct in_addr InetDstAddr;
				struct in_addr InetDstMask;
				int InetSrcPortStart;
				int InetSrcPortEnd;
				int InetDstPortStart;
				int InetDstPortEnd;
				int InetDstMacAddr;
				int InetDstMacMask;
				int InetSrcMacAddr;
				int InetSrcMacMask;
				int EnetProtoType;
				int EnetProto;
				int RuleState;
			} f;

			explicit ClassifierRule();
			virtual ~ClassifierRule();

			int Match(struct ip *);
			int MatchDst(uint32_t);

			inline void Dump()
			{
				printf("Service-flow-ID: %d\n", Sfid);
				printf("Rule-Index: %d\n", RuleIndex);
				printf("Priority: %d\n", f.Priority);
				printf("IP-ToS-Low: %d\n", f.IPToSLow);
				printf("IP-ToS-High: %d\n", f.IPToSHigh);
				printf("IP-ToS-Mask: %d\n", f.IPToSMask);
				printf("IP-Proto: %d\n", f.IPProto);
				printf("Inet-Addr-Type: %d\n", f.InetAddrType);
				printf("Inet-Src-Addr: %d\n", f.InetSrcAddr);
				printf("Inet-Src-Mask: %d\n", f.InetSrcMask);
				printf("Inet-Dst-Addr: %s\n", inet_ntoa(f.InetDstAddr));
				printf("Inet-Dst-Mask: %s\n", inet_ntoa(f.InetDstMask));
				printf("Inet-Src-Port-Start: %d\n", f.InetSrcPortStart);
				printf("Inet-Src-Port-End: %d\n", f.InetSrcPortEnd);
				printf("Inet-Dst-Port-Start: %d\n", f.InetDstPortStart);
				printf("Inet-Dst-Port-End: %d\n", f.InetDstPortEnd);
				printf("Inet-Dst-Mac-Addr: %d\n", f.InetDstMacAddr);
				printf("Inet-Dst-Mac-Mask: %d\n", f.InetDstMacMask);
				printf("Inet-Src-Mac-Addr: %d\n", f.InetSrcMacAddr);
				printf("Inet-Src-Mac-Mask: %d\n", f.InetSrcMacMask);
				printf("Enet-Proto-Type: %d\n", f.EnetProtoType);
				printf("Enet-Proto: %d\n", f.EnetProto);
				printf("Rule-State: %d\n", f.RuleState);
			}
	};

	class ProvisionedSfTable {
		private:
			vector<ServiceFlow *> List;

		public:
			inline void AddItem(ServiceFlow *sf)
			{
				List.push_back(sf);
			}

			inline ServiceFlow *GetServiceFlow(uint32_t sfindex)
			{
				vector<ServiceFlow *>::iterator iter;

				for (iter = List.begin(); iter != List.end(); iter++)
				{
					if ((*iter)->Match(sfindex))
					{
						return *iter;
					}
				}
				return NULL;
			}

			inline ServiceFlow *GetServiceFlow(uint8_t mac[6])
			{
				vector<ServiceFlow *>::iterator iter;

				for (iter = List.begin(); iter != List.end(); iter++)
				{
					if ((*iter)->Match(mac))
					{
						return *iter;
					}
				}
				return NULL;
			}

			inline void Dump()
			{
				vector<ServiceFlow *>::iterator iter;
				for (iter = List.begin(); iter != List.end(); iter++)
				{
					printf("-------------------\n");
					(*iter)->Dump();
				}
			}
	};

	class ServiceClassTable {
		private:
			vector<ServiceClass *> sclist;

		public:
			inline void AddItem(ServiceClass *sc)
			{
				sclist.push_back(sc);
			}

			inline ServiceClass *GetServiceClass(uint32_t qosIndex)
			{
				vector<ServiceClass *>::iterator iter;
				for (iter = sclist.begin(); iter != sclist.end(); iter++)
				{
					if ((*iter)->Match(qosIndex))
					{
						return *iter;
					}
				}
				return NULL;
			}

			inline void Dump()
			{
				vector<ServiceClass *>::iterator iter;
				for (iter = sclist.begin(); iter != sclist.end(); iter++)
				{
					printf("-------------------\n");
					(*iter)->Dump();
				}
			}
	};

	//struct ether_header;

	class ClassifierRuleTable {
		private: 
			vector<ClassifierRule *> List;

		public:
			explicit ClassifierRuleTable();
			virtual ~ClassifierRuleTable();

			ClassifierRule* Find(struct ether_header *, ip *);
			ClassifierRule* Find(ip *iphdr);
			ClassifierRule* Find(uint32_t);
			void Dump();

			inline void AddItem(ClassifierRule *rl)
			{
				List.push_back(rl);
			}

			inline uint32_t get_table_size()
			{
				return List.size();
			}
	};

	class TableReader {
		public:
			inline int parseLine(char **argv, int size, char *inputstr, const char *sep)
			{
				int i = 0;
				while (i < size)
				{
					argv[i] = strsep(&inputstr, sep);
					if (argv[i] == NULL)
						break;
					if (argv[i][0] == '#')
						break;
					if (argv[i][0] == 0)
						continue;
					i++;
				}
				return i;
			}
	};

	class SfTableReader: public TableReader {
		public:
			SfTableReader(FILE *fp, ProvisionedSfTable *sftable);
	};

	class SfClassTableReader: public TableReader {
		public:
			SfClassTableReader(FILE *fp, ServiceClassTable *sctable);
	};

	class ClassifierRuleTableReader: public TableReader {
		private:
			ClassifierRule *_entity;
			FILE *_fp;

		public:
			ClassifierRuleTableReader(FILE *fp, ClassifierRuleTable *crtable);
			ClassifierRuleTableReader(FILE *fp);
			bool hasNext();
			ClassifierRule *getNext();
	};
}

#endif                /* __NCTUNS_80216J_NT_SERVICE_FLOW_H__ */
