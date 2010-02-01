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

#include "management_message.h"

using namespace mobileManageMsg;

ManagementMessage::ManagementMessage(uint8_t pType)
{
	Type = pType;
	flag = 1;
}

ManagementMessage::ManagementMessage(uint8_t* pBuffer)
{
	Type = pBuffer[0];
	flag = 0;
}

ManagementMessage::~ManagementMessage()
{
}

ifTLV::ifTLV()
{
	Buffer  = new uint8_t [256];
	len     = 0;
	ptr     = NULL;
	flag    = 1;
	memset(Buffer, 0, 256);
}

ifTLV::ifTLV(uint8_t * srcbuf, int plen)
{
	Buffer  = srcbuf;
	len     = plen;
	ptr     = NULL;
	flag    = 0;
}

ifTLV::~ifTLV()
{
	if (flag == 1)
		delete [] Buffer;
}

int ifTLV::Add(int pType, int plen, unsigned int pValue)
{
	Buffer[len++] = pType;
	Buffer[len++] = plen;

	for (int i = 0; i < plen; i++)
	{
		Buffer[len + i] = pValue % 256;
		pValue /= 256;
	}
	len += plen;
	return len;
}

int ifTLV::Add(int pType, int plen, void *pBuf)
{
	char *cbuf = (char *) pBuf;

	Buffer[len++] = pType;
	Buffer[len++] = plen;

	for (int i = 0; i < plen; i++)
	{
		Buffer[len + i] = cbuf[i];
	}
	len += plen;
	return len;
}

int ifTLV::copyTo(uint8_t* dstbuf) const
{
	memcpy(dstbuf, Buffer, len);
	return 0;
}

const uint8_t *ifTLV::getBuffer()
{
	return Buffer;
}

int ifTLV::getNextType()
{
	if (ptr)
	{
		ptr += tlv_len + 2;
	}
	else
	{
		ptr = Buffer;
	}

	if (ptr - Buffer >= len)
	{
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
	for (int i = 0; i < len; i++)
		printf("%02x ", Buffer[i]);
	printf("\n");
}

	ifmgmt::ifmgmt(int pType, int pFieldSize)
: ManagementMessage(pType)    // For Send
{
	FieldSize   = pFieldSize;
	Field       = new uint8_t [FieldSize];
	Tlv         = new ifTLV();
	_offset     = 0;
	memset(Field, 0, FieldSize);
}

	ifmgmt::ifmgmt(uint8_t* pBuffer, int pLength, int pFieldSize)  // total length and Field size
: ManagementMessage(pBuffer)    // For Recv
{
	Field       = pBuffer + 1;
	FieldSize   = pFieldSize;
	Tlv         = new ifTLV(pBuffer + 1 + pFieldSize, pLength - pFieldSize - 1);
	_offset     = 0;
}

ifmgmt::~ifmgmt()
{
	if (flag == 1)
	{
		delete [] Field;
	}
	delete Tlv;
}

int ifmgmt::copyTo(uint8_t* dstbuf) const
{
	dstbuf[0] = Type;
	memcpy(dstbuf + 1, Field, FieldSize);
	Tlv->copyTo(dstbuf + 1 + FieldSize);
	return 0;
}

void ifmgmt::appendField(int pLen, int pValue)
{
	memcpy(Field + _offset / 8, &pValue, pLen);
	_offset += pLen * 8;
}

void ifmgmt::appendField(int pLen, void *pValue)
{
	memcpy(Field + _offset / 8, pValue, pLen);
	_offset += pLen * 8;
}

void ifmgmt::appendBitField(int pBits, uint32_t pValue)
{
	uint8_t data[4]  = "";

	for (int i = 0;i < 4;i++)
	{
		data[i] = (pValue & (0xFF << (i * 8))) >> (i * 8);
	}
	appendBitField(pBits, data);
}

void ifmgmt::appendBitField(int pBits, uint8_t *pValue)
{
	uint8_t bit     = 0x00;
	int position    = 0;
	uint8_t data    = 0;
	int leftBits    = pBits;

	for (int i = 0;i <= ((pBits - 1) / 8);i++)
	{
		if (leftBits >= 8)
		{
			data = pValue[i];
		}
		else
		{
			data = pValue[i] << (8 - leftBits);
		}

		for (int j = 7;j >= 0;j--)
		{
			position = 7 - _offset % 8; // 7 ~ 0

			// Extract 1-bit
			bit = (data >> j) & 0x01;
			Field[_offset / 8] |= (bit << position);

			// Update _offset
			_offset++;

			leftBits--;

			if (leftBits == 0)
			{
				break;
			}
		}
	}
}

int ifmgmt::setField(int pStart, int pLen, int pValue)
{
	if (pStart + pLen > FieldSize)
	{
		return -1;
	}
	else
	{
		memcpy(Field + pStart, &pValue, pLen);
		return 1;
	}
}

int ifmgmt::setField(int pStart, int pLen, void *pValue)
{
	if (pStart + pLen > FieldSize)
	{
		return -1;
	}
	else
	{
		memcpy(Field + pStart, pValue, pLen);
		return 1;
	}
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

void ifmgmt::extractBitField(int pBits, uint8_t *pBuf)
{
	uint8_t bit     = 0x00;
	int position    = 0;
	int leftBits    = pBits;

	for (int i = 0;i <= ((pBits - 1) / 8);i++)
	{
		for (int j = 7;j >= 0;j--)
		{
			pBuf[i] <<= 1;
			position = 7 - _offset % 8;
			bit = (Field[_offset / 8] >> position) & 0x01;
			pBuf[i] |= bit;
			_offset++;

			leftBits--;

			if (leftBits == 0)
				break;
		}
	}
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
