#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fe.h"
#include "hf.h"
#include "minirel.h"

char data_dir[STR_SIZE];

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
	int rel_fd, attr_fd;
	struct _relation_desc rel_desc;
	struct _attribute_desc attr_desc;
	int i = 0;

	if (isFileExist(dbname)) {
		printf("[DBcreate] dbname database is already exist\n");
		exit(1);
	}

	/* Creating the directory for dbname database */
	if ((ret = mkdir(dbname)) < 0) {
		printf("[DBCreate] failed to create database directory\n");
		exit(1);
	}

	sprintf(db_location, "%s/%s", data_dir, dbname);	
	if (chdir(db_location)) {
		printf("[DBCreate] failed to change pwd\n");
		exit(1);
	}

	/* Creating catalogs files: Relation */
	if ((rel_fd = HF_CreateFile(RELCATNAME, RELDECSIZE)) != 0) {
		printf("[DBCreate] failed to create relcat file\n");
		exit(1);
	}

	memcpy(rel_desc.relname, RELCATNAME);
	rel_desc.relwid = RELDECSIZE;
	rel_desc.attrcnt = RELCAT_NATTRS;
	rel_desc.indexcnt = 0;
	memcpy(rel_desc.primattr, "relname");

	if (HF_InsertRec(rel_fd, &rel_desc)) {
		printf("[DBCreate] failed to insert relation record\n");
		exit(1);
	}

	/* Creating catalogs files: Attribute */
	if ((attr_fd = HF_CreateFile(ATTRCATNAME, ATTRCAT_NATTRS)) != 0) {
		printf("[DBCreate] failed to create attribute file\n");
		exit(1);
	}

	/* Inserting attribute record into table */
	for (i = 0; i < RELCAT_NATTRS; i++) {
		memcpy(attr_desc.relname, RELCATNAME);
		/* memcpy(attr_desc.attrname, "relname"); */
		attr_desc.offset = relCatOffset(relname);
		attr_desc.attrlen = MAXNAME;
		attr_desc.attrtype = STRING_TYPE;
		attr_desc.indexed = FALSE;
		attr_attrno = 0;
	
		if (HF_InsertRec(attr_fd, &attr_desc)) {
			printf("[DBCreate] failed to insert attribute record\n");
			exit(1);
		}
	}
	/******/

	/* 
	 * 4. Catalog also table, so the catalog data should be stored into 
	 * catalogs files with in specified format 
	 */

}

void DBdestroy(char *dbname)
{
}

void DBconnect(char *dbname)
{
	/* 1. Checking whether we have database named dbname */
	/* 2. Load all appropriate data as current state */
	/* 3. Change present direectory */
}

void DBclose(char *dbname)
{
	/* 1. Storing all state */
	/* 2. Change present directory to parent dir location */
}

/*
 * DDL Functions
 */
int  CreateTable(char *relName,		/* name	of relation to create	*/
		int numAttrs,		/* number of attributes		*/
		ATTR_DESCR attrs[],	/* attribute descriptors	*/
		char *primAttrName);	/* primary index attribute	*/

int  DestroyTable(char *relName);	/* name of relation to destroy	*/

int  BuildIndex(char *relName,		/* relation name		*/
		char *attrName);	/* name of attr to be indexed	*/

int  DropIndex(char *relname,		/* relation name		*/
		char *attrName);	/* name of indexed attribute	*/

int  PrintTable(char *relName);		/* name of relation to print	*/

int  LoadTable(char *relName,		/* name of target relation	*/
		char *fileName);	/* file containing tuples	*/

int  HelpTable(char *relName);		/* name of relation		*/


