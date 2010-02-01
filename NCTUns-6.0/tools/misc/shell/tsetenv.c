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
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h> 
#include <pwd.h>

int 
main(argc,argv)
    int argc;
    char *argv[];
{
   FILE *rc;
   FILE *rh;
   char temp[100];
   char temp1[100];
   char temp2[100];
   char temp3[100];
   char rhosts[50];
   struct passwd *p;
   int isset = 0;
   int nbyte;
   char curWorkDir[512];
   
   sprintf(rhosts,"%s %s", argv[3], argv[1]);
   p = getpwnam(argv[1]);
   (void)strcpy(temp, p->pw_dir);
   (void)strcpy(temp1, p->pw_dir); 
   (void)strcpy(temp2, p->pw_dir);
   (void)strcat(temp, "/.NCTUNS_SETENV");
   
   if(strstr(p->pw_shell,"tcsh") != NULL) 
     (void)strcat(temp1, "/.tcshrc");
   else if(strstr(p->pw_shell, "csh") != NULL)
          (void)strcat(temp1, "/.cshrc");
        else if(strstr(p->pw_shell, "bash") != NULL)
               (void)strcat(temp1, "/.bashrc");
             else if(strstr(p->pw_shell, "sh") != NULL)
                  (void)strcat(temp1, "/.shrc");
             else
                (void)strcat(temp1, "/.tcshrc");
   
   
   (void)strcat(temp2, "/.rhosts");
   
   rc = fopen(temp1,"a");       
   rh = fopen(temp2,"a");
   rc = freopen(temp1,"r",rc);
   rh = freopen(temp2,"r",rh);
   
   isset = 0;
   while(!feof(rc)){
      (void)fgets(temp3, 1024, rc);
      if(strstr(temp3, ".NCTUNS_SETENV") != NULL){
         isset = 1;
	 break;
      }
   }
   if(isset != 1){
      rc = freopen(temp1, "a", rc);
      //if(rc)
      	//fprintf(rc, "source ~/.bashrc\n");
      fprintf(rc,"source %s\n", temp);
   }   
   fclose(rc);
   
   isset = 0;
   while(!feof(rh)){
      (void)fgets(temp3, 1024, rh);
      if(strstr(temp3, rhosts) != NULL){
         isset = 1;
         break;   
      }
   }
   if(isset != 1){
      rh = freopen(temp2, "a", rh);
      fprintf(rh,"%s\n",rhosts);
   }   
   fclose(rh);

   rc = fopen("/tmp/.nctuns_Cur_Workdir", "r");
   if (rc != NULL) {
     nbyte = fscanf(rc, "%s", curWorkDir);
     fclose(rc);
   } else { 
     curWorkDir[0] = '\0';
   }

   rc = fopen(temp,"w");
   if (strstr(p->pw_shell, "bash") == NULL){
      fprintf(rc, "setenv NCTUNS_WORKDIR ");
      fprintf(rc, "%s\n", curWorkDir);
      fprintf(rc, "setenv NCTUNS_NODEID ");
      fprintf(rc, "%s\n", argv[2]);
      fprintf(rc, "setenv NCTUNSHOME %s\n", getenv("NCTUNSHOME"));
      fprintf(rc, "%s/tools/script %s \n", getenv("NCTUNSHOME"), temp);
   }
   else{
      fprintf(rc, "export NCTUNS_WORKDIR=");
      fprintf(rc, "%s\n", curWorkDir);
      fprintf(rc, "export NCTUNS_NODEID=");
      fprintf(rc, "%s\n", argv[2]);
      fprintf(rc, "export NCTUNSHOME=%s\n", getenv("NCTUNSHOME"));
      fprintf(rc, "%s/tools/script %s \n", getenv("NCTUNSHOME"), temp);
   }
   fclose(rc);
   
   //setenv("NCTUNSHOME", getenv("TERM"), 1);
   //setenv("TERM", "vt100", 1);
   (void)strcpy(temp3,"chown ");
   (void)strcat(temp3,p->pw_name);
   (void)strcat(temp3," ");
   (void)strcat(temp3,temp);
   (void)system(temp3);
   
   sprintf(temp3,"chown %s %s", p->pw_name, temp1);
   (void)system(temp3);

   return 0;
}
