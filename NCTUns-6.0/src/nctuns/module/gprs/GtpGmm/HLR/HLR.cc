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

#include <stdlib.h>
#include <assert.h>
#include <gprs/GtpGmm/HLR/HLR.h>

HLR	HLR_;

HLR::HLR()
{
	//initial HLR_record
	HLR_rd  = NULL;
}
HLR::~HLR()
{
	struct HLR_record *tmp;
	while(HLR_rd != NULL)
	{
		tmp = HLR_rd;
		HLR_rd = HLR_rd->next;
		free(tmp);	
	}
}
ulong HLR::getSGSNIP(ulong RAI)
{
	HLR_record *path;
	path = HLR_rd;
	while(path != NULL)
	{
		if(path->imsi == RAI)
		{
			return(path->sgsnip);
		}
		path = path->next;
	}
	return(0);
}
int HLR::see_if_exist(ulong tlli)
{
	HLR_record *path;
	path = HLR_rd;
	int found = 0;
	while(path != NULL)
	{
		if(path->tlli == tlli)
		{
			found =1;
			break;
		}
		path = path->next;
	}
	return(found);
}
int HLR::add_record(HLR_record *h_record)
{
	struct HLR_record *tmp_HLR = (HLR_record *)malloc(sizeof(struct HLR_record));
	tmp_HLR->tlli = h_record->tlli;
	tmp_HLR->nsapi = h_record->nsapi;
	tmp_HLR->ms_ip = h_record->ms_ip;
	tmp_HLR->tid = h_record->tid;
	tmp_HLR->ggsnip = h_record->ggsnip;
	tmp_HLR->sgsnip = h_record->sgsnip;
	tmp_HLR->qos = h_record->qos;
	tmp_HLR->imsi = h_record->imsi;
	tmp_HLR->ptmsi = h_record->ptmsi;
	tmp_HLR->msisdn = h_record->msisdn;
	tmp_HLR->ca = h_record->ca;
	tmp_HLR->ra = h_record->ra;
	tmp_HLR->next = HLR_rd;
	HLR_rd = tmp_HLR;
	return(0);
}

int HLR::delete_record(HLR_record *h_record)
{
	HLR_record *path;
	HLR_record *prev;
	HLR_record *current;
	HLR_record *next;
	int	found =0;
	path = HLR_rd;
	while(path != NULL)
	{
		if(path->nsapi == h_record->nsapi || path->imsi == h_record->imsi
		  || path->msisdn == h_record->msisdn)
		{
			current = path;
			next = path->next;
			path = prev;
			path->next = next;
			free(current);
			found = 1;
			break;
		}
		
		
		prev = path;
		path = path->next;
	}
	if(found == 0)
	{
		//printf("can't find the record in HLR!!");
		return(-1);
	}
	return(0);
}

ulong HLR::modify_record(HLR_record *h_record)
{
	HLR_record *path;
	
	int	found =0;
	path = HLR_rd;
	while(path != NULL)
	{
		if(path->imsi == h_record->imsi
		  || path->msisdn == h_record->msisdn || path->ptmsi == h_record->ptmsi)
		{
			if(h_record->tlli != 0)
				path->tlli = h_record->tlli;
			if(h_record->nsapi != 0)
				path->nsapi = h_record->nsapi;
			if(h_record->ms_ip != 0)
				path->ms_ip = h_record->ms_ip;
			//printf("MS's IP in SGSN:%ld\n",path->ms_ip);
			if(h_record->tid != 0)
				path->tid = h_record->tid;
			if(h_record->ggsnip != 0)
				path->ggsnip = h_record->ggsnip;
			if(h_record->sgsnip != 0)
				path->sgsnip = h_record->sgsnip;
			//printf("SGSNIP:%ld in HLR\n",path->sgsnip);
			if(h_record->qos != 0)
				path->qos = h_record->qos;
			if(h_record->imsi != 0)
				path->imsi = h_record->imsi;
			if(h_record->ptmsi != 0)
                                path->ptmsi = h_record->ptmsi;
			if(h_record->msisdn != 0)
				path->msisdn = h_record->msisdn;
			if(h_record->ca != 0)
				path->ca = h_record->ca;
			if(h_record->ra != 0)
				path->ra = h_record->ra;
			found = 1;
			break;
		}
		path = path->next;
	}
	if(found == 0)
	{
		//printf("can't find the record in HLR and add this record!!");
		struct HLR_record *tmp_HLR = (HLR_record *)malloc(sizeof(struct HLR_record));
		bzero(tmp_HLR,sizeof(HLR_record));
		tmp_HLR->tlli = h_record->tlli;
		tmp_HLR->nsapi = h_record->nsapi;
		tmp_HLR->ms_ip = h_record->ms_ip;
		tmp_HLR->tid = h_record->tid;
		tmp_HLR->ggsnip = h_record->ggsnip;
		tmp_HLR->sgsnip = h_record->sgsnip;
		tmp_HLR->qos = h_record->qos;
		tmp_HLR->imsi = h_record->imsi;
		tmp_HLR->ptmsi = h_record->ptmsi;
		tmp_HLR->msisdn = h_record->msisdn;
		tmp_HLR->ca = h_record->ca;
		tmp_HLR->ra = h_record->ra;
		tmp_HLR->next = HLR_rd;
		HLR_rd = tmp_HLR;
		return(h_record->imsi);
	}
	return(path->imsi);
}

ip_array *HLR::search_record(ulong *ip_imsi)
{
	HLR_record *path=NULL;
	ip_array *ip_list = (struct ip_array *)malloc(sizeof(struct ip_array));
	bzero(ip_list,sizeof(struct ip_array));
	path = HLR_rd;
	int found = 0;
	while(path != NULL)
	{
		if(path->ggsnip == *(ip_imsi) || path->imsi == *(ip_imsi) 
		|| path->ms_ip == *(ip_imsi) || path->ms_ip == *(ip_imsi) || path->msisdn == *(ip_imsi))
		{
			//printf("we have found record:%ld\n",path->sgsnip);
			ip_list->ip = path->sgsnip;
			ip_list->tlli = path->tlli;
			ip_list->nsapi = path->nsapi;
			ip_list->tid = path->tid;
			ip_list->qos = path->qos;
			ip_list->imsi = path->imsi;
			ip_list->ptmsi = path->ptmsi;
			ip_list->msisdn = path->msisdn;
			ip_list->ra = path->ra;
			ip_list->next = NULL;
			found =1;
		}
		path = path->next;
	}
	
	if(found)
		return(ip_list);
	else
	{
		//printf("we don't find record in HLR!!\n");
		ip_list->ip = 0;
		return(ip_list);
	}
	return NULL;
}
ulong HLR::search_imsi(ulong *ptmsi)
{
	HLR_record *path;

        path = HLR_rd;
        while(path != NULL)
        {
                if(path->ptmsi == *(ptmsi))
                {
                        return(path->imsi);
                }
                path = path->next;
        }
        return(0);

}

int HLR::exist_record(ulong *imsi)
{
	 HLR_record *path;

        path = HLR_rd;
        while(path != NULL)
        {
                if(path->imsi == *(imsi))
                {
                        return(1);
                }
                path = path->next;
        }
        return(-1);

}
