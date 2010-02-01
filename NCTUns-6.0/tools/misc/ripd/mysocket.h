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

#include "unp.h"


    int Socket( int family , int type , int protocol )
    {
    	int fd;
        if ( (fd=socket( family, type , protocol )) < 0 )
        {
             perror("ERROR in requesting a socket descriptor:");	
             
             #ifndef RETRY
             exit(1);
             #endif
        }
        
        return fd;
    }
    
    
    
    int Bind ( int sockfd , const struct sockaddr *myaddr , socklen_t addrlen )
    {
        int res;
        if ( (res=bind( sockfd , myaddr , addrlen)) < 0 )
        {
            perror("Binding socket ERROR:");	
             
            #ifndef RETRY
            exit(1);
            #endif
        }
        
        return res;
    }
    
    
    int Connect ( int sockfd , const struct sockaddr *servaddr , socklen_t addrlen )
    {
        int res;
        if ( (res=connect(sockfd,servaddr,addrlen)) < 0 )	
        {
            perror("connecting ERROR:");	
            
            #ifndef RETRY
            exit(1);
            #endif 
        }
        
        return res;
    }
    
    int Listen ( int sockfd , int backlog )
    {
        char *ptr;
        int   res;
        
        /* read backlog in Env. variable (if available)to override the default value */
        
        if ( (ptr = getenv("LISTENQ")) != NULL )
        {
            backlog = atoi(ptr);
        }
        
        if ( (res=listen(sockfd,backlog)) < 0 )
        {
            perror("Listening ERROR:");	
            
            #ifndef RETRY
            exit(1);
            #endif
        }
        return res;
    }
    
    int Accept( int sockfd , struct sockaddr *cliaddr , socklen_t *addrlen)
    {
    	int res;
        if ( (res=accept( sockfd , cliaddr , addrlen)) < 0 )	
        {
            perror("Accept a connected socket error:");
            
            #ifndef RETRY
            exit(1);
            #endif 
        }
        
        return res;
    }
    
    
    pid_t Fork(void)
    {
        pid_t res;
        if ( (res=fork()) < 0 )	
        {
            perror("Can't Fork Child Process:");
            exit(1);
        }
    
        return res;
    }
    
        
    int Setsockopt( int sockfd , int level , int optname , const void* optval , socklen_t optlen )
    {
        int res;
        if ( (res=setsockopt( sockfd , level , optname , optval , optlen )) < 0 )
        {
            perror("Setsockopt failed:");
            exit(1);
        }
        
        return res;
    }
    
    int Getsockopt( int sockfd , int level , int optname , void* optval , socklen_t *optlen )
    {
        int res;
        if ( (res=getsockopt( sockfd , level , optname , optval , optlen )) <0 )
        {
            perror("Getsockopt failed:");
            exit(1);
        }
        
        return res;
    }
    
    
    
    
    
    
    
    
    
    
    
    /**** append a terminated indicator to a duplicated character string without zero-terminated. ****/
    
    char* append_zero( char* str , int len )
    {
        char *tmp = new char(len+1);
        tmp[len]  = '\0';
        
        return tmp;
    }
    
    int free_dupstr( char* str)
    {
         delete str;
    }
    
    void show_str( char* str , int len )
    {
        char *tmp=str;
        
        for ( int n=1 ; n <= len  ; n++ , tmp++ )
        {
             printf(" %d",*tmp);
        }
        
        printf("\n");
    }
    
