#include "minirel.h"
#include "pf.h"
#include "hf.h"


void HF_Init(void)
{
	PF_Init();
}

int HF_CreateFile(char *fileName, int recSize)
{
	/* PF_CreateFile for creating heap file
	 * PF_OpenFile for a paged file with storing information into header paged
	 * (How many pages is stored in, record size)
	 */

	int ft_idx = -1;
	char *pagebuf;

	if (PF_CreateFile(filename) != PFE_OK) {
		printf("PF_CreateFile Failed...\n");
		// return error
	}

	if ((ft_idx = PF_OpenFile(filename)) < 0) {
		printf("PF_OpenFile Failed...\n");
		// return error
	}

	/* Updating header page */
	if (PF_GetHeaderPage(ft_idx, &pagebuf) != PFE_OK) {
		printf("PF_GetHeaderPage Failed...\n");
		// return error
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
