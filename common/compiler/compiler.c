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
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include "common.h"
#include "obstack.h"
#include "xalloc.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

void
compiler_init (Compiler *compiler)
{
  obstack_init (&compiler->obstack);
  program_init (&compiler->program);
}

void
compiler_destroy (Compiler *compiler)
{
  program_destroy (&compiler->program);
  obstack_free (&compiler->obstack, NULL);
}

void *
compiler_alloc (Compiler *compiler, size_t size)
{
  return obstack_alloc (&compiler->obstack, size);
}
