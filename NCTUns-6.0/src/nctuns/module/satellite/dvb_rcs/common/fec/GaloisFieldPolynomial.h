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
  * Class: Galois Field Polynomial                                    *
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


#ifndef INCLUDE_GALOISFIELDPOLYNOMIAL_H
#define INCLUDE_GALOISFIELDPOLYNOMIAL_H

#include <iostream>
#include <vector>
#include <assert.h>
#include "GaloisField.h"
#include "GaloisFieldElement.h"


namespace galois
{

   class GaloisFieldPolynomial
   {

      public:

       GaloisFieldPolynomial(GaloisField* _gf);
       GaloisFieldPolynomial(GaloisField* _gf = NULL, const unsigned int size = 0, GaloisFieldElement* gfe = NULL);
       GaloisFieldPolynomial(const GaloisFieldPolynomial& polynomial);
       GaloisFieldPolynomial(const GaloisFieldElement& gfe);
      ~GaloisFieldPolynomial(){};

       bool valid() const;
       unsigned int deg() const;
       GaloisField* field() const;
       void set_degree(const unsigned int& x);
       void simplify();

       GaloisFieldPolynomial& operator = (const GaloisFieldPolynomial& polynomial);
       GaloisFieldPolynomial& operator = (const GaloisFieldElement&           gfe);
       GaloisFieldPolynomial& operator+= (const GaloisFieldPolynomial&        gfe);
       GaloisFieldPolynomial& operator+= (const GaloisFieldElement&           gfe);
       GaloisFieldPolynomial& operator-= (const GaloisFieldPolynomial&        gfe);
       GaloisFieldPolynomial& operator-= (const GaloisFieldElement&           gfe);
       GaloisFieldPolynomial& operator*= (const GaloisFieldPolynomial& polynomial);
       GaloisFieldPolynomial& operator*= (const GaloisFieldElement&           gfe);
       GaloisFieldPolynomial& operator/= (const GaloisFieldPolynomial&    divisor);
       GaloisFieldPolynomial& operator/= (const GaloisFieldElement&           gfe);
       GaloisFieldPolynomial& operator%= (const GaloisFieldPolynomial&    divisor);
       GaloisFieldPolynomial& operator%= (const unsigned int&               power);
       GaloisFieldPolynomial& operator^= (const int&                            n);
       GaloisFieldPolynomial& operator<<=(const unsigned int&                   n);
       GaloisFieldPolynomial& operator>>=(const unsigned int&                   n);

       GaloisFieldElement&    operator[] (const unsigned int&                term);
       GaloisFieldElement     operator() (const GaloisFieldElement&         value);
       GaloisFieldElement     operator() (GFSymbol                          value);

       const GaloisFieldElement&  operator[](const unsigned int&        term) const;
       const GaloisFieldElement   operator()(const GaloisFieldElement& value) const;
       const GaloisFieldElement   operator()(GFSymbol                  value) const;

       bool operator==(const GaloisFieldPolynomial& polynomial) const;
       bool operator!=(const GaloisFieldPolynomial& polynomial) const;

       GaloisFieldPolynomial derivative();

       friend std::ostream& operator << (std::ostream& os, const GaloisFieldPolynomial& polynomial);

      private:

       void simplify(GaloisFieldPolynomial& polynomial);

       GaloisField* gf;
       std::vector< GaloisFieldElement > poly;
   };

   GaloisFieldPolynomial operator + (const GaloisFieldPolynomial& a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator + (const GaloisFieldPolynomial& a, const GaloisFieldElement&    b);
   GaloisFieldPolynomial operator + (const GaloisFieldElement&    a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator + (const GaloisFieldPolynomial& a, const GFSymbol&              b);
   GaloisFieldPolynomial operator + (const GFSymbol&              a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator - (const GaloisFieldPolynomial& a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator - (const GaloisFieldPolynomial& a, const GaloisFieldElement&    b);
   GaloisFieldPolynomial operator - (const GaloisFieldElement&    a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator - (const GaloisFieldPolynomial& a, const GFSymbol&              b);
   GaloisFieldPolynomial operator - (const GFSymbol&              a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator * (const GaloisFieldPolynomial& a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator * (const GaloisFieldElement&    a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator * (const GaloisFieldPolynomial& a, const GaloisFieldElement&    b);
   GaloisFieldPolynomial operator / (const GaloisFieldPolynomial& a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator / (const GaloisFieldPolynomial& a, const GaloisFieldElement&    b);
   GaloisFieldPolynomial operator % (const GaloisFieldPolynomial& a, const GaloisFieldPolynomial& b);
   GaloisFieldPolynomial operator % (const GaloisFieldPolynomial& a, const unsigned int&      power);
   GaloisFieldPolynomial operator ^ (const GaloisFieldPolynomial& a, const int&                   n);
   GaloisFieldPolynomial operator <<(const GaloisFieldPolynomial& a, const unsigned int&          n);
   GaloisFieldPolynomial operator >>(const GaloisFieldPolynomial& a, const unsigned int&          n);
   GaloisFieldPolynomial         gcd(const GaloisFieldPolynomial& a, const GaloisFieldPolynomial& b);

}


#endif
