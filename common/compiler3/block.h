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

#ifndef COMPILER_BLOCK_H_INCLUDED
#define COMPILER_BLOCK_H_INCLUDED

#include <stddef.h>

#include "compiler/common.h"

Block *new_block (Compiler *compiler, Object name);
void block_append_instruction (Block *block, Instruction *ins);
Instruction *block_end (Block *block);

Link *new_link (Compiler *compiler, Value *source);
void block_add_link (Compiler *compiler, Block *block, Value *source);

#endif /* COMPILER_BLOCK_H_INCLUDED */
