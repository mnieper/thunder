/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 3, or (at
 * your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <string.h>

#include "minmax.h"
#include "vmcommon.h"

void
complex_init (mpc_t x)
{
  mpc_init2 (x, 53);
}

static void
round_quotient (mpz_t q, mpz_t num, mpz_t den)
{
  mpz_t r, d;
  mpz_inits (r, d, NULL);
  mpz_tdiv_qr (q, r, num, den);
  int c = mpz_even_p (q) ? 1 : 0;
  mpz_abs (r, r);
  mpz_mul_ui (r, r, 2);
  mpz_abs (d, den);
  if (mpz_cmp (r, d) >= c)
    {
      if (mpz_sgn (num) == mpz_sgn (den))
	mpz_add_ui (q, q, 1);
      else
	mpz_sub_ui (q, q, 1);
    }
  mpz_clears (r, d, NULL);
}

long int
get_z_10exp (mpz_t quo, mpfr_t inexact)
{
  if (mpfr_zero_p (inexact))
    {
      mpz_set_si (quo, 0);
      return 0;
    }

  /* TODO: Document and simplify code. */
  mpz_t mant, num, den, z;
  mpq_t q;
  mpfr_t x, y, lg2;
  long int point;
  mpz_inits (mant, num, den, z, NULL);
  mpq_init (q);
  mpfr_inits (x, y, lg2, NULL);
  mpfr_set_ui (lg2, 2, MPFR_RNDN);
  mpfr_log10 (lg2, lg2, MPFR_RNDN);
  mpfr_exp_t exp = mpfr_get_z_2exp (mant, inexact);
  if (exp > 0)
    {
      mpz_mul_2exp (num, mant, exp);
      mpfr_set_z (x, num, MPFR_RNDN);
      mpfr_log10 (x, x, MPFR_RNDN);
      mpfr_mul_ui (y, lg2, 53, MPFR_RNDN);
      mpfr_sub (x, x, y, MPFR_RNDN);
      point = MAX (0, mpfr_get_si (x, MPFR_RNDU));
      mpz_ui_pow_ui (den, 10, point);
      round_quotient (quo, num, den);
      mpz_mul (z, quo, den);
      mpfr_set_z (x, z,  MPFR_RNDN);
      if (mpfr_cmp (inexact, x) != 0)
	{
	  mpz_divexact_ui (den, den, 10);
	  round_quotient (quo, num, den);
	  ++point;
	}
    }
  else
    {
      mpz_set_ui (den, 1);
      mpz_mul_2exp (den, den, -exp);
      mpfr_mul_si (x, lg2, exp, MPFR_RNDN);
      point = mpfr_get_si (x, MPFR_RNDU);
      mpz_ui_pow_ui (z, 10, -point);
      mpz_mul (num, z, mant);
      round_quotient (quo, num, den);
      mpz_set (mpq_numref (q), quo);
      mpz_set (mpq_denref (q), z);
      mpq_canonicalize (q);
      mpfr_set_q (x, q, MPFR_RNDN);
      if (mpfr_cmp (inexact, x) != 0)
	{
	  mpz_mul_ui (z, z, 10);
	  mpz_mul_ui (num, num, 10);
	  round_quotient (quo, num, den);
	  point--;
	}      
    }
  mpz_clears (mant, num, den, z, NULL);
  mpq_clear (q);
  mpfr_clears (x, y, lg2, NULL);

  return point;
}

void
inexact_to_exact (mpq_t exact, mpfr_t inexact)
{
  mpz_t quo;
  mpz_init (quo);
  
  long int point = get_z_10exp (quo, inexact);
  if (point > 0)
    {
      mpz_ui_pow_ui (mpq_numref (exact), 10, point);
      mpz_mul (mpq_numref (exact), mpq_numref (exact), quo);
      mpz_set_si (mpq_denref (exact), 1);
    }
  else
    {
      mpz_ui_pow_ui (mpq_denref (exact), 10, -point);
      mpz_set (mpq_numref (exact), quo);
      mpq_canonicalize (exact);
    }

  mpz_clear (quo);
}
