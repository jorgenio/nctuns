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

#include "parse_ncc_config.h"
#include <satellite/dvb_rcs/common/ncc_config.h>

#define	MAX_LINE_SIZE	1000


int 
parse_ncc_config(char *path, Ncc_config &ncc_config, Rcst_info_list& rcst_infos)
{
	FILE*		config_file;
	char		line[MAX_LINE_SIZE];
	char		buf[1000];
	bool		max_sid_found, cen_freq_found, sbl_rate_found, roll_off_found,
			num_atm_found, num_data_slot_found, num_req_slot_found, 
			num_frame_found, coding_found, mod_found, fca_flag_found, 
			pre_len_found, bst_start_found, sat_nid_found;

	uint16_t	preamble_len_in_bit;
	uint64_t	symbol_rate;
	uint16_t	modulation_rate; // in bits/symbol.



	bzero((void*)&ncc_config, sizeof(Ncc_config));
	ncc_config.modulation_rate = 2;

	// Open file
	config_file = fopen(path, "r");
	if(!config_file)
	{
		printf("[Warning] open NCC config file (%s) error!!\n", path);
		assert(0);
	}

	max_sid_found = cen_freq_found = sbl_rate_found = roll_off_found = sat_nid_found = 
	num_frame_found = num_atm_found = num_req_slot_found = num_data_slot_found = 
	coding_found = mod_found = fca_flag_found = pre_len_found = bst_start_found = false;

	// Parse one line each time.
	while(fgets(line, MAX_LINE_SIZE, config_file))
	{
		if(sscanf(line, " SatNodeId: %u",
		&(ncc_config.sat_node_id)))
		{
			if(sat_nid_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				sat_nid_found = true;
			}
		}
		else if(sscanf(line, " MaxSuperframeID: %hu",
		&(ncc_config.max_superframe_id)))
		{
			if(max_sid_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				max_sid_found = true;
			}
		}
		else if(sscanf(line, " MinFrequency: %u",
		&(ncc_config.min_frequency)))
		{
			if(cen_freq_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				cen_freq_found = true;
			}
		}
		else if(sscanf(line, " SymbolRate: %llu", &symbol_rate))
		{
			ncc_config.symbol_rate = symbol_rate;
			if(sbl_rate_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				sbl_rate_found = true;
			}
		}
		else if(sscanf(line, " RollOffFactor: %lf",
		&(ncc_config.roll_off_factor)))
		{
			if(roll_off_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				roll_off_found = true;
			}
		}
		else if(sscanf(line, " NumOfATMPerSlot: %hu",
		&(ncc_config.num_of_atm_per_slot)))
		{
			if(num_atm_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				num_atm_found = true;
			}
		}
		else if(sscanf(line, " NumOfDataSlotPerFrame: %hu",
		&(ncc_config.num_of_data_slot_per_frame)))
		{
			if(num_data_slot_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				num_data_slot_found = true;
			}
		}
		else if(sscanf(line, " NumOfRequestSlotPerFrame: %hu",
		&(ncc_config.num_of_req_slot_per_frame)))
		{
			if(num_req_slot_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				num_req_slot_found = true;
			}
		}
		else if(sscanf(line, " NumOfFramePerSuperframe: %hu",
		&(ncc_config.num_of_frame_per_superframe)))
		{
			if(num_frame_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				num_frame_found = true;
			}
		}
		else if(sscanf(line, " PreambleLength: %hhu",
		&(ncc_config.preamble_length)))
		{
			if(pre_len_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				pre_len_found = true;
			}
		}
		else if(sscanf(line, " BurstStartSymbol: %hu",
		&(ncc_config.burst_start_symbol_len)))
		{
			if(bst_start_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				bst_start_found = true;
			}
		}
		else if(sscanf(line, " FCA: %s", buf))
		{
			// Deal with FCA on/off flag.
			if(fca_flag_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
			else
			{
				if(!strcmp(buf, "ON"))
					ncc_config.fca_turned_on = true;
				else if(!strcmp(buf, "OFF"))
					ncc_config.fca_turned_on = false;
				else
				{
					printf("Unknown FCA on/off flag\n");
					assert(0);
				}
				fca_flag_found = true;
			}
		}
		else if(sscanf(line, " Grouping: %s", buf))
		{
			uint8_t				group_id;
			uint16_t			logon_id;
			uint32_t			node_id;
			u_char				rcst_mac[6];
			uint8_t				superframe_id_for_transmission;
			char*				tok;
			list<Rcst_info>::iterator	it;
			Rcst_info			rcst_info;

			tok = strtok(buf, "_");
			assert(sscanf(tok, "%u", &node_id));

			tok = strtok(NULL, "_");
			str_to_macaddr(tok, rcst_mac);

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%hu", &logon_id));

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%hhu", &group_id));

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%hhu",
			&superframe_id_for_transmission));


			list<Rcst_info>& ref_list = rcst_infos.info_list;

			for(it=ref_list.begin();it!=ref_list.end();it++)
			{
				if(it->rcst_id.logon_id==logon_id ||
				   it->node_id==node_id)
				{
					printf("NCC configuration file format error!!\n");
					assert(0);
				}
			}

			// Construct one new rcst info entry;insert it onto
			// the rcst info list.
			rcst_info.node_id = node_id;
			rcst_info.rcst_id.group_id = group_id;
			rcst_info.rcst_id.logon_id = logon_id;
			rcst_info.rcst_state = RCST_STATE_FINE_SYNC;
			memcpy(rcst_info.rcst_mac, rcst_mac, 6);
			rcst_info.superframe_id_for_transmission =
			superframe_id_for_transmission;
			ref_list.push_back(rcst_info);
		}
		else if(sscanf(line, " Capacity: %s", buf))
		{
			uint32_t			node_id;
			uint32_t			cra_level; // Unit--> bits/s.
			uint32_t			vbdc_max_rate; // Unit--> bits/s.
			uint32_t			rbdc_max; // Unit--> bits/s.
			uint16_t			rbdc_timeout; // Unit--> superframes.
			char*				tok;
			list<Rcst_info>::iterator	it;


			tok = strtok(buf, "_");
			assert(sscanf(tok, "%u", &node_id));

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%u", &cra_level));

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%u", &vbdc_max_rate));

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%u", &rbdc_max));

			tok = strtok(NULL, "_");
			assert(sscanf(tok, "%hu", &rbdc_timeout));


			list<Rcst_info>&	ref_list = rcst_infos.info_list;

			bool	info_found = false;

			for(it=ref_list.begin();
			    (it!=ref_list.end()) && !info_found;
			    it++)
			{
				if(it->node_id==node_id)
				{
					//rcst info found;fill it out.
					it->cra_level = cra_level;
					it->vbdc_max_rate = vbdc_max_rate;
					it->rbdc_max = rbdc_max;
					it->rbdc_timeout = rbdc_timeout;
					info_found = true;
				}
			}

			//no match.
			if(!info_found)
			{
				printf("NCC configuration file format error!!\n");
				assert(0);
			}
		}
		else if(sscanf(line, " #%s", buf))
		{
			// This is a comment line. Skip it.
			continue;
		}
		else
		{
			printf("NCC configuration file format error!!\n");
			assert(0);
		}
	} //End of while.
	fclose(config_file);


	assert(max_sid_found && cen_freq_found && sbl_rate_found &&
	       roll_off_found && num_atm_found && num_data_slot_found &&
	       num_req_slot_found && num_frame_found && fca_flag_found &&
	       pre_len_found && bst_start_found && sat_nid_found);


	modulation_rate = ncc_config.modulation_rate;

	preamble_len_in_bit = ncc_config.preamble_length * modulation_rate;

	ncc_config.num_of_slot_per_frame = (ncc_config.num_of_data_slot_per_frame + 
					    ncc_config.num_of_req_slot_per_frame);

	// Compute timeslot_duration.
	const uint64_t guard_time = 2 * ncc_config.burst_start_offset();

	symbol_rate = ncc_config.symbol_rate;


	const double base_time = (double)(((ATM_CELL_SIZE * ncc_config.num_of_atm_per_slot + RS_OVERHEAD) * 
					   8 * INVERSE_CC_RATE) / modulation_rate + ncc_config.preamble_length) / symbol_rate;

	uint64_t tick;
	SEC_TO_TICK(tick, base_time);
	ncc_config.timeslot_duration = tick + guard_time;

	// Compute frame_duration.
	ncc_config.frame_duration = ncc_config.num_of_slot_per_frame * ncc_config.timeslot_duration;

	// Compute superframe_duration.
	ncc_config.superframe_duration = ncc_config.num_of_frame_per_superframe * ncc_config.frame_duration;

	// Update vbdc_max.
	list<Rcst_info>&	ref_list = rcst_infos.info_list;
	list<Rcst_info>::iterator	it;
	uint16_t			vbdc_max; // Unit--> timeslots/frame.
	uint32_t			vbdc_max_rate;

	for(it=ref_list.begin();it!=ref_list.end();it++)
	{
		vbdc_max_rate = it->vbdc_max_rate;

		vbdc_max = ncc_config.bps_to_spf(vbdc_max_rate);

		it->vbdc_max = vbdc_max;
	}

	printf("Parse ncc_config...OK\n");
	return 0;
}
