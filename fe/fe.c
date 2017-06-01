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
#include "minirel.h"
#include "catalog.h"

int Search_RelCatalog(char *relName, struct _relation_desc *rel_desc, RECID *recid);
int Search_AttrCatalog(char *relName, int attrcnt, ATTR_DESCR *attr_list, RECID *recid);

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
	return ret < 0 ? 0 : 1;
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
	ATTR_DESCR *attr_cata_list;

	if (isFileExist(dbname)) {
		printf("%s\n", dbname);
		printf("[DBcreate] dbname database is already exist\n");
		exit(1);
	}

	if ((ret = mkdir(dbname, 0777)) < 0) {
		printf("[DBCreate] failed to create database directory\n");
		exit(1);
	}

	sprintf(db_location, "%s/%s", data_dir, dbname);	
	if (chdir(db_location)) {
		printf("%s\n", db_location);
		printf("[DBCreate] failed to change pwd\n");
		exit(1);
	}

	if (HF_CreateFile(RELCATNAME, RELDESCSIZE)) {
		printf("[DBCreate] failed to create relcat file\n");
		exit(1);
	}

	if (HF_CreateFile(ATTRCATNAME, ATTRDESCSIZE) != 0) {
		printf("[DBCreate] failed to create attribute file\n");
		exit(1);
	}

	if ((fd_cat_rel = HF_OpenFile(RELCATNAME)) < 0) {
		printf("[DBCreate] failed to open relcat\n");
		exit(1);
	}

	if ((fd_cat_attr = HF_OpenFile(ATTRCATNAME)) < 0) {
		printf("[DBCreate] failed to open attrcat\n");
		exit(1);
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
	
	attr_cata_list[4].attrType = INT_TYPE;    attr_cata_list[4].attrLen = sizeof(int);
	sprintf(attr_cata_list[4].attrName, "%s", "attrtype");
	
	attr_cata_list[5].attrType = INT_TYPE;    attr_cata_list[5].attrLen = sizeof(int);
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

	if (HF_CloseFile(fd_cat_rel)) {
		printf("[DBCreate] failed to close relcat\n");
		exit(1);
	}

	if (HF_CloseFile(fd_cat_attr)) {
		printf("[DBCreate] failed to close relcat\n");
		exit(1);
	}

	if (chdir(data_dir)) {
		printf("[DBCreate] failed to change pwd\n");
		exit(1);
	}

	fd_cat_rel = fd_cat_attr = -1;
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
	sprintf(rel_desc.primattr, "%s", "none");
	HF_InsertRec(fd_cat_rel, (char*)(&rel_desc));

	printf("CreateTable rel_catalog insertion done\n");

	for (i = 0; i < numAttrs; i++) {
		memcpy(attr_desc.relname, relName, MAXNAME);
		memcpy(attr_desc.attrname, attrs[i].attrName, MAXNAME);
		attr_desc.offset = offset;
		offset += attrs[i].attrLen;
		attr_desc.attrlen = attrs[i].attrLen;
		attr_desc.attrtype = attrs[i].attrType;
		attr_desc.indexed = FALSE;
		attr_desc.attrno = 0;
		HF_InsertRec(fd_cat_attr, (char*)(&attr_desc));
		printf("CreateTable attr_catalog insertion done\n");
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
		exit(1);
	}
	printf("CreateTable start\n");
	return _CreateTable(relName, numAttrs, attrs, primAttrName, recSize);
}


int DestroyTable(char *relName)
{
	struct _relation_desc rel_desc;
	RECID rel_recid;
	RECID *attr_recids;
	ATTR_DESCR *attr_list;
	int ret = 0, i = 0;

	Search_RelCatalog(relName, &rel_desc, &rel_recid);

	attr_list = calloc(rel_desc.attrcnt, sizeof(ATTR_DESCR));
	attr_recids = calloc(rel_desc.attrcnt, sizeof(RECID));
	for (i = 0; i < rel_desc.attrcnt; i++) {
		attr_list[i].attrName = malloc(MAXNAME);
	}
	
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, attr_recids);

	if ((ret = HF_DeleteRec(fd_cat_rel, rel_recid)) < 0) {
		printf("[DestroyTable] failed to Delete record in rel catalog\n");
		exit(1);
	}

	for (i = 0; i < rel_desc.attrcnt; i++) {
		if ((ret = HF_DeleteRec(fd_cat_attr, attr_recids[i])) < 0) {
			printf("[DestroyTable] failed to Delete record in attr catalog\n");
			exit(1);
		}
	}

	return FEE_OK;
}


int LoadTable(char *relName, char *fileName)
{	
	struct _relation_desc rel_desc;
	ATTR_DESCR *attr_list;
	int fd = 0, i = 0, ret = 0, fd_rel = 0;
	char *buff;
	

	if (!isFileExist(relName) || !isFileExist(fileName)) {
		printf("[LoadTable] relation file and data file not exist \n");
		exit(1);
	}
	
	Search_RelCatalog(relName, &rel_desc, NULL);

	attr_list = calloc(rel_desc.attrcnt, sizeof(ATTR_DESCR));
	for (i = 0; i < rel_desc.attrcnt; i++) {
		attr_list[i].attrName = malloc(MAXNAME);
	}
	
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

	buff = malloc(rel_desc.relwid + 1);
	
	if ((fd_rel = HF_OpenFile(relName)) < 0) {
		printf("[LoadTable] failed to open relcat\n");
		exit(1);
	}

	if ((fd = open(fileName, O_RDONLY) < 0)) {
		printf("[LoadTable] failed to open data file\n");
		exit(1);
	}
	
	while ((ret = read(fd, buff, rel_desc.relwid)) != 0) {
		HF_InsertRec(fd_rel, buff);
	}

	if (HF_CloseFile(fd_rel)) {
		printf("[LoadTable] failed to close relation\n");
		exit(1);
	}

	return FEE_OK;
}


int HelpTable(char *relName)
{
	struct _relation_desc rel_desc;
	ATTR_DESCR *attr_list;
	int i = 0;

	if (relName == NULL) {
		PrintTable(RELCATNAME);
	} else {
		Search_RelCatalog(relName, &rel_desc, NULL);

		attr_list = calloc(rel_desc.attrcnt, sizeof(ATTR_DESCR));
		for (i = 0; i < rel_desc.attrcnt; i++) {
			attr_list[i].attrName = malloc(MAXNAME);
		}
	
		Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

		printf("Print %s Schema\n", relName);
		for (i = 0; i < rel_desc.attrcnt; i++) {
			printf("-----------------------");
		}
		printf("\n");
	
		for (i = 0; i < rel_desc.attrcnt; i++) {
			printf("|%12s[%3d:%3d] ", attr_list[i].attrName, 
					      attr_list[i].attrType,
					      attr_list[i].attrLen);
		}

		printf("\n");
		for (i = 0; i < rel_desc.attrcnt; i++) {
			printf("-----------------------");
		}
		printf("\n");
	}
}


int PrintTable(char *relName)
{
	RECID next_recid;
	struct _relation_desc rel_desc;
	struct _attribute_desc attr_desc;
	ATTR_DESCR *attr_list;
	int i = 0;
	int fd_rel = 0;
	char *data_buff;

	if (fd_cat_attr == fd_cat_rel == -1) {
		printf("[PrintTable] database is not connected\n");
		exit(1);
	}

	Search_RelCatalog(relName, &rel_desc, NULL);

	/* We find the relation in rel_catalog */
	/* Find the attribute metadata in attr_catalog */
	attr_list = calloc(rel_desc.attrcnt, sizeof(ATTR_DESCR));
	for (i = 0; i < rel_desc.attrcnt; i++) {
		attr_list[i].attrName = malloc(MAXNAME);
	}
	
	Search_AttrCatalog(relName, rel_desc.attrcnt, attr_list, NULL);

	/* Print Schema */
	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("-----------------------");
	}
	printf("\n");
	
	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("|%12s[%3d:%3d] ", attr_list[i].attrName, 
				      attr_list[i].attrType,
				      attr_list[i].attrLen);
	}

	printf("\n");
	for (i = 0; i < rel_desc.attrcnt; i++) {
		printf("-----------------------");
	}
	printf("\n");


	/* Print Data */
	if (!strcmp(relName, RELCATNAME)) {
		fd_rel = fd_cat_rel;
	} else if (!strcmp(relName, ATTRCATNAME)) {
		fd_rel = fd_cat_attr;
	} else {
		fd_rel = HF_OpenFile(relName);	
	}
	
	if (fd_rel < 0) {
		printf("[Printable] failed to open the relation named relName\n");
		exit(1);
	}
	
	data_buff = malloc(rel_desc.relwid);

	for (next_recid = HF_GetFirstRec(fd_rel, data_buff);
	     HF_ValidRecId(fd_cat_attr, next_recid);
	     next_recid = HF_GetNextRec(fd_rel, next_recid, data_buff)) {

		if (!HF_ValidRecId(fd_cat_attr, next_recid)) {
			printf("[PrintTable]: No first record in target relation\n");
			exit(1);
		}

		for (i = 0; i < rel_desc.attrcnt; i++) {
			switch (attr_list[i].attrType) {
			case INT_TYPE:
				printf("%22d ", *((int*)data_buff));
				data_buff += attr_list[i].attrLen;
				break;
			case REAL_TYPE:
				printf("%22f ", *((float*)data_buff));
				data_buff += attr_list[i].attrLen;
				break;
			case STRING_TYPE:
				printf("%22s ", (char*)data_buff);
				data_buff += attr_list[i].attrLen;
				break;
			default:
				printf("error\n");
				exit(1);
			}
		}
		printf("\n");
	}

	if (strcmp(relName, RELCATNAME) && strcmp(relName, ATTRCATNAME)) {
		HF_CloseFile(fd_rel);
	} 

	return 0;
}


/* Search relation data in rel_cataglog */
int Search_RelCatalog(char *relName, struct _relation_desc *rel_desc, RECID *recid)
{
	RECID next_recid;
	
	for (next_recid = HF_GetFirstRec(fd_cat_rel, (char *)(rel_desc));
	     HF_ValidRecId(fd_cat_rel, next_recid);
	     next_recid = HF_GetNextRec(fd_cat_rel, next_recid, (char *)(rel_desc))) {

		if (!HF_ValidRecId(fd_cat_rel, next_recid)) {
			printf("[PrintTable]: No first record in relation catalog\n");
			exit(1);
		}

		if (!strcmp(relName, rel_desc->relname)) { 
			break;
		}
	}

	if (!HF_ValidRecId(fd_cat_rel, next_recid)) {
		printf("[PrintTable]: Trying to print non-exist table\n");
		exit(1);
	}

	if (recid != NULL) {
		memcpy(recid, &next_recid, sizeof(RECID));
	}

	return 0;
}

/* Search attributes data in attr_catalog */
int Search_AttrCatalog(char *relName, int attrcnt, ATTR_DESCR *attr_list, RECID *recids)
{
	RECID next_recid;
	int count = 0;
	struct _attribute_desc attr_desc;
	for (next_recid = HF_GetFirstRec(fd_cat_attr, (char *)(&attr_desc));
	     HF_ValidRecId(fd_cat_attr, next_recid);
	     next_recid = HF_GetNextRec(fd_cat_attr, next_recid, (char *)(&attr_desc))) {

		if (!HF_ValidRecId(fd_cat_attr, next_recid)) {
			printf("[PrintTable]: No first record in relation catalog\n");
			exit(1);
		}

		if (!strcmp(relName, attr_desc.relname)) {
			
			memcpy(attr_list[count].attrName, attr_desc.attrname, MAXNAME);
			attr_list[count].attrType = attr_desc.attrtype;
			attr_list[count].attrLen = attr_desc.attrlen;
			if (recids != NULL) {
				memcpy(&recids[count], &next_recid, sizeof(RECID));
			}
			count++;
		}
	}

	if (count != attrcnt) {
		printf("[PrintTable]: Can not find all attributes data in attr catalog\n");
		exit(1);
	}
}

int BuildIndex(char *relName, char *attrName)
{
	return 0;
}

int DropIndex(char *relname, char *attrName)
{
	return 0;
}

void FE_PrintError(char *errmsg)
{
	printf("[FE_PrintError] %s\n", errmsg);
}

