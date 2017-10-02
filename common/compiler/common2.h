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

/* TODO(XXX): Use a compilation obstack to replace the xmallocs and
   frees in the code. */

#ifndef COMPILER_COMMON_H_INCLUDED
#define COMPILER_COMMON_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "bitset.h"
#include "list.h"
#include "vector2.h"
#include "block.h"
#include "compiler.h"
#include "opcode.h"
#include "parser.h"

typedef struct program Program;
typedef struct block Block;
typedef struct instruction Instruction;
typedef struct variable Variable;

void block_free (Block *);
void instruction_free (Instruction *);
void variable_free (Variable *);

DEFINE_LIST(BlockList, Block, block_list, block_free)
DEFINE_LIST(InstructionList, Instruction, instruction_list, instruction_free)
DEFINE_LIST(VariableList, Variable, variable_list, variable_free)

void program_init (Program *);
void program_destroy (Program *);

Block *program_add_block (Program *program);
Variable *program_add_variable (Program *program);
void program_add_edge (Block *, Block *);
size_t block_count (Program *);

InstructionListPosition *block_add_instruction (Block *, Instruction *);
InstructionListPosition *block_add_instruction_first (Block *, Instruction *);
InstructionListPosition
*block_add_instruction_before (Block *, InstructionListPosition*,
			       Instruction *);
Instruction *block_add_phi (Block *);

bool block_root (Block *);

Instruction *instruction_mov ();
Instruction *instruction_phi ();
Instruction *instruction_switch ();
Instruction *instruction_frob ();
Variable *instruction_dest (Instruction *);
void instruction_set_dest (Instruction *, Variable *);
VariableListPosition *instruction_add_dest (Instruction *, Variable *);
VariableListPosition *instruction_add_source (Instruction *, Variable *);
void instruction_replace_source (Instruction *, VariableListPosition *,
				 Variable *);

void program_init_dfs_tree (Program *);
bool program_back_edge (Block *, Block *);

/* Initializes the dominance tree.  Relies on an initialized dfs
   tree. */
void program_init_dominance_tree (Program *);
bool block_dominates (Block *, Block *);
void program_init_def_use_chains (Program *);

void program_init_liveness (Program *);
bool block_live_in (Program *, Block *, Variable *);
bool block_live_out (Program *, Block *, Variable *);
bool variable_live_at (Program *, Block *, size_t, Variable *);

void program_convert_out_of_ssa (Program *);

#define block_foreach(block, program)					\
  list_foreach (block, BlockList, Block, block_list, (program)->blocks)

#define variable_foreach(var, program)					\
  list_foreach (var, VariableList, Variable, variable_list, (program)->vars)

#define successor_foreach(succ, block)					\
  list_foreach (succ, BlockList, Block, block_list, (block)->successors)

#define predecessor_foreach(pred, block)				\
  list_foreach (pred, BlockList, Block, block_list, (block)->predecessors)

#define dom_child_foreach(block, var)					\
  list_foreach (block, BlockList, Block, block_list, (var)->dom_children)

#define phi_foreach(phi, block)						\
  list_foreach (phi, InstructionList, Instruction, instruction_list,	\
		(block)->phis)

#define instruction_foreach(ins, block)					\
  list_foreach (ins, InstructionList, Instruction, instruction_list,	\
		(block)->instructions)

#define dest_foreach(var, ins)					\
  list_foreach (var, VariableList, Variable, variable_list, (ins)->dests)

#define source_foreach(var, ins)					\
  list_foreach (var, VariableList, Variable, variable_list, (ins)->sources)

#define preorder_foreach(block, program)				\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (Block *(block); b##__LINE__; b##__LINE__ = false)		\
      for (size_t i##__LINE__ = 0, blocks##__LINE__ = block_count ((program)); \
	   i##__LINE__ < blocks##__LINE__				\
	     && ((block) = (program)->preorder[i##__LINE__])		\
	     && true;							\
	   i##__LINE__++)

#define dom_order_foreach(block, program)				\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (Block *(block); b##__LINE__; b##__LINE__ = false)		\
      for (size_t i##__LINE__ = 0, blocks##__LINE__ = block_count ((program)); \
	   i##__LINE__ < blocks##__LINE__				\
	     && ((block) = (program)->dom_order[i##__LINE__])		\
	     && true;							\
	   i##__LINE__++)

#define postorder_foreach(block, program)			\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (Block *(block); b##__LINE__; b##__LINE__ = false)		\
      for (size_t i##__LINE__ = block_count ((program));	\
	   i##__LINE__ > 0						\
	     && ((block) = (program)->reverse_postorder[--i##__LINE__])	\
	     && true; )

#define reverse_postorder_foreach(block, program)			\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (Block *(block); b##__LINE__; b##__LINE__ = false)		\
      for (size_t i##__LINE__ = 0, blocks##__LINE__ = block_count ((program)); \
	   i##__LINE__ < blocks##__LINE__				\
	     && ((block) = (program)->reverse_postorder[i##__LINE__])	\
	     && true;							\
	   i##__LINE__++)

#define use_foreach(use, var)						\
  vector_foreach (use, UseChain, Use, use_chain, &(var)->use_chain)

typedef struct edge Edge;
struct edge
{
  Block *source;
  Block *target;
};
DEFINE_VECTOR (EdgeVector, Edge, edge_vector)

struct program
{
  BlockList blocks;
  VariableList vars;
  EdgeVector back_edges;
  Block **preorder;
  Block **reverse_postorder;
  Block **dom_order;
};

struct block
{
  BlockList successors;
  BlockList predecessors;
  BlockList dom_children;
  ptrdiff_t preorder_number;
  ptrdiff_t reverse_postorder_number;
  ptrdiff_t dom_order_number;
  Block *idom;
  size_t max_dom_number;
  InstructionList phis;
  InstructionList instructions;
  InstructionListPosition *last_instruction;
  Instruction *move_in;
  Instruction *move_out;
  Bitset* liveness_r;
  Bitset* liveness_t;
  bool back_edge_target : 1;
};

struct instruction
{
  Block *block;
  VariableList dests;
  VariableList sources;
  // XXX: Better use enum
  bool phi : 1;
  bool move : 1;
  bool frob : 1; // XXX
};

typedef struct use Use;
struct use {
  Block *block;
  size_t last_use;
};

DEFINE_VECTOR(VariableVector, Variable *, variable_vector)
DEFINE_VECTOR(UseChain, Use, use_chain)

struct variable
{
  Instruction *def;
  VariableListPosition *operand;
  Block *def_block;
  UseChain use_chain;
  size_t dom_number;
  /* TODO(XXX): Rename NUMBER to TIME.  */
  size_t def_number;
  Variable *congruence;
  VariableVector congruence_class;
};

#endif /* COMPILER_COMMON_H_INCLUDED */
