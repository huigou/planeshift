/* Produced by texiweb from libavl.w on 2002/08/24 at 13:21. */

/* libavl - library for manipulation of binary trees.
   Copyright (C) 1998-2002 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
   See the GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA.

   The author may be contacted at <blp@gnu.org> on the Internet, or
   write to Ben Pfaff, Stanford University, Computer Science Dept., 353
   Serra Mall, Stanford CA 94305, USA.
*/

// Edited by the Atomic Blue on 2004/11
// This is deprecated in favour of csRedBlackTree
#if 0
#ifdef PRB_H
#ifndef __PRB_CPP__
#define __PRB_CPP__

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "prb.h"

template <class T>
BinaryRBTree<T>::BinaryRBTree()
{
  prb_root = NULL;
  prb_count = 0;
}

/* Search |tree| for an item matching |item|, and return it if found.
   Otherwise return |NULL|. */
template <class T> T *BinaryRBTree<T>::Find (const T *item)
{
  const BinaryRBNode<T> *p;

  assert (item != NULL);
  for (p = prb_root; p != NULL; )
    {
      if (*item == *p->prb_data)
        return p->prb_data;
      if (*item < *(p->prb_data))
        p = p->prb_link[0];
      else
        p = p->prb_link[1];
    }

  return NULL;
}

/* Inserts |item| into |tree| and returns a pointer to |item|'s address.
   If a duplicate item is found in the tree,
   returns a pointer to the duplicate without inserting |item|.
   Returns |NULL| in case of memory allocation failure. */
template <class T> T **BinaryRBTree<T>::Probe (T *item, bool owner)
{
  BinaryRBNode<T> *p; /* Traverses tree looking for insertion point. */
  BinaryRBNode<T> *q; /* Parent of |p|; node at which we are rebalancing. */
  BinaryRBNode<T> *n; /* Newly inserted node. */
  int dir = 0;        /* Side of |q| on which |n| is inserted. */

  assert (item != NULL);

  for (q = NULL, p = prb_root; p != NULL; q = p, p = p->prb_link[dir])
    {
      if (*item == *p->prb_data)
        return &p->prb_data;
      dir = !(*item < *p->prb_data);
    }

    n = new BinaryRBNode<T>;
  if (n == NULL)
    return NULL;

  prb_count++;
  n->prb_link[0] = n->prb_link[1] = NULL;
  n->prb_parent = q;
  n->prb_data = item;
  n->bOwner = owner;

  if (q != NULL)
    q->prb_link[dir] = n;
  else
    prb_root = n;
  n->prb_color = PRB_RED;

  q = n;
  for (;;)
    {
      BinaryRBNode<T> *f; /* Parent of |q|. */
      BinaryRBNode<T> *g; /* Grandparent of |q|. */

      f = q->prb_parent;
      if (f == NULL || f->prb_color == PRB_BLACK)
        break;

      g = f->prb_parent;
      if (g == NULL)
        break;

      if (g->prb_link[0] == f)
        {
          BinaryRBNode<T> *y = g->prb_link[1];
          if (y != NULL && y->prb_color == PRB_RED)
            {
              f->prb_color = y->prb_color = PRB_BLACK;
              g->prb_color = PRB_RED;
              q = g;
            }
          else
            {
              BinaryRBNode<T> *h; /* Great-grandparent of |q|. */

              h = g->prb_parent;
              if (h == NULL)
                h = (BinaryRBNode<T> *) &prb_root;

              if (f->prb_link[1] == q)
                {
                  f->prb_link[1] = q->prb_link[0];
                  q->prb_link[0] = f;
                  g->prb_link[0] = q;
                  f->prb_parent = q;
                  if (f->prb_link[1] != NULL)
                    f->prb_link[1]->prb_parent = f;

                  f = q;
                }

              g->prb_color = PRB_RED;
              f->prb_color = PRB_BLACK;

              g->prb_link[0] = f->prb_link[1];
              f->prb_link[1] = g;
              h->prb_link[h->prb_link[0] != g] = f;

              f->prb_parent = g->prb_parent;
              g->prb_parent = f;
              if (g->prb_link[0] != NULL)
                g->prb_link[0]->prb_parent = g;
              break;
            }
        }
      else
        {
          BinaryRBNode<T> *y = g->prb_link[0];
          if (y != NULL && y->prb_color == PRB_RED)
            {
              f->prb_color = y->prb_color = PRB_BLACK;
              g->prb_color = PRB_RED;
              q = g;
            }
          else
            {
              BinaryRBNode<T> *h; /* Great-grandparent of |q|. */

              h = g->prb_parent;
              if (h == NULL)
                h = (BinaryRBNode<T> *) &prb_root;

              if (f->prb_link[0] == q)
                {
                  f->prb_link[0] = q->prb_link[1];
                  q->prb_link[1] = f;
                  g->prb_link[1] = q;
                  f->prb_parent = q;
                  if (f->prb_link[0] != NULL)
                    f->prb_link[0]->prb_parent = f;

                  f = q;
                }

              g->prb_color = PRB_RED;
              f->prb_color = PRB_BLACK;

              g->prb_link[1] = f->prb_link[0];
              f->prb_link[0] = g;
              h->prb_link[h->prb_link[0] != g] = f;

              f->prb_parent = g->prb_parent;
              g->prb_parent = f;
              if (g->prb_link[1] != NULL)
                g->prb_link[1]->prb_parent = g;
              break;
            }
        }
    }
  prb_root->prb_color = PRB_BLACK;

  return &n->prb_data;
}

/* Inserts |item| into |table|.
   Returns |NULL| if |item| was successfully inserted
   or if a memory allocation error occurred.
   Otherwise, returns the duplicate item. */
template <class T> T *BinaryRBTree<T>::Insert (T *item, bool owner)
{
  T **p = Probe (item, owner);
  return p == NULL || *p == item ? NULL : *p;
}

/* Inserts |item| into |table|, replacing any duplicate item.
   Returns |NULL| if |item| was inserted without replacing a duplicate,
   or if a memory allocation error occurred.
   Otherwise, returns the item that was replaced. */
template <class T> T *BinaryRBTree<T>::Replace (T *item)
{
  T **p = Probe (item);
  if (p == NULL || *p == item)
    return NULL;
  else
    {
      T *r = *p;
      *p = item;
      return r;
    }
}

/* Deletes from |tree| and returns an item matching |item|. Note the returned |item| may have been deleted.
   Returns a null pointer if no matching item found. */
template <class T> T *BinaryRBTree<T>::Delete (const T *item)
{
  BinaryRBNode<T> *p; /* Node to delete. */
  BinaryRBNode<T> *q; /* Parent of |p|. */
  BinaryRBNode<T> *f; /* Node at which we are rebalancing. */
  int dir = 0;        /* Side of |q| on which |p| is a child;
                         side of |f| from which node was deleted. */

  assert (item != NULL);

  if (prb_root == NULL)
    return NULL;

  p = prb_root;
  for (;;)
    {
      if (*item == *p->prb_data)
        break;

      dir = !(*item < *p->prb_data);
      p = p->prb_link[dir];
      if (p == NULL)
        return NULL;
    }
  item = p->prb_data;

  q = p->prb_parent;
  if (q == NULL)
    {
      q = (BinaryRBNode<T> *) &prb_root;
      dir = 0;
    }

  if (p->prb_link[1] == NULL)
    {
      q->prb_link[dir] = p->prb_link[0];
      if (q->prb_link[dir] != NULL)
        q->prb_link[dir]->prb_parent = p->prb_parent;

      f = q;
    }
  else
    {
      unsigned char t;
      BinaryRBNode<T> *r = p->prb_link[1];

      if (r->prb_link[0] == NULL)
        {
          r->prb_link[0] = p->prb_link[0];
          q->prb_link[dir] = r;
          r->prb_parent = p->prb_parent;
          if (r->prb_link[0] != NULL)
            r->prb_link[0]->prb_parent = r;

          t = p->prb_color;
          p->prb_color = r->prb_color;
          r->prb_color = t;

          f = r;
          dir = 1;
        }
      else
        {
          BinaryRBNode<T> *s = r->prb_link[0];
          while (s->prb_link[0] != NULL)
            s = s->prb_link[0];
          r = s->prb_parent;
          r->prb_link[0] = s->prb_link[1];
          s->prb_link[0] = p->prb_link[0];
          s->prb_link[1] = p->prb_link[1];
          q->prb_link[dir] = s;
          if (s->prb_link[0] != NULL)
            s->prb_link[0]->prb_parent = s;
          s->prb_link[1]->prb_parent = s;
          s->prb_parent = p->prb_parent;
          if (r->prb_link[0] != NULL)
            r->prb_link[0]->prb_parent = r;

          t = p->prb_color;
          p->prb_color = s->prb_color;
          s->prb_color = t;

          f = r;
          dir = 0;
        }
    }

  if (p->prb_color == PRB_BLACK)
    {
      for (;;)
        {
          BinaryRBNode<T> *x; /* Node we want to recolor black if possible. */
          BinaryRBNode<T> *g; /* Parent of |f|. */
          BinaryRBNode<T> *t; /* Temporary for use in finding parent. */

          x = f->prb_link[dir];
          if (x != NULL && x->prb_color == PRB_RED)
            {
              x->prb_color = PRB_BLACK;
              break;
            }

          if (f == (BinaryRBNode<T> *) &prb_root)
            break;

          g = f->prb_parent;
          if (g == NULL)
            g = (BinaryRBNode<T> *) &prb_root;

          if (dir == 0)
            {
              BinaryRBNode<T> *w = f->prb_link[1];

              if (w->prb_color == PRB_RED)
                {
                  w->prb_color = PRB_BLACK;
                  f->prb_color = PRB_RED;

                  f->prb_link[1] = w->prb_link[0];
                  w->prb_link[0] = f;
                  g->prb_link[g->prb_link[0] != f] = w;

                  w->prb_parent = f->prb_parent;
                  f->prb_parent = w;

                  g = w;
                  w = f->prb_link[1];

                  w->prb_parent = f;
                }

              if ((w->prb_link[0] == NULL
                   || w->prb_link[0]->prb_color == PRB_BLACK)
                  && (w->prb_link[1] == NULL
                      || w->prb_link[1]->prb_color == PRB_BLACK))
                {
                  w->prb_color = PRB_RED;
                }
              else
                {
                  if (w->prb_link[1] == NULL
                      || w->prb_link[1]->prb_color == PRB_BLACK)
                    {
                      BinaryRBNode<T> *y = w->prb_link[0];
                      y->prb_color = PRB_BLACK;
                      w->prb_color = PRB_RED;
                      w->prb_link[0] = y->prb_link[1];
                      y->prb_link[1] = w;
                      if (w->prb_link[0] != NULL)
                        w->prb_link[0]->prb_parent = w;
                      w = f->prb_link[1] = y;
                      w->prb_link[1]->prb_parent = w;
                    }

                  w->prb_color = f->prb_color;
                  f->prb_color = PRB_BLACK;
                  w->prb_link[1]->prb_color = PRB_BLACK;

                  f->prb_link[1] = w->prb_link[0];
                  w->prb_link[0] = f;
                  g->prb_link[g->prb_link[0] != f] = w;

                  w->prb_parent = f->prb_parent;
                  f->prb_parent = w;
                  if (f->prb_link[1] != NULL)
                    f->prb_link[1]->prb_parent = f;
                  break;
                }
            }
          else
            {
              BinaryRBNode<T> *w = f->prb_link[0];

              if (w->prb_color == PRB_RED)
                {
                  w->prb_color = PRB_BLACK;
                  f->prb_color = PRB_RED;

                  f->prb_link[0] = w->prb_link[1];
                  w->prb_link[1] = f;
                  g->prb_link[g->prb_link[0] != f] = w;

                  w->prb_parent = f->prb_parent;
                  f->prb_parent = w;

                  g = w;
                  w = f->prb_link[0];

                  w->prb_parent = f;
                }

              if ((w->prb_link[0] == NULL
                   || w->prb_link[0]->prb_color == PRB_BLACK)
                  && (w->prb_link[1] == NULL
                      || w->prb_link[1]->prb_color == PRB_BLACK))
                {
                  w->prb_color = PRB_RED;
                }
              else
                {
                  if (w->prb_link[0] == NULL
                      || w->prb_link[0]->prb_color == PRB_BLACK)
                    {
                      BinaryRBNode<T> *y = w->prb_link[1];
                      y->prb_color = PRB_BLACK;
                      w->prb_color = PRB_RED;
                      w->prb_link[1] = y->prb_link[0];
                      y->prb_link[0] = w;
                      if (w->prb_link[1] != NULL)
                        w->prb_link[1]->prb_parent = w;
                      w = f->prb_link[0] = y;
                      w->prb_link[0]->prb_parent = w;
                    }

                  w->prb_color = f->prb_color;
                  f->prb_color = PRB_BLACK;
                  w->prb_link[0]->prb_color = PRB_BLACK;

                  f->prb_link[0] = w->prb_link[1];
                  w->prb_link[1] = f;
                  g->prb_link[g->prb_link[0] != f] = w;

                  w->prb_parent = f->prb_parent;
                  f->prb_parent = w;
                  if (f->prb_link[0] != NULL)
                    f->prb_link[0]->prb_parent = f;
                  break;
                }
            }

          t = f;
          f = f->prb_parent;
          if (f == NULL)
            f = (BinaryRBNode<T> *) &prb_root;
          dir = f->prb_link[0] != t;
        }
    }

    delete p;
    p = NULL;
  prb_count--;
  return (T *) item;
}

/* Initializes |trav| for use with |tree|
   and selects the null node. */
template <class T> BinaryRBIterator<T> ::BinaryRBIterator (BinaryRBTree<T> *tree)
{
  prb_tree = tree;
  prb_node = NULL;
}

/* Initializes |trav| for |tree|.
   Returns data item in |tree| with the least value,
   or |NULL| if |tree| is empty. */
template <class T> T* BinaryRBIterator<T>::First ()
{
    prb_node = prb_tree->prb_root;

    if (prb_node != NULL)
    {
      while (prb_node->prb_link[0] != NULL)
        prb_node = prb_node->prb_link[0];
      return prb_node->prb_data;
    }
  else
    return NULL;
}

/* Initializes |trav| for |tree|.
   Returns data item in |tree| with the greatest value,
   or |NULL| if |tree| is empty. */
template <class T> T* BinaryRBIterator<T>::Last ()
{
  prb_node = prb_tree->prb_root;
  if (prb_node != NULL)
    {
      while (prb_node->prb_link[1] != NULL)
        prb_node = prb_node->prb_link[1];
      return prb_node->prb_data;
    }
  else
    return NULL;
}

/* Searches for |item| in |tree|.
   If found, initializes |trav| to the item found and returns the item
   as well.
   If there is no matching item, initializes |trav| to the null item
   and returns |NULL|. */
template <class T> T* BinaryRBIterator<T>::Find (T *item)
{
  BinaryRBNode<T> *p;
  int dir;

  assert (item != NULL);

  for (p = prb_tree->prb_root; p != NULL; p = p->prb_link[dir])
    {
      if (*item == *p->prb_data)
        {
          prb_node = p;
          return p->prb_data;
        }

      dir = !(*item < *p->prb_data);
    }

  prb_node = NULL;
  return NULL;
}

/* Attempts to insert |item| into |tree|.
   If |item| is inserted successfully, it is returned and |trav| is
   initialized to its location.
   If a duplicate is found, it is returned and |trav| is initialized to
   its location.  No replacement of the item occurs.
   If a memory allocation failure occurs, |NULL| is returned and |trav|
   is initialized to the null item. */
template <class T> T* BinaryRBIterator<T>::Insert (T *item, bool owner)
{
  T **p;

  assert (item != NULL);

  p = prb_tree->Probe (item, owner);
  if (p != NULL)
    {
        // Slow alternative
        BinaryRBNode<T> *q;
        int dir;

        for (q = prb_tree->prb_root; q != NULL; q = q->prb_link[dir])
        {
            if (*item == *q->prb_data)
            {
                prb_node = q;
                return *p;
            }

            dir = !(*item < *q->prb_data);
        }

      //prb_node =
      //  ((BinaryRBNode<T> *)
      //   ((char *) p - offsetof (BinaryRBNode<T>, prb_data)));
      return *p;
    }
  else
    {
      prb_node = NULL;
      return NULL;
    }
}

/* Initializes |trav| to have the same current node as |src|. */
template <class T> T* BinaryRBIterator<T>::Copy( const BinaryRBIterator<T> *src)
{
  assert (src != NULL);

  prb_tree = src->prb_tree;
  prb_node = src->prb_node;

  return prb_node != NULL ? prb_node->prb_data : NULL;
}

/* Returns the next data item in inorder
   within the tree being traversed with |trav|,
   or if there are no more data items returns |NULL|. */
template <class T> T* BinaryRBIterator<T>::Next ()
{
  if (prb_node == NULL)
    return First ();
  else if (prb_node->prb_link[1] == NULL)
    {
      BinaryRBNode<T> *q, *p; /* Current node and its child. */
      for (p = prb_node, q = p->prb_parent; ;
           p = q, q = q->prb_parent)
        if (q == NULL || p == q->prb_link[0])
          {
            prb_node = q;
            return prb_node != NULL ? prb_node->prb_data : NULL;
          }
    }
  else
    {
      prb_node = prb_node->prb_link[1];
      while (prb_node->prb_link[0] != NULL)
        prb_node = prb_node->prb_link[0];
      return prb_node->prb_data;
    }
}

/* Returns the previous data item in inorder
   within the tree being traversed with |trav|,
   or if there are no more data items returns |NULL|. */
template <class T> T* BinaryRBIterator<T>::Prev ()
{
  if (prb_node == NULL)
    return Last ();
  else if (prb_node->prb_link[0] == NULL)
    {
      BinaryRBNode<T> *q, *p; /* Current node and its child. */
      for (p = prb_node, q = p->prb_parent; ;
           p = q, q = q->prb_parent)
        if (q == NULL || p == q->prb_link[1])
          {
            prb_node = q;
            return prb_node != NULL ? prb_node->prb_data : NULL;
          }
    }
  else
    {
      prb_node = prb_node->prb_link[0];
      while (prb_node->prb_link[1] != NULL)
        prb_node = prb_node->prb_link[1];
      return prb_node->prb_data;
    }
}

/* Returns |trav|'s current item. */
template <class T> T* BinaryRBIterator<T>::Curr ()
{
  return prb_node != NULL ? prb_node->prb_data : NULL;
}

/* Replaces the current item in |trav| by |new| and returns the item replaced.
   |trav| must not have the null item selected.
   The new item must not upset the ordering of the tree. */
template <class T> T* BinaryRBIterator<T>::Replace (T *new_obj)
{
  T *old;

  assert (prb_node != NULL && new_obj != NULL);
  old = prb_node->prb_data;
  prb_node->prb_data = new_obj;
  return old;
}

template <class T> static void
copy_error_recovery (BinaryRBNode<T> *q,
                     BinaryRBTree<T> *table)
{
  assert (q != NULL && table != NULL);

  for (;;)
    {
      BinaryRBNode<T> *p = q;
      q = q->prb_parent;
      if (q == NULL)
        break;

      if (p == q->prb_link[0])
        q->prb_link[1] = NULL;
    }

  delete table;
}

/* Copies |org| to a newly created tree, which is returned.
   If |copy != NULL|, each data item in |org| is first passed to |copy|,
   and the return values are inserted into the tree;
   |NULL| return values are taken as indications of failure.
   On failure, destroys the partially created new tree,
   applying |destroy|, if non-null, to each item in the new tree so far,
   and returns |NULL|.
   If |allocator != NULL|, it is used for allocation in the new tree.
   Otherwise, the same allocator used for |org| is used. */
template <class T> BinaryRBTree<T> *BinaryRBTree<T>::Copy ()
{
  BinaryRBTree<T> *newtable;
  const BinaryRBNode<T> *x;
  BinaryRBNode<T> *y;

  newtable = new BinaryRBTree<T> ();
  if (newtable == NULL)
    return NULL;
  newtable->prb_count = prb_count;
  if (newtable->prb_count == 0)
    return newtable;

  x = (const BinaryRBNode<T> *) &prb_root;
  y = (BinaryRBNode<T> *) &newtable->prb_root;
  for (;;)
    {
      while (x->prb_link[0] != NULL)
        {
          y->prb_link[0] = new BinaryRBNode<T>;
          if (y->prb_link[0] == NULL)
            {
              if (y != (BinaryRBNode<T> *) &newtable->prb_root)
                {
                  y->prb_data = NULL;
                  y->prb_link[1] = NULL;
                }

              copy_error_recovery (y, newtable);
              return NULL;
            }
          y->prb_link[0]->prb_parent = y;

          x = x->prb_link[0];
          y = y->prb_link[0];
        }
      y->prb_link[0] = NULL;

      for (;;)
        {
          y->prb_color = x->prb_color;
            y->prb_data = x->prb_data;
            y->bOwner = x->bOwner;

          if (x->prb_link[1] != NULL)
            {
              y->prb_link[1] = new BinaryRBNode<T>;
              if (y->prb_link[1] == NULL)
                {
                  copy_error_recovery (y, newtable);
                  return NULL;
                }
              y->prb_link[1]->prb_parent = y;

              x = x->prb_link[1];
              y = y->prb_link[1];
              break;
            }
          else
            y->prb_link[1] = NULL;

          for (;;)
            {
              const BinaryRBNode<T> *w = x;
              x = x->prb_parent;
              if (x == NULL)
                {
                  newtable->prb_root->prb_parent = NULL;
                  return newtable;
                }
              y = y->prb_parent;

              if (w == x->prb_link[0])
                break;
            }
        }
    }
}

/* Frees storage allocated for |tree|.
   If |destroy != NULL|, applies it to each data item in inorder. */
template <class T> BinaryRBTree<T>::~BinaryRBTree ()
{
  BinaryRBNode<T> *p, *q;

  for (p = prb_root; p != NULL; p = q)
    if (p->prb_link[0] == NULL)
      {
        q = p->prb_link[1];
        delete p;
        p = NULL;
      }
    else
      {
        q = p->prb_link[0];
        p->prb_link[0] = q->prb_link[1];
        q->prb_link[1] = p;
      }
}

#endif
#endif


#ifdef TESTBINARYRBTREE
#ifndef PRB_H
#include "prb.h"

class TestBinaryRBTreeElement
{
public:
    int id;
    TestBinaryRBTreeElement(int num)
    {
        id = num;
    };
    
    ~TestBinaryRBTreeElement()
    {
        printf("Free node %d\n",id);
    };

    bool operator==(TestBinaryRBTreeElement& other) const
    {
        return id==other.id;
    };

    bool operator<(TestBinaryRBTreeElement& other) const
    {
        return id<other.id;
    };
    
};



int main()    
{
    BinaryRBTree<TestBinaryRBTreeElement> tree;

    tree.Insert(new TestBinaryRBTreeElement(3), true);
    tree.Insert(new TestBinaryRBTreeElement(2), true);
    tree.Insert(new TestBinaryRBTreeElement(1), true);
    tree.Insert(new TestBinaryRBTreeElement(4), true);
    tree.Insert(new TestBinaryRBTreeElement(5), true);
}
// End of test
#endif
#endif 
#endif
