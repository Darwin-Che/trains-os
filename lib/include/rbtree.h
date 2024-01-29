/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
  Partly COPIED FROM
  linux/include/linux/rbtree.h
  linux/include/linux/rbtree_type.h
*/
#ifndef C_RBTREE_H
#define C_RBTREE_H

#include <stddef.h>
#include <stdint.h>

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"

#define container_of(ptr, type, member)                \
  __extension__({                                      \
    const typeof(((type *)0)->member)                  \
        *__mptr = (ptr);                               \
    (type *)((char *)__mptr - offsetof(type, member)); \
  })

#pragma GCC diagnostic pop

// #ifdef __always_inline
  #undef __always_inline
  #define __always_inline inline __attribute__((always_inline))
// #endif

struct rb_node
{
  unsigned long __rb_parent_color;
  struct rb_node *rb_right;
  struct rb_node *rb_left;
} __attribute__((aligned(sizeof(long))));

struct rb_root
{
  struct rb_node *rb_node;
};

#define RB_ROOT \
  (struct rb_root) { NULL, }

#define rb_parent(r) ((struct rb_node *)((r)->__rb_parent_color & ~3))

#define rb_entry(ptr, type, member) container_of(ptr, type, member)

#define RB_EMPTY_ROOT(root) ((root)->rb_node == NULL)

/* 'empty' nodes are nodes that are known not to be inserted in an rbtree */
#define RB_EMPTY_NODE(node) ((node)->__rb_parent_color == (unsigned long)(node))
#define RB_CLEAR_NODE(node) ((node)->__rb_parent_color = (unsigned long)(node))

extern void rb_insert_color(struct rb_node *, struct rb_root *);
extern void rb_erase(struct rb_node *, struct rb_root *);

/* Find logical next and previous nodes in a tree */
extern struct rb_node *rb_next(const struct rb_node *);
extern struct rb_node *rb_prev(const struct rb_node *);
extern struct rb_node *rb_first(const struct rb_root *);
extern struct rb_node *rb_last(const struct rb_root *);

/* Postorder iteration - always visit the parent after its children */
// extern struct rb_node *rb_first_postorder(const struct rb_root *);
// extern struct rb_node *rb_next_postorder(const struct rb_node *);

/* Fast replacement of a single node without remove/rebalance/add/rebalance */
extern void rb_replace_node(struct rb_node *victim, struct rb_node *new,
                            struct rb_root *root);

static inline void rb_link_node(struct rb_node *node, struct rb_node *parent,
                                struct rb_node **rb_link)
{
  node->__rb_parent_color = (unsigned long)parent;
  node->rb_left = node->rb_right = NULL;

  *rb_link = node;
}

/**
 * rb_add() - insert @node into @tree
 * @node: node to insert
 * @tree: tree to insert @node into
 * @less: operator defining the (partial) node order
 */
static __always_inline void rb_add(struct rb_node *node, struct rb_root *tree,
                                   int (*cmp)(struct rb_node *,
                                              const struct rb_node *))
{
  struct rb_node **link = &tree->rb_node;
  struct rb_node *parent = NULL;
  while (*link)
  {
    parent = *link;
    int c = cmp(node, parent);

    if (c < 0)
      link = &parent->rb_left;
    else
      link = &parent->rb_right;
  }

  rb_link_node(node, parent, link);
  rb_insert_color(node, tree);
}

/**
 * rb_find_add() - find equivalent @node in @tree, or add @node
 * @node: node to look-for / insert
 * @tree: tree to search / modify
 * @cmp: operator defining the node order
 *
 * Returns the rb_node matching @node, or NULL when no match is found and @node
 * is inserted.
 */
static __always_inline struct rb_node *
rb_find_add(struct rb_node *node, struct rb_root *tree,
            int (*cmp)(struct rb_node *, const struct rb_node *))
{
  struct rb_node **link = &tree->rb_node;
  struct rb_node *parent = NULL;

  while (*link)
  {
    parent = *link;
    int c = cmp(node, parent);

    if (c < 0)
      link = &parent->rb_left;
    else if (c > 0)
      link = &parent->rb_right;
    else
      return parent;
  }

  rb_link_node(node, parent, link);
  rb_insert_color(node, tree);
  return NULL;
}

/**
 * rb_find() - find @key in tree @tree
 * @key: key to match
 * @tree: tree to search
 * @cmp: operator defining the node order
 *
 * Returns the rb_node matching @key or NULL.
 */
static __always_inline struct rb_node *
rb_find(const void *key, const struct rb_root *tree,
        int (*cmp)(const void *key, const struct rb_node *))
{
  struct rb_node *node = tree->rb_node;

  while (node)
  {
    int c = cmp(key, node);

    if (c < 0)
      node = node->rb_left;
    else if (c > 0)
      node = node->rb_right;
    else
      return node;
  }

  return NULL;
}

#endif