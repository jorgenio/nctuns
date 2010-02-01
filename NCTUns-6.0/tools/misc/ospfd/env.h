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
class Env
{
   
   char **key;
   char **value;
   long index;
   //long environ;
   public:
      
   Env();
   ~Env();
   long search(char *keyword);
   long insert(char *keyword, char* strvalue );
   long remove(char *keyword);
   long change(char *keyword, char* newvalue);
   char* getvalue(long index) { 
   				  if (index < 100 && index >=0) return value[index]; 
   				  else return NULL;
                              };
   
};

Env::Env()
{
   key   = new char[100][100];
   value = new cahr[100][1000];
   index = -1; 
}

Env::~Env()
{
  if ( key )
     delete[] key;
  if ( value )
     delete[] value;
  
  index=0;
}



long Env::search(char *str)
{
   if ( !str )
   {
      for (long i=0; i<100; i++)
      {
         if ( !key[i] )
            return i;
      }
       
   }
   
   for (long i=0; i<100; i++)
   {
      if ( !key[i] )
         continue;
      if ( !strcmp( str , key[i]) )
         return i;
   }
   
   return -2;
}

long Env::insert(char *keyword , char *strvalue)
{
    if ( keyword == NULL || strvalue == NULL )
    {
        printf("bad args in insert env function\n");
        return -1;
    }
    long i=0;
    if ( (i=search( keyword )) < 0 ) //new item
    {
    	long ind = search(NULL);
    	if ( ind < 100 && ind >=0 )
    	{
           key[ind] = strdup(keyword);
           value[ind] = strdup(strvalue);
        }
    }    
    
    else 
    {
    	if ( value[i] )
    	   delete value[i];
    	   
        value[i] = strdup( strvalue );
    }   
    
    return 1;
}

Env::remove(char *keyword)
{
    if ( keyword == NULL )
    {
        printf("bad args in insert env function\n");
        return -1;
    }
    long i=0;
    if ( (i=search(keyword)) < 0 ) //new item
    {
        return -1;
    }    
    
    else 
    {
    	if ( key[i] )
    	   delete key[i];
    	if ( value[i] )
    	   delete value[i];
    	   
        value[i] = strdup( strvalue );
    }   
    
    return 1;
}

Env::change(char* keyword , char *newvalue)
{
    if ( keyword == NULL || newvalue == NULL )
    {
        printf("bad args in insert env function\n");
        return -1;
    }
    long i=0;
    if ( (i=search( keyword) ) < 0 ) //new item
    {
        return -1;
    }    
    
    else 
    {
    	
    	if ( value[i] )
    	   delete value[i];
    	   
        value[i] = strdup( newvalue );
    }   
    
    return 1;
}
