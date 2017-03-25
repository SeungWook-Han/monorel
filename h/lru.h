#ifndef _LRU_H
#define _LRU_H

struct lru {
	struct list_head *lru_list;
	struct lru_operations *l_ops;
};

struct lru_operations {
	void (*push)(struct list_head *entry);
	struct list_head * (*pop)(void);
	void (*remove)(struct list_head *entry);

};


/**
 * LRU Interface (Abstract Data Type)
 *
 * [Atomic Operations]
 * 1. void push(struct list_head *entry)
 * Insert the entry into head of LRU list.(Most Recently Used)
 *
 * 2. struct list_head *pop()
 * If the list is empty return NULL
 *
 * 3. void remove(struct list_head *entry)
 * Delete the entry from LRU list.
 * We do not care the entry is in LRU list, just make the entry solo.
 *
 * 4. struct list_head *search(?)
 *
 * 5. 
 *
 * 6. 
 *
 */
