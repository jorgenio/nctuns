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

#ifndef __NCTUNS_80216J_NT_CONNECTION_H__
#define __NCTUNS_80216J_NT_CONNECTION_H__

#include <list>
#include <vector>
#include <packet.h>
#include "structure.h"
#include "library.h"
#include "management_message.h"

#define NumBroadcast 9 
#define MaxCID 0xFFFF

using namespace std;
using namespace MR_ManageMsg_NT;

class Packet;

typedef enum frgStat {
	frgNone,
	frgLast,
	frgFirst,
	frgMiddle
} fragment_state_t;

namespace MR_Connection_NT {
	class BR_Connection {
        public:
            int pduqlen;
            list <const BR_HDR *> pduQ;

        public:
            uint16_t cid;

            BR_Connection(uint16_t);
            ~BR_Connection();
            
			inline uint16_t getCID()
			{
				return cid;
			}

            void Insert(const BR_HDR *); 
            int GetInformation(vector<int> *);
            int GetInformation();
    };

    class Connection {
		protected:
			int crcIndicator;
			int pduqlen;
			list <const void *> pduQ;

			uint8_t type; // Header Type
			uint16_t src_nid;
			uint16_t dst_nid;
			uint16_t fsn;
			frgStat   fc;

			void BuildGenericHeader(char* buf, int len);

			inline int GetHeaderLength() const
			{
				int hlen = sizeof(struct hdr_generic);

				if (type & 0x20)    // Mesh Subheader
					hlen += 2;

				if (type & 0x10)    // ARQ Feedback Payload
					;

				if (type & 0x08)    // Extended Type
					;

				if (type & 0x04)    // Fragmentation Subheader
					;

				if (type & 0x02)    // Packing Subheader
					;

				if (type & 0x01)    // Fragmentation Subheader
					;

				return hlen;
			}

		public:
			uint16_t cid;
			uint32_t flowId;

			Connection(int);

			virtual ~Connection()
			{
				;
			}

			inline uint16_t getCID()
			{
				return cid;
			}

			inline uint32_t GetFID()
			{
				return flowId;
			}

			inline void SetFID(uint32_t id)
			{
				flowId = id;
			}

			inline void SetSrcNodeID(uint16_t pNid)
			{
				src_nid = pNid;
			}

			inline uint16_t GetSrcNodeID()
			{
				return src_nid;
			}

			inline void SetDstNodeID(uint16_t pNid)
			{
				dst_nid = pNid;
			}

			inline uint16_t GetDstNodeID()
			{
				return dst_nid;
			}

			virtual bool Empty()
			{
				return pduQ.empty();
			}


			virtual int GetInformation(vector<int>*) const = 0;
			virtual int EncapsulateAll(char*, size_t) = 0;
	};

	class ManagementConnection: public Connection {
		public:
			ManagementConnection(int);
			~ManagementConnection();

			virtual void Insert(const ManagementMessage*);
			int GetInformation(vector<int>*) const;
			int EncapsulateAll(char *, size_t);
	};

	class BroadcastConnection:public ManagementConnection {
		private:
			ifmgmt *ifmmSlot[NumBroadcast];

		public:
			BroadcastConnection(int);
			~BroadcastConnection();
			void Insert(const ManagementMessage*);
			bool Empty();
			int GetInformation(vector<int>*) const;
			int EncapsulateAll(char *, size_t);
	};

	class DataConnection : public Connection {
		protected:
			Packet* _qPkt;
			int     _qLen;

		public:
			DataConnection(int);
			DataConnection(int cid, uint16_t peer_nid);

			~DataConnection();

			size_t nf_pending_packet() const;

			void Insert(const Packet*);
			bool Empty();

			int GetInformation(vector<int>* info = NULL) const;
			virtual int EncapsulateAll(char*, size_t);
	};

	class DataConnectionEthernet : public DataConnection {
		public:
			DataConnectionEthernet(int);
			int EncapsulateAll(char *, size_t);
	};

	class ConnectionReceiver {
		private:
			uint16_t _cid;
			uint16_t _src_nid;
			uint16_t _dst_nid;
			uint16_t _fsn;

			int _len, _maxlen;

			char *_buffer;
			char *_external;

			enum {
				NoData,
				NotComplete,
				Complete,
				External
			} _state;

			bool _attrARQ;
			void drop();

		public:
			ConnectionReceiver(uint16_t pCid);
			ConnectionReceiver(uint16_t pCid, uint16_t src_nid, uint16_t dst_nid);

			~ConnectionReceiver();

			inline uint16_t getCid() { return _cid; }
			inline uint16_t getSrcNodeID() { return _src_nid;  }
			inline uint16_t getDstNodeID() { return _dst_nid;  }

			void insert(struct hdr_generic *, int);
			char *getPacket(int &);
	};
}

#endif          /* __NCTUNS_80216J_NT_CONNECTION_H__ */
