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
#include <errno.h>
#include <libthunder.h>
#include <lightning.h>
#include <ltdl.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "bitrotate.h"
#include "error.h"
#include "hash.h"
#include "progname.h"
#include "vector.h"
#include "vmcommon.h"
#include "xalloc.h"

#include "compiler/block.h"
#include "compiler/cfg.h"
#include "compiler/error.h"
#include "compiler/common.h"
#include "compiler/instruction.h"
#include "compiler/label.h"
#include "compiler/register.h"

#define FRAME_SIZE 256  /* TODO: Calculate correct value. */

typedef VECTOR(jit_node_t *) EntryPointVector;

lt_dlhandle module;

static jit_pointer_t *trampoline_return;

/* Trampoline stack */
struct stack_layout {
  Vm *vm;
  jit_pointer_t heap_base;
  jit_pointer_t heap_end;
  jit_word_t live_values[6];
};

static jit_int32_t stack_base;

#define stack_store(member, reg)					\
  _Generic(((struct stack_layout) {}).member,				\
	   Vm *: jit_stxi (stack_base + offsetof (struct stack_layout, member), \
			   JIT_FP, reg),				\
	   jit_pointer_t: jit_stxi (stack_base + offsetof (struct stack_layout, member), \
				    JIT_FP, reg),			\
	   unsigned short: jit_stxi_s (stack_base + offsetof (struct stack_layout, member), \
				       JIT_FP, reg))

#define stack_load(reg, member)						\
  _Generic(((struct stack_layout) {}).member,				\
	   Vm *: jit_ldxi (reg, JIT_FP,					\
			   stack_base + offsetof (struct stack_layout, member)), \
	   jit_pointer_t: jit_ldxi (reg, JIT_FP,			\
				    stack_base + offsetof (struct stack_layout, member)), \
	   unsigned short: jit_ldxi_us (reg, JIT_FP,\
					stack_base + offsetof (struct stack_layout, member)))

/* Instruction definitions */

#define OPERAND_TYPE(type) OPERAND_TYPE_##type
#define OPERAND_TYPE_ireg jit_gpr_t
#define OPERAND_TYPE_imm jit_word_t
#define OPERAND_TYPE_fun struct fun
#define OPERAND_TYPE_label struct label *
#define OPERAND_TYPE_freg jit_fpr_t
#define OPERAND_TYPE_float float
#define OPERAND_TYPE_double double
#define OPERAND_TYPE_obj Object

int (*trampoline) (Vm *vm, void *f, void *heap, void *arg);

static jit_state_t *_jit;

void
init_compiler (void)
{
  jit_node_t *vm, *f, *heap, *arg;
  jit_node_t *done;

  jit_set_memory_functions (xmalloc, xrealloc, free);
  init_jit (program_name);

  _jit = jit_new_state ();
  jit_prolog ();
  stack_base = jit_allocai (sizeof (struct stack_layout));
  jit_frame (FRAME_SIZE);
  vm = jit_arg ();
  f = jit_arg ();
  heap = jit_arg ();
  arg = jit_arg ();
  jit_getarg (JIT_R0, vm);
  stack_store (vm, JIT_R0);
  jit_getarg (JIT_V3, heap);
  stack_store (heap_base, JIT_V3);
  jit_addi (JIT_R0, JIT_V3, 1ULL << 20); /* FIXME(XXX): Do not hardcode heap size. */
  stack_store (heap_end, JIT_R0);
  jit_getarg (JIT_R0, f);
  jit_getarg (JIT_V0, arg);
  jit_jmpr (JIT_R0);
  done = jit_indirect ();
  jit_retr (JIT_V0);
  jit_epilog ();
  trampoline = jit_emit();
  trampoline_return = jit_address (done);

  //XXX jit_disassemble ();

  jit_clear_state ();

  init_instructions ();

  module = lt_dlopen (NULL);
}

void
finish_compiler (void)
{
  finish_instructions ();

  jit_destroy_state ();
  finish_jit ();
}

void
assembly_init (Assembly assembly)
{
  assembly->clear = true;
}

void
assembly_clear (Assembly assembly)
{
#define _jit (assembly->jit)
  assembly_destroy (assembly);
  assembly->jit = jit_new_state ();
  obstack_init (&assembly->data);
#undef _jit
}

void
assembly_destroy (Assembly assembly)
{
#define _jit (assembly->jit)
  if (assembly->clear)
    return;

  jit_destroy_state ();
  obstack_free (&assembly->data, NULL);
  free (assembly->entry_points);

  assembly->clear = true;
#undef _jit
}

static void
patch_instructions (Compiler *compiler)
{
  block_foreach (block, &compiler->cfg)
    {
      instruction_foreach (ins, block)
	instruction_patch (compiler, ins);
    }
}

static void
parse_code (Compiler *compiler, Object code)
{
  size_t block_number = 0;
  Block *block = NULL;
  // TODO(XXX): Implement FOREACH in runtime.h
  for (; !is_null (code); code = cdr (code))
    {
      Object stmt = car (code);
      if (is_symbol (stmt))
	{
	  Object name = stmt;

	  Block *next_block = new_block (compiler, name);
	  if (block != NULL)
	    block_append_instruction (block, new_jump (compiler, next_block));
	  block = next_block;

	  Label *label = label_intern (compiler, name, block);
	  continue;
	}

      if (block == NULL)
	block = new_block (compiler, SYMBOL(UNNAMED));

      Instruction *ins = new_instruction (compiler, stmt);
      block_append_instruction (block, ins);

      if (instruction_terminating (ins))
	{
	  block = NULL;
	}
    }

  if (block != NULL)
    compiler_error ("%s", "unterminated basic block");
}

static void
build_cfg (Compiler *compiler)
{
  block_foreach (block, &compiler->cfg)
    {
      Instruction *ins = block_end (block);

      if (instruction_jmp (ins))
	block_add_link (compiler, block, &ins->u);
      else if (instruction_branch (ins))
	{
	  block_add_link (compiler, block, &ins->u);
	  Block *target = cfg_next_block (block);
	  if (target == NULL)
	    {
	      compiler_error ("%s", "missing basic block after branch");
	    }
	  ins->v.block = target;
	  if (target != ins->u.block)
	    block_add_link (compiler, block, &ins->v);
	}
    }
}

static void
split_critical_edge (Link *link, Block **block,
		     Link **source_link,
		     Link **target_link,
		     Compiler *compiler)
{
  *block = new_block (compiler, SYMBOL(UNNAMED));
  Instruction *jump = new_jump (compiler, link->source->block);
  block_append_instruction (*block, jump);
  *source_link = new_link (compiler, link->source);
  *target_link = new_link (compiler, &jump->u);
}

static void
split_critical_edges (Compiler *compiler)
{
  cfg_split_critical_edges (&compiler->cfg,
			    (cfg_edge_splitter) split_critical_edge, compiler);
  /*
  Block *block;
  BLOCK_FOREACH (compiler, block)
    if (block_critical (block))
      {
	Link *link;
	SUCCESSOR_FOREACH (block, link)
	  if (block_critical (link->block))
	    {
	      Block *b = new_block (compiler, SYMBOL(UNNAMED));
	      Instruction *jump = new_jump (compiler, link->block);
	      block_append_instruction (b, jump);
	      (*link->source).block = b;
	      link->block = b;
	      block_add_predecessor (compiler, block, link->source);
	      block_add_link (compiler, b, &jump->u);
	    }
      }
  */
}

static void
find_dominators (Compiler *compiler)
{


}

static void
compiler_dump (Compiler *compiler, char const *path)
{
  /*
  FILE *out = fopen (path, "w");
  if (out == NULL)
    error (EXIT_FAILURE, errno, "cannot open dump file");

  fprintf (out, "digraph G {\n");
  fprintf (out, "  node [shape=record];\n");
  Block *block;
  BLOCK_FOREACH (compiler, block)
    {
      char *s;
      fprintf (out, "  block%zd [label=\"{<i> %s:",
	       block->number,
	       s = object_get_str (block->name));
      free (s);
      Instruction *ins;
      INSTRUCTION_FOREACH (block, ins)
	{
	  fprintf (out, "|");
	  if (instruction_next (ins) == NULL)
	    fprintf (out, "<o> ");

	  instruction_out_str (out, ins);
	}
      fprintf (out, "}\"]\n");

      Link *link;
      SUCCESSOR_FOREACH (block, link)
	fprintf (out, "  block%zd:o:se -> block%zd:i:ne;\n",
		 block->number, link->block->number);
    }
  fprintf (out, "}\n");

  fclose (out);
  */
}

Object
compile (Heap *heap, Object code)
{
# define _jit (COMPILER_JIT (&compiler))
  Resource(ASSEMBLY) *res = resource_manager_allocate (ASSEMBLY,
						       &heap->resource_manager);
  struct compiler compiler = { .assembly = &RESOURCE_PAYLOAD (res) };

  assembly_clear (COMPILER_ASSEMBLY (&compiler));
  obstack_init (&compiler.stack);
  init_registers (&compiler);
  init_labels (&compiler);
  init_cfg (&compiler.cfg);

  parse_code (&compiler, code);
  check_labels (&compiler);
  check_registers (&compiler);
  patch_instructions (&compiler);
  build_cfg (&compiler);
  // join edges (make loops well-defined)
  split_critical_edges (&compiler);
  find_dominators (&compiler);

  compiler_dump (&compiler, "debug.dot");

  finish_cfg (&compiler.cfg);
  finish_registers (&compiler);
  finish_labels (&compiler);
  obstack_free (&compiler.stack, NULL);

  EntryPointVector entry_points;
  vector_init (&entry_points);

  jit_prolog ();
  jit_tramp (FRAME_SIZE);

  /* FIXME */
  jit_prepare ();
  jit_pushargi (0);
  jit_finishi (exit);

  jit_epilog ();

  jit_emit ();

  /*
  COMPILER_ASSEMBLY (compiler)->entry_point_number = vector_size (&entry_points);
  COMPILER_ASSEMBLY (compiler)->entry_points = XNMALLOC (assembly->entry_point_number, EntryPoint);
  {
    jit_node_t **label;
    EntryPoint *p = assembly->entry_points;
    VECTOR_FOREACH (&entry_points, label)
      {
	*p = jit_address (*label);
	++p;
      }
  }
  */

  jit_disassemble (); // XXX
  jit_clear_state ();

  vector_destroy (&entry_points);

  COMPILER_ASSEMBLY(&compiler)->entry_points = NULL; // TODO

  COMPILER_ASSEMBLY(&compiler)->clear = false;

  return (Object) res | POINTER_TYPE;

# undef _jit
}

bool
is_assembly (Object obj)
{
  return (obj & OBJECT_TYPE_MASK) == POINTER_TYPE
    && (((Pointer) obj)[-1] & HEADER_TYPE_MASK) == ASSEMBLY_TYPE;
}

size_t
assembly_entry_point_number (Object assembly)
{
  return (*((Assembly *) assembly)) [0].entry_point_number;
}

EntryPoint *
assembly_entry_points (Object assembly)
{
  return (*((Assembly *) assembly)) [0].entry_points;
}
