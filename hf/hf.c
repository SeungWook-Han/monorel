#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "minirel.h"
#include "pf.h"
#include "hf.h"

#define HF_HEADER_PAGE_NUM 0
#define HF_DATA_PAGE_OFFSET 1

#define HF_FREE_PAGE_LIST 1
#define HF_FULL_PAGE_LIST 0


/*
 * |--------------------------------------------------------|
 *  [data]     	    pf header  | hf_header  | data page ...
 *  [phy page]       (page 0 ) | (page 1 )  | (page 2)  ...
 *  [logical page]   (   x   ) | (   x   )  | (page 0)  ...
 * |--------------------------------------------------------|
 *
 */

/* Table getting header info of the files */
struct hf_table_entry *hf_table;
struct hf_scan_entry *hf_scan_table;

bool_t HF_SearchTable(char *filename, int *ret_fd)
{
	int i = 0;
	
	if (ret_fd == NULL) 
		return FALSE;
	
	for (i = 0; i < MAXOPENFILES; i++) {
		if (hf_table[i].valid == 1 &&
		    !strncmp(filename, &hf_table[i].fname, strlen(filename))) {
			*ret_fd = i;
			return TRUE;
		}
	}

	return FALSE;
}

void HF_Init(void)
{
	hf_table = calloc(MAXOPENFILES, sizeof(struct hf_table_entry));
	hf_scan_table = calloc(MAXOPENFILES, sizeof(struct hf_scan_entry));
	PF_Init();
}

int HF_CreateFile(char *fileName, int recSize)
{
	int pf_fd = -1;
	int alloc_page_num = -1;
	char *buf;
	struct HFHeader header;

	/* If the file is already exist then the PF_CreateFile will return error */
	/* PF Header will be stored into the created file */
	if (PF_CreateFile(filename) != PFE_OK) {
		printf("HF_CreateFile: PF_CreateFile return error\n");
		return HFE_PF_CREATE;
	}

	/* We can allocate a new page as header page by pf layer */
	if ((pf_fd = PF_OpenFile(filename)) < 0) {
		printf("HF_CreateFile: PF_OpenFile return error\n");
		return HFE_PF_OPEN;
	}

	/* Allocate the new page for store header information of HF layer */
	if (PF_AllocPage(pf_fd, &alloc_page_num, &buf) != PFE_OK) {
		printf("HF_CreateFile: Failed to allocate new page for header\n");
		return HFE_ALLOC_PAGE;
	}

	/* Header page should be the physically number 1 */
	if (alloc_page_num != HF_HEADER_PAGE_NUM) {
		printf("HF_CreateFile: Allocated page is not for header\n");
		return HFE_ALLOC_PAGE;
	}

	/* Making header information to store into the created file */
	header.rec_size = recSize;
	header.nr_rec = (PAGE_SIZE - PAGE_FORMAT_SIZE) / recSize;
	header.nr_pages = 0;
	header.free_page_list = 0;
	header.full_page_list = 0;

	/* Write the header info into file */
	if (HF_HeaderWrite(&header, pf_fd) != HFE_OK) {
		printf("HF_CreateFile: Header write is failed\n");
		return HFE_HEADER_WRITE;
	}

	/* Unpinning the header page for flush */
	if (PF_UnpinPage(pf_fd, HF_HEADER_PAGE_NUM, 1) != PFE_OK) {
		printf("HF_CreateFile: PF_UnpinPage return error\n");
		return HFE_PF_UNPIN;
	}

	/* Close the created file by pf */
	if (PF_CloseFile(pf_fd) != PFE_OK) {
		printf("HF_CreateFile: PF_CloseFile return error\n");
		return HFE_PF_CLOSE;
	}

	return HFE_OK;
}

int HF_DestroyFile(char *fileName)
{
	/* TODO: We need to check whethere the file was closed here ?*/
	if (PF_DestroyFile(fileName) != PFE_OK) {
		return HFE_FILE_DESTROY;
	}
	return HFE_OK;
}

int HF_OpenFile(char *fileName)
{
	int pf_fd = -1;
	int hf_fd = -1;
	struct hf_table_entry *alloc_entry = NULL;

	if ((pf_fd = PF_OpenFile(filename)) < 0) {
		printf("HF_OpenFile: PF_OpenFile return error\n");
		return HFE_PF_OPEN;
	}

	/* Allocate free entry in table and check already opened */
	for (hf_fd = 0; hf_fd < MAXOPENFILES; hf_fd++) {
		
		if (hf_table[hf_fd].valid == 0) {
			alloc_entry = &(hf_table[hf_fd]);
		
		} else if (strncmp(&hf_table[hf_fd].fname, fileName, strlen(fileName)) == 0) {
		
			/* Already the file is opened in HF layer */
			/* TODO: What if the function open the opened file again? */
			
			printf("HF_OpenFile: The file is already opened in HF layer\n");
			return HFE_REOPEN;
		}
	}

	/* No free entry in table */
	if (alloc_entry == NULL) {
		printf("HF_OpenFile: Try to open the file over maximum number\n");
		return HFE_FTABFULL;
	}

	/* Initialize the table entry data */
	alloc_entry->valid = 1;
	alloc_entry->scan_active = 0;
	strncpy(&alloc_entry->fname, fileName, strlen(fileName));
	alloc_entry->pf_fd = pf_fd;
	if (HF_HeaderRead(&alloc_entry->header, pf_fd) != HFE_OK) {
		printf("HF OpenFile: Fail to read header info from file\n");
		return HFE_HEADER_READ;
	}

	return hf_fd;
}

int HF_CloseFile(int HFfd)
{
	if (hf_table[HFfd].valid == 0) {
		printf("HF_CloseFile: Try to close Un-opened filed\n");
		return HFE_FD;	
	}

	/* If the pages in file is pinned then return error */
	if (PF_CloseFile(hf_table[HFfd].pf_fd) != PFE_OK) {
		printf("HF_CloseFile: PF_CloseFile return error\n");
		return HFE_PF_CLOSE;
	}

	if (hf_table[HFfd].scan_active) {
		printf("HF_CloseFile: scan is still active\n");
		return HFE_SCANOPEN;
	}

	hf_table[HFfd].valid = 0;
	return HFE_OK;
}

RECID HF_InsertRec(int HFfd, char *record)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	char *pagebuf;
	int pagenum = 0;
	int idx_bitmap = 0;
	struct page_format format;
	RECID record_id;

	/* 1. Checking whether that we have free page */
	if (table_entry->valid == 0) {
		printf("HF_InsertRec: Wrong HFfd value\n");
		return HFE_FD;
	}

	if (table_entry->header.free_page_list == 0) {
		/* Allocating new data page */
		if (PF_AllocPage(table_entry->pf_fd, 
				 &(table_entry->header.free_page_list), 
			 	 &pagebuf) != PFE_OK) {
			printf("HF_InsertRec: Failed to allocate new page\n");
			return HFE_ALLOC_PAGE;
		}
		
		(table_entry->nr_pages)++;
		HF_InitList(pagebuf);
	}

	/* 2. Get the page having free space from the list */
	if (PF_GetThisPage(table_entry->pf_fd, 
			   table_entry->header.free_page_list, 
			   &pagebuf) != PFE_OK) {
		printf("HF_InsertRec: PF_GetThisPage error\n");
		return HFE_GET_THIS_PAGE;
	}

	pagenum = table_entry->header.free_page_list;
	HF_ReadPageFormat(&format, pagebuf);

	if (format.nr_rec == table_entry->header.nr_rec) {
		printf("HF_InsertRec: Inconsistency in free page list and nr_rec\n");
		return -1;
	}

	/* 3. Finding the free slot using page fromat */
	for (idx_bitmap = 0; idx_bitmap < BITMAP_SIZE; idx_bitmap++) {
		if (!bitmap_get(format.bitmap, BITMAP_SIZE, idx_bitmap)) {
			break;
		}
	}

	if (idx_bitmap == BITMAP_SIZE) {
		printf("HF_InsertRec: Inconsisteny in bitmap and nr_rec\n");
		return -1;
	}

	/* 4. Store record data and update page format */
	memcpy(pagebuf + (idx_bitmap * (table_entry->header.rec_size)),
		record, table_entry->header.rec_size);

	bitmap_set(format.bitmap, BITMAP_SIZE, idx_bitmap);
	(format.nr_rec)++;
	HF_WritePageFormat(&format, pagebuf);

	/* 5. Checking whether the page have free space */
	if (format.nr_rec == table_entry->header.nr_rec) {
		
		HF_DeletePageList(table_entry, &format, 
				  pagenum, HF_FREE_PAGE_LIST);
		HF_InsertPageList(table_entry, &format,
				  pagenum, HF_FULL_PAGE_LIST);
	}

	/* 6. Return record id */
	record_id.pagenum = pagenum;
	record_id.recnum  = idx_bitmap;
	
	return record_id;
}

int HF_DeleteRec(int HFfd, RECID recId)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	char *pagebuf;
	struct page_format format;

	if (table_entry->valid == 0) {
		printf("HF_DeleteRec: Wrong HFfd value\n");
		return HFE_FD;
	}

	PF_GetThisPage(table_entry->pf_fd, recId.pagenum, &pagebuf);
	HF_ReadPageFormat(&format, pagebuf);

	if (!bitmap_get(format.bitmap, BITMAP_SIZE, recId.recnum)) {
		printf("HF_DeleteRec: Try to erase empty data\n");
		return -1;
	}

	bitmap_clear(format.bitmap, BITMAP_SIZE, recId.recnum);
	(format.nr_rec)--;
	HF_WritePageFormat(&format, pagebur);

	if (format.nr_rec + 1 == table_entry->header.nr_rec) {
		HF_DeletePageList(table_entry, &format,
				  recId.pagenum, HF_FULL_PAGE_LIST);
		HF_InsertPageList(table_entry, &format,
				  recId.pagenum, HF_FREE_PAGE_LIST);
	}

	return HFE_OK;
}

RECID HF_GetFirstRec(int HFfd, char *record)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	int nr_pages = table_entry->header.nr_pages;
	int i = 0, j = 0;
	int pagenum;
	char *pagebuf;
	struct page_format format;
	int find = 0;
	RECID rec_id;

	if (nr_pages == 0) {
		return HFE_EOF;
	}

	for (i = 0; i < nr_pages; i++) {
		pagenum = i + HF_DATA_PAGE_OFFSET;
		PF_GetThisPage(table_entry->pf_fd, pagenum, &pagebuf);
		HF_ReadPageFormat(&format, pagebuf);

		for (j = 0; j < table_entry->nr_rec; j++) {
			if (bitmap_get(format.bitmap, BITMAP_SIZE, j)) {
				find = 1;
				break;
			}
		}
		if (find == 1) break;
	}

	if (find == 0) return HFE_EOF;

	memcpy(record, format.bitmap + (j * table_entry->header.rec_size),
	       table_entry->header.rec_size);
	rec_id.pagenum = i;
	rec_id.recnum  = j;

	return rec_id;
}


RECID HF_GetNextRec(int HFfd, RECID recId, char *record)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	int ret = 0;
	int i = 0, j = 0;
	int nr_pages = table_entry->header.nr_pages;
	int pagenum = 0;
	char *pagebuf;
	struct page_format format;
	int find = 0;
	RECID rec_id;

	if (!table_entry->valid) {
		printf("HF_GetNextRec: Wrong HFfd value\n");
		return HFE_FD;
	}

	if ((ret = HF_GetThisRec(HFfd, recId, record)) != HFE_OK) {
		printf("HF_GetNextRec: HF_GetThisRec return error\n");
		return ret;
	}

	for (i = recId.pagenum; i < nr_pages; i++) {
		pagenum = i + HF_DATA_PAGE_OFFSET;
		PF_GetThisPage(table_entry->pf_fd, pagenum, &pagebuf);
		HF_ReadPageFormat(&format, pagebuf);
		
		for (j = recId.recnum + 1; j < table_entry->nr_rec; j++) {
			if (bitmap_get(format.bitmap, BITMAP_SIZE, j)) {
				find = 1;
				break;
			}
		}

		if (find == 1) break;
	}

	if (find == 0) return HFE_EOF;

	memcpy(record, format.bitmap + (j * table_entry->header.rec_size),
			table_entry->header.rec_size);

	rec_id.pagenum = i;
	rec_id.recnum  = j;

	return HFE_OK;
}

int HF_GetThisRec(int HFfd, RECID recId, char *record)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	char *pagebuf;
	struct page_format format;
	int ret = 0;
	int pagenum = recId.pagenum + HF_DATA_PAGE_OFFSET;

	if (!table_entry->valid) {
		printf("HF_GetThisRec: Wrong HFfd value\n");
		return HFE_FD;
	}

	ret = PF_GetThisPage(table_entry->pf_fd, pagenum, &pagebuf);
	if (ret == PFE_INVALIDPAGE) {
		printf("HF_GetThisRec: Wrong Recid \n");
		return HFE_INVALIDRECORD;
	}

	if (ret != PFE_OK) {
		printf("HF_GetThisRec: PF_GetThisPage return error\n");
		return HFE_PF;
	}

	HF_ReadPageFormat(&format, pagebuf);
	if (!bitmap_get(format.bitmap, BITMAP_SIZE, recId.recnum)) {
		printf("HF_GetThisRec: Wrong Recid \n");
		return HFE_INVALIDRECORD;
	}

	memcpy(record, format.bitmap + (recId.recnum * table_entry->header.rec_size),
			table_entry->header.rec_size);
	return HFE_OK;
}

int HF_OpenFileScan(int HFfd, char attrType, int attrLength,
		    int attrOffset, int op, char *value)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	struct hf_scan_entry *scan_entry;
	int scan_idx = 0;

	if (!table_entry->valid) {
		printf("HF_OpenFileScan: Wrong HFfd value\n");
		return HFE_FD;
	}

	for (scan_idx = 0; i < MAXOPENFILES; i++) {
		if (!hf_scan_table[scan_idx].valid) {
			break;
		}
	}

	if (scan_idx == MAXOPENFILES) {
		printf("HF_OpenFileScan: Scan table is full\n");
		return -1;
	}

	scan_entry = &hf_scan_table[scan_idx];
	scan_entry.valid = 1;
	scan_entry.hf_fd = HFfd;
	scan_entry.attrType = attrType;
	scan_entry.attrLength = attrLength;
	scan_entry.attrOffset = attrOffset;
	scan_entry.op = op;

	table_entry->scan_active = 1;

	if (value == NULL) {
		scan_entry.value = NULL;
		scan_entry.op = ALL_OP;
	} else {
		scan_entry.value = malloc(attrLength);
		memcpy(scan_entry.value, value, attrLengh);
	}
	
	scan_entry.rec_id.pagenum = -1;
	scan_entry.rec_id.recnum  = -1;

	return scan_idx;
}

RECID HF_FindNextRec(int scanDesc, char *record)
{
	struct hf_scan_entry *scan_entry = &hf_scan_table[scanDesc];
	RECID rec_id;
	
	if (!scan_entry->valid) {
		printf("HF_FindNextRec: Wrong Scan idx\n");
		return HFE_SCANNOTOPEN;
	}

	if (scan_entry->rec_id.pagenum == -1) {
		rec_id = HF_GetFirstRec(scan_entry->hf_fd, record);
	} else {
		rec_id = HF_GetNextRec(scan_entry->hf_fd, scan_entry->rec_id, record);
	}

	if (rec_id == HFE_EOF) {
		printf("HF_FindNextRec: End of file\n");
		return HFE_EOF;
	}

	
	if (scan_entry->op != ALL_OP) {
		do {
			if (HF_CheckCondition(rec_id, record, scan_entry)) {
				break;
			}
			rec_id = HF_GetNextRec(scan_entry->hf_fd, scan_entry->rec_id, recrod);

		} while (rec_id != HFE_EOF);
	}
	
	if (rec_id == HFE_EOF) {
		printf("HF_FindNextRec: End of file\n");
		return HFE_EOF;
	}

	scan_entry->rec_id = rec_id;
	return rec_id;
}

int HF_CloseFileScan(int scanDesc)
{
	hf_table[hf_scan_table[scanDesc].hf_fd].scan_active = 0;
	free(hf_scan_table[scanDesc].value);
	memset(&hf_scan_table[scanDesc], 0, sizeof(struct hf_scan_entry));
	return HFE_OK;
}

bool_t HF_ValidRecId(int HFfd, RECID recid)
{
	struct hf_table_entry *table_entry = &hf_table[HFfd];
	return (recid.pagenum < table_entry->header.nr_pages) &&
			(recid.recnum < table_entry->header.nr_rec) ? TRUE : FALSE;
}

int HF_CheckCondition(RECID rec_id, char *record, struct hf_scan_entry *scan_entry)
{
	record += scan_entry->attrOffset;
	switch (scan_entry->attrType) {
		case INT_TYPE: 
		{
			int attribute = *(int *)record;
			int value = *(int *)scan_entry->value;
			int result = attribute - value;
			int ret = 0;
			switch (scan_entry->op) {
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
			float attribute = *(float *)record;
			float value = *(float *)scan_entry->value;
			float result = attribute - value;
			int ret = 0;
			switch (scan_entry->op) {
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
			int result = strncmp(record, scan_entry->value,
						scan_entry->attrLength);
			int ret = 0;
			switch (scan_entry->op) {
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
			printf("HF_CheckCondition: attrType Error\n");
			break;
	}

	return -1;
}


int HF_DeletePageList(struct hf_table_entry *table_entry, 
		      struct page_format *format,
		      int pagenum, int is_free_page_list)
{
	int *list = is_free_page_list ? &(table_entry->free_page_list) :
					&(table_entry->full_page_list) ;
	int candidate = pagenum;

	/* Checking whethere that I'm the only one in list */
	if (format->next == pagenum && format->prev == pagenum) {
		*list = HF_HEADER_PAGE_NUM;
	} else {
		/* Do normal list delete */	
		char *prev_pagebuf, *next_pagebuf;
		struct page_format prev_format, next_format;

		PF_GetThisPage(table_entry->pf_fd, format->prev, &prev_pagebuf);
		PF_GetThisPage(table_entry->pf_fd, format->next, &next_pagebuf);
	
		HF_ReadPageFormat(&prev_format, prev_pagebuf);
		HF_ReadPageFormat(&next_format, next_pagebuf);

		if (*list == pagenum) {
			candidate = format->next;
		}

		prev_format.next = format->next;
		next_format.prev = format->prev;

		HF_WritePageFormat(&prev_format, prev_pagebuf);
		HF_WritePageFormat(&next_format, next_pagebuf);

		*list = candidate;
	}

	return 0;
}

int HF_InsertPageList(struct hf_table_entry *table_entry, 
		      struct page_format *format,
		      int pagenum, int is_free_page_list)
{
	int *list = is_free_page_list ? &(table_entry->free_page_list) :
					&(table_entry->full_page_list) ;
	
	/* Checking whethere that there is no entry in list */
	if (*list == HF_HEADER_PAGE_NUM) {
		char *pagebuf;
		
		PF_GetThisPage(table_entry->pf_fd, pagenum, &pagebuf);
		HF_InitList(pagebuf);
		*list = pagenum;

	} else {
		char *next_pagebuf, *pagebuf, *prev_pagebuf;
		struct next_format, prev_format;

		PF_GetThisPage(table_entry->pf_fd, *list, &prev_pagebuf);
		HF_ReadPageFormat(&prev_format, prev_pagebuf);

		PF_GetThisPage(table_entry->pf_fd, prev_format->next, &next_pagebuf);
		HF_ReadPageFormat(&next_format, next_pagebuf);

		PF_GetThisPage(table_entry->pf_fd, pagenum, &pagebuf);

		format->next = prev_format->next;
		format->prev = *list;

		prev_format->next = pagenum;
		next_format->prev = pagenum;

		HF_WritePageFormat(format, pagebuf);
		HF_WritePageFormat(&prev_format, prev_pagebuf);
		HF_WritePageFormat(&next_format, next_pagebuf);
	}

	return 0;
}

int HF_ReadPageFormat(struct page_format *format, char *pagebuf)
{
	memcpy(format, pagebuf + PAGE_FORMAT_OFFSET, sizeof(int) * 3);
	format->bitmap = pagebuf + PAGE_FORMAT_BITMAP_OFFSET;

	return 0;
}

int HF_WritePageFormat(struct page_format *format, char *pagebuf)
{
	memcpy(pagebuf, format, sizeof(int) * 3);
/*	memcpy(pagebuf + PAGE_FORMAT_BITMAP_OFFSET, 
			format->bitmap, BITMAP_SIZE / 8);
*/
	return 0;
}

int HF_InitList(char *pagebuf)
{
	struct page_format init_format;
	init_format.next = init_format.prev = 1;
	memcpy(pagebuf + PAGE_FORMAT_OFFSET, &init_format, sizeof(int) * 2);

	return 0;
}

int HF_HeaderRead(struct HFHeader *header, int pf_fd)
{
	char *buf;
	
	if (header == NULL) {
		printf("HF_HeaderRead: Read Buffer is empty\n");
		return HFE_NULL_POINTER;
	}

	/* 1. Getting HF_Header page */
	if (PF_GetThisPage(pf_fd, HF_HEADER_PAGE_NUM, &buf) != PFE_OK) {
		printf("HF_HeaderRead: fail to get header page\n");
		return HFE_GET_THIS_PAGE;
	}

	/* 2. Read the header data on buffer */
	memcpy(header, buf, sizeof(struct HFHeader));

	return HFE_OK;
}

int HF_HeaderWrite(struct HFHeader *header, int pf_fd)
{
	char *buf;
	
	if (header == NULL) {
		printf("HF_HeaderWRite: Trying to write null header\n");
		return HFE_NULL_POINTER;
	}
	
	/* 1. Getting HF header page */
	if (PF_GetThisPage(pf_fd, HF_HEADER_PAGE_NUM, &buf) != PFE_OK) {
		printf("HF_HeaderWrite: fail to get header page\n");
		return HFE_GET_THIS_PAGE;
	}

	/* 2. Write the header data into buffer */
	memcpy(buf, header, sizeof(struct HFHeader));

	/* 3. Make the page dirty */
	if (PF_DirtyPage(pf_fd, HF_HEADER_PAGE_NUM) != PFE_OK) {
		printf("HF_HeaderWrite: fail to dirty the page\n");
		return HFE_DIRTY_PAGE;
	}

	return HFE_OK;
}

