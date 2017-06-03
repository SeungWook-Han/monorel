#ifndef __AM_H__
#define __AM_H__

#include <minirel.h>

/****************************************************************************
 * am.h: External interface to the AM layer
 ****************************************************************************/

#define AM_ITAB_SIZE MAXOPENFILES /* max number of AM files allowed */
#define MAXISCANS MAXOPENFILES /* max number of AM files allowed */
#define MAX_VAL_LENGTH 255 /* max char size of attribute values allowed */
#define FANOUT 4 /* max fanout of trees allowed */

#define LEFT -1
#define RIGHT 1
#define NOTDEFINED 0

/* datastructrues for b+ tree
 *
 */
typedef int offset_t;
typedef char att_val_t[MAX_VAL_LENGTH]; 

typedef struct _index_t {
	att_val_t value;
	offset_t child; /* childs offset */
} index_t;

typedef struct record_t {
	att_val_t value;
	RECID recId;
	bool_t deleted;
} record_t;

typedef struct _AMHeader {
	int pf_fd;
	int indexNo;
/*	int order;  order of b+ tree */
	int height; /* height of b+ tree (exclude leafs) */
	int nr_internal_nodes; /* number of internal nodes */
	int nr_leaf_nodes; /* number of leaf nodes */
	char attrType;
	int attrLength;
 	offset_t free_page_offset; /* where to store new node */
	offset_t root_offset; /* where is the root node */
	offset_t leaf_offset; /* where is the first leaf node */
} AMHeader;

typedef struct _internal_node {
	offset_t parent;
	offset_t next;
	offset_t prev;
	int nr_childrens;
	index_t children[FANOUT];
} internal_node;

typedef struct _leaf_node {
	offset_t parent;
	offset_t next;
	offset_t prev;
	int nr_childrens;
	record_t children[FANOUT];
} leaf_node;

typedef struct _am_index_entry {
	char fileName[MAX_VAL_LENGTH];
	int pf_fd;
	int indexNo;
	int valid;
	int scan_active;
} am_index_entry;

typedef struct _am_scan_entry {
	char fileName[MAX_VAL_LENGTH];
	int pf_fd;
	int indexNo;
	int fileDesc;
	int scanDesc;
	int op;
	att_val_t value;
	RECID recId;
	int valid;
	int org_leaf_offset;
	int org_entry_offset;
	int leaf_offset;
	int entry_offset;
	int direction;
} am_scan_entry;

/*
 * prototypes for AM functions
 */
void AM_Init		(void);
int  AM_CreateIndex	(char *fileName, int indexNo, char attrType,
			int attrLength, bool_t isUnique);
int  AM_DestroyIndex	(char *fileName, int indexNo); 
int  AM_OpenIndex       (char *fileName, int indexNo);
int  AM_CloseIndex      (int fileDesc);
int  AM_InsertEntry	(int fileDesc, char *value, RECID recId);
int  AM_DeleteEntry     (int fileDesc, char *value, RECID recId);
int  AM_OpenIndexScan	(int fileDesc, int op, char *value);
RECID AM_FindNextEntry	(int scanDesc);
int  AM_CloseIndexScan	(int scanDesc);
void AM_PrintError	(char *errString);

/*
 * newly defined AM functions
 */

void printTree(int fileDesc);
bool_t AM_GetNextEntry(struct _AMHeader *header, att_val_t value, int op, int direction, offset_t *leaf_offset, offset_t *entry_offset, record_t *find_record);
bool_t AM_GetFirstEntry(struct _AMHeader *header, att_val_t value, int op, offset_t *leaf_offset, offset_t *entry_offset, record_t *find_record);
int AM_CheckCondition(att_val_t ptr_val1, att_val_t ptr_val2, char attrType, int attrLength, int op);
bool_t AM_IsSameRecId(RECID recId, RECID another_recId);
void AM_ResetIndexChildParent(struct _AMHeader *header, int begin, int end, offset_t parent, index_t *indices);
void AM_InsertKeyToIndex(struct _AMHeader *header, offset_t offset, att_val_t value, offset_t old, offset_t after);
void AM_InsertKeyToIndexNoSplit(struct _AMHeader *header, internal_node *node, att_val_t value, offset_t point_to);
void AM_InsertRecNoSplit(struct _AMHeader *header, leaf_node *leaf, att_val_t value, RECID recId);
void AM_CreateInternalNode(struct _AMHeader *header, offset_t offset, internal_node *node, internal_node *next);
void AM_CreateLeafNode(struct _AMHeader *header, offset_t offset, leaf_node *node, leaf_node *next);
int AM_Insert(struct _AMHeader *header, att_val_t value, RECID recID);
offset_t AM_SearchIndex(struct _AMHeader *header, att_val_t value);
offset_t AM_SearchLeaf(struct _AMHeader *header, offset_t offset, att_val_t value);
int AM_CompAttVal(att_val_t ptr_val1, att_val_t ptr_val2, char attrType, int attrLength);
index_t *AM_UpperBound(index_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location);
index_t *AM_LowerBound(index_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location);
record_t *AM_UpperBoundRec(record_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location);
record_t *AM_LowerBoundRec(record_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location);
offset_t AM_AllocInternalNode(struct _AMHeader *header, internal_node *node);
offset_t AM_AllocLeafNode(struct _AMHeader *header, leaf_node *node);
offset_t AM_MarkUsedPage(struct _AMHeader *header);
int AM_Unmap(void *data, size_t size, int pf_fd, offset_t page_offset);
int AM_Map(void *data, size_t size, int pf_fd, offset_t page_offset);
int AM_InitHeader(struct _AMHeader *header, int pf_fd, int indexNo, char attrType, int attrLegnth);

/*
 * AM layer constants 
 */
#define AM_NERRORS      24      /* maximun number of AM  errors */    

/*
 * AM layer error codes
 */
#define		AME_OK			0
#define         AME_PF                  (-1)
#define		AME_EOF			(-2)
#define 	AME_FULLTABLE		(-3)
#define 	AME_INVALIDATTRTYPE	(-4)
#define 	AME_FD  		(-5)
#define 	AME_INVALIDOP		(-6)
#define 	AME_INVALIDPARA		(-7)		/* Ok not to use */
#define 	AME_DUPLICATEOPEN	(-8)		/* Ok not to use */
#define 	AME_SCANTABLEFULL	(-9)
#define 	AME_INVALIDSCANDESC	(-10)
#define 	AME_UNABLETOSCAN	(-11)		/* Ok not to use */
#define 	AME_RECNOTFOUND		(-12)
#define 	AME_DUPLICATERECID	(-13)
#define 	AME_SCANOPEN		(-14)
#define 	AME_INVALIDATTRLENGTH	(-15)

/* 
 * define your own error codes here 
 */
#define		AME_UNIX		(-16)		/* Ok not to use */
#define         AME_ROOTNULL            (-17)
#define         AME_TREETOODEEP         (-18)
#define         AME_INVALIDATTR         (-19)
#define         AME_NOMEM               (-20)
#define         AME_INVALIDRECORD       (-21)
#define         AME_TOOMANYRECSPERKEY   (-22)
#define         AME_KEYNOTFOUND         (-23)
#define         AME_DUPLICATEKEY        (-24)

/*
 * newly defined error code
 */
#define AME_NULL_POINTER	-25 /* Buffer has null pointer */
#define AME_GET_THIS_PAGE	-26 /* Failed to get page by pf */
#define AME_DIRTY_PAGE		-27 /* Failed to make dirty page by pf */
#define AME_PF_CREATE		-28 /* Failed to create new file by pf */
#define AME_PF_OPEN		-29 /* Failed to open the file by pf */
#define AME_REOPEN		-30 /* Try to re-open the opend file */
#define AME_ALLOC_PAGE		-31 /* Failed to allocate the new page */
#define AME_HEADER_WRITE	-32 /* Failed to write header info */
#define AME_PF_CLOSE		-33 /* Failed to close file by pf */
#define AME_PF_UNPIN		-34 
#define AME_HEADER_READ		-35 /* Failed to read header info */
#define AME_UNPIN		-36
#define AME_COMPVAL	-37
#define AME_FILE_DESTROY	-38 /* File Destory Error */

/*
 * global error value
 */
extern int AMerrno;

#endif
