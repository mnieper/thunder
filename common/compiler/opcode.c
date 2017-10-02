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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>

#include "assure.h"
#include "bitrotate.h"
#include "common.h"
#include "hash.h"
#include "xalloc.h"

typedef struct opcode_table OpcodeTable;

static OpcodeTable *opcode_table_create ();
static void opcode_table_free (OpcodeTable *table);
static size_t opcode_table_hash (Opcode const *opcode, size_t n);
static bool opcode_table_compare (Opcode const *opcode1, Opcode const *opcode2);
static Opcode *opcode_table_lookup (OpcodeTable *table, Object name);
static void opcode_table_insert (OpcodeTable *table, Object name);

#define DECLARE_PARSER(type)						\
  static void opcode_parse_##type (Parser *parser, Block *block,	\
				   Opcode const *opcode, Object operands);
DECLARE_PARSER(ri)
DECLARE_PARSER(branch_r)
DECLARE_PARSER(jmp)
DECLARE_PARSER(ret)
#undef DECLARE_PARSER

static OpcodeTable *opcodes;

struct opcode
{
  Object name;
  void (* const parse) (Parser *, Block *, const Opcode *, Object);
};

enum {
#define EXPAND_OPCODE(name)			\
  OPCODE_INDEX_##name,
# include "opcodes.def"
#undef EXPAND_OPCODE
  OPCODE_COUNT
};

#define EXPAND_OPCODE(name, type)		\
  static Opcode opcode##_name = {		\
    .parse = opcode_parse##_type		\
  };

#define OPCODE(name) SYMBOL(OPCODE_##name)

void
init_opcode (void)
{
  opcodes = opcode_table_create ();
}

void
finish_opcode (void)
{
  opcode_table_free (opcodes);
}

Opcode *
opcode_lookup (Object name)
{
  Opcode const *opcode = opcode_table_lookup (opcodes, name);
  assure (opcode != NULL);
  return opcode;
}

void
opcode_parse (Parser *parser, Block *block, const Opcode *opcode,
	      Object operands)
{
  opcode->parse (parser, block, opcode, operands);
}

static OpcodeTable *
opcode_table_create ()
{
  OpcodeTable *table = (OpcodeTable *) hash_initialize (OPCODE_COUNT,
							NULL,
							opcode_table_hash,
							opcode_table_compare,
							NULL);
  if (table == NULL)
    xalloc_die ();
#define EXPAND_OPCODE(name, type)				\
  opcode_table_insert (table, OPCODE(name), opcode##_name)
# include "opcodes.def"
#undef EXPAND_OPCODE
  return table;
}

static size_t
opcode_table_hash (Opcode const *opcode, size_t n)
{
  size_t val = rotr_sz ((size_t) opcode->name, 3);
  return val % n;
}

static bool
opcode_table_compare (Opcode const *opcode1, Opcode const *opcode2)
{
  return opcode1->name == opcode2->name;
}

static Opcode *
opcode_table_lookup (OpcodeTable *table, Object name)
{
  return (Opcode *) hash_lookup ((Hash_table *) table,
				 &(Opcode) { .name = name });
}

static void
opcode_table_insert (OpcodeTable *table, Object name, Opcode *opcode)
{
  opcode->name = name;
  if (hash_insert ((Hash_table *) table, opcode) == NULL)
    xalloc_die ();
}

#define DEFINE_PARSER(type)						\
  static void								\
  opcode_parse_##type (Parser *parser, Block *block,			\
		       Opcode const *opcode, Object operands)
#define ASSURE_NO_MORE_OPERANDS			\
  do						\
    {						\
      assure (is_null (operands))		\
    }						\
  while (0)

DEFINE_PARSER(ri)
{
  Instruction *ins = block_add_instruction (block, opcode);
  parser_parse_dest_reg (parser, ins, &operands);
  parser_parse_source_reg (parser, ins, &operands);
  parser_parse_immediate (parser, &operands);
  ASSURE_NO_MORE_OPERANDS;
}

DEFINE_PARSER(branch_r)
{
  Instruction *ins = block_add_instruction (block, opcode);
  parser_parse_label (parser, block, &operands);
  parser_parse_label (parser, block, &operands);
  parser_parse_source_reg (parser, ins, &operands);
  parser_parse_source_reg (parser, ins, &operands);
  ASSURE_NO_MORE_OPERANDS;
  parser_terminate_block (parser);
}

DEFINE_PARSER(jmp)
{
  block_add_instruction (block, opcode);
  parser_parse_label (parser, block, &operands);
  ASSURE_NO_MORE_OPERANDS;
  parser_terminate_block (parser);
}

DEFINE_PARSER(ret)
{
  block_add_instruction (block, opcode);
  // TODO: Add RETURN register
  ASSURE_NO_MORE_OPERANDS;
  parser_terminate_block (parser);
}

#undef ASSURE_NO_MORE_OPERANDS
#undef DEFINE_PARSER
