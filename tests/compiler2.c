/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <stdbool.h>

#include "assure.h"

#include "compiler2/common.h"



#include "bitset.h"


#include "xalloc.h"
#include <stdio.h>

static void dump_program (Program *);

int main (void)
{
  Program program;
  program_init (&program);

  Block* b[] = {
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program),
    program_add_block (&program)
  };

  program_add_edge (b[0], b[1]);
  program_add_edge (b[1], b[10]);
  program_add_edge (b[1], b[2]);
  program_add_edge (b[2], b[7]);
  program_add_edge (b[2], b[3]);
  program_add_edge (b[3], b[4]);
  program_add_edge (b[4], b[5]);
  program_add_edge (b[5], b[6]);
  program_add_edge (b[5], b[4]);
  program_add_edge (b[6], b[1]);
  program_add_edge (b[7], b[8]);
  program_add_edge (b[8], b[9]);
  program_add_edge (b[8], b[5]);
  program_add_edge (b[9], b[7]);

  Variable *a1 = program_add_variable (&program);
  Variable *a2 = program_add_variable (&program);
  Variable *a3 = program_add_variable (&program);

  Instruction *ins;

  ins = instruction_frob ();
  block_add_instruction (b[4], ins);
  instruction_add_dest (ins, a2);
  ins = instruction_frob ();
  block_add_instruction (b[8], ins);
  instruction_add_dest (ins, a3);

  ins = block_add_phi (b[5]);
  instruction_add_dest (ins, a1);
  instruction_add_source (ins, a2);
  instruction_add_source (ins, a3);

  for (size_t i = 0; i < 11; i++)
    block_add_instruction (b[i], instruction_switch ());

  program_init_dfs_tree (&program);

  for (size_t i = 0; i < block_count (&program); i++)
    {
      assure (program.preorder[i] == b[i]);
      assure (b[i]->preorder_number == i);
    }

  size_t reverse_postorder[] = {
    0, 1, 3, 7, 8, 9, 10, 4, 5, 6, 2
  };
  for (size_t i = 0; i < block_count (&program); i++)
    {
      assure (program.preorder[i]->reverse_postorder_number
	      == reverse_postorder[i]);
      assure (program.reverse_postorder[i]->reverse_postorder_number == i);
    }

  {
    size_t i = 0;
    reverse_postorder_foreach (block, &program)
      assure (block->reverse_postorder_number == i++);
  }

  program_init_dominance_tree (&program);
  {
    Block *idoms[] = {
      NULL, b[0], b[1], b[2], b[2], b[2], b[5], b[2], b[7], b[8], b[1]
    };
    for (size_t i = 0; i < block_count (&program); i++)
      assure (b[i]->idom == idoms[i]);
  }

  program_init_liveness (&program);

  /*
  for (size_t i = 0; i < 11; i++)
    {
      printf ("Block %zd (dom = %zd):", i, b[i]->dom_order_number);
      Block *block = b[i];
      for (BitsetSize j = 0;
	   (j = bitset_next (block->liveness_t, j, 1)) < 11;
	   j++)
	printf (" %lld", j);
      printf ("\n");
      printf ("Max Dom = %zd\n\n", block->max_dom_number);
    }
  */

  program_convert_out_of_ssa (&program);


  dump_program (&program);


  program_destroy (&program);
}

/* -------------------------------------------------------------------------- */

static void
dump_instruction (Instruction *ins)
{
  dest_foreach (var, ins)
    printf ("%zd ", var->dom_number);
  printf ("= ");
  if (ins->phi)
    printf ("phi");
  else if (ins->move)
    printf ("mov");
  else if (ins->frob)
    printf ("frob");
  else
    printf ("switch");
  source_foreach (var, ins)
    printf (" %zd", var->dom_number);
  printf ("\n");
}

static void
dump_program (Program *program)
{
  preorder_foreach (block, program)
    {
      printf ("Block: %3zd\n", block->preorder_number);
      printf ("==========\n");
      phi_foreach (phi, block)
	dump_instruction (phi);

      instruction_foreach (ins, block)
	dump_instruction (ins);

      printf ("\n");
    }

  variable_foreach (var, program)
    printf ("Var %zd hat Kongruenz %zd.\n",
	    var->dom_number, var->congruence->dom_number);
}
