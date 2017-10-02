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

#ifndef GRAPH_H_INCLUDED
#define GRAPH_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

#include "deque.h"

#define GRAPH__EXPAND(...)        __VA_ARGS__
#define GRAPH__FIRST(a, b, c, d)  a
#define GRAPH__SECOND(a, b, c, d) b
#define GRAPH__THIRD(a, b, c, d)  c
#define GRAPH__FOURTH(a, b, c, d) d
#define GRAPH__NODE_TYPE(GRAPH)  GRAPH__EXPAND (GRAPH__FIRST GRAPH)
#define GRAPH__NODE_FIELD(GRAPH) GRAPH__EXPAND (GRAPH__SECOND GRAPH)
#define GRAPH__EDGE_TYPE(GRAPH)  GRAPH__EXPAND (GRAPH__THIRD GRAPH)
#define GRAPH__EDGE_FIELD(GRAPH) GRAPH__EXPAND (GRAPH__FOURTH GRAPH)

#define GRAPH__NODE_OFFSET(GRAPH)		\
  (offsetof (GRAPH__NODE_TYPE (GRAPH), GRAPH__NODE_FIELD (GRAPH)))
#define GRAPH__EDGE_OFFSET(GRAPH)		\
  (offsetof (GRAPH__EDGE_TYPE (GRAPH), GRAPH__EDGE_FIELD (GRAPH)))
#define GRAPH__NODE_INFO(GRAPH, node)		\
  _Generic ((node),				\
	    GRAPH__NODE_TYPE(GRAPH) *: (&((node)->GRAPH__NODE_FIELD (GRAPH))))
#define GRAPH__EDGE_INFO(GRAPH, edge)		\
  _Generic ((edge),				\
	    GRAPH__EDGE_TYPE(GRAPH) *: (&((edge)->GRAPH__EDGE_FIELD (GRAPH))))
#define GRAPH__NODE(GRAPH, node)		\
  ((GRAPH__NODE_TYPE (GRAPH) *)				\
   graph__offset ((char *) (node), - GRAPH__NODE_OFFSET (GRAPH)))
#define GRAPH__EDGE(GRAPH, edge)				\
  ((GRAPH__EDGE_TYPE (GRAPH) *)					\
   graph__offset ((char *) (edge), - GRAPH__EDGE_OFFSET (EDGE)))

#define GRAPH(node, node_field, edge, edge_field)	\
  (node, node_field, edge, edge_field)
#define EDGE_SPLITTER_DECL(GRAPH, type)		\
  void (* type) (GRAPH__EDGE_TYPE (GRAPH) *,	\
		 GRAPH__NODE_TYPE (GRAPH) **,	\
		 GRAPH__EDGE_TYPE (GRAPH) **,	\
		 GRAPH__EDGE_TYPE (GRAPH) **,	\
		 void *)
#define EDGE_SPLITTER(GRAPH) EDGE_SPLITTER_DECL (GRAPH, )
#define DATAFLOW_DECL(GRAPH, type)			\
  bool (* type) (GRAPH__NODE_TYPE (GRAPH) *, void *)
#define DATAFLOW(GRAPH) DATAFLOW_DECL(GRAPH, )

struct node
{
  DequeEntry nodes;
  Deque outgoing_edges;
  Deque incoming_edges;
  struct node *loop_header;
  size_t number;
  size_t index;
  bool visited : 1;
};
typedef struct node Node;

struct edge
{
  DequeEntry outgoing_edges;
  DequeEntry incoming_edges;
  Node *source;
  Node *target;
};
typedef struct edge Edge;

struct graph
{
  size_t node_number;
  Deque nodes;
  size_t node_count;
  Node **postorder;
  ptrdiff_t *dominance;
};
typedef struct graph Graph;

typedef void (* EdgeSplitter) (Edge *, Node **, Edge **, Edge **, void *);
typedef bool (* Dataflow) (Node *, void *);

static inline char *
graph__offset (char *p, ptrdiff_t offset)
{
  if (p == NULL)
    return NULL;
  return p + offset;
}

struct dataflow__data
{
  bool (* flow) (char *, void *);
  void *data;
  size_t offset;
};

static inline bool
dataflow__flow (char *node, struct dataflow__data const *data)
{
  return data->flow (node - data->offset, data->data);
}

struct graph__edge_splitter_data
{
  void (* splitter) (char *, char **, char **, char **, void *);
  void *data;
  size_t node_offset;
  size_t edge_offset;
};

static inline void
graph__edge_splitter_split (char *edge, char **node, char **edge1, char **edge2,
			    struct graph__edge_splitter_data const *data)
{
  data->splitter (edge - data->edge_offset, node, edge1, edge2, data->data);
  *node += data->node_offset;
  *edge1 += data->edge_offset;
  *edge2 += data->edge_offset;
}

void graph__init (struct graph *);
void graph__destroy (struct graph *);
bool node__root (Node *);
void graph__add_node (Graph *, Node *);
Node *graph__next_node (Node *);
void graph__add_edge (Edge *, Node *, Node *);
void graph__remove_edge (Edge *);
Node *edge__source (Edge *);
Node *edge__target (Edge *);
bool edge__critical (Edge *);
Node *edge__node (Edge *);
void graph__split_critical_edges (Graph *, EdgeSplitter, void *);
void postorder__init (Graph *);
void dataflow__forward (Graph *, Dataflow, void *);
void graph__dominance_init (Graph *);
Node *graph__immediate_dominator (Graph *, Node *);
bool graph__back_edge (Graph *, Edge *);
bool graph__dominates (Graph *, Node *, Node *);
Node *node__loop_header (Node *);
void graph__dump (Graph *, char const *);

#define graph_init(GRAPH, graph)		\
  (graph__init (graph))
#define graph_destroy(GRAPH, graph)		\
  (graph__destroy (graph))
#define node_root(GRAPH, node)			\
  (node__root (GRAPH__NODE_INFO (GRAPH, node)))
#define graph_add_node(GRAPH, graph, node)	\
  (graph__add_node (graph, GRAPH__NODE_INFO (GRAPH, node)))
#define graph_next_node(GRAPH, node)		\
  (GRAPH__NODE (GRAPH, graph__next_node (GRAPH__NODE_INFO (GRAPH, node))))
#define graph_add_edge(GRAPH, edge, node1, node2)	\
  (graph__add_edge (GRAPH__EDGE_INFO (GRAPH, edge),	\
		    GRAPH__NODE_INFO (GRAPH, node1),	\
		    GRAPH__NODE_INFO (GRAPH, node2)))
#define graph_remove_edge(GRAPH, edge)		\
  (graph__remove_edge (GRAPH__EDGE_INFO (GRAPH, edge)))
#define edge_source(GRAPH, edge)		\
  GRAPH__NODE (GRAPH, edge__source (GRAPH__EDGE_INFO (GRAPH, edge)))
#define edge_target(GRAPH, edge)		\
  GRAPH__NODE (GRAPH, edge__target (GRAPH__EDGE_INFO (GRAPH, edge)))
#define edge_critical(GRAPH, edge)		\
  (edge__critical (GRAPH__EDGE_INFO (GRAPH, edge)))
#define edge_node(GRAPH, edge)			\
  GRAPH__NODE (GRAPH, edge__node (GRAPH__EDGE_INFO (GRAPH, edge)))
#define graph_split_critical_edges(GRAPH, graph, split, ctx)		\
  (graph__split_critical_edges						\
   (graph, (EdgeSplitter) graph__edge_splitter_split,			\
    &(struct graph__edge_splitter_data) {				\
     .splitter = ((void (*) (char *, char **, char **, char **, void *)) \
		  _Generic ((split), EDGE_SPLITTER (GRAPH): (split))),	\
       .data = (ctx),							\
       .node_offset = GRAPH__NODE_OFFSET (GRAPH),			\
       .edge_offset = GRAPH__EDGE_OFFSET (GRAPH) }))
#define postorder_init(GRAPH, graph)		\
  (postorder__init ((graph)))
#define dataflow_forward(GRAPH, graph, do_flow, ctx)			\
  (dataflow_forward							\
   ((graph), (DataFlow) dataflow__flow,					\
     &(struct dataflow__data) {						\
     .flow = (bool (*) (char *, void *))				\
       _Generic ((do_flow), DATAFLOW (GRAPH): (do_flow)),		\
       .data = (ctx),							\
       .offset = GRAPH__NODE_OFFSET (GRAPH) }))
#define graph_dominance_init(GRAPH, graph)	\
  (graph__dominance_init ((graph)))
#define graph_immediate_dominator(GRAPH, graph, node)	\
  GRAPH__NODE (GRAPH,							\
	       graph__immediate_dominator (graph,			\
					   GRAPH__NODE_INFO (GRAPH, node)))
#define graph_back_edge(GRAPH, graph, node)	\
  (graph__back_edge (graph, GRAPH__EDGE_INFO (GRAPH, node)))
#define graph_dominates(GRAPH, graph, node1, node2)		\
  (graph__dominates (graph,					\
		     GRAPH__NODE_INFO (GRAPH, node1),		\
		     GRAPH__NODE_INFO (GRAPH, node2)))
#define node_loop_header(GRAPH, node)		\
  GRAPH__NODE (GRAPH, node__loop_header (GRAPH__NODE_INFO (GRAPH, node)))
#define graph_dump(GRAPH, graph, path)		\
  (graph__dump (graph, path))

#define node_foreach(node, GRAPH, graph)			\
  deque_foreach (node,						\
		 DEQUE (GRAPH__NODE_TYPE (GRAPH),		\
			GRAPH__NODE_FIELD (GRAPH).nodes),	\
		 &(graph)->nodes)

#define edge_outgoing_foreach(edge, GRAPH, node)		\
  deque_foreach (edge,						\
		 DEQUE (GRAPH__EDGE_TYPE (GRAPH),			\
			GRAPH__EDGE_FIELD (GRAPH).outgoing_edges),	\
		 &(node)->GRAPH__NODE_FIELD (GRAPH).outgoing_edges)

#define edge_outgoing_foreach_safe(edge, GRAPH, node)		\
  deque_foreach_safe (edge,					\
		      DEQUE (GRAPH__EDGE_TYPE (GRAPH),		\
			     GRAPH__EDGE_FIELD (GRAPH).outgoing_edges),	\
		      &(node)->GRAPH__NODE_FIELD (GRAPH).outgoing_edges)

#define reverse_postorder_foreach(node, GRAPH, graph)			\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (GRAPH__NODE_TYPE (GRAPH) *(node); b##__LINE__; b##__LINE__ = false) \
      for (size_t i##__LINE__ = (graph)->node_count;			\
	   i##__LINE__  > 0						\
	     && ((node) = GRAPH__NODE (GRAPH,				\
				       (graph)->postorder[i##__LINE__ - 1])) \
	     && true;							\
	   i##__LINE__--)

#endif /* GRAPH_H_INCLUDED */
