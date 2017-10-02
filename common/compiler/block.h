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

#ifndef COMPILER_BLOCK_H
#define COMPILER_BLOCK_H

#include "list.h"
#include "opcode.h"

#define variable_foreach(v, l)					\
  list_foreach (v, VariableList, Variable, variable_list, l)

typedef struct block Block;

struct compiler;
struct instruction;

Block *block_create (struct compiler *compiler);
void block_free (Block *block);

void block_add_successor (Block *source, Block *target);
void block_add_predecessor (Block *target, Block *source);
struct instruction *block_add_instruction (struct compiler *compiler,
					   Block *block,
					   const Opcode *opcode);

DEFINE_LIST(BlockList, Block, block_list, block_free);

#endif /* COMPILER_BLOCK_H */
