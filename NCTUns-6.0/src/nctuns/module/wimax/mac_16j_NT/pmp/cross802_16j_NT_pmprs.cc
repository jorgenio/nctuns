#include "cross802_16j_NT_pmprs.h"
#include "../../../misc/log/logpack.h"
MODULE_GENERATOR( cross802_16j_NT_PMPRS);
cross802_16j_NT_PMPRS::cross802_16j_NT_PMPRS(uint32_t type, uint32_t id,
		struct plist *pl, const char *name) :
	NslObject(type, id, pl, name) {

}

cross802_16j_NT_PMPRS::~cross802_16j_NT_PMPRS() {

}

int cross802_16j_NT_PMPRS::send(ePacket_ *pkt) {

	return 0;
}
char* ipToStr(uint32_t ip) {
	char* str = new char[16];

	sprintf(str, "%d.%d.%d.%d", (uint8_t) (0x000000FF & ip),
			(uint8_t) ((0x0000FF00 & ip) >> 8), (uint8_t) ((0x00FF0000 & ip)
					>> 16), (uint8_t) ((0xFF000000 & ip) >> 24));
	return str;
}
int cross802_16j_NT_PMPRS::recv(ePacket_ *epkt) {
	Packet *pkt = (Packet *) epkt->DataInfo_;
	int cid = *((int *) pkt->pkt_getinfo("cid"));
	int len = *((int *) pkt->pkt_getinfo("len"));
	char way = *((char *) pkt->pkt_getinfo("way"));
	char *ptr2 = pkt->pkt_sget();
	struct ip* ip = (struct ip *) ptr2;

	epkt->DataInfo_ = NULL;
	freePacket(epkt);
	pkt = new Packet();
	logRidvan(WARN, "cid %d way %d ip_len %d ", cid, way, len);
	char *buf = pkt->pkt_sattach(len);
	memcpy(buf, ptr2, len);
	pkt->pkt_sprepend(buf, len);
	pkt->pkt_addinfo("cid", (char *) &cid, sizeof(int));
#define RDV

	if (way == 'u') {
		logRidvan(WARN, "RS CROSS ip packet received packet src:%s dst:%s",
				ipToStr(ip->ip_src), ipToStr(ip->ip_dst));
		int myCid = 0;
#ifdef RDV
		hash_map<uint32_t, int>::const_iterator viter;
		viter = routingTable.find(ip->ip_dst);
		if (viter != routingTable.end()) {

			logRidvan(WARN, "routingTable[ethernet->ip_dst]=%d", viter->second);
			myCid = viter->second;

			pkt->pkt_addinfo("cid", (char *) &myCid, sizeof(int));
			//pDtConn = getDLRelayDtConn(myCid);
		}
#endif
	} else {
		routingTable.insert(ipConnPair(ip->ip_dst, cid));
	}
	pkt->pkt_addinfo("way", (char *) &way, sizeof(char));
	logRidvan(WARN, "CROSS put res=");
	pkt->pkt_setflow(PF_SEND);
	ePacket_ *edeliver = createEvent();
	edeliver->DataInfo_ = pkt;
	logRidvan(WARN, "CROSS packet cid %d way %d edeliver %d sendtaget %d",*((int *) pkt->pkt_getinfo("cid")),
			*((int *) pkt->pkt_getinfo("way")),edeliver,sendtarget_);

	int res = put(edeliver, sendtarget_);
	logRidvan(WARN, "CROSS put res=%d", res);
	return 1;
}
