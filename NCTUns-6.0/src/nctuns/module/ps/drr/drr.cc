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

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <nctuns_api.h>
#include <ps/DRR/drr.h>
#ifdef LINUX
#include <netinet/in.h>
#endif
#include <ethernet.h>
#include <tcp.h>
#include <udp.h>
#include <ip.h>
#include <mbinder.h>


MODULE_GENERATOR(drr);

drr::drr(u_int32_t type, u_int32_t id, struct plist* pl, const char *name)
                : NslObject(type, id, pl, name)
{
	/* disable flow control */
        s_flowctl = DISABLED;
	r_flowctl = DISABLED;

	DrrMaxBufSpaceForAllFlows = 200;
	DrrMaxQueLenForEachFlow = 50;
	CreditAddedPerRound = 1500;

        /*bind for RR*/
	vBind("DrrMaxBufSpaceForAllFlows", &DrrMaxBufSpaceForAllFlows);
	vBind("DrrMaxQueLenForEachFlow", &DrrMaxQueLenForEachFlow);
	vBind("CreditAddedPerRound", &CreditAddedPerRound);
        vBind("s_mask_string",&s_mask_string);
        vBind("d_mask_string",&d_mask_string);
        vBind("protocol_care",&protocol_care);
        vBind("s_portn_care",&s_portn_care);
        vBind("d_portn_care",&d_portn_care);

}

drr::~drr() {

}


int drr::init() {

	int             (NslObject::*upcall)(MBinder *);
       
        int s_pointer=0;
        int d_pointer=0;
        int x=0;

        DrrPacketTotal=0;
        DrrTableAllMax=0;

        drrTableAll=(drrTblAll *)malloc(sizeof(drrTblAll));
        drrTableAll->Counter=0;
        drrTableAll->First=NULL;
        drrTableAll->Tail=NULL;
        drrTableRR=NULL;

          //Construct a spanning Queue for traffic
         SpanningTbl=(drrTbl *)malloc(sizeof(drrTbl));
         SpanningTbl->Counter=0;
         SpanningTbl->Priority=0;
         SpanningTbl->Timer=0;
         SpanningTbl->First=NULL;
         SpanningTbl->Tail=NULL;

       


        //Construct a special Queue for traffic
        SpecialTbl=(drrTbl *)malloc(sizeof(drrTbl));
 	SpecialTbl->Counter=0;
 	SpecialTbl->Priority=0;
 	SpecialTbl->Timer=0;
 	SpecialTbl->First=NULL;
 	SpecialTbl->Tail=NULL;
        
        /* for the long-robin if not be initial */
        if(!s_mask_string)
        {
           s_mask_string=new char[16];
           strcpy(s_mask_string,"255.255.255.255");
        }
        if(!d_mask_string)
        {
           d_mask_string=new char[16];
           strcpy(d_mask_string,"255.255.255.255");
        }
        if(!protocol_care)
        {
           protocol_care=new char[4];
           strcpy(protocol_care,"on");
        }
        if(!s_portn_care)
        {
          s_portn_care=new char[4];
          strcpy(s_portn_care,"on");
        }
        if(!d_portn_care)
        {
          d_portn_care=new char[4];
          strcpy(d_portn_care,"on");
        }
        
       if(!DrrMaxBufSpaceForAllFlows)
          DrrMaxBufSpaceForAllFlows=200;

        for(x=0;x<=3;x++)
        {
           if(x==0)
 	   {
                 s_mask_long=atol(s_mask_string)*16777216;
		 d_mask_long=atol(d_mask_string)*16777216; 
           }
           if(x==1)
           {
                  s_mask_long=s_mask_long+atol(s_mask_string+s_pointer+1)*65536;
                  d_mask_long=d_mask_long+atol(d_mask_string+d_pointer+1)*65536;
           }
           if(x==2)
           {
       		  s_mask_long=s_mask_long+atol(s_mask_string+s_pointer+1)*256;
           	  d_mask_long=d_mask_long+atol(d_mask_string+d_pointer+1)*256;
           }
           if(x==3)
           {
            	  s_mask_long=s_mask_long+atol(s_mask_string+s_pointer+1);
            	  d_mask_long=d_mask_long+atol(d_mask_string+d_pointer+1);
           	  break;
           }
           d_pointer++;
           s_pointer++;
  
           while(*(s_mask_string+s_pointer)!='.')
              s_pointer++;
           while(*(d_mask_string+d_pointer)!='.')
              d_pointer++;
}


        /* Delete Queue Event */ 
        Event_ *event;
        BASE_OBJTYPE(type);
        type=POINTER_TO_MEMBER(drr,Delete_que);
        event=createEvent();
        setObjEvent(event,GetNodeCurrentTime(get_nid())+10000000,0,this,type,NULL);




	/* set upcall */
        upcall = (int (NslObject::*)(MBinder *))&drr::intrq;
        sendtarget_->set_upcall(this, upcall);
        return(1);
}

int drr::send(ePacket_ *pkt) {

	struct ether_header *eh;
	struct ip *iph;
	struct tcphdr *tcph;
	struct udphdr *udph;
	Packet *p;
	int Grades;

	drrTbl *drrFind=NULL;

	assert(pkt&&pkt->DataInfo_);

	GET_PKT(p,pkt);
	eh=(struct ether_header *)p->pkt_get();

	/* The following is spnning tree pkt */
	if(ntohs(eh->ether_type)==0x0001)
	{

	  if(sendtarget_->qfull())
	  {
	      ePacket_inQue *newPacket_inQue=(ePacket_inQue *)malloc(sizeof(ePacket_inQue));
	      newPacket_inQue->ep=pkt;
	      newPacket_inQue->Next=NULL;
	     if(SpanningTbl->Counter==0)
	     {
	       SpanningTbl->First=newPacket_inQue;
	       SpanningTbl->Tail=newPacket_inQue;
	     }else
	     {
	       SpanningTbl->Tail->Next=newPacket_inQue;
	       SpanningTbl->Tail=newPacket_inQue;
	     }
	  }
	  else {
                    return(NslObject::send(pkt));
               }
  	    return 1;
	}

	/* The following is traffic pkt */
	if(ntohs(eh->ether_type)!=0x0800)
	{


	  	if(sendtarget_->qfull())
	  	{

	     		ePacket_inQue *newPacket_inQue=(ePacket_inQue *)malloc(sizeof(ePacket_inQue));
	     		newPacket_inQue->ep=pkt;
             		newPacket_inQue->Next=NULL;
   	 	 	if(SpecialTbl->Counter==0)
   	 	 	{
   	 	   		SpecialTbl->First=newPacket_inQue;
   	 	   		SpecialTbl->Tail=newPacket_inQue;
     	 	 	}else
    	 	 	{
       				SpecialTbl->Tail->Next=newPacket_inQue;
      				SpecialTbl->Tail=newPacket_inQue;
     		  	}
          	}
   	  	else
  	  	{
                	//printf(" Traffic_Empty Send");
                	return(NslObject::send(pkt));
   	  	}

   		return 1;

      	}

        /* IP Header Type */
	iph= (struct ip *)p->pkt_sget();
	if(iph->ip_p==6)
	{
  	  	tcph=(struct tcphdr *)((char *)iph+sizeof(struct ip));
	}else if((int)iph->ip_p==17)
 	{
                udph=(struct udphdr *)((char *)iph+sizeof(struct ip));
	}else if((int)iph->ip_p==1)
	{
        }

	/*The Following is IP*/
	if( sendtarget_->qfull() ) {
           	if(DrrPacketTotal>=DrrMaxBufSpaceForAllFlows)
           	{ 
              		freePacket(pkt);
              		return 1;
           	}
	 /* Search for the true Queue */
	 drrFind=drrTableAll->First;
	 if(drrFind!=NULL)
   	 for(drrFind=drrTableAll->First;drrFind!=NULL;drrFind=drrFind->Next)
   	 {
           	Grades=0;
           	if(drrFind!=NULL)
           	{
                     	if(drrFind->s_ip==(htonl(iph->ip_src) & s_mask_long))
                        	Grades++;
                     	if(drrFind->d_ip==(htonl(iph->ip_dst) & d_mask_long))
                        	Grades++;
                     	if(drrFind->Protocol==iph->ip_p || !strcasecmp(protocol_care,"off"))
                        	Grades++;
                     	if(iph->ip_p==6)
                     	{
                        	if(drrFind->s_portn==tcph->th_sport || !strcasecmp(s_portn_care,"off"))
                           		Grades++;
                        	if(drrFind->d_portn==tcph->th_dport || !strcasecmp(d_portn_care,"off"))
                           		Grades++;
                     	}
                     	else if(iph->ip_p==17)
                     	{
        			if(drrFind->s_portn==udph->uh_sport || !strcasecmp(s_portn_care,"off"))
         		   		Grades++;
        			if(drrFind->d_portn==udph->uh_dport || !strcasecmp(d_portn_care,"off"))
         		   		Grades++;
      		     	}
      		     	else if(iph->ip_p==1)
      		     	{
                           	Grades++;
                           	Grades++;
                      	}
                   	if(Grades==5)
                      		break;
            	}//if End.
      	}//For End.

    	/* if we can't find the queue, we must add it */
    	if(drrFind==NULL)
    	{

      		ePacket_inQue *newPacket_inQue=(ePacket_inQue *)malloc(sizeof(ePacket_inQue));
      		newPacket_inQue->ep=pkt;
      		newPacket_inQue->Next=NULL;

      		drrTbl *newdrrTbl=(drrTbl *)malloc(sizeof(drrTbl));
      		newdrrTbl->s_ip=(htonl(iph->ip_src) & s_mask_long);
      		newdrrTbl->d_ip=(htonl(iph->ip_dst) & d_mask_long);
      		newdrrTbl->Protocol=iph->ip_p;
      		newdrrTbl->pkt_length=ntohs(iph->ip_len);
      		newdrrTbl->Priority=0;

      		/* We det drrFind to judge it is new or old */
      		drrFind=newdrrTbl;

      		if(newdrrTbl->Protocol==6)
      		{
       			newdrrTbl->s_portn=tcph->th_sport;
       			newdrrTbl->d_portn=tcph->th_dport;
      		}else if(newdrrTbl->Protocol==17)
      		{
        		newdrrTbl->s_portn=udph->uh_sport;
        		newdrrTbl->d_portn=udph->uh_dport;
      		}else if(newdrrTbl->Protocol==1)
      		{
        		newdrrTbl->s_portn=0;
        		newdrrTbl->d_portn=0;
      		}

      		/* The pkt in Queue counter =1 */
      		newdrrTbl->Counter=1;
      		newdrrTbl->First=newPacket_inQue;
      		newdrrTbl->Tail=newPacket_inQue;
      		newdrrTbl->Next=NULL;

      		/* Add Total pkt */
      		DrrPacketTotal++;
      
      		/* Add Total pkt queu */
      		drrTableAll->Counter++;
 
      		/* Set the Queue timer = 0 */
      		newdrrTbl->Timer=0;

      		/* We connect all queue with the new queue together */ 
      		if(drrTableAll->Counter==1)
      		{
        		drrTableAll->First=newdrrTbl;
        		drrTableAll->Tail=newdrrTbl;
      		}else if(drrTableAll->Counter>1)
      		{
        		drrTableAll->Tail->Next=newdrrTbl;
        		drrTableAll->Tail=newdrrTbl;
      		}
      		/* This record the Max queue is constructed */
      		if(DrrTableAllMax<drrTableAll->Counter)
        		DrrTableAllMax=drrTableAll->Counter;
   		}else if(drrFind!=NULL)
   		{
                	/* queue is full. Don't accept incoming pkt. */
        		if(drrFind->Counter>=DrrMaxQueLenForEachFlow)
        		{
                	                freePacket(pkt);
                			return 1;
        		}else if(drrFind->Counter<DrrMaxQueLenForEachFlow)
        	{
         		ePacket_inQue *newPacket_inQue=(ePacket_inQue *)malloc(sizeof(ePacket_inQue));
         		newPacket_inQue->ep=pkt;
         		newPacket_inQue->Next=NULL;
         		DrrPacketTotal++;
         		drrFind->Counter++;
         		if(drrFind->Counter==1)
         		{
           	      		/* If it's the first pkt , we need assign First and Tail */
           			drrFind->First=newPacket_inQue;
           			drrFind->Tail=newPacket_inQue;
           			drrFind->pkt_length=ntohs(iph->ip_len);
           			drrFind->Priority=0;

           			/* we need stop the timer */
           			drrFind->Timer=0;

         		}else if(drrFind->Counter>1)
         		{
           		        drrFind->Tail->Next=newPacket_inQue;
            			drrFind->Tail=newPacket_inQue;
         		}
      		} // < DrrMaxQueLenForEachFlow End.
   	}// if DrrFind!=NULL End.


  	//if DrrPacekt Total=0 before , the RR must point to the new queue.
  	if(DrrPacketTotal==1)
  	{
       		/* RR need be assign the pointer from the new queue*/
       	       	drrTableRR=drrFind;
       		/* Inform RR to chang the quota */
       		drrTableRR_var=1;
  	}
   		return 1;
 	}else 
	{         
	       	return(NslObject::send(pkt)); 
	}
}


int drr::recv(ePacket_ *pkt) {
        
	/* Just by pass incoming packet */
        assert(pkt&&pkt->DataInfo_);
        return(NslObject::recv(pkt));
}


int drr::intrq(MBinder *port) {
       
        ePacket_inQue *freePacket_inQue=NULL;
	ePacket_      *pkt=NULL;
        Packet      *p;
        struct ip *iph;
       	if(SpanningTbl->Counter>0)
       	{
                pkt=SpanningTbl->First->ep;
                freePacket_inQue=SpanningTbl->First;
                SpanningTbl->First=SpanningTbl->First->Next;
                SpanningTbl->Counter--;
       	}       
       	else if(SpecialTbl->Counter>0)
       	{
                pkt=SpecialTbl->First->ep;
                freePacket_inQue=SpecialTbl->First;
                SpecialTbl->First=SpecialTbl->First->Next;
                SpecialTbl->Counter--;
       	}else if(DrrPacketTotal>0)
       	{       
             	/* If RR be changed before we need give the quota. */
             	if(drrTableRR_var==1 && drrTableRR->Counter!=0)
             	{
                        drrTableRR->Priority=drrTableRR->Priority+CreditAddedPerRound;
               		drrTableRR_var=0;
               	}

                    
              	/* If the queue have the priority to send */
              	if(drrTableRR->Priority>=drrTableRR->pkt_length && drrTableRR->Counter>0)
              	{
                      	pkt=drrTableRR->First->ep;   
                 
                	/* we need to free the origional pkt in queue structure */
                	freePacket_inQue=drrTableRR->First;
                	drrTableRR->First=drrTableRR->First->Next;
                	drrTableRR->Priority=drrTableRR->Priority-drrTableRR->pkt_length;
                	drrTableRR->Counter--;
                	DrrPacketTotal--;
              
                	/* if still exist the pkt in queue,we need record the pkt length */
                	if(drrTableRR->Counter>0)
                	{
                  		GET_PKT(p,drrTableRR->First->ep);
                  		iph= (struct ip *)p->pkt_sget();
                  		drrTableRR->pkt_length=ntohs(iph->ip_len);
                	}else if(drrTableRR->Counter<=0)
                	{
                     		/*Call Timer start*/
                     		drrTableRR->Priority=0;
                	}

                	/* if the queue not own the priority,we need change RR */
                	if(drrTableRR->Counter<=0 || drrTableRR->Priority<drrTableRR->pkt_length)
                	{
                    		if(DrrPacketTotal>0)
                    		{
                       			drrTableRR=drrTableRR->Next;
                       			if(drrTableRR==NULL)
                          			drrTableRR=drrTableAll->First;
                      			while(drrTableRR->Counter<=0)
                      			{
                       				if(drrTableRR->Next==NULL)
                            				drrTableRR=drrTableAll->First;
                       				else
                            				drrTableRR=drrTableRR->Next;
                      			}
 
                      			/* if we cange RR , we need assign the Flag=1 */
                       			drrTableRR_var=1;
                    		}else if(DrrPacketTotal<=0)
                     		{drrTableRR=NULL;}
                  	} 
               	} // if priority End.

     	}// if PacketTotal >0 End.


       	if(pkt!=NULL){
         	assert(sendtarget_->enqueue(pkt) == 0);
                free(freePacket_inQue);
	}
	return(1);
}
int drr::Delete_que(Event_ *event)
{
    	//printf("\nDel_que Event");
     	freeEvent(event);

     	drrTbl *DelFind=drrTableAll->First,*DelFindPrev;
       	DelFindPrev=NULL;
     	if(DelFind!=NULL)
     	for(DelFind=drrTableAll->First;DelFind!=NULL;DelFind=DelFind->Next)
     	{
       		if(DelFind->Counter<=0)
          		DelFind->Timer++;
       		if(DelFind->Timer>=5)
       		{
                   	if(drrTableAll->First==DelFind)
                   	{
                     		drrTableAll->First=DelFind->Next;
                     		if(drrTableAll->First==NULL)
                       			drrTableAll->Tail=NULL;
                   	}
                   	else if(drrTableAll->Tail==DelFind)
                   	{
                    		DelFindPrev->Next=DelFind->Next;
                    		drrTableAll->Tail=DelFindPrev;
                   	}
                   	else
                    		DelFindPrev->Next=DelFind->Next;

              		drrTableAll->Counter--;
             		free(DelFind);
       		}
       		DelFindPrev=DelFind;

     	}
     	BASE_OBJTYPE(type);
     	type=POINTER_TO_MEMBER(drr,Delete_que);
     	event=createEvent();
     	setObjEvent(event,GetNodeCurrentTime(get_nid())+10000000,0,this,type,NULL);

    	return 1;
}
