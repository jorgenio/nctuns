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

#include "ofdma_map_ie.h"

using namespace MR_OFDMAMapIE_NT;

/*
 * OFDMA DL-MAP IE
 */
OFDMA_DLMAP_IE::OFDMA_DLMAP_IE()
{
	_ie_data    = NULL;
	_ie_bits    = 0;
	_offset     = 0;
	_diuc       = 0;
}

OFDMA_DLMAP_IE::OFDMA_DLMAP_IE(int diuc)    // for send (each burst)
{
	if (diuc < 0 || diuc > 15)
	{
		fprintf(stderr, "%s: DIUC = %d\n", __func__, diuc);
		exit(1);
	}
	else
	{
		_diuc       = diuc;
		_ie_bits    = 0;
		_ie_data    = NULL;
		_offset     = 0;
	}
}

OFDMA_DLMAP_IE::OFDMA_DLMAP_IE(uint8_t *data, int nBytes)  // for recv (all IEs)
{
	_ie_data = new uint8_t [nBytes];
	memcpy(_ie_data, data, nBytes);

	_diuc       = 0;
	_offset     = 0;
	_ie_bits    = 0;
}

OFDMA_DLMAP_IE::~OFDMA_DLMAP_IE()
{
	if (_ie_data != NULL)
		delete [] _ie_data;
}

void OFDMA_DLMAP_IE::mallocIE(int factor)
{
	switch(_diuc) {
        case 13:
            _ie_bits = 36;
            break;
        
        case 14:
			_ie_bits = 16 + factor * 8;
			break;

		case 15:
			_ie_bits = 12 + factor * 8;
			break;

		default:
			_ie_bits = 44 + factor * 16;
			break;
	}

	if (_ie_bits % 8 == 0)
	{
		_ie_data = new uint8_t [_ie_bits / 8];
		memset(_ie_data, 0, _ie_bits / 8);
	}
	else
	{
		_ie_data = new uint8_t [_ie_bits / 8 + 1];
		memset(_ie_data, 0, _ie_bits / 8 + 1);
	}
}

void OFDMA_DLMAP_IE::appendBitField(int pBits, uint32_t pValue)
{
	uint8_t data[4]  = "";

	for (int i = 0;i < 4;i++)
	{
		data[i] = (pValue & (0xFF << (i * 8))) >> (i * 8);
	}
	appendBitField(pBits, data);
}

void OFDMA_DLMAP_IE::appendBitField(int pBits, uint8_t *pValue)
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
			_ie_data[_offset / 8] |= (bit << position);

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

void OFDMA_DLMAP_IE::extractField(int pBits, uint8_t *pBuf)
{
	uint8_t bit     = 0x00;
	int position    = 0;
	int leftBits    = pBits;

	for (int i = 0;i <= ((pBits - 1) / 8);i++)
	{
		for (int j = 7;j >= 0;j--)
		{
			pBuf[i] <<= 1;
			position = 7 - _offset % 8; // 0 ~ 7
			bit = (_ie_data[_offset / 8] >> position) & 0x01;
			pBuf[i] |= bit;
			_offset++;

			leftBits--;

			if (leftBits == 0)
				break;
		}
	}
}

/*
 * OFDMA UL-MAP IE
 */
OFDMA_ULMAP_IE::OFDMA_ULMAP_IE()
{
	_ie_data    = NULL;
	_ie_bits    = 0;
	_offset     = 0;
	_cid        = 0;
	_uiuc       = 0;
}

OFDMA_ULMAP_IE::OFDMA_ULMAP_IE(uint16_t cid, int uiuc)  // for send (each burst)
{
	if (uiuc < 0 || uiuc > 15)
	{
		fprintf(stderr, "%s: UIUC = %d\n", __func__, uiuc);
		exit(1);
	}
	else
	{
		_cid        = cid;
		_uiuc       = uiuc;
		_ie_bits    = 0;
		_ie_data    = NULL;
		_offset     = 0;
	}
}

OFDMA_ULMAP_IE::OFDMA_ULMAP_IE(uint8_t *data, int nBytes)  // for recv (all IEs)
{
	_ie_data = new uint8_t [nBytes];
	memcpy(_ie_data, data, nBytes);

	_offset     = 0;
	_uiuc       = (_ie_data[2] >> 4) & 0x0F;
	_cid        = _ie_data[0] * 256 + _ie_data[1];
}

OFDMA_ULMAP_IE::~OFDMA_ULMAP_IE()
{
	if (_ie_data != NULL)
		delete [] _ie_data;
}

void OFDMA_ULMAP_IE::mallocIE(int factor)
{
	switch(_uiuc) {
		case 0:
			_ie_bits = 52;
			break;

		case 11:
			_ie_bits = 32 + factor * 8;
			break;

		case 12:
			_ie_bits = 52;
			break;

		case 13:
			break;

		case 14:
			_ie_bits = 60;
			break;

		case 15:
			_ie_bits = 32 + factor * 8;
			break;

		default:
			_ie_bits = 32;
			break;
	}

	if (_ie_bits % 8 == 0)
	{
		_ie_data = new uint8_t [_ie_bits / 8];
		memset(_ie_data, 0, _ie_bits / 8);
	}
	else
	{
		_ie_data = new uint8_t [_ie_bits / 8 + 1];
		memset(_ie_data, 0, _ie_bits / 8 + 1);
	}
}

void OFDMA_ULMAP_IE::appendBitField(int pBits, uint32_t pValue)
{
	uint8_t data[4]  = "";

	for (int i = 0;i < 4;i++)
	{
		data[i] = (pValue & (0xFF << (i * 8))) >> (i * 8);
	}
	appendBitField(pBits, data);
}

void OFDMA_ULMAP_IE::appendBitField(int pBits, uint8_t *pValue)
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
			_ie_data[_offset / 8] |= (bit << position);

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

void OFDMA_ULMAP_IE::extractField(int pBits, uint8_t *pBuf)
{
	uint8_t bit     = 0x00;
	int position    = 0;
	int leftBits    = pBits;

	for (int i = 0;i <= ((pBits - 1) / 8);i++)
	{
		for (int j = 7;j >= 0;j--)
		{
			pBuf[i] <<= 1;
			position = 7 - _offset % 8; // 0 ~ 7
			bit = (_ie_data[_offset / 8] >> position) & 0x01;
			pBuf[i] |= bit;
			_offset++;

			leftBits--;

			if (leftBits == 0)
				break;
		}
	}
}
