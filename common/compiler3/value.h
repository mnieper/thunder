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

#ifndef COMPILER_VALUE_H_INCLUDED
#define COMPILER_VALUE_H_INCLUDED

#include <stdbool.h>
#include <stdio.h>

#include "compiler/common.h"

bool value_immediate (Value value);

Value new_value (Compiler *compiler, Object operand, Instruction *def);
Value block_value (Block *block);
Value get_label (Compiler *compiler, Object operand);

void value_out_str (FILE *out, Value value);

#endif /* COMPILER_VALUE_H_INCLUDED */
