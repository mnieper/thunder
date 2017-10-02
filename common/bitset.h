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
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
 * License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifndef BITSET_H_INCLUDED
#define BITSET_H_INCLUDED

#include <stdbool.h>

typedef long long BitsetSize;
typedef struct bitset Bitset;

Bitset *bitset_create (BitsetSize);
void bitset_free (Bitset *);
void bitset_clear (Bitset *);
BitsetSize bitset_size (Bitset *);
bool bitset_at (Bitset *, BitsetSize);
void bitset_set_at (Bitset *, BitsetSize, bool);
void bitset_union (Bitset *restrict, Bitset *restrict);
void bitset_diff (Bitset *restrict, Bitset *restrict);
BitsetSize bitset_next (Bitset *, BitsetSize, bool);

#endif /* BITSET_H_INCLUDED */
