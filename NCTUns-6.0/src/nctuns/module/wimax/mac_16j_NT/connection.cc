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

#include "connection.h"
#include "../../misc/log/logpack.h"

using namespace MR_Connection_NT;

Connection::Connection(int pcid)
{

	logRidvan(TRACE,"-->Connection::Connection ");
	cid             = pcid;
	crcIndicator    = 1;
	pduqlen         = 0;
	type            = 0x00;
	src_nid         = 0;
	dst_nid         = 0;
	fsn             = 0;
	fc              = frgNone;
}

/*
 * Build generic MAC header.
 * Note that we do not implement encryption and HCS.
 */
void Connection::BuildGenericHeader(char *buf, int len)
{

	logRidvan(TRACE,"-->Connection::BuildGenericHeader ");
	int crc     = 0;
	int offset  = 0;
	struct hdr_generic *hg = (struct hdr_generic *) buf;

	hg->ht      = 0;        // Generic MAC header = 0
	hg->ec      = 0;        // Not implemented
	hg->type    = type;

	if (fc != frgNone)
		hg->type |= 0x04;

	hg->esf     = 0;        // Not implemented
	hg->ci      = crcIndicator;
	hg->eks     = 0;        // Not implemented
	hg->rsv     = 0;        // Reserved
	hg->len_msb = len / 256;
	hg->len_lsb = len % 256;
	hg->cid_msb = cid / 256;
	hg->cid_lsb = cid % 256;
	hg->hcs     = hcs_16j_NT(reinterpret_cast <char*>(hg),sizeof(struct hdr_generic) - 1);

	offset = sizeof(hdr_generic);

	if (hg->type & 0x20) {

		//memcpy(buf + offset, &NodeID, 2);
		memcpy(buf + offset, &src_nid, 2);
		offset += 2;

	}

	if (hg->type & 0x04) {

		struct subhdr_fragment* p = reinterpret_cast<struct subhdr_fragment*> (buf+offset);
		p->fc   = fc;
		p->fsn  = fsn;
		p->rsv  = 0;
		offset  += 1;

	}

	if (crcIndicator != 0) {

		crc = crc32_16j_NT(0, (char *) buf, len - 4);
		memcpy((char *) buf + len - 4, &crc, 4);

	}

	fsn = (fsn + 1) % 8;

	if (fc == frgLast)
		fc = frgNone;
}

ManagementConnection::ManagementConnection(int cid)
: Connection(cid)
{

	logRidvan(TRACE,"-->ManagementConnection::ManagementConnection ");
	;
}

ManagementConnection::~ManagementConnection()
{

	logRidvan(TRACE,"-->ManagementConnection::~ManagementConnection ");
	while(!pduQ.empty())
	{
        ManagementMessage *ptr = (ManagementMessage *)pduQ.front();
		delete ptr;
		pduQ.pop_front();
	}
}

void ManagementConnection::Insert(const ManagementMessage *pdu)
{

	logRidvan(TRACE,"-->ManagementConnection::Insert ");
	pduQ.push_back(pdu);
}

int ManagementConnection::GetInformation(vector<int> *info) const
{
	int totallen    = 0;
	int len         = 0;
	const ManagementMessage *mmsg = NULL;

	for (list<const void *>::const_iterator it = pduQ.begin();it != pduQ.end();it++)
	{
		mmsg = static_cast<const ManagementMessage *>(*it);
		len = GetHeaderLength() + mmsg->getLen();

		if (crcIndicator != 0)    // Padding CRC
			len += 4;

		if (info != NULL)
			info->push_back(len);

		totallen += len;
	}

	return totallen;

}

int ManagementConnection::EncapsulateAll(char *dstbuf, size_t maxlen)
{

	logRidvan(TRACE,"-->ManagementConnection::EncapsulateAll ");
	uint32_t sendlen   = 0;
	uint32_t len       = 0;
	uint32_t hlen      = 0;
	const ManagementMessage* mmsg = NULL;

	while (sendlen < maxlen && !pduQ.empty())
	{
		mmsg = static_cast<const ManagementMessage*>(pduQ.front());

		hlen = GetHeaderLength();
		len = hlen + mmsg->getLen();
        //printf("mmsg->getLen() = %d\n",mmsg->getLen());

		if (crcIndicator != 0)
			len += 4;

		if (sendlen + len > maxlen)
			break;

		pduQ.pop_front();
		mmsg->copyTo((uint8_t *) dstbuf + sendlen + hlen);
		BuildGenericHeader(dstbuf + sendlen, len);
		delete mmsg;
		sendlen += len;
	}

	return sendlen;
}

	BroadcastConnection::BroadcastConnection(int cid)
:ManagementConnection(cid)
{

	logRidvan(TRACE,"-->BroadcastConnection::BroadcastConnection ");
	// MG_DLMAP, MG_ULMAP, MG_DCD, MG_UCD, MG_CLKCMP, MG_MOB_TRFIND, MG_MOB_NBRADV, MG_MOB_PAGADV

	for (int i = 0;i < NumBroadcast;i++)
	{
		ifmmSlot[i] = NULL;
	}
}

BroadcastConnection::~BroadcastConnection()
{

	logRidvan(TRACE,"-->BroadcastConnection::~BroadcastConnection ");
	for (int i = 0;i < NumBroadcast;i++)
	{
		if (ifmmSlot[i] != NULL)
		{
			delete ifmmSlot[i];
			ifmmSlot[i] = NULL;
		}
	}
}

void BroadcastConnection::Insert(const ManagementMessage *pdu)
{

	logRidvan(TRACE,"-->BroadcastConnection::Insert ");
	ifmgmt *ifmm = (ifmgmt *) pdu;
	static int index[NumBroadcast] = { MG_DLMAP, MG_ULMAP, MG_DCD, MG_UCD, MG_CLKCMP, MG_MOB_TRFIND, MG_MOB_NBRADV, MG_MOB_PAGADV, MG_RMAP};

	for (int i = 0; i < NumBroadcast; i++)
	{
		if (ifmm->getType() == index[i])
		{
			if (ifmmSlot[i] != NULL)
			{
				delete ifmmSlot[i];
			}

			ifmmSlot[i] = ifmm;
			break;
		}
	}
}

bool BroadcastConnection::Empty()
{

	logRidvan(TRACE,"-->BroadcastConnection::Empty ");
	return !(ifmmSlot[0] || ifmmSlot[1] || ifmmSlot[2] || ifmmSlot[3] || ifmmSlot[4] || ifmmSlot[5] || ifmmSlot[6] || ifmmSlot[7]);
}

// Get Length Information.
int BroadcastConnection::GetInformation(vector <int> *info) const
{
	ifmgmt *ifmm    = NULL;
	int totalLen    = 0;
	int len         = 0;

	for (int i = 0; i < NumBroadcast; i++)
	{
		ifmm = ifmmSlot[i];

		if (ifmm != NULL)
		{
			len = GetHeaderLength() + ifmm->getLen();

			if (crcIndicator != 0)
			{
				len += 4;
			}

			info->push_back(len);
			totalLen += len;
		}
	}
	return totalLen;
}

// Copy all existed data to *dstbuf
int BroadcastConnection::EncapsulateAll(char *dstbuf, size_t maxlen)
{

	logRidvan(TRACE,"-->BroadcastConnection::EncapsulateAll ");
	int len             = 0;
	int sendLen         = 0;
	int hlen            = 0;
	const ifmgmt *ifmm  = NULL;

	for (int i = 0; i < NumBroadcast; i++)
	{
		ifmm = ifmmSlot[i];

		if (ifmm != NULL)
		{
			hlen = GetHeaderLength();
			len = hlen + ifmm->getLen();
			if (crcIndicator != 0)
			{
				len += 4;
			}

			if ((size_t)(sendLen + len) > maxlen)
			{
				break;
			}

			ifmmSlot[i] = NULL;
			ifmm->copyTo((uint8_t *) (dstbuf + sendLen + hlen)); // copy ifmm to dstbuf
			BuildGenericHeader(dstbuf + sendLen, len);

			delete ifmm;
			sendLen += len;
		}
	}
	return sendLen;
}

/*
 *    Data Connection Class
 */
	DataConnection::DataConnection(int cid)
:Connection(cid)
{

	logRidvan(TRACE,"-->DataConnection::DataConnection ");
	_qPkt = NULL;
	_qLen = 0;
}

	DataConnection::DataConnection(int cid, uint16_t peer_nid)
:Connection(cid)
{

	logRidvan(TRACE,"-->DataConnection::DataConnection ");
	_qPkt = NULL;
	_qLen = 0;
	SetDstNodeID(peer_nid);
}

DataConnection::~DataConnection()
{

	logRidvan(TRACE,"-->DataConnection::~DataConnection ");
	if (_qPkt != NULL)
	{
		delete _qPkt;
		_qPkt = NULL;
	}

	while(!pduQ.empty())
	{
		delete (Packet *)*(pduQ.begin());
		pduQ.erase(pduQ.begin());
	}
}

void DataConnection::Insert(const Packet *pdu)
{

	logRidvan(TRACE,"-->DataConnection::Insert ");
	pduqlen++;
	pduQ.push_back(pdu);
}

size_t DataConnection::nf_pending_packet() const
{

	logRidvan(TRACE,"-->DataConnection::nf_pending_packet ");
	return pduQ.size();
}

bool DataConnection::Empty()
{

	logRidvan(TRACE,"-->DataConnection::Empty ");
	if (_qPkt)
		return false;

	return pduQ.empty();
}

int DataConnection::GetInformation(vector<int> *info) const
{
	int totallen    = 0;
	int padding     = 0;
	int hlen        = 0;
	int len         = 0;
	list<const void *>::const_iterator it;

	if (_qPkt != NULL)
	{
		hlen = GetHeaderLength() + 1;
		len = _qPkt->pkt_getlen() - _qLen;

		if (crcIndicator != 0)
			padding = 4;
		else
			padding = 0;

		if (info != NULL)
			info->push_back(hlen + len + padding);

		totallen = hlen + len + padding;
	}

	for (it = pduQ.begin();it != pduQ.end(); it++)
	{
		const Packet* pkt = (Packet*)*it;

		hlen = GetHeaderLength();
		len = pkt->pkt_getlen();

		if (crcIndicator != 0)
			padding = 4;
		else
			padding = 0;

		if (info)
			info->push_back(hlen + len + padding);

		totallen += hlen + len + padding;
	}

	return totallen;
}

int DataConnection::EncapsulateAll(char* dstbuf, size_t maxlen)
{

	logRidvan(TRACE,"-->DataConnection::EncapsulateAll ");
	uint32_t padding    = 0;
	uint32_t sendlen    = 0;
	uint32_t hlen       = 0;
	uint32_t pdulen     = 0;
	char *ptr           = NULL;

	if (crcIndicator != 0)
		padding = 4;

	if (_qPkt)
	{
		hlen = GetHeaderLength() + 1;
		if (hlen + padding >= maxlen)    // No space enough to send any pdu
			return 0;

		pdulen = hlen + _qPkt->pkt_getlen() - _qLen + padding;
		if (pdulen <= maxlen)
		{
			fc = frgLast;    // last fragment
			memcpy(dstbuf + hlen, _qPkt->pkt_sget() + _qLen, pdulen - hlen - padding);
			delete _qPkt;
			_qPkt = NULL;
			_qLen = 0;
		}
		else
		{
			pdulen = maxlen;
			fc = frgMiddle;    // middle fragment

			memcpy(dstbuf + hlen, _qPkt->pkt_sget() + _qLen, pdulen - hlen - padding);
			_qLen += pdulen - hlen - padding;
		}
		BuildGenericHeader(dstbuf, pdulen);
		sendlen = pdulen;
	}

	ptr = dstbuf + sendlen;
	while (!_qPkt && !pduQ.empty())
	{
		Packet* pkt = (Packet*)pduQ.front();
		hlen        = GetHeaderLength();
		pdulen      = hlen + pkt->pkt_getlen() + padding;

		if (sendlen + pdulen <= maxlen)
		{
			memcpy(ptr + hlen, pkt->pkt_sget(), pkt->pkt_getlen());    // IP Packet
			fc = frgNone;    // no fragment
			pduqlen--;
			pduQ.pop_front();
			pkt->pkt_sdeattach();
			delete pkt;
		}
		else        // Fragment
		{
			hlen++;    // Fragment subheader
			pdulen = maxlen - sendlen;

			if (hlen + padding >= pdulen)    // No space for payload
				break;
			_qPkt = pkt;
			pduqlen--;
			pduQ.pop_front();
			_qLen = pdulen - hlen - padding;
			fc = frgFirst;    // first fragment
			memcpy(ptr + hlen, pkt->pkt_sget(), _qLen);    // IP Packet
		}
		BuildGenericHeader(ptr, pdulen);

		sendlen += pdulen;
		ptr += pdulen;

		if (maxlen <= sendlen + sizeof(struct hdr_generic) + padding)
			break;
	}
	return sendlen;
}

	DataConnectionEthernet::DataConnectionEthernet(int cid)
: DataConnection(cid)
{

	logRidvan(TRACE,"-->DataConnectionEthernet::DataConnectionEthernet ");
}

int DataConnectionEthernet::EncapsulateAll(char *dstbuf, size_t maxlen)
{

	logRidvan(TRACE,"-->DataConnectionEthernet::EncapsulateAll ");
	Packet *pkt         = NULL;
	char *ptr           = NULL;
	uint32_t pdulen     = 0;
	uint32_t hlen       = 0;
	uint32_t sendlen    = 0;
	uint32_t padding    = 0;

	if (crcIndicator != 0)
		padding = 4;

	if (_qPkt)
	{
		hlen = GetHeaderLength() + 1;
		if (hlen + padding >= maxlen)    // No space enough to send any pdu
			return 0;

		pdulen = hlen + _qPkt->pkt_getlen() - _qLen + padding;
		if (pdulen <= maxlen)
		{
			fc = frgLast;    // last fragment
			if (_qLen < 14)
			{
				memcpy(dstbuf + hlen, _qPkt->pkt_get() + _qLen, 14 - _qLen);
				memcpy(dstbuf + hlen + 14 - _qLen, _qPkt->pkt_sget(), pdulen - hlen - (14 - _qLen) - padding);
			}
			else
			{
				memcpy(dstbuf + hlen, _qPkt->pkt_sget() + _qLen - 14, pdulen - hlen - padding);
			}

			delete _qPkt;
			_qPkt = NULL;
			_qLen = 0;
		}
		else
		{
			pdulen = maxlen;
			fc = frgMiddle;    // middle fragment

			if (_qLen < 14)
			{
				if (14U - _qLen > pdulen - hlen - padding)
				{
					memcpy(dstbuf + hlen, _qPkt->pkt_get() + _qLen, pdulen - hlen - padding);
					_qLen += pdulen - hlen - padding;
				}
				else
				{
					memcpy(dstbuf + hlen, _qPkt->pkt_get() + _qLen, 14 - _qLen);
					memcpy(dstbuf + hlen + 14 - _qLen, _qPkt->pkt_sget(), pdulen - hlen - (14 - _qLen) - padding);
					_qLen += pdulen - hlen - padding;
				}
			}
			else
			{
				memcpy(dstbuf + hlen, _qPkt->pkt_sget() + _qLen - 14, pdulen - hlen - padding);
				_qLen += pdulen - hlen - padding;
			}
		}
		BuildGenericHeader(dstbuf, pdulen);
		sendlen = pdulen;
	}

	ptr = dstbuf + sendlen;
	while (!_qPkt && !pduQ.empty())
	{
		pkt     = (Packet *) pduQ.front();
		hlen    = GetHeaderLength();
		pdulen  = hlen + pkt->pkt_getlen() + padding;

		if (sendlen + pdulen <= maxlen)
		{
			memcpy(ptr + hlen, pkt->pkt_get(), 14);    // Ether Header
			memcpy(ptr + hlen + 14, pkt->pkt_sget(), pkt->pkt_getlen() - 14);    // IP Packet
			fc = frgNone;    // no fragment
			pduqlen--;
			pduQ.pop_front();
			delete pkt;
		}
		else        // Fragment
		{
			hlen++;    // Fragment subheader
			pdulen = maxlen - sendlen;

			if (hlen + padding >= pdulen)    // No space for payload
				break;
			_qPkt = pkt;
			pduqlen--;
			pduQ.pop_front();
			_qLen = pdulen - hlen - padding;
			fc = frgFirst;    // first fragment

			if (_qLen >= 14)
			{
				memcpy(ptr + hlen, pkt->pkt_get(), 14);    // Ether Header
				memcpy(ptr + hlen + 14, pkt->pkt_sget(), _qLen - 14);    // IP Packet
			}
			else
			{
				memcpy(ptr + hlen, pkt->pkt_get(), _qLen);    // Ether Header
			}
		}
		BuildGenericHeader(ptr, pdulen);

		sendlen += pdulen;
		ptr += pdulen;

		if (sendlen + sizeof(struct hdr_generic) + padding >= maxlen)
			break;
	}
	return sendlen;
}

BR_Connection::BR_Connection(uint16_t pcid)
{

	logRidvan(TRACE,"-->BR_Connection::BR_Connection ");
    pduqlen = 0;
    cid = pcid;
}

BR_Connection::~BR_Connection()
{

	logRidvan(TRACE,"-->BR_Connection::~BR_Connection ");
    pduqlen = 0;
    while(!pduQ.empty())
    {
        delete (BR_HDR *)*(pduQ.begin());
        pduQ.erase(pduQ.begin());
    }
}

void BR_Connection::Insert(const BR_HDR *pdu)
{

	logRidvan(TRACE,"-->BR_Connection::Insert ");
    pduqlen++;
    pduQ.push_back(pdu);
}

int BR_Connection::GetInformation()
{

	logRidvan(TRACE,"-->BR_Connection::GetInformation ");
    return pduqlen*6 ;
}

int BR_Connection::GetInformation(vector <int> *info)
{
    int totalLen = 0;
    int len = 6;    // each BR header is 6 bytes
    for(int i=0 ; i<pduqlen ; i++)
    {
        info->push_back(len);
        totalLen += len;
    }

    return totalLen;
}

ConnectionReceiver::ConnectionReceiver(uint16_t pCid)
{

	logRidvan(TRACE,"-->ConnectionReceiver::ConnectionReceiver ");
	_cid        = pCid;
	_src_nid    = 0;
	_dst_nid    = 0;
	_fsn        = 0;
	_len        = 0;
	_maxlen     = 2048;
	_buffer     = new char [_maxlen];
	_state      = NoData;
	_attrARQ    = 0;
	memset(_buffer, 0, _maxlen);
}

ConnectionReceiver::ConnectionReceiver(uint16_t pCid, uint16_t src_nid, uint16_t dst_nid)
{

	logRidvan(TRACE,"-->ConnectionReceiver::ConnectionReceiver ");
	_cid        = pCid;
	_src_nid    = src_nid;
	_dst_nid    = dst_nid;
	_fsn        = 0;
	_len        = 0;
	_maxlen     = 2048;
	_buffer     = new char [_maxlen];
	_state      = NoData;
	_attrARQ    = 0;
	memset(_buffer, 0, _maxlen);
}

ConnectionReceiver::~ConnectionReceiver()
{

	logRidvan(TRACE,"-->ConnectionReceiver::~ConnectionReceiver ");
	delete [] _buffer;
}

void ConnectionReceiver::insert(struct hdr_generic *hg, int len)
{

	logRidvan(TRACE,"-->ConnectionReceiver::insert ");
	char *ptr = reinterpret_cast < char *>(hg + 1);
	int oh = 0;
	struct subhdr_fragment *fraghdr = NULL;

	if (hg->type & tyMesh)
	{
		ptr += 2;
	}

	if (hg->type & tyGrant)
	{
		ptr += 2;
	}

	if (hg->type & tyFragment)
	{
		fraghdr = reinterpret_cast<struct subhdr_fragment*>(ptr);

		if (hg->type & tyARQ)
			ptr += 2;

		else if (hg->type & tyExtend)
			ptr += 2;

		else {
			ptr += 1;
			if (fraghdr->fsn != (_fsn + 1) % 8)
			{
				drop();
			}
			_fsn = fraghdr->fsn;
		}
	}

	if (hg->type & tyPacking)
	{
		if (hg->type & tyARQ)
			ptr += 3;

		else if (hg->type & tyExtend)
			ptr += 3;

		else
			ptr += 2;
	}

	oh = ptr - reinterpret_cast < char *>(hg);

	if (fraghdr)
	{
		switch (fraghdr->fc) {
			case frgNone:
				drop();
				memcpy(_buffer, ptr, len - oh);
				_len = len - oh;
				_state = Complete;
				break;

			case frgLast:
				if (_state == NotComplete)
				{
					if (_len + len - oh > _maxlen)
					{
						_maxlen = _len + len - oh;
						char* p = new char [_maxlen];
						memset(p, 0, _maxlen);
						memcpy(p, _buffer, _len);
						delete [] _buffer;
						_buffer = p;

					}
					memcpy(_buffer + _len, ptr, len - oh);
					_len += len - oh;
					_state = Complete;
				}
				break;

			case frgFirst:
				drop();
				if (len - oh > _maxlen)
				{
					_maxlen = len - oh;
					delete [] _buffer;
					_buffer = new char [_maxlen];
					memset(_buffer, 0, _maxlen);
				}
				memcpy(_buffer, ptr, len - oh);
				_len = len - oh;
				_state = NotComplete;
				break;

			case frgMiddle:
				if (_state == NotComplete)
				{
					memcpy(_buffer + _len, ptr, len - oh);
					_len += len - oh;
					_state = NotComplete;
				}
				break;
		}
	}
	else
	{
		_external = ptr;
		_len = len - oh;
		_state = External;
	}
}

char *ConnectionReceiver::getPacket(int &len)
{
	char *ret = NULL;

	if (_state == Complete)
	{
		ret = _buffer;
		len = _len;
		_state = NoData;
		_len = 0;
		return ret;
	}
	else if (_state == External)
	{
		len = _len;
		_len = 0;
		_state = NoData;
		return _external;
	}

	return NULL;
}

void ConnectionReceiver::drop()
{

	logRidvan(TRACE,"-->ConnectionReceiver::drop ");
	if (_state != NoData)
	{
		//printf("Drop fragment data status=%d %d\n", _state, _len);
		_state = NoData;
		_len = 0;
	}
}
