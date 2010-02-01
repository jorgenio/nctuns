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
#include <string.h>
#include <assert.h>
#include <nctuns_api.h>
#include <packet.h>
#include <gbind.h>

using namespace std;

static u_int64_t _pktid_ = 0;   

Packet::Packet()
{
	/* initialize pbuf */
	pbuf.p_next    = 0;
  	pbuf.p_type    = PT_DATA;
  	pbuf.p_len     = 0;
  	pbuf.p_dat     = PLEN + (char *)&pbuf;
	pbuf.p_tlen    = 0;
 	pbuf.p_pid     = _pktid_ ++;
	pbuf.p_flags   = PF_SEND;
  	pbuf.p_sptr    = 0;
	hdl_.h_flags   = HANDLE_NONE;  
	gateway	       = 0;  
	pkt_err_       = 0;  
	hdr_stack__.sp = 0;
}

Packet::Packet(u_int64_t id)
{
	/* initialize pbuf */
	pbuf.p_next    = 0;
 	pbuf.p_type    = PT_DATA;
 	pbuf.p_len     = 0;
 	pbuf.p_dat     = PLEN + (char *)&pbuf;
 	pbuf.p_tlen    = 0;
 	pbuf.p_pid     = id; 
	pbuf.p_flags   = PF_SEND;
	pbuf.p_sptr    = 0;
	hdl_.h_flags   = HANDLE_NONE;
	gateway	       = 0;
  	pkt_err_       = 0;  
	hdr_stack__.sp = 0;
}


Packet::~Packet()
{
	release();
}


/*---------------------------------------------------------------------------
 * copy()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
Packet * Packet::copy () {

	Packet			*pkt;
	struct pbuf		*pf, *spf, *ipf; 

	 
	pkt = new Packet(pbuf.p_pid);  
	assert(pkt);
  	pf = pkt->pkt_getpbuf();  
	pkt->rt_setgw(gateway); /* duplicate gateway information */

	/* Copy PT_DATA type pbuf */
	(void)memcpy((char *)pf, (char *)&pbuf, PLEN);

	/* Copy PT_DATA type pbuf with flag PF_EXTEND set */
	if (pbuf.p_flags & PF_EXTEND) {
		pf->p_extclstr = (char *)malloc(pcluster);
		assert(pcluster);
		(void)memcpy(pf->p_extclstr+pcluster-pf->p_len,
			pbuf.p_dat, pbuf.p_len);
		pf->p_dat = pf->p_extclstr+pcluster-pf->p_len;
	}
	else pf->p_dat = (char *)pf + PLEN - pf->p_len;

 	/* 
	 * Increase reference count of 
	 * shared buffer if needed.
	 */
	if (pbuf.p_flags & PF_WITHSHARED) {
		spf = (struct pbuf *)pbuf.p_sptr;
  		spf->p_refcnt++;
   	}

	/* Copy PT_INFO type if needed. */
	for(spf=pbuf.p_next; spf; spf=spf->p_next) {
		P_IGET(ipf);
 		(void)memcpy((char *)ipf, (char *)spf, PLEN);
  		ipf->p_dat = (char *)ipf + PLEN - ipf->p_len;
   		pf->p_next = ipf;
  		pf = ipf;  
	}

	/* Copy Callout Information if needed */
	if (hdl_.h_flags == HANDLE_FUNC) {
		pkt->pkt_setHandler(hdl_.h_func);
	}
	if (hdl_.h_flags == HANDLE_OBJ) {
		pkt->pkt_setHandler(hdl_.h_obj, hdl_.h_meth);
	}
	 
	return(pkt);
}


/*---------------------------------------------------------------------------
 * release()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::release()
{
	struct pbuf		*spf;
 
	/* free shared pbuf if needed. */
	if (pbuf.p_flags & PF_WITHSHARED) {
		spf = (struct pbuf *)pbuf.p_sptr;
  		assert(spf); // make sure
		spf->p_refcnt--; 
		if (spf->p_refcnt == 0) {
  			free(spf->p_cluster);
			spf->p_cluster = 0;  
 			free(spf); 
		}

		pbuf.p_sptr = 0;  
	}

	/* free PT_INFO pbuf */
	while (pbuf.p_next) {
		spf = pbuf.p_next;
  		pbuf.p_next = spf->p_next;
  		free(spf); 
	}  

	/* free PT_DATA pbuf with PF_EXTEND is set */
	if (pbuf.p_flags & PF_EXTEND) {
		free(pbuf.p_extclstr);
		pbuf.p_extclstr = 0;
	}

	/* free PT_DATA pbuf */
//	delete this; 

	return 0;
}


/*---------------------------------------------------------------------------
 * pkt_malloc()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_malloc(int len) {

	char			*cluster;


	if ((pbuf.p_len + len > (int)P_DATLEN) && !(pbuf.p_flags & PF_EXTEND)) {
		/* 
		 * P_DATA pbuf is full, try to enlarge
		 * the buffer space: set the PF_EXTEND
		 * flag and malloc a cluster.
		 */
		assert(!(pbuf.p_flags & PF_EXTEND)); // should not happen
		pbuf.p_flags |= PF_EXTEND;

		/*
		 * malloc a new cluster buffer and copy the 
		 * original data in pbuf to cluster buffer.
		 */
		cluster = (char *)malloc(pcluster);
		assert(cluster);
		(void)memcpy(cluster+pcluster-pbuf.p_len,
			     pbuf.p_dat, pbuf.p_len);
		pbuf.p_extclstr = cluster;
		pbuf.p_dat = cluster + pcluster - pbuf.p_len;
	}

	if ((pbuf.p_dat-len<pbuf.p_extclstr)&&(pbuf.p_flags&PF_EXTEND)) {
		printf("Out of cluster buffer!\n");
		exit(-1);
	}

	pbuf.p_len  += len;
     	pbuf.p_tlen += len;
	pbuf.p_dat  -= len;
   	return(pbuf.p_dat); 
}


/*---------------------------------------------------------------------------
 * pkt_prepend()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_prepend(char *data, int len) {

	char			*p;
	
	p = pkt_malloc(len);
	(void)memcpy(p, data, len);
	return(1); 
}


/*---------------------------------------------------------------------------
 * pkt_seek()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_seek(int offset) {

	/* If PF_EXTEND is set */
	if (pbuf.p_flags & PF_EXTEND) {
		if (pbuf.p_dat+offset >= pbuf.p_extclstr+pcluster) {
			pbuf.p_dat = pbuf.p_extclstr+pcluster;
			pbuf.p_tlen -= pbuf.p_len;
			pbuf.p_len = 0;
		} else if (pbuf.p_dat+offset < pbuf.p_extclstr) {
			pbuf.p_len = pcluster;
			pbuf.p_tlen += (pbuf.p_dat - pbuf.p_extclstr);
			pbuf.p_dat = pbuf.p_extclstr;
		} else {
			pbuf.p_dat  += offset;
			pbuf.p_len  -= offset;
			pbuf.p_tlen -= offset;
		}
		return(1);
	}

	/* For PF_EXTEND not set */
	if (pbuf.p_dat+offset >= PLEN+(char *)&pbuf) {
		pbuf.p_dat = PLEN + (char *)&pbuf;
  		pbuf.p_tlen -= pbuf.p_len;
		pbuf.p_len = 0;  
	} else if (pbuf.p_dat+offset < pbuf.p_dbuf) {
		pbuf.p_len = P_DATLEN;
  		pbuf.p_tlen += (pbuf.p_dat - pbuf.p_dbuf);
		pbuf.p_dat = pbuf.p_dbuf;  
	} else {
		pbuf.p_dat  += offset;
   		pbuf.p_len  -= offset;
   		pbuf.p_tlen -= offset;   
	}
	return(1); 
}


/*---------------------------------------------------------------------------
 * pkt_get()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_get() {

	struct pbuf		*pf;

	if (pbuf.p_len > 0)
		return(pbuf.p_dat);
		
	/* 
	 * No data in pbuf of PT_DATA type,
	 * try to return shared data.
	 */
	if (pbuf.p_flags & PF_WITHSHARED) {
		pf = (struct pbuf *)pbuf.p_sptr;
		if (pf->p_len > 0)
			return(pf->p_dat); 
	}
	return(0); 
}


/*---------------------------------------------------------------------------
 * pkt_get()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_get(int offset) {

	struct pbuf		*pf; 

	if (pbuf.p_len-offset > 0)
  		return(pbuf.p_dat+offset);

	/* Try to return shared data. */
	if (pbuf.p_flags & PF_WITHSHARED) {
		pf = (struct pbuf *)pbuf.p_sptr;
		if (pf->p_len > 0)
	  		return(pf->p_dat);
 	}
	return(0);   
}


/*---------------------------------------------------------------------------
 * pkt_sdeattach()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_sdeattach() {

	struct pbuf		*sf;

 
	/* 
	 * Make sure that the pbuf has already
	 * attached shared data buffer.
	 */
	if (!(pbuf.p_flags&PF_WITHSHARED))
		return(-1); 

 	sf = (struct pbuf *)pbuf.p_sptr;
	assert(sf && (sf->p_type&PT_SDATA)); // make sure
	pbuf.p_tlen -= sf->p_len;
 	if (sf->p_refcnt - 1 == 0) {
		/* free shared data buffer */
		free(sf->p_cluster);
 		free(sf); 
  		pbuf.p_flags &= ~PF_WITHSHARED;  // modified by jlchou
		return(1); 
	} 
	/* otherwise; decrease reference counter */
	sf->p_refcnt--;
 	pbuf.p_sptr = 0;
  	pbuf.p_flags &= ~PF_WITHSHARED;  
	return(1); 
}


/*---------------------------------------------------------------------------
 * pkt_sattach()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_sattach(const int &len)
{
	struct pbuf	*sf;
 
	/* 
	 * If pbuf with PT_DATA already has attached
	 * shared pbuf, we should not do it again.
	 */
	if (pbuf.p_flags & PF_WITHSHARED)
		return(0); 

 	P_SATTACH(&pbuf, len); 
	sf = (struct pbuf *)pbuf.p_sptr;  
	if (sf) {
		assert(sf->p_type&PT_SDATA);    // make sure
		return(sf->p_cluster+P_DATLEN); 
	}
	return(0); 
}


/*---------------------------------------------------------------------------
 * pkt_sget()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_sget() {

	struct pbuf		*sf;

	sf = (struct pbuf *)pbuf.p_sptr;
 	if (sf) {
		assert(sf->p_type&PT_SDATA);    // make sure
		return(sf->p_cluster+P_DATLEN);
	}
	return(0); 
}


/*---------------------------------------------------------------------------
 * pkt_sprepend()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_sprepend(char *data, int len) {

	struct pbuf		*sf;

	/* 
	 * Make sure that the flags of pbuf 
	 * is PF_WITHSHARED.
	 */
 	if (!(pbuf.p_flags&PF_WITHSHARED)) 
		return(-1); 

	/*
	 * If the pbuf of PT_SDATA type found but
	 * the p_cluster and the data don't point
	 * to the same buufer, then we reject to
	 * prepend the data to shared data buffer.
	 */
	sf = (struct pbuf *)pbuf.p_sptr;
 	if (!sf) return(-1);
	if ((sf->p_type & PT_SDATA) &&
	    (sf->p_cluster+P_DATLEN == data)) {
		sf->p_len += len;
   		pbuf.p_tlen += len;
   		return(1); 
	}   
	return(-1); 
}


/*---------------------------------------------------------------------------
 * info_malloc()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::info_malloc(int len)
{
	return NULL; 	
}


/*---------------------------------------------------------------------------
 * pkt_addinfo()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_addinfo(const char *iname, const char *info, int len) {

	struct pbuf		*ipf, *f;
	char			*p; 


	if(len > (int)INFOBODY) {
		printf("Warning: The size of %s should be less than %d\n",
			iname, INFOBODY);
		return(-1);
	}
 
	p = pkt_getinfo(iname);
        if (p != NULL) {
		(void)memcpy((char *)p, info, len);
	        return(1);
        }

	pbuf.p_flags |= PF_WITHINFO;  
  	for(ipf=&pbuf, f=0; ipf; f=ipf, ipf=ipf->p_next) {
		if ((ipf->p_type & PT_INFO) &&
		    (ipf->p_dat - INFOSIZE >= ipf->p_infobuf)) {
			INFO_COPYIN(ipf, iname, info, len); 
			return(1); 
		}
	}

 	/* 
	 * No enough space to store pkt_INFO or no pbuf of
	 * PT_INFO type found.
	 */
	P_IGET(ipf);
	INFO_COPYIN(ipf, iname, info, len); 
  	f->p_next = ipf;
	return(1); 
}


/*---------------------------------------------------------------------------
 * pkt_saddinfo()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_saddinfo(char *iname, const char *info, int len) {

	struct pbuf		*pf;
	char			*p;

	if (len > (int)INFOSIZE)
		return(-1);

	p = pkt_sgetinfo(iname);
        if (p != NULL) {
                (void)memcpy((char *)p, info, len);
                return(1);
        }

	pf = (struct pbuf *)pbuf.p_sptr;
  	if (!pf) return(-1);
 	if (pf->p_dat - INFOSIZE >= pf->p_sbuf) {
		pf->p_type |= PT_SINFO;  
		INFO_COPYIN(pf, iname, info, len); 
		return(1); 
	} 

	/* 
	 * I don't want to create another pbuf to
	 * store pkt_info again, cause the shared
	 * data buffer only has one pbuf to store
	 * pkt_info.
	 */
	return(-1); 
}


/*---------------------------------------------------------------------------
 * pkt_getinfo()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_getinfo(const char *iname) {

	struct pbuf		*ipf;
	char			*p; 
	int			infolen;

	/* lookup in data buffer */
 	for(ipf=&pbuf; ipf; ipf=ipf->p_next) {
		if (ipf->p_type & PT_INFO) {
		   p = (char *)ipf + PLEN - INFOSIZE;
		   infolen = ipf->p_len;
   		   for(; p>=ipf->p_infobuf&&infolen>0; p-=INFOSIZE) {
		       infolen -= INFOSIZE;
		       if (!strncmp(p, iname, INFONAME)) /* found */
		 	    return(p+INFONAME+1);   
		   }
		}
	}    

	return(0);
}



/*---------------------------------------------------------------------------
 * pkt_sgetinfo()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_sgetinfo(const char *iname) {

	struct pbuf             *ipf;
        char                    *p;
        int                     infolen;


	/* lookup in shared data buffer */
	ipf = (struct pbuf *)pbuf.p_sptr;
 	if (ipf && (ipf->p_type&PT_SINFO)) {
		p = (char *)ipf + PLEN - INFOSIZE;
		infolen = ipf->p_len;
    		for(; p>=ipf->p_sbuf&&infolen>0; p-=INFOSIZE) {
		    infolen -= INFOSIZE;
    		    if (!strncmp(p, iname, INFONAME)) /* found */
			return(p+INFONAME+1);  
		}	
	}
	return(0); 
}


/*---------------------------------------------------------------------------
 * pkt_setflow()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
void Packet::pkt_setflow(short flag) {

	if (flag == PF_RECV) {
	    	pbuf.p_flags &= ~PF_SEND;
		pbuf.p_flags |= PF_RECV;
		return;   
	}
	if (flag == PF_SEND) {
		pbuf.p_flags &= ~PF_RECV;
  		pbuf.p_flags |= PF_SEND;
  		return; 
	}    
}


/*---------------------------------------------------------------------------
 * pkt_aggregate()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
char * Packet::pkt_aggregate() {

	struct pbuf		*sf;
 
	/*
	 * The method pkt_aggregate() is to make the
	 * whole data in the continuous memory spaces.
	 */

	if (pbuf.p_tlen == 0)
  		return(0);

	/*
	 * If pbuf without shared-pbuf attached,
	 * we just return the pbuf data.
	 */
	if (!(pbuf.p_flags&PF_WITHSHARED)) 
		return(pbuf.p_dat); 

	assert((sf=(struct pbuf *)pbuf.p_sptr)); // never happen

	/*
	 * If no data in PT_DATA pbuf, then we
	 * just return the PT_SDATA pbuf 
	 */
	if (pbuf.p_len == 0) {
		return((char *)sf->p_cluster+P_DATLEN);
	}

	/*
	 * Only for PT_DATA buf with PF_EXTEND unset.
	 *
	 * There are data in PT_DATA and PT_SDATA,
	 * so we should copy data of PT_DATA pbuf into
	 * cluster of PT_SDATA pbuf.
	 */
	if (!(pbuf.p_flags & PF_EXTEND)) {
		(void)memcpy((char *)(sf->p_cluster+P_DATLEN-pbuf.p_len),
			     (char *)pbuf.p_dat, pbuf.p_len); 
		return((char *)(sf->p_cluster+P_DATLEN-pbuf.p_len)); 
	}

	/* 
	 * Only for PT_DATA buf with PF_EXTEND is set
	 *
	 * If PF_EXTEND pbuf, I temporarily don't copy data
	 * in cluster and shared data to a continuous buffer,
	 * just return shared data instead.
	 */
	if (pbuf.p_flags & PF_EXTEND)
		return((char *)sf->p_cluster+P_DATLEN);

	return NULL;
}


/*---------------------------------------------------------------------------
 * pkt_setHandler()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int
Packet::pkt_setHandler(NslObject *o, int (NslObject::*meth_)(Event *)) {

	hdl_.h_flags = HANDLE_OBJ;
  	hdl_.h_obj   = o;
  	hdl_.h_meth  = meth_;  
	return(1); 
}


/*---------------------------------------------------------------------------
 * pkt_setHandler()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_setHandler(int (*func_)(Event *)) {

	hdl_.h_flags = HANDLE_FUNC;
  	hdl_.h_func  = func_;
	return(1);   
}


/*---------------------------------------------------------------------------
 * pkt_callout()
 *
 *
 * Arguments:
 *
 * Returns:
 *
 * Side effects:
 *
 *---------------------------------------------------------------------------*/
int Packet::pkt_callout(ePacket_ *pkt) {

	if (((Packet *)pkt->DataInfo_) == NULL)
		return(-1);

	switch(hdl_.h_flags) {
	
	case HANDLE_NONE:
		return(-1);
	
	case HANDLE_FUNC:
		setFuncEvent(pkt,
			     GetCurrentTime(),
			     0,
			     hdl_.h_func,
			     (void *)this
		);
		return(1);

	case HANDLE_OBJ:
		setObjEvent(pkt,
			    GetCurrentTime(),
			    0,
			    hdl_.h_obj,
			    hdl_.h_meth,
			    (void *)this
		);
		return(1);

	default: 
		return(-1);
	}
}


int Packet::pkt_pushproto(u_char proto, u_int32_t len) {

	if (hdr_stack__.sp >= MAX_HDR_PROTO)
		return(-1);

	hdr_stack__.hdr_p[hdr_stack__.sp].hdr_proto = proto;
	hdr_stack__.hdr_p[hdr_stack__.sp++].hdr_len = len;
	return(1);
}


u_char Packet::pkt_popproto() {

	if (hdr_stack__.sp <= 0)
		return(0);
	return(hdr_stack__.hdr_p[hdr_stack__.sp--].hdr_proto);
}


u_char Packet::pkt_curproto() {

	if (hdr_stack__.sp <= 0)
		return(0);
	return(hdr_stack__.hdr_p[hdr_stack__.sp].hdr_proto);
}

