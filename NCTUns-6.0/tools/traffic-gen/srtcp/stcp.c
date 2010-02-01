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

/*
 * Send tcp just for bandwidth testing, much like ttcp.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include <time.h>

extern int errno;

int writen( int fd, char *vptr, int n)
{
	int	nleft,nwritten;
        char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (errno == EINTR)
				nwritten = 0;
			else
				return(-1);
		}
		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}


int Writen(int fd, char *ptr, int nbytes)
{
	int n;
	if ( (n=writen(fd, ptr, nbytes)) != nbytes){
		perror("stcp (writen error)");
		printf("errno= %d\n", errno);
		exit(1);
	}
	return n;
}

int
main(argc, argv)
char *argv[];
{
  int s, bufsize, i;
  int writesize, pagesize, total;
  struct sockaddr_in sin, servaddr;
  struct hostent *hp;
  char *p = NULL;
  char *ip_str = NULL;
  time_t t0, t1;
  int touchflag = 0;
  char *hostname = 0;
  int port = 5117, tries = 0;

  pagesize = getpagesize();
  writesize = 8192;
  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-t") == 0){
      touchflag = 1;
    } else if(argv[i][0] != '-' && hostname == 0){
      hostname = argv[i];
    } else if(strcmp(argv[i], "-p") == 0 && i < argc-1){
      i++;
      port = atoi(argv[i]);
    } else if(strcmp(argv[i], "-l") == 0 && i < argc-1){
      i++;
      writesize = atoi(argv[i]);
    } else if(strcmp(argv[i], "-lip") == 0 && i < argc-1){
      i++;
      ip_str = (argv[i]);
    } else {
    usage:
      fprintf(stderr, "Usage: stcp [-t] [-p port] [-l writesize] [-lip local_ip_address] host\n");
      exit(1);
    }
  }

  if(hostname == 0)
    goto usage;

  memset(&sin, 0, sizeof(sin));
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  if(isdigit(hostname[0])){
    sin.sin_addr.s_addr = inet_addr(hostname);
  } else {
    hp = gethostbyname(hostname);
    if(hp == 0){
      fprintf(stderr, "stcp: no such host %s\n", hostname);
      exit(1);
    }
    bcopy(hp->h_addr_list[0], &sin.sin_addr, 4);
    hostname = hp->h_name;
  }

 again:
  s = socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0){
    perror("socket");
    exit(1);
  }

  /* Added by C.C. Lin for binding a local IP address.  
   * This binding is required when an application program is running on a 
   * multi-homed network node, i.e., multi-interface machine.
   */
  
  servaddr.sin_family = AF_INET;  
  
  
  if ( ip_str ) {
      int res;

      servaddr.sin_addr.s_addr = inet_addr(ip_str);
      if ( (res = bind(s, (struct sockaddr *) &servaddr , sizeof(servaddr))) != 0 ) {
          printf("stcp: binding local IP address is error. return_value = %d \n", res);
          perror("stcp:");
          exit(1);
      }
      printf("stcp: binding local IP address = %s \n", ip_str );
  }
  
  bufsize = 1024*1024;
  while(bufsize > 0){
    if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) == 0) {
      printf("Final buf size is %d\n",bufsize);
      break;
    }
    bufsize -= 1024;
  }
  if(bufsize <= 0){
    perror("setsockopt");
    exit(1);
  }
  printf("tcp buffer %d bytes, writes of %d bytes\n", bufsize, writesize);
  printf("destination %s %s, port %d\n",
	 hostname,
	 inet_ntoa(sin.sin_addr), port);

  if(connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    tries++;
    perror("connect");
    if(tries < 4){
      close(s);
      sleep(5);
      goto again;
    }
    exit(1);
  }

  /* work around solaris bug */
  //if(setsockopt(s, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize)) < 0){
  //  perror("2nd setsockopt SO_SNDBUF");
  //  exit(1);
  //}

  p = valloc(writesize);
  if(p == 0){
    perror("malloc");
    exit(1);
  }

  time(&t0);
  t1 = t0;
  total = 0;

  while(1){
    if(touchflag){
      for(i = 0; i < writesize; i += pagesize)
	p[i] = i;
    }
    //if(write(s, p, writesize) != writesize){
    if(Writen(s, p, writesize) != writesize){
      perror("send");
      break;
    }
    total += writesize;
  }

  time(&t1);
  if(t1 == t0)
    t1 = t0 + 1;
  printf("%d bytes, %ld seconds, %ld kbytes/second\n",
	 total, t1-t0, (total/1024)/(t1-t0));

  exit(0);
}
