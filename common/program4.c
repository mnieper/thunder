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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "bitset.h"
#include "error.h"
#include "hash.h"
#include "program.h"
#include "set.h"
#include "vector2.h"
#include "xalloc.h"

struct block
{
  EdgeList outgoing_edges;
  EdgeList incoming_edges;
  LabelList phis;
  LabelList labels;
  VirtualRegSet defs;
  BlockListPosition *pos;
  LabelListPosition *last_label;
  Label *move_in;
  Label *move_out;
  bool visited : 1;
  size_t preorder_number;
  size_t reverse_postorder_number;
  Bitset *liveness_r;
  Bitset *liveness_t;
#ifndef NDEBUG
  ptrdiff_t number;
#endif
};

struct edge
{
  Block *source;
  Block *target;
  EdgeListPosition *pos;
  EdgeListPosition *source_pos;
  EdgeListPosition *target_pos;
  VirtualRegSet live_regs;
};

struct virtual_reg
{
  Label *def;
  // We need the blocks that are used.
  VirtualRegListPosition *pos;
  double liveness_duration;
#ifndef NDEBUG
  ptrdiff_t number;
#endif
};

struct label
{
  Block *block;
  VirtualRegList defs;
  VirtualRegList uses;
  Instruction *instruction;
};

struct instruction
{
  const char *opcode;
};

static void
virtual_reg_out_str (FILE *out, VirtualReg *reg)
{
#ifndef NDEBUG
  if (reg->number < 0)
#endif
    fprintf (out, "%p", reg);
#ifndef NDEBUG
  else
    fprintf (out, "%td", reg->number);
#endif
}

static void
label_out_str (FILE *out, Label *label)
{
  fprintf (out, "<TD>");
  {
    VirtualRegListIterator i = virtual_reg_list_iterator (label->defs);
    VirtualReg *reg;
    while (virtual_reg_list_iterator_next (&i, &reg, NULL))
      {
	virtual_reg_out_str (out, reg);
	fprintf (out, " ");
      }
    virtual_reg_list_iterator_free (&i);
  }
  if (virtual_reg_list_size (label->defs) > 0)
    fprintf (out, "&lt;- ");
  fprintf (out, "%s",
	   label->instruction == NULL ? "phi" : label->instruction->opcode);
  {
    VirtualRegListIterator i = virtual_reg_list_iterator (label->uses);
    VirtualReg *reg;
    while (virtual_reg_list_iterator_next (&i, &reg, NULL))
      {
	fprintf (out, " ");
	virtual_reg_out_str (out, reg);
      }
    virtual_reg_list_iterator_free (&i);
  }
  fprintf (out, "</TD>");
}

Block *
block_create ()
{
  Block *block = XMALLOC (Block);
  block->phis = label_list_create (true);
  block->labels = label_list_create (true);
  block->outgoing_edges = edge_list_create (false);
  block->incoming_edges = edge_list_create (false);
  block->defs = virtual_reg_set_create ();
  block->liveness_r = NULL;
  block->liveness_t = NULL;
#ifndef NDEBUG
  block->number = -1;
#endif
  return block;
};

void
block_free (Block *block)
{
  label_list_free (block->phis);
  label_list_free (block->labels);
  edge_list_free (block->outgoing_edges);
  edge_list_free (block->incoming_edges);
  bitset_free (block->liveness_r);
  bitset_free (block->liveness_t);
  virtual_reg_set_free (block->defs);
  free (block);
};

Label *
block_add_phi_func (Block *block)
{
  Label *phi = label_create (block, NULL);
  label_list_add (block->phis, phi);
  return phi;
}

Label *
block_add_label (Block *block, Instruction *ins)
{
  Label *label = label_create (block, ins);
  block->last_label = label_list_add (block->labels, label);
  return label;
}

Label *
block_add_label_first (Block *block, Instruction *ins)
{
  Label *label = label_create (block, ins);
  label_list_add_first (block->labels, label);
  return label;
}

Label *
block_add_label_before (Block *block, LabelListPosition *pos, Instruction *ins)
{
  Label *label = label_create (block, ins);
  label_list_add_before (block->labels, pos, label);
  return label;
}

void
block_add_outgoing_edge (Block *block, Edge *edge)
{
  edge->source = block;
  edge->source_pos = edge_list_add (block->outgoing_edges, edge);
}

void
block_add_incoming_edge (Block *block, Edge *edge)
{
  edge->target = block;
  edge->target_pos = edge_list_add (block->incoming_edges, edge);
}

void
block_replace_outgoing_edge (Block *block, EdgeListPosition *pos, Edge *edge)
{
  edge->source = block;
  edge_list_replace (block->outgoing_edges, pos, edge);
}

void
block_replace_incoming_edge (Block *block, EdgeListPosition *pos, Edge *edge)
{
  edge->target = block;
  edge_list_replace (block->incoming_edges, pos, edge);
}

bool
block_root (Block *block)
{
  return edge_list_size (block->incoming_edges) == 0;
}

bool
block_defines (Block *block, VirtualReg *reg)
{
  return virtual_reg_set_contains (block->defs, reg);
}

static bool
live_reg_out_str (VirtualReg *reg, void *data)
{
  struct { FILE *out; bool *first; } *context = data;
  if (*context->first)
    *context->first = false;
  else
    fprintf (context->out, " ");
  virtual_reg_out_str (context->out, reg);
  return true;
}

static bool back_edge (Edge *);

static void
edge_out_str (FILE *out, Edge *edge)
{
  bool first = true;
  if (back_edge (edge))
    fprintf (out, "[B] ");

  virtual_reg_set_foreach (edge->live_regs, live_reg_out_str,
			   &(struct { FILE *out; bool *first; })
			   { out, &first });
}

static void
block_out_str (FILE *out, Block *block)
{
  fprintf (out, "<TABLE BORDER=\"0\" CELLBORDER=\"1\" CELLSPACING=\"0\" CELLPADDING=\"2\">\n");
  {
    fprintf (out, "  <TR><TD>Preorder %zd | RevPost %zd</TD></TR>",
	     block->preorder_number,
	     block->reverse_postorder_number);

    /* Prints the T sets. */
    {
      fprintf (out, "  <TR><TD>T ");
      BitsetSize i = bitset_next (block->liveness_t, 0, true);
      while (i < bitset_size (block->liveness_t))
	{
	  fprintf (out, "%llu ", i);
	  i = bitset_next (block->liveness_t, i + 1, true);
	}
      fprintf (out, "  </TD></TR>");
    }

    LabelListIterator i = label_list_iterator (block->phis);
    Label *label;
    while (label_list_iterator_next (&i, &label, NULL))
      {
	fprintf (out, "  <TR>");
	label_out_str (out, label);
	fprintf (out, "</TR>\n");
      }
    label_list_iterator_free (&i);
  }
  {
    LabelListIterator i = label_list_iterator (block->labels);
    Label *label;
    while (label_list_iterator_next (&i, &label, NULL))
      {
	fprintf (out, "  <TR>");
	label_out_str (out, label);
	fprintf (out, "</TR>\n");
      }
    label_list_iterator_free (&i);
  }
  fprintf (out, "</TABLE>\n");
}

Edge *
edge_create (void)
{
  Edge *edge = XMALLOC (Edge);
  edge->live_regs = virtual_reg_set_create ();
  return edge;
}

void
edge_free (Edge *edge)
{
  virtual_reg_set_free (edge->live_regs);
  free (edge);
}

VirtualReg *
virtual_reg_create (void)
{
  VirtualReg *reg = XMALLOC (VirtualReg);
  reg->liveness_duration = 0.0;
#ifndef NDEBUG
  reg->number = -1;
#endif
}

void
virtual_reg_free (VirtualReg *reg)
{
  free (reg);
}

bool
virtual_reg_live (Edge *edge, VirtualReg *reg)
{
  return virtual_reg_set_contains (edge->live_regs, reg);
}

bool
virtual_reg_live_out (Block *block, VirtualReg *reg)
{
  bool res = false;
  EdgeListIterator i = edge_list_iterator (block->outgoing_edges);
  Edge *edge;
  while (edge_list_iterator_next (&i, &edge, NULL))
    if (virtual_reg_live (edge, reg))
      {
	res = true;
	break;
      }
  edge_list_iterator_free (&i);
  return res;
}

bool
virtual_reg_set_live (Edge *edge, VirtualReg *reg)
{
  if (virtual_reg_set_adjoin (edge->live_regs, reg))
    {
      reg->liveness_duration += 1.0;
      return true;
    }
  return false;
}

Label *
label_create (Block *block, Instruction *ins)
{
  Label *label = XMALLOC (Label);
  label->block = block;
  label->defs = virtual_reg_list_create (true);
  label->uses = virtual_reg_list_create (false);
  label->instruction = ins;
  return label;
}

void
label_free (Label *label)
{
  virtual_reg_list_free (label->defs);
  virtual_reg_list_free (label->uses);
  instruction_free (label->instruction);
  free (label);
}

bool
label_defines (Label *label, VirtualReg *reg)
{
  return virtual_reg_list_search (label->defs, reg) != NULL;
}

bool
label_uses (Label *label, VirtualReg *reg)
{
  return virtual_reg_list_search (label->uses, reg) != NULL;
}

VirtualReg *
label_def (Label *label)
{
  return virtual_reg_list_first (label->defs);
}

VirtualReg *
label_add_def (Program *program, Label *label)
{
  VirtualReg *virtual_reg = virtual_reg_create ();
  virtual_reg->def = label;
  virtual_reg->pos = virtual_reg_list_add (label->defs, virtual_reg);
  bool res = virtual_reg_set_adjoin (label->block->defs, virtual_reg);
  assert (res);
  res = virtual_reg_set_adjoin (program->virtual_regs, virtual_reg);
  assert (res);
  return virtual_reg;
}

void
label_remove_def (Program *program, Label *label, VirtualReg *reg)
{
  assert (label == reg->def);
  virtual_reg_list_remove (label->defs, reg->pos);
  bool res = virtual_reg_set_delete (label->block->defs, reg);
  assert (res);
  res = virtual_reg_set_delete (program->virtual_regs, reg);
  assert (res);
}

void
label_add_use (Label *label, VirtualReg *reg)
{
  virtual_reg_list_add (label->uses, reg);
}

void
label_replace_use (Label *label, VirtualRegListPosition *pos, VirtualReg *reg)
{
  VirtualReg *old_reg = virtual_reg_list_at (label->uses, pos);
  virtual_reg_list_replace (label->uses, pos, reg);
}

/* Swaps the virtual registers REG1 and REG2 at their definition sites.
   Both definitions have to reside in the same block.  */
static void
label_swap_defs (VirtualReg* reg1, VirtualReg* reg2)
{
  virtual_reg_list_replace (reg1->def->defs, reg1->pos, reg2);
  virtual_reg_list_replace (reg2->def->defs, reg2->pos, reg1);
  Label *def = reg1->def;
  reg1->def = reg2->def;
  reg2->def = def;
  VirtualRegListPosition *pos = reg1->pos;
  reg1->pos = reg2->pos;
  reg2->pos = pos;
}

/* Instructions */
Instruction *
instruction_jump ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "jmp";
  return ins;
}

Instruction *
instruction_frob ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "frob";
  return ins;
}

Instruction *
instruction_ret ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "ret";
  return ins;
}

Instruction *
instruction_br ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "br";
  return ins;
}

Instruction *
instruction_perform ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "perform";
  return ins;
}

Instruction *
instruction_movr ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "movr";
  return ins;
}

Instruction *
instruction_switch ()
{
  Instruction *ins = XMALLOC (Instruction);
  ins->opcode = "switch";
  return ins;
}

void
instruction_free (Instruction *ins)
{
  free (ins);
}

/* Programs */
void
program_init (Program *program)
{
  program->blocks = block_list_create (true);
  program->edges = edge_list_create (true);
  program->virtual_regs = virtual_reg_set_create ();
  program->reverse_postorder = NULL;
  program->preorder = NULL;
  edge_vector_init (&program->back_edges);
  postorder_init (&program->postorder);
}

void
program_destroy (Program *program)
{
  block_list_free (program->blocks);
  edge_list_free (program->edges);
  virtual_reg_set_free (program->virtual_regs);
  free (program->reverse_postorder);
  free (program->preorder);
  edge_vector_destroy (&program->back_edges);
  postorder_destroy (&program->postorder);
}

Block *
program_add_block (Program *program)
{
  Block *b = block_create ();
  b->pos = block_list_add (program->blocks, b);
  return b;
}

void
program_remove_block (Program *program, Block *block)
{
  block_list_remove (program->blocks, block->pos);
}

Edge *
program_add_edge (Program *program)
{
  Edge *edge = edge_create ();
  EdgeListPosition *pos = edge_list_add (program->edges, edge);
  edge->pos = pos;
  return edge;
}

void
program_remove_edge (Program *program, Edge *edge)
{
  edge_list_remove (program->edges, edge->pos);
}

void
program_dump (Program *program, const char *path)
{
  FILE *out = fopen (path, "w");
  if (out == NULL)
    error (EXIT_FAILURE, errno, "%s", path);
  fprintf (out, "digraph G {\n");
  {
    BlockListIterator i = block_list_iterator (program->blocks);
    Block *block;
    while (block_list_iterator_next (&i, &block, NULL))
      {
	fprintf (out, "  block%p [shape=none, margin=0, label=<\n", block);
	block_out_str (out, block);
	fprintf (out, ">];\n");
	{
	  EdgeListIterator j = edge_list_iterator (block->outgoing_edges);
	  Edge *edge;
	  while (edge_list_iterator_next (&j, &edge, NULL))
	    {
	      fprintf (out, "  block%p -> block%p [label=<",
		       (void *) edge->source, (void *) edge->target);
	      edge_out_str (out, edge);
	      fprintf (out, ">];\n");
	    }
	  edge_list_iterator_free (&j);
	}
      }
    block_list_iterator_free (&i);
  }
  fprintf (out, "}\n");

  fclose (out);
}

static void
split_critical_edge (Program *program, Edge *edge)
{
  Block *block = program_add_block (program);
  Edge *edge1 = program_add_edge (program);
  Edge *edge2 = program_add_edge (program);
  Block *source = edge->source;
  Block *target = edge->target;
  block_replace_outgoing_edge (source, edge->source_pos, edge1);
  block_replace_incoming_edge (target, edge->target_pos, edge2);
  block_add_incoming_edge (block, edge1);
  block_add_outgoing_edge (block, edge2);
  block_add_label (block, instruction_jump ());
  program_remove_edge (program, edge);
}

void
split_critical_edges (Program *program)
{
  EdgeList worklist = edge_list_create (false);
  {
    EdgeListIterator i = edge_list_iterator (program->edges);
    Edge *edge;
    while (edge_list_iterator_next (&i, &edge, NULL))
      {
	if (edge_list_size (edge->source->outgoing_edges) > 1
	    && edge_list_size (edge->target->incoming_edges) > 1)
	  edge_list_add (worklist, edge);
      }
    edge_list_iterator_free (&i);
  }
  {
    EdgeListIterator i = edge_list_iterator (worklist);
    Edge *edge;
    {
      while (edge_list_iterator_next (&i, &edge, NULL))
	split_critical_edge (program, edge);
    }
    edge_list_iterator_free (&i);
  }
  edge_list_free (worklist);
}

static void
clear_visited_blocks (Program *program)
{
  BlockListIterator i = block_list_iterator (program->blocks);
  Block *block;
  while (block_list_iterator_next (&i, &block, NULL))
    block->visited = false;
  block_list_iterator_free (&i);
}

DEFINE_VECTOR(PostorderWorklist, Block *, postorder_worklist)

void init_postorder (Program *program)
{
  clear_visited_blocks (program);

  PostorderWorklist worklist;
  postorder_worklist_init (&worklist);
  {
    BlockListIterator i = block_list_iterator (program->blocks);
    Block *block;
    while (block_list_iterator_next (&i, &block, NULL))
      if (block_root (block))
	postorder_worklist_push (&worklist, &block);
    block_list_iterator_free (&i);
  }

  while (!postorder_worklist_empty (&worklist))
    {
      Block *block = *postorder_worklist_top (&worklist);
      if (block->visited)
	{
	  postorder_push (&program->postorder, &block);
	  postorder_worklist_pop (&worklist);
	}
      else
	{
	  block->visited = true;
	  EdgeListIterator i = edge_list_iterator (block->outgoing_edges);
	  Edge *edge;
	  while (edge_list_iterator_next (&i, &edge, NULL))
	    if (!edge->target->visited)
	      postorder_worklist_push (&worklist, &edge->target);
	  edge_list_iterator_free (&i);
	}
    }
  postorder_worklist_destroy (&worklist);
}

void clear_postorder (Program *program)
{
  postorder_clear (&program->postorder);
}

typedef struct liveness_register_use LivenessRegisterUse;
struct liveness_register_use
{
  VirtualReg *reg;
  Block *block;
};

DEFINE_VECTOR(LivenessWorklist, LivenessRegisterUse, liveness_worklist)

static void
liveness_update_edge (LivenessWorklist *worklist, Edge *edge, VirtualReg *reg)
{
  if (virtual_reg_set_live (edge, reg) && !block_defines (edge->source, reg))
    liveness_worklist_push (worklist,
			    &(LivenessRegisterUse) { reg, edge->source });
}

void
init_liveness (Program *program)
{
  LivenessWorklist worklist;
  liveness_worklist_init (&worklist);

  for (Block **p = postorder_begin (&program->postorder);
       p != postorder_end (&program->postorder);
       ++p)
    {
      Block *block = *p;
      VirtualRegSet defs = virtual_reg_set_create ();

      LabelListIterator j = label_list_iterator (block->phis);
      Label *label;
      while (label_list_iterator_next (&j, &label, NULL))
	{
	  VirtualRegListIterator k = virtual_reg_list_iterator (label->uses);
	  VirtualReg *reg;
	  EdgeListIterator l = edge_list_iterator (block->incoming_edges);
	  while (virtual_reg_list_iterator_next (&k, &reg, NULL))
	    {
	      Edge *edge;
	      bool res = edge_list_iterator_next (&l, &edge, NULL);
	      assert (res);
	      liveness_update_edge (&worklist, edge, reg);
	    }
	  virtual_reg_list_iterator_free (&k);
	  edge_list_iterator_free (&l);

	  bool res = virtual_reg_set_adjoin (defs, label_def (label));
	  assert (res);
	}
      label_list_iterator_free (&j);

      j = label_list_iterator (block->labels);
      while (label_list_iterator_next (&j, &label, NULL))
	{
	  VirtualRegListIterator k = virtual_reg_list_iterator (label->uses);
	  VirtualReg *reg;
	  while (virtual_reg_list_iterator_next (&k, &reg, NULL))
	    if (!virtual_reg_set_contains (defs, reg))
	      liveness_worklist_push (&worklist,
				      &(LivenessRegisterUse) { reg, block });
	  virtual_reg_list_iterator_free (&k);

	  k = virtual_reg_list_iterator (label->defs);
	  while (virtual_reg_list_iterator_next (&k, &reg, NULL))
	    {
	      bool res = virtual_reg_set_adjoin (defs, reg);
	      assert (res);
	    }
	  virtual_reg_list_iterator_free (&k);
	}
      label_list_iterator_free (&j);
      virtual_reg_set_free (defs);
    }

  {
    LivenessRegisterUse *use;
    while ((use = liveness_worklist_pop (&worklist)) != NULL)
      {
	EdgeListIterator i = edge_list_iterator (use->block->incoming_edges);
	Edge *edge;
	while (edge_list_iterator_next (&i, &edge, NULL))
	  liveness_update_edge (&worklist, edge, use->reg);
	edge_list_iterator_free (&i);
      }
  }
  liveness_worklist_destroy (&worklist);
}

static bool
clear_duration (VirtualReg *reg, void *data)
{
  reg->liveness_duration = 0.0;
  return true;
}

void
clear_liveness (Program *program)
{
  {
    EdgeListIterator i = edge_list_iterator (program->edges);
    Edge *edge;
    while (edge_list_iterator_next (&i, &edge, NULL))
      virtual_reg_set_clear (edge->live_regs);
    edge_list_iterator_free (&i);
  }
  virtual_reg_set_foreach (program->virtual_regs, clear_duration, NULL);
}

static bool
live_range_overlap (Block *block, VirtualReg *live_in_reg,
		    VirtualReg *other_reg)
{
  LabelListPosition *pos = block->last_label;
  if (!virtual_reg_live_out (block, live_in_reg))
    {
      while (!(label_uses (label_list_at (block->labels, pos), live_in_reg)))
	{
	  pos = label_list_previous (block->labels, pos);
	  if (pos == NULL)
	    return false;
	  assert (pos != NULL);
	}
    }

  do
    {
      Label *label = label_list_at (block->labels, pos);
      if (label_defines (label, live_in_reg))
	return false;
      if (label_uses (label, live_in_reg))
	return true;
    }
  while ((pos = label_list_previous (block->labels, pos)) != NULL);

  return false;
}

static void
lift_phi_func (Program *program, Block *block, Label *phi)
{
  VirtualReg *def = label_def (phi);
  bool live_out = virtual_reg_live_out (block, def);
  VirtualRegListIterator i = virtual_reg_list_iterator (phi->uses);
  VirtualReg *use;
  VirtualRegListPosition *slot;
  EdgeListIterator j = edge_list_iterator (block->incoming_edges);
  Edge *edge;
  while (virtual_reg_list_iterator_next (&i, &use, &slot))
    {
      bool res = edge_list_iterator_next (&j, &edge, NULL);
      assert (res);
      if (live_out && virtual_reg_live_out (block, use)
	  || live_range_overlap (block, def, use))
	{
	  Label *move
	    = block_add_label_before (edge->source, edge->source->last_label,
				      instruction_movr ());
	  VirtualReg *new = label_add_def (program, move);
	  label_add_use (move, use);
	  label_replace_use (phi, slot, new);
	  virtual_reg_set_live (edge, new);
	}
    }
  virtual_reg_list_iterator_free (&i);
  edge_list_iterator_free (&j);
}

void
phi_lift (Program *program)
{
  init_liveness (program);

  for (Block **p = postorder_begin (&program->postorder);
       p != postorder_end (&program->postorder);
       ++p)
    {
      Block *block = *p;
      LabelListIterator i = label_list_iterator (block->phis);
      Label *label;
      while (label_list_iterator_next (&i, &label, NULL))
	lift_phi_func (program, block, label);
      label_list_iterator_free (&i);
    }

  clear_liveness (program);
}

void
translate_into_cssa (Program *program)
{
  BlockListIterator i = block_list_iterator (program->blocks);
  Block *block;
  while (block_list_iterator_next (&i, &block, NULL))
    {
      block->move_in = block_add_label_first (block,
					      instruction_movr ());
      block->move_out = block_add_label_before (block, block->last_label,
						instruction_movr());
    }
  block_list_iterator_free (&i);

  i = block_list_iterator (program->blocks);
  while (block_list_iterator_next (&i, &block, NULL))
    {
      LabelListIterator j = label_list_iterator (block->phis);
      Label *phi;
      while (label_list_iterator_next (&j, &phi, NULL))
	{
	  VirtualReg *new_reg = label_add_def (program, block->move_in);
	  label_add_use (block->move_in, new_reg);
	  label_swap_defs (label_def (phi), new_reg);

	  VirtualRegListIterator k = virtual_reg_list_iterator (phi->uses);
	  VirtualReg *reg;
	  VirtualRegListPosition *pos;
	  EdgeListIterator l = edge_list_iterator (block->incoming_edges);
	  while (virtual_reg_list_iterator_next (&k, &reg, &pos))
	    {
	      Edge *edge;
	      bool res = edge_list_iterator_next (&l, &edge, NULL);
	      assert (res);
	      new_reg = label_add_def (program, edge->source->move_out);
	      label_add_use (edge->source->move_out, reg);
	      label_replace_use (phi, pos, new_reg);
	    }
	  virtual_reg_list_iterator_free (&k);
	  edge_list_iterator_free (&l);
	};
      label_list_iterator_free (&j);
    }
  block_list_iterator_free (&i);
}

size_t
block_count (Program *program)
{
  return block_list_size (program->blocks);
}

DEFINE_VECTOR (DfsWorklist, Block *, dfs_worklist)

/* Return true if EDGE is a back edge, that is if its target comes
   before its source in reverse postorder.  */
static bool
back_edge (Edge *edge)
{
  return edge->target->reverse_postorder_number
    < edge->source->reverse_postorder_number;
}

void
init_dfs (Program *program)
{
  size_t blocks = block_count (program);
  clear_visited_blocks (program);
  program->preorder = XNMALLOC (blocks, Block *);
  program->reverse_postorder = XNMALLOC (blocks, Block *);

  DfsWorklist worklist;
  dfs_worklist_init (&worklist);
  {
    BlockListIterator i = block_list_iterator (program->blocks);
    Block *block;
    while (block_list_iterator_next (&i, &block, NULL))
      if (block_root (block))
	dfs_worklist_push (&worklist, &block);
    block_list_iterator_free (&i);
  }

  size_t preorder_number = 0;
  size_t reverse_postorder_number = blocks;
  while (!dfs_worklist_empty (&worklist))
    {
      Block *block = *dfs_worklist_top (&worklist);
      if (block->visited)
	{
	  program->reverse_postorder[--reverse_postorder_number]
	    = block;
	  block->reverse_postorder_number = reverse_postorder_number;
	  dfs_worklist_pop (&worklist);
	}
      else
	{
	  block->visited = true;
	  program->preorder[preorder_number] = block;
	  block->preorder_number = preorder_number++;

	  EdgeListIterator i = edge_list_iterator (block->outgoing_edges);
	  Edge *edge;
	  while (edge_list_iterator_next (&i, &edge, NULL))
	    if (!edge->target->visited)
	      dfs_worklist_push (&worklist, &edge->target);
	  edge_list_iterator_free (&i);
	}
    }
  assert (preorder_number == blocks);
  assert (reverse_postorder_number == 0);

  for (size_t i = 0; i < blocks; i++)
    {
      Block *block = program->preorder[i];
      EdgeListIterator j = edge_list_iterator (block->incoming_edges);
      Edge *edge;
      while (edge_list_iterator_next (&j, &edge, NULL))
	if (back_edge (edge))
	  edge_vector_push (&program->back_edges, &edge);
      edge_list_iterator_free (&j);
    }

  dfs_worklist_destroy (&worklist);
}

void
clear_dfs (Program *program)
{
  free (program->preorder);
  free (program->reverse_postorder);
  program->preorder = NULL;
  program->reverse_postorder = NULL;
  edge_vector_clear (&program->back_edges);
}

void
init_liveness2 (Program *program)
{
  size_t blocks = block_count (program);

  BlockListIterator i = block_list_iterator (program->blocks);
  Block *block;
  while (block_list_iterator_next (&i, &block, NULL))
    {
      block->liveness_r = bitset_create (blocks);
      block->liveness_t = bitset_create (blocks);
    }
  block_list_iterator_free (&i);

  for (ptrdiff_t j = blocks - 1; j >= 0; j--)
    {
      block = program->reverse_postorder[j];
      bitset_set_at (block->liveness_r, block->preorder_number, true);
      EdgeListIterator k = edge_list_iterator (block->outgoing_edges);
      Edge *edge;
      while (edge_list_iterator_next (&k, &edge, NULL))
	{
	  if (block != edge->target && !back_edge (edge))
	    bitset_union (block->liveness_r, edge->target->liveness_r);
	}
      edge_list_iterator_free (&k);
    }

  for (Edge **edge = edge_vector_begin (&program->back_edges);
       edge != edge_vector_end (&program->back_edges);
       edge++)
    {
      Block *block = (*edge)->target;
      bitset_set_at (block->liveness_t, block->preorder_number, true);
      for (Edge **e = edge_vector_begin (&program->back_edges);
	   e != edge;
	   e++)
	{
	  if (bitset_at (block->liveness_r, (*e)->source->preorder_number)
	      && !bitset_at (block->liveness_r, (*e)->target->preorder_number))
	    bitset_union (block->liveness_t, (*e)->target->liveness_t);
	}
    }

  for (Edge **edge = edge_vector_begin (&program->back_edges);
       edge != edge_vector_end (&program->back_edges);
       edge++)
    {
      Block *block = (*edge)->source;
      bitset_union (block->liveness_t, (*edge)->target->liveness_t);
    }

  for (ptrdiff_t i = blocks - 1; i >= 0; i--)
    {
      Block *block = program->reverse_postorder [i];
      EdgeListIterator j = edge_list_iterator (block->outgoing_edges);
      Edge *edge;
      while (edge_list_iterator_next (&j, &edge, NULL))
	if (!back_edge (edge))
	  bitset_union (block->liveness_t, edge->target->liveness_t);
      edge_list_iterator_free (&j);
      bitset_diff (block->liveness_t, block->liveness_r);
    }

  for (size_t i = 0; i < blocks; i++)
    bitset_set_at (program->preorder[i]->liveness_t, i, true);
}

void
clear_liveness2 (Program *program)
{
  BlockListIterator i = block_list_iterator (program->blocks);
  Block *block;
  while (block_list_iterator_next (&i, &block, NULL))
    {
      bitset_free (block->liveness_r);
      bitset_free (block->liveness_t);
      block->liveness_r = NULL;
      block->liveness_t = NULL;
    }
  block_list_iterator_free (&i);
}

#ifndef NDEBUG
void block_set_number (Block *block, ptrdiff_t number)
{
  block->number = number;
}
void virtual_reg_set_number (VirtualReg *reg, ptrdiff_t number)
{
  reg->number = number;
}
#endif
