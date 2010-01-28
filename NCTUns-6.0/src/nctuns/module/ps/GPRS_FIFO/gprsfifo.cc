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

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nctuns_api.h>
#include <ps/GPRS_FIFO/gprsfifo.h>
#include <gprs/include/bss_message.h>
#include <exportStr.h>
#include <mbinder.h>

extern u_int32_t *fifoIFcurqlen;
extern u_int32_t *fifoIFmaxqlen;

MODULE_GENERATOR(GPRSfifo);

GPRSfifo::GPRSfifo(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
	u_long *mytunidp;

	/* disable flow control */
    s_flowctl = DISABLED;
	r_flowctl = DISABLED;

	/* initialize interface queue */
	if_snd.ifq_head = if_snd.ifq_tail = 0;
	if_snd.ifq_len = 0;
	if_snd.ifq_drops = 0;

	/* bind variable */
    	vBind("max_qlen", &if_snd.ifq_maxlen);
	vBind("log_qlen", &log_qlen_flag);
	vBind("log_option", &log_option);
	vBind("samplerate", &log_SampleRate);
	vBind("logFileName", &logQlenFileName);

	if_snd.ifq_len = 0;
	log_SampleRate = 1;
	full_log_flag = 0;
	if_snd.ifq_maxlen = 50; /* by default */

	// Added by Prof. S.Y. Wang on 10/22/2002
	mytunidp = GET_REG_VAR(get_portls()->pid, "TUNID", u_long *);
	if (mytunidp != NULL) {
		// This fifo queue is used by a (layer-3) tunnel interface.
		// (Note that a fifo queue may be used by a layer-2 interface,
		// such as a switch port. In that case, the fifo queue is not
		// associated with a tunnel interface.
		fifoIFmaxqlen[*mytunidp] = if_snd.ifq_maxlen;
	} /*else
		printf("nid %d pid %d fifo mytunidp == NULL 0\n", get_nid(), get_port());*/

	log_qlen_flag = NULL;
	log_option = NULL;
	logQlenFileName = NULL;

}

GPRSfifo::~GPRSfifo() {

}


int GPRSfifo::init() {

	int             (NslObject::*upcall)(MBinder *);

	/* export variable */
        EXPORT("cur-queue-length", E_RONLY|E_WONLY);
	EXPORT("max-queue-length", E_RONLY|E_WONLY);

	/* check log flag */
	if ( (log_qlen_flag)&&(!strcasecmp(log_qlen_flag, "on")) ){
		/* open log file */
		char * FILEPATH = (char *)malloc(strlen(GetConfigFileDir()) +
					strlen(logQlenFileName) + 1);
		sprintf(FILEPATH,"%s%s", GetConfigFileDir(),
					logQlenFileName);
		logQlenFile = fopen(FILEPATH,"w+");
		assert(logQlenFile);
		free(FILEPATH);
		
		if ( (log_option)&&(!strcasecmp(log_option, "FullLog")) ){
			full_log_flag = 1;
		}
		else{ /* sample_rate */
			u_int64_t samplerate_tick;
			SEC_TO_TICK(samplerate_tick, log_SampleRate);
			
			BASE_OBJTYPE(type);
			type = POINTER_TO_MEMBER(GPRSfifo, log);
			logTimer.setCallOutObj(this, type);
			logTimer.start(samplerate_tick, samplerate_tick);
		}
	}

	/* set upcall */
        upcall = (int (NslObject::*)(MBinder *))&GPRSfifo::intrq;
        sendtarget_->set_upcall(this, upcall);
        return(1);
}

int GPRSfifo::log() {
	double		current_time;

	current_time = GetCurrentTime() * TICK / 1000000000.0;
	
	if ( (log_qlen_flag)&&(!strcasecmp(log_qlen_flag, "on")) )
		fprintf(logQlenFile, "%.3f\t%d\n", current_time, if_snd.ifq_len);

	return(1);
}

int GPRSfifo::send(ePacket_ *pkt) {

	u_long *mytunidp;

	assert(pkt);

	/*
	 * If Module-Binder Queue(MBQ) is full, we should
	 * insert the outgoing packet into the interface 
	 * queue. If MBQ is not full, we can call the 
	 * put() or NslObject::send() method to pass the 
	 * outgoing packet to next module.
	 */

	bss_message* bssmsg = reinterpret_cast<bss_message*>(pkt->DataInfo_);
	assert(bssmsg);


	/* C.C. Lin: 
	 * Inter-module communication messages should not be dropped and
	 * will be transmitted down to lower-layer modules immediately.
	 */

	NslObject* rlc_obj_p = sendtarget_->bindModule();
	assert(rlc_obj_p);

	if (bssmsg->imc_flag) {

	    return rlc_obj_p->send(pkt);

	}

	if( sendtarget_->qfull() ) {

		/* MBQ is full, insert to ifq */
		if (IF_QFULL(&if_snd)) {

			/* ifq full, drop it! */
			IF_DROP(&if_snd);
			destroy_gprs_pkt(pkt);
			return 0;
		
		}
		
		/* otherwise, ifq not full, insert it */
		IF_ENQUEUE(&if_snd, pkt);
	
		if(full_log_flag == 1)
		    log();

		/* Added by Prof. S.Y. Wang on 10/22/2002 */
		mytunidp = GET_REG_VAR(get_portls()->pid, "TUNID", u_long *);

		if (mytunidp != NULL) {

			// This fifo queue is used by a (layer-3) tunnel interface.
			// (Note that a fifo queue may be used by a layer-2 interface,
			// such as a switch port. In that case, the fifo queue is not
			// associated with a tunnel interface.
			fifoIFcurqlen[*mytunidp] = if_snd.ifq_len;
			
			if (0) 
			    printf("enqueue: FIFO(nid %d) pid %d tunid %lu fifoIFcurqlen[%d]\n", 
			        get_nid(), get_port(), *mytunidp, fifoIFcurqlen[*mytunidp]);
		}

		return(1);
	} 
	else {
		/* 
		 * MBQ is not full, pass outgoing packet
		 * to next module.
		 */
		return(NslObject::send(pkt)); 
	}
}


int GPRSfifo::recv(ePacket_ *pkt) {

	/* Just by pass incoming packet */
        assert(pkt&&pkt->DataInfo_);
        return(NslObject::recv(pkt));
}


int GPRSfifo::intrq(MBinder *port) {

	ePacket_	*pkt;
	u_long *mytunidp;

	/*
	 * Push the packet in the interface queue
	 * to the MBQ. Whenever the pakcet in the
	 * MBQ is sent, the scheduler will call this
	 * member function to give fifo module a 
	 * chance to send the next packet in the
	 * interface queue.
	 */
	IF_DEQUEUE(&if_snd, pkt);

	// Added by Prof. S.Y. Wang on 10/22/2002
	mytunidp = GET_REG_VAR(get_port(), "TUNID", u_long *);
	if (mytunidp != NULL) {
		// This fifo queue is used by a (layer-3) tunnel interface.
		// (Note that a fifo queue may be used by a layer-2 interface,
		// such as a switch port. In that case, the fifo queue is not
		// associated with a tunnel interface.
		fifoIFcurqlen[*mytunidp] = if_snd.ifq_len;
		if (0) 
		    printf("dequeue: FIFO(nid %d) pid %d tunid %lu fifoIFcurqlen[%d]\n", 
		         get_nid(), get_port(), *mytunidp, fifoIFcurqlen[*mytunidp]);

	}

	if( full_log_flag == 1 )
		log();	

	if (pkt != NULL) {
		/*
		 * If still exist packet in the interface
		 * queue, we try to push it to the MBQ,
		 */
		assert(sendtarget_->enqueue(pkt) == 0);
	}
	return(1);
}


int GPRSfifo::command(int argc, const char *argv[]) {
	char			tmpBuf[10];
	struct ExportStr	*ExpStr;
	u_int32_t		row,column;
	u_long *mytunidp;

	/* The Get implementation of Exported Variable "cur-queue-length" */
        if (!strcmp(argv[0], "Get")&&(argc==2)) {
                if (!strcmp(argv[1], "cur-queue-length")) {
			ExpStr = new ExportStr(1);
			row = ExpStr->Add_row();
			column = 1;
                        sprintf(tmpBuf, "%d\n", if_snd.ifq_len);
			ExpStr->Insert_cell(row, column, tmpBuf, "\n");
			EXPORT_GET_SUCCESS(ExpStr);
                        return 1;
                }
        }

	/* The Get implementation of Exported Variable "max-queue-length" */
        if (!strcmp(argv[0], "Get")&&(argc==2)) {
                if (!strcmp(argv[1], "max-queue-length")) {
			ExpStr = new ExportStr(1);
			row = ExpStr->Add_row();
			column = 1;
                        sprintf(tmpBuf, "%d\n", if_snd.ifq_maxlen);
			ExpStr->Insert_cell(row, column, tmpBuf, "\n");
			EXPORT_GET_SUCCESS(ExpStr);
                        return 1;
                }
        }

	/* The Set implementation of Exported Variable "max-queue-length" */
       if (!strcmp(argv[0], "Set")&&(argc==3)) {
                if (!strcmp(argv[1], "max-queue-length")) {
                        if_snd.ifq_maxlen = atoi(argv[2]);

						// Added by Prof. S.Y. Wang on 10/22/2002
						mytunidp = GET_REG_VAR(get_portls()->pid, "TUNID", u_long *);
						if (mytunidp != NULL) {
							 printf("b: FIFO(nid %d) pid %d tunid %lu\n", get_nid(), get_port(), *mytunidp);
							// This fifo queue is used by a (layer-3) tunnel interface.
							// (Note that a fifo queue may be used by a layer-2 interface,
							// such as a switch port. In that case, the fifo queue is not
							// associated with a tunnel interface.
							fifoIFmaxqlen[*mytunidp] = if_snd.ifq_maxlen;
						} else {
							printf("nid %d pid %d fifo mytunidp == NULL 1\n", get_nid(), get_port());
						}
						EXPORT_SET_SUCCESS();
                        return 1 ;
                }
        }

        return(NslObject::command(argc, argv));
}
