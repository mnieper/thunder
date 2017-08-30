/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thundr.
 *
 * Thunder is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Rapid Scheme.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <locale.h>
#include <stdbool.h>
#include <stdio.h>

#include "localcharset.h"
#include "macros.h"
#include "reader.h"
#include "uniconv.h"
#include "vmcommon.h"

static Heap heap;

Object
scheme_read (uint8_t *b)
{
  char *s = u8_strconv_to_encoding (b, locale_charset (), iconveh_question_mark); 
  FILE *in = fmemopen (s, strlen (s), "r");

  Reader reader;
  reader_init (&reader, &heap, in);

  Object obj;
  bool res = reader_read (&reader, &obj, NULL, NULL);

  ASSERT(res);
  
  reader_destroy (&reader);

  fclose (in);
  free (s);

  return obj;
}

bool
check_write (uint8_t *b, char const *s)
{
  char *p;
  size_t n;
  FILE *out = open_memstream (&p, &n);

  scheme_write (scheme_read (b), out);

  fclose (out);

  bool res = strcmp (p, s) == 0;
  free (p);
  return res;
}

int main (int argc, char *argv[])
{
  init ();
  
  setlocale (LC_ALL, "");

  heap_init (&heap, 1ULL << 20);

  ASSERT (check_write (u8"()", "()"));
  ASSERT (check_write (u8"#\\ ", "#\\space"));
  ASSERT (check_write (u8"#\\\n", "#\\newline"));
  ASSERT (check_write (u8"symbol", "symbol"));
  ASSERT (check_write (u8"\"str\\\\ing\"", "\"str\\\\ing\""));
  ASSERT (check_write (u8"#(a #(b c))", "#(a #(b c))"));
  ASSERT (check_write (u8"(a (b c))", "(a (b c))"));
  ASSERT (check_write (u8"(a (b . c) d)", "(a (b . c) d)"));
  ASSERT (check_write (u8"'x", "'x"));
  ASSERT (check_write (u8"10", "10"));
  ASSERT (check_write (u8"-2/6", "-1/3"));
  ASSERT (check_write (u8"#i3", ".3e1"));
  ASSERT (check_write (u8"+3e0i", "+.3e1i"));
  ASSERT (check_write (u8".3", ".3"));
  
  heap_destroy (&heap);
}
