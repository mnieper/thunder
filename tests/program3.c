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
#include <assert.h>
#include <stdio.h>

// TODO: should become compiler.c-Test

#include "localcharset.h"
#include "parser.h"
#include "program.h"
#include "uniconv.h"
#include "unitypes.h"
#include "vmcommon.h"

static Heap heap;

Object
read_string (uint8_t *b)
{
  char *s = u8_strconv_to_encoding (b, locale_charset (),
				    iconveh_question_mark);
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
    ("((frob)\n"
     " (frob)\n"
     " (jmp 3)\n"
     " (phi 0 9)\n"
     " (phi 0 10)\n"
     " (phi 1 5)\n"
     " (frob)\n"
     " (frob)\n"
     " (switch 9 12 13)\n"
     " (frob)\n"
     " (movr 3)\n"
     " (jmp 3)\n"
     " (jmp 14)\n"
     " (jmp 14)\n"
     " (phi 6 7)\n"
     " (jmp 16)\n"
     " (phi 4 19)\n"
     " (phi 14 20)\n"
     " (perform 4)\n"
     " (movr 16)\n"
     " (movr 17)\n"
     " (br 23 22)\n"
     " (jmp 16)\n"
     " (ret 20))\n");

  Program p;
  program_init (&p);

  parse_code (&p, code);

  /*
  split_critical_edges (&p);
  init_postorder (&p);
  phi_lift (&p);
  init_liveness (&p);
  */

  translate_into_cssa (&p);
  init_postorder (&p);
  init_liveness (&p);

  //  program_dump (&p, "debug.dot");

  program_destroy (&p);

  code = read_string
    ("((jmp 1)\n"
     " (br 10 2)\n"
     " (br 7 3)\n"
     " (jmp 4)\n"
     " (jmp 5)\n"
     " (br 6 4)\n"
     " (jmp 1)\n"
     " (jmp 8)\n"
     " (br 9 5)\n"
     " (jmp 7)\n"
     " (ret))\n");

  program_init (&p);
  parse_code (&p, code);
  init_dfs (&p);

  init_liveness2 (&p);

  // tests

  program_dump (&p, "debug.dot");

  program_destroy (&p);

  heap_destroy (&heap);
}
