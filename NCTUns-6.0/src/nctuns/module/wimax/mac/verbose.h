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

#ifndef __VERBOSE_HEADER__
#define __VERBOSE_HEADER__

#include <assert.h>
#include <stdint.h>


#define MSG_OPEN            8
#define MSG_TEST            7
#define MSG_DEBUG           6
#define MSG_VINFO           5 /* verbose information */
#define MSG_INFO            4
#define MSG_WARNING         3
#define MSG_ERROR           2
#define MSG_FATAL           1
#define MSG_OFF             0

#define DEFAULT_LEVEL   MSG_ERROR

#ifndef VERBOSE_LEVEL
#define VERBOSE_LEVEL   DEFAULT_LEVEL
#endif


#define xprintf(lv, ...)                                    \
if (lv <= VERBOSE_LEVEL) {                                  \
    extern uint64_t*    currentTime_;                       \
    extern int          TICK;                               \
    printf("%11.7f: ", currentTime_?                        \
            (double)currentTime_[0] * TICK / 1000000000:0); \
    printf(__VA_ARGS__);                                    \
}

#define obj_xprintf(lv, ...)    \
if (lv <= VERBOSE_LEVEL) {      \
    NSLOBJ_DEBUG_STRING_HEAD(); \
    printf(__VA_ARGS__);        \
}

#define X_FUNC(lv, x)		\
if (lv <= VERBOSE_LEVEL) {	\
	x;			\
}


#define TEST(...)           xprintf(MSG_TEST, __VA_ARGS__)
#define DEBUG(...)          xprintf(MSG_DEBUG, __VA_ARGS__)
#define VINFO(...)          xprintf(MSG_VINFO, __VA_ARGS__)
#define INFO(...)           xprintf(MSG_INFO, __VA_ARGS__)
#define WARN(...)           xprintf(MSG_WARNING, __VA_ARGS__)
#define ERROR(...)          xprintf(MSG_ERROR, __VA_ARGS__)
#define FATAL(...)          ASSERT(0, __VA_ARGS__);
#define STAT(...)           xprintf(MSG_OFF, __VA_ARGS__)

#define TEST_FUNC(x)        X_FUNC(MSG_TEST, x)
#define DEBUG_FUNC(x)       X_FUNC(MSG_DEBUG, x)
#define VINFO_FUNC(x)       X_FUNC(MSG_VINFO, x)
#define INFO_FUNC(x)        X_FUNC(MSG_INFO, x)
#define WARN_FUNC(x)        X_FUNC(MSG_WARNING, x)
#define ERROR_FUNC(x)       X_FUNC(MSG_ERROR, x)
#define STAT_FUNC(x)        X_FUNC(MSG_OFF, x)

#define NSLOBJ_TEST(...)    obj_xprintf(MSG_TEST, __VA_ARGS__)
#define NSLOBJ_DEBUG(...)   obj_xprintf(MSG_DEBUG, __VA_ARGS__)
#define NSLOBJ_VINFO(...)   obj_xprintf(MSG_VINFO, __VA_ARGS__)
#define NSLOBJ_INFO(...)    obj_xprintf(MSG_INFO, __VA_ARGS__)
#define NSLOBJ_WARN(...)    obj_xprintf(MSG_WARNING, __VA_ARGS__)
#define NSLOBJ_ERROR(...)   obj_xprintf(MSG_ERROR, __VA_ARGS__)
#define NSLOBJ_FATAL(...)   NSLOBJ_ASSERT(0, __VA_ARGS__)
#define NSLOBJ_STAT(...)    obj_xprintf(MSG_OFF, __VA_ARGS__)


#define NSLOBJ_ASSERT(exp, format, ...) do {    \
    if ( !(exp) ) {                             \
        NSLOBJ_DEBUG_STRING_HEAD();             \
        printf("<Assertion failed>" );          \
        fprintf(stderr, format,## __VA_ARGS__); \
        assert(0);                              \
    }                                           \
} while(0)


#define ASSERT(exp, format, ...) do {           \
    if ( !(exp) ) {                             \
        printf("<Assertion failed>" );          \
        fprintf(stderr, format,## __VA_ARGS__); \
        assert(0);                              \
    }                                           \
} while(0)


#define IS_NULL_STR(ep, str, ret_val) do {      \
        if ( !(ep) ) {                          \
                printf("%s\n",(str));           \
                return (ret_val);               \
            }                                   \
    } while(0)

#define IS_NULL(ep, ret_val) do {     \
            if ( !(ep) ) {            \
                return (ret_val);     \
            }                         \
    } while(0)


#endif /* __VERBOSE_HEADER__ */
