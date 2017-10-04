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

#ifndef COMPILER_OPCODE_H_INCLUDED
#define COMPILER_OPCODE_H_INCLUDED

#include "object.h"

typedef struct opcode Opcode;

struct compiler;
struct parser;
struct block;

void init_opcode (void);
void finish_opcode (void);

Opcode const* opcode_lookup (Object operator);
void opcode_parse (struct compiler *compiler, struct parser *parser,
		   struct block *block, const Opcode *opcode, Object operands);
Opcode const* get_opcode_movr (void);

#endif /* COMPILER_OPCODE_H_INCLUDED */
