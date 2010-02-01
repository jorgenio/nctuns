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
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include <linux/if.h>
#include <linux/if_tun.h>

#include <tun_mmap.h>

struct poll	iftun_[MAX_NUM_TUN] = { {0, 0} };  
uint32_t ifcnt_ = 0; 

u_int32_t *fifoIFcurqlen;
u_int32_t *fifoIFmaxqlen;
u_int32_t *tunIFqlen;
u_int32_t *t0eqlen;   // special tunnel qlen
u_int64_t *currentTime_;

//
// Noted by Prof. S.Y. Wang on 10/22/2002
// We need to map fifos' current and maximum queue length into the kernel so
// that tunoutput() can timely drop a packet when the fifo queue is full. This
// operation is very important because when tcp_output() calls tunoutput() and
// finds that a packet is dropped due to a full queue, it will call
// tcp_quench() to reduce its TCP congestion window size to avoid further
// packet dropping. Without doing this, a lot of packets will successively
// dropped during the TCP slow-start phase and severely affect a TCP
// connection's throughput.
// 
// Important!!! Due to the simulator architecture's limitation, the above
// approach only works for the simple fifo module. For all other more
// sophisticated packet scheduling or buffer management modules that may
// drop a packet before the total buffer space is used up (e.g., RED and
// round-robin), the above approach does not work. As such, we require that for
// layer-3 hosts and mobile nodes in which tcp_quench may need to be called, 
// they must use the fifo module. Layer-2 devices such as switches, on the
// other hand, can use these sophisciated PSBM modules because these devices do
// not have a TCP/IP protocol stack (and thus the TCP_quench need not be
// called).
// 
// For layer-3 routers, although they are layer-3 devices, we allow them to use
// these sophisciated PSBM modules. This is because when a packet is dropped,
// tcp_quench() in the TCP/IP protocol stack of the router need not be called. 
// However, in the ip_input.c file of the kernel, we must prevent the router
// from generating ICMP source_quench packet and sending it to the TCP source
// node of the dropped packet. The best way to do this to enlarge the maximum
// queue length of the tunnel interface so that no packets will be dropped in
// the tunnel interface before they are dropped in the RED queue (in the
// simulation engine). Therefore, we need to set the maximum queue length of
// every tunnel interface to a very large. Also, to prevent the fifo timely
// packet dropping mechanism from unnecessarily dropping packets in the kernel
// for these sophisciated PSBM modules, we should set the mapped maximum queue
// length of fifo queue length in the kernel to a very large.
//

/*
static struct kvm_list {
	const char *const symbol;
	off_t address;
} klist[] = {
	{"NCTUNS_nodeVC", 0},
	{"tunif_qlen", 0},
	{"ce_tun_qlen", 0},
	{"fifoIFmaxqlen", 0},
	{"fifoIFcurqlen", 0},
	{NULL, 0}
	};
*/
#define NCTUNS_TUN_DEVICE	"/dev/net/nctuns_tun"

//nctuns_dev mmap start
#define NCTUNS_DEVICE		"/dev/nctuns_dev"
//nctuns_dev mmap end

#define OFFSET 0x00000000
#define PTR_POS(Val) ((unsigned long)(Val))
/*
static void search_kvm()
{
	FILE *fd;
	char buf[200], path[200];
	struct utsname version;

	if (uname(&version) < 0) {
		perror("uname");
		exit(1);
	}
	sprintf(path, "/boot/System.map-%s", version.release);

	//
	// read the symbol table of this current kernel from the /proc/kallsyms
	//
	if (!(fd = fopen(path, "r"))) {
		char *home = getenv("NCTUNSHOME");

		if (!home) {
			perror("getenv");
			exit(1);
		}
		sprintf(path, "%s/etc/System.map", home);
		if (!(fd = fopen(path, "r"))) {
			printf("Cannot find symbol tables of Linux kernel!!\n");
			exit(1);
		}
	}

	for (fgets(buf, 200, fd); !feof(fd); fgets(buf, 200, fd)) {
		off_t addr;
		char str_type, str_sym[65];
		int i;
		int check = 0;

		//
		// the format of symbol tables is "address type symbol"
		///
		if (sscanf(buf, "%8lx %c %64s\n", &addr, &str_type, str_sym) != 3)
			break;

		//
		// match all symbol in kvm_list and find the address of this symbol
		//
		for (i = 0; klist[i].symbol; i++) {
			const char *const sym = klist[i].symbol;

			if (klist[i].address) {
				check++;
				continue;
			}

			if (!strcmp(sym, str_sym)) {
				klist[i].address = addr;
				break;
			}
		}

		//
		// we only need find five symbol, when they both have found,
		// stop it
		//
		if (check == 5)
			break;
	}
	fclose(fd);
}
*/
int tun_mmap()
{
	int 	fd, i;

//nctuns_dev mmap start
	struct nctuns_dev{
        uint64_t        NCTUNS_nodeVC[2];
        uint32_t        tunif_qlen[MAX_NUM_TUN];
        uint32_t        ce_tun_qlen;
        uint32_t        fifoIFmaxqlen[MAX_NUM_TUN];
        uint32_t        fifoIFcurqlen[MAX_NUM_TUN];
	};

	struct nctuns_dev *dev;	
//nctuns_dev mmap end


/* mmap /dev/kmem start
	u_int64_t *tick;
	u_int32_t *tunifqlen, *fifoifcurqlen, *fifoifmaxqlen, *t0e;

	int	offset, pagesize;
	
	
	if((fd = open("/dev/kmem", O_RDWR)) < 0) {
		printf("nctuns: open /dev/kmem error!\n");
		exit(-1);
	}
	
	search_kvm();
	pagesize = getpagesize() - 1;
mmap /dev/kmem end */

//nctuns_dev mmap start
	if((fd = open(NCTUNS_DEVICE, O_RDWR)) < 0) {
		printf("nctuns: open nctuns_device error!\n");
		exit(-1);
	}

	dev = (struct nctuns_dev*)mmap(0, sizeof(struct nctuns_dev), PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);

	if(dev == MAP_FAILED) {
		perror("mmap(nctuns_dev)");
		exit(1);
	}
//nctuns_dev mmap end
	
	/*
	 * 
	 * map VirtualClockTick
	 *
	 */

	
/* mmap /dev/kmem start
	if (klist[0].address == 0) {
		printf("nctuns: search %s in kernel image error!!\n", klist[1].symbol);
		exit(-1);
	}
	offset = PTR_POS(klist[0].address - OFFSET) - (PTR_POS(klist[0].address - OFFSET) & ~pagesize);
	
	tick = (u_int64_t *)mmap(0, PTR_POS(offset) + sizeof(u_int64_t) * 2,
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, (PTR_POS(klist[0].address - OFFSET) & ~pagesize));
	if (tick == MAP_FAILED) {
		perror("mmap(tick)");
		exit(1);
	}
	currentTime_ = (u_int64_t *)(PTR_POS(tick) + PTR_POS(offset));

	currentTime_[0] = 0;
mmap /dev/kmem end */ 

//nctuns_dev mmap start
	currentTime_ = (u_int64_t *)(PTR_POS(dev->NCTUNS_nodeVC));

	currentTime_[0] = 0;
//nctuns_dev mmap end
	
	/*
	 * 
	 * map tunnnel output queue length
	 * 
	 */

/* mmap /dev/kmem start
	if (klist[1].address == 0){
		printf("nctuns: search %s in kernel image error!!\n", klist[1].symbol);
		exit(-1);
	}
	offset = PTR_POS(klist[1].address - OFFSET) - (PTR_POS(klist[1].address - OFFSET) & ~pagesize);
	
	tunifqlen = (u_int32_t *)mmap(0, PTR_POS(offset) + sizeof(u_int32_t) * MAX_NUM_TUN, 
			PROT_READ, MAP_PRIVATE, fd, PTR_POS(klist[1].address - OFFSET) & ~pagesize);
	if (tunifqlen == MAP_FAILED) {
		perror("mmap(tunIFqlen)");
		exit(0);
	}
	tunIFqlen = (u_int32_t *)(PTR_POS(tunifqlen) + PTR_POS(offset));
mmap /dev/kmem end */

//nctuns_dev mmap start
	tunIFqlen = (u_int32_t *)(PTR_POS(dev->tunif_qlen));
//nctuns_dev mmap end	

	/*
	 * 
	 * map special tunnnel output queue length
	 * 
	 */
/* mmap /dev/kmem start
	if (klist[2].address == 0){
		printf("nctuns: search %s in kernel image error!!\n", klist[2].symbol);
		exit(-1);
	}
	offset = PTR_POS(klist[2].address - OFFSET) - (PTR_POS(klist[2].address - OFFSET) & ~pagesize);
	
	t0e = (u_int32_t *)mmap(0, PTR_POS(offset) + sizeof(u_int32_t), 
			PROT_READ, MAP_PRIVATE, fd, PTR_POS(klist[2].address - OFFSET) & ~pagesize);
	if (t0e == MAP_FAILED) {
		perror("mmap(ce_tun_qlen)");
		exit(0);
	}
	t0eqlen = (u_int32_t *)(PTR_POS(t0e) + PTR_POS(offset));
mmap /dev/kmem end */

//nctuns_dev mmap start
	t0eqlen = (u_int32_t *)(PTR_POS(&(dev->ce_tun_qlen)));
//nctuns_dev mmap end

	/* 
	 * 
	 * map fifo (any packet scheduling and buffer management module) 
	 * current output queue length 
	 * 
	 */
/* mmap /dev/kmem start
	if (klist[3].address == 0) {
		printf("nctuns: search %s in kernel image error!!\n", klist[3].symbol);
		exit(-1);
	}
	offset = PTR_POS(klist[3].address - OFFSET) - (PTR_POS(klist[3].address - OFFSET) & ~pagesize);
 	
	fifoifmaxqlen = (u_int32_t *)mmap(0, PTR_POS(offset) + sizeof(u_int32_t) * MAX_NUM_TUN, 
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, PTR_POS(klist[3].address - OFFSET) & ~pagesize);
	if (fifoifmaxqlen == MAP_FAILED) {
		perror("mmap(fifoIFmaxqlen)");
		exit(0);
	}
	fifoIFmaxqlen = (u_int32_t *)(PTR_POS(fifoifmaxqlen) + PTR_POS(offset));
mmap /dev/kmem end */

//nctuns_dev mmap start
	fifoIFmaxqlen = (u_int32_t *)(PTR_POS(dev->fifoIFmaxqlen));
//nctuns_dev mmap end

	/*
	 * 
	 * map fifo (any packet scheduling and buffer management module) 
	 * current output queue length
	 * 
	 */
/* mmap /dev/kmem start
	if (klist[4].address == 0) {
		printf("nctuns: search %s in kernel image error!!\n", klist[4].symbol);
		exit(-1);
	}
	offset = PTR_POS(klist[4].address - OFFSET) - (PTR_POS(klist[4].address - OFFSET) & ~pagesize);
	
	fifoifcurqlen = (u_int32_t *)mmap(0, PTR_POS(offset) + sizeof(u_int32_t) * MAX_NUM_TUN, 
			PROT_READ|PROT_WRITE, MAP_SHARED, fd, PTR_POS(klist[4].address - OFFSET) & ~pagesize);
	if (fifoifcurqlen == MAP_FAILED) {
		perror("mmap(fifoIFcurqlen)");
		exit(0);
	}
	fifoIFcurqlen = (u_int32_t *)(PTR_POS(fifoifcurqlen) + PTR_POS(offset));
mmap /dev/kmem end */

//nctuns_dev mmap start
	fifoIFcurqlen = (u_int32_t *)(PTR_POS(dev->fifoIFcurqlen));
//nctuns_dev mmap end
	
	// set its initial value to a very large value. When a fifo module is initialized, 
	// it will use its real maximum queue length and map it into the kernel. 
	// As explained above, doing this will make the fifo timely packet
	// dropping mechanism in the kernel not affect some sophisciated PSBM modules.
	for (i = 0; i < MAX_NUM_TUN; i++) {
		fifoIFcurqlen[i] = 0;
		fifoIFmaxqlen[i] = 1000000;
	}




	return(1);
}


u_long reg_poller(NslObject *obj, int (NslObject::*m)(Event_ *), int *fd) {

	char            path[50];
	int		i;

	/* check for duplicate registering, and filter out */
	for(i = 1; i <= (int)ifcnt_; i++) {
		if (iftun_[i].obj == obj && iftun_[i].meth_ == m)
			/* already registered, reject it */
			return(0);
	}
	
	ifcnt_++;

        iftun_[ifcnt_].obj = obj;
        iftun_[ifcnt_].meth_ = m;

	/* open tunnel device */
	sprintf(path, "tun%d", ifcnt_);
	*fd = tun_alloc(path);

        return(ifcnt_);
}


int tun_alloc(const char *dev)
{
	struct ifreq ifr;
	int fd;
	
	fd = open(NCTUNS_TUN_DEVICE, O_RDWR);
	if(fd < 0){
		printf("tun_alloc: open nctuns_tun for %s fail!!\n", dev);
		perror("==> perror ");
		exit(1);
	}

	memset(&ifr, 0, sizeof(ifr));

	ifr.ifr_flags = IFF_TUN;

	if (dev)
		memcpy(ifr.ifr_name, (void *)dev, IFNAMSIZ);

	/*
	 * allocate nctuns_tun device for simulation interface
	 */
	if (ioctl(fd, TUNSETIFF, (void*)&ifr) < 0) {
		close(fd);
		perror("[tun_alloc] ioctl ");
		exit(1);
	}

	return fd;
}
