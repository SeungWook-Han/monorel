#ifndef __BITMAP_H__
#define __BITMAP_H__

int  bitmap_set(int *bitmap, int bitmap_size, int digit);
int  bitmap_get(int *bitmap, int bitmap_size, int digit);
int  bitmap_clear(int *bitmap, int bitmap_size, int digit);
int  bitmap_print(int *bitmap, int bitmap_size);
#endif
