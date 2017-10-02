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

#ifndef COMPILER_PROGRAM_H_INCLUDED
#define COMPILER_PROGRAM_H_INCLUDED

#include "block.h"
#include "variable.h"

typedef struct program Program;

struct compiler;

void program_init (Program *program);
void program_destroy (Program *program);

Variable *program_create_var (struct compiler *compiler);
Block *program_create_block (struct compiler *compiler);
void program_add_cfg_edge (Block *source, Block *target);

struct program
{
  BlockList blocks;
  VariableList vars;
};

#endif /* COMPILER_PROGRAM_H_INCLUDED */
