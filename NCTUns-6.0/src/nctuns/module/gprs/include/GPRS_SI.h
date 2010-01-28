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

#ifndef GPRS_SI
#define GPRS_SI

class GPRS_SI13
{
    private:
    /* PBCCH description */
    	unsigned short	_tn; /* time slot number used for PBCCH and PCCCHs */
    	unsigned long	_tsc; /* training sequence code for PBCCH and corresponding PCCCH */
    	unsigned long	_arfcn; /* Nonhopping radio frequency absolute RF channel number */
    /* Localization of PSI type 1 information if PBCCH is not present. */
    	
    /* Routing Area Code */
    	unsigned long	_rac;
    
    /* Options available in GPRS cell */
    	unsigned short _nmo; /* (mode 1,2,3) */
    	unsigned short _access_burst_type; /* PRACH on 8 or 11 bits */
    /* Network control order parameters */
        unsigned short _ncop;
    /* GPRS power control parameters: not used currently */
        
    public:
        GPRS_SI13();
        unsigned short	get_TN() 	  { return _tn; }
        unsigned long 	get_TSC()	  { return _tsc; }
        unsigned long 	get_ARFCN()	  { return _arfcn; }
        unsigned long 	get_RAC()	  { return _rac; }
        unsigned short	get_NMO() 	  { return _nmo; }        
        unsigned short	get_ACCESS_BURST_TYPE()	  { return _access_burst_type; }
        unsigned short	get_NCOP() 	  { return _ncop; }
        
        int set_TN (unsigned short tn)	  { _tn  = tn; return 1;}
        int set_TSC(unsigned long  tsc)   { _tsc = tsc; return 1;}
        int set_ARFCN(unsigned long  arfcn) { _arfcn = arfcn; return 1;}
        int set_RAC(unsigned long  rac)   { _rac = rac; return 1;}
        int set_NMO (unsigned short nmo)  { _nmo  = nmo; return 1;}
        int set_ACCESS_BURST_TYPE (unsigned short type)	  { _access_burst_type = type; return 1;}
        int set_NCOP (unsigned short ncop)  { _ncop  = ncop; return 1;}
        
        
};

#endif
