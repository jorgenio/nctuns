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

#include "mac802_16j.h"

using namespace mobileRelayMac80216j;
using namespace mobileRelayMacAddress;
using namespace mobileRelayTimer;
using namespace mobileRelayCommon;

mac802_16j::mac802_16j()
{
	_addr       = new Mac_address();
	_timer_mngr = new Timer_mngr();
	_maxqlen    = 0;
}

	mac802_16j::mac802_16j(uint32_t type, uint32_t id, plist* pl, const char* name)
: NslObject(type, id, pl, name)
{
	_addr       = new Mac_address();
	_timer_mngr = new Timer_mngr();
	_maxqlen    = 0;

	vBind_mac("macaddr", _addr->buf());
	vBind("max_qlen", &_maxqlen);
}

mac802_16j::~mac802_16j()
{
	if (_timer_mngr)
		delete _timer_mngr;

	if (_addr)
		delete _addr;
}

int mac802_16j::init()
{
	uint8_t code[256][18];
	int index = 0;


	S = 0;
        N = 43;
        M = 43;
        L = 43;
        O = 43;
	P = 43;
	Q = 40;

	assert(_maxqlen > 0);

	_Ts                 = GET_REG_VAR(get_port(), "Ts", double *);
	_PS                 = GET_REG_VAR(get_port(), "PS", double *);
	_CPratio            = GET_REG_VAR(get_port(), "CPratio", double *);
	_PSratio            = GET_REG_VAR(get_port(), "PSratio", double *);
	_Pt                 = GET_REG_VAR(get_port(), "transPower", double *);
	_symbolsPerFrame    = GET_REG_VAR(get_port(), "symbolsPerFrame", int *);
	_DLsubchannels      = GET_REG_VAR(get_port(), "DLsubchannels", int *);
	_ULsubchannels      = GET_REG_VAR(get_port(), "ULsubchannels", int *);
	_TTG                = GET_REG_VAR(get_port(), "TTG", int *);
	_RTG                = GET_REG_VAR(get_port(), "RTG", int *);
	_RSTTG                = GET_REG_VAR(get_port(), "RSTTG", int *);
	_RSRTG                = GET_REG_VAR(get_port(), "RSRTG", int *);

	assert(_Ts && _PS && _Pt && _CPratio && _PSratio && _symbolsPerFrame
			&& _DLsubchannels && _ULsubchannels && _TTG && _RTG);

	// Generate Ranging codes (Spec. 16e. 8.4.7.3)
	gen_ranging_code_mr(0x00, code, 256);  // FIXME: Assume UL_PermBase = 0x00

	for (int i = S % 256; i < (S + N) % 256; i++)    // 0 ~ 42
	{
		memcpy(_initRang_code_db[index], code[i], 18);
		index++;
	}

	index = 0;
	for (int i = (S + N) % 256; i < (S + N + M) % 256; i++)  // 43 ~ 85
	{
		memcpy(_periodRang_code_db[index], code[i], 18);
		index++;
	}

	index = 0;
	for (int i = (S + N + M) % 256; i < (S + N + M + L) % 256; i++)  // 86 ~ 128
	{
		memcpy(_bwRequest_code_db[index], code[i], 18);
		index++;
	}

	index = 0;
	for (int i = (S + N + M + L) % 256; i < (S + N + M + L + O) % 256; i++)  // 129 ~ 171
	{
		memcpy(_handoverRang_code_db[index], code[i], 18);
		index++;
	}

	index = 0;
	for (int i = (S + N + M + L + O) % 256; i < (S + N + M + L + O + P) % 256; i++)  // 172 ~ 214
        {
                memcpy(_rs_initRang_code_db[index], code[i], 18);
                index++;
        }

	index = 0;
	for (int i = (S + N + M + L + O + P) % 256; i < (S + N + M + L + O + P + Q) % 256; i++)  // 215 ~ 254
        {
                memcpy(_rs_dedicated_code_db[index], code[i], 18);
                index++;
        }

	return NslObject::init();
}

int mac802_16j::getCode(int usage)
{
	int index = 0;

	switch (usage){
		case INITIAL_RANGING:
			index = (int)(random() % N) + S;
			break;

		case PERIOD_RANGING:
			index = (int)(random() % M) + S + N;
			break;

		case BW_REQUEST:
			index = (int)(random() % L) + S + N + M;
			break;

		case HANDOVER_RANGING:
			index = (int)(random() % O) + S + N + M + L;
			break;
		
		case RS_INITIAL_RANGING:
                        index = (int)(random() % P) + S + N + M + L + O;
                        break;
		
		case RS_DEDICATED_CODES:
                        index = (int)(random() % Q) + S + N + M + L + O + P;
                        break;

		default:
			printf("\e[1;31m%d:UNKNOWN Code Usage. (%s)\e[0m\n", get_nid(), __func__);
			exit(1);
	}
	return index;
}

uint8_t mac802_16j::getCodeIndex(uint8_t *code, uint8_t *usage)
{
	for (uint8_t i = 0;i < N;i++)
	{
		if (memcmp(_initRang_code_db[i], code, 18) == 0)
		{
			*usage = INITIAL_RANGING;
			return i + S;
		}
	}

	for (uint8_t i = 0;i < M;i++)
	{
		if (memcmp(_periodRang_code_db[i], code, 18) == 0)
		{
			*usage = PERIOD_RANGING;
			return i + S + N;
		}
	}

	for (uint8_t i = 0;i < L;i++)
	{
		if (memcmp(_bwRequest_code_db[i], code, 18) == 0)
		{
			*usage = BW_REQUEST;
			return i + S + N + M;
		}
	}

	for (uint8_t i = 0;i < O;i++)
	{
		if (memcmp(_handoverRang_code_db[i], code, 18) == 0)
		{
			*usage = HANDOVER_RANGING;
			return i + S + N + M + L;
		}
	}

	for (uint8_t i = 0;i < P;i++)
        {
                if (memcmp(_rs_initRang_code_db[i], code, 18) == 0)
                {
                        *usage = RS_INITIAL_RANGING;
                        return i + S + N + M + L + P;
                }
        }

	for (uint8_t i = 0;i < Q;i++)
        {
                if (memcmp(_rs_dedicated_code_db[i], code, 18) == 0)
                {
                        *usage = RS_DEDICATED_CODES;
                        return i + S + N + M + L + P + Q;
                }
        }

	return 255; // not exist
}

int mac802_16j::resetTimerT(int tno)
{
	_timer_mngr->reset_t(tno);

	return 0;
}

int mac802_16j::SendNTFYtoPHY(Pkthdr *ntfyCmd)
{
	ePacket_ *epkt  = NULL;

	epkt            = createEvent();
	ntfyCmd->flag   = PF_SEND;
	epkt->DataInfo_ = reinterpret_cast<void *>(ntfyCmd);
	put(epkt, sendtarget_);

	return 0;
}

/* PS: physical slot ( A unit of time for allocating bandwidth) */
int mac802_16j::symbolsToPSs(int nsymbols)	
{
	return static_cast<int>(nsymbols * (1 + CPratio()) / PSratio());
}

double mac802_16j::symbolsToMicro(int nsymbols)
{
	return nsymbols * Ts();
}

double mac802_16j::PSsToMicro(int nPSs)
{
	return nPSs * PS();
}

// Convert char* to Packet*
Packet *mac802_16j::asPacket(char *ptr, int pLen)
{
	Packet *pkt = NULL;
	char *buf   = NULL;

	pkt = new Packet();

	if (_CSType == csIPv4) 
	{
		buf = pkt->pkt_sattach(pLen);
		memcpy(buf, ptr, pLen);
		pkt->pkt_sprepend(buf, pLen);
	} 
	else 
	{
		buf = pkt->pkt_sattach(pLen - sizeof(struct ether_header));
		memcpy(buf, ptr + sizeof(struct ether_header), pLen - sizeof(struct ether_header));
		pkt->pkt_sprepend(buf, pLen - sizeof(struct ether_header));
		buf = pkt->pkt_malloc(sizeof(struct ether_header));
		memcpy(buf, ptr, sizeof(struct ether_header));
	}

	return pkt;
}
