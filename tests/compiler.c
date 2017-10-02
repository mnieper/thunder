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
#include <locale.h>
#include <stdio.h>

#include "assert.h"
#include "localcharset.h"
#include "macros.h"
#include "uniconv.h"
#include "unitypes.h"
#include "vmcommon.h"

static Heap heap;

Object
read_string (uint8_t *b)
{
  char *s = u8_strconv_to_encoding (b, locale_charset (), iconveh_question_mark);
  FILE *in = fmemopen (s, strlen (s), "r");

  Reader reader;
  reader_init (&reader, &heap, in);

  Object obj;
  assert (reader_read (&reader, &obj, NULL, NULL));

  reader_destroy (&reader);

  fclose (in);
  free (s);

  return obj;
}

int
main (int argc, char *argv)
{
  init ();

  heap_init (&heap, 1ULL << 16);

  Object code = read_string
    ("(entry\n"
     " (add %0 1 1)\n"
     " (beq label2 %0 1)\n"
     " label1\n"
     " (add %1 %0 %0)\n"
     " (beq label1 1 2)\n"
     " label2\n"
     " (ret 0))\n");

  Object obj = compile (&heap, code);
  ASSERT (is_assembly (obj));

  heap_destroy (&heap);
}
