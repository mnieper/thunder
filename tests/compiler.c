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

#include "assure.h"
#include "vmcommon.h"

static Heap heap;

Object
read_object (char const *path)
{
  FILE *in = fopen (path, "r");
  assure (in != NULL);

  Reader reader;
  reader_init (&reader, &heap, in);

  Object pair = make_null ();
  Object last_pair;
  Object obj;

  for (; reader_read (&reader, &obj, NULL, NULL), !is_eof_object (obj); )
    {
      if (is_null (pair))
	{
	  pair = cons (&heap, obj, make_null ());
	  last_pair = pair;
	}
      else
	{
	  set_cdr (&heap, last_pair, cons (&heap, obj, make_null ()));
	  last_pair = cdr (last_pair);
	}
    }

  reader_destroy (&reader);

  fclose (in);

  return pair;
}

int
main (void)
{
  init ();

  heap_init (&heap, 1ULL << 16);

  Object code = read_object ("tests/fact.ssa");

  Object obj = compile (&heap, code);
  assure (is_assembly (obj));

  heap_destroy (&heap);
}
