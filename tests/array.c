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
#include <stddef.h>

#include "array.h"
#include "assure.h"

DEFINE_ARRAY (Array, int, array)

int main (void)
{
  Array *a = array_create ();

  assure (array_lookup (a, 0) == NULL);

  array_insert (a, 0, 0);
  array_insert (a, 1, 1);

  assure (*array_lookup (a, 0) == 0);
  assure (*array_lookup (a, 1) == 1);

  array_insert (a, 0, 2);

  assure (*array_lookup (a, 0) == 2);

  assure (array_lookup (a, 2) == NULL);

  array_free (a);
}
