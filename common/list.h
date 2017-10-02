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
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * Authors:
 *      Marc Nieper-Wißkirchen
 */

#ifndef LIST_H_INCLUDED
#define LIST_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "gl_xlist.h"
#include "gl_linked_list.h"

#define DEFINE_LIST(List, Element, list, dispose)			\
  typedef struct list List;						\
  struct list								\
  {									\
    gl_list_t gl_list;							\
  };									\
									\
  typedef struct list##_iterator List##Iterator;			\
  struct list##_iterator						\
  {									\
    gl_list_iterator_t gl_list_iterator;				\
  };									\
									\
  typedef struct list##_position List##Position;			\
  struct list##_position;						\
  									\
  static inline	List							\
  list##_create (bool owns)						\
  {									\
    gl_list_t gl_list = gl_list_create_empty				\
      (GL_LINKED_LIST,							\
       NULL,								\
       NULL,								\
       owns ? _Generic (dispose,					\
			void (*) (Element *):				\
			(gl_listelement_dispose_fn) dispose) : NULL,	\
       false);								\
    return (List) { gl_list };						\
  }									\
									\
  static inline void							\
  list##_free (List l)							\
  {									\
    gl_list_free (l.gl_list);						\
  }									\
									\
  static inline size_t							\
  list##_size (List l)							\
  {									\
    gl_list_size (l.gl_list);						\
  }									\
									\
  static inline Element *						\
  list##_first (List l)							\
  {									\
    return (Element *) gl_list_get_at (l.gl_list, 0);			\
  }									\
									\
  static inline void							\
  list##_set_first (List l, const Element *e)				\
  {									\
    gl_list_set_at (l.gl_list, 0, e);					\
  }									\
									\
  static inline Element *						\
  list##_at (List l, List##Position *p)					\
  {									\
    return (Element *) gl_list_node_value (l.gl_list, (gl_list_node_t) p); \
  }									\
									\
  static inline List##Position *					\
  list##_previous (List l, List##Position *p)				\
  {									\
    return (List##Position *)						\
      gl_list_previous_node (l.gl_list, (gl_list_node_t) p);		\
  }									\
									\
  static inline	List##Position *					\
  list##_add (List l, Element *e)					\
  {									\
    return (List##Position *)						\
      gl_list_add_last (l.gl_list, (const void *) e);			\
  }									\
									\
  static inline	List##Position *					\
  list##_add_first (List l, Element *e)					\
  {									\
    return (List##Position *)						\
      gl_list_add_first (l.gl_list, (const void *) e);			\
  }									\
									\
  static inline List##Position *					\
  list##_add_before (List l, List##Position *p, Element *e)		\
  {									\
    return (List##Position *)						\
      gl_list_add_before (l.gl_list, (gl_list_node_t) p,		\
			  (const void *) e);				\
  }									\
  									\
  static inline void							\
  list##_remove (List l, List##Position *p)				\
  {									\
    gl_list_remove_node (l.gl_list, (gl_list_node_t) p);		\
  }									\
  									\
  static inline void							\
  list##_replace (List l, List##Position *p, Element *e)		\
  {									\
    gl_list_node_set_value (l.gl_list, (gl_list_node_t) p, (const void *) e); \
  }									\
  									\
  static inline List##Position *					\
  list##_search (List l, const Element *e)				\
  {									\
    return (List##Position *) gl_list_search (l.gl_list, e);		\
  }									\
									\
  static inline List##Iterator						\
  list##_iterator (List l)						\
  {									\
    gl_list_iterator_t i = gl_list_iterator (l.gl_list);		\
    return (List##Iterator) { i };					\
  }									\
									\
  static inline bool							\
  list##_iterator_next (List##Iterator *i, Element **e,			\
			List##Position **p)				\
  {									\
    return gl_list_iterator_next (&i->gl_list_iterator, (const void **) e, \
				  (gl_list_node_t *) p);		\
  }									\
									\
  static inline void							\
  list##_iterator_free (List##Iterator *i)				\
  {									\
    gl_list_iterator_free (&i->gl_list_iterator);			\
  }

#define list_foreach(e, List, Element, list, l)				\
  for (bool list_break = false; !list_break; )				\
    for (List##Iterator i##__LINE__ = list##_iterator ((l));		\
	 !list_break;							\
	 list_break = true,						\
	   list##_iterator_free (&i##__LINE__))				\
      for (Element *(e); !list_break					\
	     && list##_iterator_next (&i##__LINE__, &(e), NULL); )

#define list_foreach_break			\
  do						\
    {						\
      list_break = true;			\
    }						\
  while (0)

#endif /* LIST_H_INCLUDED */
