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
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

#include "error.h"
#include "vmcommon.h"

bool
is_list (Object obj)
{
  while (!is_null (obj))
    {
      if (!is_pair (obj))
	return false;
      obj = cdr (obj);
    }
  return true;
}

size_t
length (Object list)
{
  size_t n;
  for (n = 0; !is_null (list); list = cdr (list))
    ++n;
  return n;
}

Object
exact_number (Heap *heap, long int n, unsigned long int d)
{
  mpq_t q;
  mpq_init (q);
  mpq_set_si (q, n, d);
  mpq_canonicalize (q);
  Object res = make_exact_number (heap, q);
  mpq_clear (q);
  return res;
}

int
fixnum (Object number)
{
  return mpz_get_si (mpq_numref (*exact_number_value (number))); 
}

Object
runtime_list (Heap *heap, ...)
{
  Object list = make_null ();
  
  va_list ap;
  va_start (ap, heap);

  Object pair, obj;
  while ((obj = va_arg (ap, Object)) != NONE)
    {
      if (is_null (list))
	pair = list = cons (heap, obj, make_null ());
      else
	{      
	  set_cdr (heap, pair, cons (heap, obj, make_null ()));
	  pair = cdr (pair);
	}
    }
  
  va_end (ap);
  return list;
}

void
assert_list (Object obj)
{
  if (is_list (obj))
    return;

  if (is_pair (obj))
    error (EXIT_FAILURE, 0, "%s: %s", "dotted list in source", object_get_str (obj));
  
  error (EXIT_FAILURE, 0, "%s: %s", "not a list", object_get_str (obj));
}

void
assert_symbol (Object obj)
{
  if (!is_symbol (obj))
    error (EXIT_FAILURE, 0, "%s: %s", "not a symbol", object_get_str (obj));
}

void
assert_procedure (Object obj)
{
  if (!is_procedure (obj))
    error (EXIT_FAILURE, 0, "%s: %s", "not code", object_get_str (obj));
}

Object
copy_object (Object obj)
{
  /* FIXME(XXX): Return a deep copy of the object. */
  return obj;
}
