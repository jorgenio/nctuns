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
#include "management_message.h"
#include "structure.h"

ManagementMessage::ManagementMessage(u_char pType)
{
	Type = pType;
	flag = 1;
}

ManagementMessage::ManagementMessage(u_char* pBuffer)
{
	Type = pBuffer[0];
	flag = 0;
}

ManagementMessage::~ManagementMessage()
{
}

ifTLV::ifTLV()
{
	Buffer = new u_char[256];
	Len = 0;
	ptr = NULL;
	flag = 1;
}

ifTLV::ifTLV(u_char * srcbuf, int pLen)
{
	Buffer = srcbuf;
	Len = pLen;
	ptr = NULL;
	flag = 0;
}

ifTLV::~ifTLV()
{
	if (flag)
		delete Buffer;
}

int ifTLV::Add(int pType, int pLen, unsigned int pValue)
{
	int i;
	Buffer[Len++] = pType;
	Buffer[Len++] = pLen;
	for (i = 0; i < pLen; i++) {
		Buffer[Len + i] = pValue % 256;
		pValue /= 256;
	}
	Len += pLen;
	return Len;
}

int ifTLV::Add(int pType, int pLen, void *pBuf)
{
	int i;
	char *cbuf = (char *) pBuf;
	Buffer[Len++] = pType;
	Buffer[Len++] = pLen;
	for (i = 0; i < pLen; i++) {
		Buffer[Len + i] = cbuf[i];
	}
	Len += pLen;
	return Len;
}

int
ifTLV::copyTo(u_char* dstbuf) const
{
	memcpy(dstbuf, Buffer, Len);
	return 0;
}

const u_char *ifTLV::getBuffer()
{
	return Buffer;
}

int ifTLV::getNextType()
{
	if (ptr) {
		ptr += tlv_len + 2;
	} else {
		ptr = Buffer;
	}
	if (ptr - Buffer >= Len) {
		return 0;
	}
	tlv_len = ptr[1];
	tlv_value = ptr + 2;
	return ptr[0];
}

int ifTLV::getNextLen()
{
	return tlv_len;
}

int ifTLV::getNextValue(void *pBuf)
{
	memcpy(pBuf, tlv_value, tlv_len);
	return 0;
}

void ifTLV::Dump()
{
	for (int i = 0; i < Len; i++)
		printf("%02x ", Buffer[i]);
	printf("\n");
}

ifmgmt::ifmgmt(int pType, int pFieldSize):ManagementMessage(pType)	// For Send
{
	FieldSize = pFieldSize;
	Field = new u_char[FieldSize];
	ptr = Field;
	Tlv = new ifTLV();
}

ifmgmt::ifmgmt(u_char* pBuffer, int pLength, int pFieldSize)
: ManagementMessage(pBuffer)	// For Recv
{
	Field = pBuffer + 1;
	FieldSize = pFieldSize;
	Tlv = new ifTLV(pBuffer + 1 + pFieldSize, pLength - pFieldSize - 1);
	ptr = NULL;
}

ifmgmt::~ifmgmt()
{
	if (flag) {
		delete Field;
	}
	delete Tlv;
}

int
ifmgmt::copyTo(u_char* dstbuf) const
{
	dstbuf[0] = Type;
	memcpy(dstbuf + 1, Field, FieldSize);
	Tlv->copyTo(dstbuf + 1 + FieldSize);
	return 0;
}

int ifmgmt::appendField(int pLen, int pValue)
{
	memcpy(ptr, &pValue, pLen);
	ptr += pLen;
	return ptr - Field;
}

int ifmgmt::appendField(int pLen, void *pValue)
{
	memcpy(ptr, pValue, pLen);
	ptr += pLen;
	return ptr - Field;
}

int ifmgmt::setField(int pStart, int pLen, int pValue)
{
	if (pStart + pLen > FieldSize)
		return -1;
	memcpy(Field + pStart, &pValue, pLen);
	return ptr - Field;
}

int ifmgmt::setField(int pStart, int pLen, void *pValue)
{
	if (pStart + pLen > FieldSize)
		return -1;
	memcpy(Field + pStart, pValue, pLen);
	return ptr - Field;
}

int ifmgmt::appendTLV(int pType, int pLen, unsigned int pValue)
{
	return Tlv->Add(pType, pLen, pValue);
}

int ifmgmt::appendTLV(int pType, int pLen, void *pBuf)
{
	return Tlv->Add(pType, pLen, pBuf);
}

int ifmgmt::getField(int pStart, int pLen, void *pBuf)
{
	memcpy(pBuf, Field + pStart, pLen);
	return 0;
}

int ifmgmt::getNextType()
{
	return Tlv->getNextType();
}

int ifmgmt::getNextLen()
{
	return Tlv->getNextLen();
}

int ifmgmt::getNextValue(void *pBuf)
{
	return Tlv->getNextValue(pBuf);
}

void ifmgmt::Dump()
{
	printf("Length = %d \t", getLen());
	for (int i = 0; i < FieldSize; i++)
		printf("%02x ", Field[i] & 0xff);

	Tlv->Dump();
}
