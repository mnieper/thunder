/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3, or (at your option)
 * any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>

#include "assure.h"
#include "common.h"

Variable *
variable_create (Compiler *compiler)
{
  Variable *var = COMPILER_ALLOC (compiler, Variable);
  use_chain_init (variable_use_chain (var));
  variable_vector_init (variable_congruence_class (var));
  return var;
}

void
variable_free (Variable *var)
{
  use_chain_destroy (variable_use_chain (var));
  variable_vector_destroy (variable_congruence_class (var));
}

bool
variable_def_dominates (Variable *v, Variable *w)
{
  assure (v != NULL);
  assure (w != NULL);

  if (block_dominates (VARIABLE_DEF_BLOCK (v), VARIABLE_DEF_BLOCK (w)))
    return true;
  if (VARIABLE_DEF_BLOCK (v) != VARIABLE_DEF_BLOCK (w))
    return false;
  return VARIABLE_DEF_TIME (v) <= VARIABLE_DEF_TIME (w);
}
