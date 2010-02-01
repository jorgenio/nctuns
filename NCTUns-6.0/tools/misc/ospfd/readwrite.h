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
#ifndef READWRITE
#define READWRITE

    ssize_t  readn ( int fd , void* vptr , size_t total )
    {
    
        size_t  nleft;
        ssize_t nread;
        char    *ptr;
        
        ptr = (char*)vptr;
        nleft = total ;
        
        while ( nleft > 0 ) 
        {
            if ( (nread = read(fd , ptr , nleft ) < 0 ) ) 
            {
            
                if ( errno == EINTR ) 
                    nread = 0 ;
                
                else 
                    return -1;
                    
            } 
            
            else if ( nread == 0 )
                break ;              /**** has read EOF ****/
            
            nleft -= nread ;
            ptr   += nread ;
              
            //printf("\total read: %d bytes\n", nread);
            
         }
         
         return ( total - nleft );
            
        
    }
    
    ssize_t writen( int fd , const void* buf , size_t total )
    {
        size_t 		nleft;
        ssize_t		n_written;
        char 		*ptr;
        
        ptr 	= (char *)buf ;
        nleft	= total;
        
        while ( nleft > 0 )
        {
        	if ( (n_written=write( fd , (const char*)ptr , nleft)) <=0 )	
        	{
        		if ( errno == EINTR)
        		    n_written = 0 ;   /**** capture interrupted error ****/
        		else
        		    return -1;   /**** error occured ****/
        	}
        	
        	nleft -= n_written ;
        	ptr += n_written ;
        }
        
        return total;
    }
    
    /**** readline function of process fork version ****/
    static ssize_t my_read( int fd , char* ptr )
    {
        static int 	read_count = 0 ;
        static char 	*read_ptr ;
        static char 	read_buf[ MAXLEN ] ;
        
        if ( read_count <= 0 )
        {
            while (1)
            {
                if ( (read_count=read( fd , read_buf , sizeof(read_buf)) ) <0 )
                {
            	    if ( errno == EINTR )
            	        continue;
                }
                	    
                else if ( read_count == 0 )  /* read EOF */
            	    return 0;
            	    
            	else 
            	{
            	     read_ptr = read_buf ;
            	     break;
            	}    
                
            }  /**** end of while ****/	
           
        }   /**** end of read_count <= 0 ****/
        
        read_count-- ;
        *ptr = *read_ptr++;
        return 1;
    }
   
 
    ssize_t readline( int fd , void* vptr ,size_t maxlen )
    {
        ssize_t	n , rcnt ;
        char	c , *ptr ;
        
        ptr = (char*)vptr ;
        
        for ( n = 1 ; n < maxlen ; n++ )
        {
            if ( (rcnt=my_read(fd,&c)) == 1 )
            {
                *ptr++ = c ;
                if ( c == '\n' )
                    break;           /* newline needs to be stored. */
            }
            
            else if ( rcnt == 0 )
            {
                 if ( n == 1)
                     return 0;
                 else
                     break;
            }
            
            else 
                return -1;
        }
        
        *ptr = 0;
        return n;
    }
    
#endif 


