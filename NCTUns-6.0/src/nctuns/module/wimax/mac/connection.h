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

#ifndef __NCTUNS_WIMAX_CONNECTION_H__
#define __NCTUNS_WIMAX_CONNECTION_H__

#include <list>
#include <vector>

#include "management_message.h"
#include "structure.h"

class Packet;

typedef enum frgStat {

    frgNone,
    frgLast,
    frgFirst,
    frgMiddle

} fragment_state_t;

class Connection {

    protected:

    int CRCIndicator, pduqlen;
    std::list<const void*> PduQ;

    u_char Type;

    u_int16_t src_nid;
    u_int16_t dst_nid;
    u_int16_t FSN;
    frgStat   FC;

    void BuildGenericHeader(char* buf, int len);

    inline int GetHeaderLength() const
    {
        int hlen = sizeof(struct hdr_generic);

        if (Type & 0x20)	// Mesh Subheader
            hlen += 2;

        if (Type & 0x10)	// ARQ Feedback Payload
            ;

        if (Type & 0x08)	// Extended Type
            ;

        if (Type & 0x04)	// Fragmentation Subheader
            ;

        if (Type & 0x02)	// Packing Subheader
            ;

        if (Type & 0x01)	// Fragmentation Subheader
            ;

        return hlen;

    }

    public:
    u_int16_t DataCid;
    u_int32_t FlowId;

    Connection(int, bool);

    virtual ~ Connection() {

    };

    inline u_int16_t getCID() {

        return DataCid;

    }

    inline u_int32_t GetFID() {

        return FlowId;

    }

    inline void SetFID(u_int32_t flowId) {

        FlowId = flowId;

    }

    inline void SetSrcNodeID(u_int16_t pNid) {

        src_nid = pNid;

    }

    inline u_int16_t GetSrcNodeID() {

        return src_nid;

    }

    inline void SetDstNodeID(u_int16_t pNid) {

        dst_nid = pNid;

    }

    inline u_int16_t GetDstNodeID() {

        return dst_nid;

    }
    virtual bool Empty() {

        return PduQ.empty();

    }

    virtual int GetInformation(std::vector<int>*) const = 0;
    virtual int EncapsulateAll(char*, size_t) = 0;

};

class ManagementConnection: public Connection {

    public:
    ManagementConnection(int, bool = false);

    ~ManagementConnection() {

    };

    virtual void Insert(const ManagementMessage*);

    int GetInformation(std::vector<int>*) const;
    int EncapsulateAll(char *, size_t);

};

class BroadcastConnection:public ManagementConnection {

    ifmgmt *Slot[5];	//      DL_MAP, UL_MAP, DCD, UCD;

    public:
    BroadcastConnection(int);
    void Insert(const ManagementMessage*);
    bool Empty();

    int GetInformation(std::vector<int>*) const;
    int EncapsulateAll(char *, size_t);

};

class DataConnection : public Connection {

    protected:
    Packet* _qPkt;
    int     _qLen;

    public:

    DataConnection(int, bool mesh = false);
    DataConnection(int cid, u_int16_t peer_nid, bool mesh = false);

    ~DataConnection() {

    };

    size_t nf_pending_packet() const;

    void Insert(const Packet*);
    bool Empty();

    int GetInformation(std::vector<int>* info = NULL) const;
    virtual int EncapsulateAll(char*, size_t);
};

class DataConnectionEthernet : public DataConnection {

    public:

    DataConnectionEthernet(int, bool mesh = false);
    int EncapsulateAll(char *, size_t);

};

class ConnectionReceiver {

    private:

    u_int16_t _cid;
    u_int16_t _src_nid;
    u_int16_t _dst_nid;
    u_int16_t _fsn;

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

    ConnectionReceiver(u_int16_t pCid);
    ConnectionReceiver(u_int16_t pCid, u_int16_t src_nid, u_int16_t dst_nid);

    ~ConnectionReceiver();

    inline u_int16_t getCid() { return _cid; }
    inline u_int16_t getSrcNodeID() { return _src_nid;  }
    inline u_int16_t getDstNodeID() { return _dst_nid;  }

    void insert(struct hdr_generic *, int);
    char *getPacket(int &);

};

#endif          /* __NCTUNS_WIMAX_CONNECTION_H__ */
