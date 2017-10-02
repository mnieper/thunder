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
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifndef PROGRAM_H_INCLUDED
#define PROGRAM_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "list.h"
#include "set.h"
#include "vector2.h"
#include "vmcommon.h"

typedef struct program Program;
typedef struct edge Edge;
typedef struct block Block;
typedef struct virtual_reg VirtualReg;
typedef struct label Label;
typedef struct phi_func PhiFunc;
typedef struct instruction Instruction;

void edge_free (Edge *);
void block_free (Block *);
void virtual_reg_free (VirtualReg *);
void phi_func_free (PhiFunc *);
void label_free (Label *);

DEFINE_LIST(BlockList, Block, block_list, block_free)
DEFINE_LIST(EdgeList, Edge, edge_list, edge_free)
DEFINE_LIST(LabelList, Label, label_list, label_free)
DEFINE_LIST(VirtualRegList, VirtualReg, virtual_reg_list,
	    virtual_reg_free)

DEFINE_VECTOR(Postorder, Block *, postorder)

void program_init (Program *);
void program_destroy (Program *);
void program_dump (Program *, const char *);

Block *program_add_block (Program *);
void program_remove_block (Program *, Block *);
Edge *program_add_edge (Program *);
void program_remove_edge (Program *, Edge *);

Block *block_create (void);
void block_free (Block *);

Label *block_add_label (Block *, Instruction *);
Label *block_add_phi_func (Block *);

void block_add_outgoing_edge (Block *, Edge *);
void block_add_incoming_edge (Block *, Edge *);
void block_replace_outgoing_edge (Block *, EdgeListPosition *, Edge *);
void block_replace_incoming_edge (Block *, EdgeListPosition *, Edge *);

bool block_root (Block *);
bool block_defines (Block *, VirtualReg *);

Edge *edge_create (void);
void edge_free (Edge *);

Label *label_create (Block *, Instruction *);
void label_free (Label *);

VirtualReg *label_def (Label *);
VirtualReg *label_add_def (Program *, Label *);
void label_remove_def (Program* , Label *, VirtualReg *);
void label_add_use (Label *, VirtualReg *);
void label_replace_use (Label *, VirtualRegListPosition *, VirtualReg *);
bool label_defines (Label *, VirtualReg *);
bool label_uses (Label *, VirtualReg *);

VirtualReg *virtual_reg_create (void);
void virtual_reg_free (VirtualReg *);
bool virtual_reg_live (Edge *, VirtualReg *);
bool virtual_reg_live_out (Block *, VirtualReg *);

void instruction_free (Instruction *ins);
Instruction *instruction_jump (void);
Instruction *instruction_br (void);
Instruction *instruction_ret (void);
Instruction *instruction_frob (void);
Instruction *instruction_perform (void);
Instruction *instruction_movr (void);
Instruction *instruction_switch (void);

void split_critical_edges (Program *);
void init_postorder (Program *);
void clear_postorder (Program *);
void init_liveness (Program *);
void clear_liveness (Program *);
void init_dfs (Program *);
void clear_dfs (Program *);
void init_liveness2 (Program *);
void clear_liveness2 (Program *);
void phi_lift (Program *);
void translate_into_cssa (Program *);

#ifndef NDEBUG
void block_set_number (Block *block, ptrdiff_t number);
void virtual_reg_set_number (VirtualReg *reg, ptrdiff_t number);
#endif

DEFINE_SET(VirtualRegSet, VirtualReg, virtual_reg_set)

DEFINE_VECTOR (EdgeVector, Edge *, edge_vector)
struct program
{
  BlockList blocks;
  EdgeList edges;
  VirtualRegSet virtual_regs;
  Postorder postorder;

  Block **preorder;
  Block **reverse_postorder;
  EdgeVector back_edges;
};

#endif /* PROGRAM_H_INCLUDED */
