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

#include "common.h"

static void add_move_instructions (Compiler *compiler);

#define program (&compiler->program)

void
program_convert_out_of_ssa (Compiler *compiler)
{
  add_move_instructions (compiler);
}

void
add_move_instructions (Compiler *compiler)
{
  program_block_foreach (block, program)
    {
      BLOCK_MOVE_IN (block)
	= block_add_instruction_first (compiler, block, get_opcode_movr ());
      BLOCK_MOVE_OUT (block)
	= block_add_instruction_before (compiler,
					block,
					block_terminating (block),
					get_opcode_movr ());
    }
}
