/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: freespace.c
*
* Description: Free space management functions
*
**************************************************************/

#include <stdlib.h>

#include "fsLow.h"
#include "bits.h"
#include "mfs.h"

unsigned char *freeSpaceBitmap;
int freeSpaceBitmapSize;
int blocksNeededToStoreBitmap;

int initFreeSpaceBitmap()
{
    // Initialize free space by using a bitmap for free space management.
    // Calculate amount of blocks needed to store free space bitmap. 1 bit is needed for each block on volume.
    int bitsPerBlock = vcb->blockSize * 8;
    blocksNeededToStoreBitmap = vcb->blockCount / bitsPerBlock;
    if (blocksNeededToStoreBitmap * bitsPerBlock < vcb->blockCount) { // Account for truncating from integer division
        blocksNeededToStoreBitmap++;
    }
    // Malloc space for initial free space bitmap.
    freeSpaceBitmapSize = blocksNeededToStoreBitmap * vcb->blockSize;
    freeSpaceBitmap = malloc(freeSpaceBitmapSize);
    // Mark initial blocks as used (VCB block and free space bitmap blocks).
    setBit(freeSpaceBitmap, 0); // Set VCB block as used.
    vcb->freeSpaceLBA = 1;
    for (int i = 0; i < blocksNeededToStoreBitmap; i++) {
        setBit(freeSpaceBitmap, vcb->freeSpaceLBA + i); // Set blocks used to store free space bitmap as used.
    }
    return blocksNeededToStoreBitmap;
}

void readFreeSpaceBitmap()
{
    // Initialize free space by using a bitmap for free space management.
    // Calculate amount of blocks needed to store free space bitmap. 1 bit is needed for each block on volume.
    int bitsPerBlock = vcb->blockSize * 8;
    blocksNeededToStoreBitmap = vcb->blockCount / bitsPerBlock;
    if (blocksNeededToStoreBitmap * bitsPerBlock < vcb->blockCount) { // Account for truncating from integer division
        blocksNeededToStoreBitmap++;
    }
    // Malloc space for initial free space bitmap.
    freeSpaceBitmapSize = blocksNeededToStoreBitmap * vcb->blockSize;
    freeSpaceBitmap = malloc(freeSpaceBitmapSize);
    // Load free space bitmap from volume
    LBAread(freeSpaceBitmap, blocksNeededToStoreBitmap, vcb->freeSpaceLBA);
}

void writeFreeSpaceBitmap()
{
    LBAwrite(freeSpaceBitmap, blocksNeededToStoreBitmap, vcb->freeSpaceLBA);
}

int allocateFreeSpace(int blocks)
{
    int start = 0;
    int freeBlocks = 0;
    // Find a contiguous sequence of free blocks large enough for requested blocks
    for (int i = 0; i < freeSpaceBitmapSize; i++) {
        if (!testBit(freeSpaceBitmap, i)) {
            freeBlocks++;
        } else {
            start = i + 1;
            freeBlocks = 0;
        }
        // If a large enough sequence is found, set blocks as used and return sequence start position
        if (freeBlocks >= blocks) {
            for (int j = 0; j < blocks; j++) {
                setBit(freeSpaceBitmap, start + j);
            }
            writeFreeSpaceBitmap();
            return start;
        }
    }
    return -1;
}

void deallocateFreeSpace(int start, int blocks)
{
    for (int i = 0; i < blocks; i++) {
        clearBit(freeSpaceBitmap, start + i);
    }
    writeFreeSpaceBitmap();
}

void freeFreeSpaceBitmap()
{
    free(freeSpaceBitmap);
    freeSpaceBitmap = NULL;
    freeSpaceBitmapSize = 0;
    blocksNeededToStoreBitmap = 0;
}