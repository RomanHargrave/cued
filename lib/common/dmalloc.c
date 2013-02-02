/*
** dmalloc.c
**
** Copyright (C) 1996-2008 Robert William Fuller <hydrologiccycle@gmail.com>
**
** This program is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation, either version 3 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifdef HAVE_CONFIG_H
#include "cued_config.h" // CUED_OVERLOAD_LIBC
#endif

// allow for insanities such as -D'malloc(x)=dmalloc(x)' -D'calloc(x, y)=dcalloc(x, y)' -D'free(x)=dfree(x)' -D'realloc(x, y)=drealloc(x, y)'
#ifdef malloc
#undef malloc
#endif
#ifdef free
#undef free
#endif

#include "dmalloc.h"
#include "dlist.h"

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


// if performance is impacted too much, increase CHECK_ALL_BLOCKS_HZ
#define CHECK_ALL_BLOCKS_HZ    1
//#define CHECK_ALL_BLOCKS_HZ    100000

#define ARCHAIC_ZEROS

#ifdef CUED_OVERLOAD_LIBC
#define dcalloc     calloc
#define dmalloc     malloc
#define drealloc    realloc
#define dfree       free
#endif

#ifdef __cplusplus
extern "C" {
    void *dmalloc(size_t theBlockSize);
    void *dcalloc(size_t theNumElems, size_t theElemSize);
    void *drealloc(void *theBlock, size_t theNewUserBlockSize);
    void  dfree(void *theBlock);
}
#endif


#define HEADER_SIGNATURE_SIZE   64
#define TRAILER_SIGNATURE_SIZE  64

#define HEADER_SIGNATURE    0x55
#define TRAILER_SIGNATURE   0xAA

#define ALLOC_VALUE         0xFF
#define FREE_VALUE          0xDD

typedef struct _debug_header_t
{
    size_t userLength;
    size_t allocLength;
    d_list_node_t listNode;
    char signature[HEADER_SIGNATURE_SIZE];

} debug_header_t;

typedef struct _debug_trailer_t
{
    char signature[TRAILER_SIGNATURE_SIZE];

} debug_trailer_t;


typedef size_t ptr_as_int_t;


/*  utility functions
*/

static inline
void allocAbend(const char *theMessage)
{
    fprintf(stderr, "%s\n", theMessage);
    abort();
}


#ifdef USE_WHOLE_PAGES

/*  This is the new version of coerceBlockSize.  The algorithm's advantage
**  is better performance.  However, the alignment parameter must be
**  a whole power of two.  The author has yet to encounter a platform
**  whose required alignment is not a whole power of two.
*/
static inline
size_t coerceBlockSize(size_t theBlockSize, size_t theBlockAlignment)
{
    return (theBlockSize + (theBlockAlignment - 1)) & ~(theBlockAlignment - 1);
}

/*  functions to request memory from the operating system
*/

#if defined(_WIN32)

#define SMALL_PAGE_ADDRESS_BITS         16
#define SMALL_PAGE_SIZE                 (1 << SMALL_PAGE_ADDRESS_BITS)

#include <windows.h>

static inline
ptr_as_int_t allocLargeChunk(size_t theSize)
{
    return (ptr_as_int_t) VirtualAlloc(0, theSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

static inline
void freeChunk(ptr_as_int_t theChunk, size_t theSize)
{
    /*  TODO:  VirtualAlloc retains the size of the region created;
    **  hence, size is ignored by VirtualFree;  this could be corrected
    **  by using MEM_DECOMMIT in conjunction with an address reservation
    **  scheme to solve the address space inefficiency problem discussed
    **  in the other TODO for NT (see amalloc.c)
    */
    if (!VirtualFree((void *) theChunk, 0, MEM_RELEASE))
        allocAbend("VirtualFree");
}

#else // Unixen

#include <sys/mman.h> // mmap, munmap
#include <limits.h> // PAGESIZE, PAGE_SIZE
#include <unistd.h> // sysconf

#ifdef PAGESIZE
#define SMALL_PAGE_SIZE PAGESIZE
#elif defined(PAGE_SIZE)
#define SMALL_PAGE_SIZE PAGE_SIZE
#else
#define SMALL_PAGE_SIZE sysconf(_SC_PAGESIZE)
#endif

static inline
ptr_as_int_t allocLargeChunk(size_t theSize)
{
    ptr_as_int_t aChunk;

    aChunk = (ptr_as_int_t) mmap((void *)
        0, theSize, PROT_READ | PROT_WRITE, MAP_ANON | MAP_PRIVATE,
        -1, 0);

    if ((ptr_as_int_t) MAP_FAILED == aChunk)
        aChunk = 0;

    return aChunk;
}

static inline
void freeChunk(ptr_as_int_t theChunk, size_t theSize)
{
    if (munmap((void *) theChunk, theSize))
        allocAbend("munmap");
}

#endif

#endif /* USE_WHOLE_PAGES */


#if defined(CUED_OVERLOAD_LIBC) && !defined(USE_WHOLE_PAGES)

#ifndef __cplusplus
#define __USE_GNU
#endif
#include <dlfcn.h>

typedef void *(*malloc_fn_t)(size_t);
typedef void (*free_fn_t)(void *);

static void *wrapped_malloc(size_t theUserBlockSize)
{
    static malloc_fn_t malloc_fn;

    if (!malloc_fn)
    {
        malloc_fn = (malloc_fn_t) dlsym(RTLD_NEXT, "malloc");
    }

    return malloc_fn(theUserBlockSize);
}

static void wrapped_free(void *theBlock)
{
    static free_fn_t free_fn;

    if (!free_fn)
    {
        free_fn = (free_fn_t) dlsym(RTLD_NEXT, "free");
    }

    free_fn(theBlock);
}

#elif !defined(USE_WHOLE_PAGES)

#define wrapped_malloc  malloc
#define wrapped_free    free

#endif


// allow for insanities such as -D'malloc(x)=dmalloc(x)' -D'calloc(x, y)=dcalloc(x, y)' -D'free(x)=dfree(x)' -D'realloc(x, y)=drealloc(x, y)'
//

void libc_free(void *theBlock)
{
    free(theBlock);
}

void *libc_malloc(size_t theBlockSize)
{
    return malloc(theBlockSize);
}


static inline
void *getBlkPtrFrmDbgHdrPtr(debug_header_t *theBlock)
{
    return (void *) ((ptr_as_int_t) theBlock + sizeof(debug_header_t));
}

static inline
debug_header_t *getDbgHdrPtrFrmBlkPtr(void *theBlock)
{
    return (debug_header_t *) ((ptr_as_int_t) theBlock - sizeof(debug_header_t));
}

static inline
debug_trailer_t *getDbgTrlrPtrFrmHdrPtr(debug_header_t *theBlock)
{
    return (debug_trailer_t *) ((ptr_as_int_t) getBlkPtrFrmDbgHdrPtr(theBlock) + theBlock->userLength);
}


static DLIST_DECLARE(itsBlockList)
static unsigned int allocCallCtr;
void checkAllBlocks();


static inline
void *dalloc(size_t theUserBlockSize)
{
    debug_header_t *aBlock;
    debug_trailer_t *aTrailer;
    size_t anAllocBlockSize;

    if (!(++allocCallCtr % CHECK_ALL_BLOCKS_HZ))
        checkAllBlocks();

#ifdef USE_WHOLE_PAGES

    anAllocBlockSize = coerceBlockSize(sizeof(debug_header_t) + theUserBlockSize + sizeof(debug_trailer_t), SMALL_PAGE_SIZE);
    if (!((aBlock = (debug_header_t *) allocLargeChunk(anAllocBlockSize))))

#else

    anAllocBlockSize = sizeof(debug_header_t) + theUserBlockSize + sizeof(debug_trailer_t);
    if (!((aBlock = (debug_header_t *) wrapped_malloc(anAllocBlockSize))))

#endif
    {
        /*  Unix98 conformance
        */
        errno = ENOMEM;

        return 0;
    }

    aBlock->userLength = theUserBlockSize;
    aBlock->allocLength = anAllocBlockSize;
    dListInsertHead(&itsBlockList, &aBlock->listNode);

    memset(&aBlock->signature, HEADER_SIGNATURE, HEADER_SIGNATURE_SIZE);
    aTrailer = getDbgTrlrPtrFrmHdrPtr(aBlock);
    memset(&aTrailer->signature, TRAILER_SIGNATURE, TRAILER_SIGNATURE_SIZE);

    return getBlkPtrFrmDbgHdrPtr(aBlock);
}


void *dcalloc(size_t theNumElems, size_t theElemSize)
{
    void *aBlock;
    size_t theUserBlockSize;

    if (!(theUserBlockSize = theNumElems * theElemSize))
#ifdef ARCHAIC_ZEROS
        theUserBlockSize = 1;
#else
        allocAbend("calloc:  block size parameter is zero");
#endif

    aBlock = dalloc(theUserBlockSize);

#ifndef USE_WHOLE_PAGES
    if (aBlock)
        memset(aBlock, 0x00, theUserBlockSize);
#endif

    return aBlock;
}


void *dmalloc(size_t theBlockSize)
{
    void *theBlock = dalloc(theBlockSize);

    if (theBlock)
        memset(theBlock, ALLOC_VALUE, theBlockSize);

    return theBlock;
}


static void checkBlockSignatures(debug_header_t *theHeader)
{
    static char itsHeaderBlock [HEADER_SIGNATURE_SIZE];
    static char itsTrailerBlock[HEADER_SIGNATURE_SIZE];

    debug_trailer_t *theTrailer;

    if (!itsHeaderBlock[0])
    {
        /* used to use dcalloc here, but that was bad because it would result
        ** in an infinite recursion if CHECK_ALL_BLOCKS_HZ was less than 3
        */
        memset(itsHeaderBlock,   HEADER_SIGNATURE,  HEADER_SIGNATURE_SIZE);
        memset(itsTrailerBlock, TRAILER_SIGNATURE, TRAILER_SIGNATURE_SIZE);
    }

    theTrailer = getDbgTrlrPtrFrmHdrPtr(theHeader);
    if (memcmp(itsTrailerBlock, theTrailer, TRAILER_SIGNATURE_SIZE))
        allocAbend("free:  bad trailer signature");
    if (memcmp(itsHeaderBlock, &theHeader->signature, HEADER_SIGNATURE_SIZE))
        allocAbend("free:  bad header signature");
}


void checkAllBlocks()
{
    d_list_node_t *aNode;
    debug_header_t *aBlock;

    //fprintf(stderr, "Checking all blocks.\n");

    for (aNode = itsBlockList.next;  aNode != &itsBlockList;  aNode = aNode->next)
    {
        aBlock = FIELD_TO_STRUCT(aNode, listNode, debug_header_t);
        checkBlockSignatures(aBlock);
    }
}


void dfree(void *theBlock)
{
    debug_header_t *aHeader;

    if (!theBlock)
#ifdef ARCHAIC_ZEROS
        return;
#else
        allocAbend("free:  pointer parameter is null");
#endif

    aHeader = getDbgHdrPtrFrmBlkPtr(theBlock);

    checkBlockSignatures(aHeader);
    dListRemoveNode(&aHeader->listNode);

    if (!(++allocCallCtr % CHECK_ALL_BLOCKS_HZ))
        checkAllBlocks();

#ifdef USE_WHOLE_PAGES
    freeChunk((ptr_as_int_t) aHeader, aHeader->allocLength);
#else
    memset(theBlock, FREE_VALUE, aHeader->userLength);
    wrapped_free(aHeader);
#endif
}


void *drealloc(void *theBlock, size_t theNewUserBlockSize)
{
    void *aNewBlock;
    debug_header_t *aHeader;

    if (!theBlock)
#ifdef ARCHAIC_ZEROS
        return dalloc(theNewUserBlockSize);
#else
        allocAbend("realloc:  pointer parameter is null");
#endif

    if (!theNewUserBlockSize)
#ifdef ARCHAIC_ZEROS
    {
        dfree(theBlock);

        return 0;
    }
#else
    allocAbend("realloc:  block size parameter is zero");
#endif

    if (!(aNewBlock = dalloc(theNewUserBlockSize)))
    {
        /*  Unix98 conformance
        */
        errno = ENOMEM;

        return 0;
    }

    aHeader = getDbgHdrPtrFrmBlkPtr(theBlock);
    if (theNewUserBlockSize > aHeader->userLength)
    {
        memcpy(aNewBlock, theBlock, aHeader->userLength);
        memset((void *) ((ptr_as_int_t) aNewBlock + aHeader->userLength),
            ALLOC_VALUE, theNewUserBlockSize - aHeader->userLength);
    }
    else
    {
        memcpy(aNewBlock, theBlock, theNewUserBlockSize);
    }

    dfree(theBlock);

    return aNewBlock;
}
