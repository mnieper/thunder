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

#include "assure.h"
#include "common.h"

#ifdef DEBUG
# include <stdio.h>
#endif

void
program_init (Program *program)
{
  program->blocks = block_list_create (true);
  program->vars = variable_list_create (true);
  edge_vector_init (&program->back_edges);
}

void
program_destroy (Program *program)
{
  block_list_free (program->blocks);
  variable_list_free (program->vars);
  edge_vector_destroy (&program->back_edges);
}

size_t
block_count (Program *program)
{
  return block_list_size (program->blocks);
}

Block *
program_create_block (Compiler *compiler)
{
  Block *block = block_create (compiler);
  block_list_add (compiler->program.blocks, block);
  return block;
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

#ifdef DEBUG
void
program_dump (Program *program, char const *path)
{
  FILE *out = fopen (path, "w");
  assure (out != NULL);
  program_block_foreach (block, program)
    {
      block_out_str (out, block);
      fprintf (out, "\n");
    }
  program_variable_foreach (var, program)
    {
      fprintf (out, "VARIABLE (domindex=%tu, congruence=%tu)",
	       VARIABLE_INDEX (var),
	       VARIABLE_INDEX (VARIABLE_CONGRUENCE (var)));
      fprintf (out, "\n");
    }
  fclose (out);
}
#endif