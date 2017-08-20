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

#include "vmcommon.h"
#include "xalloc.h"

struct vm
{
  Heap heap;
};

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
