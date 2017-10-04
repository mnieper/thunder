/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * Thunder is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as
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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdbool.h>

#include "common.h"

static void add_move_instructions (Program *);
static void block_add_move_instructions (Block *);
static void isolate_phis (Program *);
static void isolate_phi (Program *, Block *, Instruction *);
static void coalesce_phis (Program *);
static void coalesce_phi (Program *, Block *, Instruction *);
static void init_congruence_classes (Program *);
static void merge_congruence_classes (Variable *, Variable *);
static void coalesce_aggressively (Program *);
static void block_coalesce_aggressively (Program *, Block *);
static void coalesce_move (Program *, Block *, Instruction *);
static bool intersect (Program *, Variable *, Variable *);
static bool congruence_classes_interfere (Program *, VariableVector *,
					  VariableVector *);
static bool variable_dominates(Variable *, Variable *);

void
program_convert_out_of_ssa (Program *program)
{
 add_move_instructions (program);
 isolate_phis (program);
 program_init_liveness (program);
 init_congruence_classes (program);
 coalesce_phis (program);
 coalesce_aggressively (program);


 // resolve_registers
 // resolve parallel copies
 // later pass: register allocation & spilling (e.g. interval-based
 // or dominator-tree based)
}

static void
add_move_instructions (Program *program)
{
  block_foreach (block, program)
    block_add_move_instructions (block);
}

static void
block_add_move_instructions (Block *block)
{
  block->move_in = instruction_mov ();
  block->move_out = instruction_mov ();
  block_add_instruction_first (block, block->move_in);
  block_add_instruction_before (block, block->last_instruction,
				block->move_out);
}

static void
isolate_phis (Program *program)
{
  block_foreach (block, program)
    {
      phi_foreach (phi, block)
	isolate_phi (program, block, phi);
    }
}

#define phi_source_foreach(var, pred, pos, block, phi)			\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (VariableListIterator i##__LINE__				\
	   = variable_list_iterator ((phi)->sources);			\
	 b##__LINE__; )							\
      for (BlockListIterator j##__LINE__				\
	     = block_list_iterator ((block)->predecessors);		\
	   b##__LINE__;)						\
	for (VariableListPosition *(pos); b##__LINE__;)			\
	  for (Block *(pred); b##__LINE__;				\
	       b##__LINE__ = false,					\
		 variable_list_iterator_free (&i##__LINE__),		\
		 block_list_iterator_free (&j##__LINE__))		\
	    for (Variable *(var);					\
		 variable_list_iterator_next (&i##__LINE__, &var, &pos)	\
		   && (block_list_iterator_next (&j##__LINE__, &pred, NULL) \
		       || true); )

static void
isolate_phi (Program *program, Block *block, Instruction *phi)
{
  instruction_add_dest (block->move_in, instruction_dest (phi));
  Variable *var = program_add_variable (program);
  instruction_add_source (block->move_in, var);
  instruction_set_dest (phi, var);

  phi_source_foreach (input, pred, pos, block, phi)
    {
      Variable *var = program_add_variable (program);
      instruction_add_dest (pred->move_out, var);
      instruction_add_source (pred->move_out, input);
      instruction_replace_source (phi, pos, var);
    }
}

static void
init_congruence_classes (Program *program)
{
  variable_foreach (var, program)
    {
      var->congruence = var;
      variable_vector_push (&var->congruence_class, &var);
    }
}

static void
merge_congruence_classes (Variable *var1, Variable *var2)
{
  var1 = var1->congruence;
  var2 = var2->congruence;

  if (var1 == var2)
    return;

  if (var1->dom_number > var2->dom_number)
    {
      Variable *tmp = var1;
      var1 = var2;
      var2 = tmp;
    }

  VariableVector c = var1->congruence_class;
  variable_vector_init (&var1->congruence_class);

  Variable **v1 = variable_vector_begin (&c);
  Variable **v2 = variable_vector_begin (&var2->congruence_class);

  while (v1 != variable_vector_end (&c) && v2 != variable_vector_end (&c))
    if ((*v1)->dom_number < (*v2)->dom_number)
      variable_vector_push (&var1->congruence_class, v1++);
    else if ((*v1)->dom_number > (*v2)->dom_number)
      {
	(*v2)->congruence = var1;
	variable_vector_push (&var1->congruence_class, v2++);
      }
    else
      {
	(*v2)->congruence = var1;
	variable_vector_push (&var1->congruence_class, (v1++, v2++));
      }
  while (v1 != variable_vector_end (&c))
    variable_vector_push (&var1->congruence_class, v1++);
  while (v2 != variable_vector_end (&var2->congruence_class))
    {
      (*v2)->congruence = var1;
      variable_vector_push (&var1->congruence_class, v2++);
    }

  variable_vector_destroy (&c);
  variable_vector_clear (&var2->congruence_class);
}

static void
coalesce_phis (Program *program)
{
  block_foreach (block, program)
    {
      phi_foreach (phi, block)
	coalesce_phi (program, block, phi);
    }
}

static void
coalesce_phi (Program *program, Block *block, Instruction *phi)
{
  Variable *dest = instruction_dest (phi);
  source_foreach (source, phi)
    merge_congruence_classes (dest, source);
}

static void coalesce_aggressively (Program *program)
{
  block_foreach (block, program)
    block_coalesce_aggressively (program, block);
}

static void
block_coalesce_aggressively (Program *program, Block *block)
{
  coalesce_move (program, block, block->move_in);
  coalesce_move (program, block, block->move_out);
}

#define dest_source_foreach(dest, source, ins)				\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (VariableListIterator i##__LINE__				\
	   = variable_list_iterator ((ins)->dests);			\
	 b##__LINE__; )							\
      for (VariableListIterator j##__LINE__				\
	     = variable_list_iterator ((ins)->sources);			\
	   b##__LINE__;)						\
	for (Variable *(dest); b##__LINE__;				\
	     b##__LINE__ = false,					\
	       variable_list_iterator_free (&i##__LINE__),		\
	       variable_list_iterator_free (&j##__LINE__))		\
	  for (Variable *(source);					\
	       variable_list_iterator_next (&i##__LINE__, &dest, NULL)	\
		 && (variable_list_iterator_next (&j##__LINE__, &source, NULL) \
		     || true); )

static void
coalesce_move (Program *program, Block *block, Instruction *move)
{
  dest_source_foreach (dest, source, move)
    {
      if (dest->congruence == source->congruence)
	continue;
      if (!congruence_classes_interfere (program,
					 &dest->congruence_class,
					 &source->congruence_class))
	merge_congruence_classes (dest, source);
    }
}

DEFINE_VECTOR(Worklist, Variable *, worklist)

static bool
congruence_classes_interfere (Program *program,
			      VariableVector *c1, VariableVector *c2)
{
  Worklist worklist;
  worklist_init (&worklist);

  Variable **i1 = variable_vector_begin (c1);
  Variable **i2 = variable_vector_begin (c2);
  while (i1 != variable_vector_end (c1) || i2 != variable_vector_end (c2))
    {
      Variable **other = worklist_top (&worklist);
      Variable *current;
      if (i1 == variable_vector_end (c1)
	  || (i2 != variable_vector_end (c2)
	      && (*i2)->dom_number < (*i1)->dom_number))
	current = *(i2++);
      else
	current = *(i1++);

      while ((other = worklist_top (&worklist)) != NULL
	     && !variable_dominates (*other, current))
	worklist_pop (&worklist);

      if (other != NULL && intersect (program, current, *other))
	return true;
      worklist_push (&worklist, &current);
    }

  worklist_destroy (&worklist);

  return false;
}

/* Return true if V dominates W.  */
static bool variable_dominates(Variable *restrict v, Variable *restrict w)
{
  if (block_dominates (v->def_block, w->def_block))
    return true;

  if (v->def_block != w->def_block)
    return false;

  return v->dom_number > w->dom_number;
}

/* Return true if the lifetimes of V and W intersect.  It is assumed
   that the definition of V dominates the definition of W.  */
static bool
intersect (Program *program, Variable *v, Variable *w)
{
  /* Using the SSA property, the lifetimes of V and W intersect if
     either W is live at the definition of V, or vice versa.  As the
     definition of V dominates the definition of W, we just have to
     check whether V is live at the (end of the) definition of W. */
  return variable_live_at (program, w->def_block, w->def_number, v);
}
