#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "list.h"

#include "queue.h"

/* Notice: sometimes, Cppcheck would find the potential NULL pointer bugs,
 * but some of them cannot occur. You can suppress them by adding the
 * following line.
 *   cppcheck-suppress nullPointer
 */


#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)

/* Create an empty queue */
struct list_head *q_new()
{
    struct list_head *head = malloc(sizeof(struct list_head));
    if (!head)
        return NULL;  // Malloc failure
    INIT_LIST_HEAD(head);
    return head;
}

/* Free all storage used by queue */
void q_free(struct list_head *l)
{
    if (!l)
        return;
    struct list_head *i = l;
    element_t *entry, *safe;
    list_for_each_entry_safe (entry, safe, i, list) {
        q_release_element(entry);
    }
    free(l);
}

static inline element_t *e_new(const char *s)
{
    element_t *n = malloc(sizeof(element_t));
    char *tmp = strdup(s);

    if (!n || !tmp) {
        free(n);
        free(tmp);
        return NULL;
    }

    n->value = tmp;
    return n;
}

/* Insert an element at head of queue */
bool q_insert_head(struct list_head *head, char *s)
{
    if (unlikely(!head))
        return false;
    element_t *n = e_new(s);
    if (unlikely(!n))
        return false;
    list_add(&n->list, head);
    return true;
}

/* Insert an element at tail of queue */
bool q_insert_tail(struct list_head *head, char *s)
{
    if (unlikely(!head))
        return false;

    return q_insert_head(head->prev, s);
}

/* Remove an element from head of queue */
element_t *q_remove_head(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *victim = list_first_entry(head, element_t, list);
    list_del(&victim->list);
    if (likely(sp)) {
        strncpy(sp, victim->value, bufsize);
        sp[bufsize - 1] = 0;
    }
    return victim;
}

/* Remove an element from tail of queue */
element_t *q_remove_tail(struct list_head *head, char *sp, size_t bufsize)
{
    if (!head || list_empty(head))
        return NULL;
    element_t *victim = list_last_entry(head, element_t, list);
    list_del(&victim->list);
    if (likely(sp)) {
        strncpy(sp, victim->value, bufsize);
        victim->value[bufsize - 1] = 0;
    }
    return victim;
}

/* Return number of elements in queue */
int q_size(struct list_head *head)
{
    if (!head)
        return 0;

    int len = 0;
    struct list_head *li;
    list_for_each (li, head)
        len++;
    return len;
}

/* Delete the middle node in queue */
bool q_delete_mid(struct list_head *head)
{
    // https://leetcode.com/problems/delete-the-middle-node-of-a-linked-list/
    if (!head || list_empty(head))
        return false;
    struct list_head *right = head->next, *left = head->prev;
    while (true) {
        if (right == left)
            break;
        right = right->next;
        if (right == left)
            break;
        left = left->prev;
    }
    list_del(right);
    q_release_element(list_entry(right, element_t, list));
    return true;
}

/* Delete all nodes that have duplicate string */
bool q_delete_dup(struct list_head *head)
{
    // https://leetcode.com/problems/remove-duplicates-from-sorted-list-ii/
    if (!head || list_empty(head))
        return false;
    bool mark_delete = false;
    element_t *iter, *safe;
    list_for_each_entry_safe (iter, safe, head, list) {
        if (!strcmp(iter->value, safe->value)) {
            mark_delete = true;
            list_del(&iter->list);
            q_release_element(iter);
        } else if (mark_delete) {
            mark_delete = false;
            list_del(&iter->list);
            q_release_element(iter);
        }
        if (safe->list.next == head)
            break;
    }
    return true;
}

/* Swap every two adjacent nodes */
void q_swap(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *left = head->next, *right = left->next;
    while (right != head && left != head) {
        /* Neighbors */
        right->next->prev = left;
        left->prev->next = right;
        /* The Neighbors between nodes are remained */
        right->prev = left->prev;
        left->next = right->next;
        /* Swap the nodes */
        right->next = left;
        left->prev = right;

        left = left->next;
        right = left->next;
    }
    // https://leetcode.com/problems/swap-nodes-in-pairs/
}

/* Reverse elements in queue */
void q_reverse(struct list_head *head)
{
    if (!head)
        return;
    struct list_head *fast = head->next, *slow = head, *tmp;
    do {
        tmp = slow->next;
        slow->next = slow->prev;
        slow->prev = tmp;
    } while (slow = fast, fast = fast->next, slow != head);
}

/* Reverse the nodes of the list k at a time */
void q_reverseK(struct list_head *head, int k)
{
    // https://leetcode.com/problems/reverse-nodes-in-k-group/
}

static void merge_sort_conquer(struct list_head *dest,
                               struct list_head *victim,
                               bool descend)
{
    long mask = descend ? 0 : INT_MIN;
    LIST_HEAD(result);
    struct list_head *insert;
    while (!list_empty(dest) && !list_empty(victim)) {
        element_t *e_victim = list_first_entry(victim, element_t, list),
                  *e_dest = list_first_entry(dest, element_t, list);
        insert = strcmp(e_victim->value, e_dest->value) & mask ? victim->next
                                                               : dest->next;
        list_move_tail(insert, &result);
    }

    insert = list_empty(dest) ? victim : dest;
    list_splice_tail(insert, &result);

    INIT_LIST_HEAD(insert);

    list_splice(&result, dest);
    return;
}

/* Sort elements of queue in ascending/descending order */
void q_sort(struct list_head *head, bool descend)
{
    if (!head || list_is_singular(head) || list_empty(head))
        return;

    struct list_head *right = head->next, *left = head->prev;
    while (true) {
        if (right == left)
            break;
        left = left->prev;
        if (right == left)
            break;
        right = right->next;
    }

    LIST_HEAD(mid);
    list_cut_position(&mid, head, right);

    q_sort(head, descend);
    q_sort(&mid, descend);

    merge_sort_conquer(head, &mid, descend);
}


/* Remove every node which has a node with a strictly less value anywhere to
 * the right side of it */
int q_ascend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    if (!head || list_empty(head))
        return 0;

    element_t *iter, *safe;
    bool del = false;
    list_for_each_entry_safe (iter, safe, head, list) {
        if (del) {
            del = false;
            list_del(&iter->list);
            q_release_element(iter);
        }
        if (&safe->list == head)
            break;
        /* it is NOT ALLOWED to delete `safe` */
        if (strcmp(iter->value, safe->value) >= 0)
            del = true;
    }

    return q_size(head);
}

/* Remove every node which has a node with a strictly greater value anywhere to
 * the right side of it */
int q_descend(struct list_head *head)
{
    // https://leetcode.com/problems/remove-nodes-from-linked-list/
    if (!head || list_empty(head))
        return 0;

    element_t *iter, *safe;
    bool del = false;
    list_for_each_entry_safe (iter, safe, head, list) {
        if (del) {
            del = false;
            list_del(&iter->list);
            q_release_element(iter);
        }
        if (&safe->list == head)
            break;
        if (strcmp(iter->value, safe->value) <= 0)
            del = true;
    }

    return q_size(head);
}

/* Merge all the queues into one sorted queue, which is in ascending/descending
 * order */
int q_merge(struct list_head *head, bool descend)
{
    return 0;
}
