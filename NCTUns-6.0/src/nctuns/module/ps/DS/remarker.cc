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
#include <ethernet.h>
#include <exportStr.h>
#include <mbinder.h>

#include <ps/DS/remarker.h>

#ifdef LINUX
#include <netinet/in.h>
#endif

extern u_int32_t *fifoIFcurqlen;
extern u_int32_t *fifoIFmaxqlen;

MODULE_GENERATOR(Remarker);

Remarker::Remarker(u_int32_t type, u_int32_t id,struct plist *pl, const char *name)
                : NslObject(type, id, pl, name)
{

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


	log_qlen_flag = NULL;
	log_option = NULL;
	logQlenFileName = NULL;

	debug=0;
}

Remarker::~Remarker() {

}


int Remarker::init() {

	int         (NslObject::*upcall)(MBinder *);
	u_long 		*mytunidp;

        // Moved by Prof. Wang from fifo::fifo() to here on 08/06/2003.
        // The old place is a wrong place to execute the following piece of code.

	// Added by Prof. S.Y. Wang on 10/22/2002
	mytunidp = GET_REG_VAR(get_port(), "TUNID", u_long *);
	if (mytunidp != NULL) {
		// This fifo queue is used by a (layer-3) tunnel interface.
		// (Note that a fifo queue may be used by a layer-2 interface,
		// such as a switch port. In that case, the fifo queue is not
		// associated with a tunnel interface.
		fifoIFmaxqlen[*mytunidp] = if_snd.ifq_maxlen;
	} else
		printf("nid %d pid %d Remarker mytunidp == NULL 0\n", get_nid(), get_port());


	/* export variable */
	EXPORT("ds-cur-queue-length", E_RONLY|E_WONLY);
	EXPORT("ds-max-queue-length", E_RONLY|E_WONLY);

	/* check log flag */
	if ( (log_qlen_flag)&&(!strcasecmp(log_qlen_flag, "on")) ){
		/* open log file */
		char * FILEPATH = (char *)malloc(strlen(GetConfigFileDir()) +
					strlen(logQlenFileName));
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
			type = POINTER_TO_MEMBER(Remarker, log);
			logTimer.setCallOutObj(this, type);
			logTimer.start(samplerate_tick, samplerate_tick);
		}
	}

	/* set upcall */
        upcall = (int (NslObject::*)(MBinder *))&Remarker::intrq;
        sendtarget_->set_upcall(this, upcall);
        return(1);
}

int Remarker::log() {
	double		current_time;

	current_time = GetCurrentTime() * TICK / 1000000000.0;
	
	if ( (log_qlen_flag)&&(!strcasecmp(log_qlen_flag, "on")) )
		fprintf(logQlenFile, "%.3f\t%d\n", current_time, if_snd.ifq_len);

	return(1);
}


/*
 * This is the only function differ from FIFO module.
 * do the remark here!!
 * */
int Remarker::send(ePacket_ *pkt) {

	u_long *mytunidp;
	assert(pkt&&pkt->DataInfo_);

	//---------------------------------------------------------------------------
	// REMARK
	struct ether_header *eh;
	Packet *p;
	
	GET_PKT(p,pkt);
	eh=(struct ether_header *)p->pkt_get();

	// if pkt is a ip packet, remark it.
	if(ntohs(eh->ether_type)==0x0800){
		struct ip *iph = (struct ip *)p->pkt_sget();

		// if pkt is a Network control pkt, preserve its original DSCP
		if( ((iph->ip_tos) &31) !=0)		// '00011111'
			iph->ip_tos = 0;
	}
	//---------------------------------------------------------------------------

	if( sendtarget_->qfull() ) {
		/* MBQ is full, insert to ifq */
		if (IF_QFULL(&if_snd)) {
			/* ifq full, drop it! */
			IF_DROP(&if_snd);

			
			Packet *pkt_;
			GET_PKT(pkt_,pkt);
			if ( (log_qlen_flag)&&(!strcasecmp(log_qlen_flag, "on")) )
			fprintf(logQlenFile, "Remarker(N%d,P%d), QFULL->DROP : pid %llu\n",
				get_nid(), get_port(), pkt_->pkt_getpid());
			freePacket(pkt);

			return(1);
		}
		/* otherwise, ifq not full, insert it */
		IF_ENQUEUE(&if_snd, pkt);
		if(full_log_flag == 1)
			log();

		// Added by Prof. S.Y. Wang on 10/22/2002
		mytunidp = GET_REG_VAR(get_port(), "TUNID", u_long *);
		if (mytunidp != NULL) {
			// This Remarker queue is used by a (layer-3) tunnel interface.
			// (Note that a Remarker queue may be used by a layer-2 interface,
			// such as a switch port. In that case, the Remarker queue is not
			// associated with a tunnel interface.
			fifoIFcurqlen[*mytunidp] = if_snd.ifq_len;
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


int Remarker::recv(ePacket_ *pkt) {

	/* Just by pass incoming packet */
	assert(pkt&&pkt->DataInfo_);

	return(NslObject::recv(pkt));
}


int Remarker::intrq(MBinder *port) {

	ePacket_	*pkt;
	u_long 		*mytunidp;
	char		log_flag;

	log_flag = 0;
	if( if_snd.ifq_len > 0 )
		log_flag = 1;

	/*
	 * Push the packet in the interface queue
	 * to the MBQ. Whenever the pakcet in the
	 * MBQ is sent, the scheduler will call this
	 * member function to give Remarker module a 
	 * chance to send the next packet in the
	 * interface queue.
	 */
	
	IF_DEQUEUE(&if_snd, pkt);
	if( log_flag == 1 && full_log_flag == 1 )
		log();	

	// Added by Prof. S.Y. Wang on 10/22/2002
	mytunidp = GET_REG_VAR(get_port(), "TUNID", u_long *);
	if (mytunidp != NULL) {
		// This Remarker queue is used by a (layer-3) tunnel interface.
		// (Note that a Remarker queue may be used by a layer-2 interface,
		// such as a switch port. In that case, the Remarker queue is not
		// associated with a tunnel interface.
		fifoIFcurqlen[*mytunidp] = if_snd.ifq_len;
	}

	if (pkt != NULL) {
		/*
		 * If still exist packet in the interface
		 * queue, we try to push it to the MBQ,
		 */
		assert(sendtarget_->enqueue(pkt) == 0);
	}
	return(1);
}


int Remarker::command(int argc, const char *argv[]) {
	char			tmpBuf[10];
	struct ExportStr	*ExpStr;
	u_int32_t		row,column;
	u_long *mytunidp;

	/* The Get implementation of Exported Variable "ds-cur-queue-length" */
        if (!strcmp(argv[0], "Get")&&(argc==2)) {
                if (!strcmp(argv[1], "ds-cur-queue-length")) {
			ExpStr = new ExportStr(1);
			row = ExpStr->Add_row();
			column = 1;
                        sprintf(tmpBuf, "%d\n", if_snd.ifq_len);
			ExpStr->Insert_cell(row, column, tmpBuf, "\n");
			EXPORT_GET_SUCCESS(ExpStr);
                        return 1;
                }
        }

	/* The Get implementation of Exported Variable "ds-max-queue-length" */
        if (!strcmp(argv[0], "Get")&&(argc==2)) {
                if (!strcmp(argv[1], "ds-max-queue-length")) {
			ExpStr = new ExportStr(1);
			row = ExpStr->Add_row();
			column = 1;
                        sprintf(tmpBuf, "%d\n", if_snd.ifq_maxlen);
			ExpStr->Insert_cell(row, column, tmpBuf, "\n");
			EXPORT_GET_SUCCESS(ExpStr);
                        return 1;
                }
        }

	/* The Set implementation of Exported Variable "ds-max-queue-length" */
       if (!strcmp(argv[0], "Set")&&(argc==3)) {
                if (!strcmp(argv[1], "ds-max-queue-length")) {
                        if_snd.ifq_maxlen = atoi(argv[2]);

						// Added by Prof. S.Y. Wang on 10/22/2002
						mytunidp = GET_REG_VAR(get_port(), "TUNID", u_long *);
						if (mytunidp != NULL) {
							 printf("b: FIFO(nid %d) pid %d tunid %lu\n", get_nid(), get_port(), *mytunidp);
							// This Remarker queue is used by a (layer-3) tunnel interface.
							// (Note that a Remarker queue may be used by a layer-2 interface,
							// such as a switch port. In that case, the Remarker queue is not
							// associated with a tunnel interface.
							fifoIFmaxqlen[*mytunidp] = if_snd.ifq_maxlen;
						} else {
							printf("nid %d pid %d Remarker mytunidp == NULL 1\n", get_nid(), get_port());
						}
						EXPORT_SET_SUCCESS();
                        return 1 ;
                }
        }

        return(NslObject::command(argc, argv));
}



