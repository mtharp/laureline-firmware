/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 * $Id: pll.h,v 1.10 2005/09/14 09:48:28 phk Exp $
 *
 * PLL state variables.
 * These are kept in a structure to make logging easier.
 *
 */

struct pll_state {
	double		y;	/* Phase error measurement */
	double		ly;	/* Last y */
	double		my;	/* Averaged phase error */
	double		lmy;	/* Last my */
	double		dy;	/* Two sample difference of y */
	double		ddy;	/* Previous two sample difference of y */
	double		sdy;	/* Averaged magnitude of dy */
	double		ay;	/* Two sample variance (avar sigma=1) */
	double		say;	/* Averaged ay */

	double		a;	/* 1st order correction */
	double		b;	/* 2nd order correction */
	double		c;	/* 3rd order correction */

	double		z;	/* Total correction */

	double		wa;	/* 1st order ratio */

	double		wamin;	/* Lower limit on wa */
	double		wamax;	/* Upper limit on wa */

	int		j;	/* Averaging factor */
	double		mj;	/* Max averaging factor */
	int		dl;	/* Delay timer */
	int		zc;	/* Zero-cross timer */

	int		st;	/* Pll state number */

	/* Parameters */

	double		go3;	/* which wa value enable 3. order */
	double		t3;	/* dl expansion in 3. order mode */
	double		mj0;	/* Initial mj */
	double		dl0;	/* Initial dl */
	double		wa0;	/* Initial wa */

	double		b0;	/* Initial frequency estimate */
	double		be;	/* 2nd order exponent */
	double		ce;	/* 3nd order exponent */
};

extern struct pll_state pll_state;

double pll_math(double y);
double pll_poll(void);
void pll_reset(void);

/* ----- */
/* pll.c */
/* ------*/

void pll(double o);
void pllnone(void);
void pllstep(double d);
void kern_freq(double f);

