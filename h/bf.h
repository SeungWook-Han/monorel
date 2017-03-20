#ifndef __BF_H__
#define __BF_H__

/****************************************************************************
 * bf.h: external interface definition for the BF layer
 ****************************************************************************/

#include "minirel.h"

/*
 * size of buffer pool
 */
#define BF_MAX_BUFS     40

/*
 * size of BF hash table
 */
#define BF_HASH_TBL_SIZE 20

/*
 *
 */
typedef struct BFpage {
	PFpage		fpage;		/* page data from the file in minirel.h		*/
	struct BFpage	*next_page;	/* next in the linked list of buffer pages	*/
	struct BFpage	*prev_page;	/* prev in the linked list of buffer pages	*/
	bool_t		dirty;		/* TRUE if page is manipulated			*/
	short		count;		/* pin count associated with the page like lock	*/
	int		page_num;	/* page number of this page for identifying	*/
	int		fd;		/* PF file descriptor of this page		*/
	int		unix_fd;	/* */
} BFpage;


typedef struct BFhash_entry {
	struct BFhash_entry *next_entry;
	struct BFhash_entry *prev_entry;
	int fd;
	int page_num;
	struct BFpage *bpage;
} BFhash_entry;


/*
 * prototypes for BF-layer functions
 */
void BF_Init(void);
int BF_GetBuf(BFreq bq, PFpage **fpage);
int BF_AllocBuf(BFreq bq, PFpage **fpage);
int BF_UnpinBuf(BFreq bq);
int BF_TouchBuf(BFreq bq);
int BF_FlushBuf(int fd);
void BF_ShowBuf(void);
void BF_ShowHash(void);
/*
 * BF-layer error codes
 */
#define BFE_OK			0
#define BFE_NOMEM		(-1)
#define BFE_NOBUF		(-2)
#define BFE_PAGEFIXED		(-3)
#define BFE_PAGEUNFIXED		(-4)

#define BFE_PAGEINBUF		(-50)
#define BFE_PAGENOTINBUF	(-51)
#define BFE_HASHNOTFOUND	(-52)
#define BFE_HASHPAGEEXIST	(-53)

#define BFE_MISSDIRTY		(-60)
#define BFE_INVALIDTID		(-61)

/*
 * error in UNIX system call or library routine
 */
#define BFE_INCOMPLETEREAD	(-97)
#define BFE_INCOMPLETEWRITE	(-98)
#define BFE_MSGERR              (-99)
#define BFE_UNIX		(-100)

#endif

