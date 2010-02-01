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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
//#include <netinet/in.h>
//#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <math.h>
#include <time.h> 
//#include <netinet/tcp.h>


int  tcp=1, port=3000, tcpread=8192;   /* default use TCP & port=3000 */
int  REPORT=0;
int  totalsize=0, lasttotal=0;
struct timeval now, tv0;
FILE *outpt1, *outpt2;
double ms=0, lastms=0, bandmega=0; 
char dir[1024];
char *envdir;

int readn( int, char *, int);
int Readn( int, char *, int);
void gotalarm();

char Usage[] = "\n\
Usage: rtg -type  [-options]\n\
\n\
[-type]     -t                   TCP connection(readsize default 500)\n\
            -u                   UDP connection\n\
\n\
[-options]  -v                   view per-pkt result on screen\n\
            -p ##                port number to listen at (default 3000)\n\
            -o LogFilename       record per-pkt result (only support UDP)\n\
	    -w LogFilename       report per-second throughput and record to file\n\
";


int main(argc, argv)
int   argc;
char  *argv[];
{
	int                  sockfd, newsockfd, clilen, readsize, i, j=0, pid, len, bufsize,
	                     chkid=1, lostno=0, transize, sendno=0, revno=0, ofno=0, 
	                     LOG=0, SHOW=0, BUG=0;
	struct sockaddr_in   ser_addr, cli_addr;
	double               send_time, arrival_time, tran_time, avgtrantime=0;
	char                 buf[66000], *sep="+", *brkt, *str, tok[4][50], *temp;
        
        if( argc == 1) goto usage;
        for( j=1 ; j<argc ; j++ ){
        	
                if( strcmp( argv[j], "-u" ) == 0 ){
        		tcp = 0;
        	}else if( strcmp( argv[j], "-t" ) == 0 ){
        	        tcp = 1;	
		}else if( strcmp( argv[j], "-p" ) == 0 && j < argc-1){
        		j++;
        		port = atoi( argv[j] );
        	}else if( strcmp( argv[j], "-o" ) == 0 && j < argc-1){
        	        j++;    
		        envdir = getenv("NCTUNS_WORKDIR");
			if(envdir)
				sprintf(dir,"%s/%s", envdir, argv[j]);
			else{
				sprintf(dir,"%s", argv[j]);
			}	
			printf("output file: %s\n",dir);
		        LOG = 1;
        	        outpt1 = fopen( dir, "w");        	        
			if(outpt1 == NULL ){
				printf("open file fail: %s  %p\n", dir, outpt1);
				exit(1);
			}
        	}else if( strcmp( argv[j], "-w" ) == 0 && j < argc-1){
			REPORT = 1;
			j++;
			envdir = getenv("NCTUNS_WORKDIR");
			if(envdir)
				sprintf(dir,"%s/%s", envdir, argv[j]);
			else
				sprintf(dir,"%s", argv[j]);
			printf("output file: %s\n", dir);	
			
			outpt2 = fopen( dir, "w");
			if(outpt2 == NULL ){
				printf("open file fail: %s  %p\n", dir, outpt1);
				exit(1);
			}
		}else if( strcmp( argv[j], "-v" ) == 0 ){
                        SHOW = 1;
        	}else if( strcmp( argv[j], "-d" ) == 0 ){
        	        BUG = 1;
        	}else{
usage:        	        printf("\n%s\n", Usage);
        	        exit(1);
        	}
        }
        if(LOG && tcp)  goto usage;
        
	
	if( ( sockfd = socket( AF_INET, tcp?SOCK_STREAM:SOCK_DGRAM, 0 )) < 0 ){
	       perror( "server: can't open socket");
	       exit(1);
	}
       
       bufsize = 1024*1024*1024;
       while( bufsize > 0 ){
               if( setsockopt( sockfd, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize)) == 0 ){
                       if( !SHOW && BUG )
			       printf("Final bufsize is %d\n", bufsize);
                       break;
                }
                bufsize -= 1024;
       }
       if (bufsize<=0){
	       printf("setsockopt error: rcvbuf\n");
	       exit(1);
       }
       printf("final rcvbuf size= %d\n", bufsize);

	
	       
	bzero(( char *) &ser_addr, sizeof( ser_addr ));
	ser_addr.sin_family      = AF_INET;
	ser_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	ser_addr.sin_port        = htons(port);
	
	if( bind( sockfd,(struct sockaddr *)&ser_addr, sizeof(ser_addr)) < 0 ){
	       perror( "server: can't bind local address");
	       exit(1);
	}
	

        if( REPORT ){
		int c=0;
		
		gettimeofday(&tv0, 0);
                signal( SIGALRM, gotalarm);         
                //c = ualarm(1000000, 0);
                c = alarm(1);
		printf("[rtg] alarm return %d ..\n", c);
        }

	
	if(tcp){
	       if( SHOW || BUG ) printf("\ntype: tcp(%d)      port: %d\n", tcp, port);

	       listen( sockfd, 1);
	
	       	       bzero(&cli_addr, sizeof(cli_addr));
		       clilen = sizeof(cli_addr);
		       newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, (socklen_t *)&clilen);
		       if( newsockfd < 0 ){
		               perror("server: accept error");
                               exit(1);
                       }                                                   
		       close(sockfd);
	               
		       temp = (char *)valloc( tcpread );
	               while( 1 ){
	                           readsize = Readn( newsockfd, temp, tcpread);
	                       	   
				   if( readsize == 0 )
					   goto bye;
				   totalsize += readsize; 
				   /*i = 0;
			           for( str=strtok_r(temp, sep, &brkt);
                                        str;
                                        str=strtok_r(NULL, sep, &brkt))
                                   {
             	                             strcpy( tok[i], str);
             	                             i++;
                                   }
                                   
                                   pid = (int)atoi( tok[0] );	                           	                           
	                           if( pid == -1 )
	                                   goto bye;
	                           
	                           send_time = (double)atof( tok[1] );	                       
                                   gettimeofday( &now, 0);                                   
                                   arrival_time = (double)now.tv_sec+(double)now.tv_usec/1000000.0;
                                   tran_time = arrival_time - send_time;

                                   if( BUG ) printf("%f     %f     ", 
                                                     send_time, arrival_time);
                                   if( SHOW || BUG ) printf("%6d    %7d     %4.6f\n",
                                                    pid, readsize, tran_time );*/
		       }
	}else{
	       if( SHOW || BUG ) 
		       printf("\ntype: udp(%d)     port: %d \n", tcp, port);
	                
		len = sizeof( cli_addr );
	        while(1){
			readsize = recvfrom( sockfd, buf, 66000, 0, (struct sockaddr *)&cli_addr, (socklen_t *)&len );
				   
				   i = 0;
			           for( str=strtok_r(buf, sep, &brkt);
                                        str;
                                        str=strtok_r(NULL, sep, &brkt))
                                   {
             	                             strcpy( tok[i], str);
             	                             i++;
                                   }
                                   pid = (int)atoi( tok[0] );
                                   send_time = (double)atof( tok[1] );
                                   
                                   if( pid == -1 ){
                                   	sendno   = atoi( tok[1] );
                                   	transize = atoi( tok[2] );                                   	
                                   	goto bye;
                                   }     
                                   
				   /*while( chkid != pid && ( SHOW || LOG ) ){
                                   	if( SHOW || BUG ) printf("packet  no.%d  lost\n", chkid);
                                   	if( LOG ) fprintf( outpt, "packet  no.%d  lost\n", chkid);
                                   	lostno++;
                                   	chkid++;
                                   }*/
                                   if(pid < chkid){
					   if(SHOW){
						  printf("   pkt no.%d out of order\n   ", pid); 
					   }
					   if(LOG){
						  fprintf(outpt1,"   pkt no.%d out of order\n   ", pid);
					   }
					   ofno++;
					   goto next;
				   }
								   
				   if( chkid != pid && ( SHOW || LOG ) ){
                                   	lostno += (pid - chkid);
                                   	if( SHOW || BUG )
						printf("packet  no.%d to no.%d  lost, accumulate %d pkts!!\n", chkid, pid-1, lostno);
                                   	if( LOG ) 
						fprintf( outpt1, "packet  no.%d to no.%d  lost, accumulate %d pkts lost!!\n", chkid, pid-1, lostno);
					chkid = pid;
                                   }
                                   chkid++;

next:
				   revno++;
                                   totalsize += readsize;                                                                      
                                   gettimeofday( &now, 0);      
                                                                
                                   arrival_time = (double)now.tv_sec + (double)now.tv_usec/1000000.0; 

                                   tran_time = arrival_time - send_time;
                                   avgtrantime += tran_time;


                                   if( BUG )printf("%f    %f    ", 
                                                           send_time, arrival_time);
                                   if( SHOW || BUG ) printf("pkt no.%d   size=%5d byte,  delay time=%9.6f sec\n",
                                                      pid, readsize, tran_time);
                                   
                                   if( LOG && !BUG ) fprintf( outpt1, "pkt no.%d   size=%5d byte,  delay time=%9.6f sec\n",
                                                      pid, readsize, tran_time);
                                   if( LOG && BUG )  fprintf( outpt1, "pkt no.%d   size=%5d byte,  %9.6f  %9.6f  delay time=%9.6f sec\n",
                                                      pid, readsize, send_time, arrival_time, tran_time);
								   fflush(outpt1);
                        }
	}
	

bye:
        if(!tcp){
               
               chkid--;
               if( sendno != chkid ){
                       while( sendno >= chkid ){
                               if( SHOW || BUG ) printf( "packet no.%d  lost\n", chkid);
                               if( LOG ) fprintf( outpt1, "packet no.%d  lost\n", chkid);
                               chkid++;
                       }
               }

           	if( SHOW || BUG ){
           	        printf("Total transmit packets: %d,\n", sendno);
                        printf("Total received packets: %d,     Lost packet number rate: %f %c\n", 
					revno,  ((double)(lostno-ofno)/(double)sendno)*100, '%' );                
                        printf("Total transmit bytes: %d,\n", transize);
                        printf("Total received bytes: %d,     Lost bytes rate: %f %c\n", 
					totalsize, ((double)(transize-totalsize)/((double)transize))*100,'%' );                
                        printf("Average delay time: %f sec\n", avgtrantime/(double)revno); 
                }
                
                if( LOG ){
                        fprintf( outpt1, "Total transmit packets: %d,\n", sendno);
                        fprintf( outpt1, "Total received packets: %d,     Lost packet number rate: %f %c \n", 
					revno,  ((double)(lostno-ofno)/(double)sendno)*100, '%' );                
                        fprintf( outpt1, "Total transmit bytes: %d,\n", transize);
                        fprintf( outpt1, "Total received bytes: %d,     Lost bytes rate: %f %c\n", 
					totalsize, ((double)(transize-totalsize)/((double)transize))*100,'%' );                
                        fprintf( outpt1, "Average delay time: %f sec\n", avgtrantime/(double)revno); 
                        fclose( outpt1);
                }
                close(sockfd);
        }else{
                close(newsockfd);       
        }                                
        printf("\n");
        exit(1);
}		

/*************************************************** Read "n" bytes from a descriptor. *********************/
int readn(int fd, char *vptr, int n)
{
	int	nleft,nread;
	char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;	
			else
				return(-1);
		} else if (nread == 0)
			break;		

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);	
}

int Readn(int fd, char *ptr, int nbytes)
{
	int		n;

	//if ( (n = readn(fd, ptr, nbytes)) < 0)
	if ( (n = read(fd, ptr, nbytes)) < 0)
		perror("server: readn error");
	return(n);
}

/******************************************** gotalarm ***********************/
void gotalarm( sig )
{

	if(totalsize==0){
		gettimeofday(&tv0, NULL);
		signal(SIGALRM, gotalarm);
		//ualarm(1000000, 0);
		alarm(1);
		return;
	}
	
	
	gettimeofday(&now, NULL);
	ms = (now.tv_sec - tv0.tv_sec)*1000 + (now.tv_usec - tv0.tv_usec)/1000;

	bandmega = ((double)(totalsize-lasttotal)/(double)(ms-lastms));
	printf("%ld      %8lf Kbyte/sec  ==>   %8.6lf Mbit/sec\n",
			now.tv_sec,
			//(int)bandmega,
			bandmega,
			(double)(bandmega*8)/(1024)
      	      );
        fflush(stdout);
	fprintf(outpt2, "%ld      %8lf Kbyte/sec  ==>   %8.6lf Mbit/sec\n",
			now.tv_sec,
			//(int)bandmega,
			bandmega,
			(double)(bandmega*8)/(1024)
      	      );
        fflush(outpt2);

	lastms = ms;
	lasttotal = totalsize;

        signal( SIGALRM, gotalarm);
        //ualarm(1000000, 0);
	alarm(1);
}
