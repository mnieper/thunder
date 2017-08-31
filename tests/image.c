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
#include <string.h>

#include "macros.h"
#include "vmcommon.h"

bool
test_image (Heap *heap, char *s)
{
  FILE *in = fmemopen (s, strlen (s), "r");
  Object e = load (heap, in, NULL);
  fclose (in);

  char *buf;
  size_t n;
  
  FILE *out = open_memstream (&buf, &n);
  dump (e, out);
  fclose (out);

  // XXX
  fprintf (stderr, "Vorher: %s\nDazwischen: %s\n Nachher: %s\n", s, "", buf);
  
  bool res = strcmp (s, buf) == 0;
  free (buf);
  return res;
};

int
main (int argc, char *argv)
{
  init ();

  Heap heap;
  heap_init (&heap, 1ULL << 24);

  ASSERT(test_image (&heap, "'symbol\n"));
  ASSERT(test_image (&heap,
		     "(define $0 \"string\")\n"
		     "$0\n"));
  ASSERT(test_image (&heap,
		     "(define $1 (cons #f #f))\n"
		     "(define $0 (vector $1 $1))\n"
		     "(set-car! $1 $0)\n"
		     "(set-cdr! $1 $1)\n"
                     "$0\n"));
  ASSERT(test_image (&heap,
		     "(define $2 (code '((entry) (ret))))\n"
		     "(define $3 (closure $2 #f))\n"
		     "(define $1 (closure $2 #t))\n"
		     "(define $0 (cons $1 $3))\n"
		     "$0\n"));

  heap_destroy (&heap);
}
