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

#include "GaloisFieldElement.h"

namespace galois
{

   GaloisFieldElement::GaloisFieldElement(GaloisField* _gf, GFSymbol v)
   {
      if (_gf != NULL)
      {
         gf         = _gf;
         poly_value = v & gf->size();
      }
      else
        poly_value = v;
   }


   GaloisFieldElement::GaloisFieldElement(const GaloisFieldElement& gfe)
   {
      gf          = gfe.gf;
      poly_value  = gfe.poly_value;
   }


   std::ostream& operator << (std::ostream& os, const GaloisFieldElement& gfe)
   {
      os << gfe.poly_value;
      return os;
   }


   GaloisFieldElement operator+(const GaloisFieldElement& a, const GaloisFieldElement& b)
   {
      GaloisFieldElement result  = a;
      result += b;
      return result;
   }


   GaloisFieldElement operator-(const GaloisFieldElement& a, const GaloisFieldElement& b)
   {
      GaloisFieldElement result  = a;
      result -= b;
      return result;
   }


   GaloisFieldElement operator*(const GaloisFieldElement& a, const GaloisFieldElement& b)
   {
      GaloisFieldElement result  = a;
      result *= b;
      return result;
   }


   GaloisFieldElement operator*(const GaloisFieldElement& a, const GFSymbol& b)
   {
      GaloisFieldElement result  = a;
      result *= b;
      return result;
   }


   GaloisFieldElement operator*(const GFSymbol& a, const GaloisFieldElement& b)
   {
      GaloisFieldElement result  = b;
      result *= a;
      return result;
   }


   GaloisFieldElement operator/(const GaloisFieldElement& a, const GaloisFieldElement& b)
   {
      GaloisFieldElement result  = a;
      result /= b;
      return result;
   }


   GaloisFieldElement operator^(const GaloisFieldElement& a, const int& b)
   {
      GaloisFieldElement result  = a;
      result ^= b;
      return result;
   }

}
