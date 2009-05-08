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
#ifndef PRB_H
#define PRB_H 1


#include <stddef.h>
template <class T> class BinaryRBNode;
template <class T> class BinaryRBIterator;

#define TREE_OWNS_DATA   true
#define CALLER_OWNS_DATA false

/* Maximum PRB height. */
#ifndef PRB_MAX_HEIGHT
#define PRB_MAX_HEIGHT 48
#endif

/* Color of a red-black node. */
enum prb_colors
{
    PRB_BLACK,   /* Black. */
    PRB_RED      /* Red. */
};

/* Tree data structure. */
template <class T> class CS_DEPRECATED_TYPE_MSG("BinaryRBTree deprecated. Use csRedBlackTree instead.") BinaryRBTree
{
    BinaryRBNode<T> *prb_root;        /* Tree's root. */
    size_t prb_count;                  /* Number of items in tree. */
  public:

    BinaryRBTree();
    ~BinaryRBTree ();
    void Clear(void)
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
        prb_root = NULL;
        prb_count = 0;
    }
    T *Find (const T *);
    BinaryRBTree<T> *Copy ();
    T **Probe ( T *, bool owner = CALLER_OWNS_DATA);
    T *Insert (T *, bool owner = CALLER_OWNS_DATA);

    // Does NOT change ownership
    T *Replace ( T *);

    T *Delete (const T *);
    size_t Count() const {return prb_count;}

    friend class BinaryRBIterator<T>;
};

template <class T> static void
copy_error_recovery (BinaryRBNode<T> *q,
                     BinaryRBTree<T> *table);

/* A red-black tree with parent pointers node. */
template <class T> class BinaryRBNode
{
    BinaryRBNode<T> *prb_link[2];  /* Subtrees. */
    BinaryRBNode<T> *prb_parent;   /* Parent. */
    T *prb_data;                /* Pointer to data. */

    bool bOwner;

    unsigned char prb_color;       /* Color. */

    BinaryRBNode() {bOwner = CALLER_OWNS_DATA;}
    ~BinaryRBNode() {if(bOwner) delete prb_data;}

    friend class BinaryRBTree<T>;
    friend class BinaryRBIterator<T>;
    friend void copy_error_recovery < > (BinaryRBNode<T> *q,
                     BinaryRBTree<T> *table);
};


template <class T> class BinaryRBIterator
{
    /* PRB traverser structure. */

    BinaryRBTree<T> *prb_tree;        /* Tree being traversed. */
    BinaryRBNode<T> *prb_node;          /* Current node in tree. */

public:

    BinaryRBIterator (BinaryRBTree<T> *tree);
    /* Table functions. */
    /* Table traverser functions. */
    T *First ();
    T *Last ();
    T *Find (T *);
    T *Insert (T *, bool owner = CALLER_OWNS_DATA);
    T *Copy (const BinaryRBIterator<T> *);
    T *Next ();
    T *Prev ();
    T *Curr ();
    // Does NOT change ownership
    T *Replace (T *);

    T *operator++(void) { return Next(); };
    T *operator--(void) { return Prev(); };
};

#include "prb.cpp"
#endif /* prb.h */
#endif
