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
#define dest_pos_foreach(var, pos, ins)		\
  variable_pos_foreach (var, pos, (ins)->dests)
#define source_pos_foreach(var, pos, ins)	\
  variable_pos_foreach (var, pos, (ins)->sources)

#define dest_source_foreach(dest, source, ins)				\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (VariableListIterator i##__LINE__				\
	   = variable_list_iterator ((ins)->dests);			\
	 b##__LINE__; )							\
      for (VariableListIterator j##__LINE__				\
	     = variable_list_iterator ((ins)->sources);			\
	   b##__LINE__;)						\
	for (Variable *(dest); b##__LINE__;				\
	     b##__LINE__ = false,					\
	       variable_list_iterator_free (&i##__LINE__),		\
	       variable_list_iterator_free (&j##__LINE__))		\
	  for (Variable *(source);					\
	       variable_list_iterator_next (&i##__LINE__, &dest, NULL)	\
		 && (variable_list_iterator_next (&j##__LINE__, &source, NULL) \
		     || true); )

typedef struct instruction Instruction;

struct compiler;
struct opcode;
struct variable;

Instruction *instruction_create (struct compiler *compiler,
				 struct opcode const *opcode);
void instruction_free (Instruction *ins);

Variable *instruction_dest (Instruction *ins);
static inline struct opcode const *instruction_opcode (Instruction *ins);

void instruction_add_source (Instruction *ins, struct variable *var);
void instruction_add_dest (Instruction *ins, struct variable *var);
void instruction_set_dest (Instruction *ins, struct variable *var);
void instruction_replace_source (Instruction *ins, VariableListPosition *pos,
				 Variable *var);
void instruction_replace_dest (Instruction *ins, VariableListPosition *pos,
			       Variable *var);
void instruction_replace_dests (Instruction *ins, VariableList new_dests);
void instruction_replace_sources (Instruction *ins, VariableList new_sources);

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

struct opcode const *instruction_opcode (Instruction *ins)
{
  return ins->opcode;
}

#endif /* COMPILER_INSTRUCTION_H */
