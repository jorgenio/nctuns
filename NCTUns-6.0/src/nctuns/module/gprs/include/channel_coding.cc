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
#include <string.h>
#include <sys/types.h>
#include "viterbi.h"
#include <gprs/rlc/rlc_shared_def.h>
/* constraint length */
#define K 5
#define STATENO (1 << (K-1))
/* polynomial generators */
unsigned int Polys[] = {0x19,0x1b};

/* convolutional encoder */
unsigned char cvencode(unsigned char* data, int nbits,unsigned char* symbol){
	unsigned char input , output0 , output1 , enp0 , enp1 ;
	input = output0 = output1 = enp0 = enp1 = 0;
	unsigned char state ;
	state = 0;
	int n = 0;
	
	while( n < nbits){
		input = data[n/8];
		output0 = output1 = 0;
		for(int i = 0; i < (nbits - n >8? 8: nbits - n); i++){
			enp0 = enp1 = 0;
			state = (state << 1) | ((input >> i) & 1);
			for (int j = 0; j < K; j++){
				enp0 ^= ((Polys[0] & state) >> j) & 0x1;
				enp1 ^= ((Polys[1] & state) >> j) & 0x1;
			}

			if ( i < 4){
				output0 |= enp0 << (2*i);
				output0 |= enp1 << (2*i+1);
			}else{
				output1 |= enp0 << (2*i-8);
				output1 |= enp1 << (2*i-7);
			}
		}
		symbol[(n)/4] = output0 ;
		if ( nbits - n > 4)symbol[(n)/4+1] = output1;
		n += 8;
	}
	return state;
}

inline unsigned char HammingDist(unsigned char a, unsigned char b){
	unsigned char ret = 0;
	unsigned char x = a ^ b;
	while ( x > 0){
		if (x & 1)ret++;
		x = x >> 1;
	}
	return ret;
}

/* viterbi decoder */
int viterbi(unsigned char* symbol, int nbits,unsigned char* nullsym, unsigned char* data){
	unsigned char* metric[STATENO];
	unsigned char* survivor[STATENO];
	unsigned char* selects;
	int n = 0;//,m = n/(K-1);

	for (int i = 0; i < STATENO; i++){
		metric[i] = new unsigned char[nbits/2 + 1];
		survivor[i] = new unsigned char[nbits/2 + 1];
		for (int j = 0; j < nbits/2+1; j++){
			metric[i][j] = 0xff;
			survivor[i][j] = 0xff;
		}
	}

	metric[0][0] = 0;
	
	while (n < nbits){
		unsigned char sym = (symbol[n/8] >> (n % 8))& 0x3;
		unsigned char nsym = !nullsym?0:(nullsym[n/8] >> (n % 8))& 0x3;
		for (int i = 0; i < STATENO; i++){
			if (metric[i][n/2] != 0xff){
				for (int j = 0; j < STATENO; j++){
					if (next_state[i][j] >= 0){
						unsigned char newdist = metric[i][n/2] + HammingDist(sym,next_state[i][j]);
						unsigned char newdist2;
						if (nsym){
							newdist2 = metric[i][n/2] + HammingDist(sym ^ nsym,next_state[i][j]);
							if ( newdist2 < newdist ) {
								sym ^= nsym;
								newdist = newdist2;
							}
						}
						if (metric[j][n/2+1] > newdist)
							metric[j][n/2+1] = newdist;
					}
				}
			}
		}
		n += 2;
	}

	
	for (int i = nbits/2; i > 0; i--){
		for (int j = 0; j < STATENO; j++){
			if (metric[j][i] != 0xff){
				unsigned char min = 0xff;
				for (int k = 0; k < STATENO; k++){
					if (next_state[k][j] >= 0 && metric[k][i-1] < min){
						min = metric[k][i-1];
						survivor[j][i] = k;
					}
					
				}
			}
		}
	}

	unsigned int min = 0xff;
	n = STATENO;
	for ( int i = 0; i < STATENO; i++){
		if (metric[i][nbits/2] != 0xff && metric[i][nbits/2] < min){
			n = i;
			min = metric[i][nbits/2];
		}
	}
	
	if ( n == STATENO) {
		for (int i = 0; i < STATENO; i++){
			delete metric[i];
			delete survivor[i];
		}
		return -1;
	}

	selects = new unsigned char[nbits/2 + 1];
	selects[nbits/2] = n;
	for ( int i = nbits / 2; i > 0; i--){
		selects[i - 1] = survivor[n][i];
		n = survivor[n][i];
	}

	

	bzero(data,sizeof(unsigned char[nbits/2/8]));
	for (int i = 0; i < nbits/2 ; i++){
		data[i/8] |= map[selects[i]][selects[i+1]] << (i % 8);
	}
	
	delete selects;
	for (int i = 0; i < STATENO; i++){
		delete metric[i];
		delete survivor[i];
	}
	return 0;
}

/* interleaving */
int interleaving(unsigned char* input , unsigned char* output){
	for (int i = 0; i < 456; i++){
		int j = i / 8 + (i % 8) *57;
		if ( j < 228) j = 2 * j + 1;
		else j = (j - 228) * 2;
		output[j/8] |= ((input[i/8] >> i%8) & 0x1)  << (j % 8);
	}
	return 0;
}

int deinterleaving(unsigned char* input , unsigned char* output){
	for (int i = 0; i < 456; i++){
		int j;
		if ( i % 2) j = (i - 1) / 2;
		else j = i/2 + 228;

		j = j / 57 + (j % 57) *8;
		output[j/8] |= ((input[i/8] >> i%8) & 0x1)  << (j % 8);
	}
	return 0;
}


/* checksum */

unsigned char FirePoly[] = {0x80,0x04,0x82,0x00,0x09};
int firecode(unsigned char* buf,int nbits,unsigned char *reg){
	int n = 0;
	bzero(reg,sizeof(unsigned char[5]));

	while (n < nbits){
		reg[0] = (reg[0] << 1) | ((reg[1] >> 7) & 0x1);
		reg[1] = (reg[1] << 1) | ((reg[2] >> 7) & 0x1);
		reg[2] = (reg[2] << 1) | ((reg[3] >> 7) & 0x1);
		reg[3] = (reg[3] << 1) | ((reg[4] >> 7) & 0x1);
		reg[4] = (reg[4] << 1) | ((buf[n/8] >> n%8) & 0x1);
		if ( reg[0] &  0x80){
			reg[0] ^= FirePoly[0];
			reg[1] ^= FirePoly[1];
			reg[2] ^= FirePoly[2];
			reg[3] ^= FirePoly[3];
			reg[4] ^= FirePoly[4];
		}
		n++;
	}
	return 0;
}


/*
 * CRC computation logic
 * 
 * The logic for this method of calculating the CRC 16 bit polynomial is taken
 * from an article by David Schwaderer in the April 1985 issue of PC Tech
 * Journal.
 */

static unsigned int      crctab[] =	/* CRC lookup table */
{
 0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241,
 0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440,
 0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40,
 0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841,
 0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40,
 0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41,
 0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641,
 0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040,
 0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240,
 0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441,
 0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41,
 0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840,
 0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41,
 0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40,
 0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640,
 0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041,
 0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240,
 0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441,
 0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41,
 0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840,
 0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41,
 0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40,
 0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640,
 0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041,
 0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241,
 0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440,
 0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40,
 0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841,
 0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40,
 0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41,
 0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641,
 0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

/*
 * Update a CRC check on the given buffer.
 */

unsigned int
crc(unsigned int crc, unsigned int len, unsigned char* buf)
{
	unsigned int	i;

	for (i=0; i<len; i++){
		crc = ((crc >> 8) & 0xff) ^ crctab[(crc ^ *buf++) & 0xff];
	}
	
	return (crc);
}

/*  Packet data traffic channel (PDTCH) */

/* CS-1 */
int CS1_encode(unsigned char* data, unsigned char* symbol){
	unsigned char buf[57];
	unsigned char buf2[57];
	bzero(buf,sizeof(unsigned char[57]));
	bzero(buf2,sizeof(unsigned char[57]));
	bzero(symbol,sizeof(unsigned char [57]));
	memcpy(buf,data,57);
	firecode(data,184,buf+23);
	cvencode(buf,228,buf2);
	interleaving(buf2,symbol);
	return 0;
}

int CS1_decode( unsigned char* symbol, unsigned char* data){
	unsigned char buf[57];
	unsigned char buf2[57];
	bzero(buf,sizeof(unsigned char[57]));
	bzero(buf2,sizeof(unsigned char[57]));
	bzero(data,CS1_BLKSIZE+1);
	deinterleaving(symbol,buf);
	viterbi(buf, 456, NULL, buf2);
	bzero(buf,sizeof(unsigned char[5]));
	memcpy(data,buf2,CS1_BLKSIZE+1);
	firecode(buf2,184,buf);
	if (buf[0]!= buf2[23] || buf[1] != buf2[24] || buf[2] != buf2[25] || buf[3] != buf2[26] || buf[4] != buf2[27]) return -1;
	return 0;
}

/* CS-2 */
unsigned char CS2USF[] = {0x0,0x29,0x1a,0x33,0x34,0x1d,0x2e,0x07};
int CS2_encode(unsigned char* data, unsigned char* symbol){
	unsigned char buf[74];
	unsigned char buf2[74];
	unsigned int csum;
	bzero(buf,74);
	bzero(buf2,74);
	bzero(symbol,sizeof(unsigned char [57]));
	memcpy(buf,data,34);
	buf[33] &= 0x7F;

	/* check sum bits */
	csum = crc(0,34,buf);
	buf[33] |= (csum & 1) << 7;
	buf[34] = (csum >> 1) & 0xFF;
	buf[35] = (csum >> 9) & 0x7F;
	//printf (" data[0] = %x \n" , data[0]&0x07);
	/* usf bits */
	for ( int i = 36; i > 0; i--) buf[i] = buf[i] << 3 | buf[i-1] >> 5;
	buf[0] = ((buf[0] << 3) & 0xC0) | CS2USF[data[0] & 0x7];

	/*printf("start\n");
	for (int i=0 ; i<sizeof(buf) ; ++i)
    	printf("buf1[%d] = %d \n", i , buf[i] );
    */
	cvencode(buf,294,buf2);

	/*for (int i=0 ; i<sizeof(buf) ; ++i)
    	printf("buf2[%d] = %d \n", i , buf2[i] );
	printf("end \n");
    */
	/* 
	 * punctured C(3+4j) for j = 3,4,...,146   except ofr j = 9,21,33,45,57,...,141
	 * stupid ways , process bit by bit
	 * */
	bzero(buf,sizeof(unsigned char[74]));
	buf[0] = buf2[0];
	for ( int i = 8, n = 8; i < 588; i++, n++){
		if ( i > 14 && (i % 4 == 3) && (i / 4 % 12 - 9 != 0)){
			i++;
		}
		buf[n/8] |= ((buf2[i/8] >> i % 8) & 0x1) << n % 8;
	}

    /*for (int i=0 ; i<sizeof(buf) ; ++i)
    	printf("buf[%d] = %d ", i , buf[i] );
    */
	interleaving(buf,symbol);
	return 0;
}

int CS2_decode( unsigned char* symbol, unsigned char* data){
	unsigned char buf[74];
	unsigned char buf2[74];
	bzero(buf,sizeof(unsigned char[74]));
	bzero(buf2,sizeof(unsigned char[74]));
	bzero(data,CS2_BLKSIZE+1);

	deinterleaving(symbol,buf);
	
	/* 
	 * restore putctured bits
	 * stupid ways , process bit by bit
	 * */
	buf2[0] = buf[0];
	for ( int i = 8,n = 8; i < 456; i++,n++){
		if ( n > 14 && n % 4 == 3 && (n / 4 - 9)% 12 != 0){
			i--;
		}else{
			buf2[n/8] |= ((buf[i/8] >> i % 8) & 0x1) << n % 8;
		}
	}

	bzero(buf,sizeof(unsigned char[74]));
	viterbi(buf2, 588, CS2_NullSyms,buf);
	
	/* check USF */
	unsigned char usf;
    //printf ("Decode: usf = %x \n", buf[0]& 0x3f);

	for (usf = 0 ; usf < 8; usf++)
		if (CS2USF[usf] == (buf[0] & 0x3F))break;

    //printf ("aaaaaaaaa\n");
	if (usf == 8 ) return -1;


	buf[0] = usf | ((buf[0] >> 3)& 0x18)| (buf[1] << 5);
	for ( int i = 1; i < 73; i++) buf[i] = (buf[i] >> 3) | (buf[i+1] << 5);

	/* CRC Check Sum */
	unsigned int csum = 0;
	csum = buf[35];
	csum <<= 8;
	csum |= buf[34] ;
	csum <<= 1;
	csum |= (buf[33] >> 7);
	buf[33] &= 0x7F;
	
    //printf ("bbbbbbbbb\n");
    if ( csum != crc(0,34,buf)) return -1;
	memcpy(data,buf,CS2_BLKSIZE+1);


	return 0;
}


/* CS-3 */
int CS3_encode(unsigned char* data, unsigned char* symbol){
	unsigned char buf[85];
	unsigned char buf2[85];
	unsigned int csum;
	bzero(buf,sizeof(unsigned char[85]));
	bzero(buf2,sizeof(unsigned char[85]));
	bzero(symbol,sizeof(unsigned char [57]));
	memcpy(buf,data,40);
	buf[39] &= 0x07;
	
	/* check sum bits */
	csum = crc(0,40,buf);
	buf[39] |= (csum & 0x1F) << 3;
	buf[40] = (csum >> 5) & 0xFF;
	buf[41] = (csum >> 13) & 0x7;
	
	/* usf bits */
	for ( int i = 42; i > 0; i--) buf[i] = (buf[i] << 3) | (buf[i-1] >> 5);
	buf[0] = ((buf[0] << 3) & 0xC0) | CS2USF[data[0] & 0x7];

	cvencode(buf,338,buf2);

	/* 
	 * punctured C(3+6j) and C(5+6j) for j = 2,3,...,111
	 * stupid ways , process bit by bit
	 * */
	bzero(buf,sizeof(unsigned char[85]));
	buf[0] = buf2[0];
	for ( int i = 8, n = 8; i < 676; i++, n++){
		if ( i > 14 && ((i % 6 == 3) || (i % 6 == 5)) && (i / 6 < 112)){
			i++;
		}
		buf[n/8] |= ((buf2[i/8] >> (i % 8)) & 0x1) << (n % 8);
	}

	interleaving(buf,symbol);
	return 0;
}

int CS3_decode( unsigned char* symbol, unsigned char* data){
	unsigned char buf[85];
	unsigned char buf2[85];
	bzero(buf,sizeof(unsigned char[85]));
	bzero(buf2,sizeof(unsigned char[85]));
	bzero(data,CS3_BLKSIZE+1);

	deinterleaving(symbol,buf);
	
	/* 
	 * restore putctured bits
	 * stupid ways , process bit by bit
	 * */
	buf2[0] = buf[0];
	for ( int i = 8,n = 8; i < 456; i++,n++){
		if ( n > 14 && (n % 6 == 3 || n % 6 == 5) && n / 6 < 112){
			i--;
		}else{
			buf2[n/8] |= ((buf[i/8] >> i % 8) & 0x1) << n % 8;
		}
	}

	bzero(buf,sizeof(unsigned char[85]));
	viterbi(buf2, 676, CS3_NullSyms,buf);
	
	/* check USF */
	unsigned char usf;
	for (usf = 0 ; usf < 8; usf++)
		if (CS2USF[usf] == (buf[0] & 0x3F))break;
	if (usf == 8 ) return -1;

	buf[0] = (usf & 0x7) | ((buf[0] >> 3)&0xF8) | (buf[1] << 5);
	for ( int i = 1; i < 85; i++) buf[i] = (buf[i] >> 3) | (buf[i+1] << 5);

	/* CRC Check Sum */
	unsigned int csum = buf[41];
	csum <<= 8;
	csum += buf[40];
	csum <<= 5;
	csum |= (buf[39] & 0xF8) >> 3; 
	buf[39] &= 0x7;
	if ( csum != crc(0,40,buf)) return -1;

	memcpy(data,buf,CS3_BLKSIZE+1);
	return 0;
}

unsigned int CS4USF[8] = { 0x0, 0x0D0B, 0x06EC, 0x0BE7, 0x0BB0, 0x06BB, 0x0D5C, 0x0057};
int CS4_encode( unsigned char* data, unsigned char* symbol){
	bzero(symbol,sizeof(unsigned char [57]));
	data[53] &= 0x7F;
	unsigned int csum = crc(0,54,data);;
	memcpy(symbol+1,data,54);
	for ( int i = 56; i > 0; i--) symbol[i] = (symbol[i] << 1) | (symbol[i-1] >> 7);
	symbol[56] = csum & 0xFF;
	symbol[57] = (csum >> 8) & 0xFF;
	unsigned int usf = CS4USF[data[0] & 0x7];
	symbol[0] = usf & 0xff;
	symbol[1] &= 0xF0;
	symbol[1] |= ((usf >> 8) & 0x0F);
	return 0;
}

int CS4_decode(unsigned char* symbol, unsigned char* data){
	unsigned int usf = symbol[1] & 0x0f;
	usf <<= 8;
	usf += symbol[0];
	int i;
	for (i = 0; i < 8; i++){
		if ( CS4USF[i] == usf )break;
	}
	if (i == 8)return -1;
	
	bzero(data,CS4_BLKSIZE+1);
	memcpy(data,symbol+1,CS4_BLKSIZE+1);
	for ( int j = 0; j < 54; j++) data[j] = (data[j] >> 1) | (data[j+1] << 7);
	data[0] = (data[0] & 0xF8) | i;
	data[53] &= 0x7F;
	unsigned int csum = symbol[57];
	csum <<= 8;
	csum += symbol[56];
	
	if (csum != crc(0,54,data)) return -1;
	return 0;
}
