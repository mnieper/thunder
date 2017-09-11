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
#include <libthunder.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#include "compiler.h"
#include "macros.h"
#include "runtime.h"
#include "vmcommon.h"


int
main (int argc, char *argv)
{
  init ();

  Vm *vm = vm_create ();
  Heap *heap = &vm->heap;
  
  Object p;

  p = make_null ();
  ASSERT (is_null (p));

  p = make_char ('a');
  ASSERT (is_char (p));
  ASSERT (char_value (p) == 'a');

  p = make_boolean (true);
  ASSERT (is_boolean (p));
  ASSERT (boolean_value (p));

  p = make_boolean (false);
  ASSERT (is_boolean (p));
  ASSERT (!boolean_value (p));

  p = cons (heap, make_null (), make_null ());
  ASSERT (is_pair (p));
  ASSERT (is_null (car (p)));
  ASSERT (is_null (cdr (p)));  

  p = make_symbol (heap, u8"symbol", strlen (u8"symbol"));
  ASSERT (is_symbol (p));
  ASSERT (symbol_length (p) == strlen (u8"symbol"));
  ASSERT (memcmp (symbol_bytes (p), u8"symbol", strlen (u8"symbol")) == 0);
  ASSERT (is_symbol (SYMBOL(QUOTE)));
  
  p = make_string (heap, 2, 64);
  ASSERT (is_string (p));
  string_set (p, 0, 65);
  ASSERT (is_string (p));
  ASSERT (string_ref (p, 0) == 65);
  ASSERT (string_ref (p, 1) == 64);
  ASSERT (string_length (p) == 2);
  
  p = make_vector (heap, 3, make_char ('a'));
  ASSERT (is_vector (p));
  vector_set (heap, p, 0, make_null ());
  ASSERT (is_vector (p));
  ASSERT (is_null (vector_ref (p, 0)));
  ASSERT (is_char (vector_ref (p, 1)));
  ASSERT (is_char (vector_ref (p, 2)));
  ASSERT (vector_length (p) == 3);

  Object proc = make_procedure (heap, list (heap,
					     list (heap, INSTRUCTION(entry)),
					     list (heap,
						   INSTRUCTION(movi),
						   SYMBOL(V0),
						   exact_number (heap, 42, 1)),
					     list (heap, INSTRUCTION(ret))));
  ASSERT (is_procedure (proc));
  ASSERT (assembly_entry_point_number (procedure_assembly (proc)) == 1);

  Object closure = make_closure (heap, proc, 1, make_char ('a'));
  ASSERT (closure_procedure (closure) == proc);
  ASSERT (closure_ref (closure, 0) == make_char ('a'));
  closure_set (heap, closure, 0, make_char ('b'));
  ASSERT (closure_ref (closure, 0) == make_char ('b'));
  ASSERT (closure_call (vm, closure, 0) == 42);
  ASSERT (closure_length (closure) == 1);
  
  mpq_t r;
  mpq_t *q;
  mpq_init (r);
  p = make_exact_number (heap, r);
  ASSERT (is_exact_number (p));
  q = exact_number_value (p);
  ASSERT (mpq_cmp_si (*q, 0, 1) == 0);
  mpq_clear (r);
  
  mpc_t x;
  mpc_t *y;
  mpc_init2 (x, 53);
  mpc_set_ui (x, 1, MPC_RNDNN);
  p = make_inexact_number (heap, x);
  ASSERT (is_inexact_number (p));
  y = inexact_number_value (p);
  ASSERT (mpc_cmp_si (*y, 1) == 0);
  mpc_clear (x);
      
  vm_free (vm);
}
