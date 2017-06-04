#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "minirel.h"
#include "pf.h"
#include "bf.h"
#include "am.h"

#define AM_INVALID_OFFSET -1
#define AM_HEADER_PAGE_OFFSET 0
#define AM_DATA_PAGE_OFFSET 1

struct _am_index_entry *am_index_table;
struct _am_scan_entry *am_scan_table;
int AMerrno;

void printTree(int fileDesc) {
	int nr_leaf, nr_internal, nr_per_node;
	int loop, loop2;
	int offset;
	leaf_node leaf;
	internal_node node, prev;
	
	struct _am_index_entry *index_entry = &am_index_table[fileDesc];

	AMHeader amh;
	AM_Map(&amh, sizeof(amh), index_entry->pf_fd, AM_HEADER_PAGE_OFFSET);


	nr_internal = amh.nr_internal_nodes;
	nr_leaf = amh.nr_leaf_nodes;
	offset = amh.leaf_offset;

	printf("Header Info Number of Intenal Nodes:%d\tNumber of Leaf Nodes:%d\n", nr_internal, nr_leaf);

	AM_Map(&node, sizeof(node), amh.pf_fd, amh.root_offset);
	printf("internal node - parent:%d, next:%d, prev:%d, nr_childrens:%d\n", node.parent, node.next, node.prev, node.nr_childrens);


	for (loop = 0; loop < nr_leaf; loop++)
	{	
		AM_Map(&leaf, sizeof(leaf), amh.pf_fd, offset);

		printf("Leaf Number:%d\toffset:%d\tNumber of childrens:%d\n", loop, offset, leaf.nr_childrens);

		for (loop2 = 0; loop2 < leaf.nr_childrens; loop2++) {	
		  printf("Value:%s\tRECID.recnum:%d\tDeleted:%d\n", leaf.children[loop2].value, leaf.children[loop2].recId.recnum, leaf.children[loop2].deleted);
		}


		offset = leaf.next;
		if (offset == 0) break;		
	}
}

void AM_MarkDeleted(struct _AMHeader *header, offset_t leaf_offset, offset_t entry_offset) {
	leaf_node leaf;
	AM_Map(&leaf, sizeof(leaf), header->pf_fd, leaf_offset);
	leaf.children[entry_offset].deleted = TRUE;
	AM_Unmap(&leaf, sizeof(leaf), header->pf_fd, leaf_offset);
}

int AM_Delete(struct _AMHeader *header, char *value, RECID recId)
{
	record_t find_record;
	offset_t leaf_offset, entry_offset;
	bool_t find;
	leaf_node leaf;

	if (!AM_GetFirstEntry(header, value, EQ_OP, &leaf_offset, &entry_offset, &find_record))
		return AME_RECNOTFOUND;

	if (AM_IsSameRecId(recId, find_record.recId) && find_record.deleted == FALSE) {
		AM_MarkDeleted(header, leaf_offset, entry_offset);
		return AME_OK;
	}

	/* find to left direction */
	while (TRUE) {
		if (AM_GetNextEntry(header, value, EQ_OP, LEFT, &leaf_offset, &entry_offset, &find_record)) {
			if (AM_IsSameRecId(recId, find_record.recId) && find_record.deleted == FALSE) {
				AM_MarkDeleted(header, leaf_offset, entry_offset);
				return AME_OK;
			}	
		}
	/* end of left direction */
		else
			break;
	}
	/* find to right direction */
	while (TRUE) {
		if (AM_GetNextEntry(header, value, EQ_OP, RIGHT, &leaf_offset, &entry_offset, &find_record)) {
			if (AM_IsSameRecId(recId, find_record.recId) && find_record.deleted == FALSE) {
				AM_MarkDeleted(header, leaf_offset, entry_offset);
				return AME_OK;
			}	
		}
	/* end of left direction */
		else
			break;
	}

	printf("AM_Delete: record not found\n");
	return AME_RECNOTFOUND;

}

bool_t AM_GetNextEntry(struct _AMHeader *header, att_val_t value, int op, int direction, offset_t *leaf_offset, offset_t *entry_offset, record_t *find_record)
{
	leaf_node leaf;
	int entry_loop;
	
	AM_Map(&leaf, sizeof(leaf), header->pf_fd, *leaf_offset);

	if (direction == LEFT) {
		/* move to previous leaf */
		if ((*entry_offset == 0) || (*entry_offset == -1)) {
			*leaf_offset = leaf.prev;

			/* end of first leaf
			 * terminate search */
			if (*leaf_offset == 0) {
				return FALSE;
			}

			AM_Map(&leaf, sizeof(leaf), header->pf_fd, *leaf_offset);
			*entry_offset = leaf.nr_childrens - 1;
		}


		else {
			(*entry_offset)--;
		}
	}

	else if (direction == RIGHT) {
		/* move to next leaf */
		if ((*entry_offset == leaf.nr_childrens - 1) || (*entry_offset == -1)) {
			*leaf_offset = leaf.next;

			/* end of last leaf
			 * terminate search */
			if (*leaf_offset == 0) {
				return FALSE;
			}

			AM_Map(&leaf, sizeof(leaf), header->pf_fd, *leaf_offset);
			*entry_offset = 0;
		}


		else {
			(*entry_offset)++;
		}
	}

	else {
		printf("AM_GetNextEntry: Invalid direction..\n");
	}

	if (AM_CheckCondition(leaf.children[*entry_offset].value, value, header->attrType, header->attrLength, op)) {
		memcpy(find_record, &(leaf.children[*entry_offset]), sizeof(record_t));
		return TRUE;
	}

	else
		return FALSE;
}

bool_t AM_GetFirstEntry(struct _AMHeader *header, att_val_t value, int op, offset_t *leaf_offset, offset_t *entry_offset, record_t *find_record)
{
	offset_t parent;
	leaf_node leaf;
	bool_t find = FALSE;
	int entry_loop;

	parent = AM_SearchIndex(header, value);
	*leaf_offset = AM_SearchLeaf(header, parent,value);	

	AM_Map(&leaf, sizeof(leaf), header->pf_fd, *leaf_offset);
	for (entry_loop = 0; entry_loop < leaf.nr_childrens; entry_loop++) {
		if (AM_CheckCondition(leaf.children[entry_loop].value, value, header->attrType, header->attrLength, op)) {
			find = TRUE;
			*entry_offset = entry_loop;
		}
	}


	if (find) {
		memcpy(find_record, &(leaf.children[*entry_offset]), sizeof(record_t));
	}
	
	return find;
}

int AM_CheckCondition(att_val_t ptr_val1, att_val_t ptr_val2, char attrType, int attrLength, int op) 
{
	int ret = -1;
	switch (attrType) {
		case INT_TYPE: 
		{
			int attribute = *(int *)ptr_val1;
			int value = *(int *)ptr_val2;
			int result = attribute - value;
			ret = 0;
			switch (op) {
				case EQ_OP: if (result == 0) ret = 1; break;
				case LT_OP: if (result <  0) ret = 1; break;
				case GT_OP: if (result >  0) ret = 1; break;
				case LE_OP: if (result <= 0) ret = 1; break;
				case GE_OP: if (result >= 0) ret = 1; break;
				case NE_OP: if (result != 0) ret = 1; break;
			}
			return ret;
		}

		case REAL_TYPE:
		{
			float attribute = *(float *)ptr_val1;
			float value = *(float *)ptr_val2;
			float result = attribute - value;
			ret = 0;
			switch (op) {
				case EQ_OP: if (result == 0) ret = 1; break;
				case LT_OP: if (result <  0) ret = 1; break;
				case GT_OP: if (result >  0) ret = 1; break;
				case LE_OP: if (result <= 0) ret = 1; break;
				case GE_OP: if (result >= 0) ret = 1; break;
				case NE_OP: if (result != 0) ret = 1; break;
			}
			return ret;
		}

		case STRING_TYPE:
		{
			int result = strncmp(ptr_val1, ptr_val2,
						attrLength);
			ret = 0;
			switch (op) {
				case EQ_OP: if (result == 0) ret = 1; break;
				case LT_OP: if (result <  0) ret = 1; break;
				case GT_OP: if (result >  0) ret = 1; break;
				case LE_OP: if (result <= 0) ret = 1; break;
				case GE_OP: if (result >= 0) ret = 1; break;
				case NE_OP: if (result != 0) ret = 1; break;	
			}
			return ret;
		}
		default :
			printf("AM_CheckCondition: attrType Error\n");
			break;
	}

	return ret;
}

bool_t AM_IsInvalidRecId(RECID recId)
{
	if ((recId.pagenum == -1) &&
			(recId.recnum == -1))
		return TRUE;

	else
		return FALSE;
}

bool_t AM_IsSameRecId(RECID recId, RECID another_recId) 
{
	if ((recId.pagenum == another_recId.pagenum) &&
			(recId.recnum == another_recId.recnum))
		return TRUE;

	else
		return FALSE;
}

void AM_ResetIndexChildParent(struct _AMHeader *header, int begin, int end, offset_t parent, index_t *indices)
{
	internal_node node;
	int loop;
	offset_t offset;


	for (loop = begin; loop < end + 1; loop++) {
		offset = indices[loop].child;
		AM_Map(&node, sizeof(node), header->pf_fd, offset);
		node.parent = parent;
		AM_Unmap(&node, sizeof(node), header->pf_fd, offset); 
	}
}

void AM_InsertKeyToIndex(struct _AMHeader *header, offset_t offset, att_val_t value, offset_t old, offset_t after)
{
	internal_node node;
	int point, copy_loop, idx_loop;
	bool_t place_right;
		
	att_val_t middle_key;

	if (offset == 0) {
		/* create new root */
		internal_node root;
		root.next = root.prev = root.parent = 0;
		header->root_offset = AM_AllocInternalNode(header, &root);
		header->height++;

		root.nr_childrens = 2;
		memcpy(root.children[0].value, value, MAX_VAL_LENGTH);
		root.children[0].child = old;
		root.children[1].child = after;

		AM_Unmap(header, sizeof(AMHeader), header->pf_fd, AM_HEADER_PAGE_OFFSET); 
		AM_Unmap(&root, sizeof(root), header->pf_fd, header->root_offset);

		AM_ResetIndexChildParent(header, 0, root.nr_childrens - 1, header->root_offset, root.children);		

		return; 
	}

	AM_Map(&node, sizeof(node), header->pf_fd, offset);
	
	/* split needed */
	if (node.nr_childrens == FANOUT) {
		internal_node new_node;
		AM_CreateInternalNode(header, offset, &node, &new_node);
		
		point = (node.nr_childrens - 1) / 2;

		place_right = AM_CompAttVal(value, node.children[point].value, header->attrType, header->attrLength) > 0;

		if (place_right) 
			point++;
		if (place_right && (AM_CompAttVal(value, node.children[point].value, header->attrType, header->attrLength) < 0))
			point--;

		memcpy(middle_key, node.children[point].value, MAX_VAL_LENGTH);
		
		for (idx_loop = point + 1, copy_loop = 0; idx_loop < node.nr_childrens; idx_loop++, copy_loop++) {
			memcpy(&(new_node.children[copy_loop]), &(node.children[idx_loop]), sizeof(index_t));
		}
		
		new_node.nr_childrens = node.nr_childrens - point - 1;
		node.nr_childrens = point + 1;

		if (place_right)
			AM_InsertKeyToIndexNoSplit(header, &new_node, value, after);

		else
			AM_InsertKeyToIndexNoSplit(header, &node, value, after);
		
		AM_Unmap(&node, sizeof(node), header->pf_fd, offset);
		AM_Unmap(&new_node, sizeof(new_node), header->pf_fd, node.next);

		AM_ResetIndexChildParent(header, 0, new_node.nr_childrens - 1, node.next, new_node.children);		
		
		AM_InsertKeyToIndex(header, node.parent, middle_key, offset, node.next);
	}

	else {
		AM_InsertKeyToIndexNoSplit(header, &node, value, after);
		AM_Unmap(&node, sizeof(node), header->pf_fd, offset);
	}

}

void AM_InsertKeyToIndexNoSplit(struct _AMHeader *header, internal_node *node, att_val_t value, offset_t point_to)
{
	int copy_loop;
	int location = 0;
	int *ptr_location = &location;
	index_t *where = AM_UpperBound(node->children, 0, node->nr_childrens - 1, value, header->attrType, header->attrLength, ptr_location);


	for (copy_loop = node->nr_childrens - 1; copy_loop >= *ptr_location; copy_loop--) {
		memcpy(&(node->children[copy_loop + 1]), &(node->children[copy_loop]), sizeof(index_t)); 
	}
	
	memcpy(where->value, value, MAX_VAL_LENGTH);
	where->child = (where + 1)->child;
	(where + 1)->child = point_to;
	node->nr_childrens++;
}

void AM_InsertRecNoSplit(struct _AMHeader *header, leaf_node *leaf, att_val_t value, RECID recId)
{
	int copy_loop;
	int location = 0;
	int *ptr_location = &location;
	record_t *where = AM_UpperBoundRec(leaf->children, 0, leaf->nr_childrens - 1, value, header->attrType, header->attrLength, ptr_location);
	
	if (*ptr_location == leaf->nr_childrens - 1) {
		if (AM_CompAttVal(value, leaf->children[*ptr_location].value, header->attrType, header->attrLength) > 0) 
		{
			(*ptr_location)++;
			where = &(leaf->children[*ptr_location]);
		}
	}

	for (copy_loop = leaf->nr_childrens - 1; copy_loop >= *ptr_location; copy_loop--) {
		memcpy(&(leaf->children[copy_loop + 1]), &(leaf->children[copy_loop]), sizeof(record_t)); 
	}
	
	memcpy(where->value, value, MAX_VAL_LENGTH);
	where->recId = recId;
	where->deleted = 0;
	leaf->nr_childrens++;
}

void AM_CreateInternalNode(struct _AMHeader *header, offset_t offset, internal_node *node, internal_node *next)
{
	/* new sibling node */
	next->parent = node->parent;
	next->next = node->next;
	next->prev = offset;
	node->next = AM_AllocInternalNode(header, next);
	
	/* update next node's prev */
	if (next->next != 0) {
		internal_node old_next;
		AM_Map(&old_next, sizeof(old_next), header->pf_fd, next->next);
		old_next.prev = node->next;
		AM_Unmap(&old_next, sizeof(old_next), header->pf_fd, next->next);
	}

	AM_Unmap(header, sizeof(AMHeader), header->pf_fd, AM_HEADER_PAGE_OFFSET);
}
 
void AM_CreateLeafNode(struct _AMHeader *header, offset_t offset, leaf_node *node, leaf_node *next)
{
	/* new sibling node */
	next->parent = node->parent;
	next->next = node->next;
	next->prev = offset;
	node->next = AM_AllocLeafNode(header, next);
	
	/* update next node's prev */
	if (next->next != 0) {
		leaf_node old_next;
		AM_Map(&old_next, sizeof(old_next), header->pf_fd, next->next);
		old_next.prev = node->next;
		AM_Unmap(&old_next, sizeof(old_next), header->pf_fd, next->next);
	}

	AM_Unmap(header, sizeof(AMHeader), header->pf_fd, AM_HEADER_PAGE_OFFSET);
} 

int AM_Insert(struct _AMHeader *header, att_val_t value, RECID recId)
{
	offset_t parent;
	offset_t offset;
	leaf_node leaf, new_leaf;
	int point, copy_loop, idx_loop;
	bool_t place_right;
	
	parent = AM_SearchIndex(header, value);
	offset = AM_SearchLeaf(header, parent, value);

	AM_Map(&leaf, sizeof(leaf), header->pf_fd, offset);
	
	/* split when full */
	if (leaf.nr_childrens == FANOUT) {
		/* new sibling leaf */
		AM_CreateLeafNode(header, offset, &leaf, &new_leaf);

		/* find split point */
		point = leaf.nr_childrens / 2;
		place_right = AM_CompAttVal(value, leaf.children[point].value, header->attrType, header->attrLength) > 0;
		if (place_right) point++;

		/* split node */
		for (copy_loop = 0, idx_loop = point; idx_loop < leaf.nr_childrens; copy_loop++, idx_loop++)
		{
			memcpy(&(new_leaf.children[copy_loop]), &(leaf.children[idx_loop]), sizeof(leaf.children[idx_loop]));
		}
		
/*		printf("before: leaf nr:%d, new leaf nr:%d\n", leaf.nr_childrens, new_leaf.nr_childrens); */
		new_leaf.nr_childrens = leaf.nr_childrens - point;
		leaf.nr_childrens = point;
/*		printf("after: leaf nr:%d, new leaf nr:%d\n", leaf.nr_childrens, new_leaf.nr_childrens); */

		if (place_right)
			AM_InsertRecNoSplit(header, &new_leaf, value, recId);
		else
			AM_InsertRecNoSplit(header, &leaf, value, recId);

		AM_Unmap(&leaf, sizeof(leaf), header->pf_fd, offset);
		AM_Unmap(&new_leaf, sizeof(new_leaf), header->pf_fd, leaf.next);

		/* insert key to index */
		AM_InsertKeyToIndex(header, parent, new_leaf.children[0].value, offset, leaf.next);			
	}
	
	else {
		AM_InsertRecNoSplit(header, &leaf, value, recId);
		AM_Unmap(&leaf, sizeof(leaf), header->pf_fd, offset);
	}

	return AME_OK;
	
}

offset_t AM_SearchIndex(struct _AMHeader *header, att_val_t value)
{
	offset_t org = header->root_offset;
	int height = header->height;
	int location = 0;
	int *ptr_location = &location;

	while (height > 1) {
		internal_node node;
		index_t *i;
		AM_Map(&node, sizeof(node), header->pf_fd, org);
		i = AM_UpperBound(node.children, 0, node.nr_childrens - 1, value, header->attrType, header->attrLength, ptr_location);
		org = i->child;
		--height;
	}
	
	return org;
}

offset_t AM_SearchLeaf(struct _AMHeader *header, offset_t offset, att_val_t value)
{	
	internal_node node;
	index_t *i;
	int location = 0;
	int *ptr_location = &location;
	AM_Map(&node, sizeof(node), header->pf_fd, offset);
	i = AM_UpperBound(node.children, 0, node.nr_childrens - 1, value, header->attrType, header->attrLength, ptr_location);

	return i->child;
}

int AM_CompAttVal(att_val_t ptr_val1, att_val_t ptr_val2, char attrType, int attrLength)
{
	switch (attrType) {
		case INT_TYPE:
		{
			int val1 = *(int *)ptr_val1;
			int val2 = *(int *)ptr_val2;

			int result = val1 - val2;

			if (result > 0) return 1;
			else if (result == 0) return 0;
			else return -1;
		}
		case REAL_TYPE:
		{
			float val1 = *(float *)ptr_val1;
			float val2 = *(float *)ptr_val2;

			float result = val1 - val2;

			if (result > 0) return 1;
			else if (result == 0) return 0;
			else return -1;
		}
		case STRING_TYPE:
		{
			int result = strncmp(ptr_val1, ptr_val2, attrLength);
			
			if (result > 0) return 1;
			else if (result == 0) return 0;
			else return -1;
			
		}

		default:
			printf("AM_CompAttVal: invalid attrType\n");
			break;
	}
	
	return AME_COMPVAL;
}

index_t *AM_UpperBound(index_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location)
{
	int iter = begin;
	int count = end - begin + 1;
	while (count > 0)
	{
		if (!(AM_CompAttVal(att_val, indices[iter].value, attrType, attrLength) < 0))
		{
			if (end >= iter + 1)
				iter++;
			count--;
		}
		else
			break;
	}

	*location = iter; 
	return &(indices[iter]);
}	

index_t *AM_LowerBound(index_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location)
{
	int iter = begin;
	int count = end - begin + 1;
	while (count > 0)
	{
		if (AM_CompAttVal(indices[iter].value, att_val, attrType, attrLength) < 0)
		{
			if (end >= iter + 1)
				iter++;
			count--;
		}

		else
			break;
	}

	*location = iter;
	return &(indices[iter]);
}
	
record_t *AM_UpperBoundRec(record_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location)
{
	int iter = begin;
	int count = end - begin + 1;
	while (count > 0)
	{
		if (!(AM_CompAttVal(att_val, indices[iter].value, attrType, attrLength) < 0))
		{
			if (end >= iter + 1)
				iter++;
			count--;
		}
		else
			break;
	}
	
	*location = iter;
	return &(indices[iter]);
}	

record_t *AM_LowerBoundRec(record_t *indices, int begin, int end, att_val_t att_val, char attrType, int attrLength, int *location)
{
	int iter = begin;
	int count = end - begin + 1;
	while (count > 0)
	{
		if (AM_CompAttVal(indices[iter].value, att_val, attrType, attrLength) < 0)
		{
			if (end >= iter + 1)
				iter++;
			count--;
		}

		else
			break;
	}

	*location = iter;
	return &(indices[iter]);
}

offset_t AM_AllocInternalNode(struct _AMHeader *header, internal_node *node)
{
	node->nr_childrens = 1;
	header->nr_internal_nodes++;
		
	return AM_MarkUsedPage(header);
}

offset_t AM_AllocLeafNode(struct _AMHeader *header, leaf_node *node)
{
	node->nr_childrens = 0;
	header->nr_leaf_nodes++;

	return AM_MarkUsedPage(header);
}

offset_t AM_MarkUsedPage(struct _AMHeader *header) {
	offset_t free_page_offset;
	char *buf;

	PF_AllocPage(header->pf_fd, &free_page_offset, &buf);

	PF_UnpinPage(header->pf_fd, free_page_offset, 0);

	if (header->free_page_offset != free_page_offset) {
		printf("AM_MarkUsedPage: Inconsistency in header free page offset metadata\n");	
	}

	header->free_page_offset++;

	return free_page_offset;
}

int AM_Unmap(void *data, size_t size, int pf_fd, offset_t page_offset)
{
	char *buf;
	
	if (data == NULL) {
		printf("AM_Unmap: Trying to write null data to page\n");
		return AME_NULL_POINTER;
	}

	/* 1. getting page by offset */
	if (PF_GetThisPage(pf_fd, page_offset, &buf) != PFE_OK) {
		printf("AM_Unmap: fail to get page\n");
		return AME_GET_THIS_PAGE;
	}

	/* 2. write data into buffer */
	memcpy(buf, (char *) data, size);

	/* 3. make the page dirty */
	if (PF_UnpinPage(pf_fd, page_offset, 1) != PFE_OK){
		printf("AM_Unmap: fail to dirty the page\n");
		return AME_DIRTY_PAGE;
	}
	
	return AME_OK;
}

int AM_Map(void *data, size_t size, int pf_fd, offset_t page_offset)
{
	char *buf;
	
	if (data == NULL) {
		printf("AM_map: read buffer is null\n");
		return AME_NULL_POINTER;
	}

	/* 1. getting page by offset */
	if (PF_GetThisPage(pf_fd, page_offset, &buf) != PFE_OK) {
		printf("AM_map: fail to get page\n");
		return AME_GET_THIS_PAGE;
	}

	/* 2. read data from buffer */
	memcpy((char *) data, buf, size);

	/* 3. unpinning the page */
	if (PF_UnpinPage(pf_fd, page_offset, 0) != PFE_OK){
		printf("AM_map: fail to dirty the page\n");
		return AME_UNPIN;
	}	
	
	return AME_OK;
}

void AM_Init(void)
{
	am_index_table = calloc(AM_ITAB_SIZE, sizeof(struct _am_index_entry));
	am_scan_table = calloc(MAXISCANS, sizeof(struct _am_scan_entry));
	HF_Init();
}

int AM_CreateIndex(char *fileName, int indexNo, char attrType,
		int attrLength, bool_t isUnique)
{
	int pf_fd = -1;
	offset_t alloc_page_offset = -1;
	int errNo;

	struct _AMHeader header;
	internal_node root;
	leaf_node leaf;
	char *buf;
	char indexFileName[MAX_VAL_LENGTH];

	sprintf(indexFileName, "%s%d", fileName, indexNo);

	/* If the file is already exist then the PF_CreateFile will return error */
	/* PF Header will be stored into the created file */
	if (PF_CreateFile(indexFileName) != PFE_OK) {
		printf("AM_CreateIndex: PF_CreateFile return error\n");
		return AME_PF_CREATE;
	}

	/* We can allocate a new page as header page by pf layer */
	if ((pf_fd = PF_OpenFile(indexFileName)) < 0) {
		printf("AM_CreateIndex: PF_OpenFile return error\n");
		return AME_PF_OPEN;
	}

	/* Allocate the new page for store header information of HF layer */
	if (PF_AllocPage(pf_fd, &alloc_page_offset, &buf) != PFE_OK) {
		printf("AM_CreateIndex: Failed to allocate new page for header\n");
		return AME_ALLOC_PAGE;
	}

	/* Header page should be the physically number 1 */
	if (alloc_page_offset != AM_HEADER_PAGE_OFFSET) {
		printf("AM_CreateIndex: Allocated page is not for header\n");
		return AME_ALLOC_PAGE;
	}

	if (PF_UnpinPage(pf_fd, alloc_page_offset, 0) != PFE_OK) {
		printf("AM_CreateIndex: PF_UnpinPage return error\n");
		return AME_PF_UNPIN;	
	}

	/* Marking header information */	
	if ((errNo = AM_InitHeader(&header, pf_fd, indexNo, attrType, attrLength)) != AME_OK) {
		return errNo;
	}

	/* Initialize root node */
	root.next = root.prev = root.parent = 0;
	header.root_offset = AM_AllocInternalNode(&header, &root);

	
	/* Initialize empty leaf node */
	leaf.next = leaf.prev = 0;
	leaf.parent = header.root_offset;
	header.leaf_offset = root.children[0].child = AM_AllocLeafNode(&header, &leaf);

	/* Unmap header, root, leaf node */
	AM_Unmap(&header, sizeof(header), pf_fd, AM_HEADER_PAGE_OFFSET);
	AM_Unmap(&root, sizeof(root), pf_fd, header.root_offset);
	AM_Unmap(&leaf, sizeof(leaf), pf_fd, root.children[0].child);

	if (PF_CloseFile(pf_fd) != PFE_OK) {
		printf("AM_CreateIndex: PF_CloseFile return error\n");
		return AME_PF_CLOSE;	
	}
	
	return AME_OK;
}

int AM_InitHeader(struct _AMHeader *header, int pf_fd, int indexNo, char attrType, int attrLength)
{
	if (header == NULL) {
		printf("AM_InitHeader: header is null pointer\n");
		return AME_NULL_POINTER;
	}

	header->pf_fd = pf_fd;
	header->indexNo = indexNo;
	header->height = 1;
	header->nr_internal_nodes = 0;
	header->nr_leaf_nodes = 0;
	header->attrType = attrType;
	header->attrLength = attrLength;
	header->free_page_offset = AM_DATA_PAGE_OFFSET;
	header->root_offset = AM_INVALID_OFFSET;
	header->leaf_offset = AM_INVALID_OFFSET; 

	return AME_OK;
}

int AM_DestroyIndex(char *fileName, int indexNo)
{
	char indexFileName[MAX_VAL_LENGTH];

	sprintf(indexFileName, "%s%d", fileName, indexNo);
	if (PF_DestroyFile(indexFileName) != PFE_OK) {
		return AME_FILE_DESTROY;
	}
	return AME_OK;
}
 
int AM_OpenIndex(char *fileName, int indexNo)
{
	int pf_fd = -1;
	int fileDesc = -1;
	int i = 0;
	struct _am_index_entry *alloc_entry = NULL;
	struct _AMHeader header;
	char indexFileName[MAX_VAL_LENGTH];

	sprintf(indexFileName, "%s%d", fileName, indexNo);

	if ((pf_fd = PF_OpenFile(indexFileName)) < 0) {
		printf("AM_OpenIndex: PF_OpenFile return error\n");
		return AME_PF_OPEN;
	}
	AM_Map(&header, sizeof(header), pf_fd, AM_HEADER_PAGE_OFFSET);

	if (pf_fd != header.pf_fd) {
		header.pf_fd = pf_fd;
		AM_Unmap(&header, sizeof(header), pf_fd, AM_HEADER_PAGE_OFFSET);
	}

	/* Allocate free entry in table and check already opened */
	for (i = 0; i < MAXOPENFILES; i++) {
		
		if (am_index_table[i].valid == 0) {
			alloc_entry = &(am_index_table[i]);
			fileDesc = i;
		} else if (strncmp(am_index_table[i].fileName, indexFileName, strlen(indexFileName)) == 0) {
		
			/* Already the file is opened in HF layer */
			/* TODO: What if the function open the opened file again? */
			
			printf("AME_OpenIndex: The file is already opened in HF layer\n");
			return AME_REOPEN;
		}
	}

	/* No free entry in table */
	if (alloc_entry == NULL) {
		printf("AME_OpenIndex: Try to open the file over maximum number\n");
		return AME_FULLTABLE;
	}
	
	alloc_entry->valid = 1;
	strncpy(alloc_entry->fileName, indexFileName, strlen(indexFileName));
	alloc_entry->indexNo = indexNo;
	alloc_entry->pf_fd = pf_fd;
	alloc_entry->scan_active = 0;

	return fileDesc; 
}

int AM_CloseIndex(int fileDesc)
{
	if (am_index_table[fileDesc].valid == 0) {
		printf("AM_CloseIndex: Try to close Un-opened index\n");
		return AME_FD;	
	}

	/* If the pages in file is pinned then return error */
	if (PF_CloseFile(am_index_table[fileDesc].pf_fd) != PFE_OK) {
		printf("AM_CloseIndex: PF_CloseFile return error\n");
		return AME_PF_CLOSE;
	}

	if (am_index_table[fileDesc].scan_active) {
		printf("AM_CloseIndex: scan is still active\n");
		return AME_SCANOPEN;
	}

	am_index_table[fileDesc].valid = 0;
	return AME_OK;
}

int  AM_InsertEntry(int fileDesc, char *value, RECID recId)
{
	struct _am_index_entry *index_entry = &am_index_table[fileDesc];
	struct _AMHeader header;	
	int pf_fd = -1;

	if (index_entry->valid == 0) {
		printf("AM_InsertEntry: Wrong fileDesc value\n");
		return AME_FD;
	}

	pf_fd = index_entry->pf_fd;
	AM_Map(&header, sizeof(header), pf_fd, AM_HEADER_PAGE_OFFSET);
	AM_Insert(&header, value, recId);

	return AME_OK;
}

int  AM_DeleteEntry(int fileDesc, char *value, RECID recId)
{
	struct _am_index_entry *index_entry = &am_index_table[fileDesc];
	struct _AMHeader header;
	int pf_fd = -1;	

	if (index_entry->valid == 0) {
		printf("AM_InsertEntry: Wrong fileDesc value\n");
		return AME_FD;
	}

	pf_fd = index_entry->pf_fd;

	AM_Map(&header, sizeof(header), pf_fd, AM_HEADER_PAGE_OFFSET);

	
	return AM_Delete(&header, value, recId);
}

int  AM_OpenIndexScan(int fileDesc, int op, char *value)
{
	struct _am_index_entry *index_entry = &am_index_table[fileDesc];
	struct _am_scan_entry *alloc_entry;
	int scanDesc = -1;
	int i = 0;

	if (!index_entry->valid) {
		printf("AM_OpenIndexScan: Wrong fileDesc value\n");
		return AME_FD;
	}

	for (i = 0; i < MAXISCANS; i++) {
		if (am_scan_table[i].valid == 0) {
			alloc_entry = &(am_scan_table[i]);
			scanDesc = i;
		}
	}

	/* no free entry in table */
	if (alloc_entry == NULL) {
		printf("AM_OpenIndexScan: Try to open index scan over maximum number\n");
		return AME_SCANTABLEFULL;
	}

	index_entry->scan_active = 1;

	strncpy(alloc_entry->fileName, index_entry->fileName, strlen(index_entry->fileName));
	alloc_entry->pf_fd = index_entry->pf_fd;
	alloc_entry->indexNo = index_entry->indexNo;
	alloc_entry->fileDesc = fileDesc;
	alloc_entry->scanDesc = scanDesc;
	alloc_entry->op = op;
	memcpy(alloc_entry->value, value, strlen(value));


	alloc_entry->leaf_offset = -1;
	alloc_entry->entry_offset = -1;
	alloc_entry->org_leaf_offset = -1;
	alloc_entry->org_entry_offset = -1;
	alloc_entry->direction = NOTDEFINED;
	alloc_entry->valid = 1;

	return scanDesc;
}

RECID AM_FindNextEntry(int scanDesc)
{
	struct _AMHeader header;
	struct _am_scan_entry *scan_entry = &am_scan_table[scanDesc];

	int *org_leaf_offset = &(scan_entry->org_leaf_offset);
	int *org_entry_offset = &(scan_entry->org_entry_offset);
	int *leaf_offset = &(scan_entry->leaf_offset);
	int *entry_offset = &(scan_entry->entry_offset);
	int *direction = &(scan_entry->direction);

	int op = scan_entry->op;

	record_t find_record;
	RECID invalid_recId;
	invalid_recId.pagenum = 999999;
	invalid_recId.recnum = 999999;

	if (scan_entry->valid == 0) {
		printf("AM_FindNextEntry: Wrong scanDesc value\n");
		AMerrno = AME_FD;
		return invalid_recId;
	}

	AM_Map(&header, sizeof(header), scan_entry->pf_fd, AM_HEADER_PAGE_OFFSET);

	if (*direction == NOTDEFINED) {
		if (!AM_GetFirstEntry(&header, scan_entry->value, op, leaf_offset, entry_offset, &find_record)) {
			*org_leaf_offset = *leaf_offset;
			*org_entry_offset = *entry_offset;
			*direction = LEFT;
			return AM_FindNextEntry(scanDesc);
		}

		*org_leaf_offset = *leaf_offset;
		*org_entry_offset = *entry_offset;
		*direction = LEFT;
		
		if (find_record.deleted == FALSE)
			return find_record.recId;

		else
			return AM_FindNextEntry(scanDesc);
	}

	else if (*direction == LEFT) {
		if (!AM_GetNextEntry(&header, scan_entry->value, op, LEFT, leaf_offset, entry_offset, &find_record)) {
			*direction = RIGHT;
			*leaf_offset = *org_leaf_offset;
			*entry_offset = *org_entry_offset;
			return AM_FindNextEntry(scanDesc);
		}

		if (find_record.deleted == FALSE)
			return find_record.recId;

		else
			return AM_FindNextEntry(scanDesc);
	}

	else if (*direction == RIGHT) {
		if (!AM_GetNextEntry(&header, scan_entry->value, op, RIGHT, leaf_offset, entry_offset, &find_record)) {
			AMerrno = AME_EOF;
			return invalid_recId;
		}

		if (find_record.deleted == FALSE)
			return find_record.recId;

		else
			return AM_FindNextEntry(scanDesc);
	}

	return invalid_recId;
}

int  AM_CloseIndexScan(int scanDesc)
{
	struct _am_scan_entry *scan_entry = &am_scan_table[scanDesc];
	
	am_index_table[scan_entry->fileDesc].scan_active = 0;
	scan_entry->valid = 0;
	
	return AME_OK;
}

void AM_PrintError(char *errString)
{
		printf("<%d>%s\n", AMerrno, errString);
}
