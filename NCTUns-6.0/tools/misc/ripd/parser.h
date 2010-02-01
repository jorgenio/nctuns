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
#include <string.h>
#include <stdlib.h>
#include "command.h"

/****  global data 
***** tmp is for queuing token 
***** command is for queuing command name 
****/

Queue      	*tmp       = NULL ;
Command 	*cmd_array  = NULL ;
long         	num_of_cmd = -1   ;   /**** cmd_array starts from zero. ****/


int parser ( char* command_line )
{ 
    char* token = NULL;
    char* str   = command_line ;
    long  flag = 0 ;    
    tmp         = new Queue;
    
    if ( cmd_array != NULL )
        delete [] cmd_array;
        
    cmd_array   = new Command[1000] ;
    
    num_of_cmd  = -1;  
    while ( true )
    {
        
          if (  (token = strtok( str , " \r\n\t" ) ) == NULL )
          break;
          
          //printf("T:%s\n",token);
        
          if ( strcmp( token, "|") == 0 )    /**** processing ordinary pipe symbol ****/
          {
                 /**** pack the tokens in tmp queue into a command and its arguments ****/
                
                 char* command_name     = tmp->dequeue();
                 char* arg		= NULL;
             
                 if ( !command_name )  /**** command_name == NULL ****/
                {
             	   //printf( "Parse Error: no command name could be found\n");
             	   return -1;
                }
             
                else 
                    cmd_array[ ++num_of_cmd  ].cmd_name = command_name ;
                  
                while (  (arg=tmp->dequeue() ) != NULL )  /**** processing argv ****/
               {
             	  cmd_array[ num_of_cmd ].argv->enqueue( arg ) ;
               }
          
          }
        
          else if ( strcmp( token , "||" ) == 0 )     /**** interpret pipe ourselves defined ****/
          {
                 /**** pack the tokens in tmp queue into a command and its arguments ****/
             
                 char* command_name    = tmp->dequeue();
                 char* arg		= NULL;
             
                 if ( !command_name )  /**** command_name == NULL ****/
                {
             	   printf( "Parse Error: no command name could be found\n");
             	   return -1;
                }
             
                else 
                    cmd_array[ ++num_of_cmd  ].cmd_name = command_name ;
                  
                while (  (arg=tmp->dequeue() ) != NULL )  /**** processing argv ****/
                {
             	  cmd_array[ num_of_cmd ].argv->enqueue( arg ) ;
                }
                  
             
             /**** add the double line program into command queue ****/
             
             cmd_array[ ++ num_of_cmd ].cmd_name = "doubleline";
             cmd_array[ num_of_cmd ].argv = NULL ;
             
        } /**** end of processing || ****/

        else if ( !strcmp( token , "]" ) || !strcmp( token , ">")  ) /**** finding redirection to a file symbol ****/
        {
             
             /**** pack the tokens in tmp queue into a command and its arguments ****/
             flag = 1 ;
             
             char* command_name    = tmp->dequeue();
             char* arg		= NULL;
             
             if ( !command_name )  /**** command_name == NULL ****/
             {
             	printf( "Parse Error: no command name could be found\n");
             	return -1;
             }
             
             else 
                  cmd_array[ ++num_of_cmd  ].cmd_name = command_name ;
                  
             while (  (arg=tmp->dequeue() ) != NULL )  /**** processing argv ****/
             {
             	cmd_array[ num_of_cmd ].argv->enqueue( arg ) ;
             }    
             
             /**** add the line-filter program into command queue ****/
             

             //printf("ADecmd_#1=%d:\n",num_of_cmd);
             char *redirect_arg = NULL;
             if ( (redirect_arg = strtok( str , " \r\n\t" )) != NULL ) /**** str == NULL ****/
             {
                 if ( !strcmp(redirect_arg,"|") || !strcmp(redirect_arg,"||") || 
                      !strcmp(redirect_arg,"]") || !strcmp(redirect_arg,"[" ) ||
                      !strcmp(redirect_arg,">") || !strcmp(redirect_arg,"<" ) 
                    )
                 {
                     printf("ambiguous command chain\n");
                     return -1;
                 }
                 
                 if ( !strcmp( token , ">") )
                      cmd_array[ ++ num_of_cmd ].cmd_name = "redirection_to_file"; 
                 else 
                      cmd_array[ ++ num_of_cmd ].cmd_name = "filterline_to_file";  
                      
                 cmd_array[ num_of_cmd ].argv->enqueue( redirect_arg );
                 //printf("argT:%s\n",redirect_arg);
             }	
             else 
             {
             	 printf("Parser Error: no required argument following ] \n");
             	 return -1;
             }
             //printf("ADecmd_#2=%d:\n",num_of_cmd);
        }
        
       else if ( strcmp( token , "[") == 0 )  /**** finding redirection from a file symbol ****/
       {
             
             /**** pack the tokens in tmp queue into a command and its arguments ****/
             flag = 2 ;
             char* command_name    = tmp->dequeue();
             char* arg		= NULL;
             
             if ( !command_name )  /**** command_name == NULL ****/
             {
             	printf( "Parse Error: no command name could be found\n");
             	return -1 ;
             }
             
             else 
                  cmd_array[ ++num_of_cmd  ].cmd_name = command_name ;
                  
                  
             while (  (arg=tmp->dequeue() ) != NULL )  /**** processing argv ****/
             {
             	cmd_array[ num_of_cmd ].argv->enqueue( arg ) ;
             }    
             
             /**** redirection has not implemented yet ****/
                 
       }   /**** end of processing [ symbol ****/      
        
        else /**** normal command name or arguments ****/
       {
    	 tmp->enqueue( token ) ;		 
       }
	        
        str = NULL ;
	
    } /**** end of while loop ****/    

    //printf("Ae_num_of_cmd#3==%d\n",num_of_cmd );
    /**** pack the tokens in tmp queue into a command and its arguments ****/
    char* command_name    = tmp->dequeue();
    char* arg		= NULL;
             
    if ( !command_name && flag == 0)  /**** command_name == NULL ****/
    {
        	printf( "Parse Error: no command name could be found\n");
           	return -1;
    }
    
    else if ( !command_name && flag >= 1 )
    {
         return 1; /**** since the arg of  ] or  [ is latched by themselves in their clause ****/
    }
             
    else 
    {
            cmd_array[ ++num_of_cmd  ].cmd_name = command_name ;
            //printf("De:cmd_name=%s\n",command_name );
      
    	    while (  (arg=tmp->dequeue() ) != NULL )  /**** processing argv ****/
    	    {
          	 cmd_array[ num_of_cmd ].argv->enqueue( arg ) ;
    	    }
    }
    
    delete tmp;
    
    //printf("Ae_num_of_cmd#4==%d\n",num_of_cmd );
    return 1 ;
	
}
