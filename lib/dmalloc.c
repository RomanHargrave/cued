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

#ifdef DEBUG_MALLOC

#include "dlist.h"
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>


// if performance is impacted too much, increase CHECK_ALL_BLOCKS_HZ
#define CHECK_ALL_BLOCKS_HZ    1
//#define CHECK_ALL_BLOCKS_HZ    100000

#define USE_WHOLE_PAGES
#define ARCHAIC_ZEROS


#define dcalloc     calloc
#define dmalloc     malloc
#define drealloc    realloc
#define dfree       free


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

INLINE
void allocAbend(const char *theMessage)
{
    fprintf(stderr, "%s\n", theMessage);
    abort();
}

#ifdef USE_WHOLE_PAGES

#if defined(_WIN32)
#define SMALL_PAGE_ADDRESS_BITS         16
#endif

#if defined(__sparc)
#define SMALL_PAGE_ADDRESS_BITS         13
#endif

#if defined(__unix__) && (defined(__i386__) || defined(__x86_64__))
#define SMALL_PAGE_ADDRESS_BITS         12
#endif

#define SMALL_PAGE_SIZE                 (1 << SMALL_PAGE_ADDRESS_BITS)

/*  This is the new version of coerceBlockSize.  The algorithm's advantage
**  is better performance.  However, the alignment parameter must be
**  a whole power of two.  The author has yet to encounter a platform
**  whose required alignment is not a whole power of two.
*/
INLINE
size_t coerceBlockSize(size_t theBlockSize, size_t theBlockAlignment)
{
    return (theBlockSize + (theBlockAlignment - 1)) & ~(theBlockAlignment - 1);
}

/*  functions to request memory from the operating system
*/

#if defined(_WIN32)

#include <windows.h>

INLINE
ptr_as_int_t allocLargeChunk(size_t theSize)
{
    return (ptr_as_int_t) VirtualAlloc(0, theSize, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
}

INLINE
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

#else

#include <unistd.h>
#include <sys/mman.h>

INLINE
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

INLINE
void freeChunk(ptr_as_int_t theChunk, size_t theSize)
{
    if (munmap((void *) theChunk, theSize))
        allocAbend("munmap");
}

#endif

#endif /* USE_WHOLE_PAGES */


INLINE
void *getBlkPtrFrmDbgHdrPtr(debug_header_t *theBlock)
{
    return (void *) ((ptr_as_int_t) theBlock + sizeof(debug_header_t));
}

INLINE
debug_header_t *getDbgHdrPtrFrmBlkPtr(void *theBlock)
{
    return (debug_header_t *) ((ptr_as_int_t) theBlock - sizeof(debug_header_t));
}

INLINE
debug_trailer_t *getDbgTrlrPtrFrmHdrPtr(debug_header_t *theBlock)
{
    return (debug_trailer_t *) ((ptr_as_int_t) getBlkPtrFrmDbgHdrPtr(theBlock) + theBlock->userLength);
}


static DLIST_DECLARE(itsBlockList)
static unsigned int allocCallCtr;
void checkAllBlocks();


INLINE
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
    if (!((aBlock = (debug_header_t *) amalloc(anAllocBlockSize))))

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
    afree(aHeader);
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


#endif /* DEBUG_MALLOC */
