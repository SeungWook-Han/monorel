/* bf.c 
 * 2017.3.11 by Togepi
 * TODO Refactoring hash related function
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "minirel.h"
#include "bf.h"

#define HASH_HEAD(hash_elem) (hash_elem->prev_entry == NULL)
#define HASH_TAIL(hash_elem) (hash_elem->next_entry == NULL)

/*
#define IS_LRU_EMPTY() (lru_head.next_page == &(lru_tail))
*/
#define IS_LRU_EMPTY() (lru_counter == 0)
#define IS_LRU_TAIL(page) (page == &(lru_tail))

#define FILE_PAGE_INIT_NUM -1
#define DESCRIPTOR_INIT_NUM -1

static struct BFhash_entry **hash_table = NULL;
static struct BFpage *free_pages = NULL;

/* When lru_head is NULL, always lru_tail is also NULL */
static struct BFpage lru_head;
static struct BFpage lru_tail;
static int lru_counter = 0;

void BF_Init(void);
int BF_AllocBuf(BFreq bq, PFpage **fpage);
int BF_GetBuf(BFreq bq, PFpage **fpage);
int BF_UnpinBuf(BFreq bq);
int BF_TouchBuf(BFreq bq);
int BF_FlushBuf(int fd);
void BF_ShowBuf(void);

static struct BFhash_entry *hashing(int fd, 
			      	    int page_num, 
			            struct BFhash_entry *hash_elem);


static struct BFpage *free_pages_get(void)
{
	if (free_pages == NULL) return NULL;
	else {
		struct BFpage *ret_page;

		ret_page = free_pages;
		free_pages = free_pages->next_page;
		ret_page->next_page = NULL;
		
		return ret_page;
	}
}

static void free_pages_return(struct BFpage *page)
{
	page->next_page = page->prev_page = NULL;
	page->dirty = FALSE;
	page->count = 0;
	page->page_num = page->fd = page->unix_fd = DESCRIPTOR_INIT_NUM;

	if (free_pages == NULL) free_pages = page;
	else {
		page->next_page = free_pages;
		free_pages = page;
	}

	return ;
}

static int lru_insert(struct BFpage *page)
{
	if (page->next_page != NULL && page->prev_page != NULL) return 1;
	else {
		struct BFpage *previous = lru_tail.prev_page;

		page->prev_page = previous;
		page->next_page = &lru_tail;
		
		previous->next_page = page;
		lru_tail.prev_page = page;
	
		lru_counter++;
		return BFE_OK;
	}
}

static int lru_delete(struct BFpage *page)
{
	if (page->next_page == NULL && page->prev_page == NULL) return 1;
	else {
		struct BFpage *next_page = page->next_page;
		struct BFpage *prev_page = page->prev_page;
		
		next_page->prev_page = prev_page;
		prev_page->next_page = next_page;

		page->next_page = page->prev_page = NULL;
		lru_counter--;
		return BFE_OK;
	}
}

/* Must be static.
 * Update order of the LRU-list when the page in LRU-list is accessed.
 */
static int lru_touch(struct BFpage *page)
{
	if (lru_delete(page) != BFE_OK || lru_insert(page) != BFE_OK) 
		return 1;
	
	return BFE_OK;
}

/* Must be static.
 * Hashing with fd and page number to check whether the requested page is in buffer.
 * return NULL when there is no page.
 * return BFpage when there is page.
 */
static struct BFhash_entry *hashing(int fd, 
			      	    int page_num, 
			            struct BFhash_entry *hash_elem)
{
	int hash = (fd + page_num) % BF_HASH_TBL_SIZE;	
	if (hash_elem == NULL) hash_elem = hash_table[hash];

	/* Traverse the link to find buffered page. */
	if (hash_elem == NULL) return NULL;
	else {
		if (hash_elem->fd == fd && hash_elem->page_num == page_num) 
			return hash_elem;
		else {
			if (hash_elem->next_entry == NULL) return NULL;
			return hashing(fd, page_num, hash_elem->next_entry);
		}
	}
}

static int hash_insert(struct BFpage *page)
{
	/* TODO checking whether the page to be inserted is already in hash */
	struct BFhash_entry *parent, *child;
	int hash;

	child = malloc(sizeof(struct BFhash_entry));

	child->fd = page->fd;
	child->page_num = page->page_num;
	child->bpage = page;
	child->next_entry = child->prev_entry = NULL;

	hash = (page->fd + page->page_num) % BF_HASH_TBL_SIZE;	
	parent = hash_table[hash];


	if (parent != NULL) {
		parent->prev_entry = child;
		child->next_entry = parent;
	}

	hash_table[hash] = child;
	return BFE_OK;
}

static int hash_delete(struct BFpage *page)
{
	struct BFhash_entry *hash_elem;
	int hash;


	hash_elem = hashing(page->fd, page->page_num, NULL);
	hash = (page->fd + page->page_num) % BF_HASH_TBL_SIZE;	


	/* The hash elem of page is not in hash table */
	if (hash_elem == NULL) return 1;

	if (HASH_HEAD(hash_elem) && HASH_TAIL(hash_elem)) {
		hash_table[hash] = NULL;

	} else if (HASH_HEAD(hash_elem)) {
		hash_table[hash] = hash_elem->next_entry;
		hash_table[hash]->prev_entry = NULL;

	} else if (HASH_TAIL(hash_elem)) {
		hash_elem->prev_entry->next_entry = NULL;

	} else {
		hash_elem->prev_entry->next_entry = hash_elem->next_entry;
		hash_elem->next_entry->prev_entry = hash_elem->prev_entry;
	}
	
	free(hash_elem);

	return BFE_OK;
}

static struct BFpage *evict_policy(void)
{
	struct BFpage *page = NULL;
	struct BFpage *target = NULL;
	
	if (IS_LRU_EMPTY()) {
		/* Oops something wrong... */
		printf("Oops something wrong...\n");
		return NULL;
	}

	for (page  = lru_head.next_page; 
	     !IS_LRU_TAIL(page); 
	     page  = page->next_page) {

		/* Find page for what? */
		if (page->count == 0) {
			target = page;
			break;
		}
	}

	return target;
}

static int flush_page(struct BFpage *page)
{
	if (page->dirty == TRUE) {
		if  (pwrite(page->unix_fd, page->fpage.pagebuf, 
			    PAGE_SIZE, PAGE_SIZE * page->page_num) != PAGE_SIZE) 
		{ 
			printf("File write failed!!!!\n");
			return 1;
		}
		page->dirty = FALSE;
	}
	
	return BFE_OK; 
}

/* Fail to evict the specific target page : NULL */
/* Success				  : Evicted page */
static struct BFpage *evict_page(struct BFpage *target)
{
	if (target == NULL) {
		target = evict_policy();
	}

	if (flush_page(target))
		return NULL;	
	if (hash_delete(target)) 
		return NULL;
	if (lru_delete(target)) 
		return NULL;
	
	return target;
}

/* Creating all buffer entries and add them to the free list.
 * Init hash table
 */
void BF_Init(void)
{
	int i = 0;

	/* Initializing free list */
	for (i = 0; i < BF_MAX_BUFS; i++) {
		struct BFpage *page = malloc(sizeof(struct BFpage));
		free_pages_return(page);
	}

	/* Initializing LRU-list */
	lru_head.prev_page = lru_tail.next_page = NULL;
	lru_head.next_page = &lru_tail;
	lru_tail.prev_page = &lru_head;
	
	/* Initializing hash table */
	hash_table = malloc(sizeof(struct BFhash_entry *) * BF_HASH_TBL_SIZE);
	for (i = 0; i < BF_HASH_TBL_SIZE; i ++)
		hash_table[i] = NULL;

	return ;
}

int BF_GetBuf(BFreq bq, PFpage **fpage)
{
	int fd = bq.fd;
	int unix_fd = bq.unixfd;
	int page_num = bq.pagenum;
	struct BFpage *page;
	struct BFhash_entry *hash_elem;

	/* Checking whether requested page is already in buffer */
	hash_elem = hashing(fd, page_num, NULL);
	
	if (hash_elem != NULL) page = hash_elem->bpage;
	else {
		/* 1. Get free buffered page in free_pages */
		struct BFpage *free_page = free_pages_get();

		if (free_page == NULL) {
			free_page = evict_page(NULL);
			if (free_page == NULL) return BFE_NOMEM; /* All pages are busy */

			free_pages_return(free_page);
			free_page = free_pages_get();
		}

		free_page->fd = fd;
		free_page->unix_fd = unix_fd;
		free_page->page_num = page_num;

		page = free_page;
		/* 2. Read data from file and Store into buffered page */
		if ((pread(unix_fd, page->fpage.pagebuf, 
			   PAGE_SIZE, PAGE_SIZE * page_num)) != PAGE_SIZE) 
			return BFE_UNIX;
	
		lru_insert(page);
		hash_insert(page);
	}
		
	lru_touch(page);
	page->count++;
	*fpage = &(page->fpage);

	return BFE_OK;
}

int BF_AllocBuf(BFreq bq, PFpage **fpage)
{
	/* Not overwrite, it is new write (If the page to be written is already */
	/* in buffer, than that is error...) */
	struct BFhash_entry *hash_elem;
	struct BFpage *free_page;

	hash_elem = hashing(bq.fd, bq.pagenum, NULL);
	if (hash_elem != NULL) return BFE_INCOMPLETEWRITE; 

	free_page = free_pages_get();
	if (free_page == NULL) {
		
		free_page = evict_page(NULL);
		
		if (free_page == NULL) return BFE_NOMEM;

		free_pages_return(free_page);
		free_page = free_pages_get();
	}

	/* 2. Register the page into LRU-list and Hash table then making pinned */
	free_page->fd = bq.fd;
	free_page->page_num = bq.pagenum;
	free_page->unix_fd = bq.unixfd;
	free_page->count++;
	*fpage = &(free_page->fpage);
	
	if (lru_insert(free_page)) return 1;
	if (hash_insert(free_page)) return 1;


	return BFE_OK;
}

int BF_UnpinBuf(BFreq bq)
{
	struct BFhash_entry *hash_elem;
	struct BFpage *page;

	hash_elem = hashing(bq.fd, bq.pagenum, NULL);

	if (hash_elem != NULL) {
		page = hash_elem->bpage;
		if (page->count != 0) page->count--;
	}
	return BFE_OK;
}

int BF_TouchBuf(BFreq bq)
{
	struct BFhash_entry *hash_elem = hashing(bq.fd, bq.pagenum, NULL);
	struct BFpage *page = NULL;

	if (hash_elem == NULL) return BFE_PAGENOTINBUF;

	page = hash_elem->bpage;
	page->dirty = TRUE;
	lru_touch(page);

	return BFE_OK;
}


/* FIXME  Needing the data structure for more fast method to find */
/* the pages belonging to fd. (Current implementation -> linear search) */
int BF_FlushBuf(int fd)
{
	/* Checking whether the pages are pinned. */
	struct BFpage *page = NULL;
	struct BFpage *next_page = NULL;

	for (page  = lru_head.next_page;
	     !IS_LRU_TAIL(page);
	     page  = page->next_page) {
		
		if (page->fd == fd && page->count != 0)
			return BFE_PAGEFIXED;
	}

	/* Duplicating theh code over for avoiding roll-back. */
	for (page = lru_head.next_page;
	     !IS_LRU_TAIL(page);
	     page = next_page) {
	
		next_page = page->next_page;

		if (page->fd == fd) {
			flush_page(page);
			hash_delete(page);
			lru_delete(page);
			free_pages_return(page);
		}
	}

	return BFE_OK;
}

void BF_ShowBuf(void)
{
	struct BFpage *page;

	printf("<LRU-list Printing...>\n");
	printf("%-15s%-15s%-15s%-15s%-15s%-15s\n\n", 
		"fd", "unix_fd", "page_num", "count", "dirty", "data");

	for (page  = lru_head.next_page;
	     !IS_LRU_TAIL(page);
	     page  = page->next_page) {
	
		int dirty;
		
		if (page->dirty == TRUE) dirty = 1;
		else			 dirty = 0;

		printf("%-15d%-15d%-15d%-15d%-15d%-15s\n",
			page->fd, page->unix_fd, page->page_num, 
			(int)page->count, dirty, page->fpage.pagebuf);
	}

	printf("Total LRU-list Count : %d\n", lru_counter);

	return ;
}

void BF_ShowHash(void)
{
	int i = 0;
	
	printf("<Hash Table Printing...>\n");
	for (i = 0; i < BF_HASH_TBL_SIZE; i++) {
		printf("<%d> ", i);
		if (hash_table[i] == NULL) printf("NULL\n");
		else {
			printf("%-15d%-15d\n", hash_table[i]->fd, 
					hash_table[i]->page_num);
		}

	}

	return ;
}
