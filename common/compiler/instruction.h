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

#ifndef COMPILER_INSTRUCTION_H
#define COMPILER_INSTRUCTION_H

#include "list.h"
#include "variable.h"

#ifdef DEBUG
# include <stdio.h>
#endif

#define instruction_foreach(i, l)					\
  list_foreach (i, InstructionList, Instruction, instruction_list, l)

#define dest_foreach(var, ins)			\
  variable_foreach (var, (ins)->dests)
#define source_foreach(var, ins)		\
  variable_foreach (var, (ins)->sources)

typedef struct instruction Instruction;

struct compiler;
struct opcode;
struct variable;

Instruction *instruction_create (struct compiler *compiler,
				 struct opcode const *opcode);
void instruction_free (Instruction *ins);

Variable *instruction_dest (Instruction *ins);

void instruction_add_source (Instruction *ins, struct variable *var);
void instruction_add_dest (Instruction *ins, struct variable *var);
void instruction_set_dest (Instruction *ins, struct variable *var);
void instruction_replace_source (Instruction *ins, VariableListPosition *pos,
				 Variable *var);

#ifdef DEBUG
void instruction_out_str (FILE *out, Instruction *ins);
#endif

DEFINE_LIST (InstructionList, Instruction, instruction_list, instruction_free)

struct instruction
{
  struct opcode const *opcode;
  VariableList dests;
  VariableList sources;
};

#endif /* COMPILER_INSTRUCTION_H */
