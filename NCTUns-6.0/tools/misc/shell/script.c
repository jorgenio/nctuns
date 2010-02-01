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
 * Copyright (c) 1980, 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

//add by NCTUNS
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/syscall.h> 
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pty.h>
#include <utmp.h>

#include <err.h>
#include <errno.h>
#include <fcntl.h>
#include <paths.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <nctuns_syscall.h>

int   master, slave;
int   child;
struct  termios tt;
char obuf[BUFSIZ];
char ibuf[BUFSIZ];

//add by NCTUNS
pid_t parents;
int  mode = 0;          //0, normal input, 1.command entered.
char command[100];
int  command_length = 0;
char command_type;    //0,normal, 1.ifconfig, 2.tcpdump
char *nodeID;
unsigned int nid;
char fxpnum[6];
char tunnum[6];
int  fxpnumsize;
int  tunnumsize;
int  portnumber;
int  lock = 0;
int *tuns; 

int      sockfd;
struct   sockaddr_in saddr;

void	done __P((int));
void	doshell __P((void));
void	fail __P((void));
void	finish __P((void));

//add by NCTUNS

void
createsocket()        
{
   sockfd = socket(AF_INET, SOCK_STREAM, 0);
   bzero(&saddr, sizeof(saddr));
   saddr.sin_family = AF_INET;
   saddr.sin_port = htons(9880);
   (void)inet_pton(AF_INET,"127.0.0.1",&saddr.sin_addr);
   (void)connect(sockfd, (struct sockaddr *) &saddr, sizeof(saddr));
}


ssize_t
writen(int fd, const char *vptr, size_t n)
{
   size_t nleft;
   ssize_t nwritten;
   const char *ptr;
   
   ptr = vptr;
   nleft = n;
   while(nleft > 0){
      if((nwritten = write(fd, ptr, nleft)) <= 0){
            nwritten = 0;
      }
      nleft -= nwritten;
      ptr += nwritten;
   }
   return n;
}

//get input string from keyboard and parse the command
void getcommand(char* inputbuf, int count)
{
   char tcpdump[100];

   switch(inputbuf[0]){
	  case 8:
	 if(command_length > 0)
	    command_length--;
	 else
		command_length = 0;
	 break;
      case 127:
	 if(command_length > 0)
           command_length--;
	 else
           command_length = 0;
	 break;

      //still buggy, release the function later
      case 27:
	 lock = 2;
	 break;
      case 9:
	 lock = 2;
	 break;
      case 13:
	 command[command_length] = 13;
	 command_length++;
         command[command_length] = '\0';
         mode = 1;
         if(strstr(command, "ifconfig") != NULL)
            command_type = 1;
         else if(strstr(command, "tcpdump") || strstr(command, "ettercap"))
            command_type = 2;
	 else
	    command_type = 0;
	 //command_point = -1;
         break;
      case 3:
         if(command_type == 2){
            createsocket();			       				
            sprintf(tcpdump, "Tcpdump %s %s TCPDUMP DumpFlag off\n",nodeID, fxpnum); 
            (void)writen(sockfd,tcpdump,sizeof(tcpdump));
            (void)close(sockfd);
         }
         //command_point = -1;
         command_length = 0;
	 command_type = 0;
         break;
      default:
	 if(count == 1){
	    //if (iscntrl(inputbuf[0]) == 0){
	       command[command_length] = inputbuf[0];
	       command_length++;
	    //}
	 }
	 else{
            lock = 2;
	 }
         break;
   }
}

//convert integer to string
void inttostr(tp,integer,size)
        char *tp;
        int integer;
        int *size;
{	
   if(integer > 9999){
      tp[0] = integer / 10000 + 48;
      tp[1] = (integer % 10000) / 1000 + 48;
      tp[2] = (integer % 10000 % 1000) / 100 + 48;
      tp[3] = (integer % 10000 % 1000 % 100) /10 + 48;
      tp[4] = integer % 10000 % 1000 % 100 % 10 + 48;
      tp[5] = '\0';
      *size = 5;
   }
   else if(integer > 999){
       	tp[0] = integer / 1000 + 48;
	tp[1] = (integer % 1000) / 100 + 48;
        tp[2] = (integer % 1000 % 100) / 10 + 48;
	tp[3] = (integer % 1000 % 100 % 10) /1 + 48;
	tp[4] = '\0';
	*size = 4;
   }
   else if(integer > 99){
       	tp[0] = integer / 100 + 48;
	tp[1] = (integer % 100) / 10 + 48;
        tp[2] = (integer % 100 % 10) / 1 + 48;
        tp[3] = '\0';
        *size = 3;
   }
   else if (integer > 9){
       	tp[0] = integer / 10 + 48;
	tp[1] = (integer % 10) / 1 + 48;
	tp[2] = '\0';
	*size = 2;
   }
   else{
      tp[0] = integer + 48;
      tp[1] = '\0';
      *size = 1;
   }	
}

//conver fxp number to tunnel number, return 1 if success,else return 0(not found or no value)
int convert_fxp_tun(strings)
   char* strings;
{
   char *loc;
   int  i,t;
   
   
   if ((loc = strstr(strings,"eth")) == NULL)			        
      return 0;			                
   else{			                      			            
      loc[0] = 't';
      loc[1] = 'u';
      loc[2] = 'n';			                       
      i = 0;			                     			         			                
      while((loc[i+3] != ' ') && loc[i+3] != 13){			                       	
         fxpnum[i] = loc[i+3];
         i++;
      }	
      fxpnumsize = i;			                       				                       
      fxpnum[i] = '\0';		                       			                 
      t = atoi(fxpnum);	
      if((t == 0) || (t > portnumber) ){
    	 loc[0] = 'e';
	 loc[1] = 't';
	 loc[2] = 'h';
	 return 0;
      }     		                     
      else{	                          
         inttostr(tunnum,tuns[t-1],&tunnumsize);				                       	                   	                      
	 t = command_length;
	 while((command[t] != 'n') && (command[t-1] != 'u')){
	    command[t+(tunnumsize-fxpnumsize)] = command[t];
	    t--;
         }
         for(t=0;t<tunnumsize;t++)
            loc[t+3] = tunnum[t];
         command_length += (tunnumsize-fxpnumsize);
      }
      return 1;
   }
}

//conver tunnel number to fxp number, return 1 if success,else return 0(not find or no value)
int convert_tun_fxp(strings)
    char *strings;
{
   char* loc;
   int   i,t;
   int mark = 0;
   
   if ((loc = strstr(strings,"tun")) != NULL){			                      			            
      loc[0] = 'e';
      loc[1] = 't';
      loc[2] = 'h';
			                       
      i = 0;			                     			         			                
      while((loc[i+3] >= 48) && (loc[i+3] <= 57)){			                       	
         tunnum[i] = loc[i+3];
         i++;
      }
      tunnumsize = i;			                       				                       
      tunnum[i] = '\0';		                       			                 
      t = atoi(tunnum);	
      for(i=0; i<portnumber; i++)
         if(tuns[i] == t){
            inttostr(fxpnum,i+1,&fxpnumsize);
            mark = 1;
         }	
         if(mark == 0)
           return 0;
         else{	                 		                   		                             	        				                       	                   	                      
            for(t=0; t<tunnumsize; t++){
               if(t < fxpnumsize)
                  loc[t+3] = fxpnum[t];
               else
                  loc[t+3] = ' ';
            }
            return 1;
         }
	 
   }
   else
      return 0;			   
}

int
main(argc, argv)
	int argc;
	char *argv[];
{
   register int cc;
   struct termios rtt;
   struct winsize win;
   int n;
   fd_set rfd;

   //add by nctuns
   char tcpdump[100];
   int isprint = 0;	
   char backspace = 127;
   char endline = '\n';
   char ret     = '\r';
   int imode = 0;

   //parents = getppid();

   //systemcall to get the fxp to tunnel table
   nodeID = getenv("NCTUNS_NODEID");
   nid = (unsigned)atoi(nodeID);                                                                     
   syscall_NCTUNS_misc(syscall_NSC_misc_NIDNUM, nid, 0, (unsigned long)&portnumber);
   tuns = (int *)malloc(sizeof(int) * portnumber); 
   syscall_NCTUNS_misc(syscall_NSC_misc_NIDTOTID, nid, portnumber, (unsigned long)tuns);
   (void)truncate(argv[1], 0);

	
   (void)tcgetattr(STDIN_FILENO, &tt);
   (void)ioctl(STDIN_FILENO, TIOCGWINSZ, &win);
   if (openpty(&master, &slave, NULL, &tt, &win) == -1)
      err(1, "openpty");

   rtt = tt;
   cfmakeraw(&rtt);
   rtt.c_lflag &= ~ECHO;
   (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &rtt);

   fprintf(stderr , "\e====================================[1;33mnctunstcsh failed\e[m\n");
   child = fork();
   if (child < 0) {
      warn("fork");
      done(1);
   }
   if (child == 0)
      doshell();
   FD_ZERO(&rfd);

   //the main body of the intercepter process
   for (;;) {
      FD_SET(master, &rfd);
      FD_SET(0, &rfd);
      n = select(master + 1, &rfd, 0, 0, 0);
      if (n < 0 && errno != EINTR)
         break;

      //there's something at input, get it
      if (n > 0 && FD_ISSET(0, &rfd)) {
         cc = read(0, ibuf, BUFSIZ);
         if (cc <= 0)
            break;
	 if (cc > 0) {
            
            getcommand(ibuf, cc);
            switch(mode){
	       int i;
               case 1: //end of input
		     (void)convert_fxp_tun(command);
                     for(i=0; i<command_length; i++)
                        (void)write(master, &backspace, 1); 
                     (void)write(master, command, command_length);
		    
                     if (command_type == 2){	
        	        createsocket();			       	
		        sprintf(tcpdump, "Tcpdump %s %s TCPDUMP DumpFlag on\n",nodeID, fxpnum); 
		        (void)writen(sockfd,tcpdump,sizeof(tcpdump));
		        (void)close(sockfd);
		     }
                  break;
               case 0: //normal input
		  if(lock != 2){
                     (void)write(master, ibuf, cc);
		  }
		  else{
	             lock = 0;
		  }  
		  break; 
            }
         }
      }
      if (n > 0 && FD_ISSET(master, &rfd)) {
         cc = read(master, obuf, BUFSIZ);
         if (cc <= 0)
            break;
         
         switch(mode){
           case 1: //end of input, 
	      (void)write(1, &ret, 1);
	      (void)write(1, &endline, 1);
	      command_length = 0;
	      mode = 0;
	      cc = 0;
	      break;
	   case 0: //normal output by program
	      if(command_type == 2){
		 if(obuf[0] == '%'){
		    createsocket();
		    sprintf(tcpdump, "Tcpdump %s %s TCPDUMP DumpFlag off\n",nodeID, fxpnum);
		    (void)writen(sockfd,tcpdump,sizeof(tcpdump));
		    (void)close(sockfd);
		    (void)write(1, obuf, cc);
		 }
		 else{
	            (void)convert_tun_fxp(obuf);
	            (void)write(1, obuf, cc);
		 }
	      }

	      else if(command_type == 1)//ifconfig
	      {
	         int k = 0;
		 char buffer[200];
		 while(k <= cc){
		    int c = 0;
		    if((obuf[0] == '%') && (cc == 1)){
			(void)write(1, obuf, cc);
			command_type = 0;
			imode = 0;
			isprint = 0;
			break;
		    }
		    if(imode == 1)
			break;
		    while((obuf[k+c] != '\n') && ((k+c)<=cc)){
                       buffer[c] = obuf[k+c];
		       c++;
		    }
		    buffer[c] = '\n';
		    //printf("%d %d %d\n", buffer[c], buffer[c-1], buffer[c-2]);
		    c++;
                    k = k + c;
		    if(!isprint){
		    	if( strstr(buffer, "tun") && convert_tun_fxp(buffer)){
			   isprint = 1;
			   (void)write(1, buffer, c);
			}
			else
			   isprint = 0;
                        continue;
		    }
		    else{
		       if(strstr(buffer, "tun") && !convert_tun_fxp(buffer)){
		          isprint = 0;
			  imode = 1;
		       }
		       else{
			  if(c > 4)
		          	(void)write(1, buffer, c); 
		       }
		    }
		 }
	      }

	      else{
	            (void)write(1, obuf, cc);
	      }
         }
      }
   }
   finish();
   done(0);
   return(0);
}


void
finish()
{
	int die, e, pid;
	union wait status;

	die = e = 0;
	while ((pid = wait3((int *)&status, WNOHANG, 0)) > 0)
	        if (pid == child) {
			die = 1;
			if (WIFEXITED(status))
				e = WEXITSTATUS(status);
			else if (WIFSIGNALED(status))
				e = WTERMSIG(status);
			else /* can't happen */
				e = 1;
		}

	if (die)
		done(e);
}

void
doshell()
{
   char shell[200];

   sprintf(shell, "%s/tools/nctunstcsh", getenv("NCTUNSHOME"));
   (void)close(master);
   login_tty(slave);
   execl(shell, shell, "N", nodeID, NULL);
   //execl(shell, shell, NULL);
   fail(); 

}

void
fail()
{
	fprintf(stderr , "\e[1;33mnctunstcsh failed\e[m\n");
	(void)kill(0, SIGTERM);
	done(1);
}

void
done(eno)
	int eno;
{

	(void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &tt);
	(void)close(sockfd);
	(void)close(master);
}
