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

#include <stddef.h>

#include "block.h"
#include "variable.h"
#include "vector.h"

#define program_block_foreach(block, program)	\
  block_foreach (block, (program)->blocks)

typedef struct program Program;

struct compiler;

void program_init (Program *program);
void program_destroy (Program *program);

size_t block_count (Program *program);
static inline struct edge_vector *program_back_edges (Program *program);

Variable *program_create_var (struct compiler *compiler);
Block *program_create_block (struct compiler *compiler);
void program_add_cfg_edge (Block *source, Block *target);

#define PROGRAM_PREORDER(program)		\
  ((program)->preorder)
#define PROGRAM_POSTORDER(program)		\
  ((program)->postorder)
#define PROGRAM_DOMORDER(program)		\
  ((program)->domorder)

typedef struct edge Edge;
struct edge
{
  struct block *source;
  struct block *target;
};
DEFINE_VECTOR (EdgeVector, Edge, edge_vector)

struct program
{
  BlockList blocks;
  VariableList vars;
  Block **preorder;
  Block **postorder;
  Block **domorder;
  EdgeVector back_edges;
};

static inline EdgeVector *
program_back_edges (Program *program)
{
  return &program->back_edges;
}

#endif /* COMPILER_PROGRAM_H_INCLUDED */
