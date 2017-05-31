#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include "fe.h"
#include "hf.h"
#include "minirel.h"

char data_dir[STR_SIZE];
int fd_cat_rel, fd_cat_attr;

/* Checking the file is exist in present directory 
 * @return: >= 0 (File exist)
 * 	    <  0 (Not exist)
 */
int isFileExist(char *name)
{
	int ret = -1;
	if ((ret = open(name, O_RDONLY)) >= 0)
		close(ret);
	return ret;
}

void FE_Init(void)
{
	if (!getcwd(data_dir, STR_SIZE)) {
		printf("[FE_Init] Fail to get pwd\n");
		exit(1);
	}

	fd_cat_rel = fd_cat_attr = -1;
	HF_Init();
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

	if (isFileExist(dbname)) {
		printf("[DBcreate] dbname database is already exist\n");
		exit(1);
	}

	if ((ret = mkdir(dbname)) < 0) {
		printf("[DBCreate] failed to create database directory\n");
		exit(1);
	}

	sprintf(db_location, "%s/%s", data_dir, dbname);	
	if (chdir(db_location)) {
		printf("[DBCreate] failed to change pwd\n");
		exit(1);
	}

	if (HF_CreateFile(RELCATNAME, RELDECSIZE)) {
		printf("[DBCreate] failed to create relcat file\n");
		exit(1);
	}

	if (HF_CreateFile(ATTRCATNAME, ATTRCAT_NATTRS) != 0) {
		printf("[DBCreate] failed to create attribute file\n");
		exit(1);
	}

	/* Relation table (Attribute name will be omitted) */
	attr_desc_list = calloc(RELCAT_NATTRS, sizeof(ATTR_DESCR));
	attr_desc_list[0].attrType = STRING_TYPE; attr_desc_list[0].attrLen = MAXNAME;
	attr_desc_list[1].attrType = INT_TYPE;    attr_desc_list[1].attrLen = sizeof(int);
	attr_desc_list[2].attrTYpe = INT_TYPE;    attr_desc_list[2].attrLen = sizeof(int);
	attr_desc_list[3].attrTYpe = INT_TYPE;    attr_desc_list[3].attrLen = sizeof(int);
	attr_desc_list[4].attrType = STRING_TYPE; attr_desc_list[4].attrLen = MAXNAME;
	_CreateTable(RELCATNAME, RELCAT_NATTRS, attr_desc_list, NULL);
	free(attr_desc_list);
	
	/* Attribute table */
	attr_desc_list = calloc(ATTRCAT_NATTRS, sizeof(ATTR_DESCR));
	attr_desc_list[0].attrType = STRING_TYPE; attr_desc_list[0].attrLen = MAXNAME;
	attr_desc_list[1].attrType = STRING_TYPE; attr_desc_list[1].attrLen = MAXNAME;
	attr_desc_list[2].attrType = INT_TYPE; 	  attr_desc_list[2].attrLen = sizeof(int);
	attr_desc_list[3].attrTYpe = INT_TYPE;    attr_desc_list[3].attrLen = sizeof(int);
	attr_desc_list[4].attrTYpe = INT_TYPE;    attr_desc_list[4].attrLen = sizeof(int);
	attr_desc_list[5].attrTYpe = INT_TYPE;    attr_desc_list[5].attrLen = sizeof(int);
	attr_desc_list[6].attrTYpe = INT_TYPE;    attr_desc_list[6].attrLen = sizeof(int);
	_CreateTable(ATTRCATNAME, ATTRCAT_NATTRS, attr_desc_list, NULL);
	free(attr_desc_list);

	if (chdir(data_dir)) {
		printf("[DBCreate] failed to change pwd\n");
		exit(1);
	}

	return FEE_OK;
}


void DBdestroy(char *dbname)
{
	char db_location[STR_SIZE];
	sprintf(db_location, "%s/%s", data_dir, dbname);
	if (rmdir(db_location)) {
		printf("[DBdestroy] failed to remove dir\n");
		exit(1);
	}
}

void DBconnect(char *dbname)
{
	char db_location[STR_SIZE];
	int ret = -1;
	
	sprintf(db_location, "%s/%s", data_dir, dbname);
	
	if ((ret = isFileExist(db_location)) < 0) {
		printf("[DBconnect] Trying to connect un-exist database\n");
		exit(1);
	}
	
	if (chdir(db_location)) {
		printf("[DBConnect] failed to change pwd\n");
		exit(1);
	}

	/* Opening attribute table and relation table? */
	fd_cat_rel = HF_OpenFile(RELCATNAME);
	fd_cat_attr = HF_OpenFile(ATTRCATNAME);
}

void DBclose(char *dbname)
{
	if (chdir(data_dir)) {
		printf("[DBConnect] failed to change pwd\n");
		exit(1);
	}

	/* Closing attribute table and relation table? */
	HF_CloseFile(fd_cat_rel);
	HF_CloseFile(fd_cat_attr);
	fd_cat_rel = fd_cat_attr = -1;
}

int _CreateTable(char *relName,
		 int numAttrs,
		 ATTR_DESCR attrs[],
		 char *primAttrName)
{
	struct _relation_desc rel_desc;
	struct _attribute_desc attr_desc;
	int hf_fd;

	/* 1. Inserting the relation data as relcat record */
	if ((hf_fd = HF_OpenFile(RELCATNAME)) < 0) {
		printf("[_CreateTable] failed to open relcat\n");
		exit(1);
	}

	memcpy(rel_desc.relname, relName, MAXNAME);
	rel_desc.relwid = recSize;
	rel_desc.attrcnt = numAttrs;
	rel_desc.indexcnt = 0;
	HF_InsertRec(hf_fd, &rel_desc);

	if (HF_CloseFile(hf_fd)) {
		printf("[_CreateTable] failed to close relcat\n");
		exit(1);
	}

	/* 2. Inserting all attributes as attrcat record */
	if ((hf_fd = HF_OpenFile(ATTRCATNAME)) < 0) {
		printf("[_CreateTable] failed to open attrcat\n");
		exit(1);
	}
	for (i = 0; i < numAttrs; i++) {
		memcpy(attr_desc.relname, relName, MAXNAME);
		memcpy(attr_desc.attrname, attrs[i].attrName, MAXNAME);
		attr_desc.offset = offset;
		offset += attrs[i].attrLen;
		attr_desc.attrlen = attrs[i].attrLen;
		attr_desc.attrtype = attrs[i].attrType;
		attr_desc.indexed = FALSE;
		attr_desc.attrno = 0;
		HF_InsertRec(hf_fd, &attr_desc);
	}

	if (HF_CloseFile(hf_fd)) {
		printf("[_CreateTable] failed to close attrcat\n");
		exit(1);
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

	/* 1. Create File for Relation */
	if ((hf_fd = HF_CreateFile(relName, recSize)) < 0) {
		printf("[CreateTable] failed to HF_CreateFile\n");
		exit(1);
	}

	return _CreateTable(relName, numAttrs, attrs, primAttrName);
}


int GetRelDes(char *relName, struct _relation_desc *rel_desc)
{
	char record[RELDESCSIZE];
	RECID recid;
	RECID next_recid;
	
	if (fd_cat_attr == fd_cat_rel == -1) {
		printf("[GetRelDes] database is not connected\n");
		exit(1);
	}

}


int PrintTable(char *relName)
{
	if (fd_cat_attr == fd_cat_rel == -1) {
		printf("[PrintTable] database is not connected\n");
		exit(1);
	}





}
