/* 
 * If the program is given no command-line arguments, it runs all tests.
 * You can run only some of the tests by giving the numbers of the tests
 * you want to run as command-line arguments.  They can appear in any order.
 * Notice that the clean up function is the last one, and it has associated
 * to it the number (number of tests + 1).
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "minirel.h"
#include "pf.h"
#include "hf.h"
#include "am.h"

#define FILE1       "testrel"
#define STRSIZE     32
#define TOTALTESTS  3

/****************/
/* main program */
/****************/

int main(int argc, char *argv[])
{
int value,id, am_fd, hf_fd;
   char string_val[STRSIZE];
   char files_to_delete[80];
   RECID recid;
   int sd;

   char comp_value[STRSIZE];
   char retrieved_value[STRSIZE];

   AM_Init();
   /* To avoid uninitialized bytes error reported by valgrind */
   /* Bongki, Mar/13/2011 */
   memset(string_val,'\0',STRSIZE);

   printf("***** start amtest1 *****\n");
   /* making sure FILE1 index is not present */
   sprintf(files_to_delete, "rm -f %s*", FILE1);
   system(files_to_delete);

   if (AM_CreateIndex(FILE1, 1, STRING_TYPE, STRSIZE, FALSE) != AME_OK) {
      AM_PrintError("Problem creating");
      exit(1);
   }
   if ((am_fd = AM_OpenIndex(FILE1,1)) < 0) {
      AM_PrintError("Problem opening");
      exit(1);
   }

   /* Inserting value in the HF file and the B+ Tree */
      value = 10;
      while (value < 100)
      {
         sprintf(string_val, "entry%d", value);
		 recid.pagenum = value;
		 recid.recnum = value;
         /* Notice the recid value being inserted is trash.    */
         /* The (char *)& is unnecessary in AM_InsertEntry     */
         /* because in this case string_val is (char *)        */
         /* but I am putting it here to emphasize what to pass */
         /* in other cases (int and float value                */
  
         /* Inserting the record in the B+ Tree */
         if (AM_InsertEntry(am_fd, (char *)&string_val, recid) != AME_OK) {
             AM_PrintError("Problem Inserting rec");
             exit(1);
         }
          value = value + 2;
      }

   

	printTree(am_fd);
   if (AM_CloseIndex(am_fd) != AME_OK) {
      AM_PrintError("Problem Closing");
      exit(1);
   }


   printf("***** end amtest1 *****\n");

 if ((am_fd = AM_OpenIndex(FILE1,1)) < 0) {
      AM_PrintError("Problem opening index");
      exit(1);
   }

   /* Initializing the compare value */
   memset(comp_value, ' ', STRSIZE);
   sprintf(comp_value, "entry50");

   /* Opening the index scan */
   if ((sd = AM_OpenIndexScan(am_fd, GT_OP, comp_value)) < 0) {
      AM_PrintError("Problem opening index scan");
      exit(1);
   }

   while (1)
   {
	      /* clearing retrieved_value  */
      memset(retrieved_value, ' ', STRSIZE);
      recid = AM_FindNextEntry(sd);
	      printf("the retrieved value is %d\n", recid.pagenum);
         if (AMerrno == AME_EOF) break; /*Out of records satisfying predicate */
}

   /* closing scan, index file, and HF file */
   if (AM_CloseIndexScan(sd) != AME_OK) {
      AM_PrintError("Problem closing index scan");
      exit(1);
   }

   if (AM_CloseIndex(am_fd) != AME_OK) {
      AM_PrintError("Problem closing index file");
      exit(1);
   }
   printf("***** end amtest2 *****\n");

   printf("***** Start amtest3 *****\n");

   if ((am_fd = AM_OpenIndex(FILE1,1)) < 0) {
      AM_PrintError("Problem opening index");
      exit(1);
   }

   /* Initializing the compare value */
   memset(comp_value, ' ', STRSIZE);
   sprintf(comp_value, "entry50");

   /* Opening the index scan */
   if ((sd = AM_OpenIndexScan(am_fd, GT_OP, comp_value)) < 0) {
      AM_PrintError("Problem opening index scan");
      exit(1);
   }

   /* Pulling recid using the B+ Tree and printing the record */
   /* by using the retrieved recid and HF_GetThisRec()        */
	AMerrno = AME_OK;
value = 52;
   while (1)
   {
      /* clearing retrieved_value  */
      memset(retrieved_value, ' ', STRSIZE);
      recid = AM_FindNextEntry(sd);
      printf("the retrieved value is %d\n", recid.pagenum);
      /* We are doing a deletion during an index scan            */
      /* We are deleting those entries from the B+Tree which     */
      /* are less that "entry70". Notice we are NOT deleting the */
      /* entry from the HF file.                                 */ 
     
    if (AMerrno == AME_EOF) break; /*Out of records satisfying predicate */
	  sprintf(retrieved_value, "%s%d", "entry", value);
      if (strcmp(retrieved_value, "entry70") > 0){
		  printf("try to delete - value:%s, recval:%d\n", retrieved_value, recid.pagenum); 
         if (AM_DeleteEntry(am_fd, retrieved_value, recid) != AME_OK) {
       
			 AM_PrintError("Problem deleting entry");
            exit(1);
         }
         else 
            printf("DELETING entry %s\n", retrieved_value);
      }

	  value = value + 2;

   } /* while end */
	value = 72;

	printTree(am_fd);
	
   /* closing scan, index file, and HF file */
   if (AM_CloseIndexScan(sd) != AME_OK) {
      AM_PrintError("Problem closing index scan");
      exit(1);
   }

   if (AM_CloseIndex(am_fd) != AME_OK) {
      AM_PrintError("Problem closing index file");
      exit(1);
   }

   printf("***** end amtest3 *****\n"); 

}

