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

#ifndef COMPILER_DOMINANCE_H_INCLUDED
#define COMPILER_DOMINANCE_H_INCLUDED

#include "block.h"

#define dom_child_foreach(child, block)					\
  list_foreach (child, BlockList, Block, block_list,			\
		block_dom_children ((block)))

#define domorder_foreach(block, program)			\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (Block *(block); b##__LINE__; b##__LINE__ = false)		\
      for (size_t i##__LINE__ = 0, blocks##__LINE__ = block_count ((program)); \
	   i##__LINE__ < blocks##__LINE__				\
	     && ((block) = PROGRAM_DOMORDER ((program))[i##__LINE__])	\
	     && true;							\
	   i##__LINE__++)

void program_init_dominance (struct compiler *compiler);

#endif /* COMPILER_DOMINANCE_H_INCLUDED */
