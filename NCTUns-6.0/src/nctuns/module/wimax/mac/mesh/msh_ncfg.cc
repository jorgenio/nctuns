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

#include "msh_ncfg.h"

#include "base_station_info.h"
#include "mac802_16_meshss.h"
#include "msh_ncfg/emb_data_hdr.h"
#include "neighbor.h"
#include "sch/ctrl/info.h"
#include "../mac_address.h"
#include "../verbose.h"


using namespace Msh_ncfg;


MSH_NCFG::MSH_NCFG()
: ManagementMessage(CTLMSG_TYPE_MSH_NCFG)
{
    MacAddr = NULL;
    BSEntry = NULL;

    f = new MSH_NCFG_field;
    bzero(f, sizeof(MSH_NCFG_field));
    BSEntry = new cBSEntry *[3];
    NbrEntry = new cNbrEntry *[31];
}

MSH_NCFG::MSH_NCFG(u_char* pBuffer, int pLength, link_est_ie_format_t fmt)
: ManagementMessage(pBuffer)
{
    int pos = 1 + sizeof(MSH_NCFG_field), i;

    MacAddr            = NULL;
    BSEntry            = NULL;
    NbrEntry           = NULL;
    link_est_ie_format = fmt;

    f = reinterpret_cast<MSH_NCFG_field*>(pBuffer + 1);

    if (f->MacAddressFlag) {
        MacAddr = pBuffer + pos;
        pos += 6;
    }

    BSEntry = new cBSEntry *[f->NumBSEntries];

    for (i = 0; i < f->NumBSEntries; i++) {
        BSEntry[i] = (struct cBSEntry *) (pBuffer + pos);
        pos += 3;
    }

    NbrEntry = new cNbrEntry *[f->NumNbrEntries];
    for (i = 0; i < f->NumNbrEntries; i++) {
        NbrEntry[i] = (struct cNbrEntry *) (pBuffer + pos);
        pos += 4;
        if (NbrEntry[i]->phyie.ll_InfoPresent)
            pos += 2;
    }

    if (f->EmbeddedPacketFlag) {

        while (1) {

            Embedded_data_hdr* emb_ie = Embedded_data_hdr::create_emb_ie(
                    pBuffer + pos, pLength - 1 - pos, fmt);

            assert(emb_ie);
            embedded.push_back(emb_ie);
            emb_ie->set_ncfg(this);

            pos += (emb_ie->length() + 2);

            if (!emb_ie->is_extended()) {
                break;
            }
        }
    }
}

MSH_NCFG::~MSH_NCFG()
{
    if (flag) {
        std::vector<Embedded_data_hdr*>::iterator it;
        int i;

        if (f->MacAddressFlag) {
            delete[] MacAddr;
        }

        for (i = 0; i < f->NumBSEntries; i++) {
            delete BSEntry[i];
        }
        delete[] BSEntry;

        for (i = 0; i < f->NumNbrEntries; i++) {
            delete NbrEntry[i];
        }
        delete[] NbrEntry;

        for (it = embedded.begin(); it != embedded.end(); it++) {
            delete(*it);
        }

        delete f;
    } else {
        std::vector<Embedded_data_hdr*>::iterator it;

        if (BSEntry) {
            delete[] BSEntry;
        }

        if (NbrEntry) {
            delete[] NbrEntry;
        }

        for (it = embedded.begin(); it != embedded.end(); it++) {
            delete(*it);
        }
    }
}

int
MSH_NCFG::copyTo(u_char* dstbuf) const
{
    u_int pos = 0, i;
    memcpy(dstbuf, &Type, 1);
    memcpy(dstbuf + 1, f, sizeof(MSH_NCFG_field));
    pos += 1 + sizeof(MSH_NCFG_field);

    if (f->MacAddressFlag) {
        memcpy(dstbuf + pos, MacAddr, 6);
        pos += 6;
    }

    for (i = 0; i < f->NumBSEntries; i++) {
        memcpy(dstbuf + pos, BSEntry[i], sizeof(struct cBSEntry));
        pos += sizeof(struct cBSEntry);
    }

    for (i = 0; i < f->NumNbrEntries; i++) {
        if (NbrEntry[i]->phyie.ll_InfoPresent) {
            memcpy(dstbuf + pos, NbrEntry[i],
                    sizeof(struct cNbrEntry));
            pos += sizeof(struct cNbrEntry);
        } else {
            memcpy(dstbuf + pos, NbrEntry[i],
                    sizeof(struct cNbrEntry) -
                    sizeof(struct NCFG_Nbr_Logical_IE));
            pos +=
                sizeof(struct cNbrEntry) -
                sizeof(struct NCFG_Nbr_Logical_IE);
        }
    }

    if (f->EmbeddedPacketFlag) {
        for (i = 0; i < embedded.size(); i++) {
            pos += embedded[i]->copyTo(dstbuf + pos);
        }
    }
    return pos;		// Total Length
}

//      AddBSEntry:
//      Add a BS info entry into MSH-NCFG message
//      Return Value: >0, The BS entry order after this procedure (array index)
//                    -1, Error occured

int MSH_NCFG::addBSentry(u_int16_t pBSID, u_char pHopCount,u_char pXmtEngery) {

    int i = f->NumBSEntries;

    if (i >= 3)
        return -1;

    BSEntry[i]->msbBSID   = static_cast<u_char> ((pBSID>>8) & 0x00ff);
    BSEntry[i]->lsbBSID   = static_cast<u_char> (pBSID & 0x00ff);
    BSEntry[i]->HopCount  = pHopCount;
    BSEntry[i]->XmtEngery = pXmtEngery;

    return f->NumBSEntries++;

}

/*
 * add_nbr_entry:
 * Add a Neighbor info entry into MSH-NCFG message
 * Return Value: >0, The Neighnot entry order after this procedure (array index)
 *               -1, Error occured
 */
int
MSH_NCFG::add_nbr_entry(const Ctrl_sch::Entry& entry)
{
    int i = f->NumNbrEntries;

    if (i >= 31)
        return -1;

    NbrEntry[i] = new cNbrEntry;
    NbrEntry[i]->NbrNodeID              = entry.node_id();
    NbrEntry[i]->phyie.ll_InfoPresent   = 0;
    NbrEntry[i]->phyie.ll_Requst        = 0;
    NbrEntry[i]->phyie.ll_Accepted      = 0;
    NbrEntry[i]->phyie.Hop              = entry.hops_to_nbr();
    NbrEntry[i]->phyie.EstDelay         = entry.est_prop_delay();
    NbrEntry[i]->phyie.NbrXmtMx         = entry.next_tx_mx();
    NbrEntry[i]->phyie.NbrXmtExp        = entry.tx_holdoff_exp();
    
    return f->NumNbrEntries++;
}

int MSH_NCFG::addEmbedded(Embedded_data_hdr* data)
{
    if (embedded.size() != 0) {
        Embedded_data_hdr *data;

        data = embedded.back();
        data->set_extended();
    }

    f->EmbeddedPacketFlag = 1;
    embedded.push_back(data);

    return 1;
}

int MSH_NCFG::set_tx_holdoff_exp(u_int32_t htime_exp_val) {

    f->XmtHoldExp = htime_exp_val;
    return 1;

}

int MSH_NCFG::setSchedParameter(u_int32_t pCurrXmtTime,
        u_int32_t pNextXmtTime, u_char pHoldoff,
        u_int32_t maxXmtOpps)
{
    int interval, ref1, ref2;

    interval = (1 << pHoldoff);
    ref1 = ((pCurrXmtTime - 1) / interval) * interval + 1;
    ref2 = ((pNextXmtTime - 1) / interval) * interval + 1;

    if (ref2 < ref1) {
        // Wrapping
        ref2 += maxXmtOpps;
    }

    if ((ref2 - ref1) / interval > 31) {
        f->NextXmtMx = 15;
        f->XmtHoldExp = pHoldoff;
        //printf("ref1=%d, ref2=%d, pHoldoff = %d=>%d\n", ref1, ref2, pHoldoff-1, pHoldoff);
    } else {
        f->NextXmtMx = (ref2 - ref1) / interval;
        f->XmtHoldExp = pHoldoff;
    }

    return 0;
}

int MSH_NCFG::setEntryAddress(const u_char pMacAddr[6])
{
    if (!f->MacAddressFlag) {
        f->MacAddressFlag = 1;
        MacAddr = new u_char[6];
    }
    memcpy(MacAddr, pMacAddr, 6);
    return 0;
}

void
MSH_NCFG::get_nbr_sch_entry(Ctrl_sch::Entry& nbr_sch_entry, int idx) const
{
	assert(idx < f->NumNbrEntries);

	nbr_sch_entry.set_node_id(NbrEntry[idx]->NbrNodeID);
    nbr_sch_entry.set_hops_to_nbr(NbrEntry[idx]->phyie.Hop);
	nbr_sch_entry.set_next_tx_mx(NbrEntry[idx]->phyie.NbrXmtMx);
	nbr_sch_entry.set_tx_holdoff_exp(NbrEntry[idx]->phyie.NbrXmtExp);
}

Base_station_entry*
MSH_NCFG::getBSEntry(int index)
{
    if ( index > getNumBSEntries() ) {

        FATAL("MSH_NCFG::getBSEntry:: illegal index range.\n");
        exit(1);
    }

    struct cBSEntry* entry_p = BSEntry[index];

    if (!entry_p) {

        FATAL("MSH_NCFG::getBSEntry: illegal index range.\n");
        exit(1);
    }

    Base_station_entry* ptr = new Base_station_entry;

    u_int16_t composite_nid = 0;
    composite_nid = entry_p->msbBSID;
    composite_nid <<= 8;
    composite_nid |= entry_p->lsbBSID;

    ptr->nid                = composite_nid; 
    ptr->hop_cnt            = entry_p->HopCount;
    ptr->tx_energy_level    = entry_p->XmtEngery;

    return ptr;
}

void
MSH_NCFG::getTimeStamp(u_int16_t& pFn, u_char& pSn, u_char& pHopCount) const
{
    pFn = (f->msbFrameNumber << 4) + f->lsbFrameNumber;
    pSn = f->NwCtrlSlot;
    pHopCount = f->SyncHopCount;
}

void
MSH_NCFG::getEntryAddress(Mac_address& addr) const
{
    if (getEntryFlag())
        addr.copy_from(MacAddr, 6);
}

int
MSH_NCFG::getLen() const
{
    int len = sizeof(MSH_NCFG_field) + 1;
    u_int i;

    if (f->MacAddressFlag) {
        len += 6;
    }

    len += (sizeof(cBSEntry) * f->NumBSEntries);

    for (i = 0; i < f->NumNbrEntries; i++) {
        len += ((NbrEntry[i]->phyie.ll_InfoPresent) ? 6 : 4);
    }

    if (f->EmbeddedPacketFlag) {
        for (i = 0; i < embedded.size(); i++) {
            len += (embedded[i]->length() + 2);
        }
    }

    return len;
}
