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

#include "neighbor_bs.h"

NbrBS::NbrBS(int _nid, uint8_t *_addr)
{
	nid     = _nid;
	addr    = new Mac_address(_addr);

	PHY_ProfileID   = 0;
	PreambleIndex   = 0;
	HO_Optimize     = 0;
	SchServSupport  = 0;
	DCDCfgCount     = 0;
	UCDCfgCount     = 0;
	ScanType        = 0;
	CINR            = 0;
	RSSI            = 0;
	RelativeDelay   = 0;
	RTD             = 0;
	Index           = 0;
	targetHO        = false;
}

NbrBS::~NbrBS()
{
	delete addr;
}

void NbrBS::dump()
{
	printf("#########################\n");
	printf("# Index: %d\n", Index);
	printf("# Pream: %d\n", PreambleIndex);
	printf("# Addr : %s\n", addr->str());
	printf("# DCD  : %d\n", DCDCfgCount);
	printf("# UCD  : %d\n", UCDCfgCount);
}

NeighborBSs::NeighborBSs()
{
	NBRADV_CfgCount      = 1;
	ScanDuration         = 0;
	ReportMode           = 0;
	ReportMetric         = 0;
	ReportPeriod         = 0;

	StartFrame           = 0;
	InterleavingInterval = 0;
	ScanIteration        = 0;
	ScanTimes            = 0;

	ServingBSchID        = 0;
	ScanningChID         = 0;
}

NeighborBSs::~NeighborBSs()
{
	while(!nbrBSs_Index.empty())
	{
		delete *(nbrBSs_Index.begin());
		nbrBSs_Index.erase(nbrBSs_Index.begin());
	}

	while(!nbrBSs_Full.empty())
	{
		delete *(nbrBSs_Full.begin());
		nbrBSs_Full.erase(nbrBSs_Full.begin());
	}

	while(!nbrBSs_Curr.empty())
	{
		delete *(nbrBSs_Curr.begin());
		nbrBSs_Curr.erase(nbrBSs_Curr.begin());
	}
}

NbrBS *NeighborBSs::getNbrbyBSID(uint8_t *_addr)
{
	vector<NbrBS *>::iterator iter;

	for (iter = nbrBSs_Index.begin();iter != nbrBSs_Index.end();iter++)
	{
		if (memcmp((*iter)->addr->buf(), _addr, 6) == 0)
		{
			return (*iter);
		}
	}

	for (iter = nbrBSs_Full.begin();iter != nbrBSs_Full.end();iter++)
	{
		if (memcmp((*iter)->addr->buf(), _addr, 6) == 0)
		{
			return (*iter);
		}
	}
	return NULL;
}

NbrBS *NeighborBSs::getNbrbyChID(uint8_t chID)
{
	vector<NbrBS *>::iterator iter;

	for (iter = nbrBSs_Index.begin();iter != nbrBSs_Index.end();iter++)
	{
		if ((*iter)->PreambleIndex == chID)
		{
			return (*iter);
		}
	}

	for (iter = nbrBSs_Full.begin();iter != nbrBSs_Full.end();iter++)
	{
		if ((*iter)->PreambleIndex == chID)
		{
			return (*iter);
		}
	}
	return NULL;
}

int NeighborBSs::getNextScanChID()
{
	vector<NbrBS *>::iterator iter;
	uint32_t Index_i    = 0;
	uint32_t Full_i     = 0;
	bool Index_Found    = false;
	bool Full_Found     = false;

	// scan current position first
	for (iter = nbrBSs_Index.begin();iter != nbrBSs_Index.end();iter++)
	{
		Index_i++;

		if ((*iter)->PreambleIndex == ScanningChID)
		{
			Index_Found = true;
			break;
		}
	}

	for (iter = nbrBSs_Full.begin();iter != nbrBSs_Full.end();iter++)
	{
		Full_i++;

		if ((*iter)->PreambleIndex == ScanningChID)
		{
			Full_Found = true;
			break;
		}
	}

	if (Index_Found == true)
	{
		if (Index_i == nbrBSs_Index.size())
		{
			if (nbrBSs_Full.size() != 0)
			{
				return nbrBSs_Full[0]->PreambleIndex;
			}
			else
			{
				return nbrBSs_Index[0]->PreambleIndex;
			}
		}
		else
		{
			return nbrBSs_Index[Index_i]->PreambleIndex;
		}
	}
	else if (Full_Found == true)
	{
		if (Full_i == nbrBSs_Full.size())
		{
			if (nbrBSs_Index.size() != 0)
			{
				return nbrBSs_Index[0]->PreambleIndex;
			}
			else
			{
				return nbrBSs_Full[0]->PreambleIndex;
			}
		}
		else
		{
			return nbrBSs_Full[Full_i]->PreambleIndex;
		}
	}
	else
	{
		return nbrBSs_Index[0]->PreambleIndex;
	}
}

bool NeighborBSs::checkHOterms()
{
	vector<NbrBS *>::iterator iter;
	bool hasTarget = false;

	for (iter = nbrBSs_Index.begin();iter != nbrBSs_Index.end();iter++)
	{
		if (((*iter)->CINR - ServingCINR) >= HO_THRESHOLD)
		{
			(*iter)->targetHO = true;
			hasTarget = true;
			printf("\e[1;35m## NBR_BS->CINR:%d  ##  ServingCINR:%lf ## .. Fit handover condition ..\e[0m\n", (*iter)->CINR, ServingCINR);
		}
	}

	for (iter = nbrBSs_Full.begin();iter != nbrBSs_Full.end();iter++)
	{
		if (((*iter)->CINR - ServingCINR) >= HO_THRESHOLD)
		{
			(*iter)->targetHO = true;
			hasTarget = true;
			printf("\e[1;35m## NBR_BS->CINR:%d  ##  ServingCINR:%lf ## .. Fit handover condition ..\e[0m\n", (*iter)->CINR, ServingCINR);
		}
	}

	return hasTarget;
}

NbrBS *NeighborBSs::getTargetBS()  // Now we choose the best CINR to be the target BS
{
	vector<NbrBS *>::iterator iter;
	NbrBS *trgetBS  = NULL;
	int max_CINR    = 0;

	for (iter = nbrBSs_Index.begin();iter != nbrBSs_Index.end();iter++)
	{
		if (((*iter)->targetHO == true) && ((*iter)->CINR > max_CINR))
		{
			trgetBS = *iter;
		}
	}

	for (iter = nbrBSs_Full.begin();iter != nbrBSs_Full.end();iter++)
	{
		if (((*iter)->targetHO == true) && ((*iter)->CINR > max_CINR))
		{
			trgetBS = *iter;
		}
	}

	return trgetBS;
}

void NeighborBSs::dump()
{
	vector<NbrBS *>::iterator iter;

	for (iter = nbrBSs_Index.begin();iter != nbrBSs_Index.end();iter++)
	{
		printf("===========Start nbrBSs_Index List==========\n");
		(*iter)->dump();
		printf("===========End nbrBSs_Index List===========\n");
	}

	for (iter = nbrBSs_Full.begin();iter != nbrBSs_Full.end();iter++)
	{
		printf("===========Start nbrBSs_Full List==========\n");
		(*iter)->dump();
		printf("===========End nbrBSs_Full List===========\n");
	}
}
