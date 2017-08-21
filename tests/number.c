/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <gmp.h>
#include <stddef.h>
#include <mpfr.h>

#include "vmcommon.h"
#include "macros.h"

int
main (int argc, char *argv)
{
  mpq_t q;
  mpfr_t x;
  
  mpq_init (q);
  mpfr_init (x);

  mpfr_set_str (x, "1", 10, MPFR_RNDN);
  inexact_to_exact (q, x);

  ASSERT (mpq_cmp_si (q, 1, 1) == 0);

  mpfr_set_str (x, "-1", 10, MPFR_RNDN);
  inexact_to_exact (q, x);
  ASSERT (mpq_cmp_si (q, -1, 1) == 0);

  mpfr_set_str (x, "0.1", 10, MPFR_RNDN);
  inexact_to_exact (q, x);
  ASSERT (mpq_cmp_si (q, 1, 10) == 0);

  mpfr_set_str (x, "1e18", 10, MPFR_RNDN);
  inexact_to_exact (q, x);
  ASSERT (mpq_cmp_si (q, 1000000000000000000LL, 1) == 0);

  mpfr_set_str (x, "1.448997445238699", 10, MPFR_RNDN);
  inexact_to_exact (q, x);
  ASSERT (mpq_cmp_si (q, 1448997445238699LL, 1000000000000000ULL) == 0);

  mpfr_set_str (x, "0.14285714285714282", 10, MPFR_RNDN);
  inexact_to_exact (q, x);
  ASSERT (mpq_cmp_si (q, 14285714285714282LL, 100000000000000000ULL) == 0);
  
  mpq_clear (q);
  mpfr_clear (x);
}
