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
#include <lightning.h>

#include "obstack.h"
#include "vmcommon.h"
#include "xalloc.h"

#define obstack_chunk_alloc xmalloc
#define obstack_chunk_free free

void
assembly_init (Assembly assembly)
{
  assembly->clear = true;
}

void
assembly_clear (Assembly assembly)
{
#define _jit (assembly->jit)
  assembly_destroy (assembly);
  assembly->jit = jit_new_state ();
  obstack_init (&assembly->data);
#undef _jit
}

void
assembly_destroy (Assembly assembly)
{
#define _jit (assembly->jit)
  if (assembly->clear)
    return;

  jit_destroy_state ();
  obstack_free (&assembly->data, NULL);
  free (assembly->entry_points);

  assembly->clear = true;
#undef _jit
}
