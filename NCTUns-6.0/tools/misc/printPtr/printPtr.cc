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

#include <stdlib.h>
#include <cstdio>
#include <iostream>
#include "logFormat.h"
#include <cassert>
#include <string.h>
#include <sys/types.h>

using namespace std;

void print_sat_dvbrcs(char *logEntry)
{
    cout << "DVBRCS ";

    switch ( logEntry[1] ) {
        case EVENTTYPE_TX:
            cout << "TX " << ' ';
            break;
        case EVENTTYPE_RTX:
            cout << "RTX " << ' ';
            break;
        case EVENTTYPE_RX:
            cout << "RX " << ' ';
            break;
        case EVENTTYPE_BTX:
            cout << "BTX " << ' ';
            break;
        case EVENTTYPE_BRX:
            cout << "BRX " << ' ';
            break;
        case EVENTTYPE_DROP:
            cout << "DROP " << ' ';
            break;
        default: {
            int tmp = logEntry[1];
            cout << tmp << endl;
            exit(1);
        }
    }

    u_int64_t stime;
    memcpy(&stime, &logEntry[2], 8);
    cout << stime << ' ';

    u_int32_t dtime;
    memcpy(&dtime, &logEntry[10], 4);
    cout << dtime << ' ';


    switch ( logEntry[14] ) {
        case PKT_TABLE:
            cout << "DVBRCS_PKT_TABLE " << ' ';
            break;
        case PKT_SECTION:
            cout << "PKT_SECTION " << ' ';
            break;
        case PKT_MPEG2_TS:
            cout << "PKT_MPEG2_TS " << ' ';
            break;
        case PKT_DVB_S2:
            cout << "PKT_DVB_S2 " << ' ';
            break;
        case PKT_RCSSYNC:
            cout << "PKT_RCSSYNC " << ' ';
            break;
        case PKT_RCSCSC:
            cout << "PKT_RCSCSC " << ' ';
            break;
        case PKT_ATM:
            cout << "PKT_ATM " << ' ';
            break;
        case PKT_RCSMAC:
            cout << "PKT_RCSMAC " << ' ';
            break;
        case PKT_DVBRCS:
            cout << "PKT_DVBRCS " << ' ';
            break;
        case PKT_RAWDATA:
            cout << "PKT_RAWDATA " << ' ';
            break; 
        case PKT_NONE:
            cout << "PKT_NONE " << ' ';
            break;
        
        
        default: {
            int tmp = logEntry[14];
            cout << tmp << endl;
            exit(1);
        }
    }
#if 0
    u_int16_t src, dst;
    memcpy(&src, &logEntry[15], 2);
    memcpy(&dst, &logEntry[17], 2);
    cout << '<' << src << ' ';
    cout << dst << "> ";
#endif
    u_int16_t phy_src, phy_dst;
    memcpy(&phy_src, &logEntry[19], 2);
    memcpy(&phy_dst, &logEntry[21], 2);
    
    cout << '<' << phy_src << ' ';
    cout << phy_dst << "> ";
    

    u_int64_t pid;
    memcpy(&pid, &logEntry[23], 8);
    cout << pid << ' ';

    u_int32_t plen;
    memcpy(&plen, &logEntry[31], 4);
    cout << plen << ' ';

    u_int16_t retry;
    memcpy(&retry, &logEntry[35], 2);
    cout << retry << ' ';

    switch ( logEntry[37] ) {
        case DROP_COLL:
            cout << "COLL " << ' ';
            break;
        case DROP_CAP:
            cout << "CAP " << ' ';
            break;
        case DROP_DUPX:
            cout << "DUPX " << ' ';
            break;
        case DROP_BER:
            cout << "BER " << ' ';
            break;
        case DROP_RXERR:
            cout << "RXERR " << ' ';
            break;
        case DROP_POW_CS:
            cout << "POW_CS " << ' ';
            break;
        case DROP_POW_RX:
            cout << "POW_RX " << ' ';
            break;
        case DROP_NONE:
            cout << "NONE " << ' ';
            break;
        default: {
            int tmp = logEntry[37];
            cout << tmp << endl;
            exit(1);
        }
    }

    u_char channel;
    int    _channel;
    memcpy(&channel, &logEntry[39], 1);
    _channel = channel;
    cout << _channel << ' ';

    cout << endl;
}

void print_mac802_16(char *logEntry)
{
	cout << "802.16 ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RTX:
			cout << "RTX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_BTX:
			cout << "BTX " << ' ';
			break;
		case EVENTTYPE_BRX:
			cout << "BRX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';


	switch ( logEntry[14] ) {
		case FRAMETYPE_WIMAX_MESH_DATA:
			cout << "WIMAX_MESH_DATA" << ' ';
			break;

		case FRAMETYPE_WIMAX_MESH_NENT:
			cout << "WIMAX_MESH_NENT " << ' ';
			break;
		case FRAMETYPE_WIMAX_MESH_NCFG:
			cout << "WIMAX_MESH_NCFG " << ' ';
			break;
		case FRAMETYPE_WIMAX_MESH_DSCH:
			cout << "WIMAX_MESH_DSCH " << ' ';
			break;
		case FRAMETYPE_WIMAX_MESH_SBCREQ:
			cout << "WIMAX_MESH_SBCREQ " << ' ';
			break;
		case FRAMETYPE_WIMAX_MESH_REGREQ:
			cout << "WIMAX_MESH_REGREQ " << ' ';
			break;
		case FRAMETYPE_WIMAX_MESH_SPONSOR:
			cout << "WIMAX_MESH_SPONSOR " << ' ';
			break;
		case FRAMETYPE_WIMAX_PMP_DBURST:
			cout << "WIMAX_PMP_DBURST " << ' ';
			break;
		case FRAMETYPE_WIMAX_PMP_UBURST:
			cout << "WIMAX_PMP_UBURST " << ' ';
			break;
		case FRAMETYPE_WIMAX_PMP_DLMAP:
			cout << "WIMAX_PMP_DLMAP " << ' ';
			break; 
		case FRAMETYPE_WIMAX_PMP_DLFP:
			cout << "WIMAX_PMP_DLFP " << ' ';
			break;
		case FRAMETYPE_WIMAX_PMP_ULMAP:
			cout << "WIMAX_PMP_ULMAP " << ' ';
			break;


		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);

	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";


	u_int64_t pid;
	memcpy(&pid, &logEntry[23], 8);
	cout << pid << ' ';

	u_int32_t plen;
	memcpy(&plen, &logEntry[31], 4);
	cout << plen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[35], 2);
	cout << retry << ' ';

	switch ( logEntry[37] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_CAP:
			cout << "CAP " << ' ';
			break;
		case DROP_DUPX:
			cout << "DUPX " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_RXERR:
			cout << "RXERR " << ' ';
			break;
		case DROP_POW_CS:
			cout << "POW_CS " << ' ';
			break;
		case DROP_POW_RX:
			cout << "POW_RX " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[37];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_char channel;
	int _channel;
	memcpy(&channel, &logEntry[39], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}

void print_mac802_16e(char *logEntry)
{
	cout << "802.16e ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RTX:
			cout << "RTX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_BTX:
			cout << "BTX " << ' ';
			break;
		case EVENTTYPE_BRX:
			cout << "BRX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';


	switch ( logEntry[14] ) {
		case FRAMETYPE_MobileWIMAX_PMP_DBURST:
			cout << "MobileWIMAX_PMP_DBURST " << ' ';
			break;

		case FRAMETYPE_MobileWIMAX_PMP_UBURST:
			cout << "MobileWIMAX_PMP_UBURST " << ' ';
			break;
		case FRAMETYPE_MobileWIMAX_PMP_DLMAP:
			cout << "MobileWIMAX_PMP_DLMAP " << ' ';
			break;
		case FRAMETYPE_MobileWIMAX_PMP_DLFP:
			cout << "MobileWIMAX_PMP_DLFP " << ' ';
			break;

		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);

	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";

	u_int32_t burstlen;
	memcpy(&burstlen, &logEntry[23], 4);
	cout << burstlen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[27], 2);
	cout << retry << ' ';

	switch ( logEntry[29] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_CAP:
			cout << "CAP " << ' ';
			break;
		case DROP_DUPX:
			cout << "DUPX " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_RXERR:
			cout << "RXERR " << ' ';
			break;
		case DROP_POW_CS:
			cout << "POW_CS " << ' ';
			break;
		case DROP_POW_RX:
			cout << "POW_RX " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[37];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_char channel;
	int _channel;
	memcpy(&channel, &logEntry[30], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}

void print_mac802_16j(char *logEntry)
{
	cout << "802.16j ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RTX:
			cout << "RTX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_BTX:
			cout << "BTX " << ' ';
			break;
		case EVENTTYPE_BRX:
			cout << "BRX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';


	switch ( logEntry[14] ) {
		case FRAMETYPE_MobileRelayWIMAX_PMP_DABURST:
			cout << "MobileRelayWIMAX_PMP_DABURST " << ' ';
			break;

		case FRAMETYPE_MobileRelayWIMAX_PMP_UABURST:
			cout << "MobileRelayWIMAX_PMP_UABURST " << ' ';
			break;
		case FRAMETYPE_MobileRelayWIMAX_PMP_DTBURST:
			cout << "MobileRelayWIMAX_PMP_DTBURST " << ' ';
			break;
		case FRAMETYPE_MobileRelayWIMAX_PMP_URBURST:
			cout << "MobileRelayWIMAX_PMP_URBURST " << ' ';
			break;
		case FRAMETYPE_MobileRelayWIMAX_PMP_DLMAP:
                        cout << "MobileRelayWIMAX_PMP_DLMAP " << ' ';
                        break;
		case FRAMETYPE_MobileRelayWIMAX_PMP_DLFP:
                        cout << "MobileRelayWIMAX_PMP_DLFP " << ' ';
                        break;

		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);

	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";

	u_int32_t burstlen;
	memcpy(&burstlen, &logEntry[23], 4);
	cout << burstlen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[27], 2);
	cout << retry << ' ';

	switch ( logEntry[29] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_CAP:
			cout << "CAP " << ' ';
			break;
		case DROP_DUPX:
			cout << "DUPX " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_RXERR:
			cout << "RXERR " << ' ';
			break;
		case DROP_POW_CS:
			cout << "POW_CS " << ' ';
			break;
		case DROP_POW_RX:
			cout << "POW_RX " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[37];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_char channel;
	int _channel;
	memcpy(&channel, &logEntry[30], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}

void print_mac802_16j_nt(char *logEntry)
{
	cout << "802.16j non-transparent ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RTX:
			cout << "RTX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_BTX:
			cout << "BTX " << ' ';
			break;
		case EVENTTYPE_BRX:
			cout << "BRX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
				 int tmp = logEntry[1];
				 cout << tmp << endl;
				 exit(1);
			 }
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';


	switch ( logEntry[14] ) {
		case FRAMETYPE_MR_WIMAX_NT_PMP_DLAccessBURST:
			cout << "MR_WIMAX_NT_PMP_DLAccessBURST " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_ULAccessBURST:
			cout << "MR_WIMAX_NT_PMP_ULAccessBURST " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_DLRelayBURST:
			cout << "MR_WIMAX_NT_PMP_DLRelayBURST " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_ULRelayBURST:
			cout << "MR_WIMAX_NT_PMP_ULRelayBURST " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_DLMAP:
			cout << "MR_WIMAX_NT_PMP_DLMAP " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_RMAP:
			cout << "MR_WIMAX_NT_PMP_RMAP " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_DLFP:
			cout << "MR_WIMAX_NT_PMP_DLFP " << ' ';
			break;

		case FRAMETYPE_MR_WIMAX_NT_PMP_RzonePrefix:
			cout << "MR_WIMAX_NT_PMP_RzonePrefix " << ' ';
			break;

		default: {
				 int tmp = logEntry[14];
				 cout << tmp << endl;
				 exit(1);
			 }
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);

	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";

	u_int32_t burstlen;
	memcpy(&burstlen, &logEntry[23], 4);
	cout << burstlen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[27], 2);
	cout << retry << ' ';

	switch ( logEntry[29] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_CAP:
			cout << "CAP " << ' ';
			break;
		case DROP_DUPX:
			cout << "DUPX " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_RXERR:
			cout << "RXERR " << ' ';
			break;
		case DROP_POW_CS:
			cout << "POW_CS " << ' ';
			break;
		case DROP_POW_RX:
			cout << "POW_RX " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
				 int tmp = logEntry[37];
				 cout << tmp << endl;
				 exit(1);
			 }
	}

	u_char channel;
	int _channel;
	memcpy(&channel, &logEntry[30], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}

void
print_ether802_11(char *logEntry)
{
	cout << "802.11 ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RTX:
			cout << "RTX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_BTX:
			cout << "BTX " << ' ';
			break;
		case EVENTTYPE_BRX:
			cout << "BRX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';

	switch ( logEntry[14] ) {
		case FRAMETYPE_DATA:
			cout << "DATA " << ' ';
			break;
		case FRAMETYPE_RTS:
			cout << "RTS " << ' ';
			break;
		case FRAMETYPE_CTS:
			cout << "CTS " << ' ';
			break;
		case FRAMETYPE_ACK:
			cout << "ACK " << ' ';
			break;
		case FRAMETYPE_BEACON:
			cout << "BCON " << ' ';
			break;
		case FRAMETYPE_ASSQ:
			cout << "ASSQ " << ' ';
			break;
		case FRAMETYPE_ASSR:
			cout << "ASSR " << ' ';
			break;
		case FRAMETYPE_REASSQ:
			cout << "REASSQ " << ' ';
			break;
		case FRAMETYPE_REASSR:
			cout << "REASSR " << ' ';
			break;
		case FRAMETYPE_DISASS:
			cout << "DISASS " << ' ';
			break;
		case FRAMETYPE_PROBQ:
			cout << "PROBQ " << ' ';
			break;
		case FRAMETYPE_PROBR:
			cout << "PROBR " << ' ';
			break;
                  case FRAMETYPE_ACTION:
                          cout << "ACTION " << ' ';
                          break;
                  case FRAMETYPE_QOS_DATA:
                          cout << "QoS_DATA " << ' ';
                          break;
                  case FRAMETYPE_QOS_ACK:
                          cout << "QoS_ACK " << ' ';
                          break;
                  case FRAMETYPE_QOS_POLL:
                          cout << "QoS_POLL " << ' ';
                          break;
                  case FRAMETYPE_QOS_NULL:
                          cout << "QoS_NULL " << ' ';
                          break;
		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst, mac_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);
	memcpy(&mac_dst, &logEntry[23], 2);
	cout << '<' << phy_src << ' ';
	cout << phy_dst << ' ';
	cout << mac_dst << "> ";

	u_int64_t pid;
	memcpy(&pid, &logEntry[25], 8);
	cout << pid << ' ';

	u_int32_t plen;
	memcpy(&plen, &logEntry[33], 4);
	cout << plen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[37], 2);
	cout << retry << ' ';

	switch ( logEntry[39] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_CAP:
			cout << "CAP " << ' ';
			break;
		case DROP_DUPX:
			cout << "DUPX " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_RXERR:
			cout << "RXERR " << ' ';
			break;
		case DROP_POW_CS:
			cout << "POW_CS " << ' ';
			break;
		case DROP_POW_RX:
			cout << "POW_RX " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[39];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_char channel;
	int    _channel;
	memcpy(&channel, &logEntry[40], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}


void
print_ether802_3(char *logEntry)
{
	cout << "802.3 ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RTX:
			cout << "RTX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_BTX:
			cout << "BTX " << ' ';
			break;
		case EVENTTYPE_BRX:
			cout << "BRX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';

	switch ( logEntry[14] ) {
		case FRAMETYPE_DATA:
			cout << "DATA " << ' ';
			break;
		case FRAMETYPE_RTS:
			cout << "RTS " << ' ';
			break;
		case FRAMETYPE_CTS:
			cout << "CTS " << ' ';
			break;
		case FRAMETYPE_ACK:
			cout << "ACK " << ' ';
			break;
		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);
	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";

	u_int64_t pid;
	memcpy(&pid, &logEntry[23], 8);
	cout << pid << ' ';

	u_int32_t plen;
	memcpy(&plen, &logEntry[31], 4);
	cout << plen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[35], 2);
	cout << retry << ' ';

	switch ( logEntry[37] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_CAP:
			cout << "CAP " << ' ';
			break;
		case DROP_DUPX:
			cout << "DUPX " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_RXERR:
			cout << "RXERR " << ' ';
			break;
		case DROP_POW_CS:
			cout << "POW_CS " << ' ';
			break;
		case DROP_POW_RX:
			cout << "POW_RX " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[37];
			cout << tmp << endl;
			exit(1);
		}
	}

	cout << endl;
}

void
print_optical_phy(char *logEntry)
{
	cout << "OPHY ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';

	switch ( logEntry[14] ) {
		case FRAMETYPE_OBS_DATA:
			cout << "OBS_DATA " << ' ';
			break;
		case FRAMETYPE_OBS_CTRL_NORMAL:
			cout << "OBS_CTL_NOM " << ' ';
			break;
		case FRAMETYPE_OBS_CTRL_SWITCH:
			cout << "OBS_CTL_SW " << ' ';
			break;
		case FRAMETYPE_OPTICAL_LP:
			cout << "O_LP " << ' ';
			break;
		case FRAMETYPE_OPTICAL_DATA:
			cout << "O_DATA " << ' ';
			break;
		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);
	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";

	u_int64_t pid;
	memcpy(&pid, &logEntry[23], 8);
	cout << pid << ' ';

	u_int32_t plen;
	memcpy(&plen, &logEntry[31], 4);
	cout << plen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[35], 2);
	cout << retry << ' ';

	switch ( logEntry[37] ) {
		case DROP_COLL:
			cout << "COLL " << ' ';
			break;
		case DROP_BER:
			cout << "BER " << ' ';
			break;
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[37];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_char channel;
	int    _channel;
	memcpy(&channel, &logEntry[38], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}


void
print_gprs(char *logEntry)
{
	cout << "GPRS ";

	switch ( logEntry[1] ) {
		case EVENTTYPE_TX:
			cout << "TX " << ' ';
			break;
		case EVENTTYPE_RX:
			cout << "RX " << ' ';
			break;
		case EVENTTYPE_DROP:
			cout << "DROP " << ' ';
			break;
		default: {
			int tmp = logEntry[1];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int64_t stime;
	memcpy(&stime, &logEntry[2], 8);
	cout << stime << ' ';

	u_int32_t dtime;
	memcpy(&dtime, &logEntry[10], 4);
	cout << dtime << ' ';

	switch ( logEntry[14] ) {
		case FRAMETYPE_GPRS_DATA:
			cout << "GPRS_DATA " << ' ';
			break;
		case FRAMETYPE_GPRS_ACCESS:
			cout << "GPRS_ACCESS " << ' ';
			break;
		case FRAMETYPE_GPRS_DUMMY:
			cout << "GPRS_DUMMY " << ' ';
			break;
		case FRAMETYPE_GPRS_CTL:
			cout << "GPRS_CTL " << ' ';
			break;
		default: {
			int tmp = logEntry[14];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_int16_t src, dst;
	memcpy(&src, &logEntry[15], 2);
	memcpy(&dst, &logEntry[17], 2);
	cout << '<' << src << ' ';
	cout << dst << "> ";

	u_int16_t phy_src, phy_dst;
	memcpy(&phy_src, &logEntry[19], 2);
	memcpy(&phy_dst, &logEntry[21], 2);
	cout << '<' << phy_src << ' ';
	cout << phy_dst << "> ";

	u_int64_t pid;
	memcpy(&pid, &logEntry[23], 8);
	cout << pid << ' ';

	u_int32_t plen;
	memcpy(&plen, &logEntry[31], 4);
	cout << plen << ' ';

	u_int16_t retry;
	memcpy(&retry, &logEntry[35], 2);
	cout << retry << ' ';

	switch ( logEntry[37] ) {
		case DROP_NONE:
			cout << "NONE " << ' ';
			break;
		default: {
			int tmp = logEntry[37];
			cout << tmp << endl;
			exit(1);
		}
	}

	u_char channel;
	int    _channel;
	memcpy(&channel, &logEntry[38], 1);
	_channel = channel;
	cout << _channel << ' ';

	cout << endl;
}


int
main(int argc, char *argv[])
{

        if (argc != 2) {
          printf("Usage: %s file.ptr\n", argv[0]);
          exit(0);
        }
	FILE *log = fopen(argv[1], "r");
	
	char logEntry[100];
	while ( fread(logEntry, 44, 1, log) ) {
		if ( logEntry[0] == PROTO_802_3 ) 
			print_ether802_3(logEntry);
		else
		if ( logEntry[0] == PROTO_802_11 )
			print_ether802_11(logEntry);
		else
		if ( logEntry[0] == PROTO_OPHY )
			print_optical_phy(logEntry);
		else
		if ( logEntry[0] == PROTO_GPRS )
			print_gprs(logEntry);
		else
		if ( logEntry[0] == PROTO_802_16 )
			print_mac802_16(logEntry);
		else
		if ( logEntry[0] == PROTO_SAT_DVBRCS )
			print_sat_dvbrcs(logEntry);
		else
		if ( logEntry[0] == PROTO_802_16e )
			print_mac802_16e(logEntry);
		else
		if ( logEntry[0] == PROTO_802_16j )
			print_mac802_16j(logEntry);
		else
		if ( logEntry[0] == PROTO_802_16j_NT )
			print_mac802_16j_nt(logEntry);
		else
			exit(1);
	}

	fclose(log);
}
