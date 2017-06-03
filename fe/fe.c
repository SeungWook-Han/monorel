#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <unistd.h>
#include <string.h>
#include "fe.h"
#include "hf.h"
#include "am.h"
#include "minirel.h"
#include "catalog.h"

#define SEARCH_TMP_FILE_MARK "_s_tmp"
#define JOIN_TMP_FILE_MARK "_j_tmp"

int Search_RelCatalog(char *relName, struct _relation_desc *rel_desc, RECID *recid);
int Search_AttrCatalog(char *relName, int attrcnt, ATTRDESCTYPE *attr_list, RECID *recid);

char data_dir[STR_SIZE];
int relcatFd, attrcatFd;
int FEerrno;

/* Checking the file is exist in present directory 
 * @return: >= 0 (File exist)
 * 	    <  0 (Not exist)
 */
int isFileExist(char *name)
{
	int ret = -1;
	if ((ret = open(name, O_RDONLY)) >= 0)
		close(ret);
	return ret < 0 ? 0 : 1;
}

void FE_Init(void)
{
	if (!getcwd(data_dir, STR_SIZE)) {
		printf("[FE_Init] Fail to get pwd\n");
		FEerrno = FEE_INIT;
		return ;
	}

	relcatFd = attrcatFd = -1;
	
	AM_Init();
	return ;
}

/*
 * Database Utility Functions
 */
void DBcreate(char *dbname)
{
	int ret = -1;
	char db_location[STR_SIZE];
	int i = 0;
	ATTR_DESCR *attr_desc_list;
	ATTR_DESCR *attr_cata_list;

	if (isFileExist(dbname)) {
		printf("%s\n", dbname);
		printf("[DBcreate] dbname database is already exist\n");
		FEerrno = FEE_EXISTDATABASE;
		return ;
	}

	if ((ret = mkdir(dbname, 0777)) < 0) {
		printf("[DBCreate] failed to create database directory\n");
		FEerrno = FEE_EXISTDATABASE;
		return ;
	}

	sprintf(db_location, "%s/%s", data_dir, dbname);	
	if (chdir(db_location)) {
		printf("%s\n", db_location);
		printf("[DBCreate] failed to change pwd\n");
		FEerrno = FEE_FAILCD;
		return ;
	}

	if (HF_CreateFile(RELCATNAME, RELDESCSIZE)) {
		printf("[DBCreate] failed to create relcat file\n");
		FEerrno = FEE_HF;
		return ;
	}

	if (HF_CreateFile(ATTRCATNAME, ATTRDESCSIZE) != 0) {
		printf("[DBCreate] failed to create attribute file\n");
		FEerrno = FEE_HF;
		return ;
	}

	if ((relcatFd = HF_OpenFile(RELCATNAME)) < 0) {
		printf("[DBCreate] failed to open relcat\n");
		FEerrno = FEE_HF;
		return ;
	}

	if ((attrcatFd = HF_OpenFile(ATTRCATNAME)) < 0) {
		printf("[DBCreate] failed to open attrcat\n");
		FEerrno = FEE_HF;
		return ;
	}

	/* Relation table (Attribute name will be omitted) */
	attr_desc_list = calloc(RELCAT_NATTRS, sizeof(ATTR_DESCR));
	for (i = 0; i < RELCAT_NATTRS; i++) 
		attr_desc_list[i].attrName = malloc(MAXNAME);
	
	attr_desc_list[0].attrType = STRING_TYPE; attr_desc_list[0].attrLen = MAXNAME;
	sprintf(attr_desc_list[0].attrName, "%s", "relname");
	
	attr_desc_list[1].attrType = INT_TYPE;    attr_desc_list[1].attrLen = sizeof(int);
	sprintf(attr_desc_list[1].attrName, "%s", "relwid");
	
	attr_desc_list[2].attrType = INT_TYPE;    attr_desc_list[2].attrLen = sizeof(int);
	sprintf(attr_desc_list[2].attrName, "%s", "attrcnt");
	
	attr_desc_list[3].attrType = INT_TYPE;    attr_desc_list[3].attrLen = sizeof(int);
	sprintf(attr_desc_list[3].attrName, "%s", "indexcnt");
	
	attr_desc_list[4].attrType = STRING_TYPE; attr_desc_list[4].attrLen = MAXNAME;
	sprintf(attr_desc_list[4].attrName, "%s", "primattr");
	
	_CreateTable(RELCATNAME, RELCAT_NATTRS, attr_desc_list, NULL, RELDESCSIZE);
		
	/* Attribute table */
	attr_cata_list = calloc(ATTRCAT_NATTRS, sizeof(ATTR_DESCR));
	for (i = 0; i < ATTRCAT_NATTRS; i++) 
		attr_cata_list[i].attrName = malloc(MAXNAME);

	attr_cata_list[0].attrType = STRING_TYPE; attr_cata_list[0].attrLen = MAXNAME;
	sprintf(attr_cata_list[0].attrName, "%s", "relname");
	
	attr_cata_list[1].attrType = STRING_TYPE; attr_cata_list[1].attrLen = MAXNAME;
	sprintf(attr_cata_list[1].attrName, "%s", "attrname");
	
	attr_cata_list[2].attrType = INT_TYPE; 	  attr_cata_list[2].attrLen = sizeof(int);
	sprintf(attr_cata_list[2].attrName, "%s", "offset");
	
	attr_cata_list[3].attrType = INT_TYPE;    attr_cata_list[3].attrLen = sizeof(int);
	sprintf(attr_cata_list[3].attrName, "%s", "attrlen");
	
	attr_cata_list[4].attrType = TYPE_TYPE;    attr_cata_list[4].attrLen = sizeof(int);
	sprintf(attr_cata_list[4].attrName, "%s", "attrtype");
	
	attr_cata_list[5].attrType = BOOL_TYPE;    attr_cata_list[5].attrLen = sizeof(int);
	sprintf(attr_cata_list[5].attrName, "%s", "indexed");
	
	attr_cata_list[6].attrType = INT_TYPE;    attr_cata_list[6].attrLen = sizeof(int);
	sprintf(attr_cata_list[6].attrName, "%s", "attrno");
	
	_CreateTable(ATTRCATNAME, ATTRCAT_NATTRS, attr_cata_list, NULL, ATTRDESCSIZE);
	
	for (i = 0; i < RELCAT_NATTRS; i++) 
		free(attr_desc_list[i].attrName);
	free(attr_desc_list);

	for (i = 0; i < ATTRCAT_NATTRS; i++) 
		free(attr_cata_list[i].attrName);
	
	free(attr_cata_list);

	if (HF_CloseFile(relcatFd)) {
		printf("[DBCreate] failed to close relcat\n");
		FEerrno = FEE_HF;
		return ;
	}

	if (HF_CloseFile(attrcatFd)) {
		printf("[DBCreate] failed to close relcat\n");
		FEerrno = FEE_HF;
		return ;
	}

	if (chdir(data_dir)) {
		printf("[DBCreate] failed to change pwd\n");
		FEerrno = FEE_FAILCD;
		return ;
	}

	relcatFd = attrcatFd = -1;
}

void DBdestroy(char *dbname)
{
	char db_location[STR_SIZE];
	char file_name[STR_SIZE];
	int ret = 0;
	sprintf(db_location, "%s/%s", data_dir, dbname);
	if(isFileExist(db_location)) {
		DIR *dir;
		struct dirent *ent;
		dir = opendir(db_location);
		while ((ent = readdir(dir)) != NULL) {
			if (strcmp(ent->d_name, "..") && strcmp(ent->d_name, ".")) {
				sprintf(file_name, "%s/%s", db_location, ent->d_name);
				unlink(file_name);
			}
		}
		closedir(dir);
		rmdir(db_location);
	}
}

void DBconnect(char *dbname)
{
	char db_location[STR_SIZE];
	int ret = -1;
	
	sprintf(db_location, "%s/%s", data_dir, dbname);
	
	if ((ret = isFileExist(db_location)) < 0) {
		printf("[DBconnect] Trying to connect un-exist database\n");
		FEerrno = FEE_NONEXISTDATABASE;
		return ;
	}
	
	if (chdir(db_location)) {
		printf("[DBConnect] failed to change pwd\n");
		FEerrno = FEE_FAILCD;
		return ;
	}

	relcatFd = HF_OpenFile(RELCATNAME);
	attrcatFd = HF_OpenFile(ATTRCATNAME);
}

void DBclose(char *dbname)
{
	if (chdir(data_dir)) {
		printf("[DBConnect] failed to change pwd\n");
		FEerrno = FEE_FAILCD;
		return ;
	}

	/* Closing attribute table and relation table? */
	HF_CloseFile(relcatFd);
	HF_CloseFile(attrcatFd);
	relcatFd = attrcatFd = -1;
}

int _CreateTable(char *relName,
		 int numAttrs,
		 ATTR_DESCR attrs[],
		 char *primAttrName,
		 int recSize)
{
	struct _relation_desc rel_desc;
	struct _attribute_desc attr_desc;
	int i = 0;
	int offset = 0;
	
	memcpy(rel_desc.relname, relName, MAXNAME);
	rel_desc.relwid = recSize;
	rel_desc.attrcnt = numAttrs;
	rel_desc.indexcnt = 0;
	sprintf(rel_desc.primattr, "%s", attrs[0].attrName);
	HF_InsertRec(relcatFd, (char*)(&rel_desc));

	for (i = 0; i < numAttrs; i++) {
		memcpy(attr_desc.relname, relName, MAXNAME);
		memcpy(attr_desc.attrname, attrs[i].attrName, MAXNAME);
		attr_desc.offset = offset;
		offset += attrs[i].attrLen;
		attr_desc.attrlen = attrs[i].attrLen;
		attr_desc.attrtype = attrs[i].attrType;
		attr_desc.indexed = FALSE;
		attr_desc.attrno = i;
		HF_InsertRec(attrcatFd, (char*)(&attr_desc));
	}

	return FEE_OK;
}


/*
 * DDL Functions
 */
int CreateTable(char *relName,		/* name	of relation to create	   */
		int numAttrs,		/* number of attributes		   */
		ATTR_DESCR attrs[],	/* attribute descriptors	   */
		char *primAttrName)	/* primary index attribute(ignore) */
{
	int hf_fd = -1;
	int recSize = 0;
	int i = 0;
	
	for (i = 0; i < numAttrs; i ++) {
		recSize += attrs[i].attrLen;
	}

	if ((hf_fd = HF_CreateFile(relName, recSize)) < 0) {
		printf("[CreateTable] failed to HF_CreateFile\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}
	
	return _CreateTable(relName, numAttrs, attrs, primAttrName, recSize);
}


/* Index task included */
int DestroyTable(char *relName)
{
	struct _relation_desc rel_desc;
	RECID rel_recid;
	RECID *attr_recids;
	ATTRDESCTYPE *attr_list;
	int ret = 0, i = 0;

	Search_RelCatalog(relName, &rel_desc, &rel_recid);

	attr_recids = calloc(rel_desc.attrcnt, sizeof(RECID));
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);	

	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, attr_recids);

	/* For each indexed attribute, delete index files */
	for (i = 0; i < rel_desc.attrcnt; i++) {
		if (attr_list[i].indexed == 1) {
			AM_DestroyIndex(relName, attr_list[i].attrno);
		}
	}
	/***************************************************/

	if ((ret = HF_DeleteRec(relcatFd, rel_recid)) < 0) {
		printf("[DestroyTable] failed to Delete record in rel catalog\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if ((ret = HF_DeleteRec(attrcatFd, attr_recids[i])) < 0) {
			printf("[DestroyTable] failed delete record in attr catalog\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}
	}

	free(attr_list);
	free(attr_recids);

	if ((ret = HF_DestroyFile(relName)) != HFE_OK) {
		printf("[DestroyTable] failure from hf_destroyfile\n");
		FEerrno = FEE_HF;
		return FEE_HF;	
	}

	return FEE_OK;
}

int BuildIndex(char *relName, char *attrName)
{
	RELDESCTYPE rel_desc;
	ATTRDESCTYPE *attr_list;
	int idx_attr = -1, i = 0;
	int am_fd = 0, fd = 0;
	RECID rel_recid, recid;
	RECID *attr_recids;
	char *rel_buff, *attr_buff, *idx_buff;
	int indexcnt = 0, indexed = 0;
	int ret = -1;

	Search_RelCatalog(relName, &rel_desc, &rel_recid);
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
	attr_recids = calloc(rel_desc.attrcnt, sizeof(RECID));
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, attr_recids);

	rel_buff = malloc(RELDESCSIZE);
	attr_buff = malloc(ATTRDESCSIZE);
	idx_buff = malloc(rel_desc.relwid);

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if (!strcmp(attr_list[i].attrname, attrName)) {
			idx_attr = i;
		}
	}

	if (idx_attr == -1) {
		printf("[BuildIndex] The attribute not exist\n");
		FEerrno = FEE_NONEXISTATTR;
		return FEE_NONEXISTATTR;
	}

	/* 2. Update relation and attribute record */
	if ((ret = HF_GetThisRec(relcatFd, rel_recid, rel_buff)) < 0) {
		printf("[Build] GetThisRecord error\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}
	
	indexcnt = *((int *)(rel_buff + relCatOffset(indexcnt)));
	*((int *)(rel_buff + relCatOffset(indexcnt))) = indexcnt + 1;

	if ((ret = HF_DeleteRec(relcatFd, rel_recid)) < 0) {
		printf("[Build] DeleteRec error\n");
		FEerrno = FEE_HF;
		return FEE_HF;	
	}
	
	HF_InsertRec(relcatFd, rel_buff);


	if ((ret = HF_GetThisRec(attrcatFd, attr_recids[idx_attr], attr_buff)) < 0) {
		printf("[Build] GetThisRec error\n");
		FEerrno = FEE_HF;
		return FEE_HF;		
	}

	indexed = *((int *)(attr_buff + attrCatOffset(indexed)));
	*((int *)(attr_buff + attrCatOffset(indexed))) = 1;

	if ((ret = HF_DeleteRec(attrcatFd, attr_recids[idx_attr])) < 0) {
		printf("[Build] DeleteRec error\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	HF_InsertRec(attrcatFd, attr_buff);
	
	/* 3. AM_Insert pre-exist records */
	AM_CreateIndex(relName, attr_list[idx_attr].attrno, 
				attr_list[idx_attr].attrtype,
				attr_list[idx_attr].attrlen, FALSE);

	am_fd = AM_OpenIndex(relName, attr_list[idx_attr].attrno);
	fd = HF_OpenFile(relName);
	recid = HF_GetFirstRec(fd, idx_buff);
	while (HF_ValidRecId(fd, recid)) {
		AM_InsertEntry(am_fd, idx_buff + attr_list[idx_attr].offset, recid);
		recid = HF_GetNextRec(fd, recid, idx_buff);
	}

	AM_CloseIndex(am_fd);
	HF_CloseFile(fd);
	free(attr_list);
	free(attr_recids);
	free(idx_buff);
	free(attr_buff);
	free(rel_buff);

	return FEE_OK;
}

int DropIndex(char *relName, char *attrName)
{
	/* 1. Call AM_Destroy */
	/* 2. Update relation meta data */
	RELDESCTYPE rel_desc;
	ATTRDESCTYPE *attr_list;
	int idx_attr = -1, i = 0;
	int fd = 0;
	RECID rel_recid;
	RECID *attr_recids;
	char *rel_buff, *attr_buff;
	int indexcnt = 0, indexed = 0;
	int ret = -1;

	Search_RelCatalog(relName, &rel_desc, &rel_recid);
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
	attr_recids = calloc(rel_desc.attrcnt, sizeof(RECID));
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, attr_recids);

	rel_buff = malloc(RELDESCSIZE);
	attr_buff = malloc(ATTRDESCSIZE);

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if (!strcmp(attr_list[i].attrname, attrName)) {
			idx_attr = i;
		}
	}

	if (idx_attr == -1) {
		printf("[DropIndex] The attribute not exist\n");
		FEerrno = FEE_NONEXISTATTR;
		return FEE_NONEXISTATTR;
	}

	/* 2. Update relation and attribute record */
	if ((ret = HF_GetThisRec(relcatFd, rel_recid, rel_buff)) < 0) {
		printf("[Drop] GetThisRecord error\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}
	
	indexcnt = *((int *)(rel_buff + relCatOffset(indexcnt)));
	*((int *)(rel_buff + relCatOffset(indexcnt))) = indexcnt - 1;

	if ((ret = HF_DeleteRec(relcatFd, rel_recid)) < 0) {
		printf("[Drop] DeleteRec error\n");
		FEerrno = FEE_HF;
		return FEE_HF;	
	}
	
	HF_InsertRec(relcatFd, rel_buff);


	if ((ret = HF_GetThisRec(attrcatFd, attr_recids[idx_attr], attr_buff)) < 0) {
		printf("[Drop] GetThisRec error\n");
		FEerrno = FEE_HF;
		return FEE_HF;		
	}

	indexed = *((int *)(attr_buff + attrCatOffset(indexed)));
	*((int *)(attr_buff + attrCatOffset(indexed))) = 0;

	if ((ret = HF_DeleteRec(attrcatFd, attr_recids[idx_attr])) < 0) {
		printf("[Drop] DeleteRec error\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	HF_InsertRec(attrcatFd, attr_buff);
	AM_DestroyIndex(relName, attr_list[idx_attr].attrno);

	free(rel_buff);
	free(attr_buff);
	free(attr_list);
	free(attr_recids);
	
	return FEE_OK;
}

/* Index task included */
int LoadTable(char *relName, char *fileName)
{	
	struct _relation_desc rel_desc;
	ATTRDESCTYPE *attr_list;
	int fd = 0, i = 0, ret = 0, fd_rel = 0;
	char *buff;
	ATTRDESCTYPE **index_attr_list;
	int *index_fds;
	int i_cnt = 0;
	RECID recid;

	if (!isFileExist(relName) || !isFileExist(fileName)) {
		printf("[LoadTable] relation file and data file not exist \n");
		FEerrno = FEE_NONEXISTTABLE;
		return FEE_NONEXISTTABLE;
	}
	
	Search_RelCatalog(relName, &rel_desc, NULL);
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

	index_attr_list = malloc(rel_desc.indexcnt * sizeof(ATTRDESCTYPE *));
	index_fds = malloc(rel_desc.indexcnt * sizeof(int));
	buff = malloc(rel_desc.relwid * sizeof(char));

	if ((fd_rel = HF_OpenFile(relName)) < 0) {
		printf("[LoadTable] failed to open relcat\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}
	
	if ((fd = open(fileName, O_RDWR)) < 2) {
		printf("[LoadTable] failed to open data file\n");
		FEerrno = FEE_LOADFILEOPEN;
		return FEE_LOADFILEOPEN;
	}

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if (attr_list[i].indexed == 1) {
			index_attr_list[i_cnt] = &attr_list[i];
			index_fds[i_cnt] = AM_OpenIndex(relName, 
							attr_list[i].attrno);
			i_cnt++;
		}
	}

	while ((ret = read(fd, buff, rel_desc.relwid)) != 0) {
		recid = HF_InsertRec(fd_rel, buff);
		for (i = 0; i < i_cnt; i) {
			AM_InsertEntry(index_fds[i], buff + 
					index_attr_list[i]->offset, recid);
		}
	}

	for (i = 0; i < i_cnt; i++) {
		AM_CloseIndex(index_fds[i]);
	}

	close(fd);

	if (HF_CloseFile(fd_rel)) {
		printf("[LoadTable] failed to close relation\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	free(index_attr_list);
	free(attr_list);
	free(buff);
	free(index_fds);

	return FEE_OK;
}

int Insert(char *relName, int numAttrs, ATTR_VAL values[])
{
	int i = 0, j = 0;
	RELDESCTYPE rel_desc;
	ATTRDESCTYPE *attr_list;
	char *buff;
	int fd_rel = 0;
	ATTRDESCTYPE **index_attr_list;
	int *index_fds;
	int i_cnt = 0;
	RECID recid;
	
	if (!isFileExist(relName)) {
		printf("[Insert] failed to open relation file\n");
		FEerrno = FEE_NONEXISTTABLE;
		return FEE_NONEXISTTABLE;
	}

	Search_RelCatalog(relName, &rel_desc, NULL);
	
	/* Checking attributes validation */
	if (numAttrs != rel_desc.attrcnt) {
		printf("[Insert] Wrong number of attributes\n");
		FEerrno = FEE_WRONG_ATTR_TYPE;
		return FEE_WRONG_ATTR_TYPE;
	}

	for (i = 0; i < numAttrs; i++) {
		if (values[i].value == NULL) {
			printf("[Insert] Wrong value of attr\n");
			FEerrno = FEE_WRONG_ATTR_TYPE;
			return FEE_WRONG_ATTR_TYPE;
		}
	}

	index_attr_list = malloc(rel_desc.indexcnt * sizeof(ATTRDESCTYPE *));
	index_fds = malloc(rel_desc.indexcnt * sizeof(int));
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

	buff = calloc(sizeof(char), rel_desc.relwid);
	
	if ((fd_rel = HF_OpenFile(relName)) < 0) {
		printf("[Insert] failed to open relcat\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	for (i = 0; i < numAttrs; i++) {
		int find_flag = 0;
		for (j = 0; j < rel_desc.attrcnt; j++) {
			if (!strcmp(values[i].attrName, attr_list[j].attrname)) {
				find_flag = 1;
				break;
			}
		}

		if (find_flag == 0) {
			printf("[Insert] Wrong attribute name %s\n", 
							values[i].attrName);
			FEerrno = FEE_INSERT_ATTR_NAME;
			return FEE_INSERT_ATTR_NAME;
		}
	
		memcpy(buff + attr_list[j].offset, values[i].value, values[i].valLength);
	}

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if (attr_list[i].indexed == 1) {
			index_attr_list[i_cnt] = &attr_list[i];
			index_fds[i_cnt] = AM_OpenIndex(relName,
							attr_list[i].attrno);
			i_cnt++;
		}
	}

	recid = HF_InsertRec(fd_rel, buff);

	for (i = 0; i < i_cnt; i++) {
		AM_InsertEntry(index_fds[i], buff + 
				index_attr_list[i]->offset, recid);
	}

	for (i = 0; i < i_cnt; i++) {
		AM_CloseIndex(index_fds[i]);
	}

	if (HF_CloseFile(fd_rel)) {
		printf("[Insert] failed to close table\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	free(index_attr_list);
	free(attr_list);
	free(buff);
	free(index_fds);

	return FEE_OK;
}

int Delete(char *relName, char *selAttr, int op, 
	   int valType, int valLength, void *value)
{
	int sd = 0, hd = 0;
	RELDESCTYPE rel_desc;
	ATTRDESCTYPE *attr_list;
	RECID recid;
	int i = 0;
	char *buff;
	ATTRDESCTYPE **index_attr_list;
	int *index_fds;
	int i_cnt = 0;

	if (!isFileExist(relName)) {
		printf("[Delte] the relation non-exist\n");
		FEerrno = FEE_NONEXISTTABLE;
		return FEE_NONEXISTTABLE;
	}

	if ((hd = HF_OpenFile(relName)) < 0) {
		printf("[Delte] failed to open relation\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	Search_RelCatalog(relName, &rel_desc, NULL);
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
	buff = calloc(rel_desc.relwid, sizeof(char));
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

	if (selAttr != NULL) {
		/* Find the offset of selAttr in record */
		for (i = 0; i < rel_desc.attrcnt; i++) {
			if (!strcmp(selAttr, attr_list[i].attrname)) {
				break;
			}
		}
	
		if (i == rel_desc.attrcnt) {
			printf("[Delete] selAttr is not exist in relName relation\n");
			FEerrno = FEE_DELETE_ATTR_NAME;
			return FEE_DELETE_ATTR_NAME;
		}

		if ((sd = HF_OpenFileScan(hd, valType, valLength, 
					attr_list[i].offset, op, value)) < 0) {
			printf("[Delete] problem in opening file scan\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}

	} else {
		if ((sd = HF_OpenFileScan(hd, 0, 0, 0, 0, NULL)) < 0) {
			printf("[Delete] problem in opening null file scan\n");
			FEerrno = FEE_HF;
			return FEE_HF;	
		}
	}

	index_attr_list = malloc(rel_desc.indexcnt * sizeof(ATTRDESCTYPE *));
	index_fds = malloc(rel_desc.indexcnt * sizeof(int));

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if (attr_list[i].indexed == 1) {
			index_attr_list[i_cnt] = &attr_list[i];
			index_fds[i_cnt] = AM_OpenIndex(relName,
							attr_list[i].attrno);
			i_cnt++;
		}
	}

	recid = HF_FindNextRec(sd, buff);
	while (HF_ValidRecId(hd, recid)) {
		for (i = 0; i < i_cnt; i++) {
			AM_DeleteEntry(index_fds[i], buff +
					index_attr_list[i]->offset, recid);
		}
		HF_DeleteRec(hd, recid);
		recid = HF_FindNextRec(sd, buff);
	}

	if (HF_CloseFileScan(sd) != HFE_OK) {
		printf("[Delete] failed to close scan\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	for (i = 0; i < i_cnt; i++) {
		AM_CloseIndex(index_fds[i]);
	}

	if (HF_CloseFile(hd) != HFE_OK) {
		printf("[Delete] failed to close relation file\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	free(index_attr_list);
	free(buff);
	free(attr_list);
	free(index_fds);

	return FEE_OK;
}

int Select(char *srcRelName, char *selAttr, int op, 
		int valType, int valLength, void *value, 
		int numProjAttrs, char *projAttrs[], char *resRelName)
{
	int i = 0, j = 0;
	int sd = 0, fd_dest = 0, fd_src = 0;
	int cmp_attr_idx = 0;
	RELDESCTYPE rel_desc;
	ATTRDESCTYPE *attr_list;
	ATTRDESCTYPE *filtered_attr_list;
	ATTR_DESCR *attr_list_create;
	char *in_buff, *out_buff;
	RECID recid;
	int new_table_width = 0;
	int is_tmp = 0;

	if (resRelName == NULL) {
		resRelName = malloc(MAXNAME);
		sprintf(resRelName, "%s", SEARCH_TMP_FILE_MARK);
		is_tmp = 1;
	}

	Search_RelCatalog(srcRelName, &rel_desc, NULL);
	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
	filtered_attr_list = calloc(numProjAttrs, ATTRDESCSIZE);
	attr_list_create = calloc(numProjAttrs, sizeof(ATTR_DESCR));
	Search_AttrCatalog(srcRelName, rel_desc.attrcnt, attr_list, NULL);

	for (i = 0; i < numProjAttrs; i++) {
		for (j = 0; j < rel_desc.attrcnt; j++) {
			
			if (!strcmp(projAttrs[i], attr_list[j].attrname)) {
				
				attr_list_create[i].attrName = 
						malloc(sizeof(char) * MAXNAME);
				memcpy(attr_list_create[i].attrName,
						attr_list[j].attrname, MAXNAME);
				attr_list_create[i].attrType =
						attr_list[j].attrtype;
				attr_list_create[i].attrLen =
						attr_list[j].attrlen;

				memcpy(&filtered_attr_list[i], 
					&attr_list[j], ATTRDESCSIZE);
				new_table_width += attr_list[j].attrlen;
				break;
			}
		}

		if (j == rel_desc.attrcnt) {
			printf("[Select] Can not find projAttrs in source relation\n");
			FEerrno = FEE_SELECT_ATTR_NAME;
			return FEE_SELECT_ATTR_NAME;
		}
	}

	if (!isFileExist(resRelName)) {
		CreateTable(resRelName, numProjAttrs, 
				attr_list_create, NULL);
	}
	/* Now we guarantee the dest relation is exist */
	if ((fd_dest = HF_OpenFile(resRelName)) < 0) {
		printf("[Select] failed to open relation\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	/* Retrieving the records */
	in_buff = calloc(rel_desc.relwid, sizeof(char));
	out_buff = calloc(new_table_width, sizeof(char));

	if ((fd_src = HF_OpenFile(srcRelName)) < 0) {
		printf("[Select] failed to open source relation\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}

	/* Find the ATTRDESCTYPE of selAttr in attr_list */
	if (selAttr != NULL) {
		for (i = 0; i < rel_desc.attrcnt; i++) {
			if (!strcmp(attr_list[i].attrname, selAttr)) {
				cmp_attr_idx = i;
				break;
			}
		}

		if (i == rel_desc.attrcnt) {
			printf("[Select] Can not find selAttr [%s] in src relation\n",
										selAttr);
			FEerrno = FEE_SELECT_ATTR_NAME;
			return FEE_SELECT_ATTR_NAME;
		}
		
		if ((sd = HF_OpenFileScan(fd_src, valType, valLength, 
					attr_list[cmp_attr_idx].offset,
					op, value)) < 0) {
			printf("[Select] failed to open file scan\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}
	} else {
		if ((sd = HF_OpenFileScan(fd_src, 0, 0, 0, 0, NULL)) < 0) {
			printf("[Select] failed to open default file scan\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}
	}

	recid = HF_FindNextRec(sd, in_buff);
	while (HF_ValidRecId(fd_src, recid)) {		
		
		for (i = 0; i < numProjAttrs; i++) {
			memcpy(out_buff, in_buff + filtered_attr_list[i].offset,
					filtered_attr_list[i].attrlen);
			out_buff += filtered_attr_list[i].attrlen;
		}
		out_buff -= new_table_width;	
		HF_InsertRec(fd_dest, out_buff);
		recid = HF_FindNextRec(sd, in_buff);
	}

	HF_CloseFileScan(sd);
	HF_CloseFile(fd_src);
	HF_CloseFile(fd_dest);
	free(in_buff);
	free(out_buff);
	free(attr_list);
	for (i = 0; i < numProjAttrs; i++) {
		free(attr_list_create[i].attrName);
	}
	free(attr_list_create);

	if (is_tmp == 1) {
		PrintTable(resRelName);
		DestroyTable(resRelName);
		free(resRelName);
	}

	return 0;
}

/* What will op be?, Assumption that op is always equal */
int Join(REL_ATTR *joinAttr1, int op, REL_ATTR *joinAttr2, 
	int numProjAttrs, REL_ATTR projAttrs[], char *resRelName)
{
	/*Open two input files and one output file */
	int is_tmp = 0;
	int fd_input1 = 0, fd_input2 = 0, fd_output = 0;
	int sd_input1 = 0, sd_input2 = 0;
	char *in_buff1, *in_buff2, *out_buff;
	int attr_idx1 = -1, attr_idx2 = -1;
	RECID recid1, recid2;
	ATTRDESCTYPE *attr_list1, *attr_list2;
	RELDESCTYPE rel_desc1, rel_desc2;
	ATTR_DESCR *attr_create;
	int nr_attr_create = 0;
	int i = 0;
	char **projAttrs_ch;

	/* For Test */
	int count = 0;
	/* */

	if (resRelName == NULL) {
		resRelName = malloc(MAXNAME);
		sprintf(resRelName, "%s", JOIN_TMP_FILE_MARK);
		is_tmp = 1;
	}

	Search_RelCatalog(joinAttr1->relName, &rel_desc1, NULL);
	Search_RelCatalog(joinAttr2->relName, &rel_desc2, NULL);

	attr_list1 = calloc(rel_desc1.attrcnt, ATTRDESCSIZE);
	attr_list2 = calloc(rel_desc2.attrcnt, ATTRDESCSIZE);

	Search_AttrCatalog(joinAttr1->relName, rel_desc1.attrcnt, attr_list1, NULL);
	Search_AttrCatalog(joinAttr2->relName, rel_desc2.attrcnt, attr_list2, NULL);

	fd_input1 = HF_OpenFile(joinAttr1->relName);
	fd_input2 = HF_OpenFile(joinAttr2->relName);

	/* Create new relation as the result of join */	
	nr_attr_create = rel_desc1.attrcnt + rel_desc2.attrcnt;
	attr_create = calloc(nr_attr_create, sizeof(ATTR_DESCR));
	for (i = 0; i < rel_desc1.attrcnt; i++) {
		attr_create[i].attrName = malloc(MAXNAME);
		memcpy(attr_create[i].attrName, attr_list1[i].attrname, MAXNAME);
		attr_create[i].attrType = attr_list1[i].attrtype;
		attr_create[i].attrLen = attr_list1[i].attrlen;
		if (!strcmp(joinAttr1->attrName, attr_list1[i].attrname)) {
			attr_idx1 = i;
		}
	}

	for (i = 0; i < rel_desc2.attrcnt; i++) {
		int idx = rel_desc1.attrcnt + i;
		attr_create[idx].attrName = malloc(MAXNAME);
		memcpy(attr_create[idx].attrName, attr_list2[i].attrname, MAXNAME);
		attr_create[idx].attrType = attr_list2[i].attrtype;
		attr_create[idx].attrLen = attr_list2[i].attrlen;
		if (!strcmp(joinAttr2->attrName, attr_list2[i].attrname)) {
			attr_idx2 = i;
		}
	}

	if (attr_idx1 == -1 || attr_idx2 == -1) {
		printf("[Join] Can not find join attr in each relation\n");
		FEerrno = FEE_JOIN_ATTR_NAME;
		return FEE_JOIN_ATTR_NAME;
	}

	if (!isFileExist(resRelName)) {
		CreateTable(resRelName, nr_attr_create, attr_create, NULL);
	}
	fd_output = HF_OpenFile(resRelName);

	/* Doing join */
	sd_input1 = HF_OpenFileScan(fd_input1, 0, 0, 0, 0, NULL);
	sd_input2 = HF_OpenFileScan(fd_input2, 0, 0, 0, 0, NULL);

	in_buff1 = malloc(rel_desc1.relwid);
	in_buff2 = malloc(rel_desc2.relwid);
	out_buff = malloc(rel_desc1.relwid + rel_desc2.relwid);

	/* printf("idx1 : %d, idx2 : %d\n", attr_idx1, attr_idx2); */

	recid1 = HF_FindNextRec(sd_input1, in_buff1);
	while (HF_ValidRecId(fd_input1, recid1)) {
		
		recid2 = HF_FindNextRec(sd_input2, in_buff2);
		while (HF_ValidRecId(fd_input2, recid2)) {
			if (!strncmp(in_buff1 + attr_list1[attr_idx1].offset,
				     in_buff2 + attr_list2[attr_idx2].offset,
				     attr_list1[attr_idx1].attrlen)) {
				memcpy(out_buff, in_buff1, rel_desc1.relwid);
				memcpy(out_buff + rel_desc1.relwid, in_buff2,
							   rel_desc2.relwid);
				HF_InsertRec(fd_output, out_buff);
			}

			recid2 = HF_FindNextRec(sd_input2, in_buff2);
		}

		HF_CloseFileScan(sd_input2);
		sd_input2 = HF_OpenFileScan(fd_input2, 0, 0, 0, 0, NULL);
		recid1 = HF_FindNextRec(sd_input1, in_buff1);
	}

	/* Close scan files */
	HF_CloseFileScan(sd_input1);
	HF_CloseFileScan(sd_input2);
	HF_CloseFile(fd_input1);
	HF_CloseFile(fd_input2);
	HF_CloseFile(fd_output);

	projAttrs_ch = malloc(sizeof(char *) * numProjAttrs);
	for (i = 0; i < numProjAttrs; i++) {
		projAttrs_ch[i] = malloc(MAXNAME);
		memcpy(projAttrs_ch[i], projAttrs[i].attrName, MAXNAME);
	}

	Select(resRelName, NULL, 0, 0, 0, NULL, numProjAttrs, projAttrs_ch, NULL);

	/* Mem Free */
	for (i = 0; i < nr_attr_create; i++) {
		free(attr_create[i].attrName);
	}

	free(in_buff1);
	free(in_buff2);
	free(out_buff);
	free(attr_list1);
	free(attr_list2);
	free(attr_create);

	if (is_tmp == 1) {
		/* PrintTable(resRelName); */
		DestroyTable(resRelName);
		free(resRelName);
	}

	for (i = 0; i < numProjAttrs; i++) {
		free(projAttrs_ch[i]);
	}

	free(projAttrs_ch);

	return 0;
}

int HelpTable(char *relName)
{
	struct _relation_desc rel_desc;
	/* ATTR_DESCR *attr_list; */
	ATTRDESCTYPE *attr_list;
	int i = 0;
	int nr_attr_field = 6;

	if (relName == NULL) {
		PrintTable(RELCATNAME);
	} else {
		Search_RelCatalog(relName, &rel_desc, NULL);

		attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);
		Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

		printf("<Print %s Schema>\n", relName);
		for (i = 0; i < nr_attr_field; i++) {
			printf("---------------");
		}
		printf("\n");
		
		printf("| %12s | %12s | %12s | %12s | %12s | %12s |\n", 
				"attrname", "offset", "length", 
				"type", "indexed", "attrno");
		
		for (i = 0; i < nr_attr_field; i++) {
			printf("---------------");
		}
		printf("\n");

		for (i = 0; i < rel_desc.attrcnt; i++) {
			/* attrname offset length type indexed attrno */
			printf("| %12s | %12d | %12d | %12s | %12s | %12d |\n", 
				attr_list[i].attrname, attr_list[i].offset,
				attr_list[i].attrlen, 
				attr_list[i].attrtype == INT_TYPE ? "Integer" :
				attr_list[i].attrtype == REAL_TYPE ? "Real" :
				attr_list[i].attrtype == STRING_TYPE ? "String" :
				attr_list[i].attrtype == BOOL_TYPE ? "Bool" : "Integer",
				/* attr_list[i].attrtype, */
				attr_list[i].indexed == 0 ? "No" : "Yes",
				/* attr_list[i].indexed, */
				attr_list[i].attrno);
		}

		for (i = 0; i < nr_attr_field; i++) {
			printf("---------------");
		}
		printf("\n");

		free(attr_list);
	}

	return 0;
}


int PrintTable(char *relName)
{
	RECID next_recid;
	struct _relation_desc rel_desc;
	struct _attribute_desc attr_desc;
	ATTRDESCTYPE *attr_list;
	/* ATTR_DESCR *attr_list; */
	int i = 0;
	int fd_rel = 0;
	char *data_buff;
	int count = 0;

	if (attrcatFd == relcatFd == -1) {
		printf("[PrintTable] database is not connected\n");
		FEerrno = FEE_DATABASE_NONCONNECT;
		return FEE_DATABASE_NONCONNECT;
	}

	Search_RelCatalog(relName, &rel_desc, NULL);

	/* We find the relation in rel_catalog */
	/* Find the attribute metadata in attr_catalog */

	attr_list = calloc(rel_desc.attrcnt, ATTRDESCSIZE);

	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

	printf("<Print %s Table>\n", relName);
	
	/* Print Schema */
	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("---------------");
	}
	printf("\n");
	
	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("| %12s ", attr_list[i].attrname);
	}
	
	printf("|\n");
	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("---------------");
	}
	printf("\n");


	/* Print Data */
	if (!strcmp(relName, RELCATNAME)) {
		fd_rel = relcatFd;
	} else if (!strcmp(relName, ATTRCATNAME)) {
		fd_rel = attrcatFd;
	} else {
		fd_rel = HF_OpenFile(relName);	
	}
	
	if (fd_rel < 0) {
		printf("[Printable] failed to open the relation named relName\n");
		FEerrno = FEE_HF;
		return FEE_HF;
	}
	
	data_buff = malloc(sizeof(char) * rel_desc.relwid);

	for (next_recid = HF_GetFirstRec(fd_rel, data_buff);
	     HF_ValidRecId(fd_rel, next_recid);
	     next_recid = HF_GetNextRec(fd_rel, next_recid, data_buff)) {

		if (!HF_ValidRecId(attrcatFd, next_recid)) {
			printf("[PrintTable]: No first record in target relation\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}

		for (i = 0; i < rel_desc.attrcnt; i++) {
			switch (attr_list[i].attrtype) {
			case INT_TYPE:
				printf("| %12d ", *((int*)data_buff));
				data_buff += attr_list[i].attrlen;
				break;
			case REAL_TYPE:
				printf("| %12f ", *((float*)data_buff));
				data_buff += attr_list[i].attrlen;
				break;
			case STRING_TYPE:
				printf("| %12s ", (char*)data_buff);
				data_buff += attr_list[i].attrlen;
				break;
			case BOOL_TYPE:
				printf("| %12s ", *((int*)data_buff) == 1 ?
								"Yes" : "No");
				data_buff += attr_list[i].attrlen;
				break;
			case TYPE_TYPE:
				printf("| %12s ", *((int*)data_buff) == INT_TYPE ?
						"Integer" : 
						*((int*)data_buff) == REAL_TYPE ? 
						"Real" :
						*((int*)data_buff) == STRING_TYPE ?
						"String" :
						*((int*)data_buff) == BOOL_TYPE ?
						"Bool" : "Integer");
				data_buff += attr_list[i].attrlen;
				break;
			default:
				printf("[PrintTable] Wrong attr type\n");
				FEerrno = FEE_WRONG_ATTR_TYPE;
				return FEE_WRONG_ATTR_TYPE;
			}
		}
		printf("|\n");
		data_buff -= rel_desc.relwid;
	}

	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("---------------");
	}

	printf("\n");

	if (strcmp(relName, RELCATNAME) && strcmp(relName, ATTRCATNAME)) {
		HF_CloseFile(fd_rel);
	} 

	free(attr_list);
	free(data_buff);

	return 0;
}


/* Search relation data in rel_cataglog */
int Search_RelCatalog(char *relName, struct _relation_desc *rel_desc, RECID *recid)
{
	RECID next_recid;
	
	for (next_recid = HF_GetFirstRec(relcatFd, (char *)(rel_desc));
	     HF_ValidRecId(relcatFd, next_recid);
	     next_recid = HF_GetNextRec(relcatFd, next_recid, (char *)(rel_desc))) {

		if (!HF_ValidRecId(relcatFd, next_recid)) {
			printf("[PrintTable]: No first record in relation catalog\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}

		if (!strcmp(relName, rel_desc->relname)) { 
			break;
		}
	}

	if (!HF_ValidRecId(relcatFd, next_recid)) {
		printf("[PrintTable]: Trying to print non-exist table\n");
		FEerrno = FEE_NONEXISTTABLE;
		return FEE_NONEXISTTABLE;
	}

	if (recid != NULL) {
		memcpy(recid, &next_recid, sizeof(RECID));
	}

	return 0;
}

int qsort_compare(const void *first, const void *second)
{
	if (((ATTRDESCTYPE *)first)->offset > ((ATTRDESCTYPE *)second)->offset)
		return 1;
	else if (((ATTRDESCTYPE *)first)->offset < ((ATTRDESCTYPE *)second)->offset)
		return -1;
	else
		return 0;
}

/* Search attributes data in attr_catalog */
int Search_AttrCatalog(char *relName, int attrcnt, ATTRDESCTYPE *attr_list, RECID *recids)
{
	RECID next_recid;
	int count = 0;
	struct _attribute_desc attr_desc;
	for (next_recid = HF_GetFirstRec(attrcatFd, (char *)(&attr_desc));
	     HF_ValidRecId(attrcatFd, next_recid);
	     next_recid = HF_GetNextRec(attrcatFd, next_recid, (char *)(&attr_desc))) {

		if (!HF_ValidRecId(attrcatFd, next_recid)) {
			printf("[PrintTable]: No first record in relation catalog\n");
			FEerrno = FEE_HF;
			return FEE_HF;
		}

		if (!strcmp(relName, attr_desc.relname)) {
			
			memcpy(&attr_list[count], &attr_desc, ATTRDESCSIZE);
			if (recids != NULL) {
				memcpy(&recids[count], &next_recid, sizeof(RECID));
			}
			count++;
		}
	}

	if (count != attrcnt) {
		printf("[PrintTable]: Can not find attributes in attr catalog\n");
		FEerrno = FEE_WRONG_ATTR_TYPE;
		return FEE_WRONG_ATTR_TYPE;
	}

	qsort(attr_list, attrcnt, ATTRDESCSIZE, qsort_compare);
	return 0;
}

void FE_PrintError(char *errmsg)
{
	printf("[FE_PrintError] %s\n", errmsg);
}

