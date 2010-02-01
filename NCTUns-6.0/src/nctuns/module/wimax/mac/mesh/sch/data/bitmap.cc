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
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "bitmap.h"

/*
* Member function definitions of class `Bitmap'.
*/

/*
* Constructor
*/
Bitmap::Bitmap(uint8_t nf_slot, uint16_t frame_start_max)
: _bitmap(NULL)
, _nf_frame(FRAME_MAX)
, _nf_slot(nf_slot)
, _frame_start_max(frame_start_max)
, _frame_base(0)
, _frame_start(0)
, _frame_end(FRAME_MAX)
, _slot_start(0)
, _slot_end(nf_slot)
, _debug_mode(false)
, _frame_persistent_flag(false)
{
        _bitmap = new bool[_nf_slot * _nf_frame];
        memset(_bitmap, 0, _nf_slot * _nf_frame * sizeof(*_bitmap));
}


/*
* Destructor
*/
Bitmap::~Bitmap()
{
        delete[] _bitmap;
}


/*
* Get the bit value via specified frame and minislot.
* - frame: The frame to get.
* - slot: The minislot to get.
* - return value: The specified bit value.
*/
bool
Bitmap::bit_get(uint16_t frame, uint8_t slot)
{
        return _bitmap[frame * _nf_slot + slot];
}


/*
* Set the bit value via specified frame and minislot.
* - frame_start: The frame start.
* - slot_start: The minislot start.
* - frame_range: The number of frames to set.
* - slot_range: The number of minislots to set.
*/
void
Bitmap::bit_set(uint16_t frame_start, uint8_t slot_start,
                uint16_t frame_range, uint8_t slot_range)
{
        assert(_range_check(frame_start, slot_start, frame_range, slot_range));

        for (uint32_t fr = frame_start ; fr < frame_start + frame_range; fr++) {
                for (uint8_t j = 0 ; j < slot_range ; j++) {
                        _bitmap[fr * _nf_slot + slot_start + j] = true;
                }
        }
}


/*
* Clear the bit value via specified frame and minislot.
* - frame_start: The frame start.
* - slot_start: The minislot start.
* - frame_range: The number of frames to clear.
* - slot_range: The number of minislots to clear.
*/
void
Bitmap::bit_clr(uint16_t frame_start, uint8_t slot_start,
                uint16_t frame_range, uint8_t slot_range)
{
        assert(_range_check(frame_start, slot_start, frame_range, slot_range));

        for (uint32_t fr = frame_start ; fr < frame_start + frame_range; fr++) {
                for (uint8_t j = 0 ; j < slot_range ; j++) {
                        _bitmap[fr * _nf_slot + slot_start + j] = false;
                }
        }
}


/*
* Initialize the bitmap.
* - frame_base: The first frame in the bitmap.
*/
void
Bitmap::init(uint16_t frame_base)
{
        _frame_base = frame_base;
        _frame_start = 0;
        _frame_end = FRAME_MAX;
        _slot_start = 0;
        _slot_end = _nf_slot;
        _debug_mode = false;
        _frame_persistent_flag = false;

        memset(_bitmap, 0, _nf_slot * _nf_frame * sizeof(*_bitmap));
}


/*
* The function cumulate the specified allocation on the bitmap.
* - alloc: The allocation to cumulate.
* - return value: 0
*/
int
Bitmap::cumulate(const Alloc_base& add_alloc)
{
        uint16_t frame_start;
        uint16_t frame_range;

        if (!add_alloc.is_active(_frame_base + FRAME_MAX - 1))
                return 0;

        if (add_alloc.is_active(_frame_base)) {

                if (!add_alloc.frame_remain(_frame_base))
                        return 0;

#if 0
                if (add_alloc.frame_range() == Alloc_base::FRAME_RANGE_PERSISTENCE)
                        printf("%s: \e[1;36mActive, Persistence, %#x\e[m\n",
                                        __FUNCTION__, Alloc_base::FRAME_RANGE_PERSISTENCE);
                else
                        printf("%s: \e[1;36mActive, Not Persistence, %#x\e[m\n",
                                        __FUNCTION__, Alloc_base::FRAME_RANGE_PERSISTENCE);
#endif

                frame_start = 0;
                if (add_alloc.frame_range() == Alloc_base::FRAME_RANGE_PERSISTENCE)
                        frame_range = FRAME_MAX;
                else
                        frame_range = add_alloc.frame_range() -
                                ((_frame_base - add_alloc.frame_start()) &
                                Alloc_base::FRAME_BIT_MASK);
        } else {
                /*
                * Convert absolute frame number to relative frame number.
                */
                frame_start = (add_alloc.frame_start() - _frame_base) &
                        Alloc_base::FRAME_BIT_MASK;
                /*
                * Check if the frame is persistent allocated.
                */
                if (add_alloc.frame_range() ==
                                Alloc_base::FRAME_RANGE_PERSISTENCE)

                        frame_range = FRAME_MAX;
                else
                        frame_range = add_alloc.frame_range();
        }

#if 0
        printf("%s: frame_start = %#x, frame_range = %u, _frame_base = %#x\n",
                        __FUNCTION__, frame_start, frame_range, _frame_base);
        printf("%s: dump alloc:\n", __FUNCTION__);
        add_alloc.dump();
#endif
        bit_set(frame_start, add_alloc.slot_start(),
                frame_range, add_alloc.slot_range());

        if (_debug_mode) {
                set_boundary(frame_start,
                            add_alloc.slot_start(),
                            add_alloc.frame_range(),
                            add_alloc.slot_range());
        }
        return 0;
}

/*
* Find free minislot corresponding to the specified policy, 'frame_len' and 'slot_len'.
* Here, the caller must new the allocation by itself, then we will set the result searched
* to this allocation. Hence, the caller must free the allocation by itself.
* Return value: If finding is successful, then return true;
* 	         otherwise, return flase.
*/
bool
Bitmap::get_free_alloc(class Alloc_base& ret_free_alloc, search_policy_t policy,
                    uint16_t frame_len, uint8_t slot_len, size_t needed_area)
{
        bool	return_flag = false;
        /*
        * The value '0xFFF' indicate the assignment of persistent frame.
        * So, we must find the available frames as possible as we can.
        * Finding '_nf_frame' frames is equal to find the persistent frame.
        * Hence, we must return 'false' when the '_nf_frame' frames can't be found.
        */
        if (frame_len == Alloc_base::FRAME_RANGE_PERSISTENCE) {
                frame_len = _nf_frame;
                _frame_persistent_flag = true;
        }

        switch (policy) {
                case MINISLOT: {
                        return_flag = _prefer_minislot(&ret_free_alloc,
                                        frame_len, slot_len, needed_area);
                        break;
                }
                case FRAME: {
                        return_flag = _prefer_frame(&ret_free_alloc,
                                        frame_len, slot_len, needed_area);
                        break;
                }
                default:
                        /*
                        * Undefined policy.
                        */
                        assert(0);
        };

        if (_frame_persistent_flag)
                _frame_persistent_flag = false;

        return return_flag;
}

/*
 * C.C. Lin: Refined minislot allocation algorithm with the dynamic programming technique.
 * modified date: Feb. 14, 2008. Happy Valentine's Day.
 */
bool Bitmap::_prefer_minislot(Alloc_base* ret_free_alloc, uint16_t frame_len,
                uint8_t slot_len, size_t needed_area) {

    uint8_t  determined_slot_start  = 0;
    uint8_t  determined_slot_end    = 0;

    uint32_t determined_frame_start = 0;
    uint32_t determined_frame_end   = 0;

    uint8_t  determined_max_slot_size  = 0;
    uint32_t determined_max_frame_len = 0;
    uint32_t target_total_minislot_num =  slot_len * frame_len;
    uint32_t computed_total_minislot_num = 0;

#define FIND_THE_BEST_MINISLOT_ALLOC_POLICY 1
#if FIND_THE_BEST_MINISLOT_ALLOC_POLICY
    const uint8_t given_slot_len_lower_bound = 0;
#else
    const uint8_t given_slot_len_lower_bound = slot_len;
#endif


    //printf("Print Bitmap graphically:\n\n");
    //dump();

   assert(target_total_minislot_num);

    for (uint8_t given_slot_len = slot_len; given_slot_len > given_slot_len_lower_bound; --given_slot_len) {

        if (given_slot_len == determined_max_slot_size)
            continue; /* it does not need to do redundant work */

       if (computed_total_minislot_num == target_total_minislot_num)
           break;

        uint8_t             max_slot_len = 0;
        uint32_t	max_frame_len = 0;
        bool		upto_slot_len_flag = false;

        /*
         * C.C. Lin: In the first step, this function tries finding the maximum length of
         * consecutive available mini-slots.
         * (max_slot_len must be less or equal then slot_len).
         */

        for (uint32_t fr = _frame_start; fr < _frame_end; fr++) {


                /* C.C. Lin: Added this checking on Feb. 13 to increase the efficiency
                 * of scheduling data mini-slot allocations, according to the bug
                 * reported by Janice Lau.
                 */

                if (!_frame_persistent_flag && fr >= _frame_start_max) {
                        //printf("(1) fr: %d, _fr_start_max: %d \n", fr, _frame_start_max);
                        break;
                }

                uint8_t slot_cnt = 0;
                for (uint8_t slot = _slot_start; slot < _slot_end; slot++) {

                        if (!bit_get(fr, slot))
                                slot_cnt++;
                        else
                                slot_cnt = 0;

                        if (slot_cnt > max_slot_len) {
                                max_slot_len = slot_cnt;

                                if (max_slot_len == given_slot_len) {
                                        upto_slot_len_flag = true;
                                        //printf("(1) slot_start: %d, slot_end: %d, fr: %d \n", slot - slot_cnt + 1, slot, fr);
                                        break;
                                }
                        }
                }

                if (upto_slot_len_flag)
                        break;
        }

        //printf("(1-final): max_slot_len is %d\n", max_slot_len);

        if (max_slot_len == 0) {
                printf("\e[1;31mNo mini-slots are unused now\e[m\n");
                return false;
        }

        bool upto_frame_len_flag = false;

        /*
         * C.C. Lin: In the second step, this function tries finding the maximum length of
         * consecutive frames that possese the found number of available mini-slots in the
         * first step.
         */ 

        uint32_t fr_final = 0;
        for (uint32_t fr = _frame_start; fr < _frame_end; fr++) {

                /* C.C. Lin: Added this checking on Feb. 13 to increase the efficiency
                 * of scheduling data mini-slot allocations, according to the bug
                 * reported by Janice Lau.
                 */

                if (!_frame_persistent_flag && fr >= _frame_start_max) {
                        //printf("(2) fr: %d, _fr_start_max: %d \n", fr, _frame_start_max);
                        break;
                }

                uint8_t	slot_cnt = 0;

                for (uint8_t slot = _slot_start; slot < _slot_end; slot++) {

                    /* C.C. Lin: Find the maximum number of consecutive free mini-slots
                        * in the current frame.
                        */

                    uint32_t frame_cnt = 0;

                    if (!bit_get(fr, slot)) {

                        if (slot_cnt < max_slot_len)
                                slot_cnt++;

                    }
                    else
                        slot_cnt = 0;

                    if (slot_cnt == max_slot_len) {

                        frame_cnt++;

                        if (frame_cnt > max_frame_len) {

                            max_frame_len = frame_cnt;
                            fr_final = fr;

                            /* C.C. Lin: Update the global decision information */

                            uint32_t total_ms_fr_product = slot_cnt * frame_cnt;
                            if ( total_ms_fr_product > computed_total_minislot_num) {

                                determined_slot_start = slot - slot_cnt + 1;
                                determined_slot_end   = slot;


                                determined_frame_start = fr;
                                determined_frame_end   = fr + frame_cnt - 1;

                                determined_max_slot_size  = slot_cnt;
                                determined_max_frame_len  = frame_cnt;
                                computed_total_minislot_num = total_ms_fr_product;

                            }

                            if (max_frame_len >= frame_len) {

                                upto_frame_len_flag = true;
                                //printf("(2-1) found: slot_start: %d, slot_end: %d, fr_start: %d, fr_end: %d \n",
                                //slot - slot_cnt + 1, slot, fr - frame_len + 1, fr);

                                break;
                            }
                        }

                        /* C.C. Lin: Find the maximum number of consecutive frames that owns the same range
                            * of consecutive free mini-slots.
                            */

                        for (uint32_t ext_fr = (fr + 1); ext_fr < _frame_end; ext_fr++) {

                            if (_block_check_all_unused(ext_fr, (slot + 1 - slot_cnt), 1, max_slot_len)) {
                                    frame_cnt++;
                            }
                            else {
                                    frame_cnt = 0;
                                    //printf("(2-2) found: slot_start: %d, slot_end: %d, fr_start: %d, fr_end: %d \n",
                                    //slot - slot_cnt + 1, slot, fr - frame_len + 1, fr);
                                    break;
                            }

                            if (frame_cnt > max_frame_len) {

                                max_frame_len = frame_cnt;

                                /* C.C. Lin: Update the global decision information */

                                uint32_t total_ms_fr_product = slot_cnt * frame_cnt;
                                if ( total_ms_fr_product > computed_total_minislot_num) {

                                    determined_slot_start = slot - slot_cnt + 1;
                                    determined_slot_end   = slot;

                                    determined_frame_start = fr;
                                    determined_frame_end   = ext_fr;

                                    //printf("loop-2: slot_len: %u, frame_len: %u, slot_start: %u, frame_start: %u, fr: %u, base: %d, fr_start: %u, fr_end: %u \n", slot_cnt, frame_cnt, determined_slot_start, determined_frame_start, fr, _frame_base, _frame_start, _frame_end);

                                    determined_frame_end      = fr;

                                    determined_max_slot_size  = slot_cnt;
                                    determined_max_frame_len  = frame_cnt;
                                    computed_total_minislot_num = total_ms_fr_product;

                                }

                                if (max_frame_len >= frame_len) {

                                    upto_frame_len_flag = true;

                                    //printf("(2-3) found: slot_start: %d, slot_end: %d, fr_start: %d, fr_end: %d \n",
                                    //        slot - slot_cnt + 1, slot, fr - frame_len + 1, fr);
                                    break;
                                }
                            }
                        }
                    }

                    else if (slot_cnt < max_slot_len) {

                        /* continue the searching process */
                        ;

                    }
                    else {

                        assert(0);

                    }

                    if (upto_frame_len_flag)
                            break;
                }

                if (upto_frame_len_flag)
                        break;
        }

        //printf("(2-final): fr_start: %d, fr_end: %d \n", fr_final - max_frame_len + 1, fr_final);

    }

    /*
     * C.C. Lin: Deal with the case where the allocation of a persistent schedule with the given
     * frame duration cannot be found.
     */
    if (_frame_persistent_flag && (determined_max_frame_len != _nf_frame)) {
            printf("\e[1;31mCannot find a persistent mini-slot allocation with the given frame duration.\e[m\n");
            assert(0);
    }


#if recheck_the_availability_of_the_found_area
    int check_res = _block_check_all_unused(determined_frame_start, determined_slot_start,
                            determined_max_frame_len, determined_max_slot_size);
#else
    /* C.C. Lin: we disable the recheck process to increase the simulation speed. */
    int check_res = 1;
#endif

    if (check_res) {

        #if 0
        printf("Global: slot_len: %u, frame_len: %u, slot_start: %u, frame_start: %u, base: %d \n",
                determined_max_slot_size, determined_max_frame_len, determined_slot_start,
                determined_frame_start + _frame_base, _frame_base);
        #endif

        if (_frame_persistent_flag)
            ret_free_alloc->frame_range(Alloc_base::FRAME_RANGE_PERSISTENCE);
        else
            ret_free_alloc->frame_range(determined_max_frame_len);

        //ret_free_alloc->frame_start(fr + _frame_base);
        ret_free_alloc->frame_start(determined_frame_start + _frame_base);
        ret_free_alloc->slot_start(determined_slot_start);
        ret_free_alloc->slot_range(determined_max_slot_size);

        if (!_frame_persistent_flag && needed_area)
                _adjust_free_alloc_by_needed_area(*ret_free_alloc, needed_area);

        return true;

    }
#if 0
    /*
    * In the third step, we will find the matching blocks based on
    * corresponding max_slot_len and max_frame_len. Finally,
    * we generate the 'Minislot_alloc' for the matching block, then
    * retrun it;
    */

    //printf("Global: slot_len: %d, frame_len: %d \n", determined_max_slot_size, determined_max_frame_len);
    for (uint32_t fr = _frame_start; fr < _frame_end; fr++) {

            if (!_frame_persistent_flag && fr >= _frame_start_max) {

                    printf("\e[1;31m!_frame_persistent_flag && fr >= _frame_start_max.\e[m\n");
                    printf("fr: %d, _fr_start_max: %d \n", fr, _frame_start_max);
                    assert(0);
            }

            uint8_t slot_cnt = 0;
            for (uint8_t slot = _slot_start; slot < _slot_end; slot++) {

                    uint8_t org_slot_start;

                    if (!bit_get(fr, slot)) {

                        if (slot_cnt != determined_max_slot_size) slot_cnt++;
                    }
                    else {
                            slot_cnt = 0;
                    }

                    /*
                    * As soon as find the first matching block, we will set the allocation and return it;
                    */
                    if (slot_cnt == determined_max_slot_size ) {


                        //printf("(3) slot_start: %d, slot_end: %d \n", slot - slot_cnt + 1, slot);

                        org_slot_start = slot - slot_cnt + 1;

                        if (_block_check_all_unused(fr, org_slot_start,
                                determined_max_frame_len, determined_max_slot_size)) {

                            printf("Global: slot_len: %u, frame_len: %u, slot_start: %u, frame_start: %u, fr: %u, base: %d, fr_start: %u, fr_end: %u \n", 
                               determined_max_slot_size, determined_max_frame_len, determined_slot_start,
                               determined_frame_start, fr, _frame_base, _frame_start, _frame_end);

                            printf("Found: slot_len: %u, frame_len: %u, slot_start: %u, frame_start: %u, fr: %u, base: %d \n",
                               slot_cnt, determined_max_frame_len, org_slot_start,
                               fr + _frame_base, fr, _frame_base);


                            if (_frame_persistent_flag)
                                    ret_free_alloc->frame_range(Alloc_base::FRAME_RANGE_PERSISTENCE);
                            else
                                    ret_free_alloc->frame_range(determined_max_frame_len);

                            ret_free_alloc->frame_start(fr + _frame_base);
                            ret_free_alloc->slot_start(org_slot_start);
                            ret_free_alloc->slot_range(determined_max_slot_size);

                            if (!_frame_persistent_flag && needed_area)
                                    _adjust_free_alloc_by_needed_area(*ret_free_alloc, needed_area);


                            return true;
                        }
                    }

            }

    }

#endif

    assert(0);
}

/*
* TODO: Function description.
*/
bool
Bitmap::_prefer_frame(Alloc_base* ret_free_alloc,
                uint16_t frame_len, uint8_t slot_len,
                size_t needed_area)
{
        return true;
}


/*
* Check if the specified range is legal one.
* - frame_start: The frame start.
* - slot_start: The minislot start.
* - frame_range: The number of frames.
* - slot_range: The number of minislots.
*/
bool
Bitmap::_range_check(uint16_t frame_start, uint8_t slot_start,
                        uint16_t& frame_range, uint8_t& slot_range)
{
        /*
        * Check if the frame start is too large.
        */
        if (frame_start >= Alloc_base::FRAME_START_MAX) {
                fprintf(stderr, "\e[1;31m[Warning] %s: Illegal frame: "
                        "frame_start = %#03x\e[m\n", __FUNCTION__, frame_start);
                return false;
        }

        /*
        * Adjust the persistence if it's needed.
        */
        if (frame_range != Alloc_base::FRAME_RANGE_PERSISTENCE &&
                        frame_start + frame_range > _nf_frame) {

                fprintf(stderr, "\e[1;31m[Warning] %s: Illegal frame range: "
                        "frame_start = %#03x, frame_range = %#03x\e[m\n",
                        __FUNCTION__, frame_start, frame_range);
                return false;
        }

        /*
        * Check if the slot start is too large.
        */
        if (slot_start >= _nf_slot) {
                fprintf(stderr, "\e[1;31m[Warning] %s: Illegal slot start: "
                        "slot_start = %u\e[m\n", __FUNCTION__, slot_start);
                return false;
        }

        /*
        * Adjust the slot range if it's needed.
        */
        if (slot_start + slot_range > _nf_slot)
                slot_range = _nf_slot - slot_start;

        return true;
}


#define SCREEN_WIDTH	80
#define FRAME_GRP	10
/*
* Show the bitmap graphically.
*/
void
Bitmap::dump()
{
        printf("\e[1;31mDump bitmap:\e[m\n");
        _dump_bounary_info();

//	for (unsigned int left = 0 ; left < _nf_frame ; left += SCREEN_WIDTH) {
        for (unsigned int left = _frame_start - (_frame_start % SCREEN_WIDTH);
                        left < _frame_end ; left += SCREEN_WIDTH) {
                unsigned int right;
                if (_nf_frame < left + SCREEN_WIDTH)
                        right = _nf_frame;
                else
                        right = left + SCREEN_WIDTH;

                /*
                * Print title.
                */
                printf("Absolute ");
                for (unsigned int fr = left ; fr < right ; fr += FRAME_GRP) {
                        if (_nf_frame < fr + FRAME_GRP)
                                printf("Fr %03x-%03x ",
                                                (_frame_base + fr)
                                                & Alloc_base::FRAME_BIT_MASK,
                                                (_frame_base + _nf_frame - 1)
                                                & Alloc_base::FRAME_BIT_MASK);
                        else
                                printf("Fr %03x-%03x ", (_frame_base + fr)
                                                & Alloc_base::FRAME_BIT_MASK,
                                                (_frame_base + fr + FRAME_GRP - 1)
                                                & Alloc_base::FRAME_BIT_MASK);
                }
                printf("\nMinislot ");
                for (unsigned int fr = left ; fr < right ; fr += FRAME_GRP) {
                        if (_nf_frame < fr + FRAME_GRP)
                                printf("Fr %03x-%03x ", fr, _nf_frame - 1);
                        else
                                printf("Fr %03x-%03x ", fr, fr + FRAME_GRP - 1);
                }
                printf("\n");

                /*
                * Print bitmap.
                */
//		for (unsigned int i = 0 ; i < _nf_slot ; i++) {
                for (unsigned int i = _slot_start ; i < _slot_end ; i++) {
                        printf("%8d", i);
                        for (unsigned int j = left ; j < right ; j++) {
                                if (j % FRAME_GRP == 0)
                                        putc(' ', stdout);

                                if (_debug_mode) {

                                        if ((i >= _slot_start && i < _slot_end) &&
                                            (j >= _frame_start && j < _frame_end))

                                                printf("\e[1;35m%c\e[m", bit_get(j, i)?'1':'0');
                                        else
                                                ;//putchar(bit_get(j, i)?'1':'0');

                                } else
                                        putchar(bit_get(j, i)?'1':'0');
                        }
                        putchar('\n');
                }
                putchar('\n');
        }
}

/*
* Check some block of the bitmap whether used or not.
* If all of the bits are unused, then return true; otherwise, return false.
*/
bool
Bitmap::_block_check_all_unused(uint32_t frame_start, uint8_t slot_start,
                                uint32_t frame_range, uint8_t slot_range)
{
        for (uint32_t fr = frame_start; fr < (frame_start + frame_range); fr++) {
                for (uint8_t slot = slot_start; slot < (slot_start + slot_range); slot++) {

                        if (bit_get(fr, slot))
                                return false;
                }
        }
        return true;
}

/*
* Set the boundary searched of the bitmap.
* Return value: If the setting success, then return true;
*		 otherwise, then return false;
*/
bool
Bitmap::set_boundary(uint16_t frame_start, uint8_t slot_start,
                        uint16_t frame_range, uint8_t slot_range)
{
#if 0
        printf("%s \e[1;36m=============> "
                        "frame_start = %#x, "
                        "frame_range = %#x, "
                        "slot_start = %#x, "
                        "slot_range = %#x\e[m\n",
                        __FUNCTION__,
                        frame_start,
                        frame_range,
                        slot_start,
                        slot_range);
#endif
        if (!frame_start && !slot_start && !frame_range && !slot_range) {
                /*
                * (0, 0, 0, 0) indicate no boundary limit.
                */
                _frame_end = _nf_frame;
                _slot_end = _nf_slot;
        } else if (!slot_start && !frame_range && !slot_range) {
                /*
                * (frame_start, 0, 0, 0) indicates that we set the boundary
                * of the starting frame only, but other boundaries are not limited.
                */
                uint16_t frame_diff = (frame_start - _frame_base) &
                        Alloc_base::FRAME_BIT_MASK;
#if 0
                printf("%s \e[1;36mframe_diff = %#x\e[m\n",
                                __FUNCTION__, frame_diff);
#endif
                /*
                * Convert absolute frame number to relative frame number.
                * If the frame start is less than the frame base,
                * set frame start to zero and adjust the range.
                */
                if (frame_diff < Alloc_base::FRAME_DIFF_MAX)
                        frame_start = frame_diff;
                else
                        frame_start = 0;

                _frame_end = _nf_frame;
                _slot_end = _nf_slot;
        } else {
                /*
                * Convert absolute frame number to relative frame number.
                */
                uint16_t frame_diff = (frame_start - _frame_base) &
                        Alloc_base::FRAME_BIT_MASK;
#if 0
                printf("%s \e[1;36mframe_diff = %#x\e[m\n",
                                __FUNCTION__, frame_diff);
#endif
                /*
                * If the frame start is less than the frame base,
                * set frame start to zero and adjust the range.
                */
                if (frame_diff < Alloc_base::FRAME_DIFF_MAX)
                        frame_start = frame_diff;
                else {
                        frame_diff = (_frame_base - frame_start) &
                                Alloc_base::FRAME_BIT_MASK;
                        frame_start = 0;
#if 0
                        printf("%s \e[1;36mframe_diff = %#x\e[m\n",
                                        __FUNCTION__, frame_diff);
                        printf("%s \e[1;36mframe_range 1 = %#x\e[m\n",
                                        __FUNCTION__, frame_range);
#endif
                        if (frame_range !=
                                        Alloc_base::FRAME_RANGE_PERSISTENCE) {
                                if (frame_range < frame_diff)
                                        frame_range = 0;
                                else
                                        frame_range -= frame_diff;
                        }
#if 0
                        printf("%s \e[1;36mframe_range 2 = %#x\e[m\n",
                                        __FUNCTION__, frame_range);
#endif
                }

                assert(_range_check(frame_start, slot_start,
                                        frame_range, slot_range));

                if (frame_range == Alloc_base::FRAME_RANGE_PERSISTENCE)
                        _frame_end = _nf_frame;
                else
                        _frame_end = frame_start + frame_range;

                _slot_end = slot_start + slot_range;
        }

        if (frame_start > _nf_frame || _frame_end  > _nf_frame)
                return false;

        if (slot_start > _nf_slot || _slot_end > _nf_slot)
                return false;

        _frame_start = frame_start;
        _slot_start = slot_start;

        return true;
}

/*
* Change to debug mode.
*/
void
Bitmap::set_debug_mode()
{
        _debug_mode = true;
}

/*
* Dump bounary inforamtion.
*/
void
Bitmap::_dump_bounary_info()
{
        printf("_frame_base is %#x\n", _frame_base);
        printf("_frame_start is %#x\n", _frame_start);
        printf("_slot_start is %u\n", _slot_start);
        printf("_frame_end is %#x\n", _frame_end);
        printf("_slot_end is %u\n", _slot_end);
}

/*
* Dump the inforamtion of incoming frame and slot.
*/
void
Bitmap::_dump_incoming_info(uint32_t frame_start, uint8_t slot_start,
                        uint32_t frame_range, uint8_t slot_range)
{
        printf("frame_start is %u\n", frame_start);
        printf("slot_start is %u\n", slot_start);
        printf("frame_range is %u\n", frame_range);
        printf("slot_range is %u\n\n", slot_range);
}

void
Bitmap::_adjust_free_alloc_by_needed_area(class Alloc_base& ret_free_alloc,
                                        size_t needed_area)
{
        uint16_t filled_frame_quotient;
        uint16_t filled_frame_remainder;

        if (ret_free_alloc.area() < needed_area)
                return;

        assert(ret_free_alloc.slot_range());

        filled_frame_quotient = needed_area / ret_free_alloc.slot_range();

        if (!filled_frame_quotient) {
                filled_frame_quotient = 1;
                filled_frame_remainder = needed_area % ret_free_alloc.slot_range();
                ret_free_alloc.slot_range(filled_frame_remainder);
        }

        ret_free_alloc.frame_range(filled_frame_quotient);
}

int
Bitmap::_get_nf_free_slot(uint32_t frame_start, uint8_t slot_start,
                        uint32_t frame_end, uint8_t slot_end)
{
        int nf_free_slot = 0;

        for (uint32_t fr = frame_start; fr < frame_end; fr++) {

                for (uint8_t slot = slot_start; slot < slot_end; slot++) {
                        if (!bit_get(fr, slot)) {
                                nf_free_slot++;
                        }
                }
        }
        return nf_free_slot;
}
