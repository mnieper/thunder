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

#ifndef COMPILER_PARSER_H
#define COMPILER_PARSER_H
#include <lightning.h>

#include "object.h"

typedef struct parser Parser;

struct compiler;
struct instruction;

void parse_code (struct compiler *compiler, Object code);

void parser_terminate_block (Parser *parser);
void parser_parse_label (Parser *parser, Object *operands);
void parser_parse_register (Parser *parser, struct instruction *ins,
			    Object *operands);
void parser_define_var (Parser *parser, struct instruction *ins);
jit_word_t parser_parse_immediate (Parser *parser, Object *operands);

#endif
