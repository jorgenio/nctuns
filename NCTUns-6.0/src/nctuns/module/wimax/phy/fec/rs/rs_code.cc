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

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "rs_code.h"
#define field_size 255
ReedSolomonCode::ReedSolomonCode()
{
	/* Initialize the galois field arithmetic tables */
	galois_tables();

	/* Compute the encoder generator polynomial */
	compute_genpoly(NPAR, genPoly);
}

ReedSolomonCode::~ReedSolomonCode()
{
}

int ReedSolomonCode::encode(char *input, char *output, int inputLen,
	int parityLen)
{
	unsigned char	*data = (unsigned char *) input;
	int		shift_reg[NPAR+1], dbyte;
	int		i, j;

	if (parityLen > NPAR)
	{
		printf("ReedSolomonCode::encode() parityLen > NPAR\n");
		exit(1);
	}

	if (parityLen == 0)
	{
		memcpy(output, input, inputLen);
		return inputLen;
	}

	memset(shift_reg, 0, sizeof(shift_reg));
	i=0;
	while (i<inputLen)
	{
  		dbyte = data[i] ^ shift_reg[NPAR-1];
  		j=NPAR-1;
		while(j)
		{
			shift_reg[j] = shift_reg[j-1] ^ gen_mul(genPoly[j], dbyte);
			j--;
		}

		shift_reg[0] = gen_mul(genPoly[0], dbyte);
		i++;
	}

	// Build codeword
	for (i=0; i<parityLen; i++)
	{
		output[i] = shift_reg[NPAR-1-i];
	}

	return parityLen;
}

int ReedSolomonCode::decode(char *input, char *output, int inputLen,
	int parityLen)
{
	int		ML = inputLen - parityLen + NPAR;
	unsigned char	data[ML];
	int		synBytes[MAXDEG];
	int		erasures[NPAR];
	int		nerasures = 0;
	int		sum;

	memset(data, 0, ML);
	memcpy(data, input+parityLen, inputLen-parityLen); //copy codeword 
	memcpy(data+inputLen-parityLen, input, parityLen); //copy parity
	memset(synBytes, 0, sizeof(synBytes));

	for (int i=0; i<NPAR-parityLen; i++)
	{
		erasures[nerasures++] = ML-inputLen-i-1;
	}

	for (int j=0; j<NPAR; j++)
	{
		sum = 0;
		for (int i=0; i<ML; i++)
		{
			sum = data[i] ^ gen_mul(gen_exp[j+1], sum);
		}
		synBytes[j]  = sum;
	}
	/*
	 *if all syndrom are zero,decoding is done.
	 */
	bool flag=false;
	for (int i=0; i<NPAR; i++)
	{
		if (synBytes[i] != 0)
		{
			flag=true;		
		}
	}
	if(flag){
		correct_errors(data, ML, nerasures, erasures, synBytes);
	}
	memcpy(output, data, inputLen-parityLen);

	return (inputLen - parityLen);
}

void ReedSolomonCode::galois_tables()
{
  	/* initialize the table of powers of alpha */

	int digit[9];

	memset(digit,0,sizeof(digit));
	digit[1] = 1;
      
	gen_exp[0] = 1;
	gen_exp[field_size] = gen_exp[0];
	gen_log[0] = 0;		/* shouldn't log[0] be an error? */
      
	for (int i=1; i<256; i++)
	{
		digit[0] = digit[8];
		digit[8] = digit[7];
		digit[7] = digit[6];
		digit[6] = digit[5];
		digit[5] = digit[4] ^ digit[0];
		digit[4] = digit[3] ^ digit[0];
		digit[3] = digit[2] ^ digit[0];
		digit[2] = digit[1];
		digit[1] = digit[0];
		gen_exp[i] = digit[1] + digit[2]*2 + digit[3]*4 + digit[4]*8 + digit[5]*16 + digit[6]*32 +
			digit[7]*64 + digit[8]*128;
		gen_exp[i+field_size] = gen_exp[i];
	}
      
	for (int i=1; i<256; i++)
	{
		for (int z=0; z<256; z++)
		{
			if (gen_exp[z] == i)
			{
				gen_log[i] = z;
				break;
			}
		}
	}
}

/* Create a generator polynomial for an n byte RS code. 
 * The coefficients are returned in the genPoly arg.
 * Make sure that the genPoly array which is passed in is 
 * at least n+1 bytes long.
 */
void ReedSolomonCode::compute_genpoly(int nbytes, int genpoly[])
{
	int tp[256], tp1[256];
	
	/* multiply (x + a^n) for n = 1 to nbytes */

	zero_poly(tp1);
	tp1[0] = 1;

	for (int i=1; i<=nbytes; i++) {
		zero_poly(tp);
		tp[0] = gen_exp[i];	/* set up x+a^n */
		tp[1] = 1;
		      
		mult_polys(genpoly, tp, tp1);
		copy_poly(tp1, genpoly);
	}
}


/* Combined Erasure And Error Magnitude Computation 
 * 
 * Pass in the codeword, its size in bytes, as well as
 * an array of any known erasure locations, along the number
 * of these erasures.
 * 
 * Evaluate Omega(actually Psi)/Lambda' at the roots
 * alpha^(-i) for error locs i. 
 *
 * Returns 1 if everything ok, or 0 if an out-of-bounds error is found
 *
 */
int ReedSolomonCode::correct_errors(unsigned char codeword[],
		int csize, int nerasures, int erasures[], int synBytes[])
{
	/* The Error Locator Polynomial, also known as Lambda or Sigma. Lambda[0] == 1 */
	int Lambda[MAXDEG];
	
	/* The Error Evaluator Polynomial */
	int Omega[MAXDEG];

	/* error locations found using Chien's search*/
	int ErrorLocs[256];
	int NErrors;
	
	int r, i, j, err;

	memset(Lambda, 0, sizeof(Lambda));
	memset(Omega, 0, sizeof(Omega));

	Berlekamp_Massey(nerasures, erasures, synBytes, Lambda, Omega);
	NErrors = Find_Roots(Lambda, ErrorLocs);

	if ((NErrors <= NPAR) && NErrors > 0)
	{ 

		/* first check for illegal error locs */
		for (r=0; r<NErrors; r++) {
			if (ErrorLocs[r] >= csize) {
				return(0);
  			}
		}

		for (r=0; r<NErrors; r++)
		{
			int num, denom;
			i = ErrorLocs[r];
			/* evaluate Omega at alpha^(-i) */

			num = 0;
			for (j=0; j<MAXDEG; j++) 
			{
				num ^= gen_mul(Omega[j], gen_exp[((field_size-i)*j)%field_size]);
			}

			/* evaluate Lambda' (derivative) at alpha^(-i) ; all odd powers disappear */
			denom = 0;
			for (j=1; j<MAXDEG; j+=2)
			{
				denom ^= gen_mul(Lambda[j], gen_exp[((field_size-i)*(j-1)) % field_size]);
			}

			err = gen_mul(num, gen_inverse(denom));
			codeword[csize-i-1] ^= err;
		}
  		return(1);
	}
	else
	{
		if (NErrors)
		{
			fprintf(stderr, "Uncorrectable codeword\n");
		}
  		return(0);
	}
}

/* From  Cain, Clark, "Error-Correction Coding For Digital Communications", pp. 216. */
void ReedSolomonCode::Berlekamp_Massey(int nerasures, int erasures[],
				int synBytes[], int Lambda[], int Omega[])
{
	int L, L2, k, delta;
	int psi[MAXDEG], psi2[MAXDEG], D[MAXDEG];
	int gamma[MAXDEG];

	/* initialize Gamma, the erasure locator polynomial */
	init_gamma(nerasures, erasures, gamma);

	/* initialize to z */
	copy_poly(D, gamma);
	mul_z_poly(D);
	      
	copy_poly(psi, gamma);	
	k = -1; L = nerasures;

	for (int n=nerasures; n<NPAR; n++)
	{
		delta = compute_discrepancy(psi, synBytes, L, n);
    	
		if (delta != 0)
		{
			/* psi2 = psi - d*D */
			for (int i=0; i<MAXDEG; i++)
			{
				psi2[i] = psi[i] ^ gen_mul(delta, D[i]);
			}
    	
			if (L < (n-k)) {
				L2 = n-k;
				k = n-L;
				/* D = scale_poly(gen_inverse(d), psi); */
				for (int i=0; i<MAXDEG; i++)
				{
					D[i] = gen_mul(psi[i], gen_inverse(delta));
				}
				L = L2;
			}
    		
			/* psi = psi2 */
			for (int i=0; i<MAXDEG; i++) psi[i] = psi2[i];
		}
    	
		mul_z_poly(D);
	}
      
	for(int i=0; i<MAXDEG; i++)
	{
		Lambda[i] = psi[i];
	}

	//compute_modified_omega(Lambda, synBytes, Omega);
	int product[MAXDEG*2];
	mult_polys(product, Lambda, synBytes);
	zero_poly(Omega);

	for(int i=0; i<NPAR; i++)
	{
		Omega[i] = product[i];
	}
}

/* gamma = product (1-z*a^Ij) for erasure locs Ij */
void ReedSolomonCode::init_gamma(int nerasures, int erasures[], int gamma[])
{
	int tmp[MAXDEG];

	zero_poly(gamma);
	zero_poly(tmp);
	gamma[0] = 1;
      
	for (int i=0; i<nerasures; i++)
	{
		copy_poly(tmp, gamma);
		scale_poly(gen_exp[erasures[i]], tmp);
		scale_poly(8,tmp);
		add_polys(gamma, tmp);
	}
}

int ReedSolomonCode::compute_discrepancy(int lambda[], int S[], int L, int n)
{
	int sum = 0;

	for (int i=0; i<=L; i++) 
	{
		sum ^= gen_mul(lambda[i], S[n-i]);
	}
	return (sum);
}

/* given Psi (called Lambda in (Berlekamp_Massey) and synBytes,
   compute the combined erasure/error evaluator polynomial as 
   Psi*S mod z^4
  */
/*void ReedSolomonCode::compute_modified_omega(int Lambda[], int synBytes[],
				int Omega[])
{
	int product[MAXDEG*2];
	mult_polys(product, Lambda, synBytes);
	zero_poly(Omega);

	for(int i=0; i<NPAR; i++)
	{
		Omega[i] = product[i];
	}
}*/

/* Finds all the roots of an error-locator polynomial with coefficients
 * Lambda[j] by evaluating Lambda at successive values of alpha. 
 * 
 * This can be tested with the decoder's equations case.
 */
int ReedSolomonCode::Find_Roots(int Lambda[], int ErrorLocs[])
{
	int sum, r, k;	
	int NErrors = 0;
  
	for (r=1; r<256; r++)
	{
		sum = 0;
		/* evaluate lambda at r */
		for (k=0; k<NPAR+1; k++)
		{
			sum ^= gen_mul(gen_exp[(k*r)%field_size], Lambda[k]);
		}
		if (sum == 0) 
		{ 
			ErrorLocs[NErrors] = (field_size-r);
			NErrors++; 
		}
	}

	return NErrors;
}

/* multiplication using logarithms */
int ReedSolomonCode::gen_mul(int a, int b)
{
	if (a==0) return (0);
	if (b==0) return (0);
	return (gen_exp[(gen_log[a]+gen_log[b])]);
}

int ReedSolomonCode::gen_inverse(int elt) 
{ 
	return (gen_exp[field_size-gen_log[elt]]);
}

void ReedSolomonCode::scale_poly(int number, int poly[]) 
{	

	for (int i=0; i<MAXDEG; i++)
	{
		poly[i] = gen_mul(number, poly[i]);
	}
}

/* polynomial multiplication */
void ReedSolomonCode::mult_polys(int to[], int poly1[], int poly2[])
{
	int tmp1[MAXDEG*2];
	      
	for (int i=0; i<(MAXDEG*2); i++)
	{
		to[i] = 0;
	}
	      
	for (int i=0; i<MAXDEG; i++)
	{
		for (int j=MAXDEG; j<(MAXDEG*2); j++)
		{
			tmp1[j]=0;
		}
		      
		/* scale tmp1 by p1[i] */
		for (int j=0; j<MAXDEG; j++)
		{
			tmp1[j]=gen_mul(poly2[j], poly1[i]);
		}

		/* and mult (shift) tmp1 right by i */
		for (int j=(MAXDEG*2)-1; j>=i; j--)
		{
			tmp1[j] = tmp1[j-i];
		}

		for (int j=0; j<i; j++)
		{
			tmp1[j] = 0;
		}
		      
		/* add into partial product */
		for(int j=0; j<(MAXDEG*2); j++)
		{
			to[j] ^= tmp1[j];
		}
	}
}

void ReedSolomonCode::add_polys(int to[], int from[]) 
{

	for (int i=0; i<MAXDEG; i++)
	{
		to[i] ^= from[i];
	}
}

void ReedSolomonCode::copy_poly(int to[], int from[]) 
{

	for (int i=0; i<MAXDEG; i++)
	{
		to[i] = from[i];
	}
}

void ReedSolomonCode::zero_poly(int poly[]) 
{

	for (int i=0; i<MAXDEG; i++)
	{
		poly[i] = 0;
	}
}

/* multiply by z, i.e., shift right by 1 */
void ReedSolomonCode::mul_z_poly(int from[])
{

	for (int i=MAXDEG-1; i>0; i--)
	{
		from[i] = from[i-1];
	}
	from[0] = 0;
}
