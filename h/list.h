#ifndef _LIST_H
#define _LIST_H

struct list_head {
	struct list_head *prev, *next;
};

/**
 * Initialize the list as an empty list.
 *
 * Example:
 * INIT_LIST_HEAD(&bar->list_of_foos);
 *
 * @param The list to initialized.
 */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

#define LIST_HEAD(name) \
	struct list_head name = LIST_HEAD_INIT(name)

static void INIT_LIST_HEAD(struct list_head *list)
{
	list->next = list->prev = list;
}

static void __list_add(struct list_head *entry,
	   		      struct list_head *prev,
	 		      struct list_head *next)
{
	next->prev = entry;
	entry->next = next;
	entry->prev = prev;
	prev->next = entry;
}

/**
 * Insert a new element after the given list head. The new element does not
 * need to be initialised as empty list.
 * The list changes from:
 * 	head -> some element -> ...
 * to
 * 	head -> new element -> older element -> ...
 *
 * Example:
 * struct foo *newfoo = malloc(...);
 * list_add(&newfoo->entry, &bar->list_of_foos);
 */
static void list_add(struct list_head *entry, struct list_head *head)
{
	__list_add(entry, head, head->next);
}


/**
 * Append a new element to the end of the list given with this list head.
 *
 * The list changes from:
 * 	head -> some element -> ... -> last element
 * to
 * 	head -> some element -> ... -> last element -> new element
 *
 * Example:
 * struct foo *newfoo = malloc(...);
 * list_add_tail(&newfoo->entry, &bar->list_of_foos);
 *
 * @param entry The new element to perpend to the list.
 * @param head The existing list.
 */
static void list_add_tail(struct list_head *entry, 
				 struct list_head *head)
{
	__list_add(entry, head->prev, head);
}

static void __list_del(struct list_head *prev, 
			      struct list_head *next)
{
	next->prev = prev;
	prev->next = next;
}

/**
 * Remove the element from the list it is in. Using this function will reset
 * the pointers to/from this element so it is removed from the list. It does
 * NOT free the element itself or manipulate it otherwise.
 *
 * Using list_del on a pure list head (like in the example at the tope of
 * this file) will NOT remove the first element from
 * the list but rather reset the list as empty list.
 *
 * Example:
 * list_del(&foo->entry);
 *
 * @param entry The element to remove.
 */
static void list_del(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
}

static void list_del_init(struct list_head *entry)
{
	__list_del(entry->prev, entry->next);
	INIT_LIST_HEAD(entry);
}

static void list_move_tail(struct list_head *entry,
	       			  struct list_head *head)
{
	__list_del(entry->prev, entry->next);
	list_add_tail(entry, head);
}

/**
 * Check if the list is emptry.
 *
 * Example:
 * list_empty(&bar->list_of_foos);
 *
 * @return True if the list contains one or more elements or False otherwise.
 */
static int list_empty(struct list_head *head)
{
	return head->next == head;
}

/**
 * Returns a pointer to the container of this list element.
 *
 * Example:
 * struct foo *f;
 * f = container_of(&foo->entry, struct foo, entry);
 * assert(f == foo);
 *
 * @param ptr Pointer to the result list_head.
 * @param type Data type of the list element.
 * @param member Member name of the struct list_head field in the list element.
 * @return A pointer to the data struct containing the list head.
 */

#ifndef container_of
#define container_of(ptr, type, member) 				\
	(type *)((char *)(ptr) - (char *) &((type *)0)->member)
#endif

/**
 * Alias of container_of
 */
#define list_entry(ptr, type, member) 					\
	container_of(ptr, type, member);

#define list_first_entry(ptr, type, member) 				\
	container_of(ptr->next, type, member)

#define list_last_entry(ptr, type, member) 				\
	container_of(ptr->prev, type, member)

#define __container_of(ptr, sample, member) 				\
	(void *)container_of(ptr, typeof(*(sample)), member)

/**
 * Loop through the list given by head and set pos to struct in the list.
 *
 * Example:
 * struct foo *iterator;
 * list_for_each_object(iterator, &bar->list_of_foos, entry) {
 *	[modify iterator]
 * }
 *
 * This macro is not safe for node deletion. Use list_for_each_entry_safe
 * instead.
 *
 * @param pos Iterator variable of the type fo the list elements.
 * @param head List head
 * @param member Member name of the struct list_head in the list elements.
 */
#define list_for_each_object(pos, head, member)				\
	for (pos = __container_of((head)->next, pos, member);		\
	     &pos->member != (head);					\
	     pos = __container_of(pos->member.next, pos, member))

/**
 * Loop through the list given by head and set pos to struct in the list.
 *
 * Example:
 * struct head_list *iterator;
 * list_for_each_entry(iterator, &bar->list_of_foos) {
 *	[modify iterator]
 * }
 *
 * This macro is not safe for node deletion. Use list_for_each_entry_safe
 * instead.
 *
 * @param pos Iterator variable of the list head.
 * @param head List head
 */
#define list_for_each_entry(pos, head)					\
	for (pos  = (head)->next;					\
	     pos != head;						\
	     pos  = (pos)->next)

/**
 * Loop for remove safe
 */

/*
#define list_for_each_safe_entry(po)					\
*/

#endif
