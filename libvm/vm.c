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

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <libthunder.h>

#include "error.h"
#include "vmcommon.h"
#include "xalloc.h"

struct vm
{
  Heap heap;
};

void
vm_init (void)
{
  init ();
}

Vm *
vm_create (void)
{
  
  Vm *vm = XMALLOC (struct vm);
  heap_init (&vm->heap, 1ULL << 30);
  return vm;
}

void
vm_free (Vm *vm)
{
  heap_destroy (&vm->heap);
  free (vm);
}

int
vm_load (Vm *vm, FILE *src, char const *filename)
{
  Object obj = load (&vm->heap, src, filename);

  if (!is_closure (obj))
    error_at_line (EXIT_FAILURE, 0, filename, 1, "not a thunder image");

  return closure_call (obj, 0);
}
