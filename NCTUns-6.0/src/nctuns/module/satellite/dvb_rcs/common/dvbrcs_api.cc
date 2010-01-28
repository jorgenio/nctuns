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
#include <assert.h>
#include <string.h>
#include <nctuns_api.h>
#include <satellite/dvb_rcs/common/dvb_pkt.h>
#include "dvbrcs_api.h"
#include "si_config.h"

/*---------------------------------------------------------------------------
 * dvb_pkt_copy()
 *
 * Duplicate an exist packet.
 *
 * Arguments:
 *      src             a pointer to a ePacket_ structure(event structure),
 *                      this packet is the packet which desired to be
 *                      duplicated.
 *
 * Returns:
 *      a pointer to a new duplicate-packet.
 *      NULL for replicate fail.
 *
 * Side effects:
 *      a new duplicate packet will be generated.
 *---------------------------------------------------------------------------*/
ePacket_ *dvb_pkt_copy(ePacket_ *src)
{

        ePacket_                *dst;

        if (src != NULL) {
                dst = createEvent();
                dst->timeStamp_ = src->timeStamp_;
                dst->perio_ = src->perio_;
                dst->priority_ = src->priority_;
                dst->calloutObj_ = src->calloutObj_;
                dst->func_ = src->func_;
		if (src->DataInfo_)
                	dst->DataInfo_ = (void *)(((Dvb_pkt *)(src->DataInfo_))->pkt_copy());
		else
			dst->DataInfo_ = NULL;
                dst->flag = src->flag;

                return(dst);
        }
        return(NULL);
}

/*---------------------------------------------------------------------------
 * dvb_freePacket()
 *
 * Release an exist-packet.
 *
 * Arguments:
 *      pkt             a pointer to ePacket_ structure(event structure).
 *
 * Returns:
 *      1               success
 *      < 0             failure
 *
 * Side effects:
 *      an exist-packet will be released.
 *---------------------------------------------------------------------------*/
int dvb_freePacket(ePacket_ *pkt) {

        Dvb_pkt                  *p;

        if (pkt != NULL) {
                p = (Dvb_pkt *)pkt->DataInfo_;
                if (p)
                        delete p;

                free(pkt);
                return(1);
        }
        return(-1);
}

/*---------------------------------------------------------------------------
 * calculate_bit_error()
 *
 * Release an exist-packet.
 *
 * Arguments:
 *      data            raw data
 *      len             raw data length
 *      ber             bit error rate
 *
 * Return:
 *      total error bit numbers
 *
 * Side effects:
 *      Make an exist-raw_data have some bit error depend on bit error rate
 *---------------------------------------------------------------------------*/
int calculate_bit_error(void *data, int len, double ber)
{
	unsigned int nf_err_bits, err_bit_loc, num;

	/*
	 * Compute the number of error bits in the frame.
	 */
	num = nf_err_bits = (unsigned int)(len * ber);
	for ( ; nf_err_bits ; nf_err_bits--) {
		/*
		 * Compute the location of the error bit.
		 */
		err_bit_loc = rand() % len;
		/*
		 * Flip the error bit.
		 */
		((uint8_t*)data)[err_bit_loc / 8] ^= 0x80 >> (err_bit_loc % 8);
	}

	return num;
}

/*---------------------------------------------------------------------------
 * is_forward_control()
 *
 * Check this pid of forward packet whether be control packet
 *
 * Arguments:
 *      pkt             Dvb_pkt pointer
 *
 * Return:
 *      1               true
 *      0               false
 *---------------------------------------------------------------------------*/
int is_forward_control(Dvb_pkt *pkt)
{
	switch (pkt->pkt_getfwdinfo()->pid) {
		case PAT_PID:
		case CAT_PID:
		case TSDT_PID:
		case NIT_PID: //the same with BAT
		case BAT_PID:
		case EIT_PID: //the same with CIT
		case RST_PID:
		case TDT_PID: //the same with TOT
		case RNT_PID:
		case DIT_PID:
		case SIT_PID:
		case RMT_PID:
		case SCT_PID:
		case FCT_PID:
		case TCT_PID:
		case SPT_PID:
		case CMT_PID:
		case TBTP_PID:
		case PCR_PKT_PID:
		case TX_TABLE_PID:
		case TIM_PID:
		case INT_PID:
		case INT_PMT_PID:
		case TRF_PMT_PID:
			return 1;
		default:
			return 0;
	}
}
