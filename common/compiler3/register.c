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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdbool.h>
#include <string.h>

#include "bitrotate.h"
#include "compiler/error.h"
#include "compiler/common.h"

#define REGISTER(p) ((struct reg const *) p)

static bool
register_comparator (void const *entry1, void const *entry2)
{
  return REGISTER(entry1)->name == REGISTER(entry2)->name;
}

static size_t
register_hasher (void const *entry, size_t n)
{
  size_t val = rotr_sz ((size_t) REGISTER(entry)->name, 3);
  return val % n;
}

#undef REGISTER

void
init_registers (Compiler *compiler)
{
  compiler->registers = hash_initialize (0, NULL, register_hasher,
					 register_comparator, NULL);
}

void
finish_registers (Compiler *compiler)
{
  hash_free (compiler->registers);
}

Register *
register_intern (Compiler *compiler, Object name, Instruction *def)
{
  Register* reg = ALLOC (Register);
  reg->name = name;
  Register* interned_reg = hash_insert (compiler->registers, reg);
  if (interned_reg == NULL)
    xalloc_die ();
  if (reg != interned_reg)
    {
      if (def != NULL)
	{
	  if (interned_reg->def != NULL)
	    compiler_error ("%s: %s", "redefinition of register",
			    object_get_str (name));
	  interned_reg->def = def;
	}
      return interned_reg;
    }

  reg->def = def;
  return reg;
}

static bool
check_register_processor (Register *reg, Compiler *compiler)
{
  if (reg->def == NULL)
    compiler_error ("%s: %s", "undefined register",
		    object_get_str (reg->name));
  return true;
}

void
check_registers (Compiler *compiler)
{
  hash_do_for_each (compiler->registers, (Hash_processor) check_register_processor,
		    compiler);
}
