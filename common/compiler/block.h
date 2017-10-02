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

#include "compiler.h"
#include "opcode.h"

typedef struct block Block;

Block *block_create (Compiler *compiler);
void block_free (Block *block);

Instruction *block_add_instruction (Block *block, const Opcode *opcode);

#endif /* COMPILER_BLOCK_H */
