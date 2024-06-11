/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: b_io.c
*
* Description: Basic File System - Key File I/O Operations
*
**************************************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "b_io.h"
#include "dir.h"
#include "freespace.h"
#include "fsLow.h"
#include "mfs.h"

#define MAXFCBS 20
#define B_CHUNK_SIZE 512

typedef struct b_fcb
{
    char *filename;
    de_s *fileDe; // file directory entry
    char * buf; // holds the open file buffer
    int bufIndex; // holds the current position in the buffer
    int buflen; // holds how many valid bytes are in the buffer
    int blockOffset;
    int mode; // file access mode
    int cursor; // file cursor
    int size; // current file size
    int writeLastBuffer; // whether to flush the buffer on file close
} b_fcb;

b_fcb fcbArray[MAXFCBS];

int startup = 0; // Indicates that this has not been initialized

// Method to initialize our file system
void b_init()
{
    // init fcbArray to all free
    for (int i = 0; i < MAXFCBS; i++) {
        fcbArray[i].buf = NULL; // indicates a free fcbArray
    }

    startup = 1;
}

// Method to get a free FCB element
b_io_fd b_getFCB()
{
    for (int i = 0; i < MAXFCBS; i++) {
        if (fcbArray[i].buf == NULL) {
            return i; // Not thread safe (But do not worry about it for this assignment)
        }
    }
    return -1; // all in use
}

// Interface to open a buffered file
// Modification of interface for this assignment, flags match the Linux flags for open
// O_RDONLY, O_WRONLY, or O_RDWR
b_io_fd b_open(char * filename, int flags)
{
    printf("opening file\n");
    if (startup == 0) {
        b_init(); // Initialize our system
    }

    b_io_fd returnFd = b_getFCB(); // get our own file descriptor
    if (returnFd == -1) { // check for error - all used FCB's
        printf("error: failed to open file, no more file descriptors available\n");
        return -1;
    }
    printf("got file descriptor %i\n", returnFd);

    de_s *fileDe = getFile(filename, flags & O_CREAT);
    if (fileDe == NULL) {
        printf("error: failed to open file, file %s does not exist\n", filename);
        return -1;
    }
    printf("found file %s\n", filename);
    fileDe->accessedAt = time(NULL);

    fcbArray[returnFd].filename = filename;
    fcbArray[returnFd].fileDe = fileDe;
    fcbArray[returnFd].buf = malloc(B_CHUNK_SIZE);
    fcbArray[returnFd].bufIndex = 0;
    fcbArray[returnFd].buflen = 0;
    fcbArray[returnFd].mode = flags;
    fcbArray[returnFd].blockOffset = 0;
    fcbArray[returnFd].cursor = 0;
    fcbArray[returnFd].size = fileDe->size;
    fcbArray[returnFd].writeLastBuffer = 0;

    printf("returning file descriptor %i\n", returnFd);
    return returnFd; // all set
}

// Interface to seek function
int b_seek(b_io_fd fd, off_t offset, int whence)
{
    if (startup == 0) {
        b_init(); // Initialize our system
    }

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return (off_t) -1; // invalid file descriptor
    }

    if (fcbArray[fd].buf == NULL) {
        return (off_t) -1; // file descriptor not open
    }

    off_t seek = -1;

    switch (whence)
        {
        case SEEK_SET:
            seek = offset;
            break;
        case SEEK_CUR:
            seek = fcbArray[fd].cursor + offset;
            break;
        case SEEK_END:
            seek = fcbArray[fd].size + offset;
            break;
        default:
            return (off_t) -1; // invalid whence
        }

    if (seek < 0) {
        return (off_t) -1; // negative resulting offset
    }

    // set fd cursor
    fcbArray[fd].cursor = seek;
    // return resulting cursor
    return fcbArray[fd].cursor;
}

// Interface to write function
int b_write(b_io_fd fd, char * buffer, int count)
{
    if (startup == 0) {
        b_init(); // Initialize our system
    }

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return -1; // invalid file descriptor
    }

    // check that fd is open
    if (fcbArray[fd].buf == NULL) {
        return -1; // file descriptor not open
    }

    // check for write access
    if (!((fcbArray[fd].mode & O_RDWR) == O_RDWR || (fcbArray[fd].mode & O_WRONLY) == O_WRONLY)) {
        printf("error: no write access\n");
        return -1;
    }

    int bytesCopied = 0;
    while (bytesCopied < count) {
        int currentBlock = fcbArray[fd].cursor / vcb->blockSize;
        int bytesToCopy = B_CHUNK_SIZE - fcbArray[fd].bufIndex;
        if (count - bytesCopied < bytesToCopy) {
            bytesToCopy = count - bytesCopied;
        }
        memcpy(fcbArray[fd].buf + fcbArray[fd].bufIndex, buffer + bytesCopied, bytesToCopy);
        bytesCopied += bytesToCopy;
        fcbArray[fd].bufIndex += bytesToCopy;
        if (fcbArray[fd].bufIndex == vcb->blockSize) {
            // ensure space requirement
            int bytesRequired = fcbArray[fd].cursor + fcbArray[fd].bufIndex;
            int blocksRequired = getBlocksNeededForBytes(bytesRequired);
            int blocksAllocated = getBlocksNeededForBytes(fcbArray[fd].fileDe->size);
            // allocate a new sequence of blocks if needed
            if (blocksRequired > blocksAllocated) {
                deallocateFreeSpace(fcbArray[fd].fileDe->location, blocksAllocated);
                int newLocation = allocateFreeSpace(blocksRequired);
                if (newLocation == -1) {
                    printf("error: not enough space to write file");
                    return -1;
                }
                // copy data from old blocks to new blocks
                char *buf = malloc(vcb->blockSize);
                for (int i = 0; i < blocksAllocated; i++) {
                    LBAread(buf, 1, fcbArray[fd].fileDe->location + i);
                    LBAwrite(buf, 1, newLocation + i);
                }
                free(buf);
                // update file system
                fcbArray[fd].fileDe->location = newLocation;
                fcbArray[fd].fileDe->size = bytesRequired;
            }
            // write buffer
            LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].fileDe->location + currentBlock);
            fcbArray[fd].cursor += B_CHUNK_SIZE;
            fcbArray[fd].size += B_CHUNK_SIZE;
            fcbArray[fd].bufIndex = 0;
        }
    }

    if (fcbArray[fd].bufIndex != 0) {
        fcbArray[fd].writeLastBuffer = 1;
    }

    // Set metadata
    fcbArray[fd].fileDe->modifiedAt = time(NULL);

    return bytesCopied;
}

// Interface to read a buffer

// Filling the callers request is broken into three parts
// Part 1 is what can be filled from the current buffer, which may or may not be enough
// Part 2 is after using what was left in our buffer there is still 1 or more block
//        size chunks needed to fill the callers request.  This represents the number of
//        bytes in multiples of the blocksize.
// Part 3 is a value less than blocksize which is what remains to copy to the callers buffer
//        after fulfilling part 1 and part 2.  This would always be filled from a refill
//        of our buffer.
//  +-------------+------------------------------------------------+--------+
//  |             |                                                |        |
//  | filled from |  filled direct in multiples of the block size  | filled |
//  | existing    |                                                | from   |
//  | buffer      |                                                |refilled|
//  |             |                                                | buffer |
//  |             |                                                |        |
//  | Part1       |  Part 2                                        | Part3  |
//  +-------------+------------------------------------------------+--------+
int b_read(b_io_fd fd, char * buffer, int count)
{
    if (startup == 0) {
        b_init(); // Initialize our system
    }

    // check that fd is between 0 and (MAXFCBS-1)
    if ((fd < 0) || (fd >= MAXFCBS)) {
        return -1; // invalid file descriptor
    }

    // check that fd is open
    if (fcbArray[fd].buf == NULL) {
        return -1; // file descriptor not open
    }

    // check for read access
    if (!((fcbArray[fd].mode & O_RDWR) == O_RDWR || (fcbArray[fd].mode & O_RDONLY) == O_RDONLY)) {
        printf("error: no read access\n");
        return -1;
    }

    // check preconditions
    if (fcbArray[fd].cursor < 0 || fcbArray[fd].cursor >= fcbArray[fd].size) {
        printf("error: file cursor outside valid range\n");
        return -1;
    }

    int read = 0;
    fcbArray[fd].bufIndex = 0;
    // read the first buffer
    fcbArray[fd].blockOffset = fcbArray[fd].cursor / vcb->blockSize;
    LBAread(fcbArray[fd].buf, 1, fcbArray[fd].fileDe->location + fcbArray[fd].blockOffset);
    // ensure we don't overrun the actual file, and that we only read up to count bytes
    while (fcbArray[fd].cursor < fcbArray[fd].size && read < count) {
        // update buffer with correct block if cursor does not match
        if (fcbArray[fd].blockOffset != fcbArray[fd].cursor / vcb->blockSize) {
            fcbArray[fd].blockOffset = fcbArray[fd].cursor / vcb->blockSize;
            LBAread(fcbArray[fd].buf, 1, fcbArray[fd].fileDe->location + fcbArray[fd].blockOffset);
        }
        // copy byte into caller buffer
        buffer[read] = fcbArray[fd].buf[fcbArray[fd].cursor % B_CHUNK_SIZE];
        fcbArray[fd].cursor++;
        read++;
    }

    return read;
}

// Interface to close the file
void b_close(b_io_fd fd)
{
    // check that fd is open
    if (fcbArray[fd].buf == NULL) {
        return; // file descriptor not open
    }

    // write remaining buffer
    if (fcbArray[fd].writeLastBuffer) {
        int currentBlock = fcbArray[fd].cursor / vcb->blockSize;
        // ensure space requirement
        int bytesRequired = fcbArray[fd].cursor + fcbArray[fd].bufIndex;
        int blocksRequired = getBlocksNeededForBytes(bytesRequired);
        int blocksAllocated = getBlocksNeededForBytes(fcbArray[fd].fileDe->size);
        // allocate a new sequence of blocks if needed
        if (blocksRequired > blocksAllocated) {
            deallocateFreeSpace(fcbArray[fd].fileDe->location, blocksAllocated);
            int newLocation = allocateFreeSpace(blocksRequired);
            if (newLocation == -1) {
                printf("error: not enough space to write file");
                return;
            }
            // copy data from old blocks to new blocks
            char *buf = malloc(vcb->blockSize);
            for (int i = 0; i < blocksAllocated; i++) {
                LBAread(buf, 1, fcbArray[fd].fileDe->location + i);
                LBAwrite(buf, 1, newLocation + i);
            }
            free(buf);
            // update file system
            fcbArray[fd].fileDe->location = newLocation;
            fcbArray[fd].fileDe->size = bytesRequired;
        }
        // write buffer
        LBAwrite(fcbArray[fd].buf, 1, fcbArray[fd].fileDe->location + currentBlock);
        fcbArray[fd].writeLastBuffer = 0;
        fcbArray[fd].cursor += fcbArray[fd].bufIndex;
        fcbArray[fd].size += fcbArray[fd].bufIndex;
        fcbArray[fd].bufIndex = 0;
        // update file system
        fcbArray[fd].fileDe->size = fcbArray[fd].size;
        fcbArray[fd].fileDe->modifiedAt = time(NULL);
        updateFile(fcbArray[fd].filename, fcbArray[fd].fileDe);
    }

    printf("closing file at block %i\n", fcbArray[fd].fileDe->location);

    free(fcbArray[fd].fileDe);
    fcbArray[fd].fileDe = NULL;
    free(fcbArray[fd].buf);
    fcbArray[fd].buf = NULL;
}