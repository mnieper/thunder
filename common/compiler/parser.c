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

#include "array.h"
#include "assure.h"
#include "common.h"

static void parser_init (Compiler *compiler, Parser *parser);
static void parser_destroy (Parser *parser);
static void parser_parse (Parser *parser);
static Block *get_current_block (Parser *parser);

DEFINE_ARRAY(BlockTable, Block *, block_table)

#define PROGRAM (parser->compiler.program)

void parse_code (Compiler *compiler, Object *code)
{
  Parser parser;

  parser_init (compiler, &parser);
  parser_parse (&parser);
  parser_destroy (&parser);
}

struct parser
{
  Compiler *compiler;
  BlockTable *block_table;
  size_t current_instruction_number;
};

static void
parser_init (Compiler *compiler, Parser *parser)
{
  parser->compiler = compiler;
  parser->current_instruction_number = 0;
}

static void
parser_finish (Parser *parser)
{


}

static void
parser_parse (Parser *parser, Object code)
{
  while (!is_null (code))
    {
      assure (is_pair (code));
      Object const stmt = car (code);
      code = cdr (code);
      assure (is_pair (stmt));
      Opcode const *opcode = opcode_lookup (car (stmt));
      Block *block = get_current_block (parser);
      opcode_parse (parser, block, opcode, cdr (stmt));
      ++parser->instruction_number;
    }
  assure (parser->current_block == NULL);
}

static Block *
get_current_block (Parser *parser)
{
  if (parser->current_block == NULL)
    {
      parser->current_block = program_add_block (PROGRAM);
      block_table_insert (parser->block_table,
			  parser->current_block,
			  parser->current_instruction_number);
    }
  return parser->current_block;
}
