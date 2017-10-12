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

#ifndef PARALLEL_COPY_SEQUENTIALIZATION_H_INCLUDED
#define PARALLEL_COPY_SEQUENTIALIZATION_H_INCLUDED

void sequentialize_parallel_copy (struct compiler *compiler,
				  struct block *block,
				  struct instruction_list_position *pos,
				  struct instruction *ins);

#endif /* PARALLEL_COPY_SEQUENTIALIZATION_H_INCLUDED */
