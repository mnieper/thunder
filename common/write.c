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
#include <stdbool.h>
#include <stdio.h>
#include <gmp.h>
#include <mpfr.h>

#include "localcharset.h"
#include "stack.h"
#include "uniconv.h"
#include "unistdio.h"
#include "unistr.h"
#include "vmcommon.h"
#include "xmalloca.h"

static void
write_exact_number (Object obj, FILE *out)
{
  mpq_out_str (out, 10, *exact_number_value (obj));
}

static void
write_real_number (mpfr_t x, FILE *out, bool ensure_sign)
{
  if (mpfr_nan_p (x))
    fputs ("+nan.0", out);
  else if (mpfr_inf_p (x))
    fputs (mpfr_sgn (x) > 0 ? "+inf.0" : "-inf.0", out);
  else
    {
      mpz_t mant;
      mpz_init (mant);
      long int point = get_z_10exp (mant, x);

      switch (mpz_sgn (mant))
	{
	case 0:
	case 1:
	  if (ensure_sign)
	    fputc ('+', out);
	  break;
	case -1:
	  fputc ('-', out);
	  mpz_neg (mant, mant);
	  break;
	}
      char *s = mpz_get_str (NULL, 10, mant);
      size_t len = strlen (s);
      point += len;
      for (size_t i = len - 1; i > 0; --i)
	if (s[i] == '0')
	  s[i] = '\0';
      fputc ('.', out);
      fputs (s, out);
      if (point != 0)
	{
	  fputc ('e', out);
	  fprintf (out, "%ld", point);
	}
      free (s);
      mpz_clear (mant);
    }
}

static void
write_inexact_number (Object obj, FILE *out)
{
  mpc_t *x = inexact_number_value (obj);
  if (mpfr_zero_p (mpc_realref (*x)))
    {
      write_real_number (mpc_imagref (*x), out, true);
      fputc ('i', out);
    }
  else
    {
      write_real_number (mpc_realref (*x), out, false);
      if (!mpfr_zero_p (mpc_imagref (*x)))
	{
	  write_real_number (mpc_imagref (*x), out, true);
	  fputc ('i', out);
	}
    }
}

void
write_char (ucs4_t c, FILE *out)
{
  switch (c)
    {
    case 10:
      ulc_fprintf (out, "#\\newline");
      break;
    case 32:
      ulc_fprintf (out, "#\\space");
      break;
    default:
      {
	uint32_t s[2] = { c, 0 };
	ulc_fprintf (out, "#\\%llU", s);
      }
    }
}

static bool
write_abbreviation (Object *obj, FILE *out)
{
  if (!is_pair (*obj))
    return false;
 
  Object first = car (*obj);
  if (!is_pair (cdr (*obj)) || !is_null (cddr (*obj)))
    return false;
    
  if (first == SYMBOL(QUOTE))
    {
      fputc ('\'', out);
      *obj = cadr (*obj);
      return true;
    }
  if (first == SYMBOL(QUASIQUOTE))
    {
      fputc ('`', out);
      *obj = cadr (*obj);
      return true;
    }
  if (first == SYMBOL(UNQUOTE))
    {
      fputc (',', out);
      *obj = cadr (*obj);
      return true;
    }
  if (first == SYMBOL(UNQUOTE_SPLICING))
    {
      fputs (",@", out);
      *obj = cadr (*obj);
      return true;
    }

  return false;
}
  
void
scheme_write (Object obj, FILE *out)
{
  struct frame
  {
    Object obj;
    size_t index;
  };

  STACK(struct frame) stack;
  stack_init (&stack);
  
  do
    {
      check_defined (obj);

      while (write_abbreviation (&obj, out));
      
      if (is_boolean (obj))
	{
	  if (boolean_value (obj))
	    fputs ("#t", out);
	  else
	    fputs ("#f", out);
	}
      else if (is_char (obj))
	write_char (char_value (obj), out);
      else if (is_null (obj))
	fputs ("()", out);
      else if (is_eof_object (obj))
	fputs ("#<eof>", out);
      else if (is_closure (obj))
	fputs ("#<procedure>", out);
      else if (is_symbol (obj))
	{
	  size_t len;
	  char *s = u8_conv_to_encoding (locale_charset (),
					 iconveh_question_mark,
					 symbol_bytes (obj),
					 symbol_length (obj),
					 NULL,
					 NULL,
					 &len);
	  fwrite (s, 1, len, out);
	  free (s);
	}
      else if (is_string (obj))
	{
	  fputs ("\"", out);
	  uint32_t *s = string_bytes (obj);
	  size_t n = string_length (obj);
	  while (n > 0)
	    {
	      uint32_t b[2] = { [1] = 0 };
	      int len = u32_mbtouc_unsafe (&b[0], s, n);
	      n -= len;
	      s += len;
	      switch (b[0])
		{
		case 0x22:
		  fputs ("\\\"", out);
		case 0x5c:
		  fputs ("\\\\", out);
		  break;
		default:
		  ulc_fprintf (out, "%llU", b);
		}
	    }
	  fputs ("\"", out);
	}
      else if (is_exact_number (obj))
	write_exact_number (obj, out);
      else if (is_inexact_number (obj))
	write_inexact_number (obj, out);
      else if (is_pair (obj))
	{
	  fputc ('(', out);
	  stack_push (&stack, ((struct frame) { .obj = obj, .index = 0 }));
	}
      else if (is_vector (obj))
	{
	  fputs ("#(", out);
	  stack_push (&stack, ((struct frame) { .obj = obj, .index = 0 }));
	}
      else
	fputs ("#<unknown>", out);

      while (!stack_is_empty (&stack))
	{
	  struct frame *frame = &stack_top (&stack);
	  if (is_pair (frame->obj))
	    {
	      switch (frame->index)
		{
		case 0:
		  obj = car (frame->obj);
		  frame->index = 1;
		  break;
		case 1:
		  obj = cdr (frame->obj);
		  if (is_pair (obj))
		    {
		      fputc (' ', out);
		      frame->obj = cdr (frame->obj);
		      frame->index = 0;
		      continue;
		    }
		  if (!is_null (obj))
		    {
		      fputs (" . ", out);
		      obj = cdr (frame->obj);
		      frame->index = 2;
		      break;
		    }
		  /* fallthrough */
		case 2:
		  fputc (')', out);
		  stack_pop (&stack);
		  continue;
		}
	      break;
	    }
	  else
	    {
	      if (frame->index == vector_length (frame->obj))
		{
		  fputc (')', out);
		  stack_pop (&stack);
		  continue;
		}
	      else
		{
		  if (frame->index > 0)
		    fputc (' ', out);
		  obj = vector_ref (frame->obj, frame->index);
		  frame->index += 1;
		}
	    }
	  break;
	}
    }
  while (!stack_is_empty (&stack));

  stack_destroy (&stack);
}

char *
object_get_str (Object obj)
{
  char *s;
  size_t n;
  FILE *out = open_memstream (&s, &n);
  scheme_write (obj, out);
  fclose (out);
  return s;
}
