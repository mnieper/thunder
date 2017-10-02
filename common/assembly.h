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

#ifndef ASSEMBLY_H_INCLUDED
#define ASSEMBLY_H_INCLUDED

#include <lightning.h>

#include "obstack.h"

struct assembly
{
  jit_state_t *jit;
  struct obstack data;
  jit_pointer_t *entry_points;
  size_t entry_point_count;
  bool clear : 1;
};
typedef struct assembly Assembly[1];

void assembly_init (Assembly assembly);
void assembly_destroy (Assembly assembly);
void assembly_clear (Assembly assembly);
static inline jit_state_t *assembly_jit (Assembly assembly);

jit_state_t *
assembly_jit (Assembly assembly)
{
  return assembly->jit;
}

#endif /* ASSEMBLY_H_INCLUDED */
