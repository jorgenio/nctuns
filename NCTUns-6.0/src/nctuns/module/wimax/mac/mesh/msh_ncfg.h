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

#ifndef __NCTUNS_WIMAX_MSH_NCFG_H__
#define __NCTUNS_WIMAX_MSH_NCFG_H__


#include <vector>
#include "sch/ctrl/entry_interface.h"
#include "../management_message.h"


struct NCFG_Nbr_Physical_IE {

    u_char ll_InfoPresent:1;
    u_char ll_Requst:1;
    u_char ll_Accepted:1;
    u_char Hop:1;
    u_char EstDelay:4;	// in us
    u_char NbrXmtMx:5;
    u_char NbrXmtExp:3;

};

struct NCFG_Nbr_Logical_IE {

    u_char Rcv_Quality:3;
    u_char NbrBurstProfile:4;
    u_char ExcessDemand:1;
    u_char NbrXmtPower:4;
    u_char NbrXmtAntenna:3;
    u_char PreambleFlag:1;

};

struct MSH_NCFG_field {

    u_char NumNbrEntries:5;
    u_char NumBSEntries:2;
    u_char EmbeddedPacketFlag:1;
    u_char XmtPower:4;
    u_char XmtAntenna:3;
    u_char MacAddressFlag:1;
    u_char NetworkCh:4;
    //      u_char  rsv                             : 4;
    u_char NetConfigCount:4;

    u_char msbFrameNumber:8;
    u_char lsbFrameNumber:4;
    u_char NwCtrlSlot:4;
    u_char SyncHopCount:8;

    u_char NextXmtMx:5;
    u_char XmtHoldExp:3;
};

typedef struct cBSEntry {

    u_char msbBSID;
    u_char lsbBSID;
    u_char HopCount:3;
    u_char XmtEngery:5;

} bs_info_entry_t;

class Base_station_entry;
class Mac_address;
class Neighbor;

namespace Msh_ncfg {

    typedef enum {

        original_std = 1,
        extended     = 2,

    } link_est_ie_format_t;

    class Embedded_data_hdr;
}

class MSH_NCFG: public ManagementMessage, public Ctrl_sch::Entry_interface {

    private:
        struct MSH_NCFG_field*      f;	// fixed length field part
        u_char*                     MacAddr;

        struct cBSEntry**           BSEntry;

        struct cNbrEntry {

            u_int16_t NbrNodeID;
            struct NCFG_Nbr_Physical_IE phyie;
            struct NCFG_Nbr_Logical_IE logie;

        }** NbrEntry;

        Msh_ncfg::link_est_ie_format_t              link_est_ie_format;
        std::vector<Msh_ncfg::Embedded_data_hdr*>	embedded;

    public:
        MSH_NCFG();
        MSH_NCFG(u_char* pBuffer, int pLength, Msh_ncfg::link_est_ie_format_t fmt);
        virtual ~MSH_NCFG();

        int copyTo(u_char* dstbuf) const;
        int getLen() const;

        int addBSentry(u_int16_t pBSID, u_char pHopCount, u_char pXmtEngery);

        int add_nbr_entry(const Ctrl_sch::Entry&);
        int addEmbedded(Msh_ncfg::Embedded_data_hdr* data);

        inline int setXmtParameter(u_char pPwr, u_char pAntenna)
        {
            f->XmtPower = pPwr;
            f->XmtAntenna = pAntenna;
            return 0;
        } 

        inline void setTimeStamp(uint16_t pFn, u_char pSn, u_char pHopCount)
        {
            f->msbFrameNumber = pFn >> 4;
            f->lsbFrameNumber = pFn & 0xf;
            f->NwCtrlSlot = pSn;
            f->SyncHopCount = pHopCount;
        }

        int setSchedParameter(u_int32_t pCurrXmtTime,u_int32_t pNextXmtTime, u_char pHoldoff,u_int32_t maxXmtOpps);

        int setEntryAddress(const u_char pMacAddr[6]);

        inline uint8_t get_nf_nbr_sch_entry() const { return f->NumNbrEntries; }
        void get_nbr_sch_entry(Ctrl_sch::Entry&, int) const;

        inline u_char getNumBSEntries() const { return f->NumBSEntries; }

        Base_station_entry* getBSEntry(int index);


        inline void getXmtParameter(u_char& pPwr, u_char& pAntenna)
        {
            pPwr = f->XmtPower;
            pAntenna = f->XmtAntenna;
        }

        int set_tx_holdoff_exp(u_int32_t htime_exp_val);

        void getTimeStamp(u_int16_t& pFn, u_char& pSn, u_char& pHopCount) const;

        inline void get_sch_param(Ctrl_sch::Entry& sch_entry)
        {
            sch_entry.set_next_tx_mx(f->NextXmtMx);
            sch_entry.set_tx_holdoff_exp(f->XmtHoldExp);
        }

        inline bool getEntryFlag() const { return (bool) f->MacAddressFlag; }

        void getEntryAddress(Mac_address&) const;

        /*
         * FIXME: is 8-bits enough to contain the number of embedded IE?
         */
        inline u_char getNumEmbedded() { return embedded.size(); }

        inline Msh_ncfg::Embedded_data_hdr* getEmbedded(int index)
        { return embedded[index]; }

        void dump();
};


#endif /* __NCTUNS_WIMAX_MSH_NCFG_H__ */
