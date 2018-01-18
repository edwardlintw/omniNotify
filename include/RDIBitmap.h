// -*- Mode: C++; -*-
//                              File      : RDIBitmap.h
//                              Package   : omniNotify-Library
//                              Created on: 1-Jan-1998
//                              Authors   : gruber&panagos
//
//    Copyright (C) 1998-2003 AT&T Laboratories -- Research
//
//    This file is part of the omniNotify library
//    and is distributed with the omniNotify release.
//
//    The omniNotify library is free software; you can redistribute it and/or
//    modify it under the terms of the GNU Library General Public
//    License as published by the Free Software Foundation; either
//    version 2 of the License, or (at your option) any later version.
//
//    This library is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//    Library General Public License for more details.
//
//    You should have received a copy of the GNU Library General Public
//    License along with this library; if not, write to the Free
//    Software Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
//    02111-1307, USA
//
//
// Description:
//    proprietary interface
//
 
#ifndef _RDI_BITMAP_H_
#define _RDI_BITMAP_H_

#include <string.h>
#include "RDIstrstream.h"

/** RDI_Bitmap
  *
  * A simple class that encapsulates a sequence of bits that can
  * be manipulated. 
  */

class RDI_Bitmap {
public:
  inline RDI_Bitmap(unsigned int num_bits=8);
  ~RDI_Bitmap()	{ if ( _size ) delete [] _bits; _nbit = 0; _size = 0; }

  inline RDI_Bitmap& operator= (const RDI_Bitmap& bmap);

  // Set/Clear all bits or a number of bits starting at a given
  // bit offset.  If 'offset' + 'size' is greater than the size
  // of the bitmap, no bits are altered.

  inline void set();
  inline void clear();
  inline void set(unsigned int offset, unsigned int size=1);
  inline void clear(unsigned int offset, unsigned int size=1);

  // Check if a specific bit is set/clear. If the requested bit
  // is not part of the bitmap, 0 is returned

  inline int  is_set(unsigned int offset) const;
  inline int  is_clear(unsigned int offset) const;

  // Locate the first bit that is set or clear after a specific
  // bit, including this bit

  inline int  first_set(unsigned int offset=0) const;
  inline int  first_clear(unsigned int offset=0) const;

  // Return the total number of bits that are set in the bitmap

  inline unsigned int num_set() const;

  RDIstrstream& log_output(RDIstrstream& str) const {
    str << "BITMAP[";
    for (unsigned int i=0; i < _nbit; i++)
      str << (is_set(i) ? '1' : '0');
    return str << ']';
  }

private:
  unsigned char* _bits;
  unsigned int   _nbit;
  unsigned int   _size;

  RDI_Bitmap(const RDI_Bitmap&);
};

// ------------------- I M P L E M E N T A T I O N ------------------- //

inline RDI_Bitmap::RDI_Bitmap(unsigned int num_bits)
{
  _nbit = num_bits; _size = (num_bits >> 3) + 1;
  _bits = new unsigned char[ _size ];
  this->clear();
}

inline RDI_Bitmap& RDI_Bitmap::operator= (const RDI_Bitmap& bmap)
{
  if ( ! _bits || (_size < bmap._size) ) {
	if ( _bits )
		delete [] _bits;
	_nbit = bmap._nbit;
	_size = bmap._size;
	_bits = new unsigned char[ bmap._size ];
  } else {
	_nbit = bmap._nbit;
  }
  memcpy(_bits, bmap._bits, bmap._size);
  return *this;
}

inline void RDI_Bitmap::set()
{ 
  for (unsigned int i=0; i < _size; i++)
	_bits[i] = ~0;
}

inline void RDI_Bitmap::clear()
{
  for (unsigned int i=0; i < _size; i++)
	_bits[i] = 0;
}

inline int RDI_Bitmap::is_set(unsigned int offset) const
{
  unsigned int indx = offset >> 3;
  return (offset >= _nbit) ? 0 : (_bits[indx] & (1 << (offset & 7)));
}

inline int RDI_Bitmap::is_clear(unsigned int offset) const
{
  unsigned int indx = offset >> 3;
  return (offset >= _nbit) ? 0 : ! (_bits[indx] & (1 << (offset & 7)));
}

inline void RDI_Bitmap::set(unsigned int offset, unsigned int size)
{
  if ( (offset + size - 1) < _nbit ) {
	for (; size; offset++, size--)
		_bits[offset >> 3] |= (1 << (offset & 7));
  }
}

inline void RDI_Bitmap::clear(unsigned int offset, unsigned int size)
{
  if ( (offset + size - 1) < _nbit ) {
        for (; size; offset++, size--) 
                _bits[offset >> 3] &= ~(1 << (offset & 7));
  }
}

inline int RDI_Bitmap::first_set(unsigned int offset) const
{
  unsigned int   mask = 1 << (offset & 7);
  unsigned char* bits = _bits + (offset >> 3);

  for (; offset < _nbit; offset++) {
	if ( *bits & mask ) 
		return (int) offset;
	if ( (mask <<= 1) == 0x100 ) {
		mask = 1;
		bits++;
	}
  }
  return -1;
}

inline int RDI_Bitmap::first_clear(unsigned int offset) const
{
  unsigned int   mask = 1 << (offset & 7);
  unsigned char* bits = _bits + (offset >> 3);

  for (; offset < _nbit; offset++) {
        if ( ! (*bits & mask) ) 
                return (int) offset;
        if ( (mask <<= 1) == 0x100 ) {
                mask = 1;
                bits++;
        }
  }
  return -1;
}

inline unsigned int RDI_Bitmap::num_set() const
{
  unsigned int   nset = 0, mask = 1, size = _nbit;
  unsigned char* bits = _bits;

  for ( ; size; size-- ) {
	if ( *bits & mask )
		nset += 1;
	if ( (mask <<= 1) == 0x100 ) {
		mask = 1;
		bits++;
	}
  }
  return nset;
}

////////////////////////////////////////
// Logging

inline RDIstrstream& operator<< (RDIstrstream& str, const RDI_Bitmap& bmap) { return bmap.log_output(str); }

#endif
