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
#include "logHeap.h"
#include "logpack.h"
#include <nctuns_api.h>
#include <iostream>
#include <nodetype.h>
#include <stdio.h>
#include <gbind.h>

/*======================================*
 *========= heap about =================*
 *======================================*/
logList llist;
struct mac802_3_log *mac8023log, *_mac8023log;
struct mac802_11_log *mac80211log, *_mac80211log;
struct mac802_16_log *mac80216log, *_mac80216log;
struct mac802_16e_log *mac80216elog, *_mac80216elog;
struct mac802_16j_log *mac80216jlog, *_mac80216jlog;
struct mac802_16j_NT_log *mac80216j_NT_log, *_mac80216j_NT_log;
struct ophy_log *ophylog, *_ophylog;
struct gprs_log *gprslog, *_gprslog;
struct dvbrcs_log *dvbrcslog, *_dvbrcslog;
u_int32_t diff;
u_char tmp_proto, tmp_event, pad[10];
logChain *logchain;

priority_queue < logEvent *, vector < logEvent * >, logComp > logQ;

#if IPC
extern logpack *logpack_;
#endif

//#define LOG_FUNCTION_DEBUG 1

int DequeueLogFromHeap(Event_ * ep)
{

	logEvent *logep;
	u_int64_t time;

	if (ptrlogFileOpenFlag) {

#ifdef LOG_FUNCTION_DEBUG
		printf("LogFunction is open.\n");
#endif

		if (!logQ.empty()) {

#ifdef LOG_FUNCTION_DEBUG
			printf("LogQ has something to log.\n");
#endif

			time = GetCurrentTime();
			for (logep = (logEvent *)logQ.top();
			     logep->Time <= time; logep = (logEvent *)logQ.top()) {

#ifdef LOG_FUNCTION_DEBUG
				printf("Start Log Entry.\n");
#endif

				pkt_trace_log(logep);
				logQ.pop();

				if (logQ.empty())
					break;
			}
		}

		MILLI_TO_TICK(time, 100);
		if (GetSimulationTime() - GetCurrentTime() <= time) {
			if (GetSimulationTime() - GetCurrentTime() <= 2)
				return 1;

			setEventTimeStamp(ep, GetSimulationTime() - 2, 0);
			scheduleInsertEvent(ep);
			return 1;
		}

		MILLI_TO_TICK(time, 100);
		setEventTimeStamp(ep, GetCurrentTime() + time, 0);
		scheduleInsertEvent(ep);
	}

	return 1;
}

void pkt_trace_log(logEvent *logep)
{

	if (logep->PROTO == PROTO_802_3) {
		mac8023log = (struct mac802_3_log *)logep->log;

		if (mac8023log->Event == StartTX || mac8023log->Event == StartRX) {
			logchain = new logChain[1];
			logchain->PROTO = PROTO_802_3;
			logchain->log = mac8023log;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (mac8023log->Event == SuccessTX || mac8023log->Event == DropTX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_802_3) {
					continue;
				}

				_mac8023log = (struct mac802_3_log *)logchain->log;

				if (_mac8023log->Event == StartTX &&
				    _mac8023log->sTime == mac8023log->sTime &&
				    _mac8023log->IP_Src == mac8023log->IP_Src &&
				    _mac8023log->IP_Dst == mac8023log->IP_Dst &&
				    _mac8023log->PHY_Src == mac8023log->PHY_Src &&
				    _mac8023log->PHY_Dst == mac8023log->PHY_Dst &&
				    _mac8023log->FrameID == mac8023log->FrameID &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac8023log;
					break;
				}
			}
		}
		else if (mac8023log->Event == SuccessRX || mac8023log->Event == DropRX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {
				logchain = (logChain *) * iter;
				if (logchain->PROTO != PROTO_802_3)
					continue;

				_mac8023log = (struct mac802_3_log *)logchain->log;

				if (_mac8023log->Event == StartRX &&
				    _mac8023log->sTime == mac8023log->sTime &&
				    _mac8023log->IP_Src == mac8023log->IP_Src &&
				    _mac8023log->IP_Dst == mac8023log->IP_Dst &&
				    _mac8023log->PHY_Src == mac8023log->PHY_Src &&
				    _mac8023log->PHY_Dst == mac8023log->PHY_Dst &&
				    _mac8023log->FrameID == mac8023log->FrameID &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac8023log;
					break;
				}
			}
		}

		delete logep;
	}	//end logep->PROTO == PROTO_802_3
	else if (logep->PROTO == PROTO_802_11) {
		mac80211log = (struct mac802_11_log *)logep->log;

		if (mac80211log->Event == StartTX || mac80211log->Event == StartRX) {
			logchain = new logChain[1];
			logchain->PROTO = PROTO_802_11;
			logchain->log = mac80211log;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (mac80211log->Event == SuccessTX || mac80211log->Event == DropTX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_802_11) {
					continue;
				}

				_mac80211log = (struct mac802_11_log *)logchain->log;

				if (_mac80211log->Event == StartTX &&
				    _mac80211log->sTime == mac80211log->sTime &&
				    _mac80211log->IP_Src == mac80211log->IP_Src &&
				    _mac80211log->IP_Dst == mac80211log->IP_Dst &&
				    _mac80211log->PHY_Src == mac80211log->PHY_Src &&
				    _mac80211log->PHY_Dst == mac80211log->PHY_Dst &&
				    _mac80211log->FrameID == mac80211log->FrameID &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac80211log;
					break;
				}
			}
		}
		else if (mac80211log->Event == SuccessRX || mac80211log->Event == DropRX || mac80211log->Event == DROP_RX_MAC_HEADER_ERROR) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {
				logchain = (logChain *) * iter;
				if (logchain->PROTO != PROTO_802_11)
					continue;

				_mac80211log = (struct mac802_11_log *)logchain->log;

				if (_mac80211log->Event == StartRX &&
				    _mac80211log->sTime == mac80211log->sTime &&
				    _mac80211log->IP_Src == mac80211log->IP_Src &&
				    _mac80211log->IP_Dst == mac80211log->IP_Dst &&
				    _mac80211log->PHY_Src == mac80211log->PHY_Src &&
				    _mac80211log->PHY_Dst == mac80211log->PHY_Dst &&
				    _mac80211log->FrameID == mac80211log->FrameID &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac80211log;
					break;
				}
			}
		}

		delete logep;
	}	//end logep->PROTO == PROTO_802_11

#ifdef CONFIG_OPTICAL
	else if (logep->PROTO == PROTO_OPHY) {
		ophylog = (struct ophy_log *)logep->log;

		if (ophylog->Event == StartTX || ophylog->Event == StartRX) {
			logchain = new logChain[1];
			logchain->PROTO = PROTO_OPHY;
			logchain->log = ophylog;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (ophylog->Event == SuccessTX || ophylog->Event == DropTX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_OPHY) {
					continue;
				}

				_ophylog = (struct ophy_log *)logchain->log;

				if (_ophylog->Event == StartTX &&
				    _ophylog->sTime == ophylog->sTime &&
				    _ophylog->IP_Src == ophylog->IP_Src &&
				    _ophylog->IP_Dst == ophylog->IP_Dst &&
				    _ophylog->PHY_Src == ophylog->PHY_Src &&
				    _ophylog->PHY_Dst == ophylog->PHY_Dst &&
				    _ophylog->FrameID == ophylog->FrameID &&
				    logchain->log_end == NULL) {
					logchain->log_end = ophylog;
					break;
				}
			}
		}
		else if (ophylog->Event == SuccessRX || ophylog->Event == DropRX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {
				logchain = (logChain *) * iter;
				if (logchain->PROTO != PROTO_OPHY)
					continue;

				_ophylog = (struct ophy_log *)logchain->log;

				if (_ophylog->Event == StartRX &&
				    _ophylog->sTime == ophylog->sTime &&
				    _ophylog->IP_Src == ophylog->IP_Src &&
				    _ophylog->IP_Dst == ophylog->IP_Dst &&
				    _ophylog->PHY_Src == ophylog->PHY_Src &&
				    _ophylog->PHY_Dst == ophylog->PHY_Dst &&
				    _ophylog->FrameID == ophylog->FrameID &&
				    logchain->log_end == NULL) {
					logchain->log_end = ophylog;
					break;
				}
			}
		}

		delete logep;
	}	//end logep->PROTO == PROTO_OPHY
#endif	/* CONFIG_OPTICAL */

#ifdef CONFIG_GPRS
	else if (logep->PROTO == PROTO_GPRS) {
		gprslog = (struct gprs_log *)logep->log;

		if (gprslog->Event == StartTX || gprslog->Event == StartRX) {
			logchain = new logChain[1];
			logchain->PROTO = PROTO_GPRS;
			logchain->log = gprslog;
			logchain->log_end = gprslog;

			llist.push_back(logchain);
		}

		delete logep;
	}	//end logep->PROTO == PROTO_GPRS
#endif	/* CONFIG_GPRS */

#ifdef CONFIG_WIMAX
	else if (logep->PROTO == PROTO_802_16) {

		mac80216log = (struct mac802_16_log *)logep->log;

		if (mac80216log->Event == StartTX || mac80216log->Event == StartRX) {

			logchain = new logChain[1];
			logchain->PROTO = PROTO_802_16;
			logchain->log = mac80216log;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (mac80216log->Event == SuccessTX || mac80216log->Event == DropTX) {

#ifdef LOG_FUNCTION_DEBUG
			printf("process SucessTX DropTX packet.\n");
#endif

			for (liter iter = llist.begin(), iter_end = llist.end();
			     iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_802_16) {
					continue;
				}



				_mac80216log = (struct mac802_16_log *)logchain->log;

#ifdef LOG_FUNCTION_DEBUG
				printf("process SucessTX DropTX packet stage 2: event_type = %d.\n",
				       _mac80216log->Event);
#endif

				if (_mac80216log->Event == StartTX &&
				    _mac80216log->sTime == mac80216log->sTime &&
				    _mac80216log->IP_Src == mac80216log->IP_Src &&
				    _mac80216log->IP_Dst == mac80216log->IP_Dst &&
				    _mac80216log->PHY_Src == mac80216log->PHY_Src &&
				    _mac80216log->PHY_Dst == mac80216log->PHY_Dst &&
				    _mac80216log->ConnID == mac80216log->ConnID &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac80216log;

#ifdef LOG_FUNCTION_DEBUG
					printf("process SucessTX DropTX packet stage 3: event_type = %d.\n", _mac80216log->Event);
#endif


					break;
				}
			}
		}
		else if (mac80216log->Event == SuccessRX || mac80216log->Event == DropRX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
			     iter != iter_end; ++iter) {

				logchain = (logChain *) * iter;

				if (logchain->PROTO != PROTO_802_16)
					continue;

				_mac80216log = (struct mac802_16_log *)logchain->log;

				if (_mac80216log->Event == StartRX &&
				    _mac80216log->sTime == mac80216log->sTime &&
				    _mac80216log->IP_Src == mac80216log->IP_Src &&
				    _mac80216log->IP_Dst == mac80216log->IP_Dst &&
				    _mac80216log->PHY_Src == mac80216log->PHY_Src &&
				    _mac80216log->PHY_Dst == mac80216log->PHY_Dst &&
				    _mac80216log->ConnID == mac80216log->ConnID &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac80216log;
					break;
				}
			}
		}	//end logep->PROTO == PROTO_802_16
		else;

		delete logep;

	}
#endif	/* CONFIG_WIMAX */

#ifdef CONFIG_MobileWIMAX
	else if (logep->PROTO == PROTO_802_16e) {

		mac80216elog = (struct mac802_16e_log *)logep->log;

		if (mac80216elog->Event == StartTX || mac80216elog->Event == StartRX) {

			logchain = new logChain[1];
			logchain->PROTO = PROTO_802_16e;
			logchain->log = mac80216elog;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (mac80216elog->Event == SuccessTX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
			     iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_802_16e) {
					continue;
				}

				_mac80216elog = (struct mac802_16e_log *)logchain->log;

				if (_mac80216elog->Event == StartTX &&
				    _mac80216elog->sTime == mac80216elog->sTime &&
				    _mac80216elog->IP_Src == mac80216elog->IP_Src &&
				    _mac80216elog->IP_Dst == mac80216elog->IP_Dst &&
				    _mac80216elog->PHY_Src == mac80216elog->PHY_Src &&
				    _mac80216elog->PHY_Dst == mac80216elog->PHY_Dst &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac80216elog;
					break;
				}
			}
		}
		else if (mac80216elog->Event == SuccessRX || mac80216elog->Event == DropRX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
			     iter != iter_end; ++iter) {

				logchain = (logChain *) * iter;

				if (logchain->PROTO != PROTO_802_16e)
					continue;

				_mac80216elog = (struct mac802_16e_log *)logchain->log;

				if (_mac80216elog->Event == StartRX &&
				    _mac80216elog->sTime == mac80216elog->sTime &&
				    _mac80216elog->IP_Src == mac80216elog->IP_Src &&
				    _mac80216elog->IP_Dst == mac80216elog->IP_Dst &&
				    _mac80216elog->PHY_Src == mac80216elog->PHY_Src &&
				    _mac80216elog->PHY_Dst == mac80216elog->PHY_Dst &&
				    logchain->log_end == NULL) {

					logchain->log_end = mac80216elog;
					break;
				}
			}
		}	//end logep->PROTO == PROTO_802_16e
		else;

		delete logep;

	}
#endif  /* CONFIG_MobileWIMAX */

#ifdef CONFIG_MobileRelayWIMAX
	else if (logep->PROTO == PROTO_802_16j) {

		mac80216jlog = (struct mac802_16j_log *)logep->log;

		if (mac80216jlog->Event == StartTX || mac80216jlog->Event == StartRX) {

			logchain = new logChain[1];
			logchain->PROTO = PROTO_802_16j;
			logchain->log = mac80216jlog;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (mac80216jlog->Event == SuccessTX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
			     iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_802_16j) {
					continue;
				}

				_mac80216jlog = (struct mac802_16j_log *)logchain->log;

				if (_mac80216jlog->Event == StartTX &&
				    _mac80216jlog->sTime == mac80216jlog->sTime &&
				    _mac80216jlog->IP_Src == mac80216jlog->IP_Src &&
				    _mac80216jlog->IP_Dst == mac80216jlog->IP_Dst &&
				    _mac80216jlog->PHY_Src == mac80216jlog->PHY_Src &&
				    _mac80216jlog->PHY_Dst == mac80216jlog->PHY_Dst &&
				    logchain->log_end == NULL) {
					logchain->log_end = mac80216jlog;
					break;
				}
			}
		}
		else if (mac80216jlog->Event == SuccessRX || mac80216jlog->Event == DropRX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
			     iter != iter_end; ++iter) {

				logchain = (logChain *) * iter;

				if (logchain->PROTO != PROTO_802_16j)
					continue;

				_mac80216jlog = (struct mac802_16j_log *)logchain->log;

				if (_mac80216jlog->Event == StartRX &&
				    _mac80216jlog->sTime == mac80216jlog->sTime &&
				    _mac80216jlog->IP_Src == mac80216jlog->IP_Src &&
				    _mac80216jlog->IP_Dst == mac80216jlog->IP_Dst &&
				    _mac80216jlog->PHY_Src == mac80216jlog->PHY_Src &&
				    _mac80216jlog->PHY_Dst == mac80216jlog->PHY_Dst &&
				    logchain->log_end == NULL) {

					logchain->log_end = mac80216jlog;
					break;
				}
			}
		}	//end logep->PROTO == PROTO_802_16j
		else;

		delete logep;

	}
#endif  /* CONFIG_MobileRelayWIMAX */

#ifdef CONFIG_MR_WIMAX_NT
	else if (logep->PROTO == PROTO_802_16j_NT) {

		mac80216j_NT_log = (struct mac802_16j_NT_log *)logep->log;

		if (mac80216j_NT_log->Event == StartTX || mac80216j_NT_log->Event == StartRX) {

			logchain = new logChain[1];
			logchain->PROTO = PROTO_802_16j_NT;
			logchain->log = mac80216j_NT_log;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (mac80216j_NT_log->Event == SuccessTX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
					iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_802_16j_NT) {
					continue;
				}

				_mac80216j_NT_log = (struct mac802_16j_NT_log *)logchain->log;

				if (_mac80216j_NT_log->Event == StartTX &&
						_mac80216j_NT_log->sTime == mac80216j_NT_log->sTime &&
						_mac80216j_NT_log->IP_Src == mac80216j_NT_log->IP_Src &&
						_mac80216j_NT_log->IP_Dst == mac80216j_NT_log->IP_Dst &&
						_mac80216j_NT_log->PHY_Src == mac80216j_NT_log->PHY_Src &&
						_mac80216j_NT_log->PHY_Dst == mac80216j_NT_log->PHY_Dst &&
						logchain->log_end == NULL) {
					logchain->log_end = mac80216j_NT_log;
					break;
				}
			}
		}
		else if (mac80216j_NT_log->Event == SuccessRX || mac80216j_NT_log->Event == DropRX) {

			for (liter iter = llist.begin(), iter_end = llist.end();
					iter != iter_end; ++iter) {

				logchain = (logChain *) * iter;

				if (logchain->PROTO != PROTO_802_16j_NT)
					continue;

				_mac80216j_NT_log = (struct mac802_16j_NT_log *)logchain->log;

				if (_mac80216j_NT_log->Event == StartRX &&
						_mac80216j_NT_log->sTime == mac80216j_NT_log->sTime &&
						_mac80216j_NT_log->IP_Src == mac80216j_NT_log->IP_Src &&
						_mac80216j_NT_log->IP_Dst == mac80216j_NT_log->IP_Dst &&
						_mac80216j_NT_log->PHY_Src == mac80216j_NT_log->PHY_Src &&
						_mac80216j_NT_log->PHY_Dst == mac80216j_NT_log->PHY_Dst &&
						logchain->log_end == NULL) {

					logchain->log_end = mac80216j_NT_log;
					break;
				}
			}
		}       //end logep->PROTO == PROTO_802_16j_NT
		else;

		delete logep;
	}
#endif  /* CONFIG_MR_WIMAX_NT */


#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVB_RCS)
	else if (logep->PROTO == PROTO_SAT_DVBRCS) {
		dvbrcslog = (struct dvbrcs_log *)logep->log;

		if (dvbrcslog->Event == StartTX || dvbrcslog->Event == StartRX) {
			logchain = new logChain[1];
			logchain->PROTO = PROTO_SAT_DVBRCS;
			logchain->log = dvbrcslog;
			logchain->log_end = NULL;

			llist.push_back(logchain);
		}
		else if (dvbrcslog->Event == SuccessTX || dvbrcslog->Event == DropTX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {

				logchain = (logChain *) (*iter);
				if (logchain->PROTO != PROTO_SAT_DVBRCS) {
					continue;
				}

				_dvbrcslog = (struct dvbrcs_log *)logchain->log;

				if (_dvbrcslog->Event == StartTX &&
				    _dvbrcslog->sTime == dvbrcslog->sTime &&
				    _dvbrcslog->IP_Src == dvbrcslog->IP_Src &&
				    _dvbrcslog->IP_Dst == dvbrcslog->IP_Dst &&
				    _dvbrcslog->PHY_Src == dvbrcslog->PHY_Src &&
				    _dvbrcslog->PHY_Dst == dvbrcslog->PHY_Dst &&
				    _dvbrcslog->BurstID == dvbrcslog->BurstID &&
				    logchain->log_end == NULL) {
					logchain->log_end = dvbrcslog;
					break;
				}
			}
		}
		else if (dvbrcslog->Event == SuccessRX || dvbrcslog->Event == DropRX) {
			for (liter iter = llist.begin(),
			     iter_end = llist.end(); iter != iter_end; ++iter) {
				logchain = (logChain *) * iter;
				if (logchain->PROTO != PROTO_SAT_DVBRCS)
					continue;

				_dvbrcslog = (struct dvbrcs_log *)logchain->log;

				if (_dvbrcslog->Event == StartRX &&
				    _dvbrcslog->sTime == dvbrcslog->sTime &&
				    _dvbrcslog->IP_Src == dvbrcslog->IP_Src &&
				    _dvbrcslog->IP_Dst == dvbrcslog->IP_Dst &&
				    _dvbrcslog->PHY_Src == dvbrcslog->PHY_Src &&
				    _dvbrcslog->PHY_Dst == dvbrcslog->PHY_Dst &&
				    _dvbrcslog->BurstID == dvbrcslog->BurstID &&
				    logchain->log_end == NULL) {
					logchain->log_end = dvbrcslog;
					break;
				}
			}
		}

		delete logep;
	}	//end logep->PROTO == PROTO_SAT_PROTO
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */
	else
		assert(0);


	liter iter = llist.begin(), iter_end = llist.end(), iter_tmp;

	while (iter != iter_end) {
		logchain = (logChain *) * iter;

#ifdef LOG_FUNCTION_DEBUG

		if (!(GetCurrentTime() % 1000000)) {

			printf("logchain processing: PROTO = %d. log_p = %u, log_end_p = %u \n",
			       logchain->PROTO, logchain->log, logchain->log_end);

			if (logchain->PROTO == PROTO_802_16) {
				struct mac802_16_log *m80216_logep = (struct mac802_16_log *)logchain->log;
				dump_mac80216_logep(m80216_logep);
			}

		}

#endif


		if (logchain->log && logchain->log_end) {
			if (logchain->PROTO == PROTO_802_3) {

				mac8023log = (struct mac802_3_log *)logchain->log;
				_mac8023log = (struct mac802_3_log *)logchain->log_end;

				tmp_proto = PROTO_802_3;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_mac8023log->Event == SuccessTX) {
					if (_mac8023log->PHY_Dst > PHY_BROADCAST_ID) {
						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac8023log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac8023log->PHY_Dst -= PHY_BROADCAST_ID;
					}
					else if (_mac8023log->RetryCount > 0) {
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else {
						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_mac8023log->Event == SuccessRX) {
					if (_mac8023log->PHY_Dst > PHY_BROADCAST_ID) {
						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac8023log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac8023log->PHY_Dst -= PHY_BROADCAST_ID;
					}
					else {
						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_mac8023log->Event == DropTX ||
					 _mac8023log->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_mac8023log->PHY_Dst > PHY_BROADCAST_ID) {
						mac8023log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac8023log->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);

				fwrite(&(mac8023log->Time), sizeof (u_int64_t), 1, fptr);
				diff = _mac8023log->Time - mac8023log->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&_mac8023log->FrameType, sizeof (u_char), 1, fptr);
				fwrite(&_mac8023log->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac8023log->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac8023log->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac8023log->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac8023log->FrameID, sizeof (u_int64_t), 1, fptr);
				fwrite(&_mac8023log->FrameLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&_mac8023log->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac8023log->DropReason, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 6, fptr);
				fflush(fptr);
#if IPC
				if (dynamicPtrLogFlag && !strcasecmp(dynamicPtrLogFlag, "on"))
					logpack_->addMac8023Log(tmp_proto, tmp_event, diff,
								mac8023log, _mac8023log, 6);
#endif
				delete mac8023log;
				delete _mac8023log;
			}	// logchain->PROTO == PROTO_802_3
			else if (logchain->PROTO == PROTO_802_11) {
				mac80211log = (struct mac802_11_log *)logchain->log;
				_mac80211log = (struct mac802_11_log *)logchain->log_end;

				tmp_proto = PROTO_802_11;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_mac80211log->Event == SuccessTX) {
					if (_mac80211log->RetryCount > 0) {
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else if (_mac80211log->PHY_Dst == PHY_BROADCAST_ID) {
						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80211log->PHY_Dst = 0;
						mac80211log->MAC_Dst = 0;
						_mac80211log->PHY_Dst = 0;
						_mac80211log->MAC_Dst = 0;
					}
					else {
						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_mac80211log->Event == SuccessRX) {
					if (_mac80211log->PHY_Dst > PHY_BROADCAST_ID) {
						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80211log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80211log->PHY_Dst -= PHY_BROADCAST_ID;
					}
					else {
						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_mac80211log->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_mac80211log->PHY_Dst > PHY_BROADCAST_ID) {
						mac80211log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80211log->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);


				fwrite(&(mac80211log->Time), sizeof (u_int64_t), 1, fptr);
				diff = _mac80211log->Time - mac80211log->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80211log->FrameType, sizeof (u_char), 1, fptr);
				fwrite(&mac80211log->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80211log->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80211log->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80211log->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80211log->MAC_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80211log->FrameID, sizeof (u_int64_t), 1, fptr);
				fwrite(&mac80211log->FrameLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80211log->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac80211log->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&mac80211log->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 3, fptr);
				fflush(fptr);
#if IPC
				if (dynamicPtrLogFlag && !strcasecmp(dynamicPtrLogFlag, "on"))
					logpack_->addMac80211Log(tmp_proto, tmp_event, diff,
								 mac80211log, _mac80211log, 3);
#endif
				delete mac80211log;
				delete _mac80211log;
			}	// logchain->PROTO == PROTO_802_11

#ifdef CONFIG_OPTICAL
			else if (logchain->PROTO == PROTO_OPHY) {

				ophylog = (struct ophy_log *)logchain->log;
				_ophylog = (struct ophy_log *)logchain->log_end;

				tmp_proto = PROTO_OPHY;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_ophylog->Event == SuccessTX) {
					if (_ophylog->RetryCount > 0) {
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else if (_ophylog->PHY_Dst > PHY_BROADCAST_ID) {
						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						ophylog->PHY_Dst -= PHY_BROADCAST_ID;
						_ophylog->PHY_Dst -= PHY_BROADCAST_ID;
					}
					else {
						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_ophylog->Event == SuccessRX) {
					if (_ophylog->PHY_Dst > PHY_BROADCAST_ID) {
						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						ophylog->PHY_Dst -= PHY_BROADCAST_ID;
						_ophylog->PHY_Dst -= PHY_BROADCAST_ID;
					}
					else {
						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_ophylog->Event == DropTX || _ophylog->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_ophylog->PHY_Dst > PHY_BROADCAST_ID) {
						ophylog->PHY_Dst -= PHY_BROADCAST_ID;
						_ophylog->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);

				fwrite(&(ophylog->Time), sizeof (u_int64_t), 1, fptr);
				diff = _ophylog->Time - ophylog->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&_ophylog->FrameType, sizeof (u_char), 1, fptr);
				fwrite(&_ophylog->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&_ophylog->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&_ophylog->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&_ophylog->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&_ophylog->FrameID, sizeof (u_int64_t), 1, fptr);
				fwrite(&_ophylog->FrameLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&_ophylog->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_ophylog->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&_ophylog->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 5, fptr);
				fflush(fptr);

#if IPC
				if (dynamicPtrLogFlag && !strcasecmp(dynamicPtrLogFlag, "on"))
					logpack_->addOphyLog(tmp_proto, tmp_event, diff, ophylog,
							     _ophylog, 5);
#endif
				delete ophylog;
				delete _ophylog;
			}	// logchain->PROTO == PROTO_OPHY
#endif	/* CONFIG_OPTICAL */

#ifdef CONFIG_GPRS
			else if (logchain->PROTO == PROTO_GPRS) {
				gprslog = (struct gprs_log *)logchain->log;

				tmp_proto = PROTO_GPRS;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (gprslog->Event == StartTX) {
					tmp_event = EVENTTYPE_TX;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
				}
				else if (gprslog->Event == StartRX) {
					tmp_event = EVENTTYPE_RX;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
				}
				else
					assert(0);

				fwrite(&(gprslog->sTime), sizeof (u_int64_t), 1, fptr);
				diff = gprslog->Time - gprslog->sTime;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&gprslog->BurstType, sizeof (u_char), 1, fptr);
				fwrite(&gprslog->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&gprslog->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&gprslog->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&gprslog->PHY_Dst, sizeof (u_int16_t), 1, fptr);

				fwrite(&gprslog->BurstID, sizeof (u_int64_t), 1, fptr);
				fwrite(&gprslog->BurstLen, sizeof (u_int32_t), 1, fptr);

				fwrite(&gprslog->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&gprslog->DropReason, sizeof (u_char), 1, fptr);

				fwrite(&gprslog->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 5, fptr);
				fflush(fptr);

#if IPC
				if (dynamicPtrLogFlag && !strcasecmp(dynamicPtrLogFlag, "on"))
					logpack_->addGprsLog(tmp_proto, tmp_event, diff, gprslog,
							     5);
#endif
				delete gprslog;
			}	// logchain->PROTO == PROTO_GPRS
#endif	/* CONFIG_GPRS */

#ifdef CONFIG_WIMAX
			else if (logchain->PROTO == PROTO_802_16) {
				mac80216log = (struct mac802_16_log *)logchain->log;
				_mac80216log = (struct mac802_16_log *)logchain->log_end;

				tmp_proto = PROTO_802_16;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_mac80216log->Event == SuccessTX) {
					if (_mac80216log->RetryCount > 0) {
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else if (_mac80216log->PHY_Dst == PHY_BROADCAST_ID) {

						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216log->PHY_Dst = 0;
						_mac80216log->PHY_Dst = 0;


					}
					else {

						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
				else if (_mac80216log->Event == SuccessRX) {

					if (_mac80216log->PHY_Dst > PHY_BROADCAST_ID) {

						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216log->PHY_Dst -= PHY_BROADCAST_ID;

					}
					else {

						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
				else if (_mac80216log->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_mac80216log->PHY_Dst > PHY_BROADCAST_ID) {
						mac80216log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216log->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);


				fwrite(&(mac80216log->Time), sizeof (u_int64_t), 1, fptr);
				diff = _mac80216log->Time - mac80216log->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216log->BurstType, sizeof (u_char), 1, fptr);
				fwrite(&mac80216log->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216log->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216log->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216log->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216log->ConnID, sizeof (u_int64_t), 1, fptr);
				fwrite(&mac80216log->BurstLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216log->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac80216log->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&mac80216log->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 5, fptr);
				fflush(fptr);

#if IPC
				if ((dynamicPtrLogFlag) && (!strcasecmp(dynamicPtrLogFlag, "on")));	//logpack_->addMac80211Log(tmp_proto,tmp_event,diff,mac80216log,_mac80216log,3);
#endif

				delete mac80216log;
				delete _mac80216log;
			}	// logchain->PROTO == PROTO_802_16 
#endif	/* CONFIG_WIMAX */

#ifdef CONFIG_MobileWIMAX
			else if (logchain->PROTO == PROTO_802_16e) {
				mac80216elog = (struct mac802_16e_log *)logchain->log;
				_mac80216elog = (struct mac802_16e_log *)logchain->log_end;

				tmp_proto = PROTO_802_16e;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_mac80216elog->Event == SuccessTX) {
					if (_mac80216elog->RetryCount > 0) {
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else if (_mac80216elog->PHY_Dst == PHY_BROADCAST_ID) {

						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216elog->PHY_Dst = 0;
						_mac80216elog->PHY_Dst = 0;


					}
					else {

						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
				else if (_mac80216elog->Event == SuccessRX) {

					if (_mac80216elog->PHY_Dst > PHY_BROADCAST_ID) {

						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216elog->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216elog->PHY_Dst -= PHY_BROADCAST_ID;

					}
					else {

						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
                else if (_mac80216elog->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_mac80216elog->PHY_Dst > PHY_BROADCAST_ID) {
						mac80216elog->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216elog->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);


				fwrite(&(mac80216elog->Time), sizeof (u_int64_t), 1, fptr);
				diff = _mac80216elog->Time - mac80216elog->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216elog->BurstType, sizeof (u_char), 1, fptr);
				fwrite(&mac80216elog->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216elog->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216elog->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216elog->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216elog->BurstLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216elog->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac80216elog->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&mac80216elog->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 13, fptr);
				fflush(fptr);

#if IPC
				if ((dynamicPtrLogFlag) && (!strcasecmp(dynamicPtrLogFlag, "on")))
                    ;//logpack_->addMac80216eLog(tmp_proto,tmp_event,diff,mac80216elog,_mac80216elog,5);
#endif

				delete mac80216elog;
				delete _mac80216elog;
			}	// logchain->PROTO == PROTO_802_16e
#endif	/* CONFIG_MobileWIMAX */

#ifdef CONFIG_MobileRelayWIMAX
			else if (logchain->PROTO == PROTO_802_16j) {
				mac80216jlog = (struct mac802_16j_log *)logchain->log;
				_mac80216jlog = (struct mac802_16j_log *)logchain->log_end;

				tmp_proto = PROTO_802_16j;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_mac80216jlog->Event == SuccessTX) {
					if (_mac80216jlog->RetryCount > 0) {
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else if (_mac80216jlog->PHY_Dst == PHY_BROADCAST_ID) {

						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216jlog->PHY_Dst = 0;
						_mac80216jlog->PHY_Dst = 0;


					}
					else {

						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
				else if (_mac80216jlog->Event == SuccessRX) {

					if (_mac80216jlog->PHY_Dst > PHY_BROADCAST_ID) {

						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216jlog->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216jlog->PHY_Dst -= PHY_BROADCAST_ID;

					}
					else {

						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
                else if (_mac80216jlog->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_mac80216jlog->PHY_Dst > PHY_BROADCAST_ID) {
						mac80216jlog->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216jlog->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);


				fwrite(&(mac80216jlog->Time), sizeof (u_int64_t), 1, fptr);
				diff = _mac80216jlog->Time - mac80216jlog->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216jlog->BurstType, sizeof (u_char), 1, fptr);
				fwrite(&mac80216jlog->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216jlog->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216jlog->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216jlog->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216jlog->BurstLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216jlog->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac80216jlog->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&mac80216jlog->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 13, fptr);
				fflush(fptr);

#if IPC
				if ((dynamicPtrLogFlag) && (!strcasecmp(dynamicPtrLogFlag, "on")))
                    ;//logpack_->addMac80216jLog(tmp_proto,tmp_event,diff,mac80216jlog,_mac80216elog,5);
#endif

				delete mac80216jlog;
				delete _mac80216jlog;
			}	// logchain->PROTO == PROTO_802_16j
#endif	/* CONFIG_MobileRelayWIMAX */

#ifdef CONFIG_MR_WIMAX_NT
			else if (logchain->PROTO == PROTO_802_16j_NT) {
				mac80216j_NT_log = (struct mac802_16j_NT_log *)logchain->log;
				_mac80216j_NT_log = (struct mac802_16j_NT_log *)logchain->log_end;

				tmp_proto = PROTO_802_16j_NT;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_mac80216j_NT_log->Event == SuccessTX) {
					if (_mac80216j_NT_log->RetryCount > 0)
					{
						tmp_event = EVENTTYPE_RTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
					else if (_mac80216j_NT_log->PHY_Dst == PHY_BROADCAST_ID)
					{
						tmp_event = EVENTTYPE_BTX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216j_NT_log->PHY_Dst = 0;
						_mac80216j_NT_log->PHY_Dst = 0;
					}
					else
					{
						tmp_event = EVENTTYPE_TX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					}
				}
				else if (_mac80216j_NT_log->Event == SuccessRX) {

					if (_mac80216j_NT_log->PHY_Dst > PHY_BROADCAST_ID)
					{

						tmp_event = EVENTTYPE_BRX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);
						mac80216j_NT_log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216j_NT_log->PHY_Dst -= PHY_BROADCAST_ID;

					}
					else {

						tmp_event = EVENTTYPE_RX;
						fwrite(&tmp_event, sizeof (u_char), 1, fptr);

					}
				}
				else if (_mac80216j_NT_log->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_mac80216j_NT_log->PHY_Dst > PHY_BROADCAST_ID) {
						mac80216j_NT_log->PHY_Dst -= PHY_BROADCAST_ID;
						_mac80216j_NT_log->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);


				fwrite(&(mac80216j_NT_log->Time), sizeof (u_int64_t), 1, fptr);
				diff = _mac80216j_NT_log->Time - mac80216j_NT_log->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216j_NT_log->BurstType, sizeof (u_char), 1, fptr);
				fwrite(&mac80216j_NT_log->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216j_NT_log->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216j_NT_log->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216j_NT_log->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&mac80216j_NT_log->BurstLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&mac80216j_NT_log->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_mac80216j_NT_log->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&mac80216j_NT_log->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 13, fptr);
				fflush(fptr);

#if IPC
				if ((dynamicPtrLogFlag) && (!strcasecmp(dynamicPtrLogFlag, "on")))
					;//logpack_->addmac80216j_NT_log(tmp_proto,tmp_event,diff,mac80216j_NT_log,_mac80216j_NT_log,5);
#endif

				delete mac80216j_NT_log;
				delete _mac80216j_NT_log;
			}	// logchain->PROTO == PROTO_802_16j_NT
#endif	/* CONFIG_MR_WIMAX_NT */

#if defined(CONFIG_SATELLITE) && defined(CONFIG_DVBRCS)
			else if (logchain->PROTO == PROTO_SAT_DVBRCS) {
				dvbrcslog = (struct dvbrcs_log *)logchain->log;
				_dvbrcslog = (struct dvbrcs_log *)logchain->log_end;

				tmp_proto = PROTO_SAT_DVBRCS;
				fwrite(&tmp_proto, sizeof (u_char), 1, fptr);

				if (_dvbrcslog->Event == SuccessRX) {
					tmp_event = EVENTTYPE_RX;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
				}
				else if (_dvbrcslog->Event == SuccessTX) {
					tmp_event = EVENTTYPE_TX;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
				}
				else if (_dvbrcslog->Event == DropRX) {
					tmp_event = EVENTTYPE_DROP;
					fwrite(&tmp_event, sizeof (u_char), 1, fptr);
					if (_dvbrcslog->PHY_Dst > PHY_BROADCAST_ID) {
						dvbrcslog->PHY_Dst -= PHY_BROADCAST_ID;
						_dvbrcslog->PHY_Dst -= PHY_BROADCAST_ID;
					}
				}
				else
					assert(0);

				fwrite(&(dvbrcslog->Time), sizeof (u_int64_t), 1, fptr);
				diff = _dvbrcslog->Time - dvbrcslog->Time;
				fwrite(&diff, sizeof (u_int32_t), 1, fptr);
				fwrite(&dvbrcslog->BurstType, sizeof (u_char), 1, fptr);
				fwrite(&dvbrcslog->IP_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&dvbrcslog->IP_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&dvbrcslog->PHY_Src, sizeof (u_int16_t), 1, fptr);
				fwrite(&dvbrcslog->PHY_Dst, sizeof (u_int16_t), 1, fptr);
				fwrite(&dvbrcslog->BurstID, sizeof (u_int64_t), 1, fptr);
				fwrite(&dvbrcslog->BurstLen, sizeof (u_int32_t), 1, fptr);
				fwrite(&dvbrcslog->RetryCount, sizeof (u_int16_t), 1, fptr);
				fwrite(&_dvbrcslog->DropReason, sizeof (u_char), 1, fptr);
				fwrite(&dvbrcslog->Channel, sizeof (u_char), 1, fptr);
				fwrite(pad, sizeof (u_char), 5, fptr);
				fflush(fptr);
#if IPC
				if (dynamicPtrLogFlag && !strcasecmp(dynamicPtrLogFlag, "on"))
					logpack_->addDvbrcsLog(tmp_proto, tmp_event, diff,
							       dvbrcslog, _dvbrcslog, 5);
#endif
				delete dvbrcslog;
				delete _dvbrcslog;
			}	// logchain->PROTO == PROTO_SAT_DVBRCS
#endif	/* CONFIG_SATELLITE && CONFIG_DVB_RCS */


			delete logchain;

			iter_tmp = iter;
			++iter;
			llist.erase(iter_tmp);
		}
		else
			break;
	}
}

#ifdef CONFIG_WIMAX
int dump_mac80216_logep(mac80216_log_t * logep)
{

	if (!logep) {

		printf("dump_mac80216_logep(): logep is null.\n");

		exit(1);

	}

	printf("PROTO     sTime      Time  NodeType   NID EventType PHY_Src PHY_Dst \n");
	printf(" %2d %10llu %10llu %4u %10u %5u     %5u   %5u \n", logep->PROTO, logep->sTime,
	       logep->Time, logep->NodeType, logep->NodeID, logep->Event, logep->PHY_Src,
	       logep->PHY_Dst);

	printf("ConnID BurstType BurstLen DropReason Channel \n");

	printf(" %4llu %10d %8u %4d %4d \n\n\n", logep->ConnID, logep->BurstType,
	       logep->BurstLen, logep->DropReason, logep->Channel);

	return 1;

}
#endif	/* CONFIG_WIMAX */
