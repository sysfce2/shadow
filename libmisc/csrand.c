/*
 * SPDX-FileCopyrightText:  Alejandro Colomar <alx@kernel.org>
 *
 * SPDX-License-Identifier:  BSD-3-Clause
 */

#include <config.h>

#ident "$Id$"

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#if HAVE_SYS_RANDOM_H
#include <sys/random.h>
#endif
#include "defines.h"
#include "prototypes.h"
#include "shadowlog.h"


/*
 * Return a uniformly-distributed CS random u_long value.
 */
unsigned long
csrand(void)
{
	FILE           *fp;
	unsigned long  r;

#ifdef HAVE_GETENTROPY
	/* getentropy may exist but lack kernel support.  */
	if (getentropy(&r, sizeof(r)) == 0)
		return r;
#endif

#ifdef HAVE_GETRANDOM
	/* Likewise getrandom.  */
	if (getrandom(&r, sizeof(r), 0) == sizeof(r))
		return r;
#endif

#ifdef HAVE_ARC4RANDOM_BUF
	/* arc4random_buf can never fail.  */
	arc4random_buf(&r, sizeof(r));
	return r;
#endif

	/* Use /dev/urandom as a last resort.  */
	fp = fopen("/dev/urandom", "r");
	if (NULL == fp) {
		goto fail;
	}

	if (fread(&r, sizeof(r), 1, fp) != 1) {
		fclose(fp);
		goto fail;
	}

	fclose(fp);
	return r;

fail:
	fprintf(log_get_logfd(), _("Unable to obtain random bytes.\n"));
	exit(1);
}


/*
 * Return a uniformly-distributed CS random value in the interval [0, n-1].
 *
 * Fast Random Integer Generation in an Interval
 * ACM Transactions on Modeling and Computer Simulation 29 (1), 2019
 * <https://arxiv.org/abs/1805.10941>
 */
unsigned long
csrand_uniform(unsigned long n)
{
	unsigned long      bound, rem;
	unsigned __int128  r, mult;

	if (n == 0)
		return csrand();

	bound = -n % n;  // analogous to `2^64 % n`, since `x % y == (x-y) % y`

	do {
		r = csrand();
		mult = r * n;
		rem = mult;  // analogous to `mult % 2^64`
	} while (rem < bound);  // p = (2^64 % n) / 2^64;  W.C.: n=2^63+1, p=0.5

	r = mult >> WIDTHOF(n);  // analogous to `mult / 2^64`

	return r;
}


/*
 * Return a uniformly-distributed CS random value in the interval [min, max].
 */
unsigned long
csrand_interval(unsigned long min, unsigned long max)
{
	return csrand_uniform(max - min + 1) + min;
}