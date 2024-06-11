/**************************************************************
* Class: CSC-415-02 Spring 2022
* Names: Olivier Chan Sion Moy
* Student IDs: 913202698
* GitHub Name: oliviercm
* Group Name: Group Eta
* Project: Basic File System
*
* File: freespace.h
*
* Description: Free space management functions
*
**************************************************************/

#ifndef _FREESPACE_H
#define _FREESPACE_H

extern unsigned char *freeSpaceBitmap;

/**
 * @brief Initializes free space bitmap, using the second block (after the VCB) and onwards. This should only be called if the volume is unformatted!
 * The initialized free space bitmap should be written directly after the VCB on volume (not doing so will result in unexpected behavior).
 */
int initFreeSpaceBitmap();

/**
 * @brief Loads free space bitmap from volume. This should only be called if the volume is formatted!
 */
void readFreeSpaceBitmap();

/**
 * @brief Writes free space bitmap to volume. This should only be called AFTER calling initFreeSpaceBitmap or readFreeSpaceBitmap.
 */
void writeFreeSpaceBitmap();

/**
 * @brief Returns the first block address of a contiguous sequence of free blocks with a minimum length of the passed amount of requested blocks.
 * Also updates the free space bitmap by setting the appropriate bits as used.
 *
 * @param blocks The amount of blocks to allocate.
 * @return The first block address of a contiguous sequence of free blocks with a minimum length of the passed amount of requested blocks.
 */
int allocateFreeSpace(int blocks);

/**
 * @brief Updates the free space bitmap by clearing the appropriate bits based on parameters.
 *
 * @param start The first block to deallocate.
 * @param blocks The amount of blocks to deallocate starting from start.
 */
void deallocateFreeSpace(int start, int blocks);

/**
 * @brief Frees malloced memory for free space bitmap.
 */
void freeFreeSpaceBitmap();

#endif