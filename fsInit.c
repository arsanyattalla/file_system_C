/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: fsInit.c
*
* Description: Main driver for file system assignment.
*
* This file is where you will start and initialize your system
*
**************************************************************/

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>

#include "vcb_s.h"
#include "de_s.h"

#include "fsLow.h"
#include "mfs.h"
#include "freespace.h"
#include "dir.h"

// Define globals
vcb_s *vcb;
int currentWorkingDirectoryLBA;

int initFileSystem(uint64_t numberOfBlocks, uint64_t blockSize)
{
    // Step 1: Determine whether the volume needs to be formatted or not.

    // Malloc space for VCB struct and LBAread block 0 (the VCB) into malloced space.
    vcb = malloc(blockSize);
    LBAread(vcb, 1, 0);

    // Look at VCB signature and check if it matches
    if (vcb->signature != VCB_MAGIC_NUMBER) { // If signature does not match, volume is uninitialized/unformatted.
        // Initialize/format the volume.
        printf("valid volume control block not found. initializing volume with %ld blocks with a block size of %ld...\n", numberOfBlocks, blockSize);

        // Initialize values in volume control block.
        vcb->signature = VCB_MAGIC_NUMBER;
        vcb->blockCount = numberOfBlocks;
        vcb->blockSize = blockSize;

        // Initialize free space bitmap.
        initFreeSpaceBitmap();
        writeFreeSpaceBitmap();

        // Set VCB root directory location
        vcb->rootLBA = createDir(NULL);
        if (vcb->rootLBA == -1) {
            printf("error: not enough free blocks to create root directory\n");
            free(vcb);
            return 1;
        }

        // Finished formatting volume, write VCB
        LBAwrite(vcb, 1, 0);
    } else {
        printf("valid volume control block found.\n");
        // Load free space bitmap from volume.
        readFreeSpaceBitmap();
    }

    currentWorkingDirectoryLBA = vcb->rootLBA;

    return 0;
}

void exitFileSystem()
{
    // Clean up
    free(vcb);
    vcb = NULL;
    freeFreeSpaceBitmap();
    printf("exiting file system.\n");
}