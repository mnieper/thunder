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
#include <string.h>

#include "localcharset.h"
#include "macros.h"
#include "reader.h"
#include "uniconv.h"
#include "unitypes.h"
#include "vmcommon.h"

static  Heap heap;

bool
check_number (uint8_t const *s, int radix)
{
  Object obj = read_number (&heap, s, strlen (s), radix);
  bool res = !is_boolean (obj);
  return res;
}

bool
check_datum (uint8_t *b, bool (* predicate) (Object obj))
{
  char *s = u8_strconv_to_encoding (b, locale_charset (), iconveh_question_mark); 
  FILE *in = fmemopen (s, strlen (s), "r");

  Reader reader;
  reader_init (&reader, &heap, in);

  Object obj;
  bool res = reader_read (&reader, &obj, NULL, NULL);
  
  if (res)
    ASSERT (predicate (obj));

  reader_destroy (&reader);

  fclose (in);
  free (s);

  return res;
}

int main (int argc, char *argv[])
{
  setlocale (LC_ALL, "");

  heap_init (&heap, 1ULL << 20);

  ASSERT (check_datum (u8"#f", is_boolean));
  ASSERT (check_datum (u8"#\\space", is_char));
  ASSERT (check_datum (u8"#\\ö", is_char));
  ASSERT (check_datum (u8"#(#\\a #\\b #\\c)", is_vector));
  ASSERT (check_datum (u8"\"string\"", is_string));
  ASSERT (check_datum (u8"symbol", is_symbol));
  ASSERT (check_datum (u8"()", is_null));
  ASSERT (check_datum (u8"(1 . 2)", is_pair));
  ASSERT (check_datum (u8"'x", is_pair));
  ASSERT (check_datum (u8"125", is_exact_number));
  ASSERT (check_datum (u8"1.1", is_inexact_number));
  ASSERT (check_datum (u8"#i1/2", is_inexact_number));
  
  ASSERT (check_number (u8"42", 10));
  ASSERT (check_number (u8"1@1", 10));
  ASSERT (check_number (u8"1e3@2", 10));
  ASSERT (check_number (u8"3+3.4i", 10));
  ASSERT (check_number (u8"#e1", 10));
  ASSERT (check_number (u8"dead/beef", 16));
  ASSERT (check_number (u8"#xdead/beef", 10));
  
  ASSERT (!check_number (u8"1@1@1", 10));
  ASSERT (!check_number (u8"42(", 10));
  ASSERT (!check_number (u8"#e#i4", 10));
  ASSERT (!check_number (u8"4#e", 10));
  ASSERT (!check_number (u8"dead/beef", 10));
  
  heap_destroy (&heap);
}
