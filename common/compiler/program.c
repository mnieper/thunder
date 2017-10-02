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

void
program_init (Program *program)
{
  program->blocks = block_list_create (true);
  program->vars = variable_list_create (true);
}

void
program_destroy (Program *program)
{
  block_list_free (program->blocks);
  variable_list_free (program->vars);
}

Variable *
program_create_var (Compiler *compiler)
{
  Variable *var = variable_create (compiler);
  variable_list_add (compiler->program.vars, var);
  return var;
}

void
program_add_cfg_edge (Block *source, Block *target)
{
  block_add_successor (source, target);
  block_add_predecessor (target, source);
}
