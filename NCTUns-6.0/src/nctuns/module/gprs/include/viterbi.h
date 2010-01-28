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

char next_state[16][16] = {
{	0,	3,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	2,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	0,	3,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	2,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	3,	0,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	1,	2,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	3,	0,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	1,	2},
{	3,	0,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	1,	2,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	3,	0,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	1,	2,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	3,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	2,	1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	3,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	2,	1},
};
char map[16][16] = {
{	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1},
{	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1,	-1,	-1},
{	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	-1,	0,	1},
};
unsigned char CS2_NullSyms[74] = {
0x0,
0x80,0x88,0x88,0x8,0x88,0x88,0x88,0x88,
0x88,0x8,0x88,0x88,0x88,0x88,0x88,0x8,
0x88,0x88,0x88,0x88,0x88,0x8,0x88,0x88,
0x88,0x88,0x88,0x8,0x88,0x88,0x88,0x88,
0x88,0x8,0x88,0x88,0x88,0x88,0x88,0x8,
0x88,0x88,0x88,0x88,0x88,0x8,0x88,0x88,
0x88,0x88,0x88,0x8,0x88,0x88,0x88,0x88,
0x88,0x8,0x88,0x88,0x88,0x88,0x88,0x8,
0x88,0x88,0x88,0x88,0x88,0x8,0x88,0x88,
0x8,};
unsigned char CS3_NullSyms[85] = {
0x0,
0x80,0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,
0x28,0x8a,0xa2,0x28,0x8a,0xa2,0x28,0x8a,
0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,0x28,
0x8a,0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,
0x28,0x8a,0xa2,0x28,0x8a,0xa2,0x28,0x8a,
0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,0x28,
0x8a,0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,
0x28,0x8a,0xa2,0x28,0x8a,0xa2,0x28,0x8a,
0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,0x28,
0x8a,0xa2,0x28,0x8a,0xa2,0x28,0x8a,0xa2,
0x28,0x8a,0xa2,0x0,};
