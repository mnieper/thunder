/*
 * Copyright (C) 2017  Marc Nieper-Wißkirchen
 *
 * This file is part of Thunder.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Thunder is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#if HAVE_CONFIG_H
# include <config.h>
#endif
#include <assert.h>
#include <stdbool.h>

#include "bitset.h"

int main (void)
{
  Bitset *b = bitset_create (50);

  for (BitsetSize i = 0; i < 50; ++i)
    assert (!bitset_at (b, i));

  for (BitsetSize i = 0; i < 50; i += 3)
    bitset_set_at (b, i, true);

  for (BitsetSize i = 0; i < 50; ++i)
    assert (bitset_at (b, i) == (i % 3 == 0));

  bitset_clear (b);

  bitset_set_at (b, 10, true);
  bitset_set_at (b, 32, true);
  bitset_set_at (b, 33, true);
  bitset_set_at (b, 40, true);

  int i = 0;
  i = bitset_next (b, i, false);
  assert (i == 0);
  i = bitset_next (b, i, true);
  assert (i == 10);
  i = bitset_next (b, i, false);
  assert (i == 11);
  i = bitset_next (b, i, true);
  assert (i == 32);
  i = bitset_next (b, i + 1, true);
  assert (i == 33);
  i = bitset_next (b, i + 1, true);
  assert (i == 40);
  i = bitset_next (b, i + 1, true);

  bitset_clear (b);
  Bitset *c = bitset_create (50);

  bitset_set_at (b, 0, true);
  bitset_set_at (c, 1, true);
  bitset_union (b, c);
  assert (bitset_at (b, 0));
  assert (bitset_at (b, 1));
  assert (!bitset_at (c, 0));

  // TODO(XXX): Test diff!

  bitset_free (b);
  bitset_free (c);
}
