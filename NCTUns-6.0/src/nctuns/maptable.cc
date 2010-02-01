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

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <config.h>
#include <nctuns_api.h>
#include <nctuns_syscall.h>
#include <maptable.h>
#include <tclBinder.h>
#include <tclObject.h>

extern GroupTable grp_table;
SLIST_HEAD(MPTable, maptable)		mtable = {0} ;
static u_int32_t			numNode = 0;


static int mtbl_add(u_long nid, u_long tid, u_char *mac, u_short sport) {
	if (syscall_NCTUNS_mapTable(syscall_NSC_mt_ADD, nid, tid, mac, sport) < 0)
		return(-1);

	return(0);  
}


static int mtbl_init() {
	if (syscall_NCTUNS_mapTable(syscall_NSC_mt_FLUSH, 0, 0, 0, 0) < 0)
		return(-1);

	return (0);
}


static int umtbl_setport() {

	struct maptable		*mt;
	int			nNode = 0;


	/* get the number of nodes to share port range */
	SLIST_FOREACH(mt, &mtable, nextnode) {
		nNode ++;   
	} 

	/* set port range for each node */
	SLIST_FOREACH(mt, &mtable, nextnode) {
		mt->s_port = ((ENDING_PORT-START_PORT)/nNode)*mt->sequ + 
			     START_PORT; 
	}
	return(0); 
}


int umtbl_add(u_int32_t nid, u_int32_t portid, u_int32_t tid, 
	      u_long *ip, u_long *netmask, u_char *mac) 
{

	struct if_info		*io;
	struct maptable		*mt = NULL;
	char			flag = 0;


	/* check to see duplicated tunnel ID */
	if (tid != 0) {
		SLIST_FOREACH(mt, &mtable, nextnode) {
			SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
				if(io->tid == tid) /* duplicated one is found */ 
					return(-1); 
			}
		}     
	}

	SLIST_FOREACH(mt, &mtable, nextnode) {
		if(mt->nodeID == nid) {
			flag = 1;
			break; 
		}  
	}

	/* if no node ID is found in map table, create one */
	if( !flag ) {
		mt = (struct maptable *)malloc(sizeof(struct maptable));
		if( !mt ) return(-1);

		/* fill node information */
		mt->sequ = numNode++;    
 		mt->nodeID = nid;
		mt->portnum = 1;  
		mt->ifinfo.slh_first = 0;
		mt->nextnode.sle_next = 0;

		SLIST_INSERT_HEAD(&mtable, mt, nextnode);
	}
	else mt->portnum++;   



	/* create a if_info entry */
	io = (struct if_info *)malloc(sizeof(struct if_info));
	if(!io)	return(-1);

	/* fill if_info information */
	io->tid = tid;
	io->portid = portid;
	io->mac = mac;
	io->ip = ip;
	io->netmask = netmask;  
	io->nextif.sle_next = 0;
	sprintf(io->name, "tun%u",mt->portnum); 

	SLIST_INSERT_HEAD(&(mt->ifinfo), io, nextif);

	return(0); 
}


int umtbl_configtun() {

	char			cmd[100];
	u_char			*p, *p1; 
	char	format[] = "%s/ifconfig tun%d %d.%d.%d.%d netmask %d.%d.%d.%d"; 
	struct if_info		*io; 
	struct maptable		*mt;

	/* initial kernel maptable */
	assert( mtbl_init() == 0 );

	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
			p = (u_char *)io->ip;
			p1 = (u_char *)io->netmask;   
			sprintf(cmd, format, getenv("NCTUNS_TOOLS"), 
				io->tid, p[0], p[1], p[2],
				p[3], p1[0], p1[1], p1[2], p1[3]); 
			if (system(cmd) != 0) {
				printf("SE:umtbl_configtun(): configuring the "
					"IP address of tun%d failed (given IP:"
					"%d.%d.%d.%d).\n",
					io->tid, p[0], p[1], p[2], p[3]);
				exit(0);
			}
		} 
	}
	return(0); 
}

int umtbl_cpytokern() {

	struct if_info		*io;
	struct maptable		*mt;


	umtbl_setport();

	/* copy to kernel */
	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
			assert(mtbl_add(mt->nodeID, io->tid,
			       io->mac, mt->s_port) == 0); 
		}
	}
	return(0); 
}



u_long mtbl_mactoip(u_char *mac) {

	struct if_info		*io;
	struct maptable		*mt;


	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
                        /* added by C.C. Lin on 01/09/2006.
                         * Since SgsnGtp and NS modules have IPs but not MAC Addresses,
                         * this API should be corrected to prevent from S.E.'s crashes.
                         */
			if( io->mac && !memcmp(io->mac, mac, 6))
				return((u_long)*(io->ip)); 
		}
	}   
	return(0);
}


u_char * mtbl_iptomac(u_long ip) {

	struct if_info		*io;
	struct maptable		*mt;


	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
			if(*(io->ip) == ip)
				return((u_char *)io->mac); 
		}
	}  
	return(0);
}


u_int32_t mtbl_iptonid(u_long ip) {

	struct if_info		*io;
	struct maptable 	*mt;

	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
			if (*(io->ip) == ip)
  				return(mt->nodeID); 
		}
	} 
	return(0);
}

/* Added by C.C. Lin: this mapping translation is required since 
 * several module function will use internal mapping functions directly.
 * For example, GodRouted::update_routing_entry() etc.
 */

struct NidPidProfile {

	u_int32_t nid;
	u_int32_t pid;

};

struct NidPidProfile* find_supernode_mapping(u_int32_t onid, u_int32_t opid) {

	struct NidPidProfile* res_ptr = NULL;

	GT_Entry* tmp_gte_p = grp_table.get_entry(onid, SEARCH_BY_SNID);

	if ( tmp_gte_p ) {

		res_ptr = new NidPidProfile;

		res_ptr->nid = onid;
		res_ptr->pid = opid;

	}
	else {

		tmp_gte_p = grp_table.get_entry(onid, SEARCH_BY_DNID);

		if ( tmp_gte_p ) {

			res_ptr = new NidPidProfile;

			res_ptr->nid = tmp_gte_p->snid;	

			DN_Entry* dne_p = tmp_gte_p->get_dn_entry(onid);

			if (!dne_p) {

				printf("find_supernode_mapping(): the dn_entry is found.\n");
				exit(1);

			}

			PI_Entry* pie_p = dne_p->get_pi_entry_by_ori_pid(opid);

			if (!pie_p) {

				printf("find_supernode_mapping(): the pi_entry is found.\n");
				exit(1);

			}

			res_ptr->pid = pie_p->new_pid;	

		}	

	}


	return res_ptr;

}

u_int32_t mtbl_nidtoip(u_int32_t nid, u_int32_t port) {

	struct if_info		*io;
 	struct maptable		*mt;

	/* Added by C.C. Lin: this mapping translation is required since 
	 * several module function will use internal mapping functions directly.
	 * For example, GodRouted::update_routing_entry() etc.
	 */

	struct NidPidProfile* np_profile_ptr = find_supernode_mapping(nid,port);

	if ( np_profile_ptr ) {

		printf("mtbl_nidtoip(): correct onid = %d opid = %d as nnid = %d npid = %d \n", 
			nid , port , np_profile_ptr->nid , np_profile_ptr->pid );

		nid  = np_profile_ptr->nid;
		port = np_profile_ptr->pid;

	}

 	SLIST_FOREACH(mt, &mtable, nextnode) {
		if (nid == mt->nodeID) 
  			break; 
	}
	if (!mt) return(0);

 	SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
		/* we want to find ip of node */
   		if (io->portid == port)  
			return((u_long)*(io->ip)); 
	}
	return(0); 
}


char *mtbl_getifnamebytunid(u_int32_t tid) {

	struct if_info		*io;
	struct maptable		*mt;

	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
			if (tid == io->tid)
				return(io->name);   
		}
	}  
	return(0); 
}


u_int32_t mtbl_iptopid(u_long ip) {

	struct if_info          *io;
        struct maptable         *mt;

	SLIST_FOREACH(mt, &mtable, nextnode) {
                SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
                        if (*(io->ip) == ip)
                                return(io->portid);
                }
        }
        return(0);
}


u_int32_t mtbl_getportbytunid(u_int32_t tid) {

	struct if_info		*io;
	struct maptable		*mt;

	SLIST_FOREACH(mt, &mtable, nextnode) {
		SLIST_FOREACH(io, &(mt->ifinfo), nextif)
			if (tid == io->tid)
				return(io->portid);
	}
	return(0);
}



u_char mtbl_isbroadcast(u_int32_t id, u_long ip) {

	struct if_info		*io;
	struct maptable		*mt;
	u_char			p[] = { 0xff, 0xff, 0xff, 0xff };  
	u_long			*tstip = (u_long *)p;


	SLIST_FOREACH(mt, &mtable, nextnode) {
		if(mt->nodeID == id) {
			SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
				if( (u_long)(*tstip&~(*(io->netmask))) ==
				    (u_long)(ip&~(*(io->netmask))) )
					return(1); 
			}
		}  
	}  
	return(0); 
}


struct maptable *mtbl_getnidinfo(u_int32_t nid) {

	struct maptable		*mt;

	SLIST_FOREACH(mt, &mtable, nextnode) {
		if (mt->nodeID == nid)
			return(mt);
	}
	return(0);
}



void mtbl_display() {

	struct if_info		*io;
	struct maptable		*mt;
	char			buf[50];


	SLIST_FOREACH(mt, &mtable, nextnode) {
	    printf("\nThe Info. of Layer-3 device: Node %d\n", mt->nodeID);
	    printf("\ts_port: %d\n", mt->s_port);
	    printf("\tsequ: %d\n", mt->sequ);
	    printf("\tportnum: %d\n", mt->portnum);

	    SLIST_FOREACH(io, &(mt->ifinfo), nextif) {
		printf("\t\tport ID: %d\n", io->portid);
		printf("\t\ttunnel ID: %d\n", io->tid);
		ipv4addr_to_str((u_long)*(io->ip), buf);
		printf("\t\tIPv4 addr.: %s\n", buf);
	    	ipv4addr_to_str((u_long)*(io->netmask), buf);
		printf("\t\tnetmask: %s\n", buf);
		macaddr_to_str(io->mac, buf);
		printf("\t\tieee802 mac addr.: %s\n", buf);
		printf("\t\tdevice name: %s\n\n", io->name);
	    }
	};
}


