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

#ifndef __NCTUNS_rwa_h__
#define __NCTUNS_rwa_h__

#include <stdio.h>
#include <object.h>
#include <event.h>
#include <module/opt/Lheader.h>

struct Event_pinfo{
	        int             wave;
};

//used for sending packet
struct WaTable {
	u_int8_t	wave;
	u_long		LPID; //by next-hop dst IP
	char		status;
	u_int64_t	time;
	struct WaTable  *next;
};

//used for checking the wavelength is assigned or not
struct WAssigned {
	u_int8_t	wave;
	u_long		LPID; //by pre-hop src IP
	char		status;
	struct WAssigned *next;
};

struct PKTQ {
	ePacket_	*pkt;
	u_long		LPID;
	struct PKTQ	*next;
};

struct AvailWA{
	int		used; //0 :: can't use it 	
	struct AvailWA  *next;
};

class rwa : public NslObject {

 private:
	 
	 //add by chenyuan
	 char 			*nodekindfile;
	 char			*nodeconnectfile;
	 char			*nodepathfile;
	 char			*ringfile;
	 int			WavelenConversion;	
	 struct AvailWA		*AWA;
	 int			max_wave;
	 int			choose_wave;
	 //
	 u_int16_t		WTnum;
	 u_int16_t		WAnum;
	 u_int16_t		PQnum;
	 struct WaTable 	*WT;
	 struct WAssigned	*WA;
	 struct PKTQ		*pktq;

	int                     Wa_lifetime; //unit is sec
        int                     Wa_polling;  //unit is sec
        u_int64_t               walf;        //unit is tick
        u_int64_t               wapl;        //unit is tick

 public:
 	rwa(u_int32_t type, u_int32_t id, struct plist* pl, const char *name); 
 	~rwa();   
	
	int 			Is_avail_wave(int wave);
	int 			get_avail_wave();
	void 			Initial_AWA();
	void			Set_wave_avail(int wave);
	void			Set_wave_unavail(Event_ * ep);
	int			Is_avail_wave();

	//
	int 			init(); 
	
	int                     get(ePacket_ *pkt, MBinder *frm);
	int                     send(ePacket_ *pkt);
	int			recv(ePacket_ *pkt);

 protected:
	int			qpacket(u_long LPID, ePacket_ *pkt);
	int			resume(u_long LPID, char wave);
	int			freequeue(u_long LPID);
	void			wave_tear();
}; 
 

#endif /* __NCTUNS_rwa_h__ */
