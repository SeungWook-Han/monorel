#ifndef __PF_H__
#define __PF_H__

/****************************************************************************
 * pf.h: external interface definition for the PF layer
 ****************************************************************************/

 /*
 * size of open file table
 */
#define PF_FTAB_SIZE	20

typedef struct PFhdr_str {
	unsigned int numpages;
	unsigned int offset;
}PFhdr_str;

typedef struct PFftab_ele {
	bool_t	 	valid;		// TRUE: opened FALSE: closed
	unsigned int	inode;
	char		*fname;
	unsigned int	unixfd;
	PFhdr_str	hdr;
	bool_t		hdrchanged;	//dirty
} PFftable_ele;


/*
 * prototypes for PF-layer functions
 */
void PF_Init		(void);
int  PF_CreateFile	(char *filename);
int  PF_DestroyFile	(char *filename);
int  PF_OpenFile	(char *filename);
int  PF_CloseFile	(int fd);
int  PF_AllocPage	(int fd, int *pagenum, char **pagebuf);
int  PF_GetFirstPage	(int fd, int *pagenum, char **pagebuf);
int  PF_GetNextPage	(int fd, int *pagenum, char **pagebuf);
int  PF_GetThisPage	(int fd, int pagenum, char **pagebuf);
int  PF_DirtyPage	(int fd, int pagenum);
int  PF_UnpinPage	(int fd, int pagenum, int dirty);

/*
 * PF-layer error codes
 */
#define PFE_OK			0
#define PFE_INVALIDPAGE		(-1)
#define PFE_PAGEFREE		(-2)
#define	PFE_FTABFULL		(-3)
#define PFE_FD			(-4)
#define PFE_EOF			(-5)
#define PFE_FILEOPEN		(-6)
#define PFE_FILENOTOPEN		(-7)
#define PFE_NOUSERS		(-8)
#define PFE_FILENOTEXIST	(-9)
#define PFE_FILEEXIST		(-10)
#define PFE_FILEIO		(-11)
#define PFE_CLOSE		(-12)
#define PFE_ALLOC		(-13)
#define PFE_GETTHIS		(-14)
#define PFE_GITNEXT		(-15)
#define PFE_GETFIRST		(-16)
#define PFE_GETDIRTY		(-17)
#define PFE_UNPINPAGE		(-18)

/*
 * error in UNIX system call or library routine
 */
#define PFE_HDRREAD		(-97)
#define PFE_HDRWRITE		(-98)
#define PFE_MSGERR              (-99)
#define PFE_UNIX		(-100)

/*
 * most recent PF error code
 */
extern int PFerrno;

#endif
