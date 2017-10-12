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
#include <stddef.h>
#include <stdlib.h>

#include "bitset.h"
#include "common.h"
#include "set.h"

#define PROGRAM (&(compiler)->program)

/*
 * Overview over the algorithm
 * ===========================
 *
 * We have a prespill-phase. We go through the dominator tree.
 * Whenever register pressure becomes too high, we mark a register to
 * be spilled.  For each subsequent use of the register, we lower the
 * amount of free registers (so that other registers may be spilled).
 *
 * Afterwards, the coloring phase starts.  For unspilled registers,
 * nothing is to be done.  For spilled registers, we make a post-order
 * traversal of the dominance tree.  As long as a register is free,
 * may use that register to reflect the value of a spilled register.
 * We have to load it as soon as that register is needed.
 *
 * Does Post-Order-Traversal work? Or should we think of new
 * life-spans (?)
 * New life-spans are subtrees created during first preorder traversal.
 * When a re-spill occurs, the current sub-tree is shortened... What
 * is the current subtree?
 *
 *
 * Problem: Wenn ein gespilltes Register zurückgeholt wird, brauchen
 * wir ein oder zwei Scratch-Register.  Woher bekommen wir diese? Wir
 * müssen evtl.  weitere Register spillen.  Das zieht dann aber bei
 * vorhergehenden Uses dieser neu gespillten Register den Bedarf eines
 * Scratch-Registers nach sich.  Aber das geht immer, da wir einfach
 * das Register nehmen können, was es vorher inne hatte.  Wie sieht
 * das dann mit späteren Uses aus?  Idee: Wir merken uns die Register
 * im Traversal des Dominance-Trees.  Genaue Datenstruktur???
 */

static void index_instructions (Program *program);
static size_t index_variables (Program *program);
static void init_bitsets (Program *program, size_t var_count);
static void init_defs_uses (Program *program);
static void init_live_in (Program *program, size_t var_count);
static bool liveness_local_flow (Program *program, Block *block,
				 Bitset **varset);

DEFINE_SET (BlockSet, Block, block_set)
DEFINE_VECTOR (Worklist, Block *, worklist)

#define worklist_foreach(b, w)				\
  vector_foreach(b, Worklist, Block *, worklist, w)

#define PROGRAM (&(compiler)->program)

void
program_register_allocate (struct compiler *compiler)
{
  index_instructions (PROGRAM);
  size_t var_count = index_variables (PROGRAM);
  init_bitsets (PROGRAM, var_count);
  init_defs_uses (PROGRAM);
  init_live_in (PROGRAM, var_count);

  // Für jede Variable erzeuge ein Intervall.  Diese liegen in
  // "unhandled".  Wir gehen diese in Dominance-Tree-Reihenfolge
  // durch.
  // Am Anfang sind alle Variablen auf "not spilled" gesetzt.
  //
  // Sobald ein Intervall bearbeitet wird, kommt es in die "Active
  // Liste".
  //
  // Wenn die Register Pressure zu hoch ist, müssen wir spillen.  Dazu
  // machen wir folgendes: Wir suchen das "active" Intervall mit dem
  // spätesten Endpunkt.  Das fliegt heraus.
  // Wenn wir ein "Use" oder "Def" finden, und es gibt kein Intervall,
  // so erzeugen wir ein neues Intervall in diesem
  // Dominance-Tree-Abschnitt.  Insgesamt gehören zu einer Variablen
  // dann eventuell mehrere Intervalle.  Diese sind disjunkt.  Zu den
  // Intervallen gehört jeweils eine Variable.  Wir müssen auch noch
  // nachhalten, welche Register zu welchem Zeitpunkt besetzt sind.
  //
  // Um die kleinen Bäume zu machen, brauchen wir auch immer
  // "last-use" (wie ist es mit "Last-Def"?) in einem Dominance-Tree.
  // Außerdem brauchen wir die Zahl eines "Spill-Slots".
  //
  // Zu "Definitionen vs. Uses":
  //
  // X <--- Y
  // Y <--- Z
  // In diesem Falle haben X und Z den gleichen Zeitstempel.  Wir
  // dürfen X aber vergessen, denn die Zeit könnte abgelaufen sein.
  // (Es ist "Def".)
  //
  // Wir brauchen Algorithmus: Gegeben Teilbaum des
  // Dominance-Trees. Wann ist die Livetime einer Kongruenzklasse
  // dort?


}

void
index_instructions (Program *program)
{
  size_t time = 0;
  domorder_foreach (block, program)
    {
      block_instruction_foreach (ins, block)
	INSTRUCTION_TIME (ins) = time++;
    }
}

size_t
index_variables (Program *program)
{
  program_block_foreach (block, program)
    {
      block_instruction_foreach (ins, block)
	{
	  dest_foreach (var, ins)
	    VARIABLE_INDEX (var) = -1;
	}
    }
  ptrdiff_t index = 0;
  program_block_foreach (block, program)
    {
      block_instruction_foreach (ins, block)
	{
	  dest_foreach (var, ins)
	    if (VARIABLE_INDEX (var) == -1)
	      VARIABLE_INDEX (var) = index++;
	}
    }
  return index;
}

void
init_bitsets (Program *program, size_t var_count)
{
  program_block_foreach (block, program)
    {
      BLOCK_DEFS (block) = bitset_create (var_count);
      BLOCK_USES (block) = bitset_create (var_count);
      BLOCK_LIVE_IN (block) = bitset_create (var_count);
      /* No need for LIVE_OUT (XXX) */
      BLOCK_LIVE_OUT (block) = bitset_create (var_count);
    }
}

void
init_defs_uses (Program *program)
{
  program_block_foreach (block, program)
    {
      InstructionListPosition *pos = block_terminating (block);
      while ((pos = block_previous_instruction (block, pos)) != NULL)
	{
	  Instruction *ins = block_instruction_at (block, pos);
	  dest_foreach (var, ins)
	    {
	      bitset_set_at (BLOCK_DEFS (block), VARIABLE_INDEX (var), true);
	      bitset_set_at (BLOCK_USES (block), VARIABLE_INDEX (var), false);
	    }
	  source_foreach (var, ins)
	    bitset_set_at (BLOCK_USES (block), VARIABLE_INDEX (var), true);
	}
    }
}

void
init_live_in (Program *program, size_t var_count)
{
  Bitset *varset = bitset_create (var_count);

  Worklist old_worklist, new_worklist, tmp_worklist;
  worklist_init (&old_worklist);
  worklist_init (&new_worklist);

  BlockSet in_worklist = block_set_create ();

  postorder_foreach (block, program)
    worklist_push (&new_worklist, &block);

  while (!worklist_empty (&new_worklist))
    {
      tmp_worklist = old_worklist;
      old_worklist = new_worklist;
      new_worklist = tmp_worklist;

      worklist_foreach (block, &old_worklist)
	if (liveness_local_flow (program, *block, &varset))
	  predecessor_foreach (pred, *block)
	    if (block_set_adjoin (in_worklist, pred))
	      worklist_push (&new_worklist, block);

      worklist_clear (&old_worklist);
      block_set_clear (in_worklist);
    }

  block_set_free (in_worklist);

  worklist_destroy (&old_worklist);
  worklist_destroy (&new_worklist);

  bitset_free (varset);
}

bool
liveness_local_flow (Program *program, Block *block, Bitset **varset)
{
  Bitset *out = *varset;
  bitset_clear (out);
  successor_foreach (succ, block)
    bitset_union (out, BLOCK_LIVE_IN (succ));

  Bitset *in = out;
  bitset_diff (in, BLOCK_DEFS (block));
  bitset_union (in, BLOCK_USES (block));
  if (!bitset_equal (in, BLOCK_LIVE_IN (block)))
    {
      Bitset *tmp = BLOCK_LIVE_IN (block);
      BLOCK_LIVE_IN (block) = in;
      *varset = tmp;
      return true;
    }

  bitset_free (in);
  return false;
}
