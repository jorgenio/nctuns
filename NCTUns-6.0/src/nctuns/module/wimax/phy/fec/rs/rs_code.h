/*
 * Reed-Solomon code
 *
 */

#ifndef __NCTUNS_REED_SOLOMON_CODE_H__
#define __NCTUNS_REED_SOLOMON_CODE_H__

/* This is one of 14 irreducible polynomials
 * of degree 8 and cycle length 255. (Ch 5, pp. 275, Magnetic Recording)
 * The high order 1 bit is implicit */
/* x^8 + x^4 + x^3 + x^2 + 1 */

//#define PPOLY 0x1D 

#define NPAR	16		// Number of parity bytes
#define MAXDEG	(NPAR*2)	// Maximum degree of various polynomials

class ReedSolomonCode
{
private:
	int	gen_exp[512];
	int	gen_log[256];
	int	genPoly[MAXDEG*2];

	void	galois_tables();
	void	compute_genpoly(int nbytes, int genPoly[]);
	int	check_syndrome(int synBytes[]);
	int	correct_errors(unsigned char codeword[], int csize,
			int nerasures, int erasures[], int synBytes[]);
	void	Berlekamp_Massey(int nerasures, int erasures[],
			int synBytes[], int Lambda[], int Omega[]);
	void	init_gamma(int nerasures, int erasures[], int gamma[]);
	int	compute_discrepancy(int lambda[], int S[], int L, int n);
	void	compute_modified_omega(int Lambda[], int synBytes[],
			int Omega[]);
	int	Find_Roots(int Lambda[], int ErrorLocs[]);

	int	gen_mul(int a, int b);
	int	gen_inverse(int elt) ;
	void	scale_poly(int number, int poly[]);
	void	mult_polys(int to[], int poly1[], int poly2[]);
	void	add_polys(int to[], int from[]) ;
	void	copy_poly(int to[], int from[]);
	void	zero_poly(int poly[]);
	void	mul_z_poly(int from[]);

public:
	ReedSolomonCode();
	~ReedSolomonCode();

	int encode(char *input, char *output, int inputLen, int parityLen);
	int decode(char *input, char *output, int inputLen, int parityLen);
};

#endif /* __NCTUNS_REED_SOLOMON_CODE_H__ */
