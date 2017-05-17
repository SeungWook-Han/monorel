#include "minirel.h"
#include "pf.h"
#include "hf.h"

/* Table getting header info of the files */
struct hf_table_entry *hf_table;

void HF_Init(void)
{
	hf_table = calloc(MAXOPENFILES, sizeof(struct hf_table_entry));
	PF_Init();
}

int HF_CreateFile(char *fileName, int recSize)
{
	int ft_idx = -1;
	char *pagebuf;
	int i = 0;
	struct hf_table_entry *alloc_table_entry = NULL;

	/* If the file is already exist then the PF_CreateFile will return error */
	if (PF_CreateFile(filename) != PFE_OK) {
		printf("PF_CreateFile Failed...\n");
	}

	if ((ft_idx = PF_OpenFile(filename)) < 0) {
		printf("PF_OpenFile Failed...\n");
	}

	/* Updating header page
	if (PF_GetHeaderPage(ft_idx, &pagebuf) != PFE_OK) {
		printf("PF_GetHeaderPage Failed...\n");
	}
	*/

	/* Allocating new hf_table_entry */
	for (i = 0; i < MAXOPENFILES; i++) {
		if (!hf_table[i].valid) {
			alloc_table_entry = &(hf_table[i]);	
			break;
		}
	}

	/* All hf_table_entry is used (i.e. Trying to open file more than maximum) */
	if (alloc_table_entry == NULL) {
		printf("HF_CreateFile: files open exceeds MAXOPENFILES\n");
		return HFE_FTABFULL;
	}

}

int HF_DestroyFile(char *fileName)
{
	if (PF_DestroyFile(fileName) != PFE_OK) {
		return HFE_FILE_DESTROY;
	}
	return HFE_OK;
}

int HF_OpenFile(char *fileName)
{
	/* Open the file and create file table entry using PF_OpenFile */
}

int HF_CloseFile(int HFfd)
{
	/* Close the file using PF_CloseFile.
	 * If there is active scan no further action is done. */
}
