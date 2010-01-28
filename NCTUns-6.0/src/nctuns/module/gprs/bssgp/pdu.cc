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

#include "pdu.h"

PDU::PDU()
{
	/* */
	_data = new unsigned char[128];
	size = 128;
	seek = 0;
}

PDU::PDU( unsigned char* d)
{
	_data = d;
	size = -1;
	seek = 1;
}

PDU::~PDU(){
	//if (size > 0){
	//	delete [] _data;
	//}
}

unsigned char PDU::type(){
	return _data[0];
}

void PDU::setType(unsigned char t){
	_data[0] = t;
}

unsigned char PDU::readIEI(){
	return _data[seek++];
}

int PDU::readLength(){
	if ( !(_data[seek] & 0x80) ) return _data[seek++];
	return _data[seek++] << 8 | _data[seek++];
}

unsigned char PDU::readIEByte(){
	return _data[seek++];
}

unsigned int PDU::readIEWord(){
	seek+=2;
	return (_data[seek-2] << 8) | _data[seek-1];
}

unsigned long PDU::readIEDWord(){
	seek+=4;
	return (_data[seek-4] << 24) | (_data[seek-3] << 16) | (_data[seek-2] << 8) | _data[seek-1];
}

unsigned char* PDU::readIEValue(int len){
	seek += len;
	return _data + seek - len;
}

int PDU::readTLV(unsigned char iei, unsigned char* buf){
	if (readIEI() != iei)return -1;
	int len = readLength();
	if (len <= 0) return len;
	
	bzero(buf,len);
	bcopy(readIEValue(len),buf,len);
	return len;
}

unsigned int PDU::readTLVWord(unsigned char iei){
	if (readIEI() != iei)return 0;
	readLength();
	return readIEWord();
}

unsigned long PDU::readTLVDWord(unsigned char iei){
	if (readIEI() != iei)return 0;
	readLength();
	return readIEDWord();
}

unsigned char PDU::readTLVByte(unsigned char iei){
	if (readIEI() != iei)return 0;
	readLength();
	return readIEByte();
}

int PDU::writeTLV(unsigned char iei,unsigned char * buf, int n){
	if ( n + 3 + seek >= size ){
		unsigned char * tmp = new unsigned char[size * 2];	
		size *= 2;
		memcpy(tmp,_data,seek);
		unsigned char* p = _data;
		_data = tmp;
		delete p;
	}
	_data[seek++] = iei;
	if (n < 128)
		_data[seek++] = n;
	else {
		_data[seek++] = n % 128 + 128;
		_data[seek++] = n / 128;
	}
	memcpy(_data+seek, buf, n);
	seek += n;
	return n;
}

int PDU::writeTLVWord(unsigned char iei,unsigned int word){
	if ( 4 + seek >= size ){
		unsigned char * tmp = new unsigned char[size * 2];	
		size *= 2;
		memcpy(tmp,_data,seek);
		unsigned char* p = _data;
		_data = tmp;
		delete p;
	}
	_data[seek++] = iei;
	_data[seek++] = 2;
	_data[seek++] = (word >> 8) & 0xff;
	_data[seek++] = word & 0xff;
	return 2;
}

int PDU::writeTLVDWord(unsigned char iei,unsigned long dword){
	if ( 6 + seek >= size ){
		unsigned char * tmp = new unsigned char[size * 2];	
		size *= 2;
		memcpy(tmp,_data,seek);
		unsigned char* p = _data;
		_data = tmp;
		delete p;
	}
	_data[seek++] = iei;
	_data[seek++] = 4;
	_data[seek++] = (dword >> 24) & 0xff;
	_data[seek++] = (dword >> 16) & 0xff;
	_data[seek++] = (dword >> 8) & 0xff;
	_data[seek++] = dword & 0xff;
	return 2;
}

int PDU::writeByte(unsigned char d){
	return writeV(d);
}

int PDU::writeV(unsigned char d){
	_data[seek++] = d;
	return 1;
}

int PDU::writeV(unsigned int word){
	_data[seek++] = (word >> 8) & 0xff;
	_data[seek++] = word & 0xff;
	return 1;
}

int PDU::writeV(unsigned long word){
	_data[seek++] = (word >> 24) & 0xff;
	_data[seek++] = (word >> 16) & 0xff;
	_data[seek++] = (word >> 8) & 0xff;
	_data[seek++] = word & 0xff;
	return 1;
}
