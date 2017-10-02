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

#ifndef COMPILER_INSTRUCTION_H_INCLUDED
#define COMPILER_INSTRUCTION_H_INCLUDED

#include <stdbool.h>
#include <stdio.h>

#include "compiler/common.h"
#include "vmcommon.h"

#define instruction_foreach(ins, block)				\
  deque_foreach (ins, INSTRUCTIONS, &block->instructions)

void init_instructions (void);
void finish_instructions (void);

Instruction *new_instruction (Compiler *compiler, Object stmt);
Instruction *new_jump (Compiler *compiler, Block *target);
Instruction *instruction_next (Instruction *ins);

void instruction_patch (Compiler *compiler, Instruction *ins);

bool instruction_jmp (Instruction *ins);
bool instruction_branch (Instruction *ins);
bool instruction_terminating (Instruction *ins);

void instruction_out_str (FILE *out, Instruction *ins);

#endif /* COMPILER_INSTRUCTION_H_INCLUDED */
