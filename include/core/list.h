#ifndef __CORE_LIST_H__
#define __CORE_LIST_H__

#include <core/types.h>
#include <core/compiler.h>

/*
 * Intrusive doubly linked list.  Embedding a list_head in a structure costs
 * two pointers and allows O(1) splice, which is what the scheduler and the
 * driver registries need.
 */

struct list_head {
    struct list_head *next;
    struct list_head *prev;
};

#define LIST_HEAD(name) struct list_head name = { &(name), &(name) }

static inline void INIT_LIST_HEAD(struct list_head *head)
{
    head->next = head;
    head->prev = head;
}

static inline void __list_add(struct list_head *node,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = node;
    node->next = next;
    node->prev = prev;
    prev->next = node;
}

static inline void list_add(struct list_head *node, struct list_head *head)
{
    __list_add(node, head, head->next);
}

static inline void list_add_tail(struct list_head *node, struct list_head *head)
{
    __list_add(node, head->prev, head);
}

static inline void list_del(struct list_head *node)
{
    node->next->prev = node->prev;
    node->prev->next = node->next;
    node->next = node->prev = NULL;
}

static inline void list_del_init(struct list_head *node)
{
    list_del(node);
    INIT_LIST_HEAD(node);
}

static inline void list_move(struct list_head *node, struct list_head *head)
{
    list_del(node);
    list_add(node, head);
}

static inline void list_move_tail(struct list_head *node, struct list_head *head)
{
    list_del(node);
    list_add_tail(node, head);
}

static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

static inline void list_splice(struct list_head *list, struct list_head *head)
{
    if (!list_empty(list)) {
        struct list_head *first = list->next;
        struct list_head *last  = list->prev;
        struct list_head *at    = head->next;

        first->prev = head;
        head->next  = first;
        last->next  = at;
        at->prev    = last;
        INIT_LIST_HEAD(list);
    }
}

static inline int list_is_first(const struct list_head *node,
                                const struct list_head *head)
{
    return node->prev == head;
}

static inline int list_is_last(const struct list_head *node,
                               const struct list_head *head)
{
    return node->next == head;
}

static inline void list_replace(struct list_head *old,
                                struct list_head *node)
{
    node->next       = old->next;
    node->next->prev = node;
    node->prev       = old->prev;
    node->prev->next = node;
}

#define list_entry(ptr, type, member) container_of(ptr, type, member)

#define list_first_entry(head, type, member) \
    list_entry((head)->next, type, member)

#define list_last_entry(head, type, member) \
    list_entry((head)->prev, type, member)

#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, __typeof__(*(pos)), member)

#define list_prev_entry(pos, member) \
    list_entry((pos)->member.prev, __typeof__(*(pos)), member)

#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

#define list_for_each_safe(pos, tmp, head) \
    for (pos = (head)->next, tmp = pos->next; pos != (head); \
         pos = tmp, tmp = pos->next)

#define list_for_each_entry(pos, head, member)                         \
    for (pos = list_first_entry(head, __typeof__(*pos), member);       \
         &pos->member != (head);                                       \
         pos = list_next_entry(pos, member))

#define list_for_each_entry_safe(pos, tmp, head, member)               \
    for (pos = list_first_entry(head, __typeof__(*pos), member),       \
         tmp = list_next_entry(pos, member);                           \
         &pos->member != (head);                                       \
         pos = tmp, tmp = list_next_entry(tmp, member))

#define list_for_each_entry_reverse(pos, head, member)                 \
    for (pos = list_last_entry(head, __typeof__(*pos), member);        \
         &pos->member != (head);                                       \
         pos = list_prev_entry(pos, member))

#endif /* __CORE_LIST_H__ */
