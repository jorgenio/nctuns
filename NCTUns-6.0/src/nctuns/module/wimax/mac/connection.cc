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

#include <packet.h>
#include "structure.h"
#include "library.h"
#include "connection.h"
#include "management_message.h"

using namespace std;

Connection::Connection(int cid, bool mesh) {

    DataCid         = cid;
    CRCIndicator    = 1;
    pduqlen         = 0;
    
    if (mesh)
        Type = 0x20;
    else
        Type = 0x00;
        
    src_nid = 0;
    dst_nid = 0;
    FSN     = 0;
    FC      = frgNone;
}

/*
* Build generic MAC header.
* Note that we do not implement encryption and HCS.
*/

void Connection::BuildGenericHeader(char *buf, int len) {

    int crc, offset;
    struct hdr_generic *hg = (struct hdr_generic *) buf;

    hg->ht      = 0;
    hg->ec      = 0;        // Not implemented
    hg->type    = Type;
    
    if (FC != frgNone)
        hg->type |= 0x04;
        
    hg->rsv1    = 0;        // Reserved
    hg->ci      = CRCIndicator;
    hg->eks     = 0;        // Not implemented
    hg->rsv2    = 0;        // Reserved
    hg->len_msb = len / 256;
    hg->len_lsb = len % 256;
    hg->cid_msb = DataCid / 256;;
    hg->cid_lsb = DataCid % 256;
    hg->hcs = hcs(reinterpret_cast <char*>(hg),sizeof(struct hdr_generic) - 1);

    offset = sizeof(hdr_generic);
    
    if (hg->type & 0x20) {
        
        //memcpy(buf + offset, &NodeID, 2);
        memcpy(buf + offset, &src_nid, 2);
        offset += 2;

    }

    if (hg->type & 0x04) {

        struct subhdr_fragment* p = reinterpret_cast<struct subhdr_fragment*> (buf+offset);
        p->fc   = FC;
        p->fsn  = FSN;
        p->rsv  = 0;
        offset  += 1;

    }

    if (CRCIndicator) {
        
        crc = crc32(0, (char *) buf, len - 4);
        memcpy((char *) buf + len - 4, &crc, 4);

    }
    
    FSN = (FSN + 1) % 8;

    if (FC == frgLast)
        FC = frgNone;
}

ManagementConnection::ManagementConnection(int cid, bool mesh)
: Connection(cid, mesh)
{
}

void
ManagementConnection::Insert(const ManagementMessage* pdu)
{
    PduQ.push_back(pdu);
}

int
ManagementConnection::GetInformation(vector<int>* info) const
{
    int totallen    = 0;
    int len         = 0;
    
    const ManagementMessage* mmsg = NULL;

    for (list<const void*>::const_iterator it = PduQ.begin();
	    it != PduQ.end(); it++) {

        mmsg = static_cast<const ManagementMessage*>(*it);
        len = GetHeaderLength() + mmsg->getLen();

        if (CRCIndicator)	// Padding CRC
            len += 4;

        if (info)
            info->push_back(len);

        totallen += len;
        //printf("\t(%d) SS msg: %d bytes ", getCID(), len);
    }

    return totallen;

}

int ManagementConnection::EncapsulateAll(char *dstbuf, size_t maxlen) {

    u_int sendlen = 0, len = 0, hlen;
    const ManagementMessage* mmsg;
    
    while (sendlen < maxlen && !PduQ.empty()) {
        
        mmsg = static_cast<const ManagementMessage*>(PduQ.front());

        hlen = GetHeaderLength();
        len = hlen + mmsg->getLen();
        
        if (CRCIndicator)
            len += 4;

        if (sendlen + len > maxlen)
            break;

        PduQ.pop_front();
        mmsg->copyTo((u_char *) dstbuf + sendlen + hlen);
        BuildGenericHeader(dstbuf + sendlen, len);
        delete mmsg;
        sendlen += len;
    }
    
    return sendlen;

}

BroadcastConnection::BroadcastConnection(int cid):
    ManagementConnection(cid, false) {

    Slot[0] = Slot[1] = Slot[2] = Slot[3] = Slot[4] = NULL;

}

void
BroadcastConnection::Insert(const ManagementMessage* pdu)
{
    ifmgmt *ifmm = (ifmgmt *) pdu;
    static int index[5] =
        { MG_DLMAP, MG_ULMAP, MG_DCD, MG_UCD, MG_CLKCMP };

    for (int i = 0; i < 5; i++) {
        if (ifmm->getType() == index[i]) {
            if (Slot[i])
                delete Slot[i];
            Slot[i] = ifmm;
            break;
        }
    }
}

bool BroadcastConnection::Empty()
{
    return !(Slot[0] || Slot[1] || Slot[2] || Slot[3] || Slot[4]);
}

int
BroadcastConnection::GetInformation(vector<int>* info) const
{
    ifmgmt *ifmm;
    int i, totallen = 0, len = 0;

    for (i = 0; i < 5; i++) {
        ifmm = (ifmgmt *) Slot[i];
        if (ifmm) {
            
            len = GetHeaderLength() + ifmm->getLen();
            
            if (CRCIndicator)
                len += 4;
            info->push_back(len);
            totallen += len;
//                      printf("\t(%d) Broadcast msg: %d bytes\n", i, len);
        }
    }
    
    return totallen;
}

int BroadcastConnection::EncapsulateAll(char *dstbuf, size_t maxlen)
{
    u_int len = 0, sendlen = 0, hlen, i;
    ifmgmt *ifmm;

    for (i = 0; i < 5; i++) {
        ifmm = Slot[i];
        if (ifmm) {
            hlen = GetHeaderLength();
            len = hlen + ifmm->getLen();
            if (CRCIndicator)
                len += 4;
            if (sendlen + len > maxlen)
                break;

            Slot[i] = NULL;
            ifmm->copyTo((u_char *) (dstbuf + sendlen + hlen));
            BuildGenericHeader(dstbuf + sendlen, len);

            delete ifmm;
            sendlen += len;
        }
    }

    return sendlen;
}

/*
 *	Data Connection Class
 */
DataConnection::DataConnection(int cid, bool mesh)
    :Connection(cid, mesh) {
    
    _qPkt = NULL;
    _qLen = 0;

}

DataConnection::DataConnection(int cid, u_int16_t peer_nid ,bool mesh)
    :Connection(cid, mesh) {
    
    _qPkt = NULL;
    _qLen = 0;
    SetDstNodeID(peer_nid);

}

void
DataConnection::Insert(const Packet* pdu)
{
    pduqlen++;
    PduQ.push_back(pdu);
}

size_t
DataConnection::nf_pending_packet() const
{
    return PduQ.size();
}

bool
DataConnection::Empty()
{
    if (_qPkt)
        return false;
    
    return PduQ.empty();
}

int
DataConnection::GetInformation(vector<int>* info) const
{
    int totallen = 0;
    int padding;

    if (_qPkt) {
        int hlen = GetHeaderLength() + 1;
        int len = _qPkt->pkt_getlen() - _qLen;
        if (CRCIndicator)
            padding = 4;
        else
            padding = 0;

        if (info)
            info->push_back(hlen + len + padding);

        totallen = hlen + len + padding;
        //printf("\tData: (%d, %d, %d) bytes of %d qLen=%d  ", hlen, len, padding, _qPkt->pkt_getlen()+11, _qLen);
    }
    for (list<const void*>::const_iterator it = PduQ.begin();
	    it != PduQ.end(); it++) {

        const Packet* pkt = (Packet*)*it;
        int hlen = GetHeaderLength();
        int len = pkt->pkt_getlen();
        if (CRCIndicator)
            padding = 4;
        else
            padding = 0;

        if (info)
            info->push_back(hlen + len + padding);

        totallen += hlen + len + padding;
    }
    //if( totallen )
    //      printf("Total: %d bytes (%d packets) CID(%d)\n", totallen, pduqlen, DataCid);
    return totallen;
}

int
DataConnection::EncapsulateAll(char* dstbuf, size_t maxlen)
{
    u_int padding = 0;
    if (CRCIndicator)
        padding = 4;

    u_int sendlen = 0;
    if (_qPkt) {
        u_int hlen = GetHeaderLength() + 1;
        if (hlen + padding >= maxlen)	// No space enough to send any pdu
            return 0;

        u_int pdulen = hlen + _qPkt->pkt_getlen() - _qLen + padding;
        if (pdulen <= maxlen) {
            FC = frgLast;	// last fragment
            memcpy(dstbuf + hlen, _qPkt->pkt_sget() + _qLen,
                pdulen - hlen - padding);

            delete _qPkt;
            _qPkt = NULL;
            _qLen = 0;
        } else {
            pdulen = maxlen;
            FC = frgMiddle;	// middle fragment

            memcpy(dstbuf + hlen, _qPkt->pkt_sget() + _qLen,
                pdulen - hlen - padding);
            _qLen += pdulen - hlen - padding;
        }
        BuildGenericHeader(dstbuf, pdulen);
        sendlen = pdulen;
    }

    char* ptr = dstbuf + sendlen;
    while (!_qPkt && !PduQ.empty()) {

        Packet* pkt = (Packet*)PduQ.front();

        u_int hlen = GetHeaderLength();
        u_int pdulen = hlen + pkt->pkt_getlen() + padding;
        if (sendlen + pdulen <= maxlen) {
            memcpy(ptr + hlen, pkt->pkt_sget(), pkt->pkt_getlen());	// IP Packet
            FC = frgNone;	// no fragment

            pduqlen--;
            PduQ.pop_front();
            delete pkt;
        } else		// Fragment
        {
            hlen++;	// Fragment subheader
            pdulen = maxlen - sendlen;

            if (hlen + padding >= pdulen)	// No space for payload
                break;
            _qPkt = pkt;
            pduqlen--;
            PduQ.pop_front();
            _qLen = pdulen - hlen - padding;
            FC = frgFirst;	// first fragment
            memcpy(ptr + hlen, pkt->pkt_sget(), _qLen);	// IP Packet
        }
        BuildGenericHeader(ptr, pdulen);

        sendlen += pdulen;
        ptr += pdulen;

        if (maxlen <= sendlen + sizeof(struct hdr_generic) + padding)
            break;
    }
    return sendlen;
}

DataConnectionEthernet::DataConnectionEthernet(int cid, bool mesh)
: DataConnection(cid, mesh)
{
}

int DataConnectionEthernet::EncapsulateAll(char *dstbuf, size_t maxlen)
{
    Packet *pkt;
    char *ptr;
    u_int pdulen, hlen, sendlen = 0, padding = 0;
    if (CRCIndicator)
        padding = 4;

    if (_qPkt) {
        hlen = GetHeaderLength() + 1;
        if (hlen + padding >= maxlen)	// No space enough to send any pdu
            return 0;

        pdulen = hlen + _qPkt->pkt_getlen() - _qLen + padding;
        if (pdulen <= maxlen) {
            FC = frgLast;	// last fragment
            if (_qLen < 14) {
                memcpy(dstbuf + hlen,
                    _qPkt->pkt_get() + _qLen,
                    14 - _qLen);
                memcpy(dstbuf + hlen + 14 - _qLen,
                    _qPkt->pkt_sget(),
                    pdulen - hlen - (14 - _qLen) -
                    padding);
            } else {
                memcpy(dstbuf + hlen,
                    _qPkt->pkt_sget() + _qLen - 14,
                    pdulen - hlen - padding);
            }
            delete _qPkt;
            _qPkt = NULL;
            _qLen = 0;
        } else {
            pdulen = maxlen;
            FC = frgMiddle;	// middle fragment

            if (_qLen < 14) {
                if (14U - _qLen > pdulen - hlen - padding) {
                    memcpy(dstbuf + hlen,
                        _qPkt->pkt_get() + _qLen,
                        pdulen - hlen - padding);
                    _qLen += pdulen - hlen - padding;
                } else {
                    memcpy(dstbuf + hlen,
                        _qPkt->pkt_get() + _qLen,
                        14 - _qLen);
                    memcpy(dstbuf + hlen + 14 - _qLen,
                        _qPkt->pkt_sget(),
                        pdulen - hlen - (14 -
                                _qLen) -
                        padding);

                    _qLen += pdulen - hlen - padding;
                }
            } else {
                memcpy(dstbuf + hlen,
                    _qPkt->pkt_sget() + _qLen - 14,
                    pdulen - hlen - padding);

                _qLen += pdulen - hlen - padding;
            }
        }
        BuildGenericHeader(dstbuf, pdulen);
        sendlen = pdulen;
    }

    ptr = dstbuf + sendlen;
    while (!_qPkt && !PduQ.empty()) {
        pkt = (Packet *) PduQ.front();

        hlen = GetHeaderLength();
        pdulen = hlen + pkt->pkt_getlen() + padding;
        if (sendlen + pdulen <= maxlen) {
            memcpy(ptr + hlen, pkt->pkt_get(), 14);	// Ether Header
            memcpy(ptr + hlen + 14, pkt->pkt_sget(), pkt->pkt_getlen() - 14);	// IP Packet
            FC = frgNone;	// no fragment

            pduqlen--;
            PduQ.pop_front();
            delete pkt;
        } else		// Fragment
        {
            hlen++;	// Fragment subheader
            pdulen = maxlen - sendlen;

            if (hlen + padding >= pdulen)	// No space for payload
                break;
            _qPkt = pkt;
            pduqlen--;
            PduQ.pop_front();
            _qLen = pdulen - hlen - padding;
            FC = frgFirst;	// first fragment

            if (_qLen >= 14) {
                memcpy(ptr + hlen, pkt->pkt_get(), 14);	// Ether Header
                memcpy(ptr + hlen + 14, pkt->pkt_sget(), _qLen - 14);	// IP Packet
            } else {
                memcpy(ptr + hlen, pkt->pkt_get(), _qLen);	// Ether Header
            }
        }
        BuildGenericHeader(ptr, pdulen);

        sendlen += pdulen;
        ptr += pdulen;

        if (sendlen + sizeof(struct hdr_generic) + padding >=
            maxlen)
            break;
    }
    return sendlen;
}

ConnectionReceiver::ConnectionReceiver(u_int16_t pCid)
{
    _cid = pCid;

    _src_nid = 0;
    _dst_nid = 0;

    _fsn = 0;
    _len = 0;
    _maxlen = 2048;
    _buffer = new char[_maxlen];
    _state = NoData;
    _attrARQ = 0;
}

ConnectionReceiver::ConnectionReceiver(u_int16_t pCid, u_int16_t src_nid, u_int16_t dst_nid ) {
    
    _cid = pCid;
    _src_nid = src_nid;
    _dst_nid = dst_nid;
    _fsn = 0;
    _len = 0;
    _maxlen = 2048;
    _buffer = new char[_maxlen];
    _state = NoData;
    _attrARQ = 0;

}

ConnectionReceiver::~ConnectionReceiver()
{
    delete [] _buffer;
}

void ConnectionReceiver::insert(struct hdr_generic *hg, int len) {

    char *ptr = reinterpret_cast < char *>(hg + 1);
    int oh = 0;
    struct subhdr_fragment *fraghdr = NULL;
    
    if (hg->type & tyMesh) {
        
        ptr += 2;
    
    }
    
    if (hg->type & tyGrant) {
        
        ptr += 2;
    
    }
    
    if (hg->type & tyFragment) {
        
        fraghdr = reinterpret_cast<struct subhdr_fragment*>(ptr);
        
        if (hg->type & tyARQ)
            ptr += 2;
        
        else if (hg->type & tyExtend)
            ptr += 2;
        
        else {
            
            ptr += 1;
            
            if (fraghdr->fsn != (_fsn + 1) % 8) {
                
                drop();
            }
            
            _fsn = fraghdr->fsn;
        }
    }
    
    if (hg->type & tyPacking) {
        
        if (hg->type & tyARQ)
            ptr += 3;
        
        else if (hg->type & tyExtend)
            ptr += 3;
        
        else
            ptr += 2;
    }

    oh = ptr - reinterpret_cast < char *>(hg);
    
    if (fraghdr) {
        
        switch (fraghdr->fc) {
        
        case frgNone:
            
            drop();
            memcpy(_buffer, ptr, len - oh);
            _len = len - oh;
            _state = Complete;
            break;
        
        case frgLast:
            
            if (_state == NotComplete) {
                
                if (_len + len - oh > _maxlen) {
                    
                    _maxlen = _len + len - oh;
                    char* p = new char[_maxlen];
                    memcpy(p, _buffer, _len);
                    delete _buffer;
                    _buffer = p;
                
                }

                memcpy(_buffer + _len, ptr, len - oh);
                _len += len - oh;
                _state = Complete;
            
            }
            break;
        
        case frgFirst:
            
            drop();
            
            if (len - oh > _maxlen) {
                
                _maxlen = len - oh;
                delete _buffer;
                _buffer = new char[_maxlen];
            
            }
            
            memcpy(_buffer, ptr, len - oh);
            _len = len - oh;
            _state = NotComplete;
            break;
        
        case frgMiddle:
            
            if (_state == NotComplete) {
                
                memcpy(_buffer + _len, ptr, len - oh);
                _len += len - oh;
                _state = NotComplete;
            
            }
            
            break;
        }
        
    } 
    else {
        
        _external = ptr;
        _len = len - oh;
        _state = External;
    
    }
}

char *ConnectionReceiver::getPacket(int &len) {

    char *ret;
    if (_state == Complete) {
    
        ret = _buffer;
        len = _len;
        _state = NoData;
        _len = 0;
        return ret;
    
    } 
    else if (_state == External) {
        
        len = _len;
        _len = 0;
        _state = NoData;
        return _external;
    
    }
    
    return NULL;
}

void ConnectionReceiver::drop() {

    if (_state != NoData) {
        
        printf("Drop fragment data status=%d %d\n", _state, _len);
        _state = NoData;
        _len = 0;
    
    }
}
