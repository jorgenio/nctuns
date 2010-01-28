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

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <packet.h>
#include <nodetype.h>
#include <maptable.h>
#include <regcom.h>
#include <nctuns_api.h>
#include <mac/mac8023.h>
#include <ip.h>
#include <tcp.h>

#include <arpa/inet.h>
//#include <fstream>

#include <phyInfo.h>

extern RegTable                 RegTable_;

MODULE_GENERATOR(mac8023);

mac8023::mac8023(u_int32_t type, u_int32_t id, struct plist *pl, const char *name)
                : mac(type, id, pl, name)
{
	/* mac has two buffer -
	 * send buffer: pktRetx(for retransmit)
	 * recv buffer: pktRx(for complete recv)
	 */
	pktRx = pktRetx = pktJam = 0;
	retxCnt = 1;
	busy = false;
	set(MAC_IDLE); //half duplex
	/* we need to know IFSTimer starting time
	 * , so we can calculate from IFSTimer starting
	 * time and current time to know how long IFSTimer
	 * counts.
	 */
	IFSStartTick = 0;

}


mac8023::~mac8023() {

}


void mac8023::txScheduler(ePacket_ *pkt) { 

	assert(pkt);
	assert(busy == false);	

	u_int64_t txTime = scheduleTime(pkt, *bw_, MAC_SEND);

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(mac8023, txHandler);
	txTimer.setCallOutObj(this, type);
	txTimer.start(txTime, 0);

	busy = true;

	if ( ptr_log ) {
		/* send start log */
		sslog(pkt, retxCnt - 1, txTime);
	}
}


void mac8023::txHandler() {

	assert(pktRetx);

	if ( ptr_log ) {
		/* send end log */
		selog(pktRetx, SuccessTX, DROP_NONE, retxCnt - 1);
	}

	if( _log && !strcasecmp(_log, "on") ) {
		Packet *pkt_ = (Packet *)pktRetx->DataInfo_;
		struct ether_header *eh = 
			(struct ether_header *)pkt_->pkt_get();

		if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) ) {			
			if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) )
				NumBroOut++;
			if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) )
				NumBroInOut++;
		}
		else {
			if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) )
				NumUniOut++;
			if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
				NumUniInOut++;
		}

		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) )
			OutThrput += (double)pkt_->pkt_getlen() / 1000.0;
		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
			InOutThrput += (double)pkt_->pkt_getlen() / 1000.0;
	}

	busy = false;
	retxReset();	

	resume();

}


void mac8023::rxScheduler(ePacket_ *pkt) {

	Packet			*p;
	struct phyInfo		*phyinfo;
	
	assert(pkt&&(p=(Packet *)pkt->DataInfo_));
	phyinfo = (struct phyInfo *)p->pkt_getinfo("PHY");

	u_int64_t rxTime = scheduleTime(pkt, phyinfo->TxBW, MAC_RECV);

	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(mac8023, rxHandler);
	rxTimer.setCallOutObj(this, type);
	rxTimer.start(rxTime, 0);

	if ( ptr_log ) {
		/* recv start log */
		rslog(pkt);
	}
}

	
void mac8023::rxHandler() {

	assert(pktRx);
	ePacket_ *pktRxTmp = pktRx;
	pktRx = 0;

	recvComplete(pktRxTmp);
}


void mac8023::retxReset() {

	if ( busy ) {
		assert(busy);
		assert(pktRetx);
		retxTimer.cancel();
		busy = false;
		if ( pktRetx ) freePacket(pktRetx);
		pktRetx = 0;
	}
	retxCnt = 1;
	if ( pktRetx ) freePacket(pktRetx);
	pktRetx = 0;
}


void mac8023::retxScheduler() {

	assert(pktRetx);
	assert(busy == false);

	if ( retxCnt < IEEE_8023_ALIMIT ) {
		int k = min(retxCnt, IEEE_8023_BLIMIT);
		int r = (random() % ((long)pow(2, k)));
		u_int64_t delay = 
			scheduleTime((r * IEEE_8023_SLOT), *bw_);

		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(mac8023, retxHandler);
	        retxTimer.setCallOutObj(this, type);
		retxTimer.start(delay, 0);

		busy = true;
		return;
	}
	
	retxReset();
	resume();
}


void mac8023::retxHandler() {

	assert(pktRetx);
	assert(busy == true);
	busy = false;
	++retxCnt;

	_send(pktRetx);
}

	
void mac8023::jamScheduler(int bits) {

	u_int64_t jamDelay = scheduleTime(bits, *bw_);
	
	assert(busy == false);
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(mac8023, jamHandler);
	jamTimer.setCallOutObj(this, type);
	jamTimer.start(jamDelay, 0);

	//busy == true;
}


void mac8023::jamHandler() {

	assert(pktRetx);
	assert(!retxTimer.busy_);
	busy = false;
	set(MAC_IDLE);

	retxScheduler();
}


void mac8023::IFSScheduler(double IFS_) {

	u_int64_t IFS = scheduleTime(IFS_);
	IFSStartTick = GetCurrentTime();
		
	assert(busy == false);
	BASE_OBJTYPE(type);
	type = POINTER_TO_MEMBER(mac8023, IFSHandler);
	IFSTimer.setCallOutObj(this, type);
	IFSTimer.start(IFS, 0);

	busy = true;
}


void mac8023::IFSHandler() {

	busy = false;
	__send(pktRetx);

}


int mac8023::recv(ePacket_ *pkt) {


	/* full duplex */
	if ( mode_ && !strcasecmp(mode_, "full") ) {
		return(mac::recv(pkt));
	}

	/* half duplex */
	if ( state(MAC_IDLE) ) {

		set(MAC_RECV);
		bufRxPkt(pkt);
		rxScheduler(pkt);
	}
	else {
		collision(pkt);
	}

	return(1);
}
       
	
int mac8023::send(ePacket_ *pkt) {

/* 
 * hwchu: 2005/06/13
 *   There is a frame check sequence field (FCS) and an imposed minimum
 *   frame length in the IEEE 802.3 standard, which we do not consider
 *   before. As a remedy, we treat both as padding bytes and manually
 *   adjust the frame length.
 */
	Packet *p = (Packet *)pkt->DataInfo_;
	char *ptr = p->pkt_getinfo("PADDING");

	if ((ptr == NULL) || (*ptr == 0)) {
		//printf("send pktlen = %d => ", p->pkt_getlen());
		/* Padding bytes can only be calculated once */
		int padding = IEEE_8023_FCS_LEN;
		int pktlen = p->pkt_getlen() + padding;

		if (pktlen < IEEE_8023_MINFRAME) {
			padding += (IEEE_8023_MINFRAME - pktlen);
			pktlen = IEEE_8023_MINFRAME;
		}

		p->pkt_setlen(pktlen);
		p->pkt_addinfo("PADDING", (char *)&padding, sizeof(padding));
		//printf("%d\n", p->pkt_getlen());
	}
// end of modification: 2005/06/13

	if ( mode_ && !strcasecmp(mode_, "full") ) {
		return(mac::send(pkt));
	}

	if ( pktRetx ) {
		_send(pktRetx);
		return(-1);
	}
	
	bufRetxPkt(pkt);
	_send(pkt);
	
	return(1);
}


int mac8023::_send(ePacket_ *pkt) {

	/* carrier sense */
	if ( !state(MAC_IDLE) || IFSTimer.busy_ || retxTimer.busy_ )
		return(1);

	assert(!state(MAC_SEND));

	set(MAC_IFS);
	IFSScheduler(IEEE_8023_IFS);

	return 1;
}


int mac8023::__send(ePacket_ *pkt) {
		
	set(MAC_SEND);
	txScheduler(pkt);

	ePacket_ *pkt_ = pkt_copy(pkt);
	assert(put(pkt_, sendtarget_));

	return(1);
}


void mac8023::collision(ePacket_ *pkt) {


	if( _log && !strcasecmp(_log, "on") ) {
		if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
			NumColl+=2;
			if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
				NumDrop+=2;
		}
	}

	freePacket(pkt);
	
	switch(state()) {
			
		case MAC_SEND: {
			if ( txTimer.busy_ ) {
				assert(busy);
				txTimer.cancel();
				busy = false;
			}

			if ( ptr_log ) {
				/* send end log with collision */
				selog(pktRetx, DropTX,
						DROP_COLL, retxCnt - 1);
			}

			set(MAC_IDLE);
			retxScheduler();

			break;
		}
					   
		case MAC_RECV: {
			if ( rxTimer.busy_ ) {
				rxTimer.cancel();
			}

			if ( pktRx ) {
				if ( ptr_log ) {
					/* recv end log with collision */
					relog(pktRx, DropRX, DROP_COLL);
				}
				freePacket(pktRx);
				pktRx = 0;
			}

			set(MAC_IDLE);
			
			break;
		}

		/* from IEEE8023 spec -
		 * front 2/3 interval: reset IFSTimer and
		 *		       back to carrier sense.
		 * back  1/3 interval: ignore carrier sense
		 *		       and keep counting IFSTimer.
		 */
		case MAC_IFS: {
			u_int64_t IFS = scheduleTime(IEEE_8023_IFS);
			if ( (GetCurrentTime() - IFSStartTick)
					<= (IFS * 2 / 3) ) {
				IFSTimer.cancel();
				busy = false;
				set(MAC_IDLE);
				_send(pktRetx);
			} else
			if ( (GetCurrentTime() - IFSStartTick) <= IFS ) {
			} else
				assert(0);
			
			break;
		}

		/* sending jam -
		 * ignore any collision
		 */
		case MAC_JAM: {
			break;	
		}
			
		default:
			fprintf(stderr, "node%d no such mac state\n", get_nid());
			assert(0);
	}
}


void mac8023::recvComplete(ePacket_ *pkt) {

	assert(rxTimer.busy_ == 0);
	assert(txTimer.busy_ == 0);

	if ( CRCerror(pkt) ) {
		if ( ptr_log ) {
			/* recv end log with bit error */
			relog(pkt, DropRX, DROP_BER);
		}

		if( _log && !strcasecmp(_log, "on") ) {
			if ( LogDrop && (!strcasecmp(LogDrop, "on")) )
				NumDrop++;
		}

		freePacket(pkt);
	}
	else {

		if ( ptr_log ) {
			/* recv end log */
			relog(pkt, SuccessRX, DROP_NONE);
		}

		Packet *pkt_ = (Packet *)pkt->DataInfo_;

		/* discard jam pkt(pktlen == 4) */
		if ( pkt_->pkt_getlen() == 4 ) {
			freePacket(pkt);
		}
		else {
			struct ether_header	*eh;
			if ( pktFilterOpt && !strcasecmp(pktFilterOpt, "off") ) {
				if( _log && !strcasecmp(_log, "on") ) {
					eh = (struct ether_header *)pkt_->pkt_get();

					if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) ) {
						if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) )
							NumBroIn++;
						if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) )
							NumBroInOut++;
					}
					else {
						if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) )
							NumUniIn++;
						if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
							NumUniInOut++;
					}

					if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) )
						InThrput += (double)pkt_->pkt_getlen() / 1000.0;
					if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
						InOutThrput += (double)pkt_->pkt_getlen() / 1000.0;
				}
				
				/* 
				 * hwchu: 2005/06/13
				 *   There is a frame check sequence field (FCS) and an imposed minimum
				 *   frame length in the IEEE 802.3 standard, which we do not consider
				 *   before. As a remedy, we treat both as padding bytes and manually
				 *   adjust the frame length.
				*/
				Packet *p = (Packet *)pkt->DataInfo_;
				int *padding = (int *)p->pkt_getinfo("PADDING");

				if ((padding != NULL) && (*padding != 0)) {
					/* Padding bytes can only be calculated once */
					int pktlen = p->pkt_getlen() - (*padding);

					*padding = 0;
					p->pkt_setlen(pktlen);
					//printf("recv2 pktlen = %d\n", p->pkt_getlen());
				}
				// end of modification: 2005/06/13

				assert(put(pkt, recvtarget_));
			}
			else if ( !pktFilter(pkt) ) {
				if( _log && !strcasecmp(_log, "on") ) {
					eh = (struct ether_header *)pkt_->pkt_get();

					if( !bcmp(eh->ether_dhost,ETHER_BROADCAST, 6) ) {
						if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) )
							NumBroIn++;
						if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) )
							NumBroInOut++;
					}
					else {
						if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) )
							NumUniIn++;
						if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) )
							NumUniInOut++;
					}

					if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) )
						InThrput += (double)pkt_->pkt_getlen() / 1000.0;
					if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) )
						InOutThrput += (double)pkt_->pkt_getlen() / 1000.0;
				}

				/* 
				 * hwchu: 2005/06/13
				 *   There is a frame check sequence field (FCS) and an imposed minimum
				 *   frame length in the IEEE 802.3 standard, which we do not consider
				 *   before. As a remedy, we treat both as padding bytes and manually
				 *   adjust the frame length.
				*/
				Packet *p = (Packet *)pkt->DataInfo_;
				int *padding = (int *)p->pkt_getinfo("PADDING");

				if ((padding != NULL) && (*padding != 0)) {
					/* Padding bytes can only be calculated once */
					int pktlen = p->pkt_getlen() - (*padding);

					*padding = 0;
					p->pkt_setlen(pktlen);
					//printf("recv2 pktlen = %d\n", p->pkt_getlen());
				}
				// end of modification: 2005/06/13

				assert(put(pkt, recvtarget_));
			}
			else
				freePacket(pkt);
		}
	}

	resume();
}


void mac8023::resume() {

	assert(pktRx == 0);
	assert(rxTimer.busy_ == 0);
	assert(txTimer.busy_ == 0);
	assert(IFSTimer.busy_ == 0);

	set(MAC_IDLE);

	if ( pktRetx ) {
		if ( retxTimer.busy_ == 0 ) {
			_send(pktRetx);
		}
	}
}


int mac8023::init() {
	char	*FILEPATH;

	if ( _log && (!strcasecmp(_log, "on")) ) {

		/* open log files */
		if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(UniInLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						UniInLogFileName);
			UniInLogFILE = fopen(FILEPATH,"w+");
			assert(UniInLogFILE);
			free(FILEPATH);
		}

		if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(UniOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						UniOutLogFileName);
			UniOutLogFILE = fopen(FILEPATH,"w+");
			assert(UniOutLogFILE);
			free(FILEPATH);
		}

		if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(UniInOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						UniInOutLogFileName);
			UniInOutLogFILE = fopen(FILEPATH,"w+");
			assert(UniInOutLogFILE);
			free(FILEPATH);
		}

		if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(BroInLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						BroInLogFileName);
			BroInLogFILE = fopen(FILEPATH,"w+");
			assert(BroInLogFILE);
			free(FILEPATH);
		}

		if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(BroOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						BroOutLogFileName);
			BroOutLogFILE = fopen(FILEPATH,"w+");
			assert(BroOutLogFILE);
			free(FILEPATH);
		}

		if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(BroInOutLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						BroInOutLogFileName);
			BroInOutLogFILE = fopen(FILEPATH,"w+");
			assert(BroInOutLogFILE);
			free(FILEPATH);
		}

		if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(CollLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						CollLogFileName);
			CollLogFILE = fopen(FILEPATH,"w+");
			assert(CollLogFILE);
			free(FILEPATH);
		}

		if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(DropLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						DropLogFileName);
			DropLogFILE = fopen(FILEPATH,"w+");
			assert(DropLogFILE);
			free(FILEPATH);
		}

		if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(InThrputLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						InThrputLogFileName);
			InThrputLogFILE = fopen(FILEPATH,"w+");
			assert(InThrputLogFILE);
			free(FILEPATH);
		}

		if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(OutThrputLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						OutThrputLogFileName);
			OutThrputLogFILE = fopen(FILEPATH,"w+");
			assert(OutThrputLogFILE);
			free(FILEPATH);
		}

		if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) ) {
			FILEPATH = (char*)malloc(strlen(GetConfigFileDir()) + 
					strlen(InOutThrputLogFileName) + 1);
			sprintf(FILEPATH, "%s%s", GetConfigFileDir(),
						InOutThrputLogFileName);
			InOutThrputLogFILE = fopen(FILEPATH,"w+");
			assert(InOutThrputLogFILE);
			free(FILEPATH);
		}

		/* convert log interval to tick */
		SEC_TO_TICK(logIntervalTick, logInterval);

		/* set timer to log information periodically */
		BASE_OBJTYPE(type);
		type = POINTER_TO_MEMBER(mac8023, log);
		_logTimer.setCallOutObj(this, type);
		_logTimer.start(logIntervalTick, logIntervalTick);		
	}

	return(mac::init());
}

 
int mac8023::log() {
	double		cur_time;

	cur_time = (double)(GetCurrentTime() * TICK) / 1000000000.0;

	if ( LogUniIn && (!strcasecmp(LogUniIn, "on")) ) {
		fprintf(UniInLogFILE, "%.3f\t%llu\n", cur_time, NumUniIn);
		fflush(UniInLogFILE);
		NumUniIn = 0;
	}

	if ( LogUniOut && (!strcasecmp(LogUniOut, "on")) ) {
		fprintf(UniOutLogFILE, "%.3f\t%llu\n", cur_time, NumUniOut);
		fflush(UniOutLogFILE);
		NumUniOut = 0;
	}

	if ( LogUniInOut && (!strcasecmp(LogUniInOut, "on")) ) {
		fprintf(UniInOutLogFILE, "%.3f\t%llu\n", cur_time, NumUniInOut);
		fflush(UniInOutLogFILE);
		NumUniInOut = 0;
	}

	if ( LogBroIn && (!strcasecmp(LogBroIn, "on")) ) {
		fprintf(BroInLogFILE, "%.3f\t%llu\n", cur_time, NumBroIn);
		fflush(BroInLogFILE);
		NumBroIn = 0;
	}

	if ( LogBroOut && (!strcasecmp(LogBroOut, "on")) ) {
		fprintf(BroOutLogFILE, "%.3f\t%llu\n", cur_time, NumBroOut);
		fflush(BroOutLogFILE);
		NumBroOut = 0;
	}

	if ( LogBroInOut && (!strcasecmp(LogBroInOut, "on")) ) {
		fprintf(BroInOutLogFILE, "%.3f\t%llu\n", cur_time, NumBroInOut);
		fflush(BroInOutLogFILE);
		NumBroInOut = 0;
	}

	if ( LogColl && (!strcasecmp(LogColl, "on")) ) {
		fprintf(CollLogFILE, "%.3f\t%llu\n", cur_time, NumColl);
		fflush(CollLogFILE);
		NumColl = 0;
	}

	if ( LogDrop && (!strcasecmp(LogDrop, "on")) ) {
		fprintf(DropLogFILE, "%.3f\t%llu\n", cur_time, NumDrop);
		fflush(DropLogFILE);
		NumDrop = 0;
	}

	InThrput /= logInterval;
	OutThrput /= logInterval;
	InOutThrput /= logInterval;

	if ( LogInThrput && (!strcasecmp(LogInThrput, "on")) ) {
		fprintf(InThrputLogFILE, "%.3f\t%.3f\n", cur_time, InThrput);
		fflush(InThrputLogFILE);
		InThrput = 0;
	}

	if ( LogOutThrput && (!strcasecmp(LogOutThrput, "on")) ) {
		fprintf(OutThrputLogFILE, "%.3f\t%.3f\n", cur_time, OutThrput);
		fflush(OutThrputLogFILE);
		OutThrput = 0;
	}

	if ( LogInOutThrput && (!strcasecmp(LogInOutThrput, "on")) ) {
		fprintf(InOutThrputLogFILE, "%.3f\t%.3f\n", cur_time, InOutThrput);
		fflush(InOutThrputLogFILE);
		InOutThrput = 0;
	}

	return(1);
}
