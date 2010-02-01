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

#ifndef __NCTUNS_packet_h__
#define __NCTUNS_packet_h__

#include <sys/types.h>
#include <ip.h>
#include <event.h>

/*=====================================================================
   Define PBUF Data Structure
  =====================================================================*/
#define RESV			10
#define PLEN			128 //256


/* structure for pbuf header */

struct pbuf; 
struct p_hdr {
	struct pbuf		*ph_next;  /* next buffer in chain */ 
	short			ph_type;   /* type of pbuf */
	int			ph_len;    /* amount of data in this pbuf */
	char			*ph_data;  /* pointer to data */ 
};

struct s_exthdr {
	u_int64_t		com1;
	u_int32_t 		com2;
	char			*com3; 
}; 


/* type of pbuf */
#define PT_DATA			0x0001
#define PT_SDATA		0x0002
#define PT_INFO			0x0004
#define PT_SINFO		0x0008

/* flags of pbuf */
#define PF_SEND			0x0001
#define PF_RECV			0x0002
#define PF_WITHSHARED		0x0004
#define PF_WITHINFO		0x0008
#define PF_EXTEND		0x1000

#define P_INFOLEN		(PLEN - sizeof(struct p_hdr))
#define P_SDATLEN               (P_INFOLEN - sizeof(struct s_exthdr))
#define P_DATLEN		(P_SDATLEN - sizeof(short))
#define P_EXTLEN		(P_DATLEN - sizeof(char *))

#define INFOSIZE		P_INFOLEN
#define INFONAME		5  /* info name size */
#define INFOBODY		(INFOSIZE - INFONAME -1)

#define MAXPACKETSIZE		1600


/* structure for pbuf */
struct pbuf {
	struct p_hdr	p_hdr;
	union {
	   struct {
		struct s_exthdr	exthdr;
		union {
		    struct {
			short	flags;
			union {
			    struct {
				char	*ext;
				char	extbuf[P_EXTLEN];
			    } DHDR0;
			    char	datbuf[P_DATLEN];  
			} d_data;
		    } DHDR; 
		    char	sdatbuf[P_SDATLEN]; 
		} e_data; 
	   } EHDR; 
	   char		infobuf[P_INFOLEN]; 
	} p_data; 
}; 

/* common packet header */
#define p_next			p_hdr.ph_next
#define p_type			p_hdr.ph_type
#define p_len			p_hdr.ph_len
#define p_dat			p_hdr.ph_data

/* PT_INFO: p_hdr + p_infobuf */   
#define p_infobuf		p_data.infobuf

/* PT_SDATA: p_hdr + exthdr + sdat */ 
#define p_pid			p_data.EHDR.exthdr.com1
#define p_refcnt		p_data.EHDR.exthdr.com2
#define p_cluster		p_data.EHDR.exthdr.com3
#define p_sbuf			p_data.EHDR.e_data.sdatbuf

/* PT_DATA: p_hdr + exthdr + flags + dat */    
#define p_tlen			p_data.EHDR.exthdr.com2
#define p_sptr			p_data.EHDR.exthdr.com3
#define p_flags			p_data.EHDR.e_data.DHDR.flags
#define p_dbuf			p_data.EHDR.e_data.DHDR.d_data.datbuf

/* PT_DATA with flag|PF_EXTEND */
#define p_extclstr		p_data.EHDR.e_data.DHDR.d_data.DHDR0.ext
#define p_extbuf		p_data.EHDR.e_data.DHDR.d_data.DHDR0.extbuf

/*=====================================================================
   Define Packet Macros
  =====================================================================*/
#define GET_PKT(_p_, _ep_) 					\
	assert((_ep_)&&((_p_)=(Packet *)(_ep_)->DataInfo_))

#define ATTACH_PKT(_p_, _ep_) 					\
	(_ep_)->DataInfo_ = (void *)(_p_)

#define P_DGET(dpf)  						\
{								\
	struct pbuf		*pf;				\
								\
	pf = (struct pbuf *)malloc(sizeof(struct pbuf));	\
	assert(pf);						\
								\
	pf->p_next  = 0;					\
	pf->p_type  = PT_DATA;					\
	pf->p_len   = 0;					\
	pf->p_dat   = PLEN + (char *)pf;			\
	pf->p_tlen  = 0;        				\
	pf->p_sptr  = 0;					\
	pf->p_flags = PF_SEND;    				\
	dpf = pf;  						\
}
  
#define P_IGET(ipf)						\
{								\
	struct pbuf		*pf;				\
								\
	pf = (struct pbuf *)malloc(sizeof(struct pbuf));	\
	assert(pf);						\
								\
	pf->p_next = 0;						\
	pf->p_type = PT_INFO;  					\
	pf->p_len  = 0;						\
	pf->p_dat  = PLEN + (char *)pf;				\
	ipf = pf;  						\
}

#define P_SGET(spf, len)					\
{								\
	struct pbuf		*pf; 				\
								\
	pf = (struct pbuf *)malloc(sizeof(struct pbuf));  	\
	assert(pf); 						\
								\
	pf->p_next    = 0;  					\
	pf->p_type    = PT_SDATA;  				\
	pf->p_len     = 0;  					\
	pf->p_dat     = PLEN + (char *)pf; 			\
	pf->p_refcnt  = 1; 					\
	pf->p_cluster = (char *)malloc(len+P_DATLEN+RESV);	  	\
	assert(pf->p_cluster); 					\
	spf = pf;  						\
}
	
  
#define P_SATTACH(dp, l) 					\
{								\
	struct pbuf		*sf;   				\
								\
	P_SGET(sf, l); 						\
	sf->p_pid = (dp)->p_pid; 				\
	(dp)->p_flags |= PF_WITHSHARED;  			\
	(dp)->p_sptr = (char *)sf; 				\
}


#define INFO_COPYIN(pf, name, info, len) 			\
{								\
	char		*p; 					\
								\
	pf->p_dat -= INFOSIZE;  				\
	pf->p_len += INFOSIZE;					\
	p = pf->p_dat;						\
	strncpy(p, name, INFONAME);				\
	*(p+INFONAME) = '\0';					\
	(void)memcpy(p+INFONAME+1, info, len);			\
}
  
  
struct Handler {
	u_char				flags;
	union {
		int			(*func_)(Event *);
		struct {
			NslObject	*callObj;
			int		(NslObject::*meth_)(Event *); 
		} Obj_;  
	} un;  
}; 

/* flags for flags of strut Handler */
#define HANDLE_NONE		0x00
#define HANDLE_FUNC		0x01
#define HANDLE_OBJ		0x02

/* alias for struct Handler */
#define h_flags			flags
#define h_obj			un.Obj_.callObj
#define h_meth			un.Obj_.meth_
#define h_func			un.func_ 

/* Header Stack, used to describe the frame encapsulation */
#define MAX_HDR_PROTO           20
struct hdr_stack {
	struct {
		u_char		hdr_proto;	/* header protocol */
		u_int32_t	hdr_len;	/* header length */
	} hdr_p[MAX_HDR_PROTO];
	int			sp;
};

/*=====================================================================
   Define Packet Class
  =====================================================================*/
class Packet {
private:
 	struct pbuf	pbuf;
	char ___mm[RESV]; // extra space to append data
	struct Handler hdl_; 
	struct hdr_stack hdr_stack__;
	u_long gateway; 

public:  
	int	pkt_err_; 
	
	Packet();
	Packet(u_int64_t id); 
	~Packet();
	
	Packet *copy();
	int release();
	char *pkt_malloc(int len); 
	int	pkt_prepend(char *data, int len);
	int	pkt_seek(int offset);
	char *pkt_get();
	char *pkt_get(int offset);
	char *pkt_sattach(const int &len);
	int pkt_sdeattach(); 
	int pkt_sprepend(char *data, int len); 
	char *pkt_sget(); 
	int pkt_addinfo(const char *iname, const char *info, int len); 
	char *pkt_getinfo(const char *iname); 
	char *pkt_sgetinfo(const char *iname);
	char *info_malloc(int len); 
	int	pkt_saddinfo(char *iname, const char *info, int len); 
	inline int pkt_getlen() const { return(pbuf.p_tlen); };
	inline short pkt_use_cluster() const { return(pbuf.p_flags & PF_EXTEND); };
	inline int pkt_get_pt_data_len() const { return(pbuf.p_len); };
	inline void pkt_setlen(int len) { pbuf.p_tlen = len; }; 
	inline u_int64_t pkt_getpid() { return(pbuf.p_pid); };   
	inline struct pbuf *pkt_getpbuf() { return(&pbuf); };
	inline short pkt_getflags() { return(pbuf.p_flags); };  
	void pkt_setflow(short flag); 
	void rt_setgw(u_long gw) { gateway = gw;  };  
	u_long rt_gateway() { return(gateway); };   
	char *pkt_aggregate(); 
	int	pkt_setHandler(NslObject *o, 
	int (NslObject::*meth_)(Event *)); 
	int	pkt_setHandler(int (*func_)(Event*)); 
	int	pkt_callout(ePacket_ *pkt);
	int	pkt_pushproto(u_char, u_int32_t);
	u_char pkt_popproto();
	u_char pkt_curproto();
};

#endif	/* __NCTUNS_packet_h__ */

