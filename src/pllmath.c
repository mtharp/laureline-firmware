/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@FreeBSD.ORG> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 * $Id: pllmath.c,v 1.37 2005/09/29 13:03:05 phk Exp $
 *
 * The reason why we're here in the first place...
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <unistd.h>
//#include <configkit.h>
//#include <isc/eventlib.h>
#include <math.h>

#include "ntpns.h"
#if 0
#include "ntpns_cfg.h"
#include "internal.h"
#else
#define record(a,b,c,d) /**/
#define rrrdupdate(a,...) /**/
#endif
#include "pll.h"

#define Log(a,...)

struct pll_state pll_state;

/*---------------------------------------------------------------------*/

void
pll_reset()
{
	Log(L_pll, "Explicit PLL reset");
	pll_state.st = 0;
	sys_able |= ABLE_PLL_UNLOCKED;
}

double
pll_poll()
{
	struct pll_state *ps = &pll_state;

	rrrdupdate("pll", "st:a:b:c:y:z:wa:wamin:dl:zc:my:j:sdy",
	    "N:%d:U:%e:%e:U:U:U:U:U:U:U:U:U",
	     ps->st, ps->b, ps->c);

	if (ps->st >= 5)
		ps->b += ps->c;
	return (ps->b);
}

double
pll_math(double y)
{
	struct pll_state *ps = &pll_state;

	/*
	 * Record our input and our state for furter digging
	 */
	ps->y = -y;
	record("PLL", 0, ps, sizeof *ps);

	rrrdupdate("pll", "st:a:b:c:y:z:wa:wamin:dl:zc:my:j:sdy",
	    "N:%d:%e:%e:%e:%e:%e:%e:%e:%d:%d:%e:%d:%e",
	     ps->st, ps->a, ps->b, ps->c, ps->y, ps->z, ps->wa, ps->wamin,
	     ps->dl, ps->zc, ps->my, ps->j, ps->sdy);


	Log(L_pll, "ST %d a %e b %e c %e y %e z %e wa %e wamin %e dl %d zc %d my %e j %d sdy %f",
	    ps->st, ps->a, ps->b, ps->c, ps->y, ps->z, ps->wa, ps->wamin,
	    ps->dl, ps->zc, ps->my, ps->j, ps->sdy);

	/*
	 * Sanity checks.
	 */

	if (fabs(ps->b) > 128e-6)
		ps->st = 0;
	
	/*
	 * If the state is zero, we need to initialize everything
	 */
	if (ps->st == 0) {
		ps->mj = ps->mj0;
		ps->dl = ps->dl0;
		ps->wa = ps->wa0;
		ps->j = 0;
		ps->b = 0;
		ps->c = 0;
		ps->my = 0;
		ps->ly = 0;
		ps->lmy = 0;
		ps->dy = 0;
		ps->say = 0;
		ps->sdy = 0;
		ps->zc = 0;

		ps->st = 1;	/* Goto state 1 */

		sys_able |= ABLE_PLL_UNLOCKED;
	}

	/* Count the timer down */
	ps->dl--;

	/* Adjust the averaging factor */
	if (ps->j < ps->mj)
		ps->j++;

	/* Update exponential estimate of y */
	ps->my += (ps->y - ps->my) / ps->j;

	/* Update zero-cross timer */
	if (ps->lmy * ps->my < 0) {
		ps->zc = 0;
	} else {
		ps->zc++;
	}
	ps->lmy = ps->my;

	/* Calculate slope estimates */
	ps->ddy = ps->dy;
	ps->dy = ps->y - ps->ly;
	ps->sdy += (fabs(ps->dy) - ps->sdy) / ps->j;

	/*
	 * Calculate allan variance estimate (tau=1sec)
	 * We use this number as an estimator of the jitter of our reference
	 * signal, and more or less assume that this is the worst noise 
	 * process it has.
	 */
	ps->ay = fabs(ps->dy - ps->ddy);
	ps->say += (ps->ay - ps->say) / ps->j;

	ps->ly = ps->y;

	/* Ring up our corrections */
	ps->a = ps->y * ps->wa;
	if (ps->st >= 3) 
		ps->b += ps->y * pow(ps->wa, ps->be);
	if (ps->st >= 5) {
		ps->b += ps->c;
		ps->c += ps->y * pow(ps->wa, ps->ce);
	}

	/*
	 * The total correction is the sum of the 1st and 2nd order
	 * corrections.  We accumulate the 3rd order into the 2nd order
	 * because that makes the 2nd order a true frequency estimate.
	 */
	ps->z = ps->a + ps->b0 + ps->b;

	/*
	 * If we have not seen any zero-crossings in a long time
	 * we have probably tightened the screw too hard, and need
	 * to loosen it.  If we knew just how much we would need to
	 * loosen it, we could do that, but we don't, and it would
	 * take us a long time to find out "from the underside".
	 * Instead, jack the limit up a notch, and restart the PLL.
	 * This converges fast because the error is small still, and
	 * next time we know to not tighten the screw harder than this.
	 * The amout we jack the limit up, should be larger than the
	 * amount we bang the ratio down (see later) in order to ensure
	 * convergence.
	 * XXX: .8 should be smaller than the .9 further down
	 * XXX: 15.0 is empirical.
	 *
	 * The second condition tries to capture rapid excursions without
	 * waiting for the absense of zero-crossings to alert us.  In a
	 * stable situation, the average magnitude of the slope should 
	 * be very small, and if our average offset exceeds it, things
	 * are certainly not all right.
	 * 
	 */
	if (ps->st >= 4 && 
	   ((ps->zc > 200.0 / ps->wa) || (fabs(ps->my) > 20 * ps->sdy))) {
		if (ps->wamin / .8 < ps->wamax)
			ps->wamin = ps->wa / .8;
		ps->st = 0;
		sys_able |= ABLE_PLL_UNLOCKED;
		return (ps->z);
	}

	/* 
	 * If the running average is is more than twice the limit we use
	 * for tightening (10% of sdy) we rearm the timer and wait for
	 * better news.
	 */
	if (ps->st >= 4 && (fabs(ps->my) > ps->sdy * .2))
		ps->dl = ps->mj;

	/*
	 * Wait for our timer to expire before we fiddle the PLL.
	 */
	if (ps->dl >= 0)
		return (ps->z);
	/*
	 * In state 1 we wait for the 1st order term to converge the
	 * offset.  Once inside the window defined by the noise (as
	 * represented by sdy) and the expected worst case frequency
	 * offset of 128PPM, we are almost close enough to let the 2nd
	 * order term loose, go to state 2 and set a timer.
	 * XXX: 128PPM is a changable constant.
	 */
	if (ps->st == 1 && fabs(ps->y) < (ps->sdy + 128e-6 / ps->wa)) {
		ps->st = 2;
		ps->dl = ps->mj;
		return (ps->z);
	}

	/*
	 * When the timer expires in state 2 we go to state 3 and set
	 * a timer.
	 */
	if (ps->st == 2) {
		ps->st = 3;
		ps->dl = ps->mj;
		sys_able &= ~ABLE_PLL_UNLOCKED;
		return (ps->z);
	}

	/*
	 * In state 3 the 2nd order term grabs hold and should bring the
	 * offset very close to zero (with any frequency drift induced
	 * offset being the ignorable epsilon).  We wait for the timer
	 * to expire and the average offset to be less than half the
	 * allan variance (for sigma=1) and for a zero crossing.
	 * Once we are there, we go to state 4 where we can tighten
	 * the screw gradually.
	 * XXX: the .5 is emperical
	 */
	if (ps->st == 3 && fabs(ps->my) < ps->say * .5 && ps->zc < 10) {
		ps->st = 4;
		ps->dl = ps->mj;
		return (ps->z);
	}

	if (ps->st == 4 && ps->wa < ps->go3)
		ps->st = 5;

	/*
	 * In state 4 and 5 we try to tighten the screw on the PLL as far
	 * as we can.  If we get too far, it fails to converge and the two
	 * trigger conditions above reset the PLL with the added wisdom
	 * recorded.  We only attempt to tighten the screw if our average
	 * offset is less than 10% of the allan estimate and we have just
	 * experienced a zero crossing.  If we do not see a zero-crossing
	 * we have probably already gone too far, and should not go any
	 * further since that would delay the recovery trigger.  We do of
	 * course never tighten beyond the limit we have learned.  If we
	 * have reached the limit, we start the 3rd order loop instead.
	 * Set the timer to let things stabilize.
	 * XXX: .1 is emperical.
	 * XXX: .9 is emperical and must be bigger than the .8 above.
	 * XXX: 10 is emperical.
	 */
	if (ps->st >= 4 && fabs(ps->my) < ps->say * .1 && ps->zc < 10) {
		if (ps->wa > ps->wamin) {
			ps->wa *= .9;
			ps->mj /= .8;
			ps->dl = ps->mj;
		} else {
			ps->dl = ps->mj;
		}
		if (ps->st >= 5)
			ps->dl *= ps->t3;
		return (ps->z);
	}
	return (ps->z);
}

#if 0
int
show_pllmath(CK_ARG)
{
	struct pll_state *ps = &pll_state;

	if (!(cfa->flag & CK_SHOW))
		return (0);
	ck_printf(CK_PASS, "st	%11d*	PLL state\n", ps->st);
	ck_printf(CK_PASS, "a	%11.4e	1. order correction\n", ps->a);
	ck_printf(CK_PASS, "b	%11.4e	2. order correction\n", ps->b);
	ck_printf(CK_PASS, "c	%11.4e	3. order correction\n", ps->c);
	ck_printf(CK_PASS, "z	%11.4e	Total correction\n", ps->z);
	ck_printf(CK_PASS, "wa	%11.4e	1. order ratio\n", ps->wa);
	ck_printf(CK_PASS, "wb	%11.4e	2. order ratio (wa**be)\n",
	    pow(ps->wa, ps->be));
	ck_printf(CK_PASS, "wc	%11.4e	3. order ratio (wa**ce)\n",
	    pow(ps->wa, ps->ce));
	ck_printf(CK_PASS, "j	%11d	Averaging factor\n", ps->j);
	ck_printf(CK_PASS, "mj	%11.0f	Max averaging factor\n", ps->mj);
	ck_printf(CK_PASS, "dl	%11d	Delay timer\n", ps->dl);
	ck_printf(CK_PASS, "zc	%11d	Zero Cross timer\n", ps->zc);
	ck_printf(CK_PASS, "y	%11.4e	Phase error measurement\n", ps->y);
	ck_printf(CK_PASS, "ly	%11.4e	Last y\n", ps->ly);
	ck_printf(CK_PASS, "my	%11.4e	Averaged y\n", ps->my);
	ck_printf(CK_PASS, "lmy	%11.4e	Last my\n", ps->lmy);
	ck_printf(CK_PASS, "dy	%11.4e	Two-sample y difference\n", ps->dy);
	ck_printf(CK_PASS, "ddy	%11.4e	Previous dy\n", ps->ddy);
	ck_printf(CK_PASS, "sdy	%11.4e	Average magnitude of dy\n", ps->sdy);
	ck_printf(CK_PASS, "ay	%11.4e	Avar sigma=1\n", ps->ay);
	ck_printf(CK_PASS, "say	%11.4e	Averaged ay\n", ps->say);
	ck_printf(CK_PASS, "mj0	%11.4e*	Initial mj\n", ps->mj0);
	ck_printf(CK_PASS, "dl0	%11.4e*	Initial dl\n", ps->dl0);
	ck_printf(CK_PASS, "wa0	%11.4e*	Initial wa\n", ps->wa0);
	ck_printf(CK_PASS, "wamin	%11.4e*	limit 1. order ratio\n",
	    ps->wamin);
	ck_printf(CK_PASS, "wamax	%11.4e*	limit 1. order ratio\n",
	    ps->wamax);
	ck_printf(CK_PASS, "b0	%11.4e*	Configured frequency (b) offset\n",
	    ps->b0);
	ck_printf(CK_PASS, "be	%11.4e*	2. order exponent\n", ps->be);
	ck_printf(CK_PASS, "ce	%11.4e*	3. order exponent\n", ps->ce);
	ck_printf(CK_PASS, "go3	%11.4e*	wa threshold for 3. order\n", ps->go3);
	ck_printf(CK_PASS, "t3	%11.4e*	dl expansion in 3. order mode\n", ps->t3);
	return (0);
}
#endif

/* Defaults defined post factum to avoid unintentional leakage */

#define DEF_mj0		8	/* XXX: empirical */
#define DEF_dl0		10	/* XXX: empirical */
#define DEF_wa0		.1	/* XXX: empirical */
#define DEF_b0		0.0 
#define DEF_be		2	/* XXX: empirical */
#define DEF_ce		4	/* XXX: empirical */
#define DEF_go3		.001
#define DEF_t3		10
#define DEF_wamin	0
#define DEF_wamax	DEF_wa0

#if 0
static void
cfg_pllmath_var(CK_ARG, const char **key, const char *name, double *val, double *nval, double def)
{
	char *p;

	if (key == NULL || !strcmp(*key, name)) {
		p = cfa->tree;
                asprintf(&cfa->tree, "%s %s", p, name);
		ck_RealVar(CK_PASS, nval, val, def, "%g");
		free(cfa->tree);
		cfa->tree = p;
	}
}

int
cfg_pllmath(CK_ARG, const char **key, double *val)
{
	struct pll_state *ps = &pll_state;

	cfg_pllmath_var(CK_PASS, key, "b0",    &ps->b0,    val, DEF_b0);
	cfg_pllmath_var(CK_PASS, key, "be",    &ps->be,    val, DEF_be);
	cfg_pllmath_var(CK_PASS, key, "ce",    &ps->ce,    val, DEF_ce);
	cfg_pllmath_var(CK_PASS, key, "mj0",   &ps->mj0,   val, DEF_mj0);
	cfg_pllmath_var(CK_PASS, key, "dl0",   &ps->dl0,   val, DEF_dl0);
	cfg_pllmath_var(CK_PASS, key, "go3",   &ps->go3,   val, DEF_go3);
	cfg_pllmath_var(CK_PASS, key, "t3",    &ps->t3,    val, DEF_t3);
	cfg_pllmath_var(CK_PASS, key, "wa0",   &ps->wa0,   val, DEF_wa0);
	cfg_pllmath_var(CK_PASS, key, "wamin", &ps->wamin, val, DEF_wamin);
	cfg_pllmath_var(CK_PASS, key, "wamax", &ps->wamax, val, DEF_wamax);
	if (key != NULL && !strcmp(*key, "restart") &&
	    !(cfa->flag & (CK_NO|CK_SHOW|CK_CONF))) {
		pll_reset();
	}

	return (0);
}
#endif

void
init_pllmath(void)
{
	struct pll_state *ps = &pll_state;

	ps->b0 =  DEF_b0;
	ps->be =  DEF_be;
	ps->ce =  DEF_ce;
	ps->mj0 = DEF_mj0;
	ps->dl0 = DEF_dl0;
	ps->go3 = DEF_go3;
	ps->t3 =  DEF_t3;
	ps->wa0 = DEF_wa0;
	ps->wamin = DEF_wamin;
	ps->wamax = DEF_wamax;
	//ck_TreeAdd(cft, &config_pllmath);
}
