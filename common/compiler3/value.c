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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <stddef.h>

#include "compiler/common.h"
#include "compiler/error.h"
#include "compiler/label.h"
#include "compiler/register.h"
#include "compiler/value.h"

Value
new_value (Compiler *compiler, Object operand, Instruction *def)
{
  Value value;
  if (is_symbol (operand))
    {
      char *s = symbol_value (operand);
      switch (s[0])
	{
	case '&':
	  {
	    jit_pointer_t address = lt_dlsym (module, s + 1);
	    if (address == NULL)
	      compiler_error ("%s", lt_dlerror ());
	    value.type = VALUE_IMMEDIATE;
	    value.word = (jit_word_t) address;
	    break;
	  }
	case '$':
	  if (strcmp (s + 1, "true") == 0)
	    {
	      value.type = VALUE_IMMEDIATE;
	      value.word = make_boolean (true);
	      break;
	    }
	  if (strcmp (s + 1, "false") == 0)
	    {
	      value.type = VALUE_IMMEDIATE;
	      value.word = make_boolean (false);
	      break;
	    }
	  if (strcmp (s + 1, "null") == 0)
	    {
	      value.type = VALUE_IMMEDIATE;
	      value.word = make_null ();
	      break;
	    }
	  compiler_error ("%s: %s", "unknown constant value", s);
	case '%':
	  value.type = VALUE_REGISTER;
	  value.reg = register_intern (compiler, operand, def);
	  break;
	default:
	  value.type = VALUE_LABEL;
	  value.label = label_intern (compiler, operand, NULL);
	  break;
	}

      free (s);
    }

  else if (is_string (operand))
    {
      char *s = string_value (operand);
      value.type = VALUE_IMMEDIATE;
      value.word = (jit_word_t) obstack_copy0 (&COMPILER_DATA (compiler), s,
						strlen (s));
      free (s);
    }

  else if (is_char (operand))
    {
      value.type = VALUE_IMMEDIATE;
      value.word = char_value (operand);
    }

  else if (is_exact_number (operand))
    {
      value.type = VALUE_IMMEDIATE;
      value.word = fixnum (operand);
    }

  else if (is_inexact_number (operand))
    {
      value.type = VALUE_FLOAT;
      value.float64 = flonum_d (operand);
    }

  else
    compiler_error ("%s: %s", "bad operand", object_get_str (operand));

  if (def != NULL && value.type != VALUE_REGISTER)
    compiler_error ("%s: %s", "not an lvalue", object_get_str (operand));

  return value;
}

Value
get_label (Compiler *compiler, Object operand)
{
  Value value = new_value (compiler, operand, NULL);
  if (value.type != VALUE_LABEL)
    compiler_error ("%s: %s", "not a label", object_get_str (operand));
  return value;
}

Value
block_value (Block *block)
{
  return (Value) { .type = VALUE_BLOCK, .block = block };
}

bool
value_immediate (Value value)
{
  return value.type == VALUE_IMMEDIATE;
}

void
value_out_str (FILE *out, Value value)
{
  switch (value.type)
    {
    case VALUE_LABEL:
      {
	char *s = object_get_str (value.label->name);
	fputs (s, out);
	free (s);
	break;
      }
    case VALUE_BLOCK:
      {
	char *s = object_get_str (value.block->name);
	fputs (s, out);
	free (s);
	break;
      }
    case VALUE_REGISTER:
      {
	char *s = object_get_str (value.reg->name);
	fputs (s, out);
	free (s);
	break;
      }
    case VALUE_IMMEDIATE:
      {
	fprintf (out, "%lld", (long long) value.word);
	break;
      }
    case VALUE_FLOAT:
      {
	fprintf (out, "%g", value.float64);
	break;
      }
    case VALUE_GPR:
    case VALUE_FPR:
      /* TODO */
      break;
    }
}
