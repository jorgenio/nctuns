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

/*
  *********************************************************************
  *                                                                   *
  *        Galois Field Arithmetic Library (version 0.0.1)            *
  *                                                                   *
  * Class: Galois Field Element                                       *
  * Version: 0.0.1                                                    *
  * Author: Arash Partow - 2000                                       *
  * URL: http://www.partow.net/projects/galois/index.html             *
  *                                                                   *
  * Copyright Notice:                                                 *
  * Free use of this library is permitted under the guidelines and    *
  * in accordance with the most current version of the Common Public  *
  * License.                                                          *
  * http://www.opensource.org/licenses/cpl.php                        *
  *                                                                   *
  *********************************************************************
*/


#ifndef INCLUDE_GALOISFIELDELEMENT_H
#define INCLUDE_GALOISFIELDELEMENT_H

#include <iostream>
#include <vector>
#include "GaloisField.h"


namespace galois
{

   class GaloisFieldElement
   {

      public:

       GaloisFieldElement(GaloisField* _gf = NULL, GFSymbol v = -1);
       GaloisFieldElement(const GaloisFieldElement& gfe);
      ~GaloisFieldElement(){}

       inline GaloisFieldElement& operator=(const GaloisFieldElement& gfe)
       {
          if (this == &gfe)
            return *this;

          gf          = gfe.gf;
          poly_value  = gfe.poly_value;

          return *this;
       }


       inline GaloisFieldElement& operator=(const GFSymbol& v)
       {
          poly_value  = v & gf->size();
          return *this;
       }


       inline GaloisFieldElement& operator+=(const GaloisFieldElement& gfe)
       {
          poly_value ^= gfe.poly_value;
          return *this;
       }


       inline GaloisFieldElement& operator+=(const GFSymbol& v)
       {
          poly_value ^= v;
          return *this;
       }


       inline GaloisFieldElement& operator-=(const GaloisFieldElement& gfe)
       {
           *this += gfe;
           return *this;
       }


       inline GaloisFieldElement& operator-=(const GFSymbol& v)
       {
          *this += v;
          return *this;
       }


       inline GaloisFieldElement& operator*=(const GaloisFieldElement& gfe)
       {
          poly_value = gf->mul(poly_value,gfe.poly_value);
          return *this;
       }


       inline GaloisFieldElement& operator*=(const GFSymbol& v)
       {
          poly_value = gf->mul(poly_value,v);
          return *this;
       }


       inline GaloisFieldElement& operator/=(const GaloisFieldElement& gfe)
       {
          poly_value = gf->div(poly_value,gfe.poly_value);
          return *this;
       }


       inline GaloisFieldElement& operator/=(const GFSymbol& v)
       {
          poly_value = gf->div(poly_value,v);
          return *this;
       }


       inline GaloisFieldElement& operator^=(const int& n)
       {
          poly_value = gf->exp(poly_value,n);
          return *this;
       }


       inline bool operator==(const GaloisFieldElement& gfe) const
       {
          return (
                  (gf  == gfe.gf) &&
                  (poly_value == gfe.poly_value)
                 );
       }


       inline bool operator==(const GFSymbol& v) const
       {
          return (poly_value == v);
       }


       inline bool operator!=(const GaloisFieldElement& gfe) const
       {
          return (
                  (gf  != gfe.gf) ||
                  (poly_value != gfe.poly_value)
                 );
       }


       inline bool operator!=(const GFSymbol& v) const
       {
          return (poly_value != v);
       }


       inline bool operator<(const GaloisFieldElement& gfe)
       {
          return (poly_value < gfe.poly_value);
       }


       inline bool operator<(const GFSymbol& v)
       {
          return (poly_value < v);
       }

       inline bool operator>(const GaloisFieldElement& gfe)
       {
          return (poly_value > gfe.poly_value);
       }


       inline bool operator>(const GFSymbol& v)
       {
          return (poly_value > v);
       }

       inline GFSymbol index() const
       {
          return gf->index(poly_value);
       }


       inline GFSymbol poly() const
       {
          return poly_value;
       }


       inline GaloisField* field() const
       {
          return gf;
       }


       inline GFSymbol inverse() const
       {
          return gf->inverse(poly_value);
       }


       friend std::ostream& operator << (std::ostream& os, const GaloisFieldElement& gfe);

      private:

       GaloisField* gf;
       GFSymbol     poly_value;

   };

   GaloisFieldElement operator + (const GaloisFieldElement& a, const GaloisFieldElement& b);
   GaloisFieldElement operator - (const GaloisFieldElement& a, const GaloisFieldElement& b);
   GaloisFieldElement operator * (const GaloisFieldElement& a, const GaloisFieldElement& b);
   GaloisFieldElement operator * (const GaloisFieldElement& a, const GFSymbol& b          );
   GaloisFieldElement operator * (const GFSymbol& a,           const GaloisFieldElement& b);
   GaloisFieldElement operator / (const GaloisFieldElement& a, const GaloisFieldElement& b);
   GaloisFieldElement operator ^ (const GaloisFieldElement& a, const int& b               );

}

#endif
