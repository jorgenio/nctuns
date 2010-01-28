
#ifndef __NCTUNS_80216J_NT_CROSS_80216J_NT_PMPRS_H__
#define __NCTUNS_80216J_NT_CROSS_80216J_NT_PMPRS_H__

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <math.h>
#include <vector>

#include <ethernet.h>
#include <nctuns_api.h>
#include <packet.h>
#include <object.h>
#include <tcp.h>
#include <timer.h>
#include <hash_map>
namespace std
{
	using namespace __gnu_cxx;
}
using namespace std;
class cross802_16j_NT_PMPRS:NslObject
{
private:
	hash_map<uint32_t,int> routingTable;
    typedef pair <uint32_t,int> ipConnPair;
public:
	cross802_16j_NT_PMPRS(uint32_t type, uint32_t id, struct plist *pl, const char *name);
	~cross802_16j_NT_PMPRS();
	int recv(ePacket_ *);
	int send(ePacket_ *);
};
#endif
