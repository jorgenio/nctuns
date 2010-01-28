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

#include <satellite/dvb_rcs/common/ncc_config.h>
#include <satellite/dvb_rcs/common/pcr.h>

uint32_t
Ncc_config::bandwidth_per_superframe()
{
	return (uint32_t) ceil(symbol_rate*(1+roll_off_factor));
}

uint64_t
Ncc_config::burst_start_offset()
{
	uint64_t	tick;


	SEC_TO_TICK(tick, ((double)burst_start_symbol_len / symbol_rate));
	return (tick);
}


/************************************************************
  spf_to_bps() returns bit rate achieved corresponding to
  timeslots of 'spf' in a frame.
************************************************************/
uint32_t		
Ncc_config::spf_to_bps(uint32_t spf)
{
	uint32_t	data_size_in_each_slot;	//in bit.


	const uint16_t	NUM_BIT_PER_ATM_CELL = 48 * 8;

	data_size_in_each_slot = num_of_atm_per_slot * NUM_BIT_PER_ATM_CELL;

	return (uint32_t) floor(data_size_in_each_slot * spf * 1000000000.0 / 
				(frame_duration * TICK));
}



/************************************************************
  atm_to_bps() returns bit rate achieved corresponding to
  ATM cells of 'amount_atm_cell' in a frame.
************************************************************/
uint32_t		
Ncc_config::atm_to_bps(uint32_t amount_atm_cell)
{
	const uint16_t	NUM_BIT_PER_ATM_CELL = 48 * 8;

	return (uint32_t) floor(NUM_BIT_PER_ATM_CELL * amount_atm_cell * 1000000000.0 / 
				(frame_duration * TICK));
}

/************************************************************
  bps_to_spf() returns how many slots (per frame) is needed to
  achieve bit rate 'bps'. 
************************************************************/
uint32_t
Ncc_config::bps_to_spf(uint32_t bps)
{
	double		rate_per_slot;  //in bps.


	const uint16_t	NUM_BIT_PER_ATM_CELL = 48 * 8;

	const uint32_t data_size_in_each_slot = (num_of_atm_per_slot * 
						 NUM_BIT_PER_ATM_CELL); // in bits.

	rate_per_slot = (data_size_in_each_slot * 1000000000.0 / 
			 (frame_duration * TICK));

	return (uint32_t) floor((bps / rate_per_slot));
}


/************************************************************
 * channel_capacity() computes the return channel capacity
 * (in bits/s) of each channel or superframe.
************************************************************/
uint32_t
Ncc_config::channel_capacity()
{
	return spf_to_bps(num_of_data_slot_per_frame);
}

uint32_t		
Ncc_config::channel_centre_frequency(uint16_t superframe_id)
{
	return (superframe_id * bandwidth_per_superframe() +
		min_frequency);
}


Frame_ident		
Ncc_config::next_frame(Frame_ident frame)
{
	Frame_ident	next;

	if (frame.frame_number != num_of_frame_per_superframe - 1)
	{
		next.superframe_count = frame.superframe_count;
		next.frame_number = frame.frame_number + 1;
	}
	else // Input is the last frame.
	{
		next.superframe_count = ((frame.superframe_count == MAX_SUPERFRAME_COUNT) ?
					 0 : frame.superframe_count + 1);
		next.frame_number = 0;
	}

	return (next);
}

Frame_ident		
Ncc_config::frame_add(const Frame_ident& frame, uint32_t num)
{
	Frame_ident	out = frame;

	for (uint32_t i=0; i<num; i++)
	{
		out = next_frame(out);
	}

	return (out);
}


/************************************************************
 * Return the different between f1 and f2(f1-f2).
 ***********************************************************/
uint32_t		
Ncc_config::frame_minus(const Frame_ident& f1, const Frame_ident& f2)
{
	uint32_t	diff = 0;
	Frame_ident	tmp = f2;

	while (!(tmp==f1))
	{
		diff++;
		tmp = frame_add(tmp, 1);
	}

	return (diff);
}


bool operator== (const Frame_ident& f1, const Frame_ident& f2)
{
	return (f1.frame_number==f2.frame_number) &&
		(f1.superframe_count==f2.superframe_count);
}

/* slot_time_trans.cc */
/************************************************************
  superframe_start_in_sec() find out the start time of
  the closest superframe S of superframe counter 
  'superframe_count'.
************************************************************/
uint64_t
Ncc_config::superframe_start(uint64_t ref_time, uint16_t superframe_count)
{
	uint64_t	start;
	uint32_t	i;
	const uint64_t	superframe_wrap_around = ((MAX_SUPERFRAME_COUNT+1) * 
						  superframe_duration);

	start = superframe_count * superframe_duration;

	if (ref_time > start )
	{
		i = (uint32_t) floor ((ref_time - start) / superframe_wrap_around);

		start += i * superframe_wrap_around;

		if ((ref_time - start) > (superframe_wrap_around / 2))
		{
			start += superframe_wrap_around;
		}
	}

	return (start);
}


/************************************************************
  frame_start_in_sec() find out the start time of
  the closest frame F of the same Frame identifier.
************************************************************/
uint64_t
Ncc_config::frame_start(uint64_t ref_time, uint16_t superframe_count, uint8_t frame_number)
{
	uint64_t	start;
	uint32_t	i;
	const uint64_t	superframe_wrap_around = ((MAX_SUPERFRAME_COUNT+1) * 
						  superframe_duration);
	

	assert (frame_number<num_of_frame_per_superframe);

	start = (superframe_count * superframe_duration) +
		(frame_number * frame_duration);

	if (ref_time > start )
	{
		i = (uint32_t) floor ((ref_time - start) / superframe_wrap_around);

		start += i * superframe_wrap_around;

		if ((ref_time - start) > (superframe_wrap_around / 2))
		{
			start += superframe_wrap_around;
		}
	}
	return (start);
}

/************************************************************
  timeslot_start_in_sec() find out the start time of
  the closest timeslot T of the same timeslot identifier.
************************************************************/
uint64_t
Ncc_config::timeslot_start(uint64_t ref_time, uint16_t superframe_count, 
			   uint8_t frame_number, uint16_t timeslot_number)
{
	uint64_t	start;
	uint32_t	i;
	const uint64_t	superframe_wrap_around = ((MAX_SUPERFRAME_COUNT+1) * 
						  superframe_duration);
	
	assert (frame_number < num_of_frame_per_superframe);
	assert (timeslot_number < num_of_slot_per_frame);

	start = (superframe_count * superframe_duration) +
		(frame_number * frame_duration) +
		(timeslot_number * timeslot_duration);

	if (ref_time > start )
	{
		i = (uint32_t) floor ((ref_time - start) / superframe_wrap_around);

		start += i * superframe_wrap_around;

		if ((ref_time - start) > (superframe_wrap_around / 2))
		{
			start += superframe_wrap_around;
		}
	}

	return (start);
}


Frame_ident	
Ncc_config::current_frame()
{
	uint16_t	superframe_count, timeslot_number;
	uint8_t		frame_number;


	current_slot(GetCurrentTime(), superframe_count,
		     frame_number, timeslot_number);

	return (Frame_ident(superframe_count, frame_number));
}
/************************************************************
* current_slot() return the slot s which satisfies:
  s begins at t0, and ends at t1 --> t0 < current_time <= t1.
************************************************************/
void
Ncc_config::current_slot(uint64_t current_time, uint16_t &superframe_count, 
			 uint8_t &frame_number, uint16_t &timeslot_number)
{
	uint64_t	time_offset;
	const uint64_t	superframe_wrap_around = ((MAX_SUPERFRAME_COUNT+1) * 
						  superframe_duration);
	

	if(current_time == 0)
	{
		printf("\e[36mWarning: current_slot() is called at tick 0\e[m\n");
		superframe_count = MAX_SUPERFRAME_COUNT;
		frame_number = num_of_frame_per_superframe - 1;
		timeslot_number = num_of_slot_per_frame - 1;
		return;
	}

	time_offset = current_time % superframe_wrap_around;

	// Compute superframe_count.
	superframe_count = time_offset / superframe_duration;
	// handle boundary condition.
	time_offset = time_offset % superframe_duration;
	if(time_offset == 0) // at boundary.
	{
		if(superframe_count!=0)
			superframe_count--;
		else
			superframe_count = MAX_SUPERFRAME_COUNT;
	}

	// Compute frame_number.
	frame_number = time_offset / frame_duration;
	// handle boundary condition.
	time_offset = time_offset % frame_duration;
	if(time_offset == 0) // at boundary.
	{
		if(frame_number!=0)
			frame_number--;
		else
			frame_number = num_of_frame_per_superframe - 1;
	}
	
	// Compute timeslot_number.
	timeslot_number = time_offset / timeslot_duration;
	// handle boundary condition.
	time_offset = time_offset % timeslot_duration;
	if(time_offset == 0) // at boundary.
	{
		if(timeslot_number!=0)
		{
			timeslot_number--;
		}
		else
		{
			timeslot_number = num_of_slot_per_frame - 1;
		}
	}
}


