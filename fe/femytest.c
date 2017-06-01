#include <stdio.h>
#include <stdlib.h>
#include "fe.h"
#include "minirel.h"

#define DB_NAME "TestDB"
#define REL_CATALOG_NAME "relcat"
#define ATTR_CATALOG_NAME "attrcat"


int main(void)
{
	int nr_attrs = 4;
	ATTR_DESCR *attrs;
	int i = 0;

	attrs = malloc(sizeof(ATTR_DESCR) * nr_attrs);
	for (i = 0; i < nr_attrs; i++) {
		attrs[i].attrName = malloc(MAXNAME);
		sprintf(attrs[i].attrName, "%s%d", "MyAttr", i);
		attrs[i].attrType = INT_TYPE;
		attrs[i].attrLen = 4;
	}
		
	FE_Init();
	
	DBdestroy(DB_NAME);
	DBcreate(DB_NAME);
	printf("Create Databse\n");

	DBconnect(DB_NAME);
	printf("Connect Database\n");


	CreateTable("test_table", 4, attrs, NULL);

	PrintTable(REL_CATALOG_NAME);
	PrintTable(ATTR_CATALOG_NAME);
	PrintTable("test_table");


	HelpTable("test_table");

	DestroyTable("test_table");
	PrintTable(REL_CATALOG_NAME);
	PrintTable(ATTR_CATALOG_NAME);

	HelpTable(NULL);

	DBclose(DB_NAME);

	return 0;



}
