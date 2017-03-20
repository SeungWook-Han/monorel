#ifndef _PRIORITY_LIST_H
#define _PRIORITY_LIST_H

struct priority_operations {
	int (*get_priority)(int 
};

struct priority_list {
	unsigned int count;
	struct list_head *list;
	struct priority_operations *p_ops;
};
