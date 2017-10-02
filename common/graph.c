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

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "graph.h"
#include "deque.h"
#include "vector.h"
#include "xalloc.h"

#define GRAPH__NODES          DEQUE (Node, nodes)
#define GRAPH__OUTGOING_EDGES DEQUE (Edge, outgoing_edges)
#define GRAPH__INCOMING_EDGES DEQUE (Edge, incoming_edges)

#define node__foreach(node, graph)		\
  deque_foreach (node, GRAPH__NODES, &(graph)->nodes)

#define edge__outgoing_foreach(edge, node)				\
  deque_foreach (edge, GRAPH__OUTGOING_EDGES, &(node)->outgoing_edges)

#define edge__incoming_foreach(edge, node)				\
  deque_foreach (edge, GRAPH__INCOMING_EDGES, &(node)->incoming_edges)

#define edge__outgoing_foreach_safe(edge, node)				\
  deque_foreach_safe (edge, GRAPH__OUTGOING_EDGES, &(node)->outgoing_edges)

#define reverse__postorder_foreach(node, graph)				\
  for (bool b##__LINE__ = true; b##__LINE__; )				\
    for (Node *(node); b##__LINE__; b##__LINE__ = false)		\
      for (size_t i##__LINE__ = (graph)->node_count;			\
	   i##__LINE__  > 0						\
	     && ((node) = (graph)->postorder[i##__LINE__ - 1])		\
	     && true;							\
	   i##__LINE__--)

typedef Node *NodePtr;

void
graph__init (Graph *graph)
{
  deque_init (GRAPH__NODES, &graph->nodes);
  graph->postorder = NULL;
  graph->dominance = NULL;
  graph->node_number = 0;
}

void
graph__destroy (Graph *graph)
{
  free (graph->postorder);
  free (graph->dominance);
}

bool
node__root (Node *node)
{
  return deque_empty (GRAPH__INCOMING_EDGES, &node->incoming_edges);
}

void
graph__add_node (Graph *graph, Node *node)
{
  deque_init (GRAPH__OUTGOING_EDGES, &node->outgoing_edges);
  deque_init (GRAPH__INCOMING_EDGES, &node->incoming_edges);
  node->number = graph->node_number++;
  deque_insert_last (GRAPH__NODES, &graph->nodes, node);
}

Node *
graph__next_node (Node *node)
{
  return deque_next (GRAPH__NODES, node);
}

void
graph__add_edge (Edge *edge, Node *source, Node *target)
{
  edge->source = source;
  edge->target = target;
  deque_insert_last (GRAPH__OUTGOING_EDGES, &source->outgoing_edges, edge);
  deque_insert_last (GRAPH__INCOMING_EDGES, &target->incoming_edges, edge);
}

void
graph__remove_edge (Edge *edge)
{
  deque_remove (GRAPH__OUTGOING_EDGES, &edge->source->outgoing_edges, edge);
  deque_remove (GRAPH__INCOMING_EDGES, &edge->target->incoming_edges, edge);
}

Node *
edge__source (Edge *edge)
{
  return edge->source;
}

Node *
edge__target (Edge *edge)
{
  return edge->target;
}

Node *
edge__node (Edge *edge)
{
  return deque_next (GRAPH__INCOMING_EDGES,
		     deque_first (GRAPH__INCOMING_EDGES,
				  &edge->target->incoming_edges)) != NULL
    ? edge->source
    : edge->target;
}

bool
edge__critical (Edge *edge)
{
  return deque_next (GRAPH__OUTGOING_EDGES,
		     deque_first (GRAPH__OUTGOING_EDGES,
				  &edge->source->outgoing_edges)) != NULL
    && deque_next (GRAPH__INCOMING_EDGES,
		   deque_first (GRAPH__INCOMING_EDGES,
				&edge->target->incoming_edges)) != NULL;
}

void
graph__split_critical_edges (Graph *graph, EdgeSplitter splitter,
			     void *data)
{
  node__foreach (node, graph)
    {
      edge__outgoing_foreach_safe (edge, node)
	if (edge__critical (edge))
	  {
	    Edge *e1, *e2;
	    Node *n, *s, *t;
	    s = edge->source;
	    t = edge->target;
	    graph__remove_edge (edge);
	    splitter (edge, &n, &e1, &e2, data);
	    graph__add_node (graph, n);
	    graph__add_edge (e1, s, n);
	    graph__add_edge (e2, n, t);
	  }
    }
}

void
postorder__init (Graph *graph)
{
  free (graph->postorder);
  graph->node_count = 0;
  node__foreach (node, graph)
    {
      ++graph->node_count;
      node->loop_header = NULL;
      node->visited = false;
    }
  VECTOR(NodePtr) worklist;
  graph->postorder = XNMALLOC (graph->node_count, NodePtr);
  size_t index = 0;
  vector_init (&worklist);
  node__foreach (node, graph)
    if (node__root (node))
      vector_push (&worklist, node);
  while (!vector_empty (&worklist))
    {
      NodePtr node = vector_top (&worklist);
      if (node->visited)
	{
	  graph->postorder[index] = node;
	  node->index = index++;
	  vector_pop (&worklist);
	}
      else
	{
	  node->visited = true;
	  edge__outgoing_foreach (edge, node)
	    {
	      NodePtr successor = edge__target (edge);
	      if (!successor->visited)
		vector_push (&worklist, successor);
	    }
	}
    }
  vector_destroy (&worklist);
}

void
dataflow__forward (Graph *graph, Dataflow flow, void *data)
{
  VECTOR(NodePtr) old_worklist, new_worklist, tmp_worklist;
  vector_init (&old_worklist);
  vector_init (&new_worklist);
  reverse__postorder_foreach (node, graph)
    vector_push (&new_worklist, node);
  while (!vector_empty (&new_worklist))
    {
      tmp_worklist = old_worklist;
      old_worklist = new_worklist;
      new_worklist = tmp_worklist;
      NodePtr *node;
      VECTOR_FOREACH (&old_worklist, node)
	if (flow (*node, data))
	  vector_push (&new_worklist, *node);
      vector_clear (&old_worklist);
    }
  vector_destroy (&old_worklist);
  vector_destroy (&new_worklist);
}

static bool
dominance_flow (Node *node, Graph *graph)
{
  if (node__root (node))
    {
      graph->dominance[node->index] = PTRDIFF_MAX;
      return false;
    }
  Edge *edge = deque_first (GRAPH__INCOMING_EDGES, &node->incoming_edges);
  while (graph->dominance[edge->source->index] == -1)
    edge = deque_next (GRAPH__INCOMING_EDGES, edge);

  size_t new_idom = edge->source->index;
  while ((edge = deque_next (GRAPH__INCOMING_EDGES, edge)) != NULL)
    {
      Node *pred = edge->source;
      if (graph->dominance[pred->index] == -1)
	continue;
      size_t finger = pred->index;
      while (new_idom != finger)
	{
	  while (new_idom < finger)
	    new_idom = graph->dominance[new_idom];
	  while (finger < new_idom)
	    finger = graph->dominance[finger];
	}
    }
  if (graph->dominance[node->index] == new_idom)
    return false;
  graph->dominance[node->index] = new_idom;
  return true;
}

static void
mark_loop (Graph *graph, Edge *edge)
{
  node__foreach (node, graph)
    node->visited = false;
  Node *header = edge->target;
  VECTOR(NodePtr) worklist;
  vector_init (&worklist);
  vector_push (&worklist, edge->source);
  while (!vector_empty (&worklist))
    {
      Node *node = vector_pop (&worklist);
      node->visited = true;
      node->loop_header = header;
      if (node == header)
	continue;
      edge__incoming_foreach (edge, node)
	{
	  Node *pred = edge->source;
	  if (!pred->visited)
	    vector_push (&worklist, pred);
	}
    }
  vector_destroy (&worklist);
}

static void
loop_init (Graph *graph)
{
  reverse__postorder_foreach (node, graph)
    {
      edge__incoming_foreach (edge, node)
	if (graph__back_edge (graph, edge))
	  mark_loop (graph, edge);

    }
}

void
graph__dominance_init (Graph *graph)
{
  free (graph->dominance);
  graph->dominance = XNMALLOC (graph->node_count, size_t);
  for (size_t i = 0; i < graph->node_count; ++i)
    graph->dominance[i] = -1;
  dataflow__forward (graph, (Dataflow) dominance_flow, graph);
  loop_init (graph);
}

Node *
graph__immediate_dominator (Graph *graph, Node *node)
{
  size_t idom = graph->dominance[node->index];
  return idom == PTRDIFF_MAX ? NULL : graph->postorder[idom];
}

bool
graph__back_edge (Graph *graph, Edge *edge)
{
  return graph__dominates (graph, edge->target, edge->source);
}

bool
graph__dominates (Graph *graph, Node *node1, Node *node2)
{
  size_t index1 = node1->index;
  size_t index2 = node2->index;
  while (index2 < index1)
    index2 = graph->dominance[index2];
  return index1 == index2;
}

Node *
node__loop_header (Node *node)
{
  return node->loop_header;
}

void
graph__dump (Graph *graph, char const *path)
{
  FILE *out = fopen (path, "w");
  if (out == NULL)
    error (EXIT_FAILURE, errno, "%s", path);

  fprintf (out, "digraph G {\n");
  node__foreach (node, graph)
    {
      fprintf (out, "  node%zd;\n", node->number);
      edge__outgoing_foreach (edge, node)
	fprintf (out, "  node%zd -> node%zd;\n",
		 edge->source->number,
		 edge->target->number);
    }
  fprintf (out, "}\n");

  fclose (out);
}
