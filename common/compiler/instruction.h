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
#include "opcode.h"
#include "variable.h"

typedef struct instruction Instruction;

Instruction *instruction_create (Compiler *compiler, Opcode const *opcode);
void instruction_free (Instruction *ins);

void instruction_add_source (Instruction *ins, Variable *var);
void instruction_add_dest (Instruction *ins, Variable *var);

DEFINE_LIST (InstructionList, Instruction, instruction_list, instruction_free)

#endif /* COMPILER_INSTRUCTION_H */
