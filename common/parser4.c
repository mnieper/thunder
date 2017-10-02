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
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "hash.h"
#include "parser.h"
#include "program.h"
#include "vector2.h"
#include "xalloc.h"

/* Parsing */

struct parser_edge
{
  Block *source;
  size_t target_number;
};

struct parser_use
{
  Label *label;
  size_t reg_number;
};

DEFINE_VECTOR (ParserEdgeStack, struct parser_edge, parser_edge_stack)
DEFINE_VECTOR (ParserUseStack, struct parser_use, parser_use_stack)

typedef struct parser Parser;
struct parser
{
  Program *program;
  size_t label_number;
  Block *block;
  Hash_table *blocks;
  Hash_table *regs;
  ParserEdgeStack edges;
  ParserUseStack uses;
};

struct block_entry
{
  size_t number;
  Block *block;
};

struct reg_entry
{
  size_t number;
  VirtualReg *reg;
};

static size_t
entry_hash (const void *entry, size_t n)
{
  return *(const size_t *) entry % n;
}

static bool
entry_compare (const void *entry1, const void *entry2)
{
  return *(const size_t *) entry1 == *(const size_t *) entry2;
}

static void
parser_init (Parser *parser, Program *program)
{
  parser->program = program;
  parser->label_number = 0;
  parser->block = NULL;
  parser->blocks = hash_initialize (0, NULL, entry_hash,
				    entry_compare, free);
  parser->regs = hash_initialize (0, NULL, entry_hash,
				  entry_compare, free);
  parser_edge_stack_init (&parser->edges);
  parser_use_stack_init (&parser->uses);
};

static void
parser_finish (Parser *parser)
{
  for (struct parser_edge *p = parser_edge_stack_begin (&parser->edges);
       p != parser_edge_stack_end (&parser->edges);
       ++p)
    {
      struct block_entry *entry = hash_lookup (parser->blocks,
					       &p->target_number);
      assert (entry != NULL);
      Edge *edge = program_add_edge (parser->program);
      block_add_outgoing_edge (p->source, edge);
      block_add_incoming_edge (entry->block, edge);
    }

  for (struct parser_use *p = parser_use_stack_begin (&parser->uses);
       p != parser_use_stack_end (&parser->uses);
       ++p)
  {
      struct reg_entry *entry = hash_lookup (parser->regs, &p->reg_number);
      assert (entry != NULL);
      label_add_use (p->label, entry->reg);
    }

  hash_free (parser->blocks);
  hash_free (parser->regs);

  parser_edge_stack_destroy (&parser->edges);
  parser_use_stack_destroy (&parser->uses);
}

static void
parser_add_edge (Parser *parser, Block *source, size_t target_number)
{
  parser_edge_stack_push (&parser->edges,
			  &(struct parser_edge) { source, target_number });
}

static void
parser_add_use (Parser *parser, Label *label, size_t reg_number)
{
  parser_use_stack_push (&parser->uses,
			 &(struct parser_use) { label, reg_number });
}

static Block *
parser_get_block (Parser *parser)
{
  if (parser->block == NULL)
    {
      parser->block = program_add_block (parser->program);
#ifndef NDEBUG
      block_set_number (parser->block, parser->label_number);
#endif
      struct block_entry *entry = XMALLOC (struct block_entry);
      entry->number = parser->label_number;
      entry->block = parser->block;
      if (hash_insert (parser->blocks, entry) == NULL)
	xalloc_die ();
    }
  return parser->block;
}

Object
parser_get_operand (Object *operands)
{
  assert (is_pair (*operands));
  Object op = car (*operands);
  *operands = cdr (*operands);
  return op;
}

static void
parser_parse_reg (Parser *parser, Label *label, Object *operands)
{
  Object op = parser_get_operand (operands);
  assert (is_exact_number (op));
  parser_add_use (parser, label, fixnum (op));
}

static void
parser_parse_label (Parser *parser, Block *block, Object *operands)
{
  Object op = parser_get_operand (operands);
  assert (is_exact_number (op));
  parser_add_edge (parser, block , fixnum (op));
}

static void
parser_define_reg (Parser *parser, Label *label)
{
  struct reg_entry *entry = XMALLOC (struct reg_entry);
  entry->number = parser->label_number;
  entry->reg = label_add_def (parser->program, label);
#ifndef NDEBUG
  virtual_reg_set_number (entry->reg, parser->label_number);
#endif
  if (hash_insert (parser->regs, entry) == NULL)
    xalloc_die ();
}

static void
parser_terminate_block (Parser *parser)
{
  parser->block = NULL;
}

typedef struct opcode Opcode;
struct opcode
{
  void (* parse) (Parser *, Block *, const Opcode *, Object);
};

static void
parse_jmp (Parser *parser, Block *block, const Opcode *opcode, Object operands)
{
  block_add_label (block, instruction_jump ());

  parser_parse_label (parser, block, &operands);

  parser_terminate_block (parser);
}

static void
parse_ret (Parser *parser, Block *block, const Opcode *opcode, Object operands)
{
  Label *label = block_add_label (block, instruction_ret ());

  // (XXX) parser_parse_reg (parser, label, &operands);

  parser_terminate_block (parser);
}

static void
parse_br (Parser *parser, Block *block, const Opcode *opcode, Object operands)
{
  Label *label = block_add_label (block, instruction_br ());

  parser_parse_label (parser, block, &operands);
  parser_parse_label (parser, block, &operands);

  parser_terminate_block (parser);
}

static void
parse_movr (Parser *parser, Block *block, const Opcode *opcode, Object operands)
{
  Label *label = block_add_label (block, instruction_movr ());

  parser_parse_reg (parser, label, &operands);

  parser_define_reg (parser, label);
}

static void
parse_phi (Parser *parser, Block *block, const Opcode *opcode, Object operands)
{
  Label *phi = block_add_phi_func (block);

  while (!is_null (operands))
    parser_parse_reg (parser, phi, &operands);

  parser_define_reg (parser, phi);
}

static void
parse_switch (Parser *parser, Block *block, const Opcode *opcode,
	      Object operands)
{
  Label *label = block_add_label (block, instruction_switch ());

  while (!is_null (operands))
    parser_parse_label (parser, block, &operands);

  parser_terminate_block (parser);
}

static void
parse_frob (Parser *parser, Block *block, const Opcode *opcode, Object operands)
{
  Label *label = block_add_label (block, instruction_frob ());

  while (!is_null (operands))
    parser_parse_reg (parser, label, &operands);

  parser_define_reg (parser, label);
}

static void
parse_perform (Parser *parser, Block *block, const Opcode *opcode,
	       Object operands)
{
  Label *label = block_add_label (block, instruction_perform ());

  while (!is_null (operands))
    parser_parse_reg (parser, label, &operands);
}

static const Opcode *
lookup_opcode (Object op)
{
  static Opcode jmp_opcode = {
    .parse = parse_jmp
  };

  static Opcode br_opcode = {
    .parse = parse_br
  };

  static Opcode ret_opcode = {
    .parse = parse_ret
  };

  static Opcode movr_opcode = {
    .parse = parse_movr
  };

  static Opcode phi_opcode = {
    .parse = parse_phi
  };

  static Opcode switch_opcode = {
    .parse = parse_switch
  };

  static Opcode perform_opcode = {
    .parse = parse_perform
  };

  static Opcode frob_opcode = {
    .parse = parse_frob
  };

  if (op == SYMBOL(JMP))
    return &jmp_opcode;
  else if (op == SYMBOL(BR))
    return &br_opcode;
  else if (op == SYMBOL(RET))
    return &ret_opcode;
  else if (op == SYMBOL(MOVR))
    return &movr_opcode;
  else if (op == SYMBOL(PHI))
    return &phi_opcode;
  else if (op == SYMBOL(SWITCH))
    return &switch_opcode;
  else if (op == SYMBOL(FROB))
    return &frob_opcode;
  else if (op == SYMBOL(PERFORM))
    return &perform_opcode;
  assert (false);
}

static void
parser_parse_code (Parser *parser, Object code)
{
  while (!is_null (code))
    {
      assert (is_pair (code));
      Object stmt = car (code);
      assert (is_pair (stmt));
      code = cdr (code);
      const Opcode *opcode = lookup_opcode (car (stmt));
      Block *block = parser_get_block (parser);
      opcode->parse (parser, block, opcode, cdr (stmt));
      ++parser->label_number;
    }
  assert (parser->block == NULL);
}

void parse_code (Program *program, Object code)
{
  Parser parser;
  parser_init (&parser, program);
  parser_parse_code (&parser, code);
  parser_finish (&parser);
}
