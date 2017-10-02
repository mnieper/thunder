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

#ifndef VECTOR2_H_INCLUDED
#define VECTOR2_H_INCLUDED

#include "obstack.h"

#ifndef obstack_chunk_alloc
# include "xalloc.h"
# define obstack_chunk_alloc xmalloc
#endif
#ifndef obstack_chunk_free
# include "stdlib.h"
# define obstack_chunk_free free
#endif

#define DEFINE_VECTOR(Vector, Element, vector)		\
  typedef struct vector Vector;				\
  struct vector						\
  {							\
    struct obstack obstack;				\
  };							\
							\
  static inline void					\
  vector##_init (Vector *v)				\
  {							\
    obstack_init (&v->obstack);				\
  }							\
							\
  static inline void					\
  vector##_destroy (Vector *v)				\
  {							\
    obstack_finish (&v->obstack);			\
    obstack_free (&v->obstack, NULL);			\
  }							\
							\
  static inline void					\
  vector##_clear (Vector *v)				\
  {							\
    void *p = obstack_finish (&v->obstack);		\
    obstack_free (&v->obstack, p);			\
  }							\
							\
  static inline Element *				\
  vector##_begin (Vector *v)				\
  {							\
   return obstack_base (&v->obstack);			\
  }							\
							\
  static inline Element *				\
  vector##_end (Vector *v)				\
  {							\
    return obstack_next_free (&v->obstack);		\
  }							\
							\
  static inline bool					\
  vector##_empty (Vector *v)				\
  {							\
    return vector##_begin (v) == vector##_end (v);	\
  }							\
							\
  static inline size_t					\
  vector##_size (Vector *v)				\
  {							\
    return (obstack_object_size (&v->obstack))		\
      / sizeof (Element);				\
  }							\
  							\
  static inline void					\
  vector##_push (Vector *v, Element *e)			\
  {							\
    obstack_grow (&v->obstack, e, sizeof (Element));	\
  }							\
  							\
  static inline Element *					\
  vector##_top (Vector *v)					\
  {								\
    return vector##_end (v) - 1;				\
  }								\
  								\
  static inline Element	*					\
  vector##_pop (Vector *v)					\
  {								\
    if (vector##_empty (v))					\
      return NULL;						\
    obstack_blank_fast (&v->obstack, -(int) sizeof (Element));	\
    return vector##_end (v);					\
  }

#define vector_foreach(e, Vector, Element, vector, v)			\
  for (Element *(e) = vector##_begin ((v));				\
       (e) != vector##_end ((v));					\
       (e)++)

#endif /* VECTOR2_H_INCLUDED */
