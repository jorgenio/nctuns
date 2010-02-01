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
#include <errno.h>
#include <sys/time.h>
#include <sys/select.h>
#include <iostream>

#include "IOStream.H"

using namespace std;

int IOStream::readLine(int fd, void *buffer, int maxLen){
	
	/* check if the file descriptor is not readable, return with io exception
	 * thrown
	 * */
	if (readableTimeout(fd, 5) <0)
		throw IOException();
	
	int		n, rc;
	char	c, *ptr;
	
	ptr = (char*)buffer;
	
	for (n=1; n<maxLen; n++){
		again:
			if ((rc = read(fd, &c, 1)) == 1 ){
				*ptr++ = c; 
				if (c == '\n'){
					break;
				}
			}//if
			else if (rc == 0){
				if (n == 1) 
					return 0;
				else 		
					break;				
			}// else if
			else{
				if (errno == EINTR)
					goto again;
				else
					return -1;
			}//else
	}//for
	*ptr = 0;
	return n;
}


int IOStream::writen(int fd, const void *vptr, int n){

	int nleft;
	int nwritten;
	const char *ptr;
	
	ptr = (char*)vptr;
	nleft = n;
	while (nleft > 0){
		if ((nwritten = write(fd, ptr, nleft))<=0){
			if(errno == EINTR)
				nwritten = 0;
			else
				throw IOException();
				//return -1;			
		}//if
		nleft -= nwritten;
		ptr	+= nwritten;
	}//while
	
	return n;	
}

int IOStream::readn(int fd, void* buffer, int n){
	
	/* check if the file descriptor is not readable, return with io exception
	 * thrown
	 * */
	if (readableTimeout(fd, 5) <0)
		throw IOException();

	int 	nLeft;
	int 	nRead;
	char 	*ptr;
	
	ptr = (char*)buffer;
	nLeft = n;
	
	while (nLeft > 0){
		if ((nRead = read(fd, ptr, nLeft)) < 0){
			if (errno == EINTR)
				nRead = 0;
			else
				throw IOException();
				//return -1;
		}
		else if ( nRead == 0)
			break;
		else
			nLeft -= nRead;
			ptr   += nRead;
		
	}
	return (n - nLeft); 
}

/*
 * Check if the file descriptor is readable within a timeout value
 * @param	fd		file descriptor
 * @param	sec		time out value, in seconds
 * @return	>0	readable;	<0 timeout
 */
int 
IOStream::
readableTimeout(int fd, int sec){
	struct timeval 	tv;
	fd_set			rset;

	tv.tv_sec 	= sec;
	tv.tv_usec	= 0;

	FD_ZERO(&rset);
	FD_SET(fd, &rset);

	return (select(fd+1, &rset, NULL, NULL, &tv));
		
}

/*
 * Class Constructor for IOExcption
 */
IOStream::IOException::IOException(){
	message = "NULL";

}
