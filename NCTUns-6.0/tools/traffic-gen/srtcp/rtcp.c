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
 * Receive tcp just for bandwidth testing, much like ttcp.
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
#include <sys/time.h>
#include <signal.h>
#include <errno.h>
#include <netinet/tcp.h>

struct timeval tv0; /* start time */
struct timeval now;
int ms;             /* milliseconds since start */
double total;       /* total bytes received */

//kcliao
pid_t pid=0;

int lastms;
double lasttotal;
int quiet; /* -q */
int dflag = 1;

/* add by kcliao */
int LOG=0;
FILE *file;
double bandmega=0;
char dir[512], *envdir;

void gotalarm();

int
main(argc, argv)
char *argv[];
{
  int s, bufsize, i, addrlen, s1, cc;
  int readsize, pagesize;
  struct sockaddr_in sin;
  char *p = NULL;
  char *ip_str = NULL;
  int touchflag = 0;
  int port = 5117;
  int seconds = -1; /* -T */
//kcliao
  pid = getpid();

#ifdef hpux
  pagesize = 8192;
#else
  pagesize = getpagesize();
#endif
  readsize = 8192;

  for(i = 1; i < argc; i++){
    if(strcmp(argv[i], "-t") == 0){
      touchflag = 1;
    } else if(strcmp(argv[i], "-q") == 0){
      quiet = 1;
    } else if(strcmp(argv[i], "-p") == 0 && i < argc-1){
      i++;
      port = atoi(argv[i]);
    } else if(strcmp(argv[i], "-l") == 0 && i < argc-1){
      i++;
      readsize = atoi(argv[i]);
    } else if(strcmp(argv[i], "-T") == 0 && i < argc-1){
      i++;
      seconds = atoi(argv[i]);
    } else if(strcmp(argv[i], "-w") == 0 && i < argc-1){
      LOG = 1;
      i++;
	envdir = getenv("NCTUNS_WORKDIR");
	if(envdir)
		sprintf(dir, "%s/%s", envdir, argv[i]);
	else
		sprintf(dir, "%s", argv[i]);
	file = fopen(dir, "w");
	printf("Log file: %s\n", dir);
    } else  if(strcmp(argv[i], "-d") == 0) {
      dflag = 1;
    } else if(strcmp(argv[i], "-lip") == 0 && i < argc-1){
      i++;
      ip_str = (argv[i]);
    } else {
      fprintf(stderr, "Usage: rtcp [-t] [-q] [-d] [-p port] \n\
            [-l readsize] [-w logfile] [-T seconds] [-lip local_ip_address]\n");
      exit(1);
    }
  }

  s = socket(AF_INET, SOCK_STREAM, 0);
  if(s < 0){
    perror("rtcp: socket");
    exit(1);
  }

  bufsize = 1024*1024 ;

  while(bufsize > 0){
    if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == 0) {
      if (dflag == 0)  printf("final buf size is %d\n",bufsize);
      break;
    }
    bufsize -= 1024;
  }

  if(bufsize <= 0){
    perror("rtcp: setsockopt");
    exit(1);
  }

//kcliao
  //if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0)
  //  perror("SO_REUSEADDR");

  if(quiet == 0){
    if (dflag == 0)
    printf("#tcp buffer %d bytes, reads of %d bytes, port %d",
	   bufsize, readsize, port);
    if(seconds != -1)
      if (dflag == 0) printf(", for %d seconds", seconds);
    if (dflag == 0) printf("\n");
  }
  memset(&sin, 0, sizeof(sin));
  if ( ip_str ) {
      sin.sin_addr.s_addr = inet_addr(ip_str);
      printf("rtcp: binding local IP address = %s \n", ip_str );
  }
  sin.sin_family = AF_INET;
  sin.sin_port = htons(port);

  if(bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0){
    perror("rtcp: bind");
    exit(1);
  }
  listen(s, 1);

  bzero(&sin, sizeof(sin));
  addrlen = sizeof(sin);
  s1 = accept(s, (struct sockaddr *)&sin, (socklen_t *)&addrlen);
  if(s1 < 0){
    perror("rtcp: accept");
    exit(1);
  }
  close(s);
  s = s1;
  if(quiet == 0)
    if (dflag == 0) printf("#connected to %s", inet_ntoa(sin.sin_addr));

  /* Solaris has a bug that requires this here: */
  //if(setsockopt(s, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) < 0){
  //  perror("rtcp: 2nd setsockopt SO_RCVBUF");
  //  exit(1);
  //}

  //{ int rcvbuf = 0, xlen = 4;
  //  getsockopt(s1, SOL_SOCKET, SO_RCVBUF, &rcvbuf, &xlen);
  //  if (dflag == 0) printf(", rcvbuf %d", rcvbuf);
  //}

#ifdef TCP_MAXSEG
  { int maxseg, xlen = 4;
    if(getsockopt(s1, IPPROTO_TCP, TCP_MAXSEG, &maxseg, (socklen_t *)&xlen) != -1)
      if (dflag == 0) printf(", maxseg %d", maxseg);
  }
#endif

  if (dflag == 0) printf("\n");

#ifdef hpux
  p = malloc(readsize);
#else
  p = valloc(readsize);
#endif
  if(p == 0){
    perror("rtcp: malloc");
    exit(1);
  }

  gettimeofday(&tv0, (struct timezone *) 0);
  lasttotal = total = 0;
  signal(SIGALRM, gotalarm);
  alarm(1);

  while(seconds == -1 || ms < (seconds * 1000)){
    if((cc = read(s, p, readsize)) < 0){
      if(errno == EINTR)
	continue;
      perror("rtcp: recv");
      break;
    }
    if(cc == 0)
      break;
    if(touchflag){
      for(i = 0; i < cc; i += pagesize)
	p[i] = p[i+1];
    }
    total += cc;
  }

  gettimeofday(&now, (struct timezone *) 0);
  ms = (now.tv_sec - tv0.tv_sec)*1000 + (now.tv_usec - tv0.tv_usec)/1000;
  if(quiet == 0)
    if (dflag == 0) printf("\n");
  if (dflag == 0) printf("%5d\n", (int)(total/ms));

  exit(0);
}

 void gotalarm(sig)
{
  gettimeofday(&now, (struct timezone *) 0);
  ms = (now.tv_sec - tv0.tv_sec)*1000 + (now.tv_usec - tv0.tv_usec)/1000;

  /* modified by kcliao */
  bandmega = ((double)(total-lasttotal)/(double)(ms-lastms));
  if(quiet == 0){
    //printf("%5d %5d\n", ((int) (ms/1000) + tv0.tv_sec), (int)((total-lasttotal)/(ms-lastms)));
    printf("%d     %lu    %10d Kbyte/sec ==>  %8.6lf Mbit/sec\n",
		    pid,
		    now.tv_sec,
		    (int)bandmega,
		    (bandmega*8)/(1024)
	  );
    fflush(stdout);
  }
  if(LOG){
    //fprintf(file,
//	"%5d %5d\n", ((int) (ms/1000) + tv0.tv_sec), (int)((total-lasttotal)/(ms-lastms)));
    fprintf(file, "%d      %lu    %10d Kbyte/sec  ==>  %8.6lf Mbit/sec\n",
		    pid,
		    now.tv_sec,
		    (int)bandmega,
		    (bandmega*8)/(1024)
	   );
    fflush(file);
  }

  lastms = ms;
  lasttotal = total;

  signal(SIGALRM, gotalarm);
  alarm(1);
}
