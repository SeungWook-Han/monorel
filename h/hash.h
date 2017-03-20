#ifndef _HASH_H
#define _HASH_H

#include <stdlib.h>
#include "list.h"


struct hash_operations {
	/* Returing the key according to the data */
	unsigned int (*hashing)(struct list_head *data);
};

struct hash {
	unsigned int table_size;
	unsigned int total_buckets;
	struct list_head *table;
	const struct hash_operations *h_ops;
};

#define HASH_INIT(size, ops) { size, 0, 					\
	(struct list_head *)malloc(sizeof(struct list_head) * size), ops}

#define HASH(name, size, ops) 						\
	struct hash name = HASH_INIT(size, ops);

static struct list_head *hash_search(struct hash *hash, 
					    struct list_head *entry)
{
	unsigned int key = hash->h_ops->hashing(entry);
	if (key < hash->table_size) {
		struct list_head *iter;
		list_for_each_entry(iter, &hash->table[key]) {
			if (iter == entry) 
				return iter;
		}
	}
	return NULL;
}

/* Duplicated data can not be inserted. */
static void hash_insert(struct hash *hash, struct list_head *entry)
{
	unsigned int key = hash->h_ops->hashing(entry);
	
	if (key < hash->table_size && 
			hash_search(hash, entry) == entry) {

		list_add(entry, &hash->table[key]);
		hash->total_buckets++;
	}
}

static void hash_delete(struct hash *hash, struct list_head *entry)
{
	if (hash_search(hash, entry) == entry) {
		list_del_init(entry);
		hash->total_buckets--;
	}
}
