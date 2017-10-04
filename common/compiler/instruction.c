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

#include "common.h"

#ifdef DEBUG
# include <stdio.h>
#endif

Instruction *
instruction_create (Compiler *compiler, Opcode const *opcode)
{
  Instruction *ins = COMPILER_ALLOC (compiler, Instruction);
  ins->dests = variable_list_create (false);
  ins->sources = variable_list_create (false);
  ins->opcode = opcode;
  return ins;
}

void
instruction_free (Instruction *ins)
{
  variable_list_free (ins->dests);
  variable_list_free (ins->sources);
}

Variable *instruction_dest (Instruction *ins)
{
  return variable_list_first (ins->dests);
}

void
instruction_add_source (Instruction *ins, Variable *var)
{
  variable_list_add (ins->sources, var);
}

void
instruction_add_dest (Instruction *ins, Variable *var)
{
  variable_list_add (ins->dests, var);
}

void
instruction_set_dest (Instruction *ins, struct variable *var)
{
  variable_list_set_first (ins->dests, var);
}

void
instruction_replace_source (Instruction *ins, VariableListPosition *pos,
			    Variable *var)
{
  variable_list_replace (ins->sources, pos, var);
}

#ifdef DEBUG
void
instruction_out_str (FILE *out, Instruction *ins)
{
  dest_foreach (var, ins)
    fprintf (out, "%zu ", VARIABLE_INDEX (var));
  fprintf (out, "<- ");
  opcode_out_str (out, ins->opcode);
  source_foreach (var, ins)
    fprintf (out, " %zu", VARIABLE_INDEX (var));
  fprintf (out, "\n");
}
#endif
