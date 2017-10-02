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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "bitset.h"
#include "xalloc.h"

struct bitset
{
  BitsetSize size;
  uint32_t bits[];
};

static size_t
bit_buffer_size (BitsetSize size)
{
  return (size / 32 + (size % 32 != 0)) *  sizeof (uint32_t);
}

Bitset *
bitset_create (BitsetSize size)
{
  Bitset *set = xzalloc (offsetof (struct bitset, bits)
			 + bit_buffer_size (size));
  set->size = size;
  return set;
}

void
bitset_free (Bitset *set)
{
  free (set);
}

void
bitset_clear (Bitset *set)
{
  memset (set->bits, 0, bit_buffer_size (set->size));
}

BitsetSize bitset_size (Bitset *set)
{
  return set->size;
}

bool
bitset_at (Bitset *set, BitsetSize i)
{
  return (set->bits[i / 32] & (1UL << i % 32)) != 0;
}

void
bitset_set_at (Bitset *set, BitsetSize i, bool v)
{
  if (v)
    set->bits[i / 32] |= 1UL << i % 32;
  else
    set->bits[i / 32] &= ~(1UL << i % 32);
}

void
bitset_union (Bitset *restrict set, Bitset *restrict other_set)
{
  size_t size = bit_buffer_size (set->size) / sizeof (uint32_t);
  for (size_t i = 0; i < size; i++)
    set->bits[i] |= other_set->bits[i];
}

void
bitset_diff (Bitset *restrict set, Bitset *restrict other_set)
{
  size_t size = bit_buffer_size (set->size) / sizeof (uint32_t);
  for (size_t i = 0; i < size; ++i)
    set->bits[i] &= ~other_set->bits[i];
}

BitsetSize
bitset_next (Bitset *set, BitsetSize i, bool v)
{
  BitsetSize j = i % 32;
  BitsetSize k = i / 32;
  uint32_t a;
  if (j != 0)
    {
      a = set->bits[k] >> j;
      while (i < set->size && j < 32)
	{
	  if ((a & 1) == v)
	    return i;
	  a >>= 1;
	  ++j;
	  ++i;
	}
      ++k;
    }
  while (i + 32 <= set->size && (a = set->bits[k]) == (v ? ~0 : 0))
    {
      ++k;
      i += 32;
    }
  if (i == set->size)
    return i;
  a = set->bits[k];
  while (i < set->size && (a & 1) != v)
    {
      a >>= 1;
      ++i;
    }
  return i;
}
