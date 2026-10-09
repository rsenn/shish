/* this header file comes from libowfat, http://www.fefe.de/libowfat/ */
#ifndef MMAP_H
#define MMAP_H

#include "typedefs.h"
#include "uint64.h"

/* map fd for reading
 * return pointer to map */
char* mmap_read_fd(fd_t fd, size_t* filesize);

/* open file for reading, mmap whole file, close file, write length of
 * map in filesize and return pointer to map. */
const char* mmap_read(const char* filename, size_t* filesize);

/* private anonymous zero-filled read/write map of len bytes, 0 on failure
 * (release with mmap_unmap()) */
char* mmap_anon(size_t len);

/* unmap a mapped region */
int mmap_unmap(const char* mapped, size_t maplen);

#ifdef STRALLOC_H
int mmap_filename(void* map, stralloc* sa);
#endif

#endif
